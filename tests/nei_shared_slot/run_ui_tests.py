#!/usr/bin/env python3
"""Run production pause wheels and button accessors with native game structures."""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_mm_scene_randomization_tests import function
from run_time_pedestal_tests import block_from


def main():
    for host in ("mm", "soh"):
        overlay = ("src/overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_item.c" if host == "mm"
                   else "src/overlays/misc/ovl_kaleido_scope/z_kaleido_item.c")
        ui = (ROOT / host / overlay).read_text()
        inv = (ROOT / host / "mods/extended_inventory.c").read_text()
        buttons = (ROOT / host / "mods/ext_buttons/ext_buttons.cpp").read_text()
        prefix = r'''
#include "global.h"
#include "mods/extended_inventory.h"
#include "mods/ext_buttons/ext_buttons.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <initializer_list>
SaveContext gSaveContext{};
static NeiSaveData state{};
static int icons[8]{};
static int page = 1;
static int wandRule = WAND_RANDO_MEDALLIONS;
static s16 gCurrentItemCyclingSlot = -1;
extern "C" NeiSaveData* Nei_Save() { return &state; }
extern "C" uint16_t Nei_GetOwnedItem(uint8_t slot) { return state.ownedItems[slot - 24]; }
extern "C" void Nei_SetOwnedItem(uint8_t slot, uint16_t item) { state.ownedItems[slot - 24] = item; }
int ExtInv_GetCurrentPage() { return page; }
extern "C" int32_t CVarGetInteger(const char* name, int32_t fallback) {
    return std::strcmp(name, "gRando.Options.RO_ELEMENTAL_WAND_SHUFFLE") == 0 ||
           std::strcmp(name, "gRandoSettings.ElementalWandShuffle") == 0 ? wandRule : fallback;
}
extern "C" void ItemGrantAudit_Begin(const char*, int, int, int) {}
extern "C" void ItemGrantAudit_End() {}
extern "C" s32 TradeAdult_OwnedCount() { return 0; }
extern "C" u8 TradeAdult_CellItem() { return ITEM_NONE; }
'''
        if host == "mm":
            prefix += r'''
#include "overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_scope.h"
#define GRACE_HOURGLASS_KALEIDO_CELL (SLOT_PHANTOM_HOURGLASS - 24)
#define SLATE_KALEIDO_CELL (SLOT_SHEIKAH_SLATE - 24)
#define WAND_KALEIDO_CELL (SLOT_ELEMENTAL_WAND - 24)
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
            slate_names = ("Slate_KaleidoHandle", "KaleidoScope_ResetItemCycling")
            wand_names = ("Wand_KaleidoHandle",)
        else:
            prefix += r'''
