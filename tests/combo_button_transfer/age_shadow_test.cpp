// Production implementations are supplied verbatim by run_age_tests.py.
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "global.h"
#include "mods/nei_save.h"
#include "mods/ext_buttons/ext_buttons.h"
#ifdef COMBO_BUILD
#include "gameplay/MagicMirrorItem.h"
#endif
#include "soh/Enhancements/game-interactor/vanilla-behavior/GIVanillaBehavior.h"
#define private public
#include "soh/SaveManager.h"
#undef private
SaveManager* SaveManager::Instance = nullptr;
SaveManager::SaveManager() {}
SaveContext gSaveContext{};
PlayState* gPlayState = nullptr;
static NeiSaveData nei{};
extern "C" NeiSaveData* Nei_Save() { return &nei; }
u16 gEquipMasks[] = {0x000F, 0x00F0, 0x0F00, 0xF000};
u16 gEquipNegMasks[] = {0xFFF0, 0xFF0F, 0xF0FF, 0x0FFF};
u8 gEquipShifts[] = {0, 4, 8, 12};
u32 gBitFlags[] = {1, 2, 4, 8, 16, 32, 64, 128};
extern "C" void Flags_SetInfTable(s32) {}
extern "C" void Flags_UnsetInfTable(s32) {}
bool GameInteractor_Should(GIVanillaBehavior, uint32_t result, ...) { return result; }
u8 Randomizer_GetSettingValue(RandomizerSettingKey) { return 0; }
void ExtEquip_ValidateForAgeWithoutProgression(s32) {}
static PlayState* sNonLogicAgeSwitchPlay = nullptr;
static s32 sNonLogicSourceAge, sNonLogicEntrance;
#undef osSyncPrintf
#define osSyncPrintf(...) ((void)0)
// Observe the production HUD guard, with rendering itself outside this fixture.
#undef gDPSetPrimColor
#undef gDPSetCombineMode
#undef gDPPipeSync
#undef gDPSetCombineLERP
#define gDPSetPrimColor(...) ((void)0)
#define gDPSetCombineMode(...) ((void)0)
#define gDPPipeSync(...) ((void)0)
#define gDPSetCombineLERP(...) ((void)0)
static InterfaceContext hud{};
static InterfaceContext* interfaceCtx = &hud;
static int icons = 0, ammo = 0;
static void* ExtInv_GetItemIcon(u16 item) { assert(item != ITEM_NONE); return &hud; }
static void* PikaMode_ButtonIcon(s32, void* texture) { return texture; }
extern "C" void Interface_DrawItemIconTexture(PlayState*, void*, s16) { ++icons; }
extern "C" void Interface_DrawAmmoCount(PlayState*, s16, s16) { ++ammo; }
// Equip completion reloads icons; rendering is outside this storage fixture.
extern "C" void Interface_LoadItemIcon2(PlayState*, u16) {}
extern "C" int32_t CVarGetInteger(const char* name, int32_t fallback) {
    return !std::strcmp(name, CVAR_ENHANCEMENT("DpadEquips")) ? 1 : fallback;
}
#include "button_age_production.inc"

static_assert(offsetof(ShipSaveContextData, childExtButtons) > offsetof(ShipSaveContextData, youngHorseDataValid));
static_assert(offsetof(ShipSaveContextData, adultExtButtons) ==
              offsetof(ShipSaveContextData, childExtButtons) + sizeof(ExtButtonSaveInfo));

