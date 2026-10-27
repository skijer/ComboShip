#!/usr/bin/env python3
"""Compile the native forest weather/material units; no linking or gameplay claim."""
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import argparse
import hashlib
import json
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
UNITS = ["mm/2s2h/Enhancements/Audio/MMWeather.cpp",
         "mm/2s2h/Enhancements/Graphics/AutumnSceneFoliage.cpp",
         "soh/soh/Enhancements/Graphics/SeasonFoliage.cpp"]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--include", type=Path, action="append", default=[])
parser.add_argument("--out", type=Path, default=ROOT.parent / "lock-visual-verification")
args = parser.parse_args()
args.out.mkdir(parents=True, exist_ok=True)

def compile_unit(path):
    host = path.split("/", 1)[0]
    roots = [ROOT / host, ROOT / host / "include", ROOT / host / "include/PR",
             ROOT / host / "src", ROOT / host / "assets", ROOT / host / "mods",
             ROOT / host / ("soh" if host == "soh" else "2s2h"), ROOT / "combo",
             ROOT / "combo/menu", ROOT / "libultraship/include", ROOT / "libultraship",
             ROOT / "ZAPDTR/ZAPD/resource/type", ROOT / "torch/lib/n64graphics", *args.include]
    prefixes = re.findall(r'set\((CVAR_PREFIX_\w+) "([^"]+)"\)',
                          (ROOT / "CMake" / ("soh-cvars.cmake" if host == "soh" else "2ship-cvars.cmake")).read_text())
    slug = "solo-forest-" + Path(path).stem + "-" + host
    command = ["c++", "-std=c++20", "-w", "-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DFMT_HEADER_ONLY",
               "-DENABLE_OPENGL", "-D_CONSOLE", "-DLOG_LEVEL_GAME_PRINTS=6",
               *[f'-D{key}="{value}"' for key, value in prefixes]]
    if host == "mm":
        command += ["-DMM_BUILD_DLL"]
    command += ["-I" + str(root) for root in roots]
    command += ["-c", str(ROOT / path), "-o", str(args.out / (slug + ".o"))]
    sources = [path, "combo/entrances/SoloForest.h",
               "soh/soh/Enhancements/Graphics/SeasonFoliageTint.h",
               "soh/soh/Enhancements/Graphics/SeasonFoliageMaterials.inc"]
    hashes = {source: hashlib.sha256((ROOT / source).read_bytes()).hexdigest() for source in sources}
    result = subprocess.run(command, cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (args.out / (slug + "-compile.log")).write_text(
        "Command: " + json.dumps(command) + "\nSHA256: " + json.dumps(hashes, sort_keys=True) +
        "\nExit: " + str(result.returncode) + "\n" + result.stdout)
    print(("PASS" if result.returncode == 0 else "FAIL") + " native forest unit: " + path, flush=True)
    if result.returncode:
        print(result.stdout[:3000], flush=True)
    return result.returncode

with ThreadPoolExecutor(max_workers=2) as pool:
    results = list(pool.map(compile_unit, UNITS))
raise SystemExit(any(results))