static u8 sGraceHourglassSelectorActive = 0;
static u8 sTravelToolsSelectorActive = 0;
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
            slate_names = ("Slate_KaleidoCycle", "Slate_HandleKaleidoSelector", "KaleidoScope_ResetItemCycling")
            wand_names = ("Wand_KaleidoCycle", "Wand_HandleKaleidoSelector")
        start = inv.rfind("\n", 0, inv.index("sWandQuest["))
        production = inv[start:inv.index(";", start) + 1] + "\n"
        production += "\n".join(function(inv, n) for n in (
            "GraceHourglass_Heal", "GraceHourglass_IsOwned", "GraceHourglass_Grant",
            "TravelTools_Heal", "TravelTools_IsOwned", "TravelTools_Grant",
            "Slate_RuneOwned", "Slate_GrantRune", "Slate_RuneCount", "Slate_RuneAt", "Slate_GetRune", "Slate_SetRune",
            "Slate_RuneNeighbor", "Wand_RandoMode", "Wand_ModeOwned", "Wand_GrantMode", "Wand_ModeCount",
            "Wand_ModeAt", "Wand_GetMode", "Wand_SetMode", "Wand_ModeNeighbor"))
        production += "\n" + "\n".join(function(buttons, n) for n in button_names)
        production += "\n" + function(ui, "KaleidoScope_ResetItemCycling")
        production += "\n" + function(ui, "Slate_KaleidoSyncTitle")
        if "static void Wand_KaleidoSyncTitle(" in ui:
            production += "\n" + function(ui, "Wand_KaleidoSyncTitle")
        production += "\n" + "\n".join(function(ui, n) for n in (
            "KaleidoWheel_Run", "GraceHourglass_KaleidoCycle", "GraceHourglass_KaleidoHandle",
            *(n for n in slate_names if n != "KaleidoScope_ResetItemCycling"), *wand_names))
        if host == "soh":
            # Use the native completion's final item/slot write. Animation and
            # duplicate-slot swapping are outside this wheel-selection fixture.
            equip = function(ui, "KaleidoScope_UpdateItemEquip")
            start = equip.index("            if (pauseCtx->equipTargetItem >= 0x0200)")
            icon_call = "            Interface_LoadItemIcon1(play, targetButtonIndex);"
            # Bound both assignment branches by braces, excluding the outer
            # Mirror swap guard that surrounds this write in the native path.
            assignment = block_from(equip, start)
            else_start = equip.index("else", start + len(assignment))
            assignment += " " + block_from(equip, else_start)
            production += r'''
static void CompleteNativeButtonEquip(PlayState* play, s32 button, u16 item, u8 slot) {
    PauseContext* pauseCtx = &play->pauseCtx;
    pauseCtx->equipTargetCBtn = button - 1;
    pauseCtx->equipTargetItem = item;
    pauseCtx->equipTargetSlot = slot;
    uint16_t targetButtonIndex = pauseCtx->equipTargetCBtn + 1;
''' + assignment + "\n" + icon_call + r'''
}
static void CheckOotGraceButtons(u16 item, int refreshes) {
    // OoT physical D-pad order is Up=4, Down=5, Left=6, Right=7.
    for (s32 button : {1, 4, 5}) {
        assert(ExtButton_GetItem(button) == item);
        assert(gSaveContext.equips.buttonItems[button] == (item >= 0x200 ? ITEM_EXT_BUTTON : item));
        assert(EXT_BUTTON_ITEM(button) == (item >= 0x200 ? item : 0));
        assert(gSaveContext.equips.cButtonSlots[button - 1] == (item >= 0x200 ? SLOT_NONE : 41));
        assert(icons[button] == refreshes);
    }
    assert(ExtButton_GetItem(2) == ITEM_BOW && ExtButton_GetItem(6) == EXT_ITEM_SHEIKAH_SLATE);
    for (s32 button : {3, 7}) {
        assert(ExtButton_GetItem(button) == ITEM_STICK);
        assert(gSaveContext.equips.cButtonSlots[button - 1] == SLOT_STICK);
        assert(EXT_BUTTON_ITEM(button) == 0);
    }
    assert(C_BTN_ITEM(2) == ITEM_STICK && DPAD_ITEM(3) == ITEM_STICK);
}
'''
        checks = r'''
int main() {
    PlayState play{};
    /* BIT_FLAGS */
    for (u16 first : {u16(ITEM_HYLIAS_GRACE), u16(EXT_ITEM_PHANTOM_HOURGLASS)}) {
        state = {};
        for (auto& item : state.ownedItems) item = ITEM_NONE;
        gSaveContext = {};
        std::memset(icons, 0, sizeof(icons));
        play = {};
        gCurrentItemCyclingSlot = -1;
        GraceHourglass_Grant(first);
        GraceHourglass_Grant(first == ITEM_HYLIAS_GRACE ? EXT_ITEM_PHANTOM_HOURGLASS : ITEM_HYLIAS_GRACE);
        play.pauseCtx.cursorSlot[PAUSE_ITEM] = 17;
        play.pauseCtx.cursorItem[PAUSE_ITEM] = first;
        play.pauseCtx.namedItem = first;
        /* SET_BUTTONS */
        play.state.input[0].press.button = BTN_A;
        GraceHourglass_KaleidoHandle(&play);
        assert(gCurrentItemCyclingSlot == 17);
        play.state.input[0].press.button = 0;
        /* STICK_RIGHT */
        GraceHourglass_KaleidoHandle(&play);
        const u16 next = first == ITEM_HYLIAS_GRACE ? EXT_ITEM_PHANTOM_HOURGLASS : ITEM_HYLIAS_GRACE;
        assert(ExtInv_GetSlotItem(41) == next);
        assert(play.pauseCtx.cursorItem[PAUSE_ITEM] == next);
        assert(play.pauseCtx.namedItem == PAUSE_ITEM_NONE);
        assert(state.hyliasGraceOwned && state.phantomHourglassOwned);
        /* CHECK_BUTTONS */
        /* STICK_LEFT */
        GraceHourglass_KaleidoHandle(&play);
        assert(ExtInv_GetSlotItem(41) == first);
        /* CHECK_RETURN_BUTTONS */
        /* STICK_ZERO */
        play.pauseCtx.cursorSlot[PAUSE_ITEM] = 16;
        play.pauseCtx.cursorItem[PAUSE_ITEM] = ITEM_NONE;
        GraceHourglass_KaleidoHandle(&play);
        assert(gCurrentItemCyclingSlot == -1);
        play.pauseCtx.cursorSlot[PAUSE_ITEM] = 17;
        play.pauseCtx.cursorItem[PAUSE_ITEM] = first;
        GraceHourglass_KaleidoHandle(&play);
        assert(gCurrentItemCyclingSlot == -1);
    }
    state.slateRunesOwned = 0x1F;
    state.slateMode = 0;
    ExtInv_SetSlotItem(SLOT_SHEIKAH_SLATE, EXT_ITEM_SHEIKAH_SLATE);
    play = {};
    play.pauseCtx.cursorSlot[PAUSE_ITEM] = SLOT_SHEIKAH_SLATE - 24;
    play.pauseCtx.cursorItem[PAUSE_ITEM] = EXT_ITEM_SHEIKAH_SLATE;
    play.state.input[0].press.button = BTN_A;
    /* SLATE_HANDLE */
    play.state.input[0].press.button = 0;
    /* STICK_RIGHT */
    for (int rune = 1; rune <= 5; ++rune) {
        play.pauseCtx.namedItem = EXT_ITEM_SHEIKAH_SLATE;
        /* SLATE_HANDLE */
        assert(Slate_GetRune() == rune % 5);
        assert(play.pauseCtx.namedItem == PAUSE_ITEM_NONE);
    }
    /* STICK_ZERO */
    play.pauseCtx.namedItem = EXT_ITEM_SHEIKAH_SLATE;
    Slate_GrantRune(3);
    /* SLATE_HANDLE */
    assert(Slate_GetRune() == 3 && play.pauseCtx.namedItem == PAUSE_ITEM_NONE);
    /* RESET_CHECK */
    for (int rule : {WAND_RANDO_MEDALLIONS, WAND_RANDO_SINGLE, WAND_RANDO_ELEMENTAL}) {
        wandRule = rule;
        state = {};
        gSaveContext = {};
        for (auto& item : state.ownedItems) item = ITEM_NONE;
        play = {};
        KaleidoScope_ResetItemCycling();
        Wand_GrantMode(WAND_MODE_TORNADO);
        assert(ExtInv_GetSlotItem(SLOT_ELEMENTAL_WAND) == ITEM_ELEMENTAL_WAND);
        if (rule == WAND_RANDO_MEDALLIONS) {
            assert(Wand_ModeCount() == 0 && "wand pickup must not invent medallion rewards");
            /* MEDALLIONS */
        } else if (rule == WAND_RANDO_ELEMENTAL) {
            assert(Wand_ModeCount() == 1 && Wand_GetMode() == WAND_MODE_TORNADO);
            Wand_GrantMode(WAND_MODE_SCEPTER);
        } else {
            assert(state.wandRodsOwned == 0x3F);
        }
        const u8 sparse[] = {WAND_MODE_TORNADO, WAND_MODE_SCEPTER};
        const u8 all[] = {0, 1, 2, 3, 4, 5};
        const u8* expected = rule == WAND_RANDO_SINGLE ? all : sparse;
        const int count = rule == WAND_RANDO_SINGLE ? 6 : 2;
        assert(Wand_ModeCount() == count);
        Wand_SetMode(expected[0]);
        play.pauseCtx.cursorSlot[PAUSE_ITEM] = SLOT_ELEMENTAL_WAND - 24;
        play.pauseCtx.cursorItem[PAUSE_ITEM] = ITEM_ELEMENTAL_WAND;
        play.pauseCtx.namedItem = ITEM_ELEMENTAL_WAND;
        play.state.input[0].press.button = BTN_A;
        /* WAND_HANDLE */
        play.state.input[0].press.button = 0;
        /* STICK_RIGHT */
        for (int index = 1; index <= count; ++index) {
            play.pauseCtx.namedItem = ITEM_ELEMENTAL_WAND;
            /* WAND_HANDLE */
            assert(Wand_GetMode() == expected[index % count]);
            assert(play.pauseCtx.namedItem == PAUSE_ITEM_NONE && "wand title stayed cached while cycling right");
        }
        /* STICK_LEFT */
        for (int index = 1; index <= count; ++index) {
            play.pauseCtx.namedItem = ITEM_ELEMENTAL_WAND;
            /* WAND_HANDLE */
            assert(Wand_GetMode() == expected[(count - index) % count]);
            assert(play.pauseCtx.namedItem == PAUSE_ITEM_NONE && "wand title stayed cached while cycling left");
        }
        /* STICK_ZERO */
        play.pauseCtx.namedItem = ITEM_ELEMENTAL_WAND;
        /* WAND_HANDLE */
        play.pauseCtx.namedItem = ITEM_ELEMENTAL_WAND;
        /* WAND_HANDLE */
        assert(play.pauseCtx.namedItem == ITEM_ELEMENTAL_WAND && "unchanged mode must retain title timer");
        Wand_SetMode(expected[1]);
        /* WAND_HANDLE */
        assert(play.pauseCtx.namedItem == PAUSE_ITEM_NONE && "live wand grants/selections must refresh the hovered title");
        play.pauseCtx.namedItem = ITEM_BOW;
        play.pauseCtx.cursorItem[PAUSE_ITEM] = ITEM_BOW;
        Wand_SetMode(expected[0]);
        /* WAND_HANDLE */
        assert(play.pauseCtx.namedItem == ITEM_BOW && "wand refresh must not overwrite another item's title");
    }
    puts("PASS native pause wheels: A, both directions, u16 C/Dpad items, Slate titles and all three wand grant/title rules");
}
'''
        if host == "mm":
            checks = checks.replace("/* CHECK_RETURN_BUTTONS */", "")
            checks = checks.replace("/* SET_BUTTONS */", r'''
        ExtButton_SetItem(0, EQUIP_SLOT_C_LEFT, first);
        C_SLOT_EQUIP(0, EQUIP_SLOT_C_LEFT) = first >= 0x200 ? SLOT_NONE : 89;
        BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_C_DOWN) = ITEM_BOW;
        ExtButton_SetDpadItem(0, EQUIP_SLOT_D_RIGHT, first);
        DPAD_SLOT_EQUIP(0, EQUIP_SLOT_D_RIGHT) = 89;
        ExtButton_SetDpadItem(0, EQUIP_SLOT_D_LEFT, EXT_ITEM_SHEIKAH_SLATE);
''').replace("/* CHECK_BUTTONS */", r'''
        assert(ExtButton_GetItem(0, EQUIP_SLOT_C_LEFT) == next);
        assert(C_SLOT_EQUIP(0, EQUIP_SLOT_C_LEFT) == (next >= 0x200 ? SLOT_NONE : 89));
        assert(ExtButton_GetItem(0, EQUIP_SLOT_C_DOWN) == ITEM_BOW);
        assert(ExtButton_GetDpadItem(0, EQUIP_SLOT_D_RIGHT) == next);
        assert(DPAD_SLOT_EQUIP(0, EQUIP_SLOT_D_RIGHT) == 89);
        assert(ExtButton_GetDpadItem(0, EQUIP_SLOT_D_LEFT) == EXT_ITEM_SHEIKAH_SLATE);
        assert(icons[EQUIP_SLOT_C_LEFT] == 1 && icons[EQUIP_SLOT_D_RIGHT + 4] == 1);
''')
            checks = checks.replace("/* STICK_RIGHT */", "play.pauseCtx.stickAdjX = 40;")
            checks = checks.replace("/* STICK_LEFT */", "play.pauseCtx.stickAdjX = -40;")
            checks = checks.replace("/* STICK_ZERO */", "play.pauseCtx.stickAdjX = 0;")
            checks = checks.replace("/* SLATE_HANDLE */", "Slate_KaleidoHandle(&play);")
            checks = checks.replace("/* WAND_HANDLE */", "Wand_KaleidoHandle(&play);")
            checks = checks.replace("/* BIT_FLAGS */", "")
            checks = checks.replace("/* MEDALLIONS */", "state.ootQuestItems = (1u << OOT_QUEST_MEDALLION_FOREST) | (1u << OOT_QUEST_MEDALLION_SHADOW);")
            checks = checks.replace("/* RESET_CHECK */", r'''
    KaleidoScope_ResetItemCycling();
    assert(gCurrentItemCyclingSlot == -1);
''')
        else:
            checks = checks.replace("/* SET_BUTTONS */", r'''
        sGraceHourglassSelectorActive = 0;
        // Low-byte Grace is a raw native equip, never a full-ID marker.
        ExtButton_SetItem(1, ITEM_HYLIAS_GRACE);
        gSaveContext.equips.cButtonSlots[0] = SLOT_PHANTOM_HOURGLASS;
        assert(ExtButton_GetItem(1) == ITEM_NONE);
        for (s32 button : {1, 4, 5})
            CompleteNativeButtonEquip(&play, button, first, SLOT_PHANTOM_HOURGLASS);
        CompleteNativeButtonEquip(&play, 2, ITEM_BOW, SLOT_BOW);
        CompleteNativeButtonEquip(&play, 6, EXT_ITEM_SHEIKAH_SLATE, SLOT_SHEIKAH_SLATE);
        CompleteNativeButtonEquip(&play, 3, ITEM_STICK, SLOT_STICK);
        CompleteNativeButtonEquip(&play, 7, ITEM_STICK, SLOT_STICK);
        gSaveContext.equips.cButtonSlots[6] = SLOT_NONE;
        assert(ExtButton_GetItem(7) == ITEM_NONE && DPAD_ITEM(3) == ITEM_NONE);
        CompleteNativeButtonEquip(&play, 7, ITEM_STICK, SLOT_STICK);
        std::memset(icons, 0, sizeof(icons));
        CheckOotGraceButtons(first, 0);
''').replace("/* CHECK_BUTTONS */", r'''
        assert(ExtButton_GetItem(1) == next && ExtButton_GetItem(4) == next);
        CheckOotGraceButtons(next, 1);
''')
            checks = checks.replace("/* CHECK_RETURN_BUTTONS */", "CheckOotGraceButtons(first, 2);")
            checks = checks.replace("/* STICK_RIGHT */", "play.pauseCtx.stickRelX = 40;")
            checks = checks.replace("/* STICK_LEFT */", "play.pauseCtx.stickRelX = -40;")
            checks = checks.replace("/* STICK_ZERO */", "play.pauseCtx.stickRelX = 0;")
            checks = checks.replace("/* WAND_HANDLE */", "Wand_HandleKaleidoSelector(&play);")
            checks = checks.replace("/* BIT_FLAGS */", "for (int bit = 0; bit < 32; ++bit) gBitFlags[bit] = 1u << bit;")
            checks = checks.replace("/* MEDALLIONS */", "gSaveContext.inventory.questItems = (1u << QUEST_MEDALLION_FOREST) | (1u << QUEST_MEDALLION_SHADOW);")
            checks = checks.replace("/* SLATE_HANDLE */", "Slate_HandleKaleidoSelector(&play);")
            checks = checks.replace("/* RESET_CHECK */", r'''
    play.pauseCtx.cursorItem[PAUSE_ITEM] = ExtInv_GetSlotItem(41);
    play.pauseCtx.cursorSlot[PAUSE_ITEM] = 17;
    play.state.input[0].press.button = BTN_A;
    GraceHourglass_KaleidoHandle(&play);
    assert(gCurrentItemCyclingSlot == 17);
    KaleidoScope_ResetItemCycling();
    play.state.input[0].press.button = 0;
    GraceHourglass_KaleidoHandle(&play);
    assert(gCurrentItemCyclingSlot == -1);
''')
        flags = ["-std=c++20", "-w", "-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DCONTROLLERBUTTONS_T=uint32_t"]
        includes = [host, host + "/include", host + "/include/PR", host + "/src", host + "/assets", host + "/2s2h",
                    "libultraship/include", "combo", "combo/menu"]
        flags += ["-I" + str(ROOT / p) for p in includes]
        for path in ("CMake/soh-cvars.cmake", "CMake/lus-cvars.cmake"):
            for key, value in re.findall(r'set\((CVAR_PREFIX_\w+)\s+"?([^\s"\)]+)', (ROOT / path).read_text()):
                flags.append(f'-D{key}="{value}"')
        with tempfile.TemporaryDirectory(prefix="nei-wheel-") as td:
            source = Path(td) / "test.cpp"
            source.write_text(prefix + production + checks)
            binary = Path(td) / "test"
            subprocess.run([os.environ.get("CXX", "c++"), *flags, str(source), "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True)
            print("PASS pause host:", host, flush=True)


if __name__ == "__main__":
    main()
