// Link the complete MM receipt translation unit to the real donor export. The
// runner supplies both catalogs from production, keeping the game saves private.
#include "2s2h/FleetShipCombo/FleetComboItems.h"
#include "2s2h/FleetShipCombo/FleetComboItemsGlue.h"
#include "2s2h/Rando/ItemReceiptText.h"
#include "sword_element_message.h"
#include <nlohmann/json.hpp>
#include "2s2h/Rando/Rando.h"
#include "2s2h/Rando/StaticData/StaticData.h"
#include "rando/CrossForeign.h"
#include "ComboItemReceiptText.h"
#include "ComboGameMode.h"
extern "C" {
int gComboGameMode = COMBO_GAME_BOTH;
#include "mods/extended_inventory.h"
}
#include <array>
#include <cassert>
#include <cstring>
#include <iostream>
#include <unordered_map>
#include "key_catalog.inc"
extern "C" {
PlayState* gPlayState = nullptr;
SaveContext gSaveContext{};
static NeiSaveData neiSave{};
NeiSaveData* Nei_Save() { return &neiSave; }
const NeiItem* Nei_FindByRg(int16_t) { return nullptr; }
const NeiItem* Nei_FindByItem(int32_t) { return nullptr; }
u8 Nei_BulletBagLevel() { return 0; }
u8 gItemSlots[77]{};
u32 gBitFlags[32] = {1, 2, 4};
u32 gUpgradeMasks[8]{};
u8 gUpgradeShifts[8]{};
u16 Player_GetItemReceiptTextId(s16, s16) { assert(false && "dungeon keys must use the donor randomizer receipt"); return 0; }
int32_t KeyFixture_LiveResolutions();
const char* KeyFixture_Color(const char*);
}
namespace Rando::StaticData {
const std::string& GetCheckDisplayName(RandoCheckId) { static std::string name = "key test check"; return name; }
const char* GetIconTexturePath(RandoItemId) { return nullptr; }
uint8_t GetIconForZMessage(RandoItemId id) { return static_cast<uint8_t>(Items.at(id).getItemId); }
std::string GetItemName(RandoItemId id, bool article, RandoCheckId, bool) {
    const auto& item = Items.at(id);
    return (article ? std::string(item.article) + " " : "") + item.name;
}
bool ShouldShowGetItemCutscene(RandoItemId) { return true; }
}
static uint64_t generation = 1;
static ComboRando::ForeignItem foreign;
namespace Rando::MiscBehavior {
uint64_t ComboRandoGen() { return generation; }
const ComboRando::ForeignItem* MM_LookupForeign(RandoCheckId) { return &foreign; }
}
static std::string Flat(std::string text) {
    for (auto& c : text) if (c == '\x11') c = ' ';
    return text;
}
static bool presenting = false;
static int grants = 0, crossGrants = 0;
static SaveContext expectedPreGrant{};
namespace CustomMessage {
Entry shown;
void SetActiveCustomMessage(std::string text, Entry entry) {
    assert(std::memcmp(&gSaveContext, &expectedPreGrant, sizeof(gSaveContext)) == 0 && "receipt must precede its key grant");
    presenting = true; entry.msg = std::move(text); shown = std::move(entry);
}
void StartTextbox(std::string text, Entry entry) { SetActiveCustomMessage(std::move(text), std::move(entry)); }
std::string RemoveColorCodes(const std::string& text) { return text; }
}
extern "C" int32_t CVarGetInteger(const char*, int32_t fallback) { return fallback; }
extern "C" uint8_t Wand_RandoMode(void) { return 0; }
// Mirror stump delivery has its own grant fixture; key receipts must never call it.
extern "C" bool MagicMirror_CommitStumpRewardToken(void) { assert(false); return false; }
namespace Notification { struct Info { const char* itemIcon; std::string message, suffix; }; void Emit(Info) { assert(false); } }
namespace CustomItem { constexpr int GIVE_ITEM_CUTSCENE = 1; }
static int flags = 1, param = RC_UNKNOWN, param2 = 0;
#define CUSTOM_ITEM_FLAGS flags
#define CUSTOM_ITEM_PARAM param
#define CUSTOM_ITEM_PARAM2 param2
int gMMComboGoalRequired = 0;
int (*gMMComboOtherTriforceCount)() = nullptr;
void MMAnchor_BroadcastCrossItem(int, const char*, const char*) {}
void SaveManager_SaveCurrentForCombo() {}
namespace Rando {
RandoItemId ConvertItem(RandoItemId id, RandoCheckId) { return id; }
RandoItemId CurrentJunkItem(RandoCheckId) { return RI_RUPEE_GREEN; }
void LatchComboForeign(RandoCheckId) {}
const char* ComboForeignLatchedName(RandoCheckId) { return foreign.itemName.c_str(); }
uint8_t ComboForeignMessageIcon(RandoCheckId) { return 0xF5; }
void GiveItem(RandoItemId randoItemId, RandoCheckId) {
    assert(presenting && "grant must follow receipt presentation");
    ++grants;
    switch (randoItemId) {
        /* NATIVE_KEY_GRANTS */
        default: assert(false);
    }
}
namespace MiscBehavior {
static bool queued = true;
static RandoCheckId queuedCheck = RC_UNKNOWN;
std::string BankRewardSourceSuffix(RandoCheckId) { return " (Bank reward)"; }
bool ShouldShowForeignCutscene(RandoCheckId) { return true; }
void OfferTrapItem() { assert(false); }
void SendForeignCheck(RandoCheckId) { assert(presenting); ++crossGrants; }
void BroadcastCheckObtainedIfFirst(RandoCheckId, RandoItemId, bool) {}
std::string GetTrapMessage() { return "trap"; }
/* QUEUE_RECEIVE */
}
}
static void CheckNativeKeys() {
    const RandoItemId ids[] = {RI_WOODFALL_SMALL_KEY, RI_SNOWHEAD_SMALL_KEY, RI_GREAT_BAY_SMALL_KEY, RI_STONE_TOWER_SMALL_KEY,
        RI_WOODFALL_BOSS_KEY, RI_SNOWHEAD_BOSS_KEY, RI_GREAT_BAY_BOSS_KEY, RI_STONE_TOWER_BOSS_KEY};
    const int scenes[] = {DUNGEON_SCENE_INDEX_WOODFALL_TEMPLE, DUNGEON_SCENE_INDEX_SNOWHEAD_TEMPLE,
        DUNGEON_SCENE_INDEX_GREAT_BAY_TEMPLE, DUNGEON_SCENE_INDEX_STONE_TOWER_TEMPLE};
    const uint8_t colors[] = {6, 2, 3, 4}; // Woodfall, Snowhead, Great Bay, Stone Tower.
    for (size_t i = 0; i < std::size(ids); ++i) {
        const auto id = ids[i];
        const auto& item = Rando::StaticData::Items.at(id);
        for (int count : {-1, 0, 3, 9}) {
            DUNGEON_KEY_COUNT(scenes[i % 4]) = count;
            gSaveContext.save.shipSaveInfo.rando.foundDungeonKeys[scenes[i % 4]] = count;
            const auto before = gSaveContext;
            CustomMessage::Entry entry;
            entry.icon = item.getItemId;
            assert(Rando::ApplyItemReceiptText(id, entry) && "native MM dungeon key lacks named donor-parity receipt");
            const std::string expected = "You found " + std::string(item.article) + " " +
                static_cast<char>(colors[i % 4]) + item.name + '\0' + '!';
            assert(Flat(entry.msg) == expected);
            assert(!entry.autoFormat && entry.icon == item.getItemId && !entry.receiptPresentation.singleBox);
            assert(entry.msg.find('\x10') == std::string::npos && "key receipt must stay concise");
            assert(!std::memcmp(&before, &gSaveContext, sizeof(before)));
        }
        const auto checkId = RC_CLOCK_TOWER_ROOF_OCARINA;
        auto& check = RANDO_SAVE_CHECKS[checkId];
        check = {}; check.randoItemId = id; check.eligible = true;
        DUNGEON_KEY_COUNT(scenes[i % 4]) = -1;
        gSaveContext.save.shipSaveInfo.rando.foundDungeonKeys[scenes[i % 4]] = -1;
        expectedPreGrant = gSaveContext;
        param = checkId; flags = 1; presenting = false;
        Actor actor{}; PlayState play{};
        Rando::MiscBehavior::Receive(&actor, &play);
        assert(check.obtained && check.cycleObtained && !check.eligible);
        assert(CustomMessage::shown.msg.find("Bank reward") != std::string::npos && CustomMessage::shown.msg.back() == '\xBF');
        assert(CustomMessage::shown.icon == item.getItemId);
        if (i < 4) {
            assert(DUNGEON_KEY_COUNT(scenes[i % 4]) == 1);
            assert(gSaveContext.save.shipSaveInfo.rando.foundDungeonKeys[scenes[i % 4]] == 1);
        } else assert(CHECK_DUNGEON_ITEM(DUNGEON_BOSS_KEY, scenes[i % 4]));
    }
    assert(grants == 8);
}
static void CheckImportedKeys() {
    const int nativeGrants = grants;
    std::vector<RandoItemId> keys;
    for (const auto& [id, item] : Rando::StaticData::Items)
        if (id >= RI_OOT_SMALL_KEY_BOTTOM_OF_THE_WELL && id <= RI_OOT_SMALL_KEY_WATER_TEMPLE && id != RI_OOT_SMALL_KEY_TREASURE_GAME)
            keys.push_back(id);
        else if (id >= RI_OOT_BOSS_KEY_FIRE_TEMPLE && id <= RI_OOT_BOSS_KEY_WATER_TEMPLE) keys.push_back(id);
    assert(keys.size() == 15);
    for (const auto id : keys) {
        const auto& item = Rando::StaticData::Items.at(id);
        const int fc = FcCombo_ItemForNative(id);
        assert(fc >= 0);
        for (int count : {0, 1, 8}) {
            neiSave.comboObtainedFc[fc] = count;
            const auto before = gSaveContext;
            const auto neiBefore = neiSave;
            CustomMessage::Entry local, imported;
            local.icon = imported.icon = 0xF5;
            assert(Rando::ApplyItemReceiptText(id, local));
            assert(Rando::ApplyForeignItemReceiptText(item.name, imported));
            assert(local.msg == imported.msg && !local.autoFormat && local.icon == 0xF5);
            const auto expected = ComboItemReceiptText::FromNeiMarkup("You found " + std::string(item.article) + " " + KeyFixture_Color(item.name) + item.name + "%w!");
            assert(Flat(local.msg) == expected && "imported key must use the real donor name/article/color");
            assert(local.msg.find('\x10') == std::string::npos && !local.receiptPresentation.singleBox);
            assert(!std::memcmp(&before, &gSaveContext, sizeof(before)) && !std::memcmp(&neiBefore, &neiSave, sizeof(neiSave)));
        }
        foreign.itemName = item.name; foreign.displayName = std::string(item.name) + " (OOT)";
        foreign.itemGame = ComboRando::GAME_OOT; foreign.trap = false;
        auto& check = RANDO_SAVE_CHECKS[RC_CLOCK_TOWER_ROOF_OCARINA];
        check = {}; check.randoItemId = RI_COMBO_FOREIGN; check.eligible = true;
        ++generation; param = RC_CLOCK_TOWER_ROOF_OCARINA; flags = 1; presenting = false;
        expectedPreGrant = gSaveContext;
        Actor actor{}; PlayState play{};
        const int crossBefore = crossGrants;
        Rando::MiscBehavior::Receive(&actor, &play);
        assert(crossGrants == crossBefore + 1 && grants == nativeGrants && check.obtained);
        assert(CustomMessage::shown.icon == 0xF5 && Flat(CustomMessage::shown.msg).find(item.name) != std::string::npos);
        assert(CustomMessage::shown.msg.find("Bank reward") != std::string::npos && CustomMessage::shown.msg.back() == '\xBF');
        check.cycleObtained = false; check.eligible = true; param = RC_CLOCK_TOWER_ROOF_OCARINA;
        expectedPreGrant = gSaveContext; presenting = false;
        Rando::MiscBehavior::Receive(&actor, &play);
        assert(crossGrants == crossBefore + 1 && "cycle re-collection must not deliver another OoT key");
    }
    assert(KeyFixture_LiveResolutions() == 0);
}
int main(int argc, char** argv) {
    gSaveContext.fileNum = 0;
    gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
    if (argc < 2 || std::strcmp(argv[1], "--imports-only")) CheckNativeKeys();
    CheckImportedKeys();
    CustomMessage::Entry chest;
    chest.icon = 0xF5;
    assert(Rando::ApplyItemReceiptText(RI_OOT_SMALL_KEY_TREASURE_GAME, chest));
    assert(Flat(chest.msg) == std::string("You found a ") + '\x02' + "Chest Game Small Key" + '\0' + '!');
    CustomMessage::Entry excluded;
    assert(!Rando::ApplyItemReceiptText(RI_SKELETON_KEY, excluded));
    std::cout << "PASS concise native MM / imported OoT key receipts, donor colors, icons, immutable counters, queue grant timing and cycle re-collection\n";
}
