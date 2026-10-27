#!/usr/bin/env python3
"""Check the native cutscene-actor dialogue catalog and talk lifecycle.

The catalog/controller run under ASan/UBSan. The full viewer translation unit
uses its existing CMake fixture's section-GC boundary, without sanitizer global
registration retaining unrelated renderer/actor tables. No game is executed.
"""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
VIEWER = "soh/src/overlays/actors/ovl_En_Viewer/"


def run(*args):
    subprocess.run([str(arg) for arg in args], cwd=ROOT, check=True,
                   env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})


with tempfile.TemporaryDirectory(prefix="cutscene-dialogue-") as directory:
    work = Path(directory)
    compiler = os.environ.get("CC", "cc")
    sources = [VIEWER + "static_story_" + name + ".c" for name in ("actor", "dialogue", "talk")]
    for name, units in (("dialogue", [sources[1]]), ("talk", sources)):
        binary = work / name
        run(compiler, "-std=c11", "-g", "-O1", "-Isoh/include",
            "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-pie", "-no-pie",
            "soh/tests/static_story_" + name + "_test.c", *units, "-lm", "-o", binary)
        run(binary)
        print("PASS cutscene-actor " + name + " catalog/controller sanitizer checks", flush=True)
    for combo in (False, True):
        binary = work / ("viewer-combo" if combo else "viewer-solo")
        run(compiler, "-std=gnu11", "-g", "-O1", "-DF3DEX_GBI_2", "-DCONTROLLERBUTTONS_T=uint32_t",
            "-DLOG_LEVEL_GAME_PRINTS=0", "-ffunction-sections", "-fdata-sections",
            *(["-DCOMBO_BUILD"] if combo else []),
            *["-I" + path for path in ("soh/include", "soh/src", "soh/assets", "soh",
                                      "libultraship/include", "combo", "combo/menu")],
            "soh/tests/static_story_viewer_talk_test.c", VIEWER + "z_en_viewer.c", *sources,
            "-Wl,--gc-sections", "-lm", "-o", binary)
        run(binary)
        print("PASS native viewer talk in " + ("Combo" if combo else "solo SoH"), flush=True)

run(sys.executable, "-B", "tests/custom_messages/run_story_open_tests.py")
