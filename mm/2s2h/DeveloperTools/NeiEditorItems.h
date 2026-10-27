#pragma once

#include "2s2h/Rando/Rando.h"
extern "C" {
#include "mods/extended_inventory.h"
#include "mods/extended_equipment.h"
}
#include <cstddef>
#include <string_view>
#include <vector>

namespace NeiEditor {

inline bool IsItem(RandoItemId id) {
    // MM's native Mirror Shield is the canonical Shield of Ikana identity.
    // Its name does not have the RI_OOT_EXT_ prefix used by its siblings.
    if (id == RI_SHIELD_MIRROR)
        return true;
    const auto item = Rando::StaticData::Items.find(id);
    if (item == Rando::StaticData::Items.end() || !item->second.spoilerName) {
        return false;
    }
    const std::string_view identity(item->second.spoilerName);
    return identity.starts_with("RI_OOT_NEI_") || identity.starts_with("RI_OOT_EXT_");
}

// Use the live item table, including appended identities, rather than the old
// actor/action registry (which cannot describe sibling powers or full-width IDs).
inline std::vector<RandoItemId> Catalog() {
    std::vector<RandoItemId> items;
    for (const auto& [id, item] : Rando::StaticData::Items) {
        if (IsItem(id)) {
            items.push_back(id);
        }
    }
    return items;
}

struct SlotItem {
    RandoItemId id;
    uint8_t slot;
    uint16_t item;
};
inline constexpr SlotItem slotItems[] = {
    { RI_OOT_NEI_BALL_AND_CHAIN, SLOT_BALL_AND_CHAIN, ITEM_BALL_AND_CHAIN },
    { RI_OOT_NEI_BEETLE, SLOT_BEETLE, ITEM_BEETLE },
    { RI_OOT_NEI_DEKU_LEAF, SLOT_DEKU_LEAF, ITEM_DEKU_LEAF },
    { RI_OOT_NEI_DEMISE_DESTRUCTION, SLOT_DEMISE_DESTRUCTION, ITEM_DEMISE_DESTRUCTION },
    { RI_OOT_NEI_FIRE_ROD, SLOT_FIRE_ROD, ITEM_ROD_FIRE },
    { RI_OOT_NEI_GUST_JAR, SLOT_GUST_JAR, ITEM_GUST_JAR },
    { RI_OOT_NEI_ICE_ROD, SLOT_ICE_ROD, ITEM_ROD_ICE },
    { RI_OOT_NEI_LANTERN, SLOT_LANTERN, ITEM_LANTERN },
    { RI_OOT_NEI_LIGHT_ROD, SLOT_LIGHT_ROD, ITEM_ROD_LIGHT },
    { RI_OOT_NEI_MINISH_CAP, SLOT_MINISH_CAP, ITEM_MINISH_CAP },
    { RI_OOT_NEI_MOGMA_MITTS, SLOT_MOGMA_MITTS, ITEM_MOGMA_MITTS },
    { RI_OOT_NEI_SPINNER, SLOT_SPINNER, ITEM_SPINNER },
    { RI_OOT_NEI_SWITCH_HOOK, SLOT_SWITCH_HOOK, ITEM_SWITCH_HOOK },
    { RI_OOT_NEI_TIME_GATE, SLOT_TIME_GATE, ITEM_TIME_GATE },
    { RI_OOT_NEI_WHIP, SLOT_WHIP, ITEM_WHIP },
    { RI_OOT_NEI_ZONAI_PERMAFROST, SLOT_ZONAI_PERMAFROST, ITEM_ZONAI_PERMAFROST },
    { RI_OOT_NEI_SHEIKAH_SLATE, SLOT_SHEIKAH_SLATE, EXT_ITEM_SHEIKAH_SLATE },
    { RI_OOT_NEI_SHADOW_CRYSTAL, SLOT_SHADOW_CRYSTAL, EXT_ITEM_SHADOW_CRYSTAL },
    { RI_OOT_NEI_ROD_OF_SEASONS, SLOT_ROD_OF_SEASONS, EXT_ITEM_ROD_OF_SEASONS },
};

inline constexpr RandoItemId runeItems[] = {
    RI_OOT_NEI_SLATE_RUNE_BOMB,         RI_OOT_NEI_SLATE_RUNE_STASIS, RI_OOT_NEI_SLATE_RUNE_CRYONIS,
    RI_OOT_NEI_SLATE_RUNE_MASTER_CYCLE, RI_OOT_NEI_DESIRE_SENSOR,
};
inline constexpr RandoItemId seasonItems[] = {
    RI_OOT_NEI_SEASON_SPRING,
    RI_OOT_NEI_SEASON_SUMMER,
    RI_OOT_NEI_SEASON_AUTUMN,
    RI_OOT_NEI_SEASON_WINTER,
};
inline constexpr RandoItemId caneItems[] = {
    RI_OOT_NEI_CANE_OF_SOMARIA, RI_OOT_NEI_CANE_SOMARIA_BLOCK, RI_OOT_NEI_CANE_SOMARIA_PLATFORM,
    RI_OOT_NEI_CANE_PACCI_FLIP, RI_OOT_NEI_CANE_PACCI_STONE,   RI_OOT_NEI_CANE_PACCI_ULTRAHAND,
};
inline constexpr RandoItemId wandItems[] = {
    RI_OOT_NEI_WAND_SAND_ROD,   RI_OOT_NEI_WAND_TORNADO_ROD, RI_OOT_NEI_WAND_WATER_ROD,
    RI_OOT_NEI_WAND_METEOR_ROD, RI_OOT_NEI_WAND_STORM_ROD,   RI_OOT_NEI_WAND_SHADOW_SCEPTER,
};
inline constexpr RandoItemId wandMedallions[] = {
    RI_OOT_MEDALLION_SPIRIT, RI_OOT_MEDALLION_FOREST, RI_OOT_MEDALLION_WATER,
    RI_OOT_MEDALLION_FIRE,   RI_OOT_MEDALLION_LIGHT,  RI_OOT_MEDALLION_SHADOW,
};

inline int WandMode(RandoItemId id) {
    if (id == RI_OOT_NEI_ELEMENTAL_WAND) {
        return WAND_MODE_SAND;
    }
    for (int i = 0; i < WAND_MODE_COUNT; ++i) {
        if (id == wandItems[i]) {
            return i;
        }
    }
    return -1;
}

inline bool WandModeEarned(int mode) {
    if (mode < 0 || mode >= WAND_MODE_COUNT) {
        return false;
    }
    const auto earned = Nei_Save()->wandRodsOwned & ((1u << WAND_MODE_COUNT) - 1);
    return Wand_RandoMode() == WAND_RANDO_SINGLE ? earned != 0 : (earned & (1u << mode)) != 0;
}

inline bool IsOwned(RandoItemId id) {
    if (id == RI_OOT_NEI_TIME_GATE || id == RI_OOT_NEI_MAGIC_MIRROR) {
        const auto selected = Nei_GetOwnedItem(SLOT_MAGIC_MIRROR);
        const auto item = id == RI_OOT_NEI_TIME_GATE ? ITEM_TIME_GATE : EXT_ITEM_MAGIC_MIRROR;
        return TravelTools_IsOwned(item) && (selected == ITEM_TIME_GATE || selected == EXT_ITEM_MAGIC_MIRROR);
    }
    if (id == RI_OOT_NEI_HYLIAS_GRACE || id == RI_OOT_NEI_PHANTOM_HOURGLASS) {
        const auto selected = Nei_GetOwnedItem(SLOT_PHANTOM_HOURGLASS);
        const auto item = id == RI_OOT_NEI_HYLIAS_GRACE ? ITEM_HYLIAS_GRACE : EXT_ITEM_PHANTOM_HOURGLASS;
        return GraceHourglass_IsOwned(item) &&
               (selected == ITEM_HYLIAS_GRACE || selected == EXT_ITEM_PHANTOM_HOURGLASS);
    }
    const auto* nei = Nei_Save();
    for (const auto& item : slotItems) {
        if (id == item.id) {
            return Nei_GetOwnedItem(item.slot) == item.item;
        }
    }
    for (uint8_t i = 0; i < SLATE_RUNE_COUNT; ++i) {
        if (id == runeItems[i]) {
            return Slate_RuneOwned(i) && Nei_GetOwnedItem(SLOT_SHEIKAH_SLATE) == EXT_ITEM_SHEIKAH_SLATE;
        }
    }
    for (uint8_t i = 0; i < SEASON_COUNT; ++i) {
        if (id == seasonItems[i]) {
            return (nei->seasonsOwned & (1u << i)) && Nei_GetOwnedItem(SLOT_ROD_OF_SEASONS) == EXT_ITEM_ROD_OF_SEASONS;
        }
    }
    for (uint8_t i = 0; i < 6; ++i) {
        if (id == caneItems[i]) {
            // The base identity is progressive; explicit sibling identities own one skill.
            return Nei_GetOwnedItem(SLOT_CANE_OF_SOMARIA) == ITEM_CANE_OF_SOMARIA &&
                   (i == 0 ? (nei->caneSkills & 0x3F) == 0x3F : Nei_CaneHasSkill(i));
        }
    }
    const int wand = WandMode(id);
    if (wand >= 0) {
        return WandModeEarned(wand) && Wand_ModeOwned((uint8_t)wand) &&
               Nei_GetOwnedItem(SLOT_ELEMENTAL_WAND) == ITEM_ELEMENTAL_WAND;
    }
    switch (id) {
        case RI_SHIELD_MIRROR:
            return ExtEquip_HasItem(EQUIP_TYPE_SHIELD, 3);
        case RI_OOT_NEI_BOMB_ARROWS:
            return nei->bombArrowsOwned != 0;
        case RI_OOT_NEI_SHOVEL:
            return nei->shovelOwned &&
                   (Nei_GetOwnedItem(SLOT_SHOVEL) == ITEM_SHOVEL || Nei_GetOwnedItem(SLOT_SHOVEL) == ITEM_DOMINION_ROD);
        case RI_OOT_NEI_DOMINION_ROD:
            return nei->dominionOwned &&
                   (Nei_GetOwnedItem(SLOT_SHOVEL) == ITEM_SHOVEL || Nei_GetOwnedItem(SLOT_SHOVEL) == ITEM_DOMINION_ROD);
        case RI_OOT_NEI_POKE_BALL:
            return nei->pokeballOwned != 0;
        case RI_OOT_NEI_MARIO_MASK:
            return nei->marioMaskOwned != 0;
        case RI_OOT_NEI_ROCS_FEATHER:
            return Nei_GetOwnedItem(SLOT_ROCS) == ITEM_ROCS_FEATHER_SKIJER ||
                   Nei_GetOwnedItem(SLOT_ROCS) == ITEM_ROCS_CAPE;
        case RI_OOT_NEI_ROCS_CAPE:
            return Nei_GetOwnedItem(SLOT_ROCS) == ITEM_ROCS_CAPE;
        case RI_OOT_EXT_MAGIC_CAPE:
            return ExtEquip_CapeOwned() != 0;
        case RI_OOT_EXT_CANE_OF_BYRNA:
            return ExtEquip_HasItem(EQUIP_TYPE_SWORD, 1);
        case RI_OOT_EXT_FOUR_SWORD:
            return ExtEquip_HasItem(EQUIP_TYPE_SWORD, 2);
        case RI_OOT_EXT_TRIDENT:
            return ExtEquip_HasItem(EQUIP_TYPE_SWORD, 3);
        case RI_OOT_EXT_DIVINE_SHIELD:
            return ExtEquip_HasItem(EQUIP_TYPE_SHIELD, 1);
        case RI_OOT_EXT_SHEIKAH_SHIELD:
            return ExtEquip_HasItem(EQUIP_TYPE_SHIELD, 2);
        case RI_OOT_EXT_CHAMPIONS_TUNIC:
            return ExtEquip_HasItem(EQUIP_TYPE_TUNIC, 1);
        case RI_OOT_EXT_SPIRIT_BREASTPLATE:
            return ExtEquip_HasItem(EQUIP_TYPE_TUNIC, 2);
        case RI_OOT_EXT_WATER_DRAGON_SCALE:
            return ExtEquip_HasItem(EQUIP_TYPE_TUNIC, 3);
        case RI_OOT_EXT_PEGASUS_ANKLET:
            return ExtEquip_HasItem(3, 1);
        case RI_OOT_EXT_CLIMB_BOOTS:
            return ExtEquip_HasItem(3, 2);
        case RI_OOT_EXT_ROC_BOOTS:
            return ExtEquip_HasItem(3, 3);
        default:
            return false;
    }
}

template <std::size_t N> inline bool IsOneOf(RandoItemId id, const RandoItemId (&items)[N]) {
    for (const auto item : items) {
        if (id == item) {
            return true;
        }
    }
    return false;
}

// Clear Custom Items only removes cells. Retained powers still own their host;
// restore that cell through the canonical setter without replaying earned grants
// (the cane handler ignores owned skills, and replaying runes changes selection).
inline bool RestoreOwnedHost(RandoItemId id) {
    if (id == RI_OOT_NEI_TIME_GATE || id == RI_OOT_NEI_MAGIC_MIRROR) {
        const auto before = Nei_GetOwnedItem(SLOT_MAGIC_MIRROR);
        TravelTools_Heal();
        return before != Nei_GetOwnedItem(SLOT_MAGIC_MIRROR);
    }
    if (id == RI_OOT_NEI_HYLIAS_GRACE || id == RI_OOT_NEI_PHANTOM_HOURGLASS) {
        const auto before = Nei_GetOwnedItem(SLOT_PHANTOM_HOURGLASS);
        GraceHourglass_Heal();
        return before != Nei_GetOwnedItem(SLOT_PHANTOM_HOURGLASS);
    }
    const auto* nei = Nei_Save();
    uint8_t slot = SLOT_NONE;
    uint16_t item = ITEM_NONE;
    if ((id == RI_OOT_NEI_SHEIKAH_SLATE || IsOneOf(id, runeItems)) && nei->slateRunesOwned) {
        slot = SLOT_SHEIKAH_SLATE;
        item = EXT_ITEM_SHEIKAH_SLATE;
    } else if ((id == RI_OOT_NEI_ROD_OF_SEASONS || IsOneOf(id, seasonItems)) &&
               (nei->seasonsRodOwned || nei->seasonsOwned)) {
        slot = SLOT_ROD_OF_SEASONS;
        item = EXT_ITEM_ROD_OF_SEASONS;
    } else if (IsOneOf(id, caneItems) && nei->caneSkills) {
        slot = SLOT_CANE_OF_SOMARIA;
        item = ITEM_CANE_OF_SOMARIA;
    } else if (id == RI_OOT_NEI_SHOVEL && nei->shovelOwned) {
        slot = SLOT_SHOVEL;
        item = ITEM_SHOVEL;
    } else if (id == RI_OOT_NEI_DOMINION_ROD && nei->dominionOwned) {
        slot = SLOT_SHOVEL;
        item = ITEM_DOMINION_ROD;
    } else if (WandMode(id) >= 0 && (nei->wandRodsOwned & ((1u << WAND_MODE_COUNT) - 1))) {
        slot = SLOT_ELEMENTAL_WAND;
        item = ITEM_ELEMENTAL_WAND;
    }
    if (slot != SLOT_NONE && Nei_GetOwnedItem(slot) == ITEM_NONE) {
        ExtInv_GiveItem(slot, item);
        return true;
    }
    return false;
}

inline bool Grant(RandoItemId id) {
    if (!IsItem(id)) {
        return false;
    }
    if (id == RI_SHIELD_MIRROR) {
        if (IsOwned(id))
            return false;
        // An editor ownership edit uses the same setter as the equipment grid.
        // It must not equip the native shield or replay a received pickup.
        ExtEquip_GiveItem(EQUIP_TYPE_SHIELD, 3);
        return true;
    }
    const bool restored = RestoreOwnedHost(id);
    const int wand = WandMode(id);
    if (wand >= 0) {
        bool changed = restored;
        if (!WandModeEarned(wand)) {
            Rando::GiveItem(id);
            changed = true;
        }
        // Repair usability separately from earned ownership: a retained rod
        // may be missing its medallion, and must not be awarded a second time.
        if (Wand_RandoMode() == WAND_RANDO_MEDALLIONS && !Wand_ModeOwned((uint8_t)wand)) {
            Rando::GiveItem(wandMedallions[wand]);
            changed = true;
        }
        return changed;
    }
    if (IsOwned(id)) {
        return restored;
    }
    Rando::GiveItem(id);
    return true;
}

inline void GrantAll() {
    const uint8_t season = Nei_Save()->season;
    const bool keepSeason = season == SEASON_OFF || Seasons_SeasonOwned(season);
    for (const auto id : Catalog()) {
        Grant(id);
    }
    // Individual season pickups select their reward. A bulk ownership repair
    // should retain the player's choice instead of implicitly selecting Winter.
    Seasons_SetSeason(keepSeason ? season : SEASON_SPRING);
}

} // namespace NeiEditor
