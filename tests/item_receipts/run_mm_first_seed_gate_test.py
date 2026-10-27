#!/usr/bin/env python3
"""Execute both complete combo mutation panels against the native MM-first park.

This isolates actual caller gating, not the fill algorithm. ImGui rendering and
the worker callback are observers; native GameState and the complete park/query
and menu control bodies are unchanged. --baseline executes a previous native
menu revision as a behavioral negative control.
"""
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
parser.add_argument("--baseline", help="git revision for the old ComboMenu negative control")
parser.add_argument("--sanitizers", action="store_true")
args = parser.parse_args()
otr = (ROOT / "soh/soh/OTRGlobals.cpp").read_text()
menu = subprocess.check_output(["git", "show", args.baseline + ":combo/gui/ComboMenu.cpp"], cwd=ROOT, text=True) \
    if args.baseline else (ROOT / "combo/gui/ComboMenu.cpp").read_text()
source = (ROOT / "tests/item_receipts/mm_first_seed_gate_test.cpp").read_text()
# Keep the panel's actual release-highlight helper. These additional ImGui
# observers cover presentation only; the fixture's disabled/button behavior
# and the complete production generation/reload gates remain unchanged.
source = r'''
#include "gui/WhatsNew.h"
ImGuiStyle::ImGuiStyle() = default;
namespace ImGui {
ImGuiStyle& GetStyle() { static ImGuiStyle style; return style; }
float GetFrameHeight() { return 20.0f; }
float GetTextLineHeight() { return 16.0f; }
ImVec2 CalcTextSize(const char*, const char*, bool, float) { return {}; }
ImVec2 GetContentRegionAvail() { return { 1000.0f, 1000.0f }; }
bool IsItemHovered(ImGuiHoveredFlags) { return false; }
void SetTooltip(const char*, ...) {}
}
''' + source
functions = "\n".join(block_from(otr, otr.index(signature)) for signature in
                       ('extern "C" COMBO_EXPORT uint8_t SOH_IsOnFileSelect',
                        'extern "C" COMBO_EXPORT void SOH_ParkForComboMMResume'))
source = source.replace("/* OOT_PARK_AND_QUERY */", functions)
state_start = menu.index("struct PlandoPickItem {")
state_end = menu.index("static PlandoState sPlando;", state_start) + len("static PlandoState sPlando;")
source = source.replace("/* ACTUAL_PLANDO_STATE */", menu[state_start:state_end])
source = source.replace("/* ACTUAL_FILE_SELECT_GATE */",
                        block_from(menu, menu.index("bool ComboSeedFileSelectActive()"))
                        if "bool ComboSeedFileSelectActive()" in menu else "")
source = source.replace("/* ACTUAL_PLANDO_PANEL */",
                        block_from(menu, menu.index("static std::string NormalizeSearch")) + "\n" +
                        block_from(menu, menu.index("void DrawComboPlandoPanel()")))
source = source.replace("/* ACTUAL_COMBO_PANEL */",
                        block_from(menu, menu.index("void ComboMenu::DrawComboPanel()")))
with tempfile.TemporaryDirectory(prefix="mm-first-seed-gate-") as directory:
    build = Path(directory)
    cpp = build / "gate.cpp"
    cpp.write_text(source)
    binary = build / "gate"
    includes = ["-I" + str(p) for p in (ROOT, ROOT / "soh", ROOT / "soh/include",
                ROOT / "soh/assets", ROOT / "libultraship/include", ROOT / "combo", ROOT / "combo/menu")]
    sanitizer_flags = ["-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer"] \
        if args.sanitizers else []
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-DF3DEX_GBI_2", "-DCOMBO_BUILD",
                    *sanitizer_flags,
                    *includes, str(cpp), "-o", str(binary)], cwd=ROOT, check=True)
    sys.exit(subprocess.run([str(binary)], cwd=ROOT).returncode)
