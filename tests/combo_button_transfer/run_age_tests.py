#!/usr/bin/env python3
"""Execute native OoT age swaps and button persistence, without booting the game.

Uses full native save types, verbatim accessors and age-swap functions, actual
SaveManager traversal/templates and its three button save/load blocks. HUD
graphics commands are inert; the production C/D icon/ammo gates still execute.
"""
import argparse
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
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-ref", help="revision supplying production implementations")
    parser.add_argument("--json-include", type=Path, default=Path("/usr/include"))
    args = parser.parse_args()

    def read(path):
        if args.source_ref:
            return subprocess.check_output(["git", "show", f"{args.source_ref}:{path}"], cwd=ROOT, text=True)
        return (ROOT / path).read_text()

    parameter = read("soh/src/code/z_parameter.c")
    age = read("soh/soh/Enhancements/SwitchAge.cpp")
    buttons = read("soh/mods/ext_buttons/ext_buttons.cpp")
    manager = read("soh/soh/SaveManager.cpp")
    kaleido = read("soh/src/overlays/misc/ovl_kaleido_scope/z_kaleido_item.c")
    production = buttons[buttons.index('extern "C" {'):]
    production += "\n" + "\n".join(function(parameter, name) for name in (
        "Rando_Inventory_SwapAgeEquipment", "Inventory_SwapAgeEquipment"))
    production += "\n" + function(age, "SwitchAge_HandleNonLogicEquipmentSwap")
    production += "\n" + function(read("soh/src/overlays/actors/ovl_player_actor/z_player.c"), "Player_GetItemOnButton")
    production += "\n" + "\n".join(function(manager, "SaveManager::" + name) for name in (
        "SaveArray", "LoadArray", "SaveStruct", "LoadStruct"))
    for method, start_method, action in (("SaveButtons", "SaveBase", "Save"),
                                       ("LoadButtons", "LoadBaseVersion4", "Load")):
        body = function(manager, "SaveManager::" + start_method)
        first = body.index(f'    SaveManager::Instance->{action}Struct("childEquips"')
        last = body.index(f'    SaveManager::Instance->{action}Struct("inventory"', first)
        signature = "const SaveContext& saveContext" if action == "Save" else ""
        production += f"\nstatic void {method}({signature}) {{\n" + body[first:last] + "\n}\n"
    production += "\nstatic void ResetButtonStorage() {\n"
    init = function(manager, "SaveManager::InitFileNormal")
    if "ExtButton_ResetSaveData();" in init:
        production += "    ExtButton_ResetSaveData();\n"
    else:
        start = init.index("    for (int button = 0; button < ARRAY_COUNT(gSaveContext.ship.extButtons.items)")
        end = init.index("    // Inventory", start)
        production += init[start:end]
    production += "}\n"
    # Execute the native equip completion branch that writes the item together
    # with its inventory slot; animation/input handling is outside this probe.
    equip = function(kaleido, "KaleidoScope_UpdateItemEquip")
    start = equip.index("            // If the item is on another button already, swap the two")
    end = equip.index("            Interface_LoadItemIcon1(play, targetButtonIndex);", start)
    production += "\nstatic void CompleteNativeButtonEquip(PlayState* play) {\n"
    production += "    PauseContext* pauseCtx = &play->pauseCtx;\n"
    production += equip[start:end] + "}\n"
    for index, label in enumerate(("C-Left", "C-Down", "C-Right", "DPad-Up", "DPad-Down", "DPad-Left", "DPad-Right"), 1):
        start = parameter.index(f"// {label} Button Icon & Ammo Count")
        match = re.search(r"\bif\s*\(", parameter[start:])
        gate = block_from(parameter, start + match.start())
        production += f"\nstatic void DrawButton{index}(PlayState* play) {{\n" + gate + "\n}\n"

    fixture = (ROOT / "tests/combo_button_transfer/age_shadow_test.cpp").read_text()
    with tempfile.TemporaryDirectory(prefix="combo-age-buttons-") as temp:
        temp = Path(temp)
        (temp / "button_age_production.inc").write_text(production)
        (temp / "BS_thread_pool.hpp").write_text("#pragma once\nnamespace BS { class thread_pool; }\n")
        includes = ["-I" + str(path) for path in (temp, args.json_include, ROOT, *(ROOT / path for path in (
            "soh", "soh/include", "soh/include/PR", "soh/src", "soh/assets", "libultraship/include", "combo")))]
        flags = ["-std=c++20", "-w", "-DF3DEX_GBI_2", "-DCONTROLLERBUTTONS_T=uint32_t",
                 '-DCVAR_PREFIX_ENHANCEMENT="gEnhancements"',
                 "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-g", "-no-pie"]
        failures = 0
        for combined in (False, True):
            binary = temp / ("combo" if combined else "standalone")
            subprocess.run([os.environ.get("CXX", "c++"), *flags, *(["-DCOMBO_BUILD"] if combined else []),
                            *includes, "-fsyntax-only", str(ROOT / "soh/mods/ext_buttons/ext_buttons.cpp")], check=True)
            subprocess.run([os.environ.get("CXX", "c++"), *flags, *(["-DCOMBO_BUILD"] if combined else []),
                            *includes, str(ROOT / "tests/combo_button_transfer/age_shadow_test.cpp"),
                            "-o", str(binary)], check=True)
            for mode in ("native", "nonlogic", "storage", "empty-hud"):
                result = subprocess.run([str(binary), mode], env={**os.environ,
                    "ASAN_OPTIONS": "detect_leaks=0", "UBSAN_OPTIONS": "halt_on_error=1"})
                failures += bool(result.returncode)
                print(binary.name, mode, "FAIL" if result.returncode else "PASS", flush=True)
        return int(bool(failures))


if __name__ == "__main__":
    raise SystemExit(main())
