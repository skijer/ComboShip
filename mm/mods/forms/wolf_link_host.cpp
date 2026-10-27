#include "mods/forms/wolf_link_host.h"
#include "mods/nei_oot_compat.h"
#include "mods/forms/custom_forms.h"
#include "mods/ext_buttons/ext_buttons.h"
#include "mods/extended_inventory.h"
#include "mods/extended_equipment.h"
#include "mods/items/custom_items.h"
#include "mods/items/logic/item_beetle.h"
#include "mods/nei_save.h"
#include "mods/o2r_loader/o2r_loader.h"
#include "mods/pak_loader/pak_loader.h"
#include "mods/transformation_masks/wolf_link_form.h"
#include "expansions/sm64/sm64_mario.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"
#include <libultraship/bridge/consolevariablebridge.h>
#include <spdlog/spdlog.h>

extern "C" PlayState* gPlayState;

extern "C" {
// MM installs these actions for knockback, electricity, freezing and thawing.
void Player_Action_20(Player*, PlayState*);
void Player_Action_21(Player*, PlayState*);
void Player_Action_82(Player*, PlayState*);
void Player_Action_83(Player*, PlayState*);
}

namespace {
struct HostState {
    PlayState* play = nullptr;
    Player* player = nullptr;
    Input raw{};
    Input effective{};
    u32 frame = 0;
    s16 scene = 0;
    s16 file = -1;
    u16 filter = 0;
    u16 toggle = 0;
    u16 crystalPress = 0;
    bool attemptLogged = false;
};
HostState sHost;

constexpr u16 kItemButtons = BTN_CLEFT | BTN_CDOWN | BTN_CRIGHT | BTN_DRIGHT | BTN_DLEFT | BTN_DDOWN | BTN_DUP;
constexpr u16 kMashButtons = BTN_A | BTN_B | BTN_R;
constexpr u32 kInteractionStates = PLAYER_STATE1_TALKING | PLAYER_STATE1_CARRYING_ACTOR | PLAYER_STATE1_400 |
                                   PLAYER_STATE1_4 | PLAYER_STATE1_2000 | PLAYER_STATE1_4000 |
                                   PLAYER_STATE1_FIRST_PERSON | PLAYER_STATE1_CLIMBING_LADDER |
                                   PLAYER_STATE1_IN_CUTSCENE | PLAYER_STATE1_8000000 | PLAYER_STATE1_800000;

bool NativeDamage(const Player* player) {
    return (player->stateFlags1 & PLAYER_STATE1_4000000) || player->actionFunc == Player_Action_20 ||
           player->actionFunc == Player_Action_21 || player->actionFunc == Player_Action_82 ||
           player->actionFunc == Player_Action_83;
}

bool FormItem(u16 item) {
    return (item >= ITEM_MASK_DEKU && item <= ITEM_MASK_GIANT) || item == ITEM_MARIO_MASK || item == ITEM_POKEBALL ||
           item == ITEM_OOT_MASK_PLACEHOLDER || item == ITEM_OOT_MASK_PREV || item == ITEM_OOT_MASK_NEXT;
}

u16 ItemButtons(u16 item, bool forms) {
    constexpr u16 cButtons[] = { BTN_CLEFT, BTN_CDOWN, BTN_CRIGHT };
    constexpr u16 dButtons[] = { BTN_DRIGHT, BTN_DLEFT, BTN_DDOWN, BTN_DUP };
    u16 buttons = 0;
    for (s32 i = 0; i < 3; ++i) {
        const u16 equipped = ExtButton_GetItem(0, EQUIP_SLOT_C_LEFT + i);
        if (forms ? FormItem(equipped) : equipped == item)
            buttons |= cButtons[i];
    }
    if (CVarGetInteger("gEnhancements.Dpad.DpadEquips", 0)) {
        for (s32 i = 0; i < 4; ++i) {
            const u16 equipped = ExtButton_GetDpadItem(0, i);
            if (forms ? FormItem(equipped) : equipped == item)
                buttons |= dButtons[i];
        }
    }
    return buttons;
}

const char* OwnerRejection(Player* player) {
    if (CustomForms_ActiveForm() != CUSTOM_FORM_NONE)
        return "custom-form";
    if (Sm64Mario_IsActive() || CVarGetInteger("gSm64Mario", 0))
        return "mario-form";
    if (PakLoader_HasActiveBodyModel())
        return "pak-body-model";
    if (O2rLoader_HasActiveModel())
        return "o2r-body-model";
    if (Beetle_IsFlying())
        return "beetle";
    if (KiteSurf_IsActive())
        return "kite";
    if (Trident_OwnsPlayerAction())
        return "trident";
    if (CustomItems_BlocksMovement(player))
        return "custom-item-action";
    return nullptr;
}

const char* SelectionRejection(PlayState* play, Player* player) {
    if (player != GET_PLAYER(play))
        return "non-player-actor";
    if (player->transformation != PLAYER_FORM_HUMAN)
        return "native-form";
    if (player->currentMask != PLAYER_MASK_NONE)
        return "worn-mask";
    if (const char* owner = OwnerRejection(player))
        return owner;
    if (!CVarGetInteger("gMods.WolfLink.Enabled", 1))
        return "disabled";
    if (gSaveContext.gameMode != GAMEMODE_NORMAL)
        return "game-mode";
    if (player->stateFlags1 & PLAYER_STATE1_DEAD)
        return "dead";
    if (gSaveContext.save.saveInfo.playerData.health <= 0)
        return "no-health";
    if (player->stateFlags3 & PLAYER_STATE3_FLYING_WITH_HOOKSHOT)
        return "hookshot-flight";
    return nullptr;
}

bool SceneTransition(PlayState* play) {
    return play->transitionTrigger != TRANS_TRIGGER_OFF || play->transitionMode != TRANS_MODE_OFF;
}

const char* ReleaseRejection(PlayState* play, Player* player) {
    if (const char* selection = SelectionRejection(play, player))
        return selection;
    if (SceneTransition(play))
        return "scene-transition";
    if (play->actorCtx.isOverrideInputOn)
        return "override-input";
    if (player->stateFlags1 & (PLAYER_STATE1_200 | PLAYER_STATE1_20))
        return "native-input-state";
    return nullptr;
}

bool MustRelease(PlayState* play, Player* player) {
    return ReleaseRejection(play, player) != nullptr;
}

bool ContextAction(const Player* player) {
    return (player->stateFlags1 & kInteractionStates) ||
           ((player->stateFlags2 & PLAYER_STATE2_CAN_ACCEPT_TALK_OFFER) && player->talkActor) ||
           player->interactRangeActor || player->doorType != PLAYER_DOORTYPE_NONE ||
           player->csAction != PLAYER_CSACTION_NONE || player->textboxBtnCooldownTimer != 0;
}

const char* ActivationRejection(PlayState* play, Player* player) {
    if (const char* release = ReleaseRejection(play, player))
        return release;
    if (NativeDamage(player))
        return "native-damage";
    if (ContextAction(player))
        return "native-interaction";
    if (player->heldActor || player->rightHandActor)
        return "held-actor";
    if (player->itemAction > PLAYER_IA_NONE)
        return "held-item";
    if (player->meleeWeaponState != PLAYER_MELEE_WEAPON_STATE_0)
        return "melee-action";
    if (Nei_Save()->ownedItems[SLOT_SHADOW_CRYSTAL - 24] != EXT_ITEM_SHADOW_CRYSTAL)
        return "crystal-not-owned";
    return nullptr;
}

bool CanActivate(PlayState* play, Player* player) {
    return ActivationRejection(play, player) == nullptr;
}

void LogAttempt(const char* result, Player* player) {
    SPDLOG_INFO("MM Wolf: Shadow Crystal result={} frame={} scene={} raw={:04x} effective={:04x} "
                "itemAction={} heldActor={} rightHandActor={} state1={:08x} state2={:08x} state3={:08x}",
                result, sHost.frame, sHost.scene, sHost.raw.press.button, sHost.effective.press.button,
                static_cast<s32>(player->itemAction), player->heldActor != nullptr, player->rightHandActor != nullptr,
                player->stateFlags1, player->stateFlags2, player->stateFlags3);
    sHost.attemptLogged = true;
}

bool Active(Player* player) {
    return sHost.player == player && WolfLinkForm_IsSelected() && WolfLinkForm_IsReady();
}

void Stop(PlayState* play, Player* player) {
    WolfLinkForm_Cleanup(player, play);
    sHost.filter = 0;
}

void Suspend(PlayState* play, Player* player) {
    const u8 selected = WolfLinkForm_IsSelected();
    Stop(play, player);
    WolfLinkForm_Select(selected);
}

void ReleaseForState(PlayState* play, Player* player) {
    if (SelectionRejection(play, player)) {
        Stop(play, player);
    } else if (Active(player)) {
        WolfLinkForm_SuspendAction(player);
    }
}

void ResetSession() {
    // Save hooks may run without a live player, including a same-slot reload.
    PlayState* play = gPlayState == sHost.play ? gPlayState : nullptr;
    Player* player = play && GET_PLAYER(play) == sHost.player ? sHost.player : nullptr;
    Stop(play, player);
    sHost = HostState{};
}

void Filter(Input* input, u16 buttons) {
    input->cur.button &= ~buttons;
    input->press.button &= ~buttons;
    input->rel.button &= ~buttons;
}
} // namespace

