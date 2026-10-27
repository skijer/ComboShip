#pragma once

extern "C" {
#include "z64.h"
#include "mods/extended_inventory.h"
uint8_t Cane_GiveSkill(uint8_t skill);
}

namespace OotNeiEditor {

// The legacy byte table predates the u16 cell owners. Keep the gameplay table
// and existing saves untouched; project the current layout for this editor.
inline uint16_t Page2Item(int index) {
    if (index < 0 || index >= 24) {
        return ITEM_NONE;
    }
    switch (24 + index) {
        case SLOT_SHEIKAH_SLATE:
            return EXT_ITEM_SHEIKAH_SLATE;
        case SLOT_SHADOW_CRYSTAL:
            return EXT_ITEM_SHADOW_CRYSTAL;
        case SLOT_ROD_OF_SEASONS:
            return EXT_ITEM_ROD_OF_SEASONS;
        default:
            return gPage2Items[index];
    }
}

inline void GrantSeason(uint8_t season) {
    if (season >= SEASON_COUNT) {
        return;
    }
    Seasons_GrantSeason(season);
    // A save-editor grant explicitly unlocks the selected season under either
    // ownership policy. It does not change the configured randomizer policy.
    if (Seasons_RandoMode() == NEI_SEASONS_GATED) {
        Nei_Save()->seasonsGates |= (1u << season);
    }
}

inline void GrantAll() {
    auto* nei = Nei_Save();
    const uint8_t rune = nei->slateMode;
    const uint8_t season = nei->season;
    const uint16_t shared = Nei_GetOwnedItem(SLOT_PHANTOM_HOURGLASS);
    const uint16_t tool = Nei_GetOwnedItem(SLOT_SHOVEL);
    if (shared != ITEM_HYLIAS_GRACE && shared != EXT_ITEM_PHANTOM_HOURGLASS) {
        Nei_SetOwnedItem(SLOT_PHANTOM_HOURGLASS, ITEM_NONE);
    }

    for (int index = 0; index < 24; ++index) {
        const uint8_t slot = 24 + index;
        uint16_t item = Page2Item(index);
        if (slot == SLOT_ROCS) {
            item = ITEM_ROCS_CAPE;
        } else if (slot == SLOT_PHANTOM_HOURGLASS && shared == EXT_ITEM_PHANTOM_HOURGLASS) {
            item = shared;
        } else if (slot == SLOT_SHOVEL && tool == ITEM_DOMINION_ROD) {
            item = tool;
        }
        ExtInv_GiveItem(slot, item);
    }
    GraceHourglass_Grant(ITEM_HYLIAS_GRACE);
    GraceHourglass_Grant(EXT_ITEM_PHANTOM_HOURGLASS);
    nei->shovelOwned = nei->dominionOwned = nei->bombArrowsOwned = 1;
    for (uint8_t skill = 0; skill < 6; ++skill) {
        Cane_GiveSkill(skill);
    }
    for (uint8_t mode = 0; mode < WAND_MODE_COUNT; ++mode) {
        Wand_GrantMode(mode);
        Wand_SetModeOwned(mode, 1);
    }
    for (uint8_t power = 0; power < SLATE_RUNE_COUNT; ++power) {
        if (!Slate_RuneOwned(power)) {
            Slate_GrantRune(power);
        }
    }
    Seasons_GrantRod();
    for (uint8_t power = 0; power < SEASON_COUNT; ++power) {
        if (!(nei->seasonsOwned & (1u << power)) || !Seasons_SeasonOwned(power)) {
            GrantSeason(power);
        }
    }
    // Bulk grants repair ownership without choosing a different rune or season.
    nei->slateMode = rune < SLATE_RUNE_COUNT ? rune : SLATE_RUNE_BOMB;
    Seasons_SetSeason(season <= SEASON_OFF ? season : SEASON_SPRING);
}

} // namespace OotNeiEditor
