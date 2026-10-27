#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include "combo/menu/ComboFairyBottle.h"
#include "combo/menu/ComboItemDrawABI.h"

using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using u8 = uint8_t;
using f32 = float;
struct PlayState {};
struct Gfx {};
#define ARRAY_COUNT(x) (sizeof(x) / sizeof((x)[0]))

std::string selectedCallback;
std::unordered_set<std::string> selectedMods;
bool bundledAvailable = true;
int ResourceMgr_IsModAsset(const char* path) { return selectedMods.count(path); }
int ResourceMgr_IsModAssetForGame(const char*, const char* path) {
    return selectedMods.count(std::string("__OTR__") + path);
}
int ResourceMgr_IsCustomAssetForGame(const char*, const char*) { return 0; }
int ResourceMgr_FileExists(const char* path) { return bundledAvailable && strstr(path, "combo_bottle_gi"); }
Gfx* ResourceMgr_LoadGfxByName(const char*) { return nullptr; }
int ComboBottleGi_HasPotionRecipe(const char*, const char*, const char*, Gfx* (*)(const char*)) { return 0; }

/* PRODUCTION_NATIVE */

namespace oot {
#include "soh/soh/Enhancements/item-tables/ItemTableTypes.h"
#define RANDO_ENUM_BEGIN(name) enum name {
#define RANDO_ENUM_ITEM(name) name,
#define RANDO_ENUM_END(name) };
#include "soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerGet.h"
#undef RANDO_ENUM_BEGIN
#undef RANDO_ENUM_ITEM
#undef RANDO_ENUM_END

using ItemType = int;
using LogicVal = int;
using RandomizerHintTextKey = int;
constexpr int ITEMTYPE_ITEM = 0, ITEMTYPE_SHOP = 1, LOGIC_BOTTLES = 0, LOGIC_FAIRY_ACCESS = 1;
constexpr int RHT_BOTTLE_WITH_MILK = 0, RHT_BOTTLE_WITH_FAIRY = 1;
constexpr int MOD_NONE = 0, MOD_RANDOMIZER = 1, TABLE_VANILLA = 0, TABLE_RANDOMIZER = 1;
constexpr int TEXT_RANDOMIZER_CUSTOM_ITEM = 0x9000;
struct Text {
    Text() = default;
    Text(const char*, const char* = "", const char* = "") {}
};
// Only the Item constructor's fields are needed; its body and GetItemEntry
// definition are production code, so grant/draw identity follows the real ABI.
struct Item {
    RandomizerGet randomizerGet{};
    Text name;
    ItemType type{};
    int16_t getItemId{};
    bool advancement{};
    LogicVal logicVal{};
    RandomizerHintTextKey hintKey{};
    GetItemCategory category{};
    Text article;
    std::string color;
    uint16_t price{};
    std::shared_ptr<GetItemEntry> giEntry;
    Item() = default;
    Item(RandomizerGet, Text, ItemType, int16_t, bool, LogicVal, RandomizerHintTextKey, uint16_t, uint16_t,
         uint16_t, uint16_t, uint16_t, int16_t, GetItemCategory, uint16_t, Text = {}, std::string = "%g",
         uint16_t = 0);
};
std::map<RandomizerGet, Item> itemTable;
/* PRODUCTION_CATALOG */
}

struct MmReceipt { const char* name; int gid; };
/* PRODUCTION_MM_RECEIPTS */

static void CheckOotRecipe(const oot::GetItemEntry& entry) {
    void* resources[8]{};
    int xlu = -1, kind = -1;
    float scale = 0;
    uint8_t colors[16]{};
    const int count = oot::GetItem_GetDrawTableEntry(entry.gid, resources, 8, &xlu, &scale, &kind, colors);
    printf("OoT receipt object=%u gid=%u count=%d kind=%d\n", entry.objectId, entry.gid, count, kind);
    fflush(stdout);
    assert(kind == CW_DRAW_KIND_FAIRY && "fairy reward still exports an empty bottle to the foreign host");
    assert(count == 3 && xlu == 1 && resources[2] == oot::gGiFairyContainerContentsDL);
    assert(entry.objectId == oot::OBJECT_GI_SOUL && "fairy receipt must load the object for its draw recipe");
    assert(entry.gi == entry.gid + 1 && entry.collectable && entry.getItemFrom == oot::ITEM_FROM_NPC);
    assert(entry.drawFunc == nullptr && entry.field == 0x80);
    assert(!strcmp((const char*)resources[0], "__OTR__objects/combo_bottle_gi/EmptyOpaque"));
    assert(!strcmp((const char*)resources[1], "__OTR__objects/combo_bottle_gi/FairyXlu"));
    oot::sDrawItemTable[entry.gid].drawFunc(nullptr, entry.gid);
    assert(selectedCallback == "GetItem_DrawFairy" && "native receipt must invoke the contents renderer");
}

