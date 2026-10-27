"""Syntax-check the changed game and launcher units with native headers/CVar prefixes.

This is compiler verification, not a complete build/link or gameplay test.
External dependency include directories can be supplied through CPATH.
"""
import os
import re
import shlex
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCES = [
    "mm/2s2h/Rando/NeiGrace.cpp", "mm/2s2h/Rando/StaticData/Options.cpp",
    "mm/2s2h/Rando/Logic/GeneratePools.cpp", "mm/2s2h/Rando/Menu.cpp", "mm/src/code/z_bgcheck.c",
    "mm/src/overlays/actors/ovl_Object_Kankyo/z_object_kankyo.c",
    "mm/src/overlays/actors/ovl_player_actor/z_player.c", "mm/2s2h/BenPort.cpp",
    "soh/soh/Enhancements/randomizer/NeiGrace.cpp", "soh/soh/Enhancements/randomizer/settings.cpp",
    "soh/soh/Enhancements/randomizer/logic.cpp", "soh/soh/Enhancements/randomizer/3drando/item_pool.cpp",
    "soh/soh/SohGui/SohMenuNEI.cpp", "soh/src/overlays/actors/ovl_player_actor/z_player.c",
    "soh/soh/OTRGlobals.cpp", "combo/ComboShip.cpp", "combo/ComboRandoHeadless.cpp",
    "combo/gui/ComboSettingsSync.cpp",
]

failures = []
for filename in (sys.argv[1:] or SOURCES):
    game = filename.split("/")[0]
    cpp = filename.endswith(".cpp")
    compiler = shlex.split(os.environ.get("CXX" if cpp else "CC", "c++" if cpp else "cc"))
    flags = ["-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DCONTROLLERBUTTONS_T=uint32_t", "-DLOG_LEVEL_GAME_PRINTS=0",
             "-DNON_EQUIVALENT", "-DNON_MATCHING", "-DIMGUI_DEFINE_MATH_OPERATORS"]
    for key, value in re.findall(r'set\((CVAR_\w+) "([^"]+)"',
                                (ROOT / "libultraship/cmake/cvars.cmake").read_text()):
        flags.append(f'-D{key}="{value}"')
    if game == "combo":
        release = re.search(r'set\(COMBO_RELEASE_VERSION "([^"]+)"', (ROOT / "CMakeLists.txt").read_text())
        flags.append(f'-DCOMBO_RELEASE_VERSION="{release[1]}"')
    directories = ["libultraship/include", "libultraship/src", "combo", "combo/menu", "combo/gui"]
    if game in ("mm", "soh"):
        directories = [f"{game}/include", f"{game}/include/PR", f"{game}/src", game, f"{game}/assets", f"{game}/mods",
                       "mm/2s2h" if game == "mm" else "soh/soh", *directories]
        flags += ["-DMM_BUILD_DLL" if game == "mm" else "-DSOH_BUILD_DLL"]
        for key, value in re.findall(r'set\((CVAR_PREFIX_\w+) "([^"]+)"\)',
                                    (ROOT / "CMake" / ("2ship-cvars.cmake" if game == "mm" else "soh-cvars.cmake")).read_text()):
            flags.append(f'-D{key}="{value}"')
    flags += ["-I" + str(ROOT / directory) for directory in directories]
    # GCC 15 diagnoses existing UI fluent methods that share their enum names.
    # Keep those baseline declarations as warnings during this compiler probe.
    if cpp:
        flags += shlex.split(os.environ.get("SYNTAX_CXXFLAGS", ""))
        if game == "mm":
            flags += ["-fpermissive"]  # mm/CMakeLists.txt uses this for GNU C++.
    if cpp and game == "mm":
        for header in ("libultraship/bridge/consolevariablebridge.h", "ship/Context.h",
                       "ship/resource/ResourceManager.h", "ship/window/Window.h", "spdlog/spdlog.h",
                       "variables.h", "functions.h", "macros.h", "z64.h", "2s2h/GameInteractor/GameInteractor.h"):
            flags += ["-include", header]
    if not cpp:
        flags += ["-Werror=implicit-function-declaration", "-Wno-int-conversion", "-Wno-incompatible-pointer-types"]
    result = subprocess.run([*compiler, "-std=gnu++20" if cpp else "-std=gnu11", *flags,
                             "-fsyntax-only", str(ROOT / filename)], capture_output=True, text=True)
    if result.returncode:
        failures.append(filename)
        print("FAIL " + filename + "\n" + result.stderr, flush=True)
    else:
        print(f"PASS native-header syntax: {filename} ({result.stderr.count('warning:')} warnings)", flush=True)
if failures:
    raise SystemExit("Syntax failures: " + ", ".join(failures))
