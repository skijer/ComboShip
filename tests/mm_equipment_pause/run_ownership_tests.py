#!/usr/bin/env python3
"""Run MM's equipment draw, cursor, actions and sync against saved ownership.

GPU/resource, controller, audio and player-refresh boundaries are fixtures. The
optional sword-element chooser is explicitly disabled through a typed boundary;
its production state, native actions and renderer have their own real-header tests.
The native equipment IDs and NeiSaveData layout, ownership accessors, equipment
cell selectors, draw loop, cursor update, action guards and sync publishers come
from production.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

import run_tests as selectors
from sword_element_fixture import enhancement_off_boundary

ROOT = Path(__file__).resolve().parents[2]


def production_function(source, name):
    # A call inside an earlier if block is not a function definition.
    match = re.search(r"^(?:static\s+)?(?:u8|u16|u32|s16|s32|void|int|uint8_t|uint16_t|uint32_t|bool)\s+" + re.escape(name) +
                      r"\([^;{}]*\)\s*\{", source, re.M)
    if not match:
        raise ValueError(name)
    cursor, depth = match.end(), 1
    while depth:
        depth += (source[cursor] == "{") - (source[cursor] == "}")
        cursor += 1
    return source[match.start():cursor]


def native_syntax(directory):
    equipment = selectors.read_source(ROOT / "mm/mods/extended_equipment.c")
    names = ("ExtEquip_RecordNativeSwordOwnership", "ExtEquip_RecordNativeShieldOwnership")
    if not all("void " + name + "(" in equipment for name in names):
        return
    sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
    from run_mm_nei_tests import flags
    source = directory / "native_ownership.c"
    source.write_text('#include "mm/mods/extended_equipment.h"\n#include "mm/mods/nei_save.h"\n'
                      '#include "mm/2s2h/FleetShipCombo/FleetComboItems.h"\n#include "mm/include/variables.h"\n' +
                      "\n".join(production_function(equipment, name) for name in names))
    result = subprocess.run([os.environ.get("CC", "cc"), "-std=gnu11", *flags(), f"-I{ROOT}",
                             "-DCOMBO_BUILD", "-DMM_BUILD_DLL", "-Werror=implicit-function-declaration",
                             "-fsyntax-only", str(source)], capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    print(f"PASS: native ownership helper syntax ({result.stderr.count('warning:')} header warnings)", flush=True)


def fixture():
    folder = ROOT / "mm/src/overlays/kaleido_scope/ovl_kaleido_scope"
    equipment = selectors.read_source(ROOT / "mm/mods/extended_equipment.c")
    pause = selectors.read_source(folder / "z_kaleido_equipment.c")
    sync = selectors.read_source(ROOT / "mm/2s2h/FleetShipCombo/FleetSync.cpp")
    code = r'''
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <nlohmann/json.hpp>
using u8 = uint8_t; using u16 = uint16_t; using u32 = uint32_t;
using s16 = int16_t; using s32 = int32_t;
#include "mm/include/z64item.h"
#include "mm/mods/nei_save.h"
#include "mm/2s2h/FleetShipCombo/FleetComboItems.h"
#define EQUIP_TYPE_BOOTS 3
#define EXT_EQUIP_OWNED_SHIFT 16
#define CVAR_EXT_EQUIP_ENABLED "gCheats.ExtendedEquipment"
#define MM_PLAYER_FORM_PIKACHU 9
#define EQUIP_SLOT_B 0
#define EQUIP_SLOT_C_LEFT 1
#define EQUIP_SLOT_C_DOWN 2
#define EQUIP_SLOT_C_RIGHT 3
#define PAUSE_MASK 3
#define PAUSE_ITEM_NONE 999
#define PAUSE_CURSOR_COLOR_SET_WHITE 0
#define PAUSE_STATE_MAIN 1
#define PAUSE_MAIN_STATE_IDLE 0
#define PAUSE_CURSOR_PAGE_LEFT -1
#define PAUSE_CURSOR_PAGE_RIGHT 1
#define BTN_A 1
#define BTN_L 2
#define BTN_CLEFT 4
#define BTN_CDOWN 8
#define BTN_CRIGHT 16
#define BTN_CUP 32
#define CHECK_BTN_ALL(value, bits) (((value) & (bits)) == (bits))
#define CHECK_BTN_ANY(value, bits) (((value) & (bits)) != 0)
#define EQUIP_GRID_COL_BASE 2
#define EQUIP_MASK_COL(col) ((col) == 0 ? 0 : (col) + EQUIP_GRID_COL_BASE)
#define EQUIP_CELL(row, col) ((row) * 6 + EQUIP_MASK_COL(col))
#define NA_SE_SY_ERROR 1
#define NA_SE_SY_DECIDE 2
#define NA_SE_SY_CURSOR 3
static int failures, checks;
static void check(bool ok, const char* label) {
    ++checks;
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", label); ++failures; }
}
struct Gfx {};
struct Vtx {};
struct Input { struct { u16 button; } press = {}; };
struct PauseContext {
    u16 pageIndex = PAUSE_MASK, cursorItem[4] = {}, cursorSlot[4] = {}, namedItem = PAUSE_ITEM_NONE;
    s16 state = PAUSE_STATE_MAIN, mainState = PAUSE_MAIN_STATE_IDLE, cursorColorSet = 0;
    s16 cursorSpecialPos = 0, stickAdjX = 0, stickAdjY = 0;
    s16 cursorPoint[4] = {}, cursorXIndex[4] = {}, cursorYIndex[4] = {};
    u8 alpha = 255, itemDescriptionOn = 0;
    Vtx maskVtx[24 * 4] = {};
};
struct PlayState { struct { void* gfxCtx = nullptr; Input input; } state; PauseContext pauseCtx; };
#define CONTROLLER1(state) (&(state)->input)
static NeiSaveData save;
NeiSaveData* Nei_Save() { return &save; }
static u8 currentExt[4], swordLevel, greatFairy;
static struct {
    struct { struct { struct { u8 buttonItems[4][4]; u16 equipment; } equips; } saveInfo; } save;
} gSaveContext;
#define MM_EQ gSaveContext.save.saveInfo.equips
// Native helpers and sync's direct equipment writes must use the same storage.
// The proxy keeps the original fixture's readable per-slot setup syntax.
static struct NativeEquipment {
    struct Slot {
        int type;
        operator u8() const { return (MM_EQ.equipment >> (4 * type)) & 0xF; }
        Slot operator=(u8 value) {
            MM_EQ.equipment = (MM_EQ.equipment & ~(0xF << (4 * type))) | ((value & 0xF) << (4 * type));
            return *this;
        }
    };
    Slot operator[](int type) { return Slot{type}; }
} nativeEquipment;
static auto& buttons = gSaveContext.save.saveInfo.equips.buttonItems[0];
static struct {
    u8& currentExtSword = currentExt[0]; u8& currentExtShield = currentExt[1];
    u8& currentExtTunic = currentExt[2]; u8& currentExtBoots = currentExt[3];
} gExtEquipState;
static int activeGame = 1, errors, equipWrites, iconLoads;
static u8 enabled, page;
#define GET_CUR_EQUIP_VALUE(type) nativeEquipment[type]
#define SET_EQUIP_VALUE(type, value) (nativeEquipment[type] = (value))
#define BUTTON_ITEM_EQUIP(form, button) buttons[button]
int FleetShipCombo_GetActiveGame() { return activeGame; }
u8 WeaponUpgrade_KokiriLevel() { return swordLevel; }
u8 WeaponUpgrade_HasGreatFairy() { return greatFairy; }
u8 ExtEquip_CapeOwned() { return save.capeOwned; }
u8 ExtEquip_PendantOwned() { return save.pendantOwned; }
u8 ExtEquip_CapeVisible() { return save.capeOwned && !save.capeHidden; }
u8 ExtEquip_PendantActive() { return save.pendantOwned && !save.pendantEffectOff; }
u8 ExtEquip_GetCurrent(s16 type) { return currentExt[type]; }
u8 ExtEquip_SlotRetired(s16, u8) { return 0; }
u8 ExtEquip_CheckAgeReq(s16, u8) { return 1; }
u8 ExtEquip_IsEnabled() { return enabled; }
u8 ExtEquip_GetPage() { return page; }
void ExtEquip_SwitchPage() { page = !page; }
void ExtEquip_Update() {}
u8 ExtEquip_CanSwitch() { return 1; }
void CVarSetInteger(const char*, int value) { enabled = value; }
u8 TransformMasks_IsTransformedAny() { return 0; }
int MmForm_GetCurrentForm() { return 0; }
void ExtEquip_SetSlot(s16, u8);
void ExtEquip_Unequip(s16 type) { ExtEquip_SetSlot(type, 0); }
void ExtEquip_RefreshPlayer() { ++equipWrites; }
void ExtEquip_ReloadBIcon() { ++iconLoads; }
void ExtEquip_CleanupSlot(s16, u8) {}
void Inventory_ChangeEquipment(u16 value) { nativeEquipment[EQUIP_TYPE_SHIELD] = value; }
void ExtEquip_ToggleCapeVisibility() { save.capeHidden = !save.capeHidden; }
void ExtEquip_TogglePendantEffect() { save.pendantEffectOff = !save.pendantEffectOff; }
void Interface_LoadItemIconImpl(PlayState*, u8) { ++iconLoads; }
void Audio_PlaySfx(int sfx) { if (sfx == NA_SE_SY_ERROR) ++errors; }
static int gSfxDefaultPos;
static float gSfxDefaultFreqAndVolScale;
static int8_t gSfxDefaultReverb;
#define NA_SE_IT_SHIELD_REMOVE 4
void Audio_PlaySoundGeneral(int, int*, int, float*, float*, int8_t*) {}
int BrokenItems_Enabled() { return 0; }
int BrokenItems_CurrentForm() { return 0; }
int BrokenItems_FormCount() { return 0; }
void* BrokenItems_FormIconTex(int) { return nullptr; }
u8 BrokenItems_FormUnlocked(int) { return 0; }
u16 BrokenItems_FormItem(int) { return ITEM_NONE; }
void BrokenItems_EquipForm(PlayState*, int) {}
const char* PauseItemDesc_GetEquipUpgrade(s16) { return nullptr; }
const char* PauseItemDesc_Get(u16, int) { return nullptr; }
void PauseItemDesc_Show(PlayState*, const char*, int) {}
u8 PauseItemDesc_ShowItem(PlayState*, u16, s32, u8) { return 0; }
u8 PauseItemDesc_ShowEquipment(PlayState*, s16, s16, s16, u8) { return 0; }
static int capeVisibilityCalls;
static bool capeVisibilityAvailable = true;
u8 PauseItemDesc_ShowCapeVisibility(PlayState*, u8) { ++capeVisibilityCalls; return capeVisibilityAvailable; }
u8 PauseItemDesc_ShowForm(PlayState*, s32, u8) { return 0; }
void KaleidoScope_SetCursorVtxPos(PauseContext*, u16, Vtx*) {}
void KaleidoScope_MoveCursorToSpecialPos(PlayState* play, s16 pos) { play->pauseCtx.cursorSpecialPos = pos; }
void KaleidoScope_MoveCursorFromSpecialPos(PlayState* play) { play->pauseCtx.cursorSpecialPos = 0; }
static s16 sEquipSubPage, sEquipCursorX, sEquipCursorY, sTransformCursor;
static u8 gExtEquipGridNameContext, gExtEquipSuppressIconOverride;
typedef struct { s16 item, equipType, index; const char* ootIcon; } EquipCell;
// Resource availability is held constant: the actual draw loop decides which saved items draw.
void* KaleidoEquip_OotTex(const char* path) { return (void*)path; }
void* KaleidoEquip_OotTex(void* path) { return path; }
void* ExtEquip_GetIcon(s16, u8) { return (void*)"ext icon"; }
void* ExtInv_GetItemIcon(u16) { return (void*)"native icon"; }
void KaleidoEquip_BuildUpgradeColVtx(PauseContext*) {}
static Vtx sUpgradeColVtx[16];
void KaleidoEquip_RenderDollFB(PlayState*) {}
void KaleidoEquip_DrawDollImage(PlayState*) {}
static Gfx* POLY_OPA_DISP;
static int gEquippedItemOutlineTex, drawSlot = -1, drawCounts[24];
static int upgradeRow = -1, upgradeDrawCounts[4];
static u8 primAlpha, upgradeAlphas[4];
static const char* upgradeIcons[4];
static Vtx* gridBegin;
static bool grayscale;
static void RecordVertex(Vtx* vertex) {
    upgradeRow = -1;
    for (int row = 0; row < 4; ++row) {
        if (vertex == &sUpgradeColVtx[row * 4]) {
            upgradeRow = row;
            drawSlot = -1;
            return;
        }
    }
    drawSlot = (vertex >= gridBegin && vertex < gridBegin + 24 * 4) ? (vertex - gridBegin) / 4 : -1;
}
void KaleidoScope_DrawTexQuadRGBA32(void*, void* icon, u16, u16, u16) {
    if (drawSlot >= 0) ++drawCounts[drawSlot];
    if (upgradeRow >= 0) {
        ++upgradeDrawCounts[upgradeRow];
        upgradeIcons[upgradeRow] = static_cast<const char*>(icon);
        upgradeAlphas[upgradeRow] = primAlpha;
    }
}
Gfx* Gfx_DrawTexQuadIA8(Gfx* gfx, int, u16, u16, u16) { return gfx; }
void RecordPrimColor(u8, u8, u8, u8 alpha) { primAlpha = alpha; }
#define OPEN_DISPS(...) ((void)0)
#define CLOSE_DISPS(...) ((void)0)
#define Gfx_SetupDL42_Opa(...) ((void)0)
#define gDPSetCombineMode(...) ((void)0)
#define gDPSetPrimColor(gfx, min, lod, r, g, b, a) RecordPrimColor(r, g, b, a)
#define gSPVertex(gfx, vertex, count, offset) RecordVertex(vertex)
#define gDPSetGrayscaleColor(...) ((void)0)
#define gSPGrayscale(gfx, value) (grayscale = (value))
'''
    code += selectors.defines((ROOT / "mm/mods/extended_equipment.h").read_text(), "ITEM_EXT_")
    for prefix in ("EQUIP_SUBPAGE_", "EQUIP_UPGRADE_ROW_", "EQUIP_EXT_ROW_", "CELL_", "OOT_ICON_"):
        code += selectors.defines(pause, prefix)
    code += enhancement_off_boundary(pause)
    functions = ["ExtEquip_GetBit", "ExtEquip_HasItem", "ExtEquip_GiveItem", "ExtEquip_GetItemId",
                 "ExtEquip_TridentAllowsShield", "ExtEquip_SetCurrentByType", "ExtEquip_ApplyVanillaBase",
                 "ExtEquip_ApplyTridentShieldPolicy", "ExtEquip_SetSlot", "ExtEquip_Equip", "ExtEquip_ToggleFromCButton"]
    if "void ExtEquip_RecordNativeShieldOwnership(" in equipment:
        functions.insert(0, "ExtEquip_RecordNativeShieldOwnership")
    if "void ExtEquip_RecordNativeSwordOwnership(" in equipment:
        functions.insert(0, "ExtEquip_RecordNativeSwordOwnership")
    for name in functions:
        code += production_function(equipment, name) + "\n"
    for name in ("ExtEquip_GetCapeIcon", "ExtEquip_GetPendantIcon"):
        code += selectors.function(equipment, name) + "\n"
    code += selectors.function(pause, "KaleidoEquip_UpgradeIcon") + "\n"
    code += production_function(sync, "KokiriChainLevel") + "\n"
    # Execute the actual exported kokiri expression too: defining a correct helper
    # without wiring it into ExtractShared must still fail the publishing test.
    kokiri_expression = re.search(r'\{\s*"kokiri"\s*,\s*(.*?)\s*\}', sync, re.S)
    sword_nibble = re.search(r'int swordNibble\s*=\s*MM_EQ\.equipment\s*&\s*0xF\s*;', sync)
    if not kokiri_expression or not sword_nibble:
        raise ValueError("MM ExtractShared kokiri expression")
    export_body = ("NeiSaveData* nei = Nei_Save();\n" + sword_nibble.group(0) +
                   "\nreturn " + kokiri_expression.group(1) + ";\n")
    if re.search(r"^(?:static\s+)?bool\s+NativeKokiriOwned\(", sync, re.M):
        code += production_function(sync, "NativeKokiriOwned") + "\n"
    else:
        # Baseline has no predicate. This compatibility wrapper executes its exact
        # ExtractShared expression, rather than substituting an invented rule.
        code += "bool NativeKokiriOwned() {\n" + export_body + "}\n"
    code += "bool ExportedKokiriFlag() {\n" + export_body + "}\n"
    for name in ("ComputeShieldOwned", "GetEquippedShieldCanonical", "SetEquippedShieldCanonical"):
        code += production_function(sync, name) + "\n"
    incoming = selectors.between(sync, '    if (sh.contains("shieldOwned")) {',
                                 '    // Upgrade-column equipment')
    code += ("void ApplyIncomingShields(const nlohmann::json& sh) {\n"
             "NeiSaveData* nei = Nei_Save();\n" + incoming + "}\n")
    if "static u8 KaleidoEquip_UsesLegacyFreeEquipment(" in pause:
        code += production_function(pause, "KaleidoEquip_UsesLegacyFreeEquipment") + "\n"
    for name in ("KaleidoEquip_UpgradeValue", "KaleidoEquip_UpgradeNameItem", "KaleidoEquip_GetCell",
                 "KaleidoEquip_CellOwned", "KaleidoEquip_CellEquipped", "KaleidoEquip_CursorCanSit",
                 "KaleidoEquip_EquipCell", "KaleidoEquip_AssignCButton", "KaleidoEquip_CycleSubPage",
                 "KaleidoScope_UpdateEquipmentCursor", "KaleidoScope_DrawEquipment"):
        source = production_function(pause, name)
        # The production TU is C: make its implicit enum-to-u16 ternary conversion
        # explicit for this C++ fixture, preserving the exact value and branches.
        if name == "KaleidoEquip_EquipCell":
            source = source.replace("? EQUIP_VALUE_SHIELD_HERO", "? (u16)EQUIP_VALUE_SHIELD_HERO")
            source = source.replace(": EQUIP_VALUE_SHIELD_MIRROR;", ": (u16)EQUIP_VALUE_SHIELD_MIRROR;")
        code += source + "\n"
    publish = selectors.between(pause, "    // --- Publish cursor state", "    // Keep the shared cursor machinery")
    code += "void Publish(PauseContext* pauseCtx) { EquipCell cell;\n" + publish + "}\n"
    code += r'''
static void Reset() {
    std::memset(&save, 0, sizeof(save));
    std::memset(currentExt, 0, sizeof(currentExt));
    std::memset(buttons, ITEM_NONE, sizeof(buttons));
    MM_EQ.equipment = 0;
    swordLevel = greatFairy = enabled = page = 0;
    activeGame = 1; errors = equipWrites = iconLoads = 0;
    sEquipCursorX = 1; sEquipCursorY = 0;
}
static void Draw(PlayState* play) {
    std::memset(drawCounts, 0, sizeof(drawCounts));
    std::memset(upgradeDrawCounts, 0, sizeof(upgradeDrawCounts));
    std::memset(upgradeIcons, 0, sizeof(upgradeIcons));
    drawSlot = -1; gridBegin = play->pauseCtx.maskVtx; grayscale = false;
    KaleidoScope_DrawEquipment(play);
}
static int GridDraws() {
    int count = 0;
    for (s16 row = 0; row < 4; ++row) for (s16 col = 1; col <= 3; ++col) count += drawCounts[EQUIP_CELL(row,col)];
    return count;
}
static void PackNativeEquipment() {
    MM_EQ.equipment = nativeEquipment[EQUIP_TYPE_SWORD] | (nativeEquipment[EQUIP_TYPE_SHIELD] << 4);
}
static void CheckKokiriExport(bool owned, const char* label) {
    PackNativeEquipment();
    NeiSaveData before = save;
    check(NativeKokiriOwned() == owned, label);
    check(ExportedKokiriFlag() == owned, "actual ExtractShared expression exports only proven Kokiri ownership");
    check(std::memcmp(&before, &save, sizeof(save)) == 0, "Kokiri ownership publisher preserves all receipt counters");
}
int main() {
    Reset(); PlayState play;
    // Native randomized startup: only the default clothing is owned, and all NEI bits are clear.
    for (s16 subpage = 0; subpage <= 1; ++subpage) {
        sEquipSubPage = subpage;
        NeiSaveData before = save;
        Draw(&play);
        check(GridDraws() == (subpage == 0 ? 2 : 0), "fresh save draws only owned equipment on each page");
        for (s16 row = 0; row < 4; ++row) {
            check(upgradeDrawCounts[row] == 0, "fresh save has no passive or placeholder icon");
            check(!KaleidoEquip_CursorCanSit(row,0), "unearned passives and retired sockets cannot hold the cursor");
        }
        for (s16 row = 0; row < 4; ++row) for (s16 col = 1; col <= 3; ++col) {
            EquipCell cell; KaleidoEquip_GetCell(subpage, row, col, &cell);
            const bool owned = KaleidoEquip_CellOwned(&cell);
            if (owned) continue;
            check(drawCounts[EQUIP_CELL(row,col)] == 0, "unearned cell has no icon");
            check(!KaleidoEquip_CursorCanSit(row,col), "cursor cannot select unearned equipment");
            sEquipCursorX = col; sEquipCursorY = row;
            Publish(&play.pauseCtx);
            check(play.pauseCtx.cursorItem[PAUSE_MASK] == PAUSE_ITEM_NONE, "unearned cell cannot publish an owned-item title");
            int writes = equipWrites, loads = iconLoads;
            KaleidoEquip_EquipCell(&play,row,col);
            check(equipWrites == writes && iconLoads == loads, "A cannot equip unearned equipment");
            if (subpage == 1) {
                KaleidoEquip_AssignCButton(&play,row,col,BTN_CLEFT);
                check(buttons[EQUIP_SLOT_C_LEFT] == ITEM_NONE, "C cannot assign unearned equipment");
            }
        }
        check(std::memcmp(&before, &save, sizeof(save)) == 0, "browsing and blocked actions preserve all saved ownership and checks");
    }
    // Per-file switch/cursor persistence must not leave a title or cursor over a hidden item.
    sEquipSubPage = 1; sEquipCursorX = 3; sEquipCursorY = 3;
    play.pauseCtx.cursorItem[PAUSE_MASK] = ITEM_EXT_BOOTS_3;
    gExtEquipGridNameContext = 1;
    KaleidoScope_UpdateEquipmentCursor(&play);
    check(play.pauseCtx.cursorSpecialPos == PAUSE_CURSOR_PAGE_LEFT, "fresh empty equipment page moves the stale cursor to a page arrow");
    check(play.pauseCtx.cursorItem[PAUSE_MASK] == PAUSE_ITEM_NONE, "fresh empty equipment page has no acquired-item title");
    check(!gExtEquipGridNameContext, "empty equipment page clears the previous grid title context");
    // The two actual passives retain distinct icons, titles and toggle behavior on both grid pages.
    for (s16 subpage = 0; subpage <= 1; ++subpage) {
        Reset(); play = {}; sEquipSubPage = subpage;
        save.capeOwned = save.pendantOwned = 1;
        Draw(&play);
        check(upgradeDrawCounts[0] == 1 && upgradeDrawCounts[1] == 1,
              "owned Cape and Pendant each draw in their real socket");
        check(std::strcmp(upgradeIcons[0], "__OTR__textures/icon_item_custom/gItemIconMagicCapePassiveTex") == 0,
              "Cape socket uses its passive cloth icon rather than a reused tunic");
        check(std::strcmp(upgradeIcons[1], "__OTR__icon_item_static_yar/gItemIconPendantOfMemoriesTex") == 0,
              "Pendant socket uses its native MM icon rather than Climb Boots");
        for (s16 row : { (s16)2, (s16)3 }) {
            check(upgradeDrawCounts[row] == 0, "retired sockets stay empty when real passives are owned");
            check(!KaleidoEquip_CursorCanSit(row,0), "retired sockets cannot be selected beside real passives");
        }
        for (s16 row = 0; row <= 1; ++row) {
            sEquipCursorX = 0; sEquipCursorY = row;
            check(KaleidoEquip_CursorCanSit(row,0), "owned passive remains selectable");
            Publish(&play.pauseCtx);
            check(play.pauseCtx.cursorItem[PAUSE_MASK] == (row == 0 ? ITEM_EXT_TUNIC_1 : ITEM_EXT_BOOTS_2) &&
                  !gExtEquipGridNameContext, "passive publishes its non-grid title context");
            play.state.input.press.button = BTN_A;
            const int popupCalls = capeVisibilityCalls;
            KaleidoScope_UpdateEquipmentCursor(&play);
            if (row == 0) {
                check(capeVisibilityCalls == popupCalls + 1 && play.pauseCtx.itemDescriptionOn,
                      "A on Cape opens the Show/Hide popup");
                check(!save.capeHidden && save.capeOwned, "opening Cape choices preserves visibility and ownership");
                const NeiSaveData popupSave = save;
                KaleidoScope_UpdateEquipmentCursor(&play);
                play.state.input.press.button = BTN_L;
                KaleidoScope_UpdateEquipmentCursor(&play);
                check(capeVisibilityCalls == popupCalls + 1 && sEquipSubPage == subpage &&
                      std::memcmp(&popupSave,&save,sizeof(save)) == 0,
                      "Cape popup owns A and L input until the native description closes");
                play.pauseCtx.itemDescriptionOn = 0; // The native textbox close resumes the equipment cursor.
            }
        }
        check(save.capeOwned && save.pendantOwned && !save.capeHidden && save.pendantEffectOff,
              "Cape popup leaves its passive owned while Pendant A retains its effect toggle");
        save.capeHidden = 1; // Confirming Hide in the native message handler changes only this flag.
        Draw(&play);
        check(upgradeAlphas[0] == play.pauseCtx.alpha, "hidden Cape retains its active passive icon alpha");
        check(upgradeAlphas[1] == 127, "inactive Pendant retains its half-transparent icon");
        play.state.input.press.button = 0; play.pauseCtx.stickAdjY = -40;
        KaleidoScope_UpdateEquipmentCursor(&play);
        check(sEquipCursorY == 1, "moving down from Pendant skips the retired lower sockets");
    }
    Reset(); play = {}; sEquipSubPage = 0; sEquipCursorX = sEquipCursorY = 0;
    save.capeOwned = 1; capeVisibilityAvailable = false;
    const NeiSaveData popupFailureSave = save;
    play.state.input.press.button = BTN_A;
    KaleidoScope_UpdateEquipmentCursor(&play);
    check(errors == 1 && !play.pauseCtx.itemDescriptionOn &&
          std::memcmp(&popupFailureSave,&save,sizeof(save)) == 0,
          "unavailable Cape popup refuses without changing visibility or ownership");
    capeVisibilityAvailable = true;
    // Native starter equipment has no separate MM ownership mask. Its true current
    // slot remains usable, while an imported/NEI piece borrowing that slot owns only itself.
    Reset(); sEquipSubPage = 0;
    nativeEquipment[EQUIP_TYPE_SWORD] = EQUIP_VALUE_SWORD_KOKIRI;
    buttons[EQUIP_SLOT_B] = ITEM_SWORD_KOKIRI;
    nativeEquipment[EQUIP_TYPE_SHIELD] = EQUIP_VALUE_SHIELD_HERO;
    EquipCell kokiri, hero;
    KaleidoEquip_GetCell(0,0,1,&kokiri); KaleidoEquip_GetCell(0,1,2,&hero);
    check(KaleidoEquip_CellOwned(&kokiri), "true native starter Kokiri Sword remains owned");
    check(KaleidoEquip_CellOwned(&hero), "true native starter Hero Shield remains owned");
    Draw(&play);
    check(drawCounts[EQUIP_CELL(0,1)] == 1 && drawCounts[EQUIP_CELL(1,2)] == 1,
          "both native starter equipment icons remain visible");
    NeiSaveData starter = save;
    check(KaleidoEquip_CursorCanSit(1,2), "native starter Hero Shield remains selectable");
    check(std::memcmp(&starter, &save, sizeof(save)) == 0, "reading native starter ownership does not change the save");
    nativeEquipment[EQUIP_TYPE_SWORD] = EQUIP_VALUE_SWORD_GILDED;
    buttons[EQUIP_SLOT_B] = ITEM_SWORD_MASTER; save.comboObtained[FC_OOT_SWORD_MASTER] = 1;
    check(!KaleidoEquip_CellOwned(&kokiri), "Master Sword's borrowed Gilded nibble cannot grant Kokiri ownership");
    save.comboObtainedFc[FCI_KOKIRI_SWORD] = 1;
    check(KaleidoEquip_CellOwned(&kokiri), "an earned Kokiri receipt stays owned while Master Sword is held");
    save.comboObtainedFc[FCI_KOKIRI_SWORD] = 0; swordLevel = 1;
    check(KaleidoEquip_CellOwned(&kokiri), "earned native progressive sword upgrade stays owned while Master is held");
    Reset(); sEquipSubPage = 0;
    nativeEquipment[EQUIP_TYPE_SWORD] = EQUIP_VALUE_SWORD_KOKIRI; buttons[EQUIP_SLOT_B] = ITEM_SWORD_KOKIRI;
    save.comboObtained[FC_OOT_SWORD_MASTER] = 1;
    KaleidoEquip_EquipCell(&play,0,2);
    check(KaleidoEquip_CellOwned(&kokiri), "native starter Kokiri remains available after selecting an earned Master Sword");
    check(save.comboObtainedFc[FCI_KOKIRI_SWORD] == 1 && save.comboAppliedFc[FCI_KOKIRI_SWORD] == 1,
          "preserved native Kokiri receipt is already applied and cannot replay a grant");
    KaleidoEquip_EquipCell(&play,0,1); KaleidoEquip_EquipCell(&play,0,2);
    check(save.comboObtainedFc[FCI_KOKIRI_SWORD] == 1 && save.comboAppliedFc[FCI_KOKIRI_SWORD] == 1,
          "repeated native replacement does not count another acquisition");
    Reset(); sEquipSubPage = 1;
    nativeEquipment[EQUIP_TYPE_SWORD] = EQUIP_VALUE_SWORD_KOKIRI; buttons[EQUIP_SLOT_B] = ITEM_SWORD_KOKIRI;
    ExtEquip_GiveItem(EQUIP_TYPE_SWORD,2);
    KaleidoEquip_EquipCell(&play,0,2); ExtEquip_Unequip(EQUIP_TYPE_SWORD);
    check(KaleidoEquip_CellOwned(&kokiri), "native starter Kokiri survives an earned Four Sword equip/unequip");
    Reset(); sEquipSubPage = 1;
    nativeEquipment[EQUIP_TYPE_SWORD] = EQUIP_VALUE_SWORD_KOKIRI; buttons[EQUIP_SLOT_B] = ITEM_SWORD_KOKIRI;
    ExtEquip_GiveItem(EQUIP_TYPE_SWORD,3);
    KaleidoEquip_EquipCell(&play,0,3); ExtEquip_Unequip(EQUIP_TYPE_SWORD);
    check(KaleidoEquip_CellOwned(&kokiri), "native starter Kokiri survives an earned Trident equip/unequip");
    for (u8 index = 1; index <= 3; ++index) {
        Reset(); sEquipSubPage = 1;
        ExtEquip_GiveItem(EQUIP_TYPE_SWORD,index);
        KaleidoEquip_EquipCell(&play,0,index);
        check(buttons[EQUIP_SLOT_B] == ExtEquip_GetItemId(EQUIP_TYPE_SWORD,index),
              "every owned extended sword, including Byrna, puts its own identity on B");
        check(nativeEquipment[EQUIP_TYPE_SWORD] == EQUIP_VALUE_SWORD_NONE,
              "an extended sword carrier cannot manufacture a native sword equip");
        check(save.comboObtainedFc[FCI_KOKIRI_SWORD] == 0 && save.comboAppliedFc[FCI_KOKIRI_SWORD] == 0,
              "an extended sword carrier preserves native ownership receipt counts");
        KaleidoEquip_EquipCell(&play,0,index);
        check(buttons[EQUIP_SLOT_B] == ITEM_NONE && currentExt[EQUIP_TYPE_SWORD] == 0,
              "removing any extended sword, including Byrna, releases B");
    }
    for (u8 index = 1; index <= 3; ++index) {
        Reset(); ExtEquip_GiveItem(EQUIP_TYPE_TUNIC,index);
        const u32 owned = save.extEquipOwnedBits;
        ExtEquip_ToggleFromCButton(ExtEquip_GetItemId(EQUIP_TYPE_TUNIC,index));
        check(enabled && currentExt[EQUIP_TYPE_TUNIC] == index && save.extEquipTunic == index,
              "an owned C-assigned tunic opts into equipment behavior with the legacy toggle off");
        check(save.extEquipOwnedBits == owned && buttons[EQUIP_SLOT_B] == ITEM_NONE,
              "a tunic C toggle preserves earned ownership and does not take B");
        ExtEquip_ToggleFromCButton(ExtEquip_GetItemId(EQUIP_TYPE_TUNIC,index));
        check(currentExt[EQUIP_TYPE_TUNIC] == 0 && save.extEquipTunic == 0,
              "a second tunic C press removes the same equipment in RAM and save");
    }
    for (s16 type = 0; type <= 3; ++type) for (u8 index = 1; index <= 3; ++index) {
        Reset(); const NeiSaveData before = save;
        ExtEquip_ToggleFromCButton(ExtEquip_GetItemId(type,index));
        check(!enabled && currentExt[type] == 0 && std::memcmp(&before,&save,sizeof(save)) == 0,
              "an unowned C-assigned equipment item cannot opt in or mutate acquisition/loadout state");
    }
    for (u8 shield = 0; shield <= 3; ++shield) {
        Reset(); sEquipSubPage = 1;
        ExtEquip_GiveItem(EQUIP_TYPE_SWORD,3);
        for (u8 index = 1; index <= 3; ++index) ExtEquip_GiveItem(EQUIP_TYPE_SHIELD,index);
        save.shieldOwned = FC_SHIELD_IKANA;
        if (shield != 0) ExtEquip_SetSlot(EQUIP_TYPE_SHIELD,shield);
        const NeiSaveData before = save;
        KaleidoEquip_EquipCell(&play,0,3);
        const u8 expectedShield = shield == 1 || shield == 3 ? shield : 0;
        check(currentExt[EQUIP_TYPE_SHIELD] == expectedShield && save.extEquipShield == expectedShield,
              "Trident retains a selected compatible shield and never auto-equips an owned shield");
        check(nativeEquipment[EQUIP_TYPE_SHIELD] == (expectedShield == 3 ? EQUIP_VALUE_SHIELD_MIRROR :
              expectedShield == 1 ? EQUIP_VALUE_SHIELD_HERO : EQUIP_VALUE_SHIELD_NONE),
              "Trident's native shield projection follows the explicit compatible selection");
        check(save.extEquipOwnedBits == before.extEquipOwnedBits && save.shieldOwned == before.shieldOwned,
              "Trident's shield policy preserves genuine extended and native ownership");
        const NeiSaveData equipped = save;
        Draw(&play); Publish(&play.pauseCtx); Draw(&play);
        check(std::memcmp(&equipped,&save,sizeof(save)) == 0 && currentExt[EQUIP_TYPE_SHIELD] == expectedShield,
              "equipment-page reentry does not re-select another shield or mutate the loadout");
    }
    for (u8 shield : {(u8)EQUIP_VALUE_SHIELD_HERO,(u8)EQUIP_VALUE_SHIELD_MIRROR}) {
        Reset(); sEquipSubPage = 1; ExtEquip_GiveItem(EQUIP_TYPE_SWORD,3); ExtEquip_GiveItem(EQUIP_TYPE_SHIELD,1);
        nativeEquipment[EQUIP_TYPE_SHIELD] = shield;
        KaleidoEquip_EquipCell(&play,0,3);
        check(currentExt[EQUIP_TYPE_SHIELD] == 0 && nativeEquipment[EQUIP_TYPE_SHIELD] ==
              (shield == EQUIP_VALUE_SHIELD_MIRROR ? EQUIP_VALUE_SHIELD_MIRROR : EQUIP_VALUE_SHIELD_NONE),
              "Trident retains a selected native Mirror but does not substitute an owned Divine for Hero");
        check(save.shieldOwned == (shield == EQUIP_VALUE_SHIELD_HERO ? FC_SHIELD_HYLIAN : 0),
              "clearing an incompatible genuine Hero preserves its earned ownership");
    }
    for (u8 borrowed : { (u8)ITEM_SWORD_MASTER, (u8)ITEM_SWORD_BGS, (u8)ITEM_EXT_SWORD_2, (u8)ITEM_EXT_SWORD_3 }) {
        Reset(); nativeEquipment[EQUIP_TYPE_SWORD] = EQUIP_VALUE_SWORD_GILDED; buttons[EQUIP_SLOT_B] = borrowed;
        check(!KaleidoEquip_CellOwned(&kokiri), "borrowed sword slot cannot prove native Kokiri ownership");
        ExtEquip_SetSlot(EQUIP_TYPE_SWORD,2);
        check(save.comboObtainedFc[FCI_KOKIRI_SWORD] == 0 && save.comboAppliedFc[FCI_KOKIRI_SWORD] == 0,
              "replacing a borrowed sword never fabricates a native acquisition receipt");
    }
    Reset(); nativeEquipment[EQUIP_TYPE_SWORD] = EQUIP_VALUE_SWORD_GILDED; buttons[EQUIP_SLOT_B] = ITEM_SWORD_KOKIRI;
    check(!KaleidoEquip_CellOwned(&kokiri), "stale mismatched native B slot cannot prove Kokiri ownership");
    ExtEquip_SetSlot(EQUIP_TYPE_SWORD,2);
    check(save.comboObtainedFc[FCI_KOKIRI_SWORD] == 0, "stale mismatched native B slot cannot record an acquisition");
    Reset(); currentExt[EQUIP_TYPE_SWORD] = 2;
    nativeEquipment[EQUIP_TYPE_SWORD] = EQUIP_VALUE_SWORD_KOKIRI; buttons[EQUIP_SLOT_B] = ITEM_SWORD_KOKIRI;
    ExtEquip_SetSlot(EQUIP_TYPE_SWORD,3);
    check(save.comboObtainedFc[FCI_KOKIRI_SWORD] == 0, "active extended sword cannot record a stale native B acquisition");
    Reset(); nativeEquipment[EQUIP_TYPE_SWORD] = EQUIP_VALUE_SWORD_KOKIRI; buttons[EQUIP_SLOT_B] = ITEM_SWORD_KOKIRI;
    save.comboObtainedFc[FCI_KOKIRI_SWORD] = 3; save.comboAppliedFc[FCI_KOKIRI_SWORD] = 1;
    ExtEquip_SetSlot(EQUIP_TYPE_SWORD,2);
    check(save.comboObtainedFc[FCI_KOKIRI_SWORD] == 3 && save.comboAppliedFc[FCI_KOKIRI_SWORD] == 1,
          "native ownership preservation cannot consume pending progressive grants above the proven tier");
    Reset(); currentExt[EQUIP_TYPE_SHIELD] = 1;
    nativeEquipment[EQUIP_TYPE_SHIELD] = EQUIP_VALUE_SHIELD_HERO; save.shieldOwned = FC_SHIELD_DIVINE;
    check(!KaleidoEquip_CellOwned(&hero), "Divine Shield's borrowed Hero nibble cannot grant native ownership");
    currentExt[EQUIP_TYPE_SHIELD] = 0; save.vanillaShieldSkin = 1; save.shieldOwned = FC_SHIELD_DEKU;
    check(!KaleidoEquip_CellOwned(&hero), "Deku Shield's borrowed Hero nibble cannot grant native ownership");
    sEquipSubPage = 0; KaleidoEquip_EquipCell(&play,1,1);
    check(save.shieldOwned == FC_SHIELD_DEKU, "equipping earned Deku Shield never grants a Hylian Shield");
    Reset(); sEquipSubPage = 0;
    nativeEquipment[EQUIP_TYPE_SHIELD] = EQUIP_VALUE_SHIELD_MIRROR;
    save.vanillaShieldSkin = 2; save.shieldOwned = FC_SHIELD_MIRROR_OOT | FC_SHIELD_HYLIAN;
    KaleidoEquip_EquipCell(&play,1,2);
    check(save.shieldOwned == (FC_SHIELD_MIRROR_OOT | FC_SHIELD_HYLIAN),
          "swapping an OoT Mirror skin for an earned Hylian Shield cannot grant Ikana Shield");
    Reset(); sEquipSubPage = 1;
    nativeEquipment[EQUIP_TYPE_SHIELD] = EQUIP_VALUE_SHIELD_HERO;
    ExtEquip_GiveItem(EQUIP_TYPE_SHIELD,1);
    KaleidoEquip_EquipCell(&play,1,1);
    check(save.shieldOwned == FC_SHIELD_HYLIAN, "equipping an earned NEI shield retains the true native starter shield");
    check(KaleidoEquip_CellOwned(&hero), "native starter shield remains owned while an NEI shield borrows its slot");
    ExtEquip_Unequip(EQUIP_TYPE_SHIELD);
    check(KaleidoEquip_CellOwned(&hero), "native starter shield remains available after unequipping the NEI shield");
    Reset(); sEquipSubPage = 0;
    nativeEquipment[EQUIP_TYPE_SHIELD] = EQUIP_VALUE_SHIELD_HERO; save.shieldOwned = FC_SHIELD_DEKU;
    KaleidoEquip_EquipCell(&play,1,1);
    check(save.shieldOwned == (FC_SHIELD_DEKU | FC_SHIELD_HYLIAN),
          "equipping an earned Deku skin preserves only the actually equipped native starter shield");
    Reset(); currentExt[EQUIP_TYPE_SHIELD] = 1;
    nativeEquipment[EQUIP_TYPE_SHIELD] = EQUIP_VALUE_SHIELD_HERO; save.shieldOwned = FC_SHIELD_DIVINE | FC_SHIELD_KITE;
    ExtEquip_SetSlot(EQUIP_TYPE_SHIELD,2);
    check(save.shieldOwned == (FC_SHIELD_DIVINE | FC_SHIELD_KITE), "changing between NEI shields cannot preserve a borrowed native shield");
    Reset(); nativeEquipment[EQUIP_TYPE_SHIELD] = EQUIP_VALUE_SHIELD_MIRROR;
    ExtEquip_SetSlot(EQUIP_TYPE_SHIELD,1);
    check(save.shieldOwned == FC_SHIELD_IKANA, "a true native Mirror Shield retains only its own ownership before borrowing");
    // Execute the real MM ownership/export publishers. A borrowed native slot
    // must not refill the saved flags and make a hidden pause icon reappear.
    Reset(); PackNativeEquipment();
    check(ComputeShieldOwned() == 0, "sync leaves an empty shield ownership mask empty");
    check(GetEquippedShieldCanonical() == 0, "sync exports no shield for an empty native slot");
    Reset(); nativeEquipment[EQUIP_TYPE_SHIELD] = EQUIP_VALUE_SHIELD_HERO; PackNativeEquipment();
    check(ComputeShieldOwned() == FC_SHIELD_HYLIAN, "sync retains only the actual native Hero starter");
    check(GetEquippedShieldCanonical() == 2, "native Hero exports as Hylian");
    Reset(); nativeEquipment[EQUIP_TYPE_SHIELD] = EQUIP_VALUE_SHIELD_MIRROR; PackNativeEquipment();
    check(ComputeShieldOwned() == FC_SHIELD_IKANA, "native Mirror ownership cannot invent a Hylian Shield");
    check(GetEquippedShieldCanonical() == 6, "native Mirror exports as Ikana");
    const u16 extShieldBits[] = { 0, FC_SHIELD_DIVINE, FC_SHIELD_KITE, FC_SHIELD_IKANA };
    for (u8 index = 1; index <= 3; ++index) {
        Reset(); sEquipSubPage = 1;
        ExtEquip_GiveItem(EQUIP_TYPE_SHIELD,index); KaleidoEquip_EquipCell(&play,1,index);
        PackNativeEquipment();
        check(ComputeShieldOwned() == extShieldBits[index], "sync infers only the actually earned extended shield");
        check(GetEquippedShieldCanonical() == 3 + index, "each equipped NEI shield keeps its own canonical export");
        sEquipSubPage = 0; Draw(&play);
        check(!KaleidoEquip_CellOwned(&hero) && drawCounts[EQUIP_CELL(1,2)] == 0,
              "sync cannot make an unearned Hero icon reappear after a borrowed shield equip");
    }
    Reset(); sEquipSubPage = 1;
    nativeEquipment[EQUIP_TYPE_SHIELD] = EQUIP_VALUE_SHIELD_HERO;
    ExtEquip_GiveItem(EQUIP_TYPE_SHIELD,1); KaleidoEquip_EquipCell(&play,1,1); PackNativeEquipment();
    check(ComputeShieldOwned() == (FC_SHIELD_HYLIAN | FC_SHIELD_DIVINE),
          "sync preserves the true native starter previously replaced by an earned extended shield");
    Reset(); sEquipSubPage = 0; save.shieldOwned = FC_SHIELD_DEKU;
    KaleidoEquip_EquipCell(&play,1,1); PackNativeEquipment();
    check(ComputeShieldOwned() == FC_SHIELD_DEKU, "sync cannot infer Hylian from a Deku skin's borrowed Hero slot");
    check(GetEquippedShieldCanonical() == 1, "matching Deku skin exports the Deku canonical shield");
    Draw(&play);
    check(!KaleidoEquip_CellOwned(&hero) && drawCounts[EQUIP_CELL(1,2)] == 0,
          "sync cannot make an unearned Hero icon reappear after a Deku equip");
    Reset(); sEquipSubPage = 0; save.shieldOwned = FC_SHIELD_MIRROR_OOT;
    KaleidoEquip_EquipCell(&play,1,3); PackNativeEquipment();
    check(ComputeShieldOwned() == FC_SHIELD_MIRROR_OOT,
          "sync cannot infer native Hylian or Ikana ownership from an OoT Mirror skin");
    check(GetEquippedShieldCanonical() == 3, "matching OoT Mirror skin exports the OoT Mirror canonical shield");
    Draw(&play);
    check(!KaleidoEquip_CellOwned(&hero) && drawCounts[EQUIP_CELL(1,2)] == 0,
          "sync cannot make an unearned Hero icon reappear after an OoT Mirror equip");
    for (u8 skin : { (u8)1, (u8)2 }) {
        Reset(); save.vanillaShieldSkin = skin;
        nativeEquipment[EQUIP_TYPE_SHIELD] = skin == 1 ? EQUIP_VALUE_SHIELD_MIRROR : EQUIP_VALUE_SHIELD_HERO;
        PackNativeEquipment();
        check(ComputeShieldOwned() == 0, "mismatched skin and native shield nibble cannot prove shield ownership");
        check(GetEquippedShieldCanonical() == 0, "mismatched skin and native shield nibble export no invented shield");
    }
    Reset(); nativeEquipment[EQUIP_TYPE_SHIELD] = 3; PackNativeEquipment();
    check(ComputeShieldOwned() == 0, "invalid native shield nibble cannot invent a Hylian Shield");
    check(GetEquippedShieldCanonical() == 0, "invalid native shield nibble exports no shield");
    Reset(); save.shieldOwned = FC_SHIELD_DEKU | FC_SHIELD_HYLIAN | FC_SHIELD_MIRROR_OOT | FC_SHIELD_IKANA;
    ExtEquip_GiveItem(EQUIP_TYPE_SHIELD,1); ExtEquip_GiveItem(EQUIP_TYPE_SHIELD,2); PackNativeEquipment();
    check(ComputeShieldOwned() == (FC_SHIELD_DEKU | FC_SHIELD_HYLIAN | FC_SHIELD_MIRROR_OOT |
                                   FC_SHIELD_IKANA | FC_SHIELD_DIVINE | FC_SHIELD_KITE),
          "sync retains every existing earned shield bit while adding real NEI receipts");
    Reset(); CheckKokiriExport(false, "sync cannot export Kokiri ownership from an empty native sword slot");
    for (u8 tier = 1; tier <= 3; ++tier) {
        Reset(); nativeEquipment[EQUIP_TYPE_SWORD] = tier;
        buttons[EQUIP_SLOT_B] = ITEM_SWORD_KOKIRI + tier - 1;
        CheckKokiriExport(true, "a matching native Kokiri-chain sword proves sync ownership");
    }
    for (u8 borrowed : { (u8)ITEM_SWORD_MASTER, (u8)ITEM_SWORD_BGS, (u8)ITEM_EXT_SWORD_2, (u8)ITEM_EXT_SWORD_3 }) {
        Reset(); nativeEquipment[EQUIP_TYPE_SWORD] = EQUIP_VALUE_SWORD_GILDED; buttons[EQUIP_SLOT_B] = borrowed;
        if (borrowed == ITEM_EXT_SWORD_2 || borrowed == ITEM_EXT_SWORD_3) {
            currentExt[EQUIP_TYPE_SWORD] = save.extEquipSword = borrowed == ITEM_EXT_SWORD_2 ? 2 : 3;
        }
        CheckKokiriExport(false, "a borrowed Master/BGS/Four/Trident native nibble cannot publish Kokiri ownership");
    }
    Reset(); nativeEquipment[EQUIP_TYPE_SWORD] = EQUIP_VALUE_SWORD_GILDED; buttons[EQUIP_SLOT_B] = ITEM_SWORD_KOKIRI;
    CheckKokiriExport(false, "a mismatched native B tier cannot publish Kokiri ownership");
    Reset(); nativeEquipment[EQUIP_TYPE_SWORD] = EQUIP_VALUE_SWORD_KOKIRI; buttons[EQUIP_SLOT_B] = ITEM_SWORD_KOKIRI;
    currentExt[EQUIP_TYPE_SWORD] = save.extEquipSword = 2;
    CheckKokiriExport(false, "an active extended sword cannot publish stale matching native B ownership");
    Reset(); nativeEquipment[EQUIP_TYPE_SWORD] = EQUIP_VALUE_SWORD_DIETY; buttons[EQUIP_SLOT_B] = ITEM_SWORD_DEITY;
    CheckKokiriExport(false, "a Fierce Deity nibble does not prove Kokiri-chain ownership");
    Reset(); nativeEquipment[EQUIP_TYPE_SWORD] = EQUIP_VALUE_SWORD_GILDED; buttons[EQUIP_SLOT_B] = ITEM_SWORD_MASTER;
    save.comboObtainedFc[FCI_KOKIRI_SWORD] = 1;
    CheckKokiriExport(true, "a legitimate Kokiri receipt remains exportable while Master borrows Gilded");
    for (u8 upgrade : { (u8)(1 << 1), (u8)(1 << 2) }) {
        Reset(); nativeEquipment[EQUIP_TYPE_SWORD] = EQUIP_VALUE_SWORD_GILDED; buttons[EQUIP_SLOT_B] = ITEM_SWORD_MASTER;
        save.weaponUpgrades = upgrade;
        CheckKokiriExport(true, "a legitimate progressive Kokiri upgrade remains exportable while Master borrows Gilded");
    }
    Reset(); nativeEquipment[EQUIP_TYPE_SWORD] = EQUIP_VALUE_SWORD_KOKIRI; buttons[EQUIP_SLOT_B] = ITEM_SWORD_KOKIRI;
    save.comboObtainedFc[FCI_KOKIRI_SWORD] = 3; save.comboAppliedFc[FCI_KOKIRI_SWORD] = 1;
    CheckKokiriExport(true, "pending progressive Kokiri receipts remain owned without being consumed by sync");
    check(save.comboObtainedFc[FCI_KOKIRI_SWORD] == 3 && save.comboAppliedFc[FCI_KOKIRI_SWORD] == 1,
          "sync Kokiri export preserves unapplied progressive receipt deficits");
    // Incoming shield projection executes the actual setter and the exact
    // shieldOwned/equippedShield ApplyShared block against real JSON payloads.
    Reset(); nativeEquipment[EQUIP_TYPE_SHIELD] = EQUIP_VALUE_SHIELD_HERO;
    SetEquippedShieldCanonical(6);
    check(save.shieldOwned == FC_SHIELD_HYLIAN,
          "canonical native replacement records the genuine outgoing Hero starter");
    check(nativeEquipment[EQUIP_TYPE_SHIELD] == EQUIP_VALUE_SHIELD_MIRROR && save.vanillaShieldSkin == 0,
          "canonical Ikana equips the native Mirror slot without an imported skin");
    check(ComputeShieldOwned() == (FC_SHIELD_HYLIAN | FC_SHIELD_IKANA),
          "direct canonical native replacement preserves both legitimately equipped shields");
    Reset(); nativeEquipment[EQUIP_TYPE_SHIELD] = EQUIP_VALUE_SHIELD_HERO;
    save.comboObtainedFc[101] = 3; save.comboAppliedFc[101] = 1;
    ApplyIncomingShields({ {"shieldOwned", FC_SHIELD_IKANA}, {"equippedShield", 6} });
    check(save.shieldOwned == (FC_SHIELD_HYLIAN | FC_SHIELD_IKANA),
          "incoming owned Ikana and equipped canon6 preserve the fresh native Hero starter");
    check(nativeEquipment[EQUIP_TYPE_SHIELD] == EQUIP_VALUE_SHIELD_MIRROR && GetEquippedShieldCanonical() == 6,
          "incoming Ikana payload projects the native Mirror and exports the same canonical shield");
    check(save.comboObtainedFc[101] == 3 && save.comboAppliedFc[101] == 1,
          "incoming shield projection preserves unrelated earned checks and pending deficits");
    NeiSaveData incomingOnce = save;
    ApplyIncomingShields({ {"shieldOwned", FC_SHIELD_IKANA}, {"equippedShield", 6} });
    check(std::memcmp(&incomingOnce, &save, sizeof(save)) == 0,
          "repeating an incoming shield payload does not invent another acquisition");
    sEquipSubPage = 0; Draw(&play);
    check(KaleidoEquip_CellOwned(&hero) && drawCounts[EQUIP_CELL(1,2)] == 1,
          "the real native Hero starter remains on the pause page after incoming Ikana projection");
    Reset(); nativeEquipment[EQUIP_TYPE_SHIELD] = EQUIP_VALUE_SHIELD_HERO;
    ApplyIncomingShields({ {"shieldOwned", FC_SHIELD_IKANA} });
    check(save.shieldOwned == (FC_SHIELD_HYLIAN | FC_SHIELD_IKANA),
          "ownership-only incoming native projection retains the outgoing Hero starter before overwriting its nibble");
    for (int canon : { 2, 6 }) {
        Reset(); nativeEquipment[EQUIP_TYPE_SHIELD] = EQUIP_VALUE_SHIELD_MIRROR;
        save.vanillaShieldSkin = 2; save.shieldOwned = FC_SHIELD_MIRROR_OOT;
        SetEquippedShieldCanonical(canon);
        check(save.vanillaShieldSkin == 0 && GetEquippedShieldCanonical() == canon,
              "incoming native canon2/6 clears an outgoing OoT Mirror skin");
        check(save.shieldOwned == FC_SHIELD_MIRROR_OOT,
              "native canonical replacement cannot record ownership from the outgoing borrowed Mirror skin");
        check(nativeEquipment[EQUIP_TYPE_SHIELD] == (canon == 2 ? EQUIP_VALUE_SHIELD_HERO : EQUIP_VALUE_SHIELD_MIRROR),
              "native canonical replacement projects the requested Hero/Mirror nibble");
    }
    for (int canon : { 1, 3 }) {
        const u16 importedBit = canon == 1 ? FC_SHIELD_DEKU : FC_SHIELD_MIRROR_OOT;
        const u8 skin = canon == 1 ? 1 : 2;
        const u8 nibble = canon == 1 ? EQUIP_VALUE_SHIELD_HERO : EQUIP_VALUE_SHIELD_MIRROR;
        Reset(); nativeEquipment[EQUIP_TYPE_SHIELD] = EQUIP_VALUE_SHIELD_HERO;
        ApplyIncomingShields({ {"shieldOwned", importedBit}, {"equippedShield", canon} });
        check(save.vanillaShieldSkin == skin && nativeEquipment[EQUIP_TYPE_SHIELD] == nibble,
              "incoming canonical Deku/OoT Mirror uses the matching native alias and skin");
        check(save.shieldOwned == (FC_SHIELD_HYLIAN | importedBit),
              "incoming imported shield retains only its earned bit and the genuine replaced native Hero");
        check(GetEquippedShieldCanonical() == canon && ComputeShieldOwned() == (FC_SHIELD_HYLIAN | importedBit),
              "incoming imported canonical shield round-trips without another ownership inference");
        Reset(); ExtEquip_GiveItem(EQUIP_TYPE_SHIELD,1); ExtEquip_SetSlot(EQUIP_TYPE_SHIELD,1);
        ApplyIncomingShields({ {"shieldOwned", importedBit}, {"equippedShield", canon} });
        check(currentExt[EQUIP_TYPE_SHIELD] == 0 && save.extEquipShield == 0,
              "incoming imported canonical shield releases the outgoing extended shield in RAM and save");
        check(save.vanillaShieldSkin == skin && nativeEquipment[EQUIP_TYPE_SHIELD] == nibble &&
              GetEquippedShieldCanonical() == canon,
              "incoming imported shield replaces an extended base with a coherent native alias");
        check(ComputeShieldOwned() == (FC_SHIELD_DIVINE | importedBit),
              "incoming imported shield never records Hylian or Ikana from an outgoing extended base");
    }
    Reset(); sEquipSubPage = 1;
    ExtEquip_GiveItem(EQUIP_TYPE_SHIELD,1); ExtEquip_SetSlot(EQUIP_TYPE_SHIELD,1);
    ApplyIncomingShields({ {"shieldOwned", FC_SHIELD_IKANA} });
    check(ComputeShieldOwned() == (FC_SHIELD_DIVINE | FC_SHIELD_IKANA) && !KaleidoEquip_CellOwned(&hero),
          "incoming real Ikana ownership cannot preserve a borrowed Hero from an outgoing extended shield");
    for (int skin : { 1, 2 }) {
        Reset(); save.vanillaShieldSkin = skin;
        const u8 nibble = skin == 1 ? EQUIP_VALUE_SHIELD_HERO : EQUIP_VALUE_SHIELD_MIRROR;
        const u16 ownedBit = skin == 1 ? FC_SHIELD_DEKU : FC_SHIELD_MIRROR_OOT;
        nativeEquipment[EQUIP_TYPE_SHIELD] = nibble; save.shieldOwned = ownedBit;
        ApplyIncomingShields({ {"shieldOwned", FC_SHIELD_IKANA} });
        check(nativeEquipment[EQUIP_TYPE_SHIELD] == nibble && save.vanillaShieldSkin == skin,
              "ownership-only shield delta preserves the currently equipped imported alias and skin");
        check(GetEquippedShieldCanonical() == (skin == 1 ? 1 : 3),
              "ownership-only shield delta preserves the currently equipped imported identity");
        check(ComputeShieldOwned() == (ownedBit | FC_SHIELD_IKANA),
              "ownership-only shield delta adds its real receipt without inventing a native Hero");
    }
    for (u8 index : { (u8)1, (u8)2 }) for (bool explicitSameEquip : { false, true }) {
        Reset(); ExtEquip_GiveItem(EQUIP_TYPE_SHIELD,index); ExtEquip_SetSlot(EQUIP_TYPE_SHIELD,index);
        nlohmann::json payload = { {"shieldOwned", FC_SHIELD_IKANA} };
        if (explicitSameEquip) payload["equippedShield"] = 3 + index;
        ApplyIncomingShields(payload);
        check(nativeEquipment[EQUIP_TYPE_SHIELD] == EQUIP_VALUE_SHIELD_HERO,
              "new shield ownership preserves the active Divine/Kite native base even when the ext setter is unchanged");
        check(GetEquippedShieldCanonical() == 3 + index && currentExt[EQUIP_TYPE_SHIELD] == index,
              "new shield ownership preserves the active NEI shield identity");
        check(ComputeShieldOwned() == (extShieldBits[index] | FC_SHIELD_IKANA),
              "new shield ownership retains only real extended and incoming shield receipts");
    }
    // Use the real grant accessor to acquire each NEI piece independently; never fill unrelated bits.
    for (s16 row = 0; row < 4; ++row) for (s16 col = 1; col <= 3; ++col) {
        Reset(); sEquipSubPage = 1;
        ExtEquip_GiveItem(row,col);
        NeiSaveData before = save;
        Draw(&play);
        check(GridDraws() == 1 && drawCounts[EQUIP_CELL(row,col)] == 1, "a real NEI grant reveals exactly its own cell");
        check(KaleidoEquip_CursorCanSit(row,col), "earned NEI equipment is selectable");
        sEquipCursorX = col; sEquipCursorY = row; Publish(&play.pauseCtx);
        check(play.pauseCtx.cursorItem[PAUSE_MASK] == ExtEquip_GetItemId(row,col), "earned NEI equipment retains its correct title ID");
        KaleidoEquip_EquipCell(&play,row,col);
        check(currentExt[row] == col && enabled, "earned NEI equipment remains equippable");
        switch (row) {
            case 0: before.extEquipSword = col; break;
            case 1: before.extEquipShield = col; break;
            case 2: before.extEquipTunic = col; break;
            case 3: before.extEquipBoots = col; break;
        }
        check(std::memcmp(&before, &save, sizeof(save)) == 0, "equipping earned NEI equipment never fabricates ownership or checks");
    }
    // Full earned loadout: native equipment flags, imports and all 12 legitimate NEI grants survive.
    Reset(); nativeEquipment[EQUIP_TYPE_SWORD] = EQUIP_VALUE_SWORD_GILDED;
    buttons[EQUIP_SLOT_B] = ITEM_SWORD_GILDED; swordLevel = 2;
    save.shieldOwned = FC_SHIELD_DEKU | FC_SHIELD_HYLIAN | FC_SHIELD_MIRROR_OOT | FC_SHIELD_IKANA;
    for (int id : { FC_OOT_SWORD_MASTER, FC_OOT_SWORD_BIGGORON, FC_OOT_TUNIC_GORON, FC_OOT_TUNIC_ZORA,
                    FC_OOT_BOOTS_IRON, FC_OOT_BOOTS_HOVER }) save.comboObtained[id] = 1;
    for (s16 row = 0; row < 4; ++row) for (u8 col = 1; col <= 3; ++col) ExtEquip_GiveItem(row,col);
    save.comboObtainedFc[101] = 6; save.comboAppliedFc[101] = 6;
    NeiSaveData before = save;
    for (s16 subpage = 0; subpage <= 1; ++subpage) {
        sEquipSubPage = subpage; Draw(&play);
        check(GridDraws() == 12, "all genuinely earned equipment remains visible on both pages");
        for (s16 row = 0; row < 4; ++row) for (s16 col = 1; col <= 3; ++col) {
            check(KaleidoEquip_CursorCanSit(row,col), "all earned cells remain selectable");
            sEquipCursorX = col; sEquipCursorY = row; Publish(&play.pauseCtx);
            check(play.pauseCtx.cursorItem[PAUSE_MASK] != PAUSE_ITEM_NONE, "all earned cells retain titles");
        }
    }
    check(std::memcmp(&before, &save, sizeof(save)) == 0, "owned loadout and earned check registries survive both page draws");
    std::printf("%s: MM equipment ownership (%d checks, %d failures)\n", failures ? "FAIL" : "PASS", checks, failures);
    return failures != 0;
}
'''
    return "#include <initializer_list>\n" + code


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-ref", help="Execute production code from a git ref without changing HEAD")
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    selectors.SOURCE_REF = args.source_ref
    with tempfile.TemporaryDirectory(prefix="equipment-ownership-") as directory:
        native_syntax(Path(directory))
        source = Path(directory) / "ownership.cpp"
        binary = Path(directory) / "ownership"
        source.write_text(fixture())
        flags = ["-std=c++20", "-Wall", "-Wextra", "-Werror", "-Wno-unused-function", "-Wno-unused-variable", "-Wno-unused-parameter"]
        if args.sanitize:
            flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-g"]
        subprocess.run([os.environ.get("CXX", "c++"), *flags, f"-I{ROOT}", f"-I{ROOT/'mm'}", str(source), "-o", str(binary)], check=True)
        env = dict(os.environ, ASAN_OPTIONS="detect_leaks=0", UBSAN_OPTIONS="halt_on_error=1")
        return subprocess.run([str(binary)], env=env).returncode


if __name__ == "__main__":
    raise SystemExit(main())
