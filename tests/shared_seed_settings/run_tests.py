#!/usr/bin/env python3
"""Run real shared menu construction and native MM information rule boundaries."""
import argparse
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--baseline", help="run prior production sources as a negative control")
parser.add_argument("--sanitize", action="store_true")
parser.add_argument("--case", choices=("menu", "native"))
args = parser.parse_args()


def read(path):
    if args.baseline:
        return subprocess.check_output(["git", "show", args.baseline + ":" + path], cwd=ROOT, text=True)
    return (ROOT / path).read_text()


def function(source, signature):
    return block_from(source, source.index(signature))


compiler = os.environ.get("CXX", "c++")
sanitizers = ["-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer",
              "-fno-pie", "-no-pie"] if args.sanitize else []
with tempfile.TemporaryDirectory(prefix="shared-seed-settings-") as temporary:
    build = Path(temporary)
    if args.case in (None, "menu"):
        menu = read("combo/gui/ComboMenu.cpp")
        helpers = menu[menu.index("struct HubEntry {"):menu.index("// Render one game's tracker sidebar inline")]
        source = (ROOT / "tests/shared_seed_settings/menu_test.cpp").read_text()
        source = source.replace("/* MENU_HELPERS */", helpers)
        source = source.replace("/* SHARED_PANEL */", function(menu, "void ComboMenu::DrawSharedPanel()"))
        cpp = build / "menu.cpp"
        cpp.write_text(source)
        binary = build / "menu"
        includes = ["-I" + str(ROOT / path) for path in
                    ("combo", "combo/menu", "libultraship/extern/imgui", "libultraship/include")]
        subprocess.run([compiler, "-std=c++20", "-Wall", "-Wextra", "-Werror", *sanitizers,
                        *includes, str(cpp), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
    if args.case in (None, "native"):
        includes = ["-DF3DEX_GBI_2", "-DCONTROLLERBUTTONS_T=uint32_t", "-DNON_EQUIVALENT", "-DNON_MATCHING"]
        includes += ["-I" + str(ROOT / path) for path in
                     ("mm", "mm/2s2h", "mm/include", "mm/include/PR", "mm/src", "mm/assets",
                      "libultraship/include", "combo", "combo/menu")]
        includes += ["-include", str(ROOT / "mm/include/z64.h")]
        receipt = read("mm/2s2h/Rando/ItemReceiptText.cpp")
        conversions = read("mm/2s2h/BenJsonConversions.hpp")
        start = conversions.index('    const auto savedOptions = j.at("randoSaveOptions")')
        end = conversions.index('    j.at("randoStartingItems")', start)
        source = (ROOT / "tests/shared_seed_settings/solo_map_test.cpp").read_text()
        source = source.replace("/* INFORMATION_GATE */", function(receipt, "bool Rando::MapCompassInfoEnabled()"))
        source = source.replace("/* NATIVE_HINT_MENU */", function(read("mm/2s2h/Rando/Menu.cpp"), "static void DrawHintsTab()"))
        source = source.replace("/* SAVE_OPTIONS_LOAD */", conversions[start:end])
        cpp = build / "native.cpp"
        cpp.write_text(source)
        for combo in (True, False):
            binary = build / ("native-combo" if combo else "native-solo")
            subprocess.run([compiler, "-std=c++20", *includes, "-I" + str(ROOT), *sanitizers,
                            *(["-DCOMBO_BUILD"] if combo else []), str(cpp),
                            str(ROOT / "mm/2s2h/Rando/StaticData/Options.cpp"), "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True)