extern "C" void WolfLinkHost_PreUpdate(PlayState* play, Player* player) {
    if (player != GET_PLAYER(play))
        return;
    WolfLinkForm_UpdateSfx(play);
    if (sHost.file != -1 && sHost.file != gSaveContext.fileNum) {
        Stop(sHost.play == play ? play : nullptr, sHost.player == player ? player : nullptr);
        sHost = HostState{};
    }
    if (sHost.player && (sHost.player != player || sHost.play != play)) {
        // A missed destroy must not dereference an actor/play allocation from the old scene.
        Suspend(nullptr, nullptr);
    } else if (sHost.player && (sHost.scene != play->sceneId || play->gameplayFrames < sHost.frame)) {
        Suspend(play, player);
    }
    sHost.play = play;
    sHost.player = player;
    sHost.scene = play->sceneId;
    sHost.file = gSaveContext.fileNum;
    sHost.frame = play->gameplayFrames;
    sHost.raw = play->state.input[0];
    sHost.effective = Input{};
    sHost.filter = 0;
    sHost.toggle = 0;
    sHost.crystalPress = sHost.raw.press.button & ItemButtons(EXT_ITEM_SHADOW_CRYSTAL, false);
    sHost.attemptLogged = false;

    if (const char* selection = SelectionRejection(play, player)) {
        if (sHost.crystalPress)
            LogAttempt(selection, player);
        if (WolfLinkForm_IsSelected() || WolfLinkForm_IsReady())
            Stop(play, player);
        return;
    }

    if (sHost.raw.press.button & ItemButtons(0, true)) {
        if (sHost.crystalPress)
            LogAttempt("form-item-input", player);
        if (WolfLinkForm_IsSelected() || WolfLinkForm_IsReady())
            Stop(play, player);
        return;
    }
    if (WolfLinkForm_IsSelected() && !WolfLinkForm_IsReady() && !WolfLinkForm_LoadSkeleton(play))
        Stop(play, player);
    // Restore the selected presentation before native entrance/exit controls finish.
    // Those actions retain their input, speed and collision ownership.
    if (const char* release = ReleaseRejection(play, player)) {
        if (sHost.crystalPress)
            LogAttempt(release, player);
        ReleaseForState(play, player);
        return;
    }
    if (Active(player) || CanActivate(play, player)) {
        sHost.toggle = ItemButtons(EXT_ITEM_SHADOW_CRYSTAL, false);
        sHost.filter = sHost.toggle;
    } else if (sHost.crystalPress) {
        LogAttempt(ActivationRejection(play, player), player);
    }
    if (Active(player)) {
        sHost.filter |= kItemButtons;
        if (!NativeDamage(player)) {
            sHost.filter |= BTN_B | BTN_R;
            if (!ContextAction(player))
                sHost.filter |= BTN_A;
        }
    }
    // Raw listeners (equipment/tools/Slate/Hourglass) run before native local-pad filtering.
    Filter(&play->state.input[0], sHost.filter);
}

