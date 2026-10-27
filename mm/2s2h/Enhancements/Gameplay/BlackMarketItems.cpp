#ifdef COMBO_BUILD
#include "BlackMarketItems.h"
#include "2s2h/Rando/Rando.h"
extern "C" {
#include "global.h"
#include "mods/nei_save.h"
#include "mods/extended_inventory.h"
#include "mods/extended_equipment.h"
u8 Cane_GiveSkill(u8 skill);
u8 Cane_HasSkill(u8 skill);
}

namespace ComboBlackMarketItems {
RandoItemId Id(const ComboRando::BlackMarket::Item& item) {
    return Rando::StaticData::GetItemIdFromName(item.identity);
}

// Physical ownership, never current wheel selection or a progressive resolver.
bool Owned(const ComboRando::BlackMarket::Item& item) {
    const auto id = Id(item);
    const auto* nei = Nei_Save();
    switch (id) {
        case RI_OOT_NEI_SHOVEL:
            return nei->shovelOwned;
        case RI_OOT_NEI_DOMINION_ROD:
            return nei->dominionOwned;
        case RI_OOT_NEI_BOMB_ARROWS:
            return nei->bombArrowsOwned;
        case RI_OOT_NEI_MARIO_MASK:
            return nei->marioMaskOwned;
        case RI_OOT_NEI_POKE_BALL:
            return nei->pokeballOwned;
        case RI_GREAT_FAIRY_SWORD:
            return INV_CONTENT(ITEM_SWORD_GREAT_FAIRY) == ITEM_SWORD_GREAT_FAIRY;
        case RI_OOT_NEI_CANE_OF_SOMARIA:
            return Cane_HasSkill(0);
        case RI_OOT_NEI_CANE_SOMARIA_BLOCK:
            return Cane_HasSkill(1);
        case RI_OOT_NEI_CANE_SOMARIA_PLATFORM:
            return Cane_HasSkill(2);
        case RI_OOT_NEI_CANE_PACCI_FLIP:
            return Cane_HasSkill(3);
        case RI_OOT_NEI_CANE_PACCI_STONE:
            return Cane_HasSkill(4);
        case RI_OOT_NEI_CANE_PACCI_ULTRAHAND:
            return Cane_HasSkill(5);
        case RI_OOT_NEI_SLATE_RUNE_BOMB:
            return Slate_RuneOwned(SLATE_RUNE_BOMB);
        case RI_OOT_NEI_SLATE_RUNE_STASIS:
            return Slate_RuneOwned(SLATE_RUNE_STASIS);
        case RI_OOT_NEI_SLATE_RUNE_CRYONIS:
            return Slate_RuneOwned(SLATE_RUNE_CRYONIS);
        case RI_OOT_NEI_SLATE_RUNE_MASTER_CYCLE:
            return Slate_RuneOwned(SLATE_RUNE_MASTER_CYCLE);
        case RI_OOT_NEI_DESIRE_SENSOR:
            return Slate_RuneOwned(SLATE_RUNE_SENSOR);
        case RI_OOT_NEI_SHEIKAH_SLATE:
            return (nei->slateRunesOwned & 31) == 31;
        case RI_OOT_NEI_SEASON_SPRING:
            return nei->seasonsOwned & (1u << SEASON_SPRING);
        case RI_OOT_NEI_SEASON_SUMMER:
            return nei->seasonsOwned & (1u << SEASON_SUMMER);
        case RI_OOT_NEI_SEASON_AUTUMN:
            return nei->seasonsOwned & (1u << SEASON_AUTUMN);
        case RI_OOT_NEI_SEASON_WINTER:
            return nei->seasonsOwned & (1u << SEASON_WINTER);
        case RI_OOT_NEI_ROD_OF_SEASONS:
            return (nei->seasonsOwned & 15) == 15 && Seasons_HasRod();
        case RI_OOT_NEI_WAND_SAND_ROD:
            return nei->wandRodsOwned & (1u << WAND_MODE_SAND);
        case RI_OOT_NEI_WAND_TORNADO_ROD:
            return nei->wandRodsOwned & (1u << WAND_MODE_TORNADO);
        case RI_OOT_NEI_WAND_WATER_ROD:
            return nei->wandRodsOwned & (1u << WAND_MODE_WATER);
        case RI_OOT_NEI_WAND_METEOR_ROD:
            return nei->wandRodsOwned & (1u << WAND_MODE_METEOR);
        case RI_OOT_NEI_WAND_STORM_ROD:
            return nei->wandRodsOwned & (1u << WAND_MODE_STORM);
        case RI_OOT_NEI_WAND_SHADOW_SCEPTER:
            return nei->wandRodsOwned & (1u << WAND_MODE_SCEPTER);
        case RI_OOT_NEI_ELEMENTAL_WAND:
            return (nei->wandRodsOwned & 63) == 63;
        case RI_OOT_NEI_HYLIAS_GRACE:
            return GraceHourglass_IsOwned(ITEM_HYLIAS_GRACE);
        case RI_OOT_NEI_PHANTOM_HOURGLASS:
            return GraceHourglass_IsOwned(EXT_ITEM_PHANTOM_HOURGLASS);
        case RI_OOT_EXT_MAGIC_CAPE:
            return ExtEquip_CapeOwned();
        case RI_OOT_EXT_CANE_OF_BYRNA:
            return ExtEquip_HasItem(0, 1);
        case RI_OOT_EXT_FOUR_SWORD:
            return ExtEquip_HasItem(0, 2);
        case RI_OOT_EXT_TRIDENT:
            return ExtEquip_HasItem(0, 3);
        case RI_OOT_EXT_DIVINE_SHIELD:
            return ExtEquip_HasItem(1, 1);
        case RI_OOT_EXT_SHEIKAH_SHIELD:
            return ExtEquip_HasItem(1, 2);
        case RI_OOT_EXT_CHAMPIONS_TUNIC:
            return ExtEquip_HasItem(2, 1);
        case RI_OOT_EXT_SPIRIT_BREASTPLATE:
            return ExtEquip_HasItem(2, 2);
        case RI_OOT_EXT_WATER_DRAGON_SCALE:
            return ExtEquip_HasItem(2, 3);
        case RI_OOT_EXT_PEGASUS_ANKLET:
            return ExtEquip_HasItem(3, 1);
        case RI_OOT_EXT_CLIMB_BOOTS:
            return ExtEquip_HasItem(3, 2);
        case RI_OOT_EXT_ROC_BOOTS:
            return ExtEquip_HasItem(3, 3);
        case RI_OOT_NEI_ROCS_FEATHER:
            return Nei_GetOwnedItem(SLOT_ROCS) == ITEM_ROCS_FEATHER_SKIJER ||
                   Nei_GetOwnedItem(SLOT_ROCS) == ITEM_ROCS_CAPE;
        case RI_OOT_NEI_ROCS_CAPE:
            return Nei_GetOwnedItem(SLOT_ROCS) == ITEM_ROCS_CAPE;
#define OWN_SLOT(ri, slot, value) \
    case ri:                      \
        return Nei_GetOwnedItem(slot) == value
            OWN_SLOT(RI_OOT_NEI_LANTERN, SLOT_LANTERN, ITEM_LANTERN);
            OWN_SLOT(RI_OOT_NEI_WHIP, SLOT_WHIP, ITEM_WHIP);
            OWN_SLOT(RI_OOT_NEI_BEETLE, SLOT_BEETLE, ITEM_BEETLE);
            OWN_SLOT(RI_OOT_NEI_GUST_JAR, SLOT_GUST_JAR, ITEM_GUST_JAR);
            OWN_SLOT(RI_OOT_NEI_BALL_AND_CHAIN, SLOT_BALL_AND_CHAIN, ITEM_BALL_AND_CHAIN);
            OWN_SLOT(RI_OOT_NEI_FIRE_ROD, SLOT_FIRE_ROD, ITEM_ROD_FIRE);
            OWN_SLOT(RI_OOT_NEI_ICE_ROD, SLOT_ICE_ROD, ITEM_ROD_ICE);
            OWN_SLOT(RI_OOT_NEI_LIGHT_ROD, SLOT_LIGHT_ROD, ITEM_ROD_LIGHT);
            OWN_SLOT(RI_OOT_NEI_SPINNER, SLOT_SPINNER, ITEM_SPINNER);
            OWN_SLOT(RI_OOT_NEI_MOGMA_MITTS, SLOT_MOGMA_MITTS, ITEM_MOGMA_MITTS);
            OWN_SLOT(RI_OOT_NEI_DEKU_LEAF, SLOT_DEKU_LEAF, ITEM_DEKU_LEAF);
            OWN_SLOT(RI_OOT_NEI_SWITCH_HOOK, SLOT_SWITCH_HOOK, ITEM_SWITCH_HOOK);
            OWN_SLOT(RI_OOT_NEI_MINISH_CAP, SLOT_MINISH_CAP, ITEM_MINISH_CAP);
            OWN_SLOT(RI_OOT_NEI_TIME_GATE, SLOT_TIME_GATE, ITEM_TIME_GATE);
            OWN_SLOT(RI_OOT_NEI_ZONAI_PERMAFROST, SLOT_ZONAI_PERMAFROST, ITEM_ZONAI_PERMAFROST);
            OWN_SLOT(RI_OOT_NEI_SHADOW_CRYSTAL, SLOT_SHADOW_CRYSTAL, EXT_ITEM_SHADOW_CRYSTAL);
            OWN_SLOT(RI_OOT_NEI_DEMISE_DESTRUCTION, SLOT_DEMISE_DESTRUCTION, ITEM_DEMISE_DESTRUCTION);
#undef OWN_SLOT
        default:
            return id != RI_UNKNOWN && !item.renewable && !Rando::IsItemObtainable(id);
    }
}

bool Available(const ComboRando::BlackMarket::Item& item) {
    if (Id(item) == RI_UNKNOWN)
        return false;
    if (item.renewable)
        return Inventory_HasEmptyBottle();
    if (std::string_view(item.key) == "wildcard.magic_upgrade")
        return gSaveContext.save.saveInfo.playerData.isMagicAcquired;
    // Native mode getters still enforce medallions/dungeon gates after a grant.
    // Never use the developer ownership toggles, which also grant those rewards.
    return true;
}

bool Grant(const ComboRando::BlackMarket::Item& item) {
    if (Owned(item) || !Available(item))
        return false;
    const auto key = std::string_view(item.key);
    if (key.starts_with("nei.cane.") && key != "nei.cane.byrna") {
        const auto id = Id(item);
        const u8 skill = id == RI_OOT_NEI_CANE_OF_SOMARIA         ? 0
                         : id == RI_OOT_NEI_CANE_SOMARIA_BLOCK    ? 1
                         : id == RI_OOT_NEI_CANE_SOMARIA_PLATFORM ? 2
                         : id == RI_OOT_NEI_CANE_PACCI_FLIP       ? 3
                         : id == RI_OOT_NEI_CANE_PACCI_STONE      ? 4
                                                                  : 5;
        Cane_GiveSkill(skill); // Exact skill bits travel through FleetSync, never progressive FC aliases.
    } else if (key == "nei.bundle.slate") {
        for (u8 i = 0; i < SLATE_RUNE_COUNT; ++i)
            Slate_GrantRune(i);
    } else if (key == "nei.bundle.seasons") {
        Seasons_GrantRod();
        for (u8 i = 0; i < SEASON_COUNT; ++i)
            Seasons_GrantSeason(i);
    } else if (key == "nei.bundle.wand") {
        for (u8 i = 0; i < WAND_MODE_COUNT; ++i)
            Wand_GrantMode(i);
    } else {
        Rando::GiveItem(Id(item)); // fixed identity; never ConvertItem or a check.
    }
    return item.renewable || Owned(item);
}
} // namespace ComboBlackMarketItems
#endif