static void Reset() {
    gSaveContext = {}; nei = {};
    for (auto* equips : {&gSaveContext.equips, &gSaveContext.childEquips, &gSaveContext.adultEquips}) {
        std::memset(equips->buttonItems, ITEM_NONE, sizeof(equips->buttonItems));
        std::memset(equips->cButtonSlots, SLOT_NONE, sizeof(equips->cButtonSlots));
        equips->equipment = 0x1101;
    }
    std::memset(gSaveContext.inventory.items, ITEM_NONE, sizeof(gSaveContext.inventory.items));
    gSaveContext.inventory.items[SLOT_STICK] = ITEM_STICK;
    gSaveContext.inventory.items[SLOT_BOW] = ITEM_BOW;
    gSaveContext.inventory.equipment = 0x7777;
    gSaveContext.childEquips.buttonItems[0] = ITEM_SWORD_KOKIRI;
    gSaveContext.adultEquips.buttonItems[0] = ITEM_SWORD_MASTER;
    gSaveContext.equips.buttonItems[0] = ITEM_SWORD_KOKIRI;
    gSaveContext.linkAge = LINK_AGE_CHILD;
}
static void Equip(s32 button, u16 item) {
    ExtButton_ClearItem(button);
    if (item >= 0x0200) ExtButton_SetItem(button, item);
    else gSaveContext.equips.buttonItems[button] = item;
    gSaveContext.equips.cButtonSlots[button - 1] = item == ITEM_STICK ? SLOT_STICK :
        item == ITEM_BOW ? SLOT_BOW : SLOT_NONE;
}
static void Swap(bool nonlogic) {
    if (nonlogic) {
        static PlayState play{};
        gPlayState = &play;
        sNonLogicAgeSwitchPlay = &play;
        sNonLogicSourceAge = gSaveContext.linkAge;
        sNonLogicEntrance = 0x123;
        play.nextEntranceIndex = 0x123;
        play.linkAgeOnLoad = gSaveContext.linkAge ^ 1;
        gSaveContext.respawnFlag = 1;
        assert(SwitchAge_HandleNonLogicEquipmentSwap());
    } else Inventory_SwapAgeEquipment();
    assert(gSaveContext.equips.buttonItems[0] ==
           (gSaveContext.linkAge == LINK_AGE_CHILD ? ITEM_SWORD_MASTER : ITEM_SWORD_KOKIRI));
    gSaveContext.linkAge ^= 1;
}
static void Check(bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
int main(int argc, char** argv) {
    SaveManager manager; SaveManager::Instance = &manager;
    const std::string mode = argc > 1 ? argv[1] : "native";
    if (mode == "native" || mode == "nonlogic") {
        for (auto quest : {QUEST_NORMAL, QUEST_RANDOMIZER, QUEST_OOTXMM}) for (s32 button = 1; button < 8; ++button) {
            Reset(); gSaveContext.ship.quest.id = quest;
            Equip(button, 0x0223);
            Swap(mode == "nonlogic");
            Equip(button, ITEM_BOW); // native arrival/equip clears the shared u16 shadow
            Swap(mode == "nonlogic");
            Check(ExtButton_GetItem(button) == 0x0223, "child custom item lost its full ID after adult native equip");
            Swap(mode == "nonlogic");
            Check(ExtButton_GetItem(button) == ITEM_BOW, "adult native equip changed during child restore");
            Equip(button, 0x0202);
            Swap(mode == "nonlogic");
            Check(ExtButton_GetItem(button) == 0x0223, "child custom ID became the adult's different custom ID");
            Swap(mode == "nonlogic");
            Check(ExtButton_GetItem(button) == 0x0202, "adult custom ID was not independently restored");
            Equip(button, ITEM_STICK); Swap(mode == "nonlogic"); Swap(mode == "nonlogic");
            Check(ExtButton_GetItem(button) == ITEM_STICK && gSaveContext.equips.cButtonSlots[button-1] == SLOT_STICK,
                  "legitimate native stick must survive both age swaps");
        }
        if (mode == "nonlogic") for (s32 age : {LINK_AGE_CHILD, LINK_AGE_ADULT}) {
            Reset(); gSaveContext.linkAge = age;
            auto* target = age == LINK_AGE_CHILD ? &gSaveContext.adultEquips : &gSaveContext.childEquips;
            target->equipment = 0;
            Equip(5, 0x0223); Swap(true);
            Check(ExtButton_GetItem(5) == 0x0223, "uninitialized nonlogic target must retain current custom loadout");
        }
        puts("PASS native/rando/nonlogic age C/D custom shadows and legitimate native stick controls");
    } else if (mode == "storage") {
        Reset(); Equip(5, 0x0223); Swap(false); Equip(5, 0x0202); Swap(false);
        nlohmann::json blob = nlohmann::json::object(); manager.currentJsonContext = &blob;
        SaveButtons(gSaveContext); blob = nlohmann::json::parse(blob.dump());
        Reset(); manager.currentJsonContext = &blob; LoadButtons();
        Check(ExtButton_GetItem(5) == 0x0223, "active custom ID did not survive native JSON save");
        Swap(false); Check(ExtButton_GetItem(5) == 0x0202, "inactive adult custom ID did not survive native JSON save");
        Swap(false); Check(ExtButton_GetItem(5) == 0x0223, "inactive child custom ID did not survive native JSON save");
        auto legacy = blob;
        legacy["childEquips"].erase("extButtonItems"); legacy["adultEquips"].erase("extButtonItems");
        manager.currentJsonContext = &legacy; LoadButtons();
        Check(ExtButton_GetItem(5) == 0x0223, "legacy active extButtonItems must remain readable");
        Swap(false); Check(ExtButton_GetItem(5) == ITEM_NONE, "missing legacy age shadow must not resolve as stick");
        ResetButtonStorage();
        Check(ExtButton_GetItem(5) == ITEM_NONE, "new/reset save must clear current custom shadows");
        nlohmann::json cleared = nlohmann::json::object(); manager.currentJsonContext = &cleared;
        SaveButtons(gSaveContext);
        for (const char* age : {"equips", "childEquips", "adultEquips"})
            for (const auto& item : cleared[age]["extButtonItems"])
                Check(item == 0, "new/reset save must clear both cached ages as well as active shadows");
        puts("PASS native JSON active/inactive age roundtrip, legacy defaults, reset");
    } else {
        void (*draws[])(PlayState*) = {DrawButton1, DrawButton2, DrawButton3, DrawButton4, DrawButton5, DrawButton6, DrawButton7};
        for (s32 button = 1; button < 8; ++button) for (bool marker : {false, true}) {
            Reset();
            if (marker) ExtButton_SetItem(button, 0);
            else {
                gSaveContext.equips.buttonItems[button] = ITEM_STICK;
                gSaveContext.equips.cButtonSlots[button-1] = SLOT_NONE;
            }
            const auto before = gSaveContext;
            Check(ExtButton_GetItem(button) == ITEM_NONE,
                  marker ? "unresolved EXT shadow zero must not read as stick" :
                           "raw C/D zero with SLOT_NONE must not read as stick");
            icons = ammo = 0; draws[button-1](nullptr);
            Check(icons == 0 && ammo == 0, "invalid zero must skip native HUD icon and ammo draw");
            PlayState play{};
            Check(Player_GetItemOnButton(&play, button) == (marker ? ITEM_EXT_BUTTON : ITEM_NONE),
                  "invalid native zero press must be empty; EXT marker must retain native inert ID boundary");
            Check(!std::memcmp(&before, &gSaveContext, sizeof(before)), "empty getter/HUD must not mutate save");
            Equip(button, 0x0223); draws[button-1](nullptr);
            Check(icons == 1 && ammo == 1, "valid full custom ID must retain native HUD icon gate");
            Check(Player_GetItemOnButton(&play, button) == ITEM_EXT_BUTTON,
                  "native input must retain the raw custom marker rather than receive the full u16 ID");
            play.pauseCtx.equipTargetCBtn = button-1;
            play.pauseCtx.equipTargetItem = ITEM_STICK;
            play.pauseCtx.equipTargetSlot = SLOT_STICK;
            CompleteNativeButtonEquip(&play);
            Check(gSaveContext.equips.buttonItems[button] == ITEM_STICK &&
                  gSaveContext.equips.cButtonSlots[button-1] == SLOT_STICK &&
                  ExtButton_GetItem(button) == ITEM_STICK,
                  "native equip completion must retain raw zero plus SLOT_STICK as a real stick");
            draws[button-1](nullptr);
            Check(icons == 2 && ammo == 2, "real native stick must retain native HUD icon gate");
            Check(Player_GetItemOnButton(&play, button) == ITEM_STICK,
                  "valid native SLOT_STICK press must remain a real stick");
            const s32 status = button >= 4 ? button+1 : button;
            gSaveContext.buttonStatus[status] = BTN_DISABLED;
            Check(Player_GetItemOnButton(&play, button) == ITEM_NONE,
                  "native C/D disabled status must retain its original buttonStatus offset");
            gSaveContext.buttonStatus[status] = BTN_ENABLED;
            gSaveContext.equips.buttonItems[button] = ITEM_BOTTLE;
            gSaveContext.equips.cButtonSlots[button-1] = SLOT_BOTTLE_1;
            Check(Player_GetItemOnButton(&play, button) == ITEM_BOTTLE,
                  "native bottles must retain their original input behavior");
        }
        Reset(); gSaveContext.equips.buttonItems[0] = ITEM_STICK;
        const auto beforeB = gSaveContext;
        Check(ExtButton_GetItem(0) == ITEM_STICK, "B raw zero must retain its existing behavior without a C/D slot");
        PlayState play{};
        Check(Player_GetItemOnButton(&play, 0) == ITEM_STICK, "B raw zero native press must retain its existing behavior");
        Check(!std::memcmp(&beforeB, &gSaveContext, sizeof(beforeB)), "B getter must remain passive");
        puts("PASS invalid native/EXT zero is passive empty; native C/D stick equip and B zero controls preserved");
    }
}
