// combo/gui/ComboSettingsSync.cpp — see ComboSettingsSync.h for rationale.
#include "ComboSettingsSync.h"
#include "ComboMenuModel.h"
#include "ComboWidgetStyle.h"
#include <libultraship/libultraship.h>
#include <ship/Context.h>
#include <ship/window/Window.h>
#include <ship/window/gui/GuiWindow.h>
#include <imgui.h>
#include <algorithm>
#include <memory>
#include <string>

namespace {

using ComboRando::ComboMenu_PopCheckbox;
using ComboRando::ComboMenu_PushCheckbox;
using ComboRando::ComboMenu_ThemeColor;

constexpr const char* kCvarEnabled = "gCombo.Sync.Enabled";

enum SyncType { SYNC_INT, SYNC_FLOAT, SYNC_TEXT_SPEED };

enum SyncGroup {
    GROUP_CONTROLS,
    GROUP_CAMERA,
    GROUP_CHEATS,
    GROUP_TIMESAVERS,
    GROUP_INTERFACE,
    GROUP_GAMEPLAY,
    GROUP_AUDIO,
    GROUP_RANDOMIZER,
    GROUP_COUNT
};

struct GroupInfo {
    const char* cvar;
    const char* label;
    const char* blurb;
};

const GroupInfo kGroups[GROUP_COUNT] = {
    { "gCombo.Sync.Controls", "Controls & items", "D-pad equips, ocarina controls, item handling." },
    { "gCombo.Sync.Camera", "Camera & aiming", "First-person and free-look, both games." },
    { "gCombo.Sync.Cheats", "Cheats", "Infinite ammo, money, hookshot anywhere..." },
    { "gCombo.Sync.Timesavers", "Time savers", "Cutscene skips, fast chests, text speed." },
    { "gCombo.Sync.Interface", "Interface & graphics", "Notifications, draw distance, health bars." },
    { "gCombo.Sync.Gameplay", "Gameplay & saving", "Difficulty toggles, autosave, input buffer." },
    { "gCombo.Sync.Audio", "Audio", "Companion call mute. Volumes already mirror through the audio bridge." },
    { "gCombo.Sync.Randomizer", "Randomizer", "Shuffle options both generators understand the same way." },
};

// One player-facing setting that the two games spell differently. mmMin/mmMax clamp the value on its
// way INTO MM when MM's widget accepts a narrower range than OOT's; equal bounds mean no clamp.
struct SyncPair {
    const char* mm;
    const char* oot;
    SyncType type;
    SyncGroup group;
    float mmMin;
    float mmMax;
};

// Only 1:1 pairs. Deliberately absent, because a numeric copy would corrupt them: ClimbSpeed
// (MM multiplies 1-5, OOT adds 0-12), DamageMultiplier and MirroredWorld (enum lists overlap only
// in part), CrouchStab (inverted polarity, 1:2), SkipToFileSelect (bool vs a BootSequence enum),
// SkipGetItemCutscenes (4 values vs 3), SkipEnemyCutscenes (1 MM bool vs 2 OOT bools), and
// AlternateAssets (per-game archives, split on purpose).
const SyncPair kPairs[] = {
    { "gEnhancements.Dpad.DpadEquips", "gEnhancements.DpadEquips", SYNC_INT, GROUP_CONTROLS, 0, 0 },
    { "gEnhancements.Equipment.ItemUnequip", "gEnhancements.ItemUnequip", SYNC_INT, GROUP_CONTROLS, 0, 0 },
    { "gEnhancements.Player.InstantPutaway", "gEnhancements.InstantPutaway", SYNC_INT, GROUP_CONTROLS, 0, 0 },
    { "gEnhancements.Player.UnsheatheWithoutSlashing", "gEnhancements.UnsheatheWithoutSlashing", SYNC_INT,
      GROUP_CONTROLS, 0, 0 },
    { "gEnhancements.Playback.CustomizeOcarinaControls", "gSettings.CustomOcarina.Enabled", SYNC_INT, GROUP_CONTROLS, 0,
      0 },
    { "gEnhancements.Playback.DpadOcarina", "gSettings.CustomOcarina.Dpad", SYNC_INT, GROUP_CONTROLS, 0, 0 },
    { "gEnhancements.Playback.RightStickOcarina", "gSettings.CustomOcarina.RightStick", SYNC_INT, GROUP_CONTROLS, 0,
      0 },
    { "gEnhancements.Playback.NoDropOcarinaInput", "gEnhancements.DpadNoDropOcarinaInput", SYNC_INT, GROUP_CONTROLS, 0,
      0 },
    { "gEnhancements.Songs.FasterSongPlayback", "gEnhancements.FastOcarinaPlayback", SYNC_INT, GROUP_CONTROLS, 0, 0 },
    { "gEnhancements.PlayerActions.ArrowCycle", "gEnhancements.BowArrowCycle", SYNC_INT, GROUP_CONTROLS, 0, 0 },
    { "gEnhancements.PlayerActions.RemoteBombchu", "gEnhancements.RemoteBombchu", SYNC_INT, GROUP_CONTROLS, 0, 0 },
    { "gEnhancements.Items.RemoveExplosiveLimit", "gEnhancements.RemoveExplosiveLimit", SYNC_INT, GROUP_CONTROLS, 0,
      0 },

    { "gEnhancements.Camera.FirstPerson.InvertX", "gSettings.Controls.InvertAimingXAxis", SYNC_INT, GROUP_CAMERA, 0,
      0 },
    { "gEnhancements.Camera.FirstPerson.InvertY", "gSettings.Controls.InvertAimingYAxis", SYNC_INT, GROUP_CAMERA, 0,
      0 },
    { "gEnhancements.Camera.FirstPerson.RightStickEnabled", "gSettings.Controls.RightStickAim", SYNC_INT, GROUP_CAMERA,
      0, 0 },
    { "gEnhancements.Camera.FirstPerson.MoveInFirstPerson", "gSettings.MoveInFirstPerson", SYNC_INT, GROUP_CAMERA, 0,
      0 },
    { "gEnhancements.Camera.FirstPerson.DisableFirstPersonAutoCenterView", "gSettings.DisableFirstPersonAutoCenterView",
      SYNC_INT, GROUP_CAMERA, 0, 0 },
    // OOT's first-person sensitivity slider reaches 5x, MM's stops at 2x.
    { "gEnhancements.Camera.FirstPerson.SensitivityX", "gSettings.FirstPersonCameraSensitivity.X", SYNC_FLOAT,
      GROUP_CAMERA, 0.01f, 2.0f },
    { "gEnhancements.Camera.FirstPerson.SensitivityY", "gSettings.FirstPersonCameraSensitivity.Y", SYNC_FLOAT,
      GROUP_CAMERA, 0.01f, 2.0f },
    { "gEnhancements.Camera.FreeLook.Enable", "gSettings.FreeLook.Enabled", SYNC_INT, GROUP_CAMERA, 0, 0 },
    { "gEnhancements.Camera.RightStick.InvertXAxis", "gSettings.FreeLook.InvertXAxis", SYNC_INT, GROUP_CAMERA, 0, 0 },
    { "gEnhancements.Camera.RightStick.InvertYAxis", "gSettings.FreeLook.InvertYAxis", SYNC_INT, GROUP_CAMERA, 0, 0 },
    { "gEnhancements.Camera.RightStick.CameraSensitivity.X", "gSettings.FreeLook.CameraSensitivity.X", SYNC_FLOAT,
      GROUP_CAMERA, 0.01f, 5.0f },
    { "gEnhancements.Camera.RightStick.CameraSensitivity.Y", "gSettings.FreeLook.CameraSensitivity.Y", SYNC_FLOAT,
      GROUP_CAMERA, 0.01f, 5.0f },
    { "gEnhancements.Camera.FreeLook.MaxCameraDistance", "gSettings.FreeLook.MaxCameraDistance", SYNC_INT, GROUP_CAMERA,
      100, 900 },
    { "gEnhancements.Camera.FreeLook.TransitionSpeed", "gSettings.FreeLook.TransitionSpeed", SYNC_INT, GROUP_CAMERA, 1,
      900 },

    { "gCheats.InfiniteRupees", "gCheats.InfiniteMoney", SYNC_INT, GROUP_CHEATS, 0, 0 },
    { "gCheats.InfiniteConsumables", "gCheats.InfiniteAmmo", SYNC_INT, GROUP_CHEATS, 0, 0 },
    { "gCheats.InfiniteEponaCarrots", "gCheats.InfiniteEponaBoost", SYNC_INT, GROUP_CHEATS, 0, 0 },
    { "gCheats.UnrestrictedItems", "gCheats.NoRestrictItems", SYNC_INT, GROUP_CHEATS, 0, 0 },
    { "gCheats.HookshotAnywhere", "gCheats.HookshotEverything", SYNC_INT, GROUP_CHEATS, 0, 0 },
    { "gCheats.ClimbAnywhere", "gCheats.ClimbEverything", SYNC_INT, GROUP_CHEATS, 0, 0 },

    { "gEnhancements.Cutscenes.SkipIntroSequence", "gEnhancements.TimeSavers.SkipCutscene.Intro", SYNC_INT,
      GROUP_TIMESAVERS, 0, 0 },
    { "gEnhancements.Cutscenes.SkipEntranceCutscenes", "gEnhancements.TimeSavers.SkipCutscene.Entrances", SYNC_INT,
      GROUP_TIMESAVERS, 0, 0 },
    { "gEnhancements.Cutscenes.SkipStoryCutscenes", "gEnhancements.TimeSavers.SkipCutscene.Story", SYNC_INT,
      GROUP_TIMESAVERS, 0, 0 },
    { "gEnhancements.Cutscenes.SkipOnePointCutscenes", "gEnhancements.TimeSavers.SkipCutscene.OnePoint", SYNC_INT,
      GROUP_TIMESAVERS, 0, 0 },
    { "gEnhancements.Cutscenes.SkipMiscInteractions", "gEnhancements.TimeSavers.SkipMiscInteractions", SYNC_INT,
      GROUP_TIMESAVERS, 0, 0 },
    { "gEnhancements.Cutscenes.HideTitleCards", "gEnhancements.TimeSavers.DisableTitleCard", SYNC_INT, GROUP_TIMESAVERS,
      0, 0 },
    { "gEnhancements.Timesavers.FastChests", "gEnhancements.FastChests", SYNC_INT, GROUP_TIMESAVERS, 0, 0 },
    { "gEnhancements.Timesavers.FasterRupeeAccumulator", "gEnhancements.FasterRupeeAccumulator", SYNC_INT,
      GROUP_TIMESAVERS, 0, 0 },
    { "gEnhancements.Playback.SkipScarecrowSong", "gEnhancements.InstantScarecrow", SYNC_INT, GROUP_TIMESAVERS, 0, 0 },
    { "gEnhancements.Dialogue.SkipBottlePickupMessages", "gEnhancements.FastBottles", SYNC_INT, GROUP_TIMESAVERS, 0,
      0 },
    { "gEnhancements.Dialogue.FastText", "gEnhancements.TextSpeed", SYNC_TEXT_SPEED, GROUP_TIMESAVERS, 0, 0 },

    { "gNotifications.Position", "gSettings.Notifications.Position", SYNC_INT, GROUP_INTERFACE, 0, 0 },
    { "gNotifications.Duration", "gSettings.Notifications.Duration", SYNC_FLOAT, GROUP_INTERFACE, 3.0f, 30.0f },
    { "gNotifications.BgOpacity", "gSettings.Notifications.BgOpacity", SYNC_FLOAT, GROUP_INTERFACE, 0.0f, 1.0f },
    { "gNotifications.Size", "gSettings.Notifications.Size", SYNC_FLOAT, GROUP_INTERFACE, 1.0f, 5.0f },
    { "gEnhancements.Graphics.IncreaseActorDrawDistance", "gEnhancements.DisableDrawDistance", SYNC_INT,
      GROUP_INTERFACE, 1, 5 },
    { "gEnhancements.Graphics.ActorCullingAccountsForWidescreen", "gEnhancements.WidescreenActorCulling", SYNC_INT,
      GROUP_INTERFACE, 0, 0 },
    { "gEnhancements.Graphics.DisableBlackBars", "gEnhancements.DisableBlackBars", SYNC_INT, GROUP_INTERFACE, 0, 0 },
    { "gEnhancements.Graphics.EnemyHealthBars", "gEnhancements.EnemyHealthBar", SYNC_INT, GROUP_INTERFACE, 0, 0 },
    { "gEnhancements.Graphics.3DItemDrops", "gEnhancements.NewDrops", SYNC_INT, GROUP_INTERFACE, 0, 0 },
    { "gEnhancements.Graphics.BowReticle", "gEnhancements.BowReticle", SYNC_INT, GROUP_INTERFACE, 0, 0 },
    { "gEnhancements.Mods.DisableBombBillboarding", "gEnhancements.DisableBombBillboarding", SYNC_INT, GROUP_INTERFACE,
      0, 0 },
    { "gEnhancements.Mods.DisableGrottoRotation", "gEnhancements.DisableGrottoRotation", SYNC_INT, GROUP_INTERFACE, 0,
      0 },
    { "gEnhancements.Mods.AlternateAssetsHotkey", "gSettings.Mods.AlternateAssetsHotkey", SYNC_INT, GROUP_INTERFACE, 0,
      0 },
    { "gEnhancements.A11y.NoScreenFlashForEnemyKill", "gSettings.A11yNoScreenFlashForFinishingBlow", SYNC_INT,
      GROUP_INTERFACE, 0, 0 },

    { "gAudioEditor.DisableTatlCallAudio", "gAudioEditor.DisableNaviCallAudio", SYNC_INT, GROUP_AUDIO, 0, 0 },

    { "gEnhancements.DifficultyOptions.PermanentHeartLoss", "gEnhancements.PermanentHeartLoss", SYNC_INT,
      GROUP_GAMEPLAY, 0, 0 },
    { "gEnhancements.DifficultyOptions.DeleteFileOnDeath", "gEnhancements.DeleteFileOnDeath", SYNC_INT, GROUP_GAMEPLAY,
      0, 0 },
    { "gEnhancements.DifficultyOptions.NoHeartDrops", "gEnhancements.NoHeartDrops", SYNC_INT, GROUP_GAMEPLAY, 0, 0 },
    { "gEnhancements.DifficultyOptions.NoRandomDrops", "gEnhancements.NoRandomDrops", SYNC_INT, GROUP_GAMEPLAY, 0, 0 },
    { "gEnhancements.DifficultyOptions.HyperEnemies", "gEnhancements.HyperEnemies", SYNC_INT, GROUP_GAMEPLAY, 0, 0 },
    { "gEnhancements.Restorations.PauseBufferWindow", "gEnhancements.PauseBufferWindow", SYNC_INT, GROUP_GAMEPLAY, 0,
      40 },
    { "gEnhancements.Saving.RememberSaveLocation", "gEnhancements.RememberSaveLocation", SYNC_INT, GROUP_GAMEPLAY, 0,
      0 },
    { "gEnhancements.Saving.Autosave", "gEnhancements.Autosave", SYNC_INT, GROUP_GAMEPLAY, 0, 0 },
    { "gEnhancements.Fixes.FixTexturesOOB", "gEnhancements.FixTexturesOOB", SYNC_INT, GROUP_GAMEPLAY, 0, 0 },

    // Rando rows are the option each generator reads, so a change only matters for the NEXT seed.
    // Every row below was checked against soh's OPT_BOOL/OPT_U8 declaration and MM's RO() default:
    // only same-value-space options are here. Notably absent, all OPT_U8 in OOT against a plain MM
    // bool: pots, crates, grass, freestanding, wonder items, boss souls, tokens, shopsanity, item
    // pool, ice traps. Also absent: the Triforce/win condition (the launcher owns it and overwrites
    // both games) and the "add the OTHER game's content" pairs, which are mirrors, not equals.
    { "gRando.Options.RO_SHUFFLE_COWS", "gRandoSettings.ShuffleCows", SYNC_INT, GROUP_RANDOMIZER, 0, 0 },
    { "gRando.Options.RO_SHUFFLE_TREE_DROPS", "gRandoSettings.ShuffleTrees", SYNC_INT, GROUP_RANDOMIZER, 0, 0 },
    { "gRando.Options.RO_SHUFFLE_HIVE_DROPS", "gRandoSettings.ShuffleBeehives", SYNC_INT, GROUP_RANDOMIZER, 0, 0 },
    { "gRando.Options.RO_SHUFFLE_BUTTERFLIES", "gRandoSettings.ShuffleButterflyFairies", SYNC_INT, GROUP_RANDOMIZER, 0,
      0 },
    { "gRando.Options.RO_SHUFFLE_OCARINA_BUTTONS", "gRandoSettings.ShuffleOcarinaButtons", SYNC_INT, GROUP_RANDOMIZER,
      0, 0 },
    { "gRando.Options.RO_SHUFFLE_SWIM", "gRandoSettings.ShuffleSwim", SYNC_INT, GROUP_RANDOMIZER, 0, 0 },
    { "gRando.Options.RO_SHUFFLE_SKELETON_KEY", "gRandoSettings.SkeletonKey", SYNC_INT, GROUP_RANDOMIZER, 0, 0 },
    { "gRando.Options.RO_SHUFFLE_TYCOON_WALLET", "gRandoSettings.IncludeTycoonWallet", SYNC_INT, GROUP_RANDOMIZER, 0,
      0 },
    { "gRando.Options.RO_SHUFFLE_NEI_ITEMS", "gRandoSettings.SkijerCustomItems", SYNC_INT, GROUP_RANDOMIZER, 0, 0 },
    // Both are the same three-value enum, in the same order.
    { "gRando.Options.RO_SHUFFLE_BOMB_ARROWS", "gRandoSettings.ShuffleBombArrows", SYNC_INT, GROUP_RANDOMIZER, 0, 0 },
    { "gRando.Options.RO_ELEMENTAL_WAND_SHUFFLE", "gRandoSettings.ElementalWandShuffle", SYNC_INT, GROUP_RANDOMIZER, 0,
      0 },
    { "gRando.Options.RO_CROSSOVER_POKEBALL", "gRandoSettings.CrossoverPokeball", SYNC_INT, GROUP_RANDOMIZER, 0, 0 },
    { "gRando.Options.RO_CROSSOVER_MARIO_MASK", "gRandoSettings.CrossoverMarioMask", SYNC_INT, GROUP_RANDOMIZER, 0, 0 },
};

constexpr size_t kPairCount = sizeof(kPairs) / sizeof(kPairs[0]);

// Last value this module saw on each side. A pair is only propagated when one side MOVED away from
// what was recorded — never because the two disagree. Someone who arrives with the two games set
// differently keeps both values until they actually change one.
struct Shadow {
    float mm = 0.0f;
    float oot = 0.0f;
    bool armed = false;
};
Shadow sShadow[kPairCount];

float ReadSide(const SyncPair& p, bool mmSide) {
    const char* cvar = mmSide ? p.mm : p.oot;
    if (p.type == SYNC_FLOAT) {
        return CVarGetFloat(cvar, 0.0f);
    }
    return static_cast<float>(CVarGetInteger(cvar, 0));
}

// MM's Fast Text is one boolean; OOT's Text Speed is a 1-6 slider whose top is "Instant".
constexpr int kOotTextSpeedFast = 6;
constexpr int kOotTextSpeedNormal = 1;

float TranslateToPeer(const SyncPair& p, float value, bool toMm) {
    if (p.type == SYNC_TEXT_SPEED) {
        return toMm ? (value > kOotTextSpeedNormal ? 1.0f : 0.0f)
                    : static_cast<float>(value != 0.0f ? kOotTextSpeedFast : kOotTextSpeedNormal);
    }
    if (toMm && p.mmMin != p.mmMax) {
        return std::clamp(value, p.mmMin, p.mmMax);
    }
    return value;
}

void WriteSide(const SyncPair& p, bool mmSide, float value) {
    const char* cvar = mmSide ? p.mm : p.oot;
    if (p.type == SYNC_FLOAT) {
        CVarSetFloat(cvar, value);
    } else {
        CVarSetInteger(cvar, static_cast<int>(value));
    }
    const ComboRando::GameMenu& game =
        mmSide ? ComboRando::ComboMenuModel::Get().Mm() : ComboRando::ComboMenuModel::Get().Oot();
    if (game.applyCVarChange) {
        game.applyCVarChange(cvar);
    }
}

bool GroupEnabled(SyncGroup group) {
    return CVarGetInteger(kGroups[group].cvar, 1) != 0;
}

// A pair whose group is off must re-arm from scratch when the group comes back, or the values it
// missed would land the moment it is re-enabled.
void Reconcile() {
    ComboRando::ComboMenuModel::Get().EnsureLoaded();
    const bool syncOn = CVarGetInteger(kCvarEnabled, 1) != 0;
    bool wrote = false;
    for (size_t i = 0; i < kPairCount; ++i) {
        const SyncPair& p = kPairs[i];
        Shadow& shadow = sShadow[i];
        const float mmValue = ReadSide(p, true);
        const float ootValue = ReadSide(p, false);
        if (!syncOn || !GroupEnabled(p.group)) {
            shadow.armed = false;
            continue;
        }
        if (!shadow.armed) {
            shadow = { mmValue, ootValue, true };
            continue;
        }
        const bool mmMoved = mmValue != shadow.mm;
        const bool ootMoved = ootValue != shadow.oot;
        if (!mmMoved && !ootMoved) {
            continue;
        }
        // MM wins a same-frame tie: the feature exists so a change made in 2Ship reaches Ship.
        const bool sourceIsMm = mmMoved;
        const float value = TranslateToPeer(p, sourceIsMm ? mmValue : ootValue, !sourceIsMm);
        WriteSide(p, !sourceIsMm, value);
        shadow.mm = ReadSide(p, true);
        shadow.oot = ReadSide(p, false);
        wrote = true;
    }
    if (wrote) {
        Ship::Context::GetRawInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
    }
}

// Draw-only shell: the reconcile needs a per-frame tick, not a window.
class SyncWindow final : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;
    void Draw() override {
        Reconcile();
    }

