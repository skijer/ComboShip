#ifdef COMBO_BUILD
#include "ComboBlackMarketItems.h"
#include "soh/Enhancements/randomizer/static_data.h"
extern "C" {
#include "global.h"
#include "mods/nei_save.h"
#include "mods/extended_inventory.h"
#include "mods/extended_equipment.h"
u8 Cane_GiveSkill(u8 skill);
u8 Cane_HasSkill(u8 skill);
u16 Randomizer_Item_Give(PlayState*, GetItemEntry);
}

namespace ComboBlackMarketOotItems {
using ComboRando::BlackMarket::Item;
RandomizerGet Id(const Item& item) {
    static constexpr std::pair<std::string_view, RandomizerGet> ids[] = {
        { "nei.shovel", RG_SHOVEL },
        { "nei.lantern", RG_LANTERN },
        { "nei.whip", RG_WHIP },
        { "nei.beetle", RG_BEETLE },
        { "nei.season.spring", RG_SEASON_SPRING },
        { "nei.season.summer", RG_SEASON_SUMMER },
        { "nei.season.autumn", RG_SEASON_AUTUMN },
        { "nei.season.winter", RG_SEASON_WINTER },
        { "nei.gust_jar", RG_GUST_JAR },
        { "nei.ball_chain", RG_BALL_AND_CHAIN },
        { "nei.fire_rod", RG_FIRE_ROD },
        { "nei.ice_rod", RG_ICE_ROD },
        { "nei.light_rod", RG_LIGHT_ROD },
        { "nei.bomb_arrows", RG_BOMB_ARROWS },
        { "nei.spinner", RG_SPINNER },
        { "nei.goddess_shield", RG_EXT_DIVINE_SHIELD },
        { "nei.kite_shield", RG_EXT_SHEIKAH_SHIELD },
        { "nei.pegasus_boots", RG_EXT_PEGASUS_ANKLET },
        { "nei.climb_boots", RG_EXT_CLIMB_BOOTS },
        { "nei.sages_tunic", RG_EXT_WATER_DRAGON_SCALE },
        { "nei.magic_tunic", RG_EXT_SPIRIT_BREASTPLATE },
        { "nei.champions_tunic", RG_EXT_CHAMPIONS_TUNIC },
        { "nei.magic_cape", RG_EXT_MAGIC_CAPE },
        { "nei.four_sword", RG_EXT_FOUR_SWORD },
        { "nei.trident", RG_EXT_TRIDENT },
        { "nei.cane.byrna", RG_EXT_CANE_OF_BYRNA },
        { "nei.mogma_mitts", RG_MOGMA_MITTS },
        { "nei.deku_leaf", RG_DEKU_LEAF },
        { "nei.rocs_feather", RG_ROCS_FEATHER },
        { "nei.rocs_cape", RG_ROCS_CAPE },
        { "nei.rocs_boots", RG_EXT_ROC_BOOTS },
        { "nei.switch_hook", RG_SWITCH_HOOK },
        { "nei.minish_cap", RG_MINISH_CAP },
        { "nei.dominion_rod", RG_DOMINION_ROD },
        { "nei.hylias_grace", RG_HYLIAS_GRACE },
        { "nei.phantom_hourglass", RG_PHANTOM_HOURGLASS },
        { "nei.time_gate", RG_TIME_GATE },
        { "nei.zonai_timer", RG_ZONAI_PERMAFROST },
        { "nei.shadow_crystal", RG_SHADOW_CRYSTAL },
        { "nei.demise", RG_DEMISE_DESTRUCTION },
        { "nei.rune.remote_bomb", RG_SLATE_RUNE_BOMB },
        { "nei.rune.master_cycle", RG_SLATE_RUNE_MASTER_CYCLE },
        { "nei.rune.stasis", RG_SLATE_RUNE_STASIS },
        { "nei.rune.cryonis", RG_SLATE_RUNE_CRYONIS },
        { "nei.rune.sensor", RG_DESIRE_SENSOR },
        { "nei.cane.statues", RG_CANE_OF_SOMARIA },
        { "nei.cane.blocks", RG_CANE_SOMARIA_BLOCK },
        { "nei.cane.trirod", RG_CANE_SOMARIA_PLATFORM },
        { "nei.cane.flip", RG_CANE_PACCI_FLIP },
        { "nei.cane.lift", RG_CANE_PACCI_STONE },
        { "nei.cane.ultrahand", RG_CANE_PACCI_ULTRAHAND },
        { "nei.wand.sand", RG_WAND_SAND_ROD },
        { "nei.wand.tornado", RG_WAND_TORNADO_ROD },
        { "nei.wand.water", RG_WAND_WATER_ROD },
        { "nei.wand.meteor", RG_WAND_METEOR_ROD },
        { "nei.wand.storm", RG_WAND_STORM_ROD },
        { "nei.wand.shadow", RG_WAND_SHADOW_SCEPTER },
        { "nei.bundle.slate", RG_SHEIKAH_SLATE },
        { "nei.bundle.seasons", RG_ROD_OF_SEASONS },
        { "nei.bundle.wand", RG_ELEMENTAL_WAND },
        { "wildcard.mario_mask", RG_MARIO_MASK },
        { "wildcard.poke_ball", RG_POKEBALL },
        { "wildcard.magic_upgrade", RG_MAGIC_DOUBLE },
        { "wildcard.double_defense", RG_DOUBLE_DEFENSE }
    };
    for (const auto& [key, id] : ids)
        if (key == item.key)
            return id;
    return RG_NONE;
}

bool Owned(const Item& item) {
    const auto id = Id(item);
    const auto* nei = Nei_Save();
    switch (id) {
        case RG_SHOVEL:
            return nei->shovelOwned || Nei_GetOwnedItem(SLOT_SHOVEL) == ITEM_SHOVEL;
        case RG_DOMINION_ROD:
            return nei->dominionOwned || Nei_GetOwnedItem(SLOT_SHOVEL) == ITEM_DOMINION_ROD ||
                   Nei_GetOwnedItem(SLOT_ROD_OF_SEASONS) == ITEM_DOMINION_ROD; // legacy pre-relayout slot
        case RG_BOMB_ARROWS:
            return nei->bombArrowsOwned;
        case RG_MARIO_MASK:
            return Flags_GetRandomizerInf(RAND_INF_OBTAINED_MARIO_MASK);
        case RG_POKEBALL:
            return nei->pokeballOwned;
        case RG_ROCS_FEATHER:
            return Nei_GetOwnedItem(SLOT_ROCS) == ITEM_ROCS_FEATHER_SKIJER ||
                   Nei_GetOwnedItem(SLOT_ROCS) == ITEM_ROCS_CAPE;
        case RG_ROCS_CAPE:
            return Nei_GetOwnedItem(SLOT_ROCS) == ITEM_ROCS_CAPE;
        case RG_HYLIAS_GRACE:
            return GraceHourglass_IsOwned(ITEM_HYLIAS_GRACE);
        case RG_PHANTOM_HOURGLASS:
            return GraceHourglass_IsOwned(EXT_ITEM_PHANTOM_HOURGLASS);
        case RG_MAGIC_DOUBLE:
            return gSaveContext.isDoubleMagicAcquired;
        case RG_DOUBLE_DEFENSE:
            return gSaveContext.isDoubleDefenseAcquired;
        case RG_CANE_OF_SOMARIA:
            return Cane_HasSkill(0);
        case RG_CANE_SOMARIA_BLOCK:
            return Cane_HasSkill(1);
        case RG_CANE_SOMARIA_PLATFORM:
            return Cane_HasSkill(2);
        case RG_CANE_PACCI_FLIP:
            return Cane_HasSkill(3);
        case RG_CANE_PACCI_STONE:
            return Cane_HasSkill(4);
        case RG_CANE_PACCI_ULTRAHAND:
            return Cane_HasSkill(5);
        case RG_SLATE_RUNE_BOMB:
            return Slate_RuneOwned(SLATE_RUNE_BOMB);
        case RG_SLATE_RUNE_STASIS:
            return Slate_RuneOwned(SLATE_RUNE_STASIS);
        case RG_SLATE_RUNE_CRYONIS:
            return Slate_RuneOwned(SLATE_RUNE_CRYONIS);
        case RG_SLATE_RUNE_MASTER_CYCLE:
            return Slate_RuneOwned(SLATE_RUNE_MASTER_CYCLE);
        case RG_DESIRE_SENSOR:
            return Slate_RuneOwned(SLATE_RUNE_SENSOR);
        case RG_SHEIKAH_SLATE:
            return (nei->slateRunesOwned & 31) == 31;
        case RG_SEASON_SPRING:
            return nei->seasonsOwned & (1u << SEASON_SPRING);
        case RG_SEASON_SUMMER:
            return nei->seasonsOwned & (1u << SEASON_SUMMER);
        case RG_SEASON_AUTUMN:
            return nei->seasonsOwned & (1u << SEASON_AUTUMN);
        case RG_SEASON_WINTER:
            return nei->seasonsOwned & (1u << SEASON_WINTER);
        case RG_ROD_OF_SEASONS:
            return (nei->seasonsOwned & 15) == 15 && Seasons_HasRod();
        case RG_WAND_SAND_ROD:
            return nei->wandRodsOwned & (1u << WAND_MODE_SAND);
        case RG_WAND_TORNADO_ROD:
            return nei->wandRodsOwned & (1u << WAND_MODE_TORNADO);
        case RG_WAND_WATER_ROD:
            return nei->wandRodsOwned & (1u << WAND_MODE_WATER);
        case RG_WAND_METEOR_ROD:
            return nei->wandRodsOwned & (1u << WAND_MODE_METEOR);
        case RG_WAND_STORM_ROD:
            return nei->wandRodsOwned & (1u << WAND_MODE_STORM);
        case RG_WAND_SHADOW_SCEPTER:
            return nei->wandRodsOwned & (1u << WAND_MODE_SCEPTER);
        case RG_ELEMENTAL_WAND:
            return (nei->wandRodsOwned & 63) == 63;
        case RG_EXT_MAGIC_CAPE:
            return ExtEquip_CapeOwned();
        case RG_EXT_CANE_OF_BYRNA:
            return ExtEquip_HasItem(0, 1);
        case RG_EXT_FOUR_SWORD:
            return ExtEquip_HasItem(0, 2);
        case RG_EXT_TRIDENT:
            return ExtEquip_HasItem(0, 3);
        case RG_EXT_DIVINE_SHIELD:
            return ExtEquip_HasItem(1, 1);
        case RG_EXT_SHEIKAH_SHIELD:
            return ExtEquip_HasItem(1, 2);
        case RG_EXT_CHAMPIONS_TUNIC:
            return ExtEquip_HasItem(2, 1);
        case RG_EXT_SPIRIT_BREASTPLATE:
            return ExtEquip_HasItem(2, 2);
        case RG_EXT_WATER_DRAGON_SCALE:
            return ExtEquip_HasItem(2, 3);
        case RG_EXT_PEGASUS_ANKLET:
            return ExtEquip_HasItem(3, 1);
        case RG_EXT_CLIMB_BOOTS:
            return ExtEquip_HasItem(3, 2);
        case RG_EXT_ROC_BOOTS:
            return ExtEquip_HasItem(3, 3);
        default: {
            const auto* row = Nei_FindByRg(id);
            return row && row->slot != NEI_NO_SLOT && row->item != NEI_NO_ITEM &&
                   (Nei_GetOwnedItem(row->slot) == row->item ||
                    (id == RG_ROCS_FEATHER && Nei_GetOwnedItem(row->slot) == ITEM_ROCS_CAPE));
        }
    }
}

bool Available(const Item& item) {
    return ComboRando::BlackMarket::Eligible(item, ComboRando::BlackMarket::Games::Oot) && Id(item) != RG_NONE &&
           (std::string_view(item.key) != "wildcard.magic_upgrade" || gSaveContext.isMagicAcquired);
}

void LatchToolOwnership() {
    // Native pause migration normally latches these flags. The shop may grant
    // both shared-cell tools, or replace the legacy rod slot with Seasons,
    // before that pause code ever runs. Cache only physically owned tools.
    auto* nei = Nei_Save();
    nei->shovelOwned |= Nei_GetOwnedItem(SLOT_SHOVEL) == ITEM_SHOVEL;
    nei->dominionOwned |= Nei_GetOwnedItem(SLOT_SHOVEL) == ITEM_DOMINION_ROD ||
                          Nei_GetOwnedItem(SLOT_ROD_OF_SEASONS) == ITEM_DOMINION_ROD;
}

bool Grant(const Item& item) {
    if (Owned(item) || !Available(item))
        return false;
    LatchToolOwnership();
    const auto key = std::string_view(item.key);
    if (key.starts_with("nei.cane.") && key != "nei.cane.byrna") {
        const auto id = Id(item);
        const u8 skill = id == RG_CANE_OF_SOMARIA         ? 0
                         : id == RG_CANE_SOMARIA_BLOCK    ? 1
                         : id == RG_CANE_SOMARIA_PLATFORM ? 2
                         : id == RG_CANE_PACCI_FLIP       ? 3
                         : id == RG_CANE_PACCI_STONE      ? 4
                                                          : 5;
        Cane_GiveSkill(skill);
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
    } else if (key == "nei.rocs_feather") {
        // RG_ROCS_FEATHER is SoH's different, vanilla-slot feather. The NEI
        // market promises the page-two feather, never the progressive chain.
        ExtInv_SetItemById(ITEM_ROCS_FEATHER_SKIJER);
    } else {
        const auto entry = Rando::StaticData::RetrieveItem(Id(item)).GetGIEntryUnresolved();
        if (!entry || entry->modIndex != MOD_RANDOMIZER)
            return false;
        Randomizer_Item_Give(gPlayState, *entry); // exact, frozen native RG row; no progressive resolver/check
    }
    LatchToolOwnership();
    return Owned(item);
}
} // namespace ComboBlackMarketOotItems
#endif
