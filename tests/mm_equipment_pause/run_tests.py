#!/usr/bin/env python3
"""Execute production equipment selectors and name-panel dispatch, without a GPU."""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SOURCE_REF = None


def read_source(path):
    if SOURCE_REF:
        return subprocess.check_output(["git", "show", f"{SOURCE_REF}:{path.relative_to(ROOT).as_posix()}"],
                                       cwd=ROOT, text=True)
    return path.read_text()


def function(source, name):
    match = re.search(r"^(?:static\s+)?[^\n{};]+\b" + re.escape(name) + r"\([^;{}]*\)\s*\{", source, re.M)
    if not match:
        raise ValueError(name)
    start, cursor, depth = match.start(), match.end(), 1
    while depth:
        depth += (source[cursor] == "{") - (source[cursor] == "}")
        cursor += 1
    return source[start:cursor]


def between(source, start, end):
    first = source.index(start)
    return source[first:source.index(end, first)]


def defines(source, prefix):
    return "\n".join(re.findall(r"^#define " + prefix + r"[^\n]*", source, re.M)) + "\n"


COMMON = r'''
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <unordered_set>
#include "combo/menu/ComboItemIconOwnership.h"
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
typedef int16_t s16; typedef int32_t s32;
#define ARRAY_COUNTU(a) (sizeof(a) / sizeof((a)[0]))
static int failures, checks;
static void check(bool ok, const char* label) {
    ++checks;
    if (!ok) { fprintf(stderr, "FAIL: %s\n", label); ++failures; }
}
static void name_is(void* actual, const char* expected, const char* label) {
    check(actual && strcmp((const char*)actual, expected) == 0, label);
}
static bool localBase = true, localAlt, localAltEnabled, donorAvailable;
u8 ResourceMgr_FileExists(const char* path) { return path && localBase; }
u8 ResourceMgr_FileAltExists(const char* path) { return path && localAlt; }
bool ResourceMgr_IsAltAssetsEnabled(void) { return localAltEnabled; }
// This selector/name fixture controls availability at the same local/Alt seam.
// Real routed texture/type rejection is exercised by run_missing_icon_tests.py.
u8 MM_IsRgba32TextureAvailable(const char* path) {
    return path && (localBase || (localAltEnabled && localAlt));
}
int NeiResource_Available(const char* path) { return path && donorAvailable; }
// Interning models the existing bridge's lifetime; the production selector is under test.
const char* NeiResource_Route(const char* path) {
    static std::unordered_set<std::string> paths;
    return paths.insert(std::string("__OTR__@oot:") + (path + 7)).first->c_str();
}
static u8 capeOwned, pendantOwned, gExtEquipGridNameContext;
#define osSyncPrintf(...) ((void)0)
u8 ExtEquip_CapeOwned(void) { return capeOwned; }
u8 ExtEquip_PendantOwned(void) { return pendantOwned; }
void* ExtInv_GetCustomItemNameTex(uint16_t, uint8_t) { return NULL; }
static u8 swordLevel, greatFairy;
u8 WeaponUpgrade_KokiriLevel(void) { return swordLevel; }
u8 WeaponUpgrade_HasGreatFairy(void) { return greatFairy; }
u8 ExtEquip_SlotRetired(s16, u8) { return false; }
'''