  protected:
    void InitElement() override {
    }
    void UpdateElement() override {
    }
    void DrawElement() override {
    }
};

std::shared_ptr<SyncWindow> sSyncWindow;

} // namespace

void ComboSync::RegisterSync() {
    auto ctx = Ship::Context::GetRawInstance();
    if (!ctx || !ctx->GetWindow() || !ctx->GetWindow()->GetGui()) {
        return;
    }
    if (!sSyncWindow) {
        sSyncWindow = std::make_shared<SyncWindow>("", true, " Combo Settings Sync");
    }
    ctx->GetWindow()->GetGui()->AddGuiWindow(sSyncWindow);
}

void ComboSync::DrawSyncSharedPanel() {
    const ImVec4 theme = ComboMenu_ThemeColor();
    bool changed = false;

    ComboMenu_PushCheckbox(theme);
    bool enabled = CVarGetInteger(kCvarEnabled, 1) != 0;
    if (ImGui::Checkbox("Sync shared settings between both games", &enabled)) {
        CVarSetInteger(kCvarEnabled, enabled ? 1 : 0);
        changed = true;
    }
    ComboMenu_PopCheckbox();
    ImGui::TextDisabled("Most settings are already one CVar in both games. These are the ones Ship and 2Ship\n"
                        "named differently: change one and the other follows.");
    ImGui::TextDisabled("Only a setting you actually change is copied. Two games that already disagree stay\n"
                        "as they are until you move one of them.");

    ImGui::SeparatorText("Categories");
    ImGui::BeginDisabled(!enabled);
    ComboMenu_PushCheckbox(theme);
    for (int g = 0; g < GROUP_COUNT; ++g) {
        size_t rows = 0;
        for (const SyncPair& p : kPairs) {
            if (p.group == g) {
                ++rows;
            }
        }
        bool on = CVarGetInteger(kGroups[g].cvar, 1) != 0;
        std::string label = std::string(kGroups[g].label) + " (" + std::to_string(rows) + ")";
        if (ImGui::Checkbox(label.c_str(), &on)) {
            CVarSetInteger(kGroups[g].cvar, on ? 1 : 0);
            changed = true;
        }
        ImGui::TextDisabled("    %s", kGroups[g].blurb);
    }
    ComboMenu_PopCheckbox();
    ImGui::EndDisabled();

    if (changed) {
        Ship::Context::GetRawInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
    }
}
