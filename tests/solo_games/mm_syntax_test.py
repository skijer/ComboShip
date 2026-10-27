#!/usr/bin/env python3
"""Compile the 25 solo-MM production units with real project/dependency headers.

Set CPLUS_INCLUDE_PATH/C_INCLUDE_PATH for external dependencies; the fallback
matches oot_syntax_test.py's available workspace headers. No headers are stubbed.
--source-rev checks revisioned source bodies against the current project headers.
This is syntax verification, not full linking or game-runtime evidence.
"""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[2]
SOURCES = [
    "mm/2s2h/BenGui/BenGui.cpp",
    "mm/2s2h/BenGui/BenMenu.cpp",
    "mm/2s2h/BenPort.cpp",
    "mm/2s2h/DeveloperTools/DebugConsole.cpp",
    "mm/2s2h/Enhancements/Graphics/ComboLostWoodsPassage.cpp",
    "mm/2s2h/Enhancements/Items/ColorPictograph.cpp",
    "mm/2s2h/Enhancements/Trackers/ItemTracker/ItemTrackerSettings.cpp",
    "mm/2s2h/FleetShipCombo/FleetComboGoal.cpp",
    "mm/2s2h/FleetShipCombo/FleetPicto.cpp",
    "mm/2s2h/FleetShipCombo/FleetSharedItems.cpp",
    "mm/2s2h/FleetShipCombo/FleetShipCombo.cpp",
    "mm/2s2h/Rando/CheckTracker/CheckTracker.cpp",
    "mm/2s2h/Rando/GiveItem.cpp",
    "mm/2s2h/Rando/Logic/GeneratePools.cpp",
    "mm/2s2h/Rando/Logic/Logic.cpp",
    "mm/2s2h/Rando/Menu.cpp",
    "mm/2s2h/Rando/MiscBehavior/CheckQueue.cpp",
    "mm/2s2h/Rando/Rando.cpp",
    "mm/2s2h/Rando/Spoiler/Apply.cpp",
    "mm/2s2h/SaveManager/SaveManager.cpp",
    "mm/src/code/title_setup.c",
    "mm/src/code/z_play.c",
    "mm/src/overlays/actors/ovl_Boss_07/z_boss_07.c",
    "mm/src/overlays/gamestates/ovl_file_choose/z_file_copy_erase.c",
    "mm/src/overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_equipment.c",
]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--source-rev", help="git revision supplying source bodies; headers remain current")
parser.add_argument("--source", action="append", choices=SOURCES, help="explicitly restrict to this source (repeatable)")
args = parser.parse_args()
flags = ["-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DMM_BUILD_DLL", "-DCONTROLLERBUTTONS_T=uint32_t",
         "-DLOG_LEVEL_GAME_PRINTS=0", "-DNON_EQUIVALENT", "-DNON_MATCHING", "-DIMGUI_DEFINE_MATH_OPERATORS"]
for file, pattern in [("libultraship/cmake/cvars.cmake", r'set\((CVAR_\w+) "([^"]+)"'),
                      ("CMake/2ship-cvars.cmake", r'set\((CVAR_PREFIX_\w+) "([^"]+)"\)')]:
    flags += [f'-D{key}="{value}"' for key, value in re.findall(pattern, (ROOT / file).read_text())]
flags += ["-I" + str(ROOT / directory) for directory in
          ["mm/include", "mm/include/PR", "mm/src", "mm", "mm/assets", "mm/mods", "mm/2s2h",
           "libultraship/include", "libultraship/src", "combo", "combo/menu", "combo/gui"]]
cpp_flags = ["-fpermissive", *shlex.split(os.environ.get("SYNTAX_CXXFLAGS", ""))]
for header in ["libultraship/bridge/consolevariablebridge.h", "ship/Context.h",
               "ship/resource/ResourceManager.h", "ship/window/Window.h", "spdlog/spdlog.h",
               "variables.h", "functions.h", "macros.h", "z64.h", "2s2h/GameInteractor/GameInteractor.h"]:
    cpp_flags += ["-include", header]
c_flags = ["-Werror=implicit-function-declaration", "-Wno-int-conversion", "-Wno-incompatible-pointer-types"]
fallback = ":".join(["/workspace/scratch/4288e889429f/build-deps/include",
                     "/workspace/scratch/4288e889429f/build-deps/sysroot/usr/include",
                     "/workspace/scratch/60110ae7df32/analysis/deps",
                     "/workspace/scratch/60110ae7df32/analysis/deps/SDL2"])
env = {**os.environ, **{key: os.environ.get(key, fallback) for key in ["CPLUS_INCLUDE_PATH", "C_INCLUDE_PATH"]}}
sources, failures = args.source or SOURCES, []
print(f"Checking {len(sources)} MM units" + (f" from {args.source_rev} (current headers)" if args.source_rev else ""), flush=True)
for source in sources:
    cpp = source.endswith(".cpp")
    compiler = shlex.split(os.environ.get("CXX" if cpp else "CC", "c++" if cpp else "cc"))
    command = [*compiler, "-std=gnu++20" if cpp else "-std=gnu11", *flags,
               *(cpp_flags if cpp else c_flags), "-fsyntax-only"]
    try:
        content = None
        if args.source_rev:
            content = subprocess.check_output(["git", "show", f"{args.source_rev}:{source}"], cwd=ROOT, text=True)
            command += ["-iquote", str((ROOT / source).parent), "-x", "c++" if cpp else "c", "-"]
        else:
            command += [str(ROOT / source)]
        result = subprocess.run(command, input=content, cwd=ROOT, capture_output=True, text=True, env=env)
    except (OSError, subprocess.CalledProcessError) as error:
        failures.append(source)
        print(f"FAIL {source}: {error}", flush=True)
        continue
    if result.returncode:
        failures.append(source)
        missing_png = "png.h: No such file or directory" in result.stderr
        reason = "ENVIRONMENT FAILURE (missing real png.h; install libpng development headers)" if missing_png else "FAIL"
        print(f"{reason}: {source}\n{result.stdout}{result.stderr}", flush=True)
    else:
        print(f"PASS {source} ({result.stderr.count('warning:')} header/unity warnings)", flush=True)
print(f"MM syntax: {len(sources) - len(failures)}/{len(sources)} passed; {len(failures)} failed", flush=True)
if failures:
    raise SystemExit(1)
