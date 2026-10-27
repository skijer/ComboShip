#!/usr/bin/env python3
"""Execute MM's production item-cursor function with native pause/save structures."""
import argparse
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_mm_scene_randomization_tests import function

PREFIX = r'''
#include "global.h"
#include "mods/extended_inventory.h"
#include "overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_scope.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/CustomMessage/PauseItemDescriptions.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
SaveContext gSaveContext;
static NeiSaveData nei;
static ExtendedInventoryState sExtInvState;
static int customEnabled = 1, masksEnabled = 1, cycleActive;
static s16 sEquipMagicArrowSlotHoldTimer, sEquipState, sEquipAnimTimer;
static u8 sPlayerFormItems[PLAYER_FORM_MAX];
NeiSaveData* Nei_Save(void) { return &nei; }
uint16_t Nei_GetOwnedItem(uint8_t slot) { return slot >= 24 && slot < 72 ? nei.ownedItems[slot - 24] : ITEM_NONE; }
void Nei_SetOwnedItem(uint8_t slot, uint16_t item) { nei.ownedItems[slot - 24] = item; }
uint8_t ExtInv_GetOotSlotItem(int slot) { return ITEM_NONE; }
void ExtInv_SetOotSlotItem(int slot, uint8_t item) {}
void ItemGrantAudit_Begin(const char* origin, int item, int slot, int immediate) {}
void ItemGrantAudit_End(void) {}
s32 TradeAdult_OwnedCount(void) { return 0; }
u8 TradeAdult_CellItem(void) { return ITEM_NONE; }
int32_t CVarGetInteger(const char* name, int32_t fallback) {
    if (!strcmp(name,"gMods.CustomItems.Enabled")) return customEnabled;
    if (!strcmp(name,"gMods.MmMasks.InventoryEnabled")) return masksEnabled;
    return fallback;
}
void Audio_PlaySfx(u16 id) {}
void Interface_SetHudVisibility(u16 visibility) {}
s32 Player_GetCurMaskItemId(PlayState* play) { return ITEM_NONE; }
bool GameInteractor_Should(GIVanillaBehavior flag, uint32_t result, ...) { return result; }
uint32_t GameInteractor_Dpad(GIDpadType type, uint32_t buttons) { return 0; }
const char* PauseItemDesc_Get(u16 item, s32 page) { return NULL; }
u8 PauseItemDesc_VanillaTextExists(u16 textId) { return 0; }
void PauseItemDesc_Show(PlayState* play, const char* desc, u8 position) {}
u8 PauseItemDesc_ShowItem(PlayState* play, u16 item, s32 page, u8 position) { return 0; }
void func_801514B0(PlayState* play, u16 textId, u8 position) {}
s32 KaleidoScope_IsItemCycling(void) { return cycleActive; }
void KaleidoScope_HandleItemCycles(PlayState* play) {}
void KaleidoScope_MoveCursorToSpecialPos(PlayState* play, s16 position) { play->pauseCtx.cursorSpecialPos = position; }
void KaleidoScope_MoveCursorFromSpecialPos(PlayState* play) { play->pauseCtx.cursorSpecialPos = 0; }
'''

