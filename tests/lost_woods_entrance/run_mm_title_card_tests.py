#!/usr/bin/env python3
"""Run MM's real composed boot, native entrance policy and native scene-title setup.

Disk loading, entrance cutscene dispatch and message decoding are boundaries.
Destination title IDs, native message mode/layout and request lifetime execute.
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
from run_time_pedestal_tests import block_from


def function(source, signature):
    return block_from(source, source.index(signature))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    boot = (ROOT / "mm/src/code/title_setup.c").read_text()
    port = (ROOT / "mm/2s2h/BenPort.cpp").read_text()
    demo = (ROOT / "mm/src/code/z_demo.c").read_text()
    message = (ROOT / "mm/src/code/z_message.c").read_text()
    table = (ROOT / "mm/src/code/z_scene_table.c").read_text()
    fixture = (ROOT / "tests/lost_woods_entrance/mm_title_card_test.cpp").read_text()
    state = ""
    if "int Combo_MM_ConsumeCrossGameTitleCard(" in boot:
        state = re.search(r"static s32 sComboCrossGameTitleCardPending[^;]*;", boot)[0]
        state += "\n" + function(boot, "int Combo_MM_ConsumeCrossGameTitleCard(")
        if "void Combo_MM_RequestCrossGameTitleCard(" in boot:
            state += "\n" + function(boot, "void Combo_MM_RequestCrossGameTitleCard(")
    fixture = fixture.replace("/* REQUEST_STATE */", state)
    regional = (ROOT / "mm/2s2h/Enhancements/Gameplay/ComboRegionalPassages.cpp").read_text()
    arrival_start = regional.index("struct Arrival {")
    arrival_end = regional.index("} sArrival;", arrival_start) + len("} sArrival;")
    arrival_state = "namespace {\nusing ComboMmRegional::Placement;\n" + regional[arrival_start:arrival_end] + "\n}\n"
    fixture = fixture.replace("/* REGIONAL_ARRIVAL */", arrival_state + function(regional, 'extern "C" int ComboRegionalPassagesMm_ApplyArrival('))
    fixture = fixture.replace("/* BOOT_BODY */", re.sub(r"\bthis\b", "self", function(boot, "void Setup_InitImpl(")))
    fixture = fixture.replace("/* ARRIVAL_BODY */", function(port, 'extern "C" void MM_ApplyComboPortalArrival('))
    fixture = fixture.replace("/* CUSTOM_TRANSITIONS */", "\n".join(function((ROOT / path).read_text(), signature)
        for path, signature in (("mm/2s2h/Enhancements/Gameplay/BlackMarket.cpp", "void Transition("),
                                ("mm/2s2h/Enhancements/Gameplay/ComboSoloForest.cpp", "void ForestTransition("))))
    fixture = fixture.replace("/* ENTRANCE_POLICY */", function(demo, "void Cutscene_HandleEntranceTriggers("))
    fixture = fixture.replace("/* NATIVE_MESSAGE_TITLE */", function(message, "void Message_DisplaySceneTitleCard("))
    scene_table = table[table.index("// 2S2H Added columns"):table.index("static EntranceTableEntry")]
    fixture = fixture.replace("/* SCENE_TABLE */", scene_table)
    entries = "\n".join(re.search(r"static EntranceTableEntry " + name + r"\[\] = \{.*?\n\};", table, re.S)[0]
                        for name in ("sWoodfallEntrance0", "sWoodsOfMysteryEntrance0", "sSouthClockTownEntrance0"))
    fixture = fixture.replace("/* ENTRANCE_TABLE */", entries)
    paths = ("mm", "mm/2s2h", "mm/include", "mm/include/PR", "mm/src", "mm/assets", "libultraship/include", "combo")
    flags = ["-std=c++20", "-w", "-DF3DEX_GBI_2", "-DFMT_HEADER_ONLY",
             *["-I" + str(ROOT / path) for path in paths]]
    if args.sanitize:
        flags += ["-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-no-pie"]
    with tempfile.TemporaryDirectory(prefix="mm-cross-game-title-") as temporary:
        source = Path(temporary) / "test.cpp"
        source.write_text(fixture)
        for combined in (True, False):
            binary = Path(temporary) / ("combo" if combined else "solo")
            subprocess.run([os.environ.get("CXX", "c++"), *flags,
                            *(["-DCOMBO_BUILD"] if combined else []), str(source), "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})


if __name__ == "__main__":
    main()
