#include "Rando.h"
#include "2s2h/CustomMessage/CustomMessage.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "Rando/ActorBehavior/ActorBehavior.h"
#include "Rando/MiscBehavior/MiscBehavior.h"
#include "Rando/MiscBehavior/ClockShuffle.h"
#include "Rando/Spoiler/Spoiler.h"
#include "Rando/CheckTracker/CheckTracker.h"
#include "2s2h/ShipInit.hpp"
#include <ship/window/FileDropMgr.h>
#include <ship/Context.h>
#include <libultraship/bridge/consolevariablebridge.h>
#ifdef COMBO_BUILD
#include "rando/CrossForeign.h" // ComboShip: family-B foreign-item-location fallback
// ComboShip (#164): combo Hint Tracker reveal sink (the launcher registers it, see BenPort.cpp).
extern "C" void (*gMMComboHintReveal)(int fileNum, int kind, int poolIndex, const char* key, const char* text);
#endif

// When a save is loaded, we want to unregister all hooks and re-register them if it's a rando save
void OnSaveLoadHandler(s16 fileNum) {
    Rando::MiscBehavior::OnFileLoad();
    Rando::ActorBehavior::OnFileLoad();
    Rando::CheckTracker::OnFileLoad();
    Rando::ClockShuffle::OnFileLoad();

    // Re-initalizes enhancements that are effected by the save being rando or not
    ShipInit::Init("IS_RANDO");
}

// Entry point for the module, run once on game boot
void Rando::Init() {
    Rando::Spoiler::RefreshOptions();
    Rando::MiscBehavior::Init();
    Rando::ActorBehavior::Init();
    Rando::CheckTracker::Init();
    Ship::Context::GetRawInstance()->GetFileDropMgr()->RegisterDropHandler(Rando::Spoiler::HandleFileDropped);

    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSaveLoad>(OnSaveLoadHandler);
}

RandoCheckId Rando::FindItemPlacement(RandoItemId randoItemId) {
    for (auto& [randoCheckId, check] : Rando::StaticData::Checks) {
        if (RANDO_SAVE_CHECKS[randoCheckId].randoItemId == randoItemId) {
            return randoCheckId;
        }
    }

    return RC_UNKNOWN;
}

// Location text for a hint, valid in the OoT+MM combo too. An item with no MM check landed on the OoT
// side, so name the area it ended up in instead of "in an Unknown Location" — what testers saw on
// EVERY cross-game hint. Skijer's NEI
std::string Rando::GetHintLocationText(RandoItemId randoItemId, RandoCheckId randoCheckId, bool exact) {
    if (randoCheckId != RC_UNKNOWN) {
        return Rando::StaticData::GetLocationNameForHint(randoCheckId, exact);
    }

    if (randoItemId == RI_NONE) {
        return Rando::StaticData::GetLocationNameForHint(randoCheckId, exact);
    }

#ifdef COMBO_BUILD
    // ComboShip owns the foreign-placement map and marks the hint read in the tracker, so defer to it
    // rather than reading the NEI sidecar, which a ComboShip-generated seed never writes.
    return Rando::GetItemLocationHintName(randoItemId, exact);
#else
    std::string ootArea = Rando::Spoiler::GetOotAreaForItem(Rando::StaticData::Items[randoItemId].spoilerName);
    if (!ootArea.empty()) {
        return "in " + ootArea;
    }

    return Rando::StaticData::GetLocationNameForHint(randoCheckId, exact);
#endif
}

std::vector<RandoCheckId> Rando::FindMultiItemPlacement(RandoItemId randoItemId) {
    std::vector<RandoCheckId> itemPlacements;
    for (auto& [randocheckId, check] : Rando::StaticData::Checks) {
        if (RANDO_SAVE_CHECKS[randocheckId].randoItemId == randoItemId) {
            itemPlacements.push_back(randocheckId);
        }
    }
    return itemPlacements;
}

