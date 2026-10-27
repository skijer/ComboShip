#!/usr/bin/env python3
"""Run the real MM boot/arrival bodies against native save and respawn types."""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
MODE_PREAMBLE = '#include "ComboGameMode.h"\nextern "C" { int gComboGameMode = COMBO_GAME_BOTH; }\n'
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from

subprocess.run([sys.executable, '-B', str(ROOT / 'tests/lost_woods_entrance/run_vertical_scroll_tests.py')], check=True)
subprocess.run([sys.executable, '-B', str(ROOT / 'tests/lost_woods_entrance/run_woodfall_tests.py')], check=True)
subprocess.run([sys.executable, '-B', str(ROOT / 'tests/lost_woods_entrance/run_woodfall_draw_tests.py')], check=True)

title = (ROOT / "mm/src/code/title_setup.c").read_text()
port = (ROOT / "mm/2s2h/BenPort.cpp").read_text()
body = block_from(title, title.index("void Setup_InitImpl("))
body = re.sub(r"\bthis\b", "self", body)
signature = 'extern "C" void MM_ApplyComboPortalArrival('
arrival = block_from(port, port.index(signature))
routes = "\n".join(block_from(port, port.index(signature)) for signature in (
    "static void Combo_CancelLostWoodsPassage(",
    "static void Combo_ClearReturnRequests(",
    'extern "C" COMBO_EXPORT void MM_SetComboEntryIsResume(',
    'extern "C" COMBO_EXPORT void MM_SetComboPortalRoute('))

with tempfile.TemporaryDirectory(prefix="lost-woods-arrival-") as directory:
    source = Path(directory) / "arrival.cpp"
    source.write_text(MODE_PREAMBLE + (ROOT / "tests/lost_woods_entrance/mm_arrival_test.cpp").read_text()
                      .replace("/* ARRIVAL_BODY */", arrival)
                      .replace("/* ROUTE_BODY */", routes)
                      .replace("/* BOOT_BODY */", body))
    binary = Path(directory) / "arrival"
    result = subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-w", "-fmax-errors=5", "-DF3DEX_GBI_2", "-DCOMBO_BUILD",
                    "-DFMT_HEADER_ONLY", "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections",
                    *["-I" + str(ROOT / path) for path in
                      ("mm", "mm/2s2h", "mm/include", "mm/include/PR", "mm/src", "mm/assets",
                       "libultraship/include", "combo")], str(source), "-o", str(binary)], capture_output=True, text=True)
    if result.returncode:
        sys.exit(result.stderr[:6000])
    subprocess.run([str(binary)], check=True)

    begin = port.index("    // Reverse MM->OOT trigger:")
    end = port.index("    } // Combo-only handoff hooks:", begin)
    state_begin = port.index("static bool sComboReturnPending = false;")
    state_end = port.index("// MM's own ResourceManager", state_begin)
    source.write_text(MODE_PREAMBLE + (ROOT / "tests/lost_woods_entrance/mm_handoff_test.cpp").read_text()
                      .replace("/* RETURN_STATE */", port[state_begin:state_end])
                      .replace("/* HANDOFF_HOOKS */", port[begin:end]))
    result = subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-w", "-fmax-errors=5",
                    "-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DFMT_HEADER_ONLY",
                    "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections",
                    *["-I" + str(ROOT / path) for path in
                      ("mm", "mm/2s2h", "mm/include", "mm/include/PR", "mm/src", "mm/assets",
                       "libultraship/include", "combo")], str(source), "-o", str(binary)],
                    capture_output=True, text=True)
    if result.returncode:
        sys.exit(result.stderr[:6000])
    subprocess.run([str(binary)], check=True)

    oot = (ROOT / "soh/soh/OTRGlobals.cpp").read_text(encoding="utf-8-sig")
    oot_title = (ROOT / "soh/src/code/title_setup.c").read_text()
    source.write_text(MODE_PREAMBLE + (ROOT / "tests/lost_woods_entrance/oot_arrival_test.cpp").read_text()
                      .replace("/* ARRIVAL_BODY */", block_from(oot, oot.index(
                          'extern "C" void SOH_ApplyComboPortalArrival(')))
                      .replace("/* BOOT_BODY */", block_from(oot_title, oot_title.index(
                          "void TitleSetup_InitImpl("))))
    result = subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-w", "-fmax-errors=5",
                    "-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DFMT_HEADER_ONLY",
                    "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections",
                    *["-I" + str(ROOT / path) for path in
                      ("soh", "soh/soh", "soh/include", "soh/src", "soh/assets",
                       "libultraship/include", "combo")], str(source), "-o", str(binary)],
                    capture_output=True, text=True)
    if result.returncode:
        sys.exit(result.stderr[:6000])
    subprocess.run([str(binary)], check=True)
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-Wall", "-Wextra", "-Werror",
                    "-I" + str(ROOT / "combo"), str(ROOT / "tests/lost_woods_entrance/gates_test.cpp"),
                    "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)

    begin = oot.index("    // Flag set when we want to switch to MM.")
    end = oot.index("    // ComboShip release gate:", begin)
    source.write_text(MODE_PREAMBLE + (ROOT / "tests/lost_woods_entrance/oot_handoff_test.cpp").read_text()
                      .replace("/* MIRROR_QUEUE */", re.search(
                          r"static int sComboMirrorSwitchSlot = [^;]+;", oot)[0] + "\n" + block_from(
                          oot, oot.index('extern "C" int SOH_QueueMagicMirrorHandoff(')))
                      .replace("/* HANDOFF_HOOKS */", oot[begin:end]))
    result = subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-w", "-fmax-errors=5",
                    "-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DFMT_HEADER_ONLY",
                    "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections",
                    *["-I" + str(ROOT / path) for path in
                      ("soh", "soh/soh", "soh/include", "soh/src", "soh/assets",
                       "libultraship/include", "combo")], str(source), "-o", str(binary)],
                    capture_output=True, text=True)
    if result.returncode:
        sys.exit(result.stderr[:6000])
    subprocess.run([str(binary)], check=True)

