#!/usr/bin/env python3
"""Exercise actual Wand icon selectors and wheel builders without game state.

Compiles the exact C functions from each host, checks sparse wheel ownership,
all 256 selector inputs, native/foreign receipt mapping and 32px logical slots.
Also compares grant/ownership/confirmation functions against the POC baseline.
This proves presentation routing, not runtime drawing or game acceptance.
"""
import re
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
BASE = "122dd5f68cb37fcf515c3726f8a77043db5dcdf4"
RODS = [("SAND_ROD", "SandRod"), ("TORNADO_ROD", "TornadoRod"), ("WATER_ROD", "WaterRod"),
        ("METEOR_ROD", "MeteorRod"), ("STORM_ROD", "StormRod"), ("SHADOW_SCEPTER", "ShadowScepter")]


def function(source, name):
    match = re.search(r"^(?:static\s+)?[^\n{};]+\b" + name + r"\([^;{}]*\)\s*\{", source, re.M)
    assert match, name
    start, cursor, depth = match.start(), match.end(), 1
    while depth:
        depth += (source[cursor] == "{") - (source[cursor] == "}")
        cursor += 1
    return source[start:cursor]


def verify_sources():
    oot = (ROOT / "soh/soh/Enhancements/randomizer/item_list.cpp").read_text()
    mm = (ROOT / "mm/2s2h/Rando/StaticData/Items.cpp").read_text()
    for enum, name in [("ELEMENTAL_WAND", "ElementalWand")] + [("WAND_"+k, v) for k, v in RODS]:
        row = next(x for x in oot.splitlines() if "itemTable[RG_"+enum+"] =" in x)
        assert ".CustomIcon(gItemIcon" + name + "Tex);" in row, (enum, "OOT receipt path/default32")
        pattern = (r"case RI_OOT_NEI_" + enum + r":\s*return \"__OTR__textures/icon_item_custom/gItemIcon"
                   + name + r"Tex\";")
        assert re.search(pattern, mm), (enum, "MM native/foreign receipt path")
        path = "textures/icon_item_custom/gItemIcon" + name + "Tex"
        for host, header in (("soh", "soh_assets.h"), ("mm", "2s2h_assets.h")):
            assert '"__OTR__' + path + '"' in (ROOT / host / "assets" / header).read_text()
            target = ROOT / host / "assets/custom" / path
            assert target.with_name(target.name + ".rgba32.png").exists(), (path, "missing built-in fallback")
    item_header = (ROOT / "soh/soh/Enhancements/randomizer/item.h").read_text()
    assert "CustomIconSize iconSize_ = ICON_SIZE_32" in item_header
    foreign_oot = (ROOT / "combo/menu/ComboItemDrawOOT.h").read_text()
    assert "out->path = item.GetCustomIcon();" in foreign_oot
    assert "item.GetCustomIconSize() == ICON_SIZE_24 ? 24 : 32" in foreign_oot
    assert 'strstr(texturePath, "icon_item_custom")' in mm and "size = 32; // custom" in mm
    message = (ROOT / "soh/soh/Enhancements/randomizer/Messages/ItemMessages.cpp").read_text()
    assert "customIcon = Rando::StaticData::RetrieveItem(rgid).GetCustomIcon();" in message
    assert "iconSize = Rando::StaticData::RetrieveItem(rgid).GetCustomIconSize();" in message


