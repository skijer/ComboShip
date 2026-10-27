#!/usr/bin/env python3
"""Run OoT's real item-selection input block with native pause/save structures.

The regression catches a page button gated behind grid focus: an empty sub-page
leaves the cursor on an arrow and makes owned items on other pages unreachable.
Graphics submission and external service effects are the fixture boundaries.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]

def function(source, name):
    match = re.search(r"^(?:static\s+)?(?:bool|void|int|s32|u16|uint8_t)\s+" + re.escape(name) +
                      r"\([^;{}]*\)\s*\{", source, re.M)
    if match is None:
        raise ValueError(name)
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end]

PREFIX = r'''
#include "z_kaleido_scope.h"
#include "soh/Enhancements/enhancementTypes.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#undef IS_RANDO
#define IS_RANDO 0
#undef osSyncPrintf
#define osSyncPrintf(...) ((void)0)
SaveContext gSaveContext;
static NeiSaveData nei;
static ExtendedInventoryState sExtInvState;
static int customEnabled=1, masksEnabled=1, ngc, cycleActive;
static int sGustOverlayActive, sArrowWheelOverlayActive, equipCalls;
static u16 equippedItem, equippedSlot;
NeiSaveData* Nei_Save(void) { return &nei; }
uint16_t Nei_GetOwnedItem(uint8_t slot) { return slot>=24 && slot<72 ? nei.ownedItems[slot-24] : ITEM_NONE; }
unsigned char Picto_IsOwned(void) { return 0; }
unsigned char PowerKeg_IsOwned(void) { return 0; }
uint8_t ExtInv_GetSlotAgeReq(uint8_t slot) { return AGE_REQ_NONE; }
int32_t CVarGetInteger(const char* key, int32_t fallback) {
    if (!strcmp(key,"gMods.CustomItems.Enabled")) return customEnabled;
    if (!strcmp(key,"gMods.MmMasks.InventoryEnabled")) return masksEnabled;
    if (!strcmp(key,CVAR_ENHANCEMENT("NGCKaleidoSwitcher"))) return ngc;
    return fallback;
}
bool IsItemCycling(void) { return cycleActive; }
void KaleidoScope_HandleItemCycles(PlayState* play) {}
void HarpoonDrops_RequestDropFromPause(int tab, int slot) {}
bool GameInteractor_Should(GIVanillaBehavior flag, uint32_t result, ...) { return result; }
void KaleidoScope_SetCursorVtx(PauseContext* pause, u16 index, Vtx* vertices) {}
void KaleidoScope_SetupItemEquip(PlayState* play, u16 item, u16 slot, s16 x, s16 y) {
    ++equipCalls; equippedItem=item; equippedSlot=slot;
}
void Audio_PlaySfxGeneral(u16 id, Vec3f* pos, u8 token, f32* freq, f32* volume, s8* reverb) {}
Vec3f gSfxDefaultPos;
f32 gSfxDefaultFreqAndVolScale;
s8 gSfxDefaultReverb;
'''

CHECKS = r'''
static PlayState play;
static Vtx vertices[24*4];
static Vtx cursorVertices[4];
static void reset(void) {
    memset(&play,0,sizeof(play)); memset(&gSaveContext,0,sizeof(gSaveContext));
    memset(&nei,0,sizeof(nei)); memset(gSaveContext.inventory.items,ITEM_NONE,24);
    for (int i=0;i<48;i++) nei.ownedItems[i]=ITEM_NONE;
    customEnabled=masksEnabled=1; ngc=cycleActive=0;
    sGustOverlayActive=sArrowWheelOverlayActive=equipCalls=0;
    play.pauseCtx.state=6; play.pauseCtx.pageIndex=PAUSE_ITEM;
    play.pauseCtx.cursorItem[PAUSE_ITEM]=PAUSE_ITEM_NONE;
    play.pauseCtx.itemVtx=vertices; play.pauseCtx.cursorVtx=cursorVertices; ExtInv_Reset();
}
static void tick(u16 pressed, s16 stickX) {
    play.state.input[0].press.button=pressed;
    play.pauseCtx.stickRelX=stickX; play.pauseCtx.stickRelY=0;
    Step(&play);
}
static void cooldown(void) { for (int i=0;i<15;i++) tick(0,0); }
int main(int argc,char** argv) {
    for (int stored=0;stored<2;stored++) {
        reset(); customEnabled=masksEnabled=0;
        if (stored) nei.magicMirrorOwned=1;
        else nei.ownedItems[SLOT_MAGIC_MIRROR-24]=EXT_ITEM_MAGIC_MIRROR;
        assert(MagicMirror_IsOwned() && ExtInv_IsCustomItemsEnabled() && ExtInv_GetMaxPages()==2);
    }
    int sameFrame=argc>1 && !strcmp(argv[1],"same-frame");
    if (sameFrame) {
        for (int mode=0;mode<2;mode++) {
            reset(); ngc=mode;
            gSaveContext.inventory.items[0]=ITEM_STICK;
            nei.ownedItems[0]=ITEM_ROD_FIRE;
            play.pauseCtx.cursorItem[PAUSE_ITEM]=ITEM_STICK;
            tick((mode?BTN_Z:BTN_L)|BTN_CLEFT,0);
            assert(ExtInv_GetCurrentPage()==1);
            assert(equipCalls==0 && "page switching cannot equip the previous page's item into the new slot");
            assert(play.pauseCtx.cursorItem[PAUSE_ITEM]==ITEM_ROD_FIRE);
            tick(BTN_CLEFT,0);
            assert(equipCalls==1 && equippedItem==ITEM_ROD_FIRE && equippedSlot==24);
        }
        puts("PASS OoT: page-switch frame preserves item/slot identity and defers equip");
        return 0;
    }
    for (int mode=0;mode<2;mode++) for (int side=0;side<2;side++) {
        reset(); ngc=mode;
        int arrow=side?PAUSE_CURSOR_PAGE_RIGHT:PAUSE_CURSOR_PAGE_LEFT;
        play.pauseCtx.cursorSpecialPos=arrow;
        nei.ownedItems[3]=ITEM_ELEMENTAL_WAND;
        tick(mode?BTN_Z:BTN_L,0);
        assert(ExtInv_GetCurrentPage()==1 && "empty-page arrow must not block access to unlocked custom items");
        assert(play.pauseCtx.cursorSpecialPos==arrow);
        tick(0,side?-40:40);
        assert(play.pauseCtx.cursorSpecialPos==0);
        assert(play.pauseCtx.cursorItem[PAUSE_ITEM]==ITEM_ELEMENTAL_WAND);
        assert(play.pauseCtx.cursorPoint[PAUSE_ITEM]==3);
        assert(play.pauseCtx.cursorSlot[PAUSE_ITEM]==3);
        cooldown(); tick(mode?BTN_Z:BTN_L,0);
        assert(ExtInv_GetCurrentPage()==2);
        cooldown(); tick(mode?BTN_Z:BTN_L,0);
        assert(ExtInv_GetCurrentPage()==0);
        assert(equipCalls==0);
    }
    for (int mode=0;mode<2;mode++) {
        reset(); ngc=mode; play.pauseCtx.cursorSpecialPos=PAUSE_CURSOR_PAGE_LEFT;
        tick(mode?BTN_L:BTN_Z,0);
        assert(ExtInv_GetCurrentPage()==0 && "the tab-switch shoulder must not change inventory sub-pages");
        tick(mode?BTN_Z:BTN_L,0); tick(mode?BTN_Z:BTN_L,0);
        assert(ExtInv_GetCurrentPage()==1 && "cooldown prevents repeated page changes");
    }
    for (int guard=0;guard<7;guard++) {
        reset(); play.pauseCtx.cursorSpecialPos=PAUSE_CURSOR_PAGE_RIGHT;
        if (guard==0) cycleActive=1;
        if (guard==1) play.pauseCtx.unk_1E4=3;
        if (guard==2) play.pauseCtx.pageIndex=PAUSE_MAP;
        if (guard==3) customEnabled=masksEnabled=0;
        if (guard==4) sGustOverlayActive=1;
        if (guard==5) sArrowWheelOverlayActive=1;
        if (guard==6) play.pauseCtx.debugState=1;
        tick(BTN_L,0); assert(ExtInv_GetCurrentPage()==0);
    }
    reset(); customEnabled=0; play.pauseCtx.cursorSpecialPos=PAUSE_CURSOR_PAGE_LEFT;
    nei.ownedItems[29]=ITEM_MM_MASK_DEKU;
    tick(BTN_L,0); assert(ExtInv_GetCurrentPage()==2);
    tick(0,40);
    assert(play.pauseCtx.cursorSpecialPos==0 && play.pauseCtx.cursorItem[PAUSE_ITEM]==ITEM_MM_MASK_DEKU);
    assert(play.pauseCtx.cursorPoint[PAUSE_ITEM]==5);
    cooldown(); tick(BTN_L,0); assert(ExtInv_GetCurrentPage()==0);
    reset(); masksEnabled=0; play.pauseCtx.cursorSpecialPos=PAUSE_CURSOR_PAGE_RIGHT;
    tick(BTN_L,0); assert(ExtInv_GetCurrentPage()==1);
    cooldown(); tick(BTN_L,0); assert(ExtInv_GetCurrentPage()==0);
    puts("PASS OoT: both arrows, both shoulder mappings, unlocked-item reentry, all pages, cooldown and input guards");
    return 0;
}
'''

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-ref")
    parser.add_argument("--sanitize",action="store_true")
    args=parser.parse_args()
    def read(path):
        return subprocess.check_output(["git","show",args.source_ref+":"+path],cwd=ROOT,text=True) if args.source_ref else (ROOT/path).read_text()
    inventory=read("soh/mods/extended_inventory.c")
    ui=read("soh/src/overlays/misc/ovl_kaleido_scope/z_kaleido_item.c")
    scope=read("soh/src/overlays/misc/ovl_kaleido_scope/z_kaleido_scope_PAL.c")
    draw=function(ui,"KaleidoScope_DrawItemSelect")
    locals_=draw[draw.index("    Input* input"):draw.index("    OPEN_DISPS")]
    input_=draw[draw.index("    // Update extended inventory pagination timer"):draw.index("    gDPSetCombineLERP(OVERLAY_DISP")]
    code=PREFIX+"\n"+"\n".join(function(inventory,n) for n in (
        "TravelTools_IsOwned","MagicMirror_IsOwned",
        "ExtInv_Reset","ExtInv_Update","ExtInv_CanSwitchPage","ExtInv_SwitchPage","ExtInv_GetCurrentPage",
        "ExtInv_GetMaxPages","ExtInv_IsCustomItemsEnabled","ExtInv_IsMmMasksEnabled","ExtInv_GetInventorySlot"))
    code+="\n"+function(scope,"KaleidoScope_MoveCursorToSpecialPos")
    code+="\nstatic void Step(PlayState* play) {\n"+locals_+input_+"\n}\n"+CHECKS
    flags=["-std=gnu11","-w","-DF3DEX_GBI_2","-DCONTROLLERBUTTONS_T=uint32_t",
           '-DCVAR_PREFIX_ENHANCEMENT="gEnhancements"','-DCVAR_PREFIX_SETTING="gSettings"',
           "-Werror=implicit-function-declaration","-Werror=incompatible-pointer-types"]
    if args.sanitize:
        flags += ["-fsanitize=address,undefined","-fno-omit-frame-pointer","-fno-pie","-no-pie","-g"]
    includes=["-I"+str(ROOT/p) for p in ("soh","soh/include","soh/include/PR","soh/src","soh/assets","soh/soh",
        "soh/src/overlays/misc/ovl_kaleido_scope","libultraship/include","combo","combo/menu")]
    with tempfile.TemporaryDirectory(prefix="oot-pause-inventory-") as td:
        source=Path(td)/"test.c"; source.write_text(code)
        env={**os.environ,"ASAN_OPTIONS":"detect_leaks=0","UBSAN_OPTIONS":"halt_on_error=1"}
        failed=False
        for combined in (False,True):
            binary=Path(td)/("combo" if combined else "solo")
            subprocess.run([os.environ.get("CC","cc"),*flags,*( ["-DCOMBO_BUILD"] if combined else []),*includes,str(source),"-o",str(binary)],check=True)
            for mode in ("navigation","same-frame"):
                failed |= subprocess.run([str(binary),mode],env=env).returncode != 0
        return int(failed)

if __name__=="__main__":
    raise SystemExit(main())
