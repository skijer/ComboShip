#include "global.h"
#include "rando/BlackMarket.h"
#include "soh/Enhancements/randomizer/randomizerEnums.h"
#include "soh/Enhancements/item-tables/ItemTableTypes.h"
extern "C" {
#include "mods/nei_save.h"
#include "mods/extended_inventory.h"
#include "mods/extended_equipment.h"
}
#include <cassert>
#include <cstdio>
#include <memory>
using ComboRando::BlackMarket::Item;
SaveContext gSaveContext{};
PlayState* gPlayState;
NeiSaveData nei;
int nativeGrants, lastNative;
extern "C" NeiSaveData* Nei_Save() { return &nei; }
extern "C" uint16_t Nei_GetOwnedItem(uint8_t slot) { return nei.ownedItems[slot - 24]; }
extern "C" u8 Cane_HasSkill(u8 skill) { return nei.caneSkills & (1u << skill); }
extern "C" u8 Cane_GiveSkill(u8 skill) { nei.caneSkills |= 1u << skill; return 1; }
extern "C" u8 Slate_RuneOwned(u8 rune) { return nei.slateRunesOwned & (1u << rune); }
extern "C" void Slate_GrantRune(u8 rune) { nei.slateRunesOwned |= 1u << rune; }
extern "C" u8 Seasons_HasRod() { return nei.seasonsOwned != 0; }
extern "C" void Seasons_GrantRod() { nei.ownedItems[SLOT_ROD_OF_SEASONS - 24] = EXT_ITEM_ROD_OF_SEASONS; }
extern "C" void Seasons_GrantSeason(u8 season) { nei.seasonsOwned |= 1u << season; }
extern "C" void Wand_GrantMode(u8 mode) { nei.wandRodsOwned |= 1u << mode; }
extern "C" u8 GraceHourglass_IsOwned(u16) { return 0; }
extern "C" u8 ExtEquip_CapeOwned() { return 0; }
extern "C" u8 ExtEquip_HasItem(s16 type, u8 index) {
    return nei.extEquipOwnedBits & (1u << (16 + type * 3 + index - 1));
}
extern "C" s32 Flags_GetRandomizerInf(RandomizerInf) { return 0; }
extern "C" u8 ExtInv_GetItemSlot(u16 item) {
    assert(item == ITEM_ROCS_FEATHER_SKIJER);
    return SLOT_ROCS;
}
extern "C" void Nei_SetOwnedItem(u8 slot, u16 item) { nei.ownedItems[slot - 24] = item; }
extern "C" void ItemGrantAudit_Begin(const char*, int, int, int) {}
extern "C" void ItemGrantAudit_End() {}
extern "C" const NeiItem* Nei_FindByRg(int16_t rg) {
    static NeiItem lantern{}, shovel{}, dominion{};
    lantern.item = ITEM_LANTERN; lantern.slot = SLOT_LANTERN;
    shovel.item = ITEM_SHOVEL; shovel.slot = SLOT_SHOVEL;
    dominion.item = ITEM_DOMINION_ROD; dominion.slot = SLOT_SHOVEL;
    if (rg == RG_SHOVEL) return &shovel;
    if (rg == RG_DOMINION_ROD) return &dominion;
    return rg == RG_LANTERN ? &lantern : nullptr;
}
namespace Rando::StaticData {
struct NativeRow {
    std::shared_ptr<GetItemEntry> entry;
    std::shared_ptr<GetItemEntry> GetGIEntryUnresolved() const { return entry; }
};
NativeRow RetrieveItem(RandomizerGet id) {
    auto entry = std::make_shared<GetItemEntry>();
    entry->modIndex = MOD_RANDOMIZER; entry->getItemId = id;
    return { entry };
}
}
extern "C" u16 Randomizer_Item_Give(PlayState*, GetItemEntry entry) {
    ++nativeGrants; lastNative = entry.getItemId;
    if (entry.getItemId == RG_LANTERN) nei.ownedItems[SLOT_LANTERN - 24] = ITEM_LANTERN;
    if (entry.getItemId == RG_SHOVEL) nei.ownedItems[SLOT_SHOVEL - 24] = ITEM_SHOVEL;
    if (entry.getItemId == RG_DOMINION_ROD) nei.ownedItems[SLOT_SHOVEL - 24] = ITEM_DOMINION_ROD;
    return 0;
}
/* PRODUCTION */
void Reset() {
    nei = {}; for (auto& item : nei.ownedItems) item = ITEM_NONE;
    nativeGrants = lastNative = 0; gSaveContext = {};
}
int main() {
    using namespace ComboRando::BlackMarket;
    for (const auto& item : Catalog()) {
        if (Eligible(item, Games::Oot)) assert(Id(item) != RG_NONE);
        else assert(Id(item) == RG_NONE && !Available(item));
    }
    static constexpr const char* cane[] = { "nei.cane.statues", "nei.cane.blocks", "nei.cane.trirod",
        "nei.cane.flip", "nei.cane.lift", "nei.cane.ultrahand" };
    for (u8 skill = 0; skill < 6; ++skill) {
        Reset(); const auto& item = *Find(cane[skill]);
        assert(Grant(item) && Owned(item));
        assert(nei.caneSkills == (1u << skill) && nativeGrants == 0);
        assert(!Grant(item));
    }
    Reset(); assert(Grant(*Find("nei.bundle.slate")) && nei.slateRunesOwned == 31 && nativeGrants == 0);
    Reset(); assert(Grant(*Find("nei.bundle.seasons")) && nei.seasonsOwned == 15 && nativeGrants == 0);
    Reset(); assert(Grant(*Find("nei.bundle.wand")) && nei.wandRodsOwned == 63 && nativeGrants == 0);
    Reset(); const auto nayru = gSaveContext.inventory.items[SLOT_NAYRUS_LOVE];
    assert(Grant(*Find("nei.rocs_feather")) && nativeGrants == 0);
    assert(Nei_GetOwnedItem(SLOT_ROCS) == ITEM_ROCS_FEATHER_SKIJER);
    assert(gSaveContext.inventory.items[SLOT_NAYRUS_LOVE] == nayru);
    nei.ownedItems[SLOT_ROCS - 24] = ITEM_ROCS_CAPE;
    assert(Owned(*Find("nei.rocs_feather")) && !Grant(*Find("nei.rocs_feather")));
    Reset(); assert(Grant(*Find("nei.lantern")) && lastNative == RG_LANTERN && nativeGrants == 1);
    Reset(); assert(Grant(*Find("nei.shovel")) && lastNative == RG_SHOVEL && nativeGrants == 1);
    assert(nei.shovelOwned && Owned(*Find("nei.shovel")));
    Reset(); assert(Grant(*Find("nei.dominion_rod")) && lastNative == RG_DOMINION_ROD && nativeGrants == 1);
    assert(nei.dominionOwned && Owned(*Find("nei.dominion_rod")));
    for (const bool shovelFirst : { false, true }) {
        Reset();
        const auto& first = *Find(shovelFirst ? "nei.shovel" : "nei.dominion_rod");
        const auto& second = *Find(shovelFirst ? "nei.dominion_rod" : "nei.shovel");
        assert(Grant(first) && Grant(second));
        assert(nei.shovelOwned && nei.dominionOwned);
        assert(Owned(first) && Owned(second));
        assert(nativeGrants == 2 && !Grant(first) && !Grant(second));
    }
    Reset(); nei.ownedItems[SLOT_ROD_OF_SEASONS - 24] = ITEM_DOMINION_ROD;
    assert(Owned(*Find("nei.dominion_rod")));
    assert(Grant(*Find("nei.bundle.seasons")));
    assert(nei.dominionOwned && Owned(*Find("nei.dominion_rod")));
    assert(!Available(*Find("wildcard.magic_upgrade")));
    gSaveContext.isMagicAcquired = 1;
    assert(Available(*Find("wildcard.magic_upgrade")));
    std::puts("PASS OoT native identity coverage, exact cane/bundles/NEI feather and fixed native grant dispatch");
}
