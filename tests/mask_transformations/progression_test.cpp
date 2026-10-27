// Actual progression module with config, completion, save and UI boundaries replaced.
#include "z64item.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
using u8 = uint8_t;
using u16 = uint16_t;
using s32 = int32_t;
#define RANDO_ENUM_BEGIN(name) enum name {
#define RANDO_ENUM_ITEM(name) name,
#define RANDO_ENUM_END(name) };
#include "soh/Enhancements/randomizer/randomizerEnums/RandomizerInf.h"
#include "soh/Enhancements/randomizer/randomizerEnums/RandomizerSettingKey.h"
/* PRODUCTION_DUNGEON_FLAGS */
struct PlayState {};
struct { uint8_t fileNum = 0; uint8_t tradeItem = ITEM_NONE; } gSaveContext;
#define INV_CONTENT(item) gSaveContext.tradeItem
static std::unordered_map<std::string, int> cvars;
static std::set<int> events, tradeFlags, masks;
static bool isRando = false;
static std::set<int> seedOptions;
#define IS_RANDO isRando
#define RAND_GET_OPTION(option) seedOptions.count(option)
static std::vector<std::string> notifications;
int CVarGetInteger(const char* name, int fallback) {
    auto it = cvars.find(name);
    return it == cvars.end() ? fallback : it->second;
}
s32 Flags_GetEventChkInf(s32 flag) { return events.count(flag); }
s32 Flags_GetRandomizerInf(RandomizerInf flag) { return tradeFlags.count(flag); }
void Flags_SetRandomizerInf(RandomizerInf flag) { tradeFlags.insert(flag); }
int32_t ExtInv_HasMmMask(uint16_t item) { return masks.count(item); }
void ExtInv_SetItemById(uint16_t item) { masks.insert(item); }
namespace Notification {
struct Options { std::string message; float remainingTime = 0.0f; };
void Emit(Options options) { notifications.push_back(options.message); }
}
/* PRODUCTION_PROGRESSION */

static void reset() {
    cvars.clear(); events.clear(); masks.clear(); tradeFlags.clear(); notifications.clear();
    isRando = false; seedOptions.clear();
    gSaveContext.fileNum = 0;
    gSaveContext.tradeItem = ITEM_NONE;
}