extern "C" void WolfLinkHost_RestorePlayerInput(Player* player, Input* input) {
    if (sHost.player != player)
        return;
    // Reconstitute only reservations made before raw listeners. MM selects/suppresses
    // this physical pad afterward, and OnPassPlayerInputs still has final policy control.
    const u16 reserved = (sHost.filter & kMashButtons) | sHost.toggle;
    input->cur.button |= sHost.raw.cur.button & reserved;
    input->press.button |= sHost.raw.press.button & reserved;
    input->rel.button |= sHost.raw.rel.button & reserved;
}

extern "C" void WolfLinkHost_FilterInput(Player* player, Input* input) {
    if (sHost.player != player)
        return;
    sHost.effective = *input;
    if (MustRelease(sHost.play, player)) {
        ReleaseForState(sHost.play, player);
        sHost.filter = 0;
        return;
    }
    const u16 toggle = sHost.toggle;
    if (input->press.button & toggle) {
        if (Active(player)) {
            Stop(sHost.play, player);
            WolfLinkForm_PlayTransformSfx(0);
            LogAttempt("deactivated", player);
        } else if (const char* rejection = ActivationRejection(sHost.play, player)) {
            LogAttempt(rejection, player);
        } else if (WolfLinkForm_LoadSkeleton(sHost.play)) {
            WolfLinkForm_Select(1);
            WolfLinkForm_PlayTransformSfx(1);
            LogAttempt("activated", player);
        } else {
            LogAttempt("asset-load-failed", player);
        }
    } else if (sHost.crystalPress && !sHost.attemptLogged) {
        LogAttempt("input-suppressed", player);
    }
    sHost.filter = toggle;
    if (Active(player)) {
        sHost.filter |= kItemButtons;
        if (!NativeDamage(player)) {
            sHost.filter |= BTN_B | BTN_R;
            if (!ContextAction(player))
                sHost.filter |= BTN_A;
        }
    }
    Filter(input, sHost.filter);
}

