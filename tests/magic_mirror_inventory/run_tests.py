#!/usr/bin/env python3
"""Execute both hosts' real travel inventory, selectors, and u16 button accessors."""
import os
import re
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_mm_scene_randomization_tests import function


def run(host):
    ui_path = ("src/overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_item.c" if host == "mm"
               else "src/overlays/misc/ovl_kaleido_scope/z_kaleido_item.c")
    ui = (ROOT / host / ui_path).read_text()
    inv = (ROOT / host / "mods/extended_inventory.c").read_text()
    buttons = (ROOT / host / "mods/ext_buttons/ext_buttons.cpp").read_text()
    prefix = r'''
#include "global.h"
#include "mods/extended_inventory.h"
#include "mods/ext_buttons/ext_buttons.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <initializer_list>
static_assert(EXT_ITEM_MAGIC_MIRROR == 0x0224 && SLOT_MAGIC_MIRROR == 31);
static_assert(SLOT_TIME_GATE == 31);
SaveContext gSaveContext{};
static NeiSaveData state{};
static int icons[8]{};
static int page = 1;
static s16 gCurrentItemCyclingSlot = -1;
extern "C" NeiSaveData* Nei_Save() { return &state; }
extern "C" uint16_t Nei_GetOwnedItem(uint8_t slot) { return state.ownedItems[slot - 24]; }
extern "C" void Nei_SetOwnedItem(uint8_t slot, uint16_t item) { state.ownedItems[slot - 24] = item; }
int ExtInv_GetCurrentPage() { return page; }
extern "C" int32_t CVarGetInteger(const char*, int32_t fallback) { return fallback; }
extern "C" void ItemGrantAudit_Begin(const char*, int, int, int) {}
extern "C" void ItemGrantAudit_End() {}
extern "C" s32 TradeAdult_OwnedCount() { return 0; }
extern "C" u8 TradeAdult_CellItem() { return ITEM_NONE; }
'''
    if host == "mm":
        prefix += r'''
#include "overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_scope.h"
#define TRAVEL_TOOLS_KALEIDO_CELL (SLOT_MAGIC_MIRROR - 24)
typedef void (*KaleidoWheelCycleFunc)(PlayState*, s32);
extern "C" void Audio_PlaySfx(u16) {}
extern "C" void Interface_LoadItemIconImpl(PlayState*, u8 btn) { ++icons[btn]; }
extern "C" void Interface_Dpad_LoadItemIconImpl(PlayState*, u8 btn) { ++icons[btn + 4]; }
void ExtInv_RefreshButtonIconsForItem(PlayState*, uint16_t) {}
uint8_t ExtInv_GetOotSlotItem(int) { return ITEM_NONE; }
void ExtInv_SetOotSlotItem(int, uint8_t) {}
'''
        button_names = ("ExtButton_GetItem", "ExtButton_SetItem", "ExtButton_ClearItem",
                        "ExtButton_GetDpadItem", "ExtButton_SetDpadItem")
        set_buttons = r'''
        BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_C_LEFT) = first <= 0xFF ? first : ITEM_NONE;
        if (first > 0xFF) ExtButton_SetItem(0, EQUIP_SLOT_C_LEFT, first);
        C_SLOT_EQUIP(0, EQUIP_SLOT_C_LEFT) = first > 0xFF ? SLOT_NONE : SLOT_MAGIC_MIRROR + 48;
        ExtButton_SetDpadItem(0, EQUIP_SLOT_D_RIGHT, first);
        DPAD_SLOT_EQUIP(0, EQUIP_SLOT_D_RIGHT) = SLOT_MAGIC_MIRROR + 48;
        BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_C_RIGHT) = ITEM_BOW;
'''
        check_buttons = r'''
        assert(ExtButton_GetItem(0, EQUIP_SLOT_C_LEFT) == next);
        assert(ExtButton_GetDpadItem(0, EQUIP_SLOT_D_RIGHT) == next);
        assert(BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_C_RIGHT) == ITEM_BOW);
        assert(C_SLOT_EQUIP(0, EQUIP_SLOT_C_LEFT) == (next > 0xFF ? SLOT_NONE : SLOT_MAGIC_MIRROR + 48));
        assert(DPAD_SLOT_EQUIP(0, EQUIP_SLOT_D_RIGHT) == SLOT_MAGIC_MIRROR + 48);
        assert(icons[EQUIP_SLOT_C_LEFT] == 1 && icons[EQUIP_SLOT_D_RIGHT + 4] == 1);
'''
        stick_right = "play.pauseCtx.stickAdjX = 40;"
    else:
        prefix += r'''
static u8 sTravelToolsSelectorActive = 0;
static u8 sGraceHourglassSelectorActive = 0;
static u8 sSlateSelectorActive = 0;
static u8 sWandSelectorActive = 0;
u32 gBitFlags[32]{};
extern "C" uint8_t Picto_IsOwned() { return 0; }
extern "C" uint8_t PowerKeg_IsOwned() { return 0; }
Vec3f gSfxDefaultPos{};
f32 gSfxDefaultFreqAndVolScale = 1;
s8 gSfxDefaultReverb = 0;
extern "C" void Audio_PlaySoundGeneral(u16, Vec3f*, u8, f32*, f32*, s8*) {}
extern "C" void Interface_LoadItemIcon1(PlayState*, u16 btn) { ++icons[btn]; }
'''
        button_names = ("ExtButton_GetItem", "ExtButton_SetItem", "ExtButton_ClearItem")
        set_buttons = r'''
        gSaveContext.equips.buttonItems[1] = first <= 0xFF ? first : ITEM_NONE;
        if (first > 0xFF) ExtButton_SetItem(1, first);
        gSaveContext.equips.cButtonSlots[0] = first > 0xFF ? SLOT_NONE : SLOT_MAGIC_MIRROR;
        gSaveContext.equips.buttonItems[2] = ITEM_BOW;
'''
        check_buttons = r'''
        assert(ExtButton_GetItem(1) == next);
        assert(gSaveContext.equips.buttonItems[2] == ITEM_BOW);
        assert(gSaveContext.equips.cButtonSlots[0] == (next > 0xFF ? SLOT_NONE : SLOT_MAGIC_MIRROR));
        assert(icons[1] == 1 && icons[2] == 0);
'''
        stick_right = "play.pauseCtx.stickRelX = 40;"
    helpers = ["GraceHourglass_Heal", "GraceHourglass_IsOwned", "GraceHourglass_Grant"]
    helpers += ["TravelTools_Heal", "TravelTools_IsOwned", "TravelTools_Grant", "MagicMirror_IsOwned"]
    production = "\n".join(function(inv, n) for n in helpers)
    production += "\n" + "\n".join(function(buttons, n) for n in button_names)
    production += "\n" + "\n".join(function(ui, n) for n in (
        "KaleidoScope_ResetItemCycling", "KaleidoWheel_Run", "TravelTools_KaleidoCycle", "TravelTools_KaleidoHandle"))
    # The first real name-dispatch switch handles all EXT IDs before other item families.
    name = function(inv, "ExtInv_GetCustomItemNameTex")
    name = name[:name.index("    // ", name.index("    switch (itemId)"))] + "    return nullptr;\n}"
    production += "\nuint8_t Slate_RuneCount() { return 0; }\nuint8_t Slate_GetRune() { return 0; }\nvoid* Slate_RuneNameTex(uint8_t) { return nullptr; }\n" + name
    checks = r'''
static void Reset() {
    state = {};
    for (auto& item : state.ownedItems) item = ITEM_NONE;
    gSaveContext = {};
    std::memset(icons, 0, sizeof(icons));
    gCurrentItemCyclingSlot = -1;
    /* RESET_SELECTOR */
}
int main() {
    assert(std::strcmp(static_cast<const char*>(ExtInv_GetCustomItemNameTex(EXT_ITEM_MAGIC_MIRROR, 0)),
                       "__OTR__textures/item_name_custom/gMagicMirrorNameTex") == 0);
    for (u16 first : {u16(ITEM_TIME_GATE), u16(EXT_ITEM_MAGIC_MIRROR)}) {
        Reset();
        const u16 second = first == ITEM_TIME_GATE ? EXT_ITEM_MAGIC_MIRROR : ITEM_TIME_GATE;
        ExtInv_GiveItem(SLOT_MAGIC_MIRROR, first);
        ExtInv_GiveItem(SLOT_MAGIC_MIRROR, second);
        assert(ExtInv_GetSlotItem(SLOT_MAGIC_MIRROR) == first && "granting sibling erased selected travel tool");
        /* OWNERSHIP_CHECKS */
        PlayState play{};
        play.pauseCtx.cursorSlot[PAUSE_ITEM] = 7;
        play.pauseCtx.cursorItem[PAUSE_ITEM] = first;
        play.pauseCtx.namedItem = first;
        /* SET_BUTTONS */
        play.state.input[0].press.button = BTN_A;
        TravelTools_KaleidoHandle(&play);
        assert(gCurrentItemCyclingSlot == 7);
        play.state.input[0].press.button = 0;
        /* STICK_RIGHT */
        TravelTools_KaleidoHandle(&play);
        const u16 next = second;
        assert(ExtInv_GetSlotItem(SLOT_MAGIC_MIRROR) == next);
        assert(play.pauseCtx.cursorItem[PAUSE_ITEM] == next && play.pauseCtx.namedItem == PAUSE_ITEM_NONE);
        /* CHECK_BUTTONS */
    }
    /* LIFETIME_CHECKS */
    puts("PASS travel inventory: both orders, legacy ownership, u16 C/D-pad selection, name refresh, independent removal/reset, no unrelated overwrite");
}
'''
    ownership = r'''
        assert(TravelTools_IsOwned(ITEM_TIME_GATE) && TravelTools_IsOwned(EXT_ITEM_MAGIC_MIRROR));
        assert(MagicMirror_IsOwned());
        assert(!TravelTools_IsOwned(ITEM_BOW));
        TravelTools_Grant(ITEM_BOW);
        assert(ExtInv_GetSlotItem(SLOT_MAGIC_MIRROR) == first);
        ExtInv_SetSlotItem(SLOT_MAGIC_MIRROR - 1, 0xF123);
        ExtInv_SetSlotItem(SLOT_MAGIC_MIRROR + 1, 0x0234);
        TravelTools_Heal();
        assert(ExtInv_GetSlotItem(SLOT_MAGIC_MIRROR - 1) == 0xF123);
        assert(ExtInv_GetSlotItem(SLOT_MAGIC_MIRROR + 1) == 0x0234);
'''
    lifetime = r'''
    for (u16 legacy : {u16(ITEM_TIME_GATE), u16(EXT_ITEM_MAGIC_MIRROR)}) {
        Reset();
        ExtInv_SetSlotItem(SLOT_MAGIC_MIRROR, legacy);
        TravelTools_Heal();
        assert(state.timeGateOwned == (legacy == ITEM_TIME_GATE));
        assert(state.magicMirrorOwned == (legacy == EXT_ITEM_MAGIC_MIRROR));
        assert(TravelTools_IsOwned(legacy));
    }
    Reset();
    state.magicMirrorOwned = 1;
    TravelTools_Heal();
    assert(ExtInv_GetSlotItem(SLOT_MAGIC_MIRROR) == EXT_ITEM_MAGIC_MIRROR);
    TravelTools_Grant(ITEM_TIME_GATE);
    state.magicMirrorOwned = 0;
    ExtInv_SetSlotItem(SLOT_MAGIC_MIRROR, ITEM_NONE);
    TravelTools_Heal();
    assert(!MagicMirror_IsOwned() && TravelTools_IsOwned(ITEM_TIME_GATE));
    assert(ExtInv_GetSlotItem(SLOT_MAGIC_MIRROR) == ITEM_TIME_GATE);
    TravelTools_Grant(EXT_ITEM_MAGIC_MIRROR);
    state.timeGateOwned = 0;
    ExtInv_SetSlotItem(SLOT_MAGIC_MIRROR, ITEM_NONE);
    TravelTools_Heal();
    assert(!TravelTools_IsOwned(ITEM_TIME_GATE) && MagicMirror_IsOwned());
    assert(ExtInv_GetSlotItem(SLOT_MAGIC_MIRROR) == EXT_ITEM_MAGIC_MIRROR);
    ExtInv_ClearPage2Items();
    TravelTools_Heal();
    assert(!TravelTools_IsOwned(ITEM_TIME_GATE) && !MagicMirror_IsOwned());
    assert(ExtInv_GetSlotItem(SLOT_MAGIC_MIRROR) == ITEM_NONE);
    Reset();
    state.timeGateOwned = state.magicMirrorOwned = 1;
    ExtInv_SetSlotItem(SLOT_MAGIC_MIRROR, 0xF123);
    TravelTools_Heal();
    assert(ExtInv_GetSlotItem(SLOT_MAGIC_MIRROR) == 0xF123);
'''
    checks = checks.replace("/* SET_BUTTONS */", set_buttons).replace("/* CHECK_BUTTONS */", check_buttons)
    checks = checks.replace("/* STICK_RIGHT */", stick_right)
    checks = checks.replace("/* RESET_SELECTOR */", "sTravelToolsSelectorActive = 0;" if host == "soh" else "")
    checks = checks.replace("/* OWNERSHIP_CHECKS */", ownership)
    checks = checks.replace("/* LIFETIME_CHECKS */", lifetime)
    with tempfile.TemporaryDirectory(prefix="mirror-inventory-" + host + "-") as td:
        source = Path(td) / "test.cpp"
        source.write_text(prefix + production + checks)
        binary = Path(td) / "test"
        includes = ["-I" + str(ROOT / p) for p in (host, host + "/include", host + "/include/PR", host + "/src", host + "/assets", host + "/2s2h", "libultraship/include", "combo", "combo/menu")]
        flags = ["-std=c++20", "-w", "-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DCONTROLLERBUTTONS_T=uint32_t"]
        for path in ("CMake/soh-cvars.cmake", "CMake/lus-cvars.cmake"):
            for key, value in re.findall(r'set\((CVAR_PREFIX_\w+)\s+"?([^\s"\)]+)', (ROOT / path).read_text()):
                flags.append(f'-D{key}="{value}"')
        subprocess.run([os.environ.get("CXX", "c++"), *flags, *includes, str(source), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
        print("PASS host:", host, flush=True)


if __name__ == "__main__":
    for host in ("mm", "soh"):
        run(host)