std::string Rando::GetItemLocationHintName(RandoItemId randoItemId, bool exact) {
    RandoCheckId rc = FindItemPlacement(randoItemId);
    if (rc != RC_UNKNOWN) {
        return Rando::StaticData::GetLocationNameForHint(rc, exact);
    }
#ifdef COMBO_BUILD
    // ComboShip: this MM item was placed at an OOT check by the combo fill. Try Phase 3's
    // pre-rendered region text first; fall back to Phase 1's raw check-name string for seeds
    // generated before hints.mm.itemLocations existed.
    int slot = gSaveContext.fileNum;
    if (slot != 0xFF) {
        // ComboShip: the consolidated file keys foreign items by the friendly combo-spoiler name.
        const std::string& friendlyName = Rando::StaticData::GetItemDisplayName(randoItemId);
        if (!friendlyName.empty()) {
            static uint64_t s_hintsGen = (uint64_t)-1;
            static ComboRando::MmHints s_hints;
            if (s_hintsGen != Rando::MiscBehavior::ComboRandoGen()) {
                s_hints = ComboRando::LoadHintsMM(slot);
                s_hintsGen = Rando::MiscBehavior::ComboRandoGen();
            }
            auto hintIt = s_hints.itemLocations.find(friendlyName);
            if (hintIt != s_hints.itemLocations.end()) {
                // ComboShip (#164): this NPC hint is about to be shown — mark it read in the tracker.
                if (gMMComboHintReveal != nullptr) {
                    gMMComboHintReveal(slot, 2, -1, friendlyName.c_str(), hintIt->second.c_str());
                }
                return hintIt->second;
            }

            static uint64_t s_mapGen = (uint64_t)-1;
            static std::unordered_map<std::string, ComboRando::ForeignPlacement> s_map;
            if (s_mapGen != Rando::MiscBehavior::ComboRandoGen()) {
                s_map = ComboRando::LoadForeignByItem(slot, ComboRando::GAME_MM);
                s_mapGen = Rando::MiscBehavior::ComboRandoGen();
            }
            auto it = s_map.find(friendlyName);
            if (it != s_map.end()) {
                return "at " + it->second.checkName + " (OOT)";
            }
        }
    }
#endif
    return "in an Unknown Location";
}

// =============================================================================
// Sheikah Sensor rune (mods/actors/sensor_rune.c) — hints for the desired items
// =============================================================================

static std::string sSensorHintText;

static RandoItemId SensorGetDesire(s32 slot) {
    if (slot < 0 || slot >= SENSOR_DESIRE_SLOTS) {
        return RI_NONE;
    }
    int32_t item = CVarGetInteger((std::string(CVAR_SENSOR_DESIRE_PREFIX) + std::to_string(slot)).c_str(), RI_NONE);
    if (item <= RI_NONE || item >= RI_MAX) {
        return RI_NONE;
    }
    return static_cast<RandoItemId>(item);
}

// Where a wished-for item is still waiting. RC_UNKNOWN = collected already, or never placed —
// either way the rune moves on to the next wish.
static RandoCheckId SensorFindOutstandingCheck(RandoItemId randoItemId) {
    for (RandoCheckId rc : Rando::FindMultiItemPlacement(randoItemId)) {
        const RandoSaveCheck& save = RANDO_SAVE_CHECKS[rc];
        if (save.shuffled && !save.obtained && !save.cycleObtained) {
            return rc;
        }
    }
    return RC_UNKNOWN;
}

/**
 * Consult the wish list in slot order and cache the hint for the first wish still out there.
 * Returns 0 when no wish is answerable — the rune refuses BEFORE charging for it.
 */
extern "C" u8 SensorRune_PrepareHint(void) {
    sSensorHintText.clear();

    for (s32 slot = 0; slot < SENSOR_DESIRE_SLOTS; slot++) {
        RandoItemId item = SensorGetDesire(slot);
        if (item == RI_NONE) {
            continue;
        }

        RandoCheckId rc = SensorFindOutstandingCheck(item);
        if (rc == RC_UNKNOWN) {
            continue;
        }

        sSensorHintText = "The slate senses " + Rando::StaticData::GetItemName(item) + " " +
                          Rando::GetHintLocationText(item, rc) + "...";
        return 1;
    }
    return 0;
}

// \002 = adjustable colour, \021 = newline, \302 = two-choice control code (hex escapes would eat
// the following letters, so these must stay octal — same trap as time_gate_message.cpp).
extern "C" void SensorRune_OpenPrompt(void) {
    CustomMessage::StartTextbox("Asking costs one Heart Container,\021forever. Ask the slate?\002\021\302Yes\021No",
                                {});
}

extern "C" void SensorRune_OpenHint(void) {
    CustomMessage::StartTextbox(sSensorHintText, {});
}