int main() {
    PlayState play;
    const struct { int flag; std::set<int> items; int ootItem; RandomizerInf tradeFlag; } gates[] = {
        {EVENTCHKINF_USED_DEKU_TREE_BLUE_WARP, {ITEM_MM_MASK_DEKU, ITEM_MM_MASK_KEATON},
         ITEM_MASK_KEATON, RAND_INF_CHILD_TRADES_HAS_MASK_KEATON},
        {EVENTCHKINF_USED_DODONGOS_CAVERN_BLUE_WARP, {ITEM_MM_MASK_GORON},
         ITEM_MASK_GORON, RAND_INF_CHILD_TRADES_HAS_MASK_GORON},
        {EVENTCHKINF_USED_JABU_JABUS_BELLY_BLUE_WARP, {ITEM_MM_MASK_ZORA},
         ITEM_MASK_ZORA, RAND_INF_CHILD_TRADES_HAS_MASK_ZORA},
    };
    // Missing/disabled setting preserves all existing pickups and saves.
    for (bool explicitOff : {false, true}) {
        reset();
        if (explicitOff) cvars["gMods.TransformMasks.ChildDungeonGates"] = 0;
        for (const auto& gate : gates) events.insert(gate.flag);
        MaskProgression_Update(&play);
        assert(masks.empty() && tradeFlags.empty() && notifications.empty());
        for (int item : {ITEM_MM_MASK_DEKU, ITEM_MM_MASK_GORON, ITEM_MM_MASK_ZORA, ITEM_MASK_KEATON})
            assert(MaskProgression_CanTransform(item));
    }
    // Some generated seeds require a transformation before its child dungeon.
    // The loaded seed's logic options must override a previously enabled gate.
    for (unsigned enabled = 1; enabled < 4; ++enabled) {
        reset(); isRando = true;
        cvars["gMods.TransformMasks.ChildDungeonGates"] = 1;
        if (enabled & 1) seedOptions.insert(RSK_MM_MASKS_ALL);
        if (enabled & 2) seedOptions.insert(RSK_MM_MASKS_TRANSFORM);
        for (int item : {ITEM_MM_MASK_DEKU, ITEM_MM_MASK_GORON, ITEM_MM_MASK_ZORA, ITEM_MASK_KEATON})
            assert(MaskProgression_CanTransform(item) && "gate blocks a mask required by seed logic");
        for (const auto& gate : gates) events.insert(gate.flag);
        MaskProgression_Update(&play);
        assert(masks.empty() && tradeFlags.empty() && notifications.empty() && "incompatible seed received gate rewards");
    }
    // Exhaustive cleared-dungeon combinations, independent of reward-item possession.
    for (unsigned clears = 0; clears < 8; ++clears) {
        reset();
        isRando = true; // Ordinary rando without transformation masks in its logic.
        cvars["gMods.TransformMasks.ChildDungeonGates"] = 1;
        std::set<int> expected;
        for (unsigned i = 0; i < 3; ++i) if (clears & (1u << i)) {
            events.insert(gates[i].flag);
            expected.insert(gates[i].items.begin(), gates[i].items.end());
        }
        MaskProgression_Update(&play);
        assert(masks == expected);
        for (unsigned i = 0; i < 3; ++i) {
            assert(bool(tradeFlags.count(gates[i].tradeFlag)) == bool(clears & (1u << i)));
            assert(bool(MaskProgression_CanTransform(gates[i].ootItem)) == bool(clears & (1u << i)));
            for (int item : gates[i].items)
                assert(bool(MaskProgression_CanTransform(item)) == bool(clears & (1u << i)));
        }
        for (int item : {ITEM_MASK_GERUDO, ITEM_MM_MASK_FIERCE_DEITY, ITEM_MM_MASK_GARO,
                         ITEM_MM_MASK_KAFEI, ITEM_RITO_MASK, ITEM_MASK_SKULL, ITEM_MASK_TRUTH})
            assert(MaskProgression_CanTransform(item));
        const auto count = notifications.size();
        MaskProgression_Update(&play);
        assert(masks == expected && notifications.size() == count && "repeated grant/notification");
    }
    reset();
    cvars["gMods.TransformMasks.ChildDungeonGates"] = 1;
    gSaveContext.tradeItem = ITEM_LETTER_ZELDA;
    for (const auto& gate : gates) events.insert(gate.flag);
    MaskProgression_Update(&play);
    assert(gSaveContext.tradeItem == ITEM_LETTER_ZELDA && "dungeon mask replaced a held trade item");
    assert(masks.size() == 4 && tradeFlags.size() == 3);
    // Already-owned early masks stay gated, without deleting inventory; clearing catches up.
    reset(); cvars["gMods.TransformMasks.ChildDungeonGates"] = 1;
    masks.insert(ITEM_MM_MASK_GORON);
    MaskProgression_Update(&play);
    assert(!MaskProgression_CanTransform(ITEM_MM_MASK_GORON) && masks.count(ITEM_MM_MASK_GORON));
    events.insert(EVENTCHKINF_USED_DODONGOS_CAVERN_BLUE_WARP);
    MaskProgression_Update(&play);
    assert(MaskProgression_CanTransform(ITEM_MASK_GORON));
    assert(tradeFlags.count(RAND_INF_CHILD_TRADES_HAS_MASK_GORON));
    cvars["gMods.TransformMasks.ChildDungeonGates"] = 0;
    events.clear();
    MaskProgression_Update(&play);
    assert(masks.count(ITEM_MM_MASK_GORON) && tradeFlags.count(RAND_INF_CHILD_TRADES_HAS_MASK_GORON));
    assert(MaskProgression_CanTransform(ITEM_MM_MASK_GORON) && "disabling mode did not restore normal use");
    // Invalid/file-select contexts must never mutate a slot or grant a mask.
    reset(); cvars["gMods.TransformMasks.ChildDungeonGates"] = 1;
    for (const auto& gate : gates) events.insert(gate.flag);
    MaskProgression_Update(nullptr);
    assert(masks.empty() && tradeFlags.empty());
    gSaveContext.fileNum = 0xFF;
    MaskProgression_Update(&play);
    assert(masks.empty() && tradeFlags.empty());
    std::cout << "PASS child-dungeon masks: 8 completion combinations, unsafe seed guard, catch-up, default/off, early pickups, trade preservation and idempotent grants\n";
}