CHECKS = r'''
static PlayState play;
static Vtx vertices[24*4];
static void reset(void) {
    memset(&gSaveContext,0,sizeof(gSaveContext));
    memset(&nei,0,sizeof(nei));
    memset(&play,0,sizeof(play));
    memset(gSaveContext.save.saveInfo.inventory.items,ITEM_NONE,48);
    for (int i=0;i<48;i++) nei.ownedItems[i]=ITEM_NONE;
    gSaveContext.save.playerForm=PLAYER_FORM_HUMAN;
    customEnabled=masksEnabled=1; cycleActive=0; ExtInv_Reset();
    play.pauseCtx.state=PAUSE_STATE_MAIN;
    play.pauseCtx.mainState=PAUSE_MAIN_STATE_IDLE;
    play.pauseCtx.pageIndex=PAUSE_ITEM;
    play.pauseCtx.cursorItem[PAUSE_ITEM]=PAUSE_ITEM_NONE;
    play.pauseCtx.itemVtx=vertices;
}
int main(int argc, char** argv) {
    for (int stored=0;stored<2;stored++) {
        reset(); customEnabled=masksEnabled=0;
        if (stored) nei.magicMirrorOwned=1;
        else nei.ownedItems[SLOT_MAGIC_MIRROR-24]=EXT_ITEM_MAGIC_MIRROR;
        assert(MagicMirror_IsOwned() && ExtInv_IsCustomItemsEnabled() && ExtInv_GetMaxPages()==2);
    }
    int navigation = argc < 2 || strcmp(argv[1],"entry");
    if (navigation) {
        for (int arrow=PAUSE_CURSOR_PAGE_LEFT;arrow<=PAUSE_CURSOR_PAGE_RIGHT;arrow++) {
            reset(); play.pauseCtx.cursorSpecialPos=arrow;
            nei.ownedItems[3]=ITEM_ELEMENTAL_WAND;
            play.state.input[0].press.button=BTN_L;
            KaleidoScope_UpdateItemCursor(&play);
            assert(ExtInv_GetCurrentPage()==1 && "L must reach custom items from an empty-page arrow");
            for (int i=0;i<15;i++) ExtInv_Update();
            KaleidoScope_UpdateItemCursor(&play); assert(ExtInv_GetCurrentPage()==2);
            for (int i=0;i<15;i++) ExtInv_Update();
            KaleidoScope_UpdateItemCursor(&play); assert(ExtInv_GetCurrentPage()==0);
        }
        reset(); play.state.input[0].press.button=BTN_L;
        KaleidoScope_UpdateItemCursor(&play); assert(ExtInv_GetCurrentPage()==1);
        KaleidoScope_UpdateItemCursor(&play); assert(ExtInv_GetCurrentPage()==1 && "cooldown must prevent repeat page changes");
        reset(); cycleActive=1; play.state.input[0].press.button=BTN_L;
        KaleidoScope_UpdateItemCursor(&play); assert(ExtInv_GetCurrentPage()==0);
        reset(); play.pauseCtx.itemDescriptionOn=true; play.state.input[0].press.button=BTN_L;
        KaleidoScope_UpdateItemCursor(&play); assert(ExtInv_GetCurrentPage()==0);
        reset(); play.pauseCtx.mainState=PAUSE_MAIN_STATE_EQUIP_ITEM; play.state.input[0].press.button=BTN_L;
        KaleidoScope_UpdateItemCursor(&play); assert(ExtInv_GetCurrentPage()==0);
        reset(); customEnabled=masksEnabled=0; play.state.input[0].press.button=BTN_L;
        KaleidoScope_UpdateItemCursor(&play); assert(ExtInv_GetCurrentPage()==0);
        reset(); gSaveContext.save.saveInfo.inventory.items[SLOT_DEKU_STICK]=ITEM_DEKU_STICK;
        play.state.input[0].press.button=BTN_L|BTN_CLEFT;
        KaleidoScope_UpdateItemCursor(&play);
        assert(ExtInv_GetCurrentPage()==1);
        assert(play.pauseCtx.mainState==PAUSE_MAIN_STATE_IDLE && "same-frame L/C must not equip the old-page item");
        puts("PASS MM L: empty-page arrows, all pages, cooldown, wheel/description/equip guards and same-frame identity");
    } else {
        for (int arrow=PAUSE_CURSOR_PAGE_LEFT;arrow<=PAUSE_CURSOR_PAGE_RIGHT;arrow++) {
            reset(); play.pauseCtx.cursorSpecialPos=arrow;
            play.pauseCtx.stickAdjX=arrow==PAUSE_CURSOR_PAGE_LEFT?40:-40;
            KaleidoScope_UpdateItemCursor(&play);
            assert(play.pauseCtx.cursorSpecialPos==0 && "empty inventory must still accept the cursor");
            assert(play.pauseCtx.cursorItem[PAUSE_ITEM]==PAUSE_ITEM_NONE);
            assert(play.pauseCtx.cursorPoint[PAUSE_ITEM]==(arrow==PAUSE_CURSOR_PAGE_LEFT?0:5));
            assert(play.pauseCtx.cursorXIndex[PAUSE_ITEM]==(arrow==PAUSE_CURSOR_PAGE_LEFT?0:5));
            assert(play.pauseCtx.cursorYIndex[PAUSE_ITEM]==0);
            reset(); play.pauseCtx.cursorSpecialPos=arrow;
            play.pauseCtx.stickAdjX=arrow==PAUSE_CURSOR_PAGE_LEFT?40:-40;
            gSaveContext.save.saveInfo.inventory.items[SLOT_OCARINA]=ITEM_OCARINA_OF_TIME;
            KaleidoScope_UpdateItemCursor(&play);
            assert(play.pauseCtx.cursorSpecialPos==0);
            assert(play.pauseCtx.cursorItem[PAUSE_ITEM]==ITEM_OCARINA_OF_TIME);
            assert(play.pauseCtx.cursorPoint[PAUSE_ITEM]==7 && "populated pages retain first-owned-item selection");
        }
        puts("PASS MM entry: both empty-page arrows accept grid entry; owned-item search retained");
    }
    return 0;
}
'''

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-ref")
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    def read(path):
        if args.source_ref:
            return subprocess.check_output(["git", "show", args.source_ref + ":" + path], cwd=ROOT, text=True)
        return (ROOT / path).read_text()
    inv = read("mm/mods/extended_inventory.c")
    ui = read("mm/src/overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_item.c")
    first = inv.index("static const uint8_t sOotPage0Map[24]")
    mapping = inv[first:inv.index("\n};", first) + 3]
    code = PREFIX + mapping + "\n" + "\n".join(function(inv, n) for n in (
        "TravelTools_IsOwned", "MagicMirror_IsOwned",
        "ExtInv_Reset", "ExtInv_Update", "ExtInv_CanSwitchPage", "ExtInv_SwitchPage", "ExtInv_GetCurrentPage",
        "ExtInv_GetMaxPages", "ExtInv_IsCustomItemsEnabled", "ExtInv_IsMmMasksEnabled", "ExtInv_GetInventorySlot"))
    code += "\n" + function(ui, "KaleidoScope_UpdateItemCursor") + CHECKS
    with tempfile.TemporaryDirectory(prefix="mm-pause-inventory-") as td:
        path = Path(td); source = path / "test.c"; binary = path / "test"
        source.write_text(code)
        flags = ["-std=gnu11", "-w", "-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DCONTROLLERBUTTONS_T=uint32_t",
                 "-Werror=implicit-function-declaration", "-Werror=incompatible-pointer-types"]
        if args.sanitize:
            flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-g"]
        includes = ["-I" + str(ROOT / p) for p in ("mm", "mm/include", "mm/include/PR", "mm/src", "mm/assets", "mm/2s2h",
            "libultraship/include", "combo", "combo/menu")]
        subprocess.run([os.environ.get("CC", "cc"), *flags, *includes, str(source), "-o", str(binary)], check=True)
        env = dict(os.environ, ASAN_OPTIONS="detect_leaks=0", UBSAN_OPTIONS="halt_on_error=1")
        results = [subprocess.run([str(binary), mode], env=env).returncode for mode in ("navigation", "entry")]
        return int(any(results))

if __name__ == "__main__":
    raise SystemExit(main())
