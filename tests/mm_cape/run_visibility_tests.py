#!/usr/bin/env python3
"""Run the complete OoT/MM cape draw against native player-hide flags."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
sys.path.insert(0, str(ROOT / "tests/mm_presentation"))
from run_time_pedestal_tests import functions
from run_mm_nei_tests import flags as mm_flags


def oot_flags():
    return ["-DF3DEX_GBI_2", "-DCONTROLLERBUTTONS_T=uint32_t", "-DLOG_LEVEL_GAME_PRINTS=0",
            *["-I" + str(ROOT / path) for path in (
                "soh", "soh/include", "soh/src", "soh/assets", "libultraship/include",
                "combo", "combo/menu")]]


def main():
    failed = False
    with tempfile.TemporaryDirectory(prefix="cape-native-visibility-") as directory:
        work = Path(directory)
        for game in ("soh", "mm"):
            graph = functions((ROOT / game / "src/code/graph.c").read_text())
            helpers = graph["Graph_OpenDisps"] + "\n" + graph["Graph_CloseDisps"]
            if game == "mm":
                helpers += "\n" + functions((ROOT / "mm/src/code/stubs.c").read_text())["gSPVertex"]
            (work / "cape_graph.inc").write_text(helpers)
            binary = work / game
            flags = mm_flags() + ["-DCAPE_TEST_MM"] if game == "mm" else oot_flags()
            compiled = subprocess.run([
                os.environ.get("CC", "cc"), "-std=gnu2x", *flags, "-DCOMBO_BUILD", "-I" + str(work),
                "-w", "-O1", "-g", "-ffunction-sections", "-fdata-sections",
                "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer",
                "-fno-pie", "-no-pie", str(ROOT / "tests/mm_cape/cape_visibility_test.c"),
                "-Wl,--gc-sections", "-lm", "-o", str(binary)
            ], cwd=ROOT, capture_output=True, text=True)
            if compiled.returncode:
                raise RuntimeError(compiled.stdout + compiled.stderr)
            for mode in ("draw", "resume"):
                result = subprocess.run([str(binary), mode], cwd=ROOT,
                                        env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
                print(("PASS " if result.returncode == 0 else "FAIL ") + game + " cape " + mode,
                      flush=True)
                failed |= result.returncode != 0
    return int(failed)


if __name__ == "__main__":
    raise SystemExit(main())