subprocess.run([sys.executable, "-B", str(ROOT / "tests/lost_woods_entrance/run_arrival_fade_tests.py")], check=True)
subprocess.run([sys.executable, "-B", str(ROOT / "tests/lost_woods_entrance/run_fountain_return_tests.py"), "--sanitize"], check=True)
subprocess.run([sys.executable, str(ROOT / "tests/lost_woods_entrance/renderer_linkage_test.py")], check=True)
subprocess.run([sys.executable, "-B", str(ROOT / "tests/lost_woods_entrance/run_mm_camera_arrival_test.py")], check=True)
subprocess.run([sys.executable, "-B", str(ROOT / "tests/lost_woods_entrance/opening_forest_test.py")], check=True)
subprocess.run([sys.executable, "-B", str(ROOT / "tests/lost_woods_entrance/portal_save_test.py")], check=True)
subprocess.run([sys.executable, "-B", str(ROOT / "tests/lost_woods_entrance/mm3d_pack_test.py")], check=True)
subprocess.run([sys.executable, "-B", str(ROOT / "tests/lost_woods_entrance/run_title_card_tests.py")], check=True)
subprocess.run([sys.executable, "-B", str(ROOT / "tests/lost_woods_entrance/run_oot_title_card_tests.py"), "--sanitize"], check=True)
subprocess.run([sys.executable, "-B", str(ROOT / "tests/lost_woods_entrance/run_mm_title_card_tests.py"), "--sanitize"], check=True)
subprocess.run([sys.executable, "-B", str(ROOT / "tests/lost_woods_entrance/stump_return_test.py"), "--sanitize"], check=True)
subprocess.run([sys.executable, "-B", str(ROOT / "tests/lost_woods_entrance/stump_tool_safety_test.py")], check=True)
subprocess.run([sys.executable, "-B", str(ROOT / "tests/lost_woods_entrance/run_portal_camera_tests.py")], check=True)
subprocess.run([sys.executable, "-B", str(ROOT / "tests/lost_woods_entrance/run_portal_finish_test.py")], check=True)
subprocess.run([sys.executable, "-B", str(ROOT / "tests/lost_woods_entrance/visual_tool_safety_test.py")], check=True)