def verify_selectors():
    for host in ("soh", "mm"):
        inventory_path = host + "/mods/extended_inventory.c"
        wheel_path = host + "/mods/items/logic/item_elemental_wand.c"
        inventory = (ROOT / inventory_path).read_text()
        wheel = (ROOT / wheel_path).read_text()
        old_inventory = subprocess.check_output(["git", "show", BASE+":"+inventory_path], cwd=ROOT, text=True)
        old_wheel = subprocess.check_output(["git", "show", BASE+":"+wheel_path], cwd=ROOT, text=True)
        for name in ("Wand_GrantMode", "Wand_SetModeOwned", "Wand_SetMode", "Wand_ModeOwned", "Wand_ModeAt", "Wand_ModeCount"):
            assert function(inventory, name) == function(old_inventory, name), (host, "gameplay changed", name)
        if host == "soh":
            assert function(wheel, "Wand_OnWheelConfirm") == function(old_wheel, "Wand_OnWheelConfirm")
        else:
            # MM now reloads cached HUD pointers; the selection statement remains native.
            assert "Wand_SetMode(Wand_ModeAt((u8)index));" in function(wheel, "Wand_OnWheelConfirm")
        array = re.search(r"static void\* const sWandIconTex\[WAND_MODE_COUNT\] = \{.*?\};", inventory, re.S).group(0)
        declarations = "\n".join('static const char gItemIcon'+n+'Tex[] = "'+n+'";'
                                  for n in ["ElementalWand"]+[x[1] for x in RODS])
        expected = ",".join('"'+x[1]+'"' for x in RODS)
        code = """
#include <stdint.h>
#include <assert.h>
#include <string.h>
typedef uint8_t u8;
typedef int32_t s32;
#define WAND_MODE_COUNT 6
typedef struct { const char* iconPath; u8 iconSize; u8 enabled; } BoxMenuEntry;
static u8 owned[6];
static s32 owned_count;
static u8 selected_mode;
static s32 Wand_ModeCount(void) { return owned_count; }
static u8 Wand_ModeAt(u8 index) { assert(index < owned_count); return owned[index]; }
static void Wand_SetMode(u8 mode) { selected_mode = mode; }
""" + ("""
#define ITEM_ELEMENTAL_WAND 0xD0
static void* gPlayState;
static int refreshed;
static void ExtInv_RefreshButtonIconsForItem(void* play, uint16_t item) {
    assert(play == gPlayState && item == ITEM_ELEMENTAL_WAND); refreshed++;
}
""" if host == "mm" else "") + declarations + "\n" + array + "\n" + function(inventory, "Wand_ModeIcon") + "\n" + function(wheel, "Wand_BuildWheel") + "\n" + function(wheel, "Wand_OnWheelConfirm") + "\n" + """
int main(void) {
    const char* expected[6] = {""" + expected + """};
    for (int i = 0; i < 256; i++) {
        assert(strcmp((const char*)Wand_ModeIcon((u8)i), i < 6 ? expected[i] : "ElementalWand") == 0);
    }
    BoxMenuEntry rows[6];
    for (int pass = 0; pass < 3; pass++) {
        owned_count = pass == 0 ? 6 : (pass == 1 ? 3 : 0);
        for (int i = 0; i < 6; i++) { owned[i] = (u8)i; rows[i].enabled = 77; }
        if (pass == 1) { owned[0] = 5; owned[1] = 1; owned[2] = 3; }
        assert(Wand_BuildWheel(rows) == owned_count);
        for (int i = 0; i < owned_count; i++) {
            assert(strcmp(rows[i].iconPath, expected[owned[i]]) == 0);
            assert(rows[i].iconSize == 32 && rows[i].enabled == 1);
        }
        for (int i = owned_count; i < 6; i++) { assert(rows[i].enabled == 77); }
    }
    owned_count = 3; owned[0] = 5; owned[1] = 1; owned[2] = 3;
    Wand_OnWheelConfirm(2); assert(selected_mode == 3);
""" + ("""
    assert(refreshed == 0);
    gPlayState = &refreshed;
    Wand_OnWheelConfirm(0); assert(selected_mode == 5 && refreshed == 1);
""" if host == "mm" else "") + """
    return 0;
}
"""
        with tempfile.TemporaryDirectory(prefix="nei-icon-selectors-") as directory:
            source, binary = Path(directory)/"probe.c", Path(directory)/"probe"
            source.write_text(code)
            subprocess.run(["cc", "-std=c99", "-Wall", "-Wextra", "-Werror", str(source), "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True)
        print(host + ": actual icon selector and wheel pass all 256 modes, sparse/empty ownership, logical 32px size and confirmation; grants unchanged" + ("; cached HUD refresh verified" if host == "mm" else ""))


if __name__ == "__main__":
    verify_sources()
    verify_selectors()
    print("native/foreign receipt paths and default 32px logical icon slots verified")