extern "C" void WolfLinkHost_BeforeAction(PlayState* play, Player* player, Input* input) {
    if (!Active(player))
        return;
    // Native damage/scene processing runs inside Player_UpdateCommon after PreUpdate.
    if (MustRelease(play, player)) {
        ReleaseForState(play, player);
        return;
    }
    const bool native = NativeDamage(player);
    Input actionInput = sHost.effective;
    if (native) {
        // Preserve freeze/thaw mash inputs even when MM installed the action this frame.
        input->cur.button |= sHost.effective.cur.button & kMashButtons;
        input->press.button |= sHost.effective.press.button & kMashButtons;
        input->rel.button |= sHost.effective.rel.button & kMashButtons;
    } else if (ContextAction(player)) {
        WolfLinkForm_ReleaseAction(player);
        Filter(&actionInput, kMashButtons);
    }
    WolfLinkForm_Update(player, play, &actionInput, native);
}

extern "C" void WolfLinkHost_ApplyCollisionShape(Player* player) {
    if (Active(player) && !MustRelease(sHost.play, player))
        WolfLinkForm_ApplyCollisionShape(player);
}

extern "C" u8 WolfLinkHost_PlayFootstep(Player* player) {
    if (!Active(player) || SelectionRejection(sHost.play, player))
        return 0;
    switch (player->floorSfxOffset) {
        case NA_SE_PL_WALK_GROUND - SFX_FLAG:
        case NA_SE_PL_WALK_SAND - SFX_FLAG:
        case NA_SE_PL_WALK_CONCRETE - SFX_FLAG:
        case NA_SE_PL_WALK_DIRT - SFX_FLAG:
        case NA_SE_PL_WALK_GRASS - SFX_FLAG:
        case NA_SE_PL_WALK_SNOW - SFX_FLAG:
        case NA_SE_PL_WALK_ICE - SFX_FLAG:
            if (!(player->actor.bgCheckFlags & BGCHECKFLAG_GROUND))
                return 1;
            // Match MM Wolfos' complete paw-contact cue, without Link's metal equipment layer.
            Player_PlaySfx(player, NA_SE_EN_WOLFOS_WALK);
            return 1;
        default:
            return 0; // Retain native water, magma, metal/glass and unknown surface contacts.
    }
}

extern "C" u8 WolfLinkHost_PlayClimbVoice(Player* player, u16 sfxId) {
    if (sfxId != NA_SE_VO_LI_CLIMB_END || !Active(player) || SelectionRejection(sHost.play, player))
        return 0;
    WolfLinkForm_PlayExertionSfx();
    return 1;
}

extern "C" u8 WolfLinkHost_Draw(PlayState* play, Player* player) {
    if (!Active(player))
        return 0;
    if (MustRelease(play, player)) {
        ReleaseForState(play, player);
        if (!Active(player))
            return 0;
    }
    return WolfLinkForm_Draw(play, player) ? 1 : 2;
}

extern "C" void WolfLinkHost_OnUseItem(PlayState* play, Player* player, s32 item) {
    if (sHost.player == player && WolfLinkForm_IsSelected() && FormItem(item))
        Stop(play, player);
}

extern "C" void WolfLinkHost_Destroy(PlayState* play, Player* player) {
    if (sHost.player == player && sHost.play == play) {
        const bool retain = !play->state.running && play->state.init == Play_Init &&
                            sHost.file == gSaveContext.fileNum && !SelectionRejection(play, player);
        if (retain)
            Suspend(play, player);
        else
            Stop(play, player);
        sHost = HostState{};
        if (retain)
            sHost.file = gSaveContext.fileNum;
    }
}

static void RegisterWolfLinkHost() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSaveInit>([](s16) { ResetSession(); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSaveLoad>([](s16) { ResetSession(); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameStateUpdate>(
        []() { WolfLinkForm_UpdateSfx(gPlayState); });
}

static RegisterShipInitFunc initWolfLinkHost(RegisterWolfLinkHost, {});
