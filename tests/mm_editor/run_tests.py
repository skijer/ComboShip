#!/usr/bin/env python3
"""Exercise editor grants with production item data, grant arms, stores and FC recording."""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_mm_scene_randomization_tests import function
from run_mm_nei_tests import flags


def source(path, control=False):
    if control:
        return subprocess.check_output(["git", "show", "1c29c83f5d676d02ececa7302c138f7aa46b952a:" + path], cwd=ROOT,
                                       env={**os.environ, "GIT_NO_LAZY_FETCH": "1"}).decode("utf-8-sig")
    return (ROOT / path).read_text()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--control", action="store_true", help="Run the old editor path; expected to fail missing-slot assertion")
    args = parser.parse_args()
    items = source("mm/2s2h/Rando/StaticData/Items.cpp")
    macro = items[items.index("#define RI("):items.index("// clang-format off")]
    item_map = items[items.index("std::map<RandoItemId, RandoStaticItem> Items ="):
                     items.index("// clang-format on")]
    production = "namespace Rando::StaticData {\n" + macro + item_map + "\n}\n"
    item_data = production
    nei = source("mm/mods/nei_save.cpp")
    for name in ["Nei_Save", "Nei_GetOwnedItem", "Nei_SetOwnedItem", "Nei_InitNewSave"]:
        production += function(nei, name) + "\n"
    production += nei[nei.index("#define NEI_CANE_SKILL_MAX"):nei.index('extern "C" uint8_t Nei_CaneActiveSkill')]
    production += function(source("mm/mods/items/logic/item_cane_of_somaria.c"), "Cane_GiveSkill") + "\n"
    seasons = source("mm/2s2h/Rando/NeiSeasons.cpp").replace('extern "C" ', '')
    for name in ["Seasons_RandoMode", "Seasons_UpdateGates"]:
        production += function(seasons, name) + "\n"
    inventory = source("mm/mods/extended_inventory.c")
    production += inventory[inventory.index("static const uint8_t sWandQuest"):
                            inventory.index("static void* const sWandNameTex")]
    for name in ["TravelTools_Heal", "TravelTools_IsOwned", "TravelTools_Grant", "GraceHourglass_Heal", "GraceHourglass_IsOwned", "GraceHourglass_Grant", "Wand_RandoMode", "Wand_ModeOwned", "Wand_GrantMode", "Slate_RuneOwned", "Slate_GrantRune",
                 "Seasons_HasRod", "Seasons_GrantRod", "Seasons_SeasonOwned", "Seasons_GrantSeason", "Seasons_SetSeason"]:
        production += function(inventory, name) + "\n"
    equipment = source("mm/mods/extended_equipment.c")
    for name in ["ExtEquip_GetBit", "ExtEquip_HasItem", "ExtEquip_CapeOwned", "ExtEquip_GiveCape", "ExtEquip_GiveItem"]:
        production += function(equipment, name) + "\n"
    glue = source("mm/2s2h/FleetShipCombo/FleetComboItemsGlue.cpp")
    production += glue[glue.index("namespace {"):glue.index('extern "C" const char* FcCombo_NativeNameForItem')]

    give = function(source("mm/2s2h/Rando/GiveItem.cpp"), "Rando::GiveItem")
    switch_start = give.index("    switch (randoItemId) {\n        case RI_CLOCK_TOWN_STRAY_FAIRY:")
    labels = list(re.finditer(r"^        (?:case RI_[A-Z0-9_]+:|default:)", give[switch_start:], re.M))
    # Every selected arm is copied intact from the real switch. All other native/receipt
    # branches are excluded so an accidental ordinary-inventory grant fails immediately.
    selected = []
    for i, match in enumerate(labels):
        name_match = re.search(r"RI_[A-Z0-9_]+", match.group())
        if not name_match:
            continue
        name = name_match.group()
        if name.startswith(("RI_OOT_NEI_", "RI_OOT_EXT_", "RI_OOT_MEDALLION_")):
            end = labels[i + 1].start() if i + 1 < len(labels) else len(give) - switch_start
            selected.append(give[switch_start + match.start():switch_start + end])
    production += give[:switch_start] + "    switch (randoItemId) {\n" + "\n".join(selected)
    production += '\n        default: fprintf(stderr, "unexpected editor grant %d\\n", (int)randoItemId); abort();\n'
    production += "    }\n}\n"

    if not args.control:
        production += '#include "2s2h/DeveloperTools/NeiEditorItems.h"\n'
    else:
        # RED control: execute the editor's current Give All Custom Items block and
        # actual debug helper. Registry data are projected from production into the
        # real NeiItem struct; only .item/.slot are read by this ownership-only path.
        registry = source("mm/mods/extended_player.c", control=True)
        registry = registry[registry.index("static const NeiItem sNeiItems[]"):
                            registry.index("#define NEI_ITEMS_COUNT")]
        entries = re.findall(r"\{\s*(ITEM_[A-Z0-9_]+).*?\b(SLOT_[A-Z0-9_]+|NEI_NO_SLOT),\s*AGE_REQ", registry, re.S)
        production += "static const NeiItem sNeiItems[] = {\n"
        production += "\n".join("{.item=" + item + ", .slot=" + slot + "}," for item, slot in entries)
        production += "\n};\n#define NEI_ITEMS_COUNT (sizeof(sNeiItems)/sizeof(sNeiItems[0]))\n"
        production += function(source("mm/mods/extended_player.c", control=True), "Nei_FindByItem") + "\n"
        production += function(inventory, "ExtInv_GetItemSlot") + "\n"
        production += function(source("mm/mods/extended_player.c", control=True), "ExtInv_DebugGiveAll") + "\n"
        editor = source("mm/2s2h/DeveloperTools/SaveEditor.cpp", control=True)
        start = editor.index('    if (ImGui::Button("Give All Custom Items"))')
        end = editor.index("    // NOTE: visual slots", start)
        production += 'namespace ImGui { bool Button(const char* label) { return strcmp(label, "Give All Custom Items") == 0; } void SameLine() {} }\n'
        production += "namespace NeiEditor { void GrantAll() {\n" + editor[start:end] + "\n} }\n"

    with tempfile.TemporaryDirectory(prefix="mm-editor-") as temporary:
        build = Path(temporary)
        (build / "editor_production.inc").write_text(production)
        binary = build / "editor_grant_test"
        cxx = shlex.split(os.environ.get("CXX", "c++"))
        subprocess.run([*cxx, "-std=c++20", "-fpermissive", "-O0", "-g",
                        "-fsanitize=address,undefined,bounds", "-fno-sanitize-recover=all",
                        *flags(), "-DCOMBO_BUILD", "-DMM_BUILD_DLL", "-DCONTROLLERBUTTONS_T=uint32_t",
                        *(["-DMM_EDITOR_CONTROL"] if args.control else []),
                        "-I" + str(build), "-include", "nlohmann/json.hpp",
                        "-Wall", "-Wextra", "-Wno-unused-parameter", "-Wno-missing-field-initializers",
                        str(ROOT / "tests/mm_editor/grant_test.cpp"), "-o", str(binary)], check=True)
        environment = {**os.environ, "ASAN_OPTIONS": os.environ.get("ASAN_OPTIONS", "") + ":detect_leaks=0"}
        subprocess.run([str(binary)], check=True, env=environment)
        bindings = source("mm/2s2h/Rando/NeiGiPresentation.cpp")
        pool_binding = item_data + "\nnamespace Rando::Logic {\n"
        constraints = source("mm/2s2h/Rando/Logic/PlacementConstraints.cpp")
        pool_binding += "\n".join(function(constraints, name) for name in
                                  ("DungeonItemToDungeon", "DungeonItemPlacementOption", "StaysAtVanillaCheck",
                                   "IsSongLocationItem")) + "\n"
        pool_binding += function(source("mm/2s2h/Rando/Logic/GeneratePools.cpp"), "GeneratePools") + "\n}\n"
        pool_binding += bindings[bindings.index("struct Binding {"):bindings.index("// Only roots that differ")]
        pool_binding += function(bindings, "HasMmLegacyGiMod") + "\n"
        pool_binding += function(bindings, "GetSelectedOwnerGi") + "\n"
        pool_binding += function(bindings, "MM_DescribeNeiGi") + "\n"
        (build / "pool_binding_production.inc").write_text(pool_binding)
        pool_binary = build / "pool_binding_test"
        # Distro spdlog uses external fmt; this standalone fixture links no logging library.
        subprocess.run([*cxx, "-std=c++20", "-fpermissive", "-O0", "-g",
                        "-DFMT_HEADER_ONLY",
                        "-fsanitize=address,undefined,bounds", "-fno-sanitize-recover=all",
                        *flags(), "-DCOMBO_BUILD", "-DMM_BUILD_DLL", "-DCONTROLLERBUTTONS_T=uint32_t",
                        "-I" + str(build), "-include", "nlohmann/json.hpp", "-include", "ship/Context.h",
                        "-include", "spdlog/spdlog.h", str(ROOT / "tests/mm_editor/pool_binding_test.cpp"),
                        "-Wl,--export-dynamic-symbol=OOT_GetNeiGiDrawInfo", "-ldl", "-o", str(pool_binary)], check=True)
        subprocess.run([str(pool_binary)], check=True, env=environment)
        native_flags = ["-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DMM_BUILD_DLL", "-DCONTROLLERBUTTONS_T=uint32_t",
                        "-DNON_EQUIVALENT", "-DNON_MATCHING"]
        native_flags += ["-I" + str(ROOT / p) for p in
                         ("mm/include", "mm/include/PR", "mm/src", "mm", "mm/2s2h", "mm/assets",
                          "libultraship/include", "libultraship/src", "combo", "combo/menu")]
        for path in ["mm/2s2h/DeveloperTools/SaveEditor.cpp", "mm/2s2h/Rando/GiveItem.cpp"]:
            subprocess.run([*cxx, "-std=c++20", "-fpermissive", *native_flags,
                            "-include", "nlohmann/json.hpp", "-include", str(ROOT / "mm/include/global.h"),
                            "-include", "spdlog/spdlog.h", "-include", "ship/Context.h", "-fsyntax-only",
                            str(ROOT / path)], check=True)
            print("PASS real-header syntax: " + path)


if __name__ == "__main__":
    main()
