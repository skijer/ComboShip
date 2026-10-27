#!/usr/bin/env python3
"""Compile native solo forest integration units without linking either engine."""
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import argparse
import hashlib
import json
import os
import re
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[2]
UNITS = ["mm/2s2h/Enhancements/Gameplay/ComboSoloForest.cpp",
         "soh/soh/Enhancements/Gameplay/ComboSoloForest.cpp",
         "mm/2s2h/Enhancements/Gameplay/MagicMirror.cpp",
         "soh/soh/Enhancements/Gameplay/MagicMirror.cpp", "mm/2s2h/z_play_2SH.cpp",
         "soh/mods/transformation_masks/assets/mm_asset_loader.cpp",
         "mm/2s2h/Enhancements/Graphics/ComboLostWoodsPassage.cpp",
         "mm/2s2h/Enhancements/Gameplay/BlackMarket.cpp",
         "soh/soh/Enhancements/Gameplay/ComboBlackMarketNative.cpp",
         "soh/src/code/title_setup.c",
         "soh/soh/Enhancements/TimeSavers/timesaver_hook_handlers.cpp",
         "mm/2s2h/SaveManager/SaveManager.cpp", "mm/src/code/z_play.c", "soh/src/code/z_play.c",
         "soh/src/code/z_scene_table.c",
         "soh/soh/Enhancements/gameplaystats.cpp", "soh/soh/util.cpp"]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--include", type=Path, action="append", default=[])
parser.add_argument("--unit", choices=UNITS, action="append")
parser.add_argument("--jobs", type=int, default=2)
parser.add_argument("--out", type=Path, default=ROOT.parent / "lock-visual-verification")
args = parser.parse_args()
args.out.mkdir(parents=True, exist_ok=True)

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def compile_unit(path):
    host = path.split("/", 1)[0]
    native = ROOT / path
    is_c = native.suffix == ".c"
    roots = [ROOT / host, ROOT / host / "include", ROOT / host / "include/PR",
             ROOT / host / "src", ROOT / host / "assets", ROOT / host / "mods",
             ROOT / host / ("soh" if host == "soh" else "2s2h"), ROOT / "combo",
             ROOT / "combo/menu", ROOT / "combo/title", ROOT / "libultraship/include", ROOT / "libultraship",
             ROOT / "ZAPDTR/ZAPD/resource/type", ROOT / "torch/lib/n64graphics", *args.include]
    prefixes = re.findall(r'set\((CVAR_PREFIX_\w+) "([^"]+)"\)',
                          (ROOT / "CMake" / ("soh-cvars.cmake" if host == "soh" else "2ship-cvars.cmake")).read_text())
    slug = "solo-forest-core-" + host + "-" + native.stem
    obj, dep = args.out / (slug + ".o"), args.out / (slug + ".d")
    # Match the per-host CMake language standards and native MM global-header
    # precompile environment. All includes are real headers, with no test shims.
    compiler = os.environ.get("CC" if is_c else "CXX", "cc" if is_c else "c++")
    standard = ("gnu17" if host == "mm" else "gnu2x") if is_c else "c++20"
    command = [compiler, "-std=" + standard, "-w", "-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DFMT_HEADER_ONLY",
               "-DENABLE_OPENGL", "-D_CONSOLE", "-DLOG_LEVEL_GAME_PRINTS=6", "-fvisibility=hidden",
               "-DMM_BUILD_DLL" if host == "mm" else "-DSOH_BUILD_DLL",
               *[f'-D{key}="{value}"' for key, value in prefixes]]
    if host == "soh":
        command += ["-Dfunc_80041DB8=SurfaceType_GetWallFlags"]
    command += ["-I" + str(root) for root in roots]
    if is_c and host == "mm":
        command += ["-include", "global.h"]
    command += ["-MMD", "-MF", str(dep), "-c", str(native), "-o", str(obj)]
    hashes = {path: digest(native), "combo/entrances/SoloForest.h": digest(ROOT / "combo/entrances/SoloForest.h"),
              "combo/entrances/SoloForestResources.h": digest(ROOT / "combo/entrances/SoloForestResources.h")}
    result = subprocess.run(command, cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if result.returncode == 0:
        dependencies = shlex.split(dep.read_text().replace("\\\n", ""))[1:]
        manifest = {str(Path(p)): digest(Path(p)) for p in dependencies}
        (args.out / (slug + "-dependencies.json")).write_text(json.dumps(manifest, sort_keys=True, indent=2) + "\n")
        hashes["object"] = digest(obj)
    (args.out / (slug + "-compile.log")).write_text(
        "Command: " + json.dumps(command) + "\nSHA256: " + json.dumps(hashes, sort_keys=True) +
        "\nExit: " + str(result.returncode) + "\n" + result.stdout)
    print(("PASS" if result.returncode == 0 else "FAIL") + " native core unit: " + path, flush=True)
    if result.returncode:
        print(result.stdout[:4000], flush=True)
    return result.returncode

with ThreadPoolExecutor(max_workers=max(1, args.jobs)) as pool:
    results = list(pool.map(compile_unit, args.unit or UNITS))
raise SystemExit(any(results))
