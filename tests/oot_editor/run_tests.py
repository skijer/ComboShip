#!/usr/bin/env python3
"""Run OoT editor grants against production inventory stores and power handlers."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_mm_scene_randomization_tests import function

parser = argparse.ArgumentParser()
parser.add_argument("--control", action="store_true", help="Exercise the baseline editor; expected to fail")
args = parser.parse_args()
inventory = (ROOT / "soh/mods/extended_inventory.c").read_text()
store = (ROOT / "soh/mods/nei_save.cpp").read_text()
cane = (ROOT / "soh/mods/items/logic/item_cane_of_somaria.c").read_text()
helper = ROOT / "soh/soh/Enhancements/debugger/NeiEditorItems.h"

prefix = r'''
#include "z64.h"
#include "macros.h"
#include "mods/extended_inventory.h"
extern "C" {
#include "mods/items/logic/item_cane_of_somaria.h"
}
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>
SaveContext gSaveContext{};
static NeiSaveData gNeiSave{};
int wandRule = WAND_RANDO_MEDALLIONS;
uint8_t seasonsRule = NEI_SEASONS_ROD;
uint32_t gBitFlags[32];
extern "C" void ItemGrantAudit_Begin(const char*, int, int, int) {}
extern "C" void ItemGrantAudit_End() {}
uint8_t Wand_RandoMode() { return wandRule; }
uint8_t Seasons_RandoMode() { return seasonsRule; }
void Seasons_UpdateGates() {}
int32_t CVarGetInteger(const char*, int32_t defaultValue) { return defaultValue; }
extern "C" unsigned char Picto_IsOwned() { assert(false); return 0; }
extern "C" unsigned char PowerKeg_IsOwned() { assert(false); return 0; }
'''
production = "\n".join(function(store, name) for name in ("Nei_Save", "Nei_GetOwnedItem", "Nei_SetOwnedItem"))
production += "\n" + store[store.index("#define NEI_CANE_SKILL_MAX"):store.index('extern "C" uint8_t Nei_CaneActiveSkill')]
production += "\n" + inventory[inventory.index("const uint8_t gPage2Items["):inventory.index("// Age requirements for page 2")]
production += "\n" + inventory[inventory.index("static const uint8_t sWandQuest"):inventory.index("static void* const sWandNameTex")]
production += "\n" + "\n".join(function(inventory, name) for name in (
    "TravelTools_Heal", "TravelTools_Grant", "GraceHourglass_Heal", "GraceHourglass_IsOwned", "GraceHourglass_Grant",
    "Wand_ModeOwned", "Wand_GrantMode", "Wand_SetModeOwned", "Wand_ModeCount",
    "Slate_RuneOwned", "Slate_GrantRune", "Seasons_HasRod", "Seasons_GrantRod",
    "Seasons_SeasonOwned", "Seasons_GrantSeason", "Seasons_SetSeason"))
production += '\nextern "C" ' + function(cane, "Cane_GiveSkill")
control = args.control or not helper.exists()
if control:
    editor = subprocess.check_output(["git", "show", "1bbb94e692629c0d910f97b57dd7cd02240d3775:soh/soh/Enhancements/debugger/debugSaveEditor.cpp"], cwd=ROOT).decode()
    start = editor.index('if (ImGui::Button("Give All Custom Items (Max)"))')
    end = editor.index('        ImGui::SameLine();', start)
    action = editor[start:end]
    production += '''
namespace ImGui { bool Button(const char*) { return true; } }
namespace OotNeiEditor {
uint16_t Page2Item(int index) { return gPage2Items[index]; }
void GrantAll() {
''' + action + "\n}\n}\n"
else:
    production += '\n#include "soh/Enhancements/debugger/NeiEditorItems.h"\n'

checks = r'''
static void reset() {
    gSaveContext = {};
    gNeiSave = {};
    for (auto& item : gNeiSave.ownedItems) item = ITEM_NONE;
    for (int i = 0; i < 24; ++i) gSaveContext.inventory.items[i] = i + 1;
    gSaveContext.inventory.items[SLOT_BOTTLE_1] = ITEM_FISH;
    gSaveContext.inventory.items[SLOT_BOTTLE_2] = ITEM_BLUE_FIRE;
    gSaveContext.inventory.ammo[SLOT_BOMB] = 13;
    gSaveContext.rupees = 73;
    gSaveContext.inventory.questItems = 0x1234;
    for (int slot = 48; slot < 72; ++slot) Nei_SetOwnedItem(slot, 0xC0 + slot - 48);
}
int main() {
    for (int i = 0; i < 32; ++i) gBitFlags[i] = 1u << i;
    assert(OotNeiEditor::Page2Item(15) == EXT_ITEM_SHEIKAH_SLATE);
    assert(OotNeiEditor::Page2Item(20) == EXT_ITEM_SHADOW_CRYSTAL);
    assert(OotNeiEditor::Page2Item(23) == EXT_ITEM_ROD_OF_SEASONS);
    puts("PASS editor catalog: full-width canonical items replace the retired cell entries");
    for (int wand = 0; wand < 3; ++wand) {
        for (uint8_t seasons : {NEI_SEASONS_ROD, NEI_SEASONS_INDIVIDUAL, NEI_SEASONS_GATED}) {
            wandRule = wand;
            seasonsRule = seasons;
            reset();
            gNeiSave.season = SEASON_OFF;
            gNeiSave.slateMode = SLATE_RUNE_STASIS;
            auto inventoryBefore = gSaveContext.inventory;
            const auto rupeesBefore = gSaveContext.rupees;
            OotNeiEditor::GrantAll();
            assert(Nei_GetOwnedItem(39) == EXT_ITEM_SHEIKAH_SLATE);
            assert(Nei_GetOwnedItem(44) == EXT_ITEM_SHADOW_CRYSTAL);
            assert(Nei_GetOwnedItem(47) == EXT_ITEM_ROD_OF_SEASONS);
            assert(Nei_GetOwnedItem(24) == ITEM_ROCS_CAPE);
            assert(gNeiSave.hyliasGraceOwned && gNeiSave.phantomHourglassOwned);
            assert(gNeiSave.slateRunesOwned == 0x1F && gNeiSave.caneSkills == 0x3F);
            assert(gNeiSave.wandRodsOwned == 0x3F && gNeiSave.seasonsOwned == 0x0F);
            for (uint8_t i = 0; i < 6; ++i) assert(Wand_ModeOwned(i));
            for (uint8_t i = 0; i < 4; ++i) assert(Seasons_SeasonOwned(i));
            assert(gNeiSave.season == SEASON_OFF && gNeiSave.slateMode == SLATE_RUNE_STASIS);
            assert(gNeiSave.shovelOwned && gNeiSave.dominionOwned && gNeiSave.bombArrowsOwned);
            assert(memcmp(inventoryBefore.items, gSaveContext.inventory.items, sizeof(inventoryBefore.items)) == 0);
            assert(memcmp(inventoryBefore.ammo, gSaveContext.inventory.ammo, sizeof(inventoryBefore.ammo)) == 0);
            assert(inventoryBefore.equipment == gSaveContext.inventory.equipment);
            assert(inventoryBefore.upgrades == gSaveContext.inventory.upgrades);
            assert(gSaveContext.rupees == rupeesBefore);
            for (int slot = 48; slot < 72; ++slot) assert(Nei_GetOwnedItem(slot) == 0xC0 + slot - 48);
            auto completed = gNeiSave;
            OotNeiEditor::GrantAll();
            assert(memcmp(&completed, &gNeiSave, sizeof(completed)) == 0);
            for (int slot = 24; slot < 48; ++slot) Nei_SetOwnedItem(slot, ITEM_NONE);
            OotNeiEditor::GrantAll();
            assert(Nei_GetOwnedItem(39) == EXT_ITEM_SHEIKAH_SLATE);
            assert(Nei_GetOwnedItem(41) == ITEM_HYLIAS_GRACE);
            assert(Nei_GetOwnedItem(44) == EXT_ITEM_SHADOW_CRYSTAL);
            assert(Nei_GetOwnedItem(47) == EXT_ITEM_ROD_OF_SEASONS);
            assert(gNeiSave.season == SEASON_OFF && gNeiSave.slateMode == SLATE_RUNE_STASIS);
        }
    }
    puts("PASS grants: all 9 wand/season rule combinations; complete powers; native items, bottles, masks and equipment preserved");
    puts("PASS regrant: repeat-safe and cleared host cells restored without changing active choices");
    reset();
    GraceHourglass_Grant(EXT_ITEM_PHANTOM_HOURGLASS);
    gNeiSave.slateMode = SLATE_RUNE_CRYONIS;
    gNeiSave.season = SEASON_SUMMER;
    gNeiSave.seasonsRodOwned = 1;
    gNeiSave.seasonsOwned = 0xF;
    gNeiSave.seasonsGates = 0xF;
    OotNeiEditor::GrantAll();
    assert(Nei_GetOwnedItem(41) == EXT_ITEM_PHANTOM_HOURGLASS);
    assert(gNeiSave.slateMode == SLATE_RUNE_CRYONIS && gNeiSave.season == SEASON_SUMMER);
    puts("PASS shared item selection: Hourglass, Slate rune and chosen season retained");
}
'''
with tempfile.TemporaryDirectory(prefix="oot-editor-") as directory:
    directory = Path(directory)
    source = directory / "editor.cpp"
    source.write_text(prefix + production + checks)
    binary = directory / "editor"
    includes = ("soh", "soh/include", "soh/include/PR", "soh/src", "soh/assets", "soh/soh", "libultraship/include", "combo", "combo/menu")
    command = [*shlex.split(os.environ.get("CXX", "c++")), "-std=c++20", "-DF3DEX_GBI_2", "-DCOMBO_BUILD",
               "-DCONTROLLERBUTTONS_T=uint32_t", "-O0", "-g", "-fsanitize=undefined,bounds", "-fno-sanitize-recover=all"]
    command += ["-I" + str(ROOT / path) for path in includes]
    result = subprocess.run([*command, str(source), "-o", str(binary)], capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    subprocess.run([str(binary)], check=True)
