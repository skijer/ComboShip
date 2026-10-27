#!/usr/bin/env python3
"""Build the real market collision index, not a scene-entry controller double.

The immutable bundled collision is decoded into native engine types. Production
BgCheck allocation/subdivision/list construction and triangle math run under
ASan/UBSan. Arena allocations are isolated malloc blocks so an overflowing node
pool fails at its first write rather than corrupting the adjacent lookup table.
"""
from pathlib import Path
import argparse
import os
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/black_market"))
import native_market


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-ref", help="Replay a prior MM collision implementation for RED verification")
    args = parser.parse_args()
    resources = native_market.read_bundle(ROOT / "tools/native_market/bundled")
    with tempfile.TemporaryDirectory(prefix="market-collision-") as directory:
        work = Path(directory)
        collision = work / "collision.bin"
        collision.write_bytes(resources[native_market.COLLISION])
        includes = ("mm", "mm/2s2h", "mm/include", "mm/include/PR", "mm/src", "mm/assets",
                    "libultraship/include", "combo")
        flags = ["-std=gnu17", "-w", "-DF3DEX_GBI_2", "-g", "-O1",
                 "-ffunction-sections", "-fdata-sections", "-fsanitize=address,undefined",
                 # Native cube-face flag macros shift signed masks into bit 31.
                 "-fno-sanitize=shift",
                 "-fno-sanitize-recover=all", "-fno-omit-frame-pointer", "-include", "global.h",
                 *["-I" + str(ROOT / path) for path in includes]]
        bgcheck = ROOT / "mm/src/code/z_bgcheck.c"
        if args.source_ref:
            bgcheck = work / "z_bgcheck.c"
            bgcheck.write_bytes(subprocess.check_output(
                ["git", "show", args.source_ref + ":mm/src/code/z_bgcheck.c"], cwd=ROOT))
        for mode in ("normal", "combo", "combo-arena"):
            mode_flags = ([] if mode == "normal" else ["-DCOMBO_BUILD"])
            if mode == "combo-arena":
                mode_flags.append("-DMARKET_REAL_ARENA")
            sources = [bgcheck, ROOT / "mm/src/code/sys_math3d.c", ROOT / "mm/src/code/z_lib.c",
                       ROOT / "tests/black_market/collision_pool_test.c"]
            if mode == "combo-arena":
                sources.append(ROOT / "mm/src/code/TwoHeadArena.c")
            objects = []
            for index, path in enumerate(sources):
                obj = work / f"{mode}-{index}.o"
                subprocess.run([os.environ.get("CC", "cc"), *flags, *mode_flags,
                                "-c", str(path), "-o", str(obj)], check=True)
                objects.append(str(obj))
            binary = work / mode
            subprocess.run([os.environ.get("CC", "cc"), *objects, "-Wl,--gc-sections", "-fsanitize=address,undefined",
                            "-no-pie", "-lm", "-o", str(binary)], check=True)
            subprocess.run([str(binary), str(collision)], check=True,
                           env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})


if __name__ == "__main__":
    main()