static void CheckOotReceipt(oot::RandomizerGet rg) {
    const auto& entry = *oot::itemTable.at(rg).giEntry;
    CheckOotRecipe(entry);
    if (rg == oot::RG_BOTTLE_WITH_FAIRY) {
        assert(entry.itemId == oot::RG_BOTTLE_WITH_FAIRY && entry.drawItemId == oot::RG_BOTTLE_WITH_FAIRY);
        assert(entry.getItemId == oot::RG_BOTTLE_WITH_FAIRY && entry.tableId == oot::TABLE_RANDOMIZER);
        assert(entry.modIndex == oot::MOD_RANDOMIZER && entry.drawModIndex == oot::MOD_RANDOMIZER);
        assert(entry.getItemCategory == oot::ITEM_CATEGORY_MAJOR && entry.textId == oot::TEXT_RANDOMIZER_CUSTOM_ITEM);
    } else {
        assert(entry.itemId == oot::ITEM_FAIRY && entry.drawItemId == oot::ITEM_FAIRY);
        assert(entry.getItemId == oot::GI_FAIRY && entry.tableId == oot::TABLE_VANILLA);
        assert(entry.modIndex == oot::MOD_NONE && entry.drawModIndex == oot::MOD_NONE);
        assert(entry.getItemCategory == oot::ITEM_CATEGORY_JUNK && entry.textId == 0x46);
        assert(oot::itemTable.at(rg).price == 50);
    }
}

int main(int argc, char** argv) {
    assert(argc == 2);
    oot::InitReceipts();
    if (!strcmp(argv[1], "randomizer-bottle")) {
        CheckOotReceipt(oot::RG_BOTTLE_WITH_FAIRY);
        puts("PASS OoT fairy bottle catalog -> native callback and foreign export; grant identity preserved");
        return 0;
    }
    if (!strcmp(argv[1], "randomizer-refill")) {
        CheckOotReceipt(oot::RG_BUY_FAIRYS_SPIRIT);
        puts("PASS OoT fairy refill catalog -> native callback and foreign export; grant identity preserved");
        return 0;
    }
    const auto& empty = *oot::itemTable.at(oot::RG_EMPTY_BOTTLE).giEntry;
    assert(empty.objectId == oot::OBJECT_GI_BOTTLE && empty.gid == oot::GID_BOTTLE && empty.itemId == oot::ITEM_BOTTLE);
    if (!strcmp(argv[1], "vanilla-fairy")) {
        oot::ItemTableManager manager;
        oot::ItemTableManager::Instance = &manager;
        oot::VanillaItemTable_Init();
        auto entry = manager.RetrieveItemEntry(oot::MOD_NONE, oot::GI_FAIRY);
        CheckOotRecipe(entry);
        assert(entry.itemId == oot::ITEM_FAIRY && entry.drawItemId == oot::ITEM_FAIRY);
        assert(entry.getItemId == oot::GI_FAIRY && entry.tableId == oot::TABLE_VANILLA);
        assert(entry.modIndex == oot::MOD_NONE && entry.drawModIndex == oot::MOD_NONE);
        assert(entry.getItemCategory == oot::ITEM_CATEGORY_JUNK && entry.textId == 0x46);
        entry = manager.RetrieveItemEntry(oot::MOD_NONE, oot::GI_BOTTLE);
        assert(entry.itemId == oot::ITEM_BOTTLE && entry.objectId == oot::OBJECT_GI_BOTTLE && entry.gid == oot::GID_BOTTLE);
        puts("PASS real VanillaItemTable_Init/RetrieveItemEntry GI_FAIRY -> fairy recipe; refill and empty-bottle identity preserved");
        return 0;
    }
    assert(!strcmp(argv[1], "mm"));

    for (const auto& receipt : mmReceipts) {
        void* resources[8]{};
        int xlu = -1, scroll = 0, kind = -1;
        float scale = 0;
        int count = mm::GetItem_GetDrawTableEntry(receipt.gid, resources, 8, &xlu, &scale, &scroll, &kind);
        assert(xlu == 1 && resources[2] != nullptr);
        assert(!strcmp((const char*)resources[0], "__OTR__objects/combo_bottle_gi/EmptyOpaque"));
        assert(!strcmp((const char*)resources[1], "__OTR__objects/combo_bottle_gi/FairyXlu"));
        if (!strcmp(receipt.name, "RI_OOT_BOTTLE_FAIRY")) {
            assert(count == 4 && kind == CW_DRAW_KIND_MM_FAIRY_CONTAINER);
            assert(resources[2] == mm::gGiFairyBottleContentsDL && resources[3] == &mm::gGiFairyBottleBillboardRotMtx);
        } else {
            assert(count == 3 && kind == CW_DRAW_KIND_MM_FAIRY_BOTTLE);
            assert(resources[2] == mm::gGiFairyContainerContentsDL);
        }
    }
    puts("PASS MM native fairy/refill catalog -> complete fairy exports for the OoT host");
}