def fixture(host):
    equipment = read_source(ROOT / host / "mods/extended_equipment.c")
    names = read_source(ROOT / host / "mods/equipment/ext_equip_names.c")
    code = COMMON + f'\n#include "{host}/include/z64item.h"\n'
    code += f'#include "{host}/assets/{"2s2h" if host == "mm" else "soh"}_assets.h"\n'
    if host == "mm":
        code += '#include "mm/mods/equipment/ext_equip_name_assets.h"\n'
        code += '#include "mm/mods/equipment/ext_equip_icon_assets.h"\n'
    code += defines((ROOT / host / "mods/extended_equipment.h").read_text(), "ITEM_EXT_")
    code += function(names, "ExtEquip_LookupNameTex") + "\n"
    code += function(equipment, "ExtEquip_GetNameTex") + "\n"
    code += function(equipment, "ExtEquip_GetCapeIcon") + "\n"
    tests = r'''
    gExtEquipGridNameContext = 0;
    name_is(ExtEquip_GetNameTex(0xE6, 0), "__OTR__textures/item_name_custom/gMagicCapeNameTex", "passive Cape title");
    name_is(ExtEquip_GetNameTex(0xEA, 0), "__OTR__item_name_static/gItemNamePendantOfMemoriesENGTex", "passive Pendant title");
    name_is(ExtEquip_GetCapeIcon(), "__OTR__textures/icon_item_custom/gItemIconMagicCapePassiveTex", "Cape cannot use legacy purple-tunic override");
    gExtEquipGridNameContext = 1;
    name_is(ExtEquip_GetNameTex(0xE6, 0), "__OTR__textures/item_name_custom/gChampionsTunicNameTex", "grid Champion title");
    name_is(ExtEquip_GetNameTex(0xEA, 0), "__OTR__textures/item_name_custom/gClimbBootsNameTex", "grid Climb Boots title");
    check(ExtEquip_GetNameTex(0xFFFF, 0) == NULL, "invalid extended ID stays unnamed");
'''
    if host == "mm":
        folder = ROOT / "mm/src/overlays/kaleido_scope/ovl_kaleido_scope"
        equip = read_source(folder / "z_kaleido_equipment.c")
        kaleido = read_source(folder / "z_kaleido_scope_NES.c")
        code += '#include "mm/assets/archives/item_name_static/item_name_static.h"\n'
        code += function(equipment, "ExtEquip_GetItemId") + "\n"
        for prefix in ("EQUIP_SUBPAGE_", "EQUIP_UPGRADE_ROW_", "CELL_", "OOT_ICON_"):
            code += defines(equip, prefix)
        code += r'''
#define EQUIP_TYPE_BOOTS 3
#define PAUSE_ITEM_NONE 999
#define PAUSE_ITEM 0
#define PAUSE_MAP 1
#define PAUSE_QUEST 2
#define PAUSE_MASK 3
#define PAUSE_NAME_COLOR_SET_WHITE 0
#define PAUSE_MAIN_STATE_IDLE_CURSOR_ON_SONG 9
#define EQUIP_CELL(row,col) ((row)*6+((col)?(col)+2:0))
static s16 sEquipSubPage, sEquipCursorX, sEquipCursorY;
static bool sInDungeonScene;
typedef struct { s16 item, equipType, index; const char* ootIcon; } EquipCell;
// These title/resource fixtures browse acquired equipment. The production ownership,
// draw, cursor and action paths are exercised separately by run_ownership_tests.py.
u8 KaleidoEquip_CellOwned(EquipCell*) { return true; }
typedef struct {
    u16 cursorItem[4], cursorSlot[4], namedItem, pageIndex;
    void* nameSegment;
    s16 nameDisplayTimer, nameColorSet, mainState, cursorSpecialPos;
} PauseContext;
typedef struct { PauseContext pauseCtx; } PlayState;
void Kaleido_LoadMapNameStatic(void**, u32) {}
'''
        for name in ("KaleidoEquip_OotTex", "KaleidoEquip_UpgradeValue", "KaleidoEquip_UpgradeNameItem", "KaleidoEquip_GetCell"):
            code += function(equip, name) + "\n"
        if "KaleidoEquip_GetNameTex(" in equip:
            code += function(equip, "KaleidoEquip_GetNameTex") + "\n"
        else:
            code += "void* KaleidoEquip_GetNameTex(void) { return NULL; }\n"
        publish = between(equip, "    // --- Publish cursor state", "    // Keep the shared cursor machinery")
        code += "void Publish(PauseContext* pauseCtx) { EquipCell cell;\n" + publish + "}\n"
        # C permits the existing const texture-path assignments to void*. C++ needs
        # explicit qualifier casts; these preserve the exact pointer and selection.
        loader = function(kaleido, "Kaleido_LoadItemNameStatic")
        loader = loader.replace("*segment = gItemNameStatics[texIndex];", "*segment = (void*)gItemNameStatics[texIndex];")
        loader = loader.replace("*segment = gEmptyTexture;", "*segment = (void*)gEmptyTexture;")
        code += loader + "\n"
        code += function(kaleido, "KaleidoScope_UpdateNamePanel") + "\n"
        tests += r'''
    PlayState play = {}; PauseContext* pause = &play.pauseCtx;
    pause->pageIndex = PAUSE_MASK; capeOwned = pendantOwned = 1;
    sEquipSubPage = 1;
    static const char* ext[] = { "gCaneOfByrnaNameTex", "gFourSwordNameTex", "gTridentNameTex",
        "gGoddessShieldNameTex", "gKiteShieldNameTex", "gShieldOfIkanaNameTex",
        "gChampionsTunicNameTex", "gMagicTunicNameTex", "gSagesTunicNameTex",
        "gPegasusBootsNameTex", "gClimbBootsNameTex", "gRocBootsNameTex" };
    for (s16 r=0; r<4; ++r) for (s16 c=1; c<=3; ++c) {
        sEquipCursorY=r; sEquipCursorX=c; Publish(pause); KaleidoScope_UpdateNamePanel(&play);
        check(pause->cursorItem[PAUSE_MASK] == 0xE0+r*3+c-1, "every extended cell publishes full ID");
        const std::string expected = std::string("__OTR__textures/item_name_custom/")+ext[r*3+c-1];
        name_is(pause->nameSegment, expected.c_str(), "every extended cell loads its title");
    }
    sEquipCursorX=0; sEquipCursorY=0; Publish(pause); KaleidoScope_UpdateNamePanel(&play);
    name_is(pause->nameSegment, "__OTR__textures/item_name_custom/gMagicCapeNameTex", "Cape through live name panel");
    sEquipCursorY=1; Publish(pause); KaleidoScope_UpdateNamePanel(&play);
    name_is(pause->nameSegment, "__OTR__item_name_static/gItemNamePendantOfMemoriesENGTex", "Pendant through live name panel");
    // Same item ID in two contexts must reload, including when both context and slot change.
    sEquipSubPage=1; sEquipCursorY=3; sEquipCursorX=2; Publish(pause); KaleidoScope_UpdateNamePanel(&play);
    name_is(pause->nameSegment, "__OTR__textures/item_name_custom/gClimbBootsNameTex", "Pendant to Climb cache refresh");
    sEquipCursorY=2; sEquipCursorX=1; Publish(pause); KaleidoScope_UpdateNamePanel(&play);
    sEquipCursorX=0; sEquipCursorY=0; Publish(pause); KaleidoScope_UpdateNamePanel(&play);
    name_is(pause->nameSegment, "__OTR__textures/item_name_custom/gMagicCapeNameTex", "Champion to Cape cache refresh");
    capeOwned=pendantOwned=0;
    Publish(pause); check(pause->cursorItem[PAUSE_MASK] == PAUSE_ITEM_NONE, "unowned passive has no title");
    sEquipSubPage=0; sEquipCursorX=1; sEquipCursorY=1; Publish(pause); KaleidoScope_UpdateNamePanel(&play);
    name_is(pause->nameSegment, "__OTR__textures/item_name_static/gDekuShieldItemNameENGTex", "Deku shield title by cell");
    sEquipCursorX=2; Publish(pause); KaleidoScope_UpdateNamePanel(&play);
    name_is(pause->nameSegment, "__OTR__textures/item_name_static/gHylianShieldItemNameENGTex", "shared shield ID refresh");
    localBase=false; donorAvailable=true;
    static const char* imported[] = { "gItemIconSwordMasterTex", "gItemIconTunicGoronTex",
        "gItemIconTunicZoraTex", "gItemIconBootsIronTex", "gItemIconBootsHoverTex" };
    for (auto name : imported) {
        const std::string path=std::string("__OTR__textures/icon_item_static/")+name;
        const std::string routed=std::string("__OTR__@oot:")+path.substr(7);
        name_is(KaleidoEquip_OotTex(path.c_str()), routed.c_str(), "imported icon in donor-only archive");
    }
    static const char* importedTitles[] = {"gMasterSwordItemNameENGTex", "gGoronTunicItemNameENGTex",
        "gZoraTunicItemNameENGTex", "gIronBootsItemNameENGTex", "gHoverBootsItemNameENGTex"};
    const s16 rows[]={0,2,2,3,3}, cols[]={2,2,3,2,3};
    for (int i=0;i<5;++i) {
        sEquipCursorY=rows[i]; sEquipCursorX=cols[i]; Publish(pause); KaleidoScope_UpdateNamePanel(&play);
        const std::string path=std::string("__OTR__@oot:textures/item_name_static/")+importedTitles[i];
        name_is(pause->nameSegment,path.c_str(),"imported equipment uses donor title");
    }
    const char* path="__OTR__textures/icon_item_static/gItemIconSwordMasterTex";
    localBase=true; name_is(KaleidoEquip_OotTex(path),path,"local replacement retains precedence");
    localBase=false; localAlt=true; localAltEnabled=true;
    name_is(KaleidoEquip_OotTex(path),path,"enabled local Alt-only texture keeps local owner");
    localAltEnabled=false;
    name_is(KaleidoEquip_OotTex(path),"__OTR__@oot:textures/icon_item_static/gItemIconSwordMasterTex","disabled local Alt uses donor owner");
    // A resource-owner change must refresh the title without moving the cursor.
    localAltEnabled=true;
    sEquipCursorY=0; sEquipCursorX=2; Publish(pause); KaleidoScope_UpdateNamePanel(&play);
    name_is(pause->nameSegment,"__OTR__textures/item_name_static/gMasterSwordItemNameENGTex","enabled local Alt title keeps local owner");
    localAltEnabled=false; KaleidoScope_UpdateNamePanel(&play);
    name_is(pause->nameSegment,"__OTR__@oot:textures/item_name_static/gMasterSwordItemNameENGTex","stationary Alt disable refreshes title owner");
    localAltEnabled=true; KaleidoScope_UpdateNamePanel(&play);
    name_is(pause->nameSegment,"__OTR__textures/item_name_static/gMasterSwordItemNameENGTex","stationary Alt enable restores local title");
    localAltEnabled=false; donorAvailable=false; KaleidoScope_UpdateNamePanel(&play);
    name_is(pause->nameSegment,"__OTR__textures/virtual/gEmptyTexture","stationary unavailable title clears old owner");
    donorAvailable=true; KaleidoScope_UpdateNamePanel(&play);
    name_is(pause->nameSegment,"__OTR__@oot:textures/item_name_static/gMasterSwordItemNameENGTex","stationary available donor restores title");
    pause->nameDisplayTimer=12; KaleidoScope_UpdateNamePanel(&play);
    check(pause->nameDisplayTimer==13,"unchanged title owner preserves display timing");
    donorAvailable=false;
    check(KaleidoEquip_OotTex(path)==NULL,"missing resource stays absent");
    check(KaleidoEquip_OotTex(NULL)==NULL,"null resource is safe");
    localBase=true;
    void* name=NULL; Kaleido_LoadItemNameStatic(&name,ITEM_SWORD_KOKIRI);
    name_is(name,"__OTR__item_name_static/gItemNameKokiriSwordENGTex","native MM title remains native");
    sEquipCursorY=0; sEquipCursorX=3; greatFairy=1; Publish(pause); KaleidoScope_UpdateNamePanel(&play);
    name_is(pause->nameSegment,"__OTR__item_name_static/gItemNameGreatFairysSwordENGTex","Great Fairy upgrade keeps MM title");
    Kaleido_LoadItemNameStatic(&name,0x100E6);
    name_is(name,"__OTR__textures/virtual/gEmptyTexture","oversized title ID cannot wrap");
    gExtEquipGridNameContext=1; pause->pageIndex=PAUSE_ITEM;
    pause->cursorItem[PAUSE_ITEM]=ITEM_EXT_TUNIC_1; pause->namedItem=ITEM_EXT_TUNIC_1;
    KaleidoScope_UpdateNamePanel(&play);
    check(gExtEquipGridNameContext==0,"leaving equipment page clears grid context");
    name_is(pause->nameSegment,"__OTR__textures/item_name_custom/gMagicCapeNameTex","inventory does not inherit grid title");
'''
    else:
        folder = ROOT / "soh/src/overlays/misc/ovl_kaleido_scope"
        equip = read_source(folder / "z_kaleido_equipment.c")
        start = ("        // Equipment name context" if "// Equipment name context" in equip
                 else "        gExtEquipGridNameContext = false;")
        publish = between(equip, start, "        if (!extEquipPage")
        code += function(equipment, "ExtEquip_GetItemId") + "\n"
        code += r'''
#define PAUSE_EQUIP 3
#define PAUSE_ITEM_NONE 999
static const s16 sEquipmentItemOffsets[16] = {};
typedef struct {
    s16 cursorX[4], cursorY[4], cursorPoint[4], cursorSpecialPos, cursorColorSet;
    u16 namedItem;
} PauseContext;
u16 Publish(PauseContext* pauseCtx, bool extEquipPage) {
    u16 cursorItem = PAUSE_ITEM_NONE;
'''
        code += publish + "return cursorItem; }\n"
        tests += r'''
    PauseContext pause={}; capeOwned=pendantOwned=1; gExtEquipGridNameContext=1;
    pause.cursorY[PAUSE_EQUIP]=0; pause.namedItem=0xE6;
    check(Publish(&pause,true)==0xE6,"SoH owned Cape publishes title ID");
    check(pause.namedItem==PAUSE_ITEM_NONE,"SoH grid-to-passive title refresh");
    pause.cursorY[PAUSE_EQUIP]=1;
    check(Publish(&pause,false)==0xEA,"SoH owned Pendant publishes title ID");
    capeOwned=pendantOwned=0;
    check(Publish(&pause,true)==PAUSE_ITEM_NONE,"SoH unowned passive stays unnamed");
    pause.cursorX[PAUSE_EQUIP]=1; pause.cursorY[PAUSE_EQUIP]=2;
    pause.namedItem=0xE6;
    check(Publish(&pause,true)==0xE6,"SoH Champion publishes same shared ID");
    check(pause.namedItem==PAUSE_ITEM_NONE && gExtEquipGridNameContext,"SoH passive-to-grid refresh");
'''
    return code + '\nint main() {\n' + tests + f'\nprintf("{host}: %d checks, %d failures\\n", checks, failures); return failures ? 1 : 0; }}\n'


def main():
    global SOURCE_REF
    parser = argparse.ArgumentParser()
    parser.add_argument("--sanitize", action="store_true")
    parser.add_argument("--source-ref", help="Read production source from a git ref to run a control without changing HEAD")
    args = parser.parse_args()
    SOURCE_REF = args.source_ref
    result = 0
    with tempfile.TemporaryDirectory(prefix="equipment-pause-") as td:
        for host in ("mm", "soh"):
            source = Path(td) / f"{host}.cpp"
            binary = Path(td) / host
            source.write_text(fixture(host))
            flags = ["-std=c++20", "-Wall", "-Wextra", "-Werror", "-Wno-unused-function", "-Wno-unused-variable", "-Wno-unused-parameter"]
            if args.sanitize:
                flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-g"]
            subprocess.run(["g++", *flags, f"-I{ROOT}", f"-I{ROOT/host}", f"-I{ROOT/host/'include'}",
                            str(source), "-o", str(binary)], check=True)
            env = dict(os.environ, ASAN_OPTIONS="detect_leaks=0", UBSAN_OPTIONS="halt_on_error=1")
            result |= subprocess.run([str(binary)], env=env).returncode
    return result


if __name__ == "__main__":
    raise SystemExit(main())
