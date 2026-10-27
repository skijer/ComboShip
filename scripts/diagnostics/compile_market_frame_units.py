#!/usr/bin/env python3
"""Bounded native object checks for market capture; never links or runs a game.

Pass restored dependency include roots through --include or CPATH. Compiler
commands, source/header hashes and output are saved per unit for review.
"""
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import argparse
import hashlib
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
UNITS = ["soh/soh/Enhancements/randomizer/NeiGiPresentation.cpp", "mm/2s2h/Rando/NeiGiPresentation.cpp",
         "soh/soh/Enhancements/randomizer/draw.cpp", "mm/2s2h/Rando/DrawItem.cpp",
         "soh/src/code/z_draw.c", "mm/src/code/z_draw.c", "soh/soh/ResourceManagerHelpers.cpp",
         "mm/2s2h/BenPort.cpp", "mm/mods/transformation_masks/assets/mm_asset_loader.cpp"]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--unit", action="append", choices=UNITS)
parser.add_argument("--include", action="append", type=Path, default=[])
parser.add_argument("--out", type=Path, default=ROOT.parent / "lock-visual-verification")
parser.add_argument("--jobs", type=int, choices=(1, 2, 3), default=2)
args = parser.parse_args()
args.out.mkdir(parents=True, exist_ok=True)

def compile_unit(target):
    engine = "soh" if target.startswith("soh/") else "mm"
    include = [ROOT / engine, ROOT / engine / "include", ROOT / engine / "include/PR", ROOT / engine / "src",
               ROOT / engine / "assets", ROOT / engine / ("soh" if engine == "soh" else "2s2h"),
               ROOT / "combo", ROOT / "combo/menu", ROOT / "combo/title", ROOT / engine / "mods",
               ROOT / "libultraship/include", ROOT / "libultraship", ROOT / "ZAPDTR/ZAPD/resource/type",
               ROOT / "torch/lib/n64graphics", *args.include]
    cvars = ROOT / "CMake" / ("soh-cvars.cmake" if engine == "soh" else "2ship-cvars.cmake")
    prefixes = re.findall(r'set\((CVAR_PREFIX_\w+) "([^"]+)"\)', cvars.read_text())
    slug = Path(target).stem + "-" + engine
    command = ["cc", "-std=gnu11"] if target.endswith(".c") else ["c++", "-std=c++20"]
    command += ["-w", "-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DFMT_HEADER_ONLY", "-DENABLE_OPENGL", "-D_CONSOLE",
                "-DLOG_LEVEL_GAME_PRINTS=6", *[f'-D{key}="{value}"' for key, value in prefixes]]
    if engine == "mm":
        command.append("-DMM_BUILD_DLL")
    if target == "soh/soh/ResourceManagerHelpers.cpp":
        command += ["-include", "imgui.h"]
    if target == "mm/2s2h/BenPort.cpp":
        command += ['-DCVAR_VSYNC_ENABLED="gSettings.VsyncEnabled"', '-DCVAR_GAME_OVERLAY_FONT="gSettings.OverlayFont"']
    if target == "mm/2s2h/Rando/DrawItem.cpp":
        command += ["-include", "nlohmann/json.hpp"]
    command += [*["-I" + str(path) for path in include], "-c", str(ROOT / target),
                "-o", str(args.out / (slug + ".o"))]
    result = subprocess.run(command, cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    hashes = "\n".join(hashlib.sha256((ROOT / path).read_bytes()).hexdigest() + "  " + path for path in
                       [target, "combo/BlackMarketFrame.h", "combo/NeiGiModelBounds.h", "combo/NeiAssetPriorityResource.h"])
    (args.out / (slug + "-compile.log")).write_text("Command: " + repr(command) + "\nSHA256:\n" + hashes +
                                                     "\nExit: " + str(result.returncode) + "\n" + result.stdout)
    print(("PASS" if not result.returncode else "FAIL") + " native object: " + target, flush=True)
    if result.returncode:
        print(result.stdout[:2500], flush=True)
    return result.returncode

with ThreadPoolExecutor(max_workers=args.jobs) as pool:
    results = list(pool.map(compile_unit, args.unit or UNITS))
raise SystemExit(any(results))
