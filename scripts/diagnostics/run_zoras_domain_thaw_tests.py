#!/usr/bin/env python3
"""Exercise native Domain rendering/collision and seasonal water without a game run."""
from pathlib import Path
import argparse
import os
import re
import subprocess
import tempfile

from run_soh_season_weather_tests import extract
from run_mm_weather_tests import production_function

ROOT = Path(__file__).resolve().parents[2]


def run(out, sanitize):
    out.mkdir(parents=True, exist_ok=True)
    actor = ROOT / "soh/src/overlays/actors/ovl_Bg_Spot07_Taki/z_bg_spot07_taki.c"
    (out / "zoras_domain_actor.inc").write_text(actor.read_text())
    rod = ROOT / "soh/mods/items/logic/item_rod_of_seasons.c"
    getters = extract(ROOT / "soh/mods/extended_inventory.c", [
        "Seasons_HasRod", "Seasons_SeasonOwned", "Seasons_SeasonCount",
        "Seasons_SeasonAt", "Seasons_GetSeason",
    ])
    getters += extract(ROOT / "soh/src/code/z_bg_item.c", ["DynaPolyActor_Init"])
    getters += extract(rod, ["Seasons_WalksOnWater"])
    getters += extract(ROOT / "soh/src/code/z_scene_table.c", ["func_8009E730"])
    (out / "zoras_domain_native.inc").write_text(getters)

    scene = ROOT / "soh/mods/season_scene.cpp"
    source = scene.read_text()
    water = '#include "global.h"\n#include "mods/extended_inventory.h"\n'
    header = ROOT / "soh/soh/Enhancements/Graphics/ZorasDomain.h"
    if header.exists():
        water += '#include "soh/Enhancements/Graphics/ZorasDomain.h"\n'
    water += 'extern "C" int zoraWaterLook;\n'
    water += '#define SEASON_WATER_GONE (-32000)\n#define SEASON_MAX_WATERBOXES 32\n'
    water += ('static CollisionHeader* sSuppressedHeader;\n'
              'static s16 sWaterSurfaceBackup[SEASON_MAX_WATERBOXES];\n'
              'static s32 sSuppressedCount;\n')
    water += re.search(r"typedef enum \{\s*SEASON_LOOK_VANILLA,.*?} SeasonWaterLook;", source, re.S)[0]
    water += '\nstatic void SeasonScene_SetWaterLook(u8 look) { zoraWaterLook = look; }\n'
    water += extract(scene, ["SeasonScene_Active", "SeasonScene_RestoreWaterBoxes",
                            "SeasonScene_SuppressWaterBoxes", "SeasonScene_WaterLook"])
    water += 'extern "C" ' + production_function(source.replace('extern "C" ', ''), "SeasonScene_UpdateWater")
    (out / "water.cpp").write_text(water)

    base = ["-O1", "-g", "-DNDEBUG", "-DF3DEX_GBI_2", "-DCONTROLLERBUTTONS_T=uint32_t",
            "-DLOG_LEVEL_GAME_PRINTS=0", '-DCVAR_PREFIX_ENHANCEMENT="gEnhancements"']
    base += ["-I" + str(ROOT / p) for p in [
        "soh/include", "soh/src", "soh/assets", "soh", "libultraship/include", "combo",
    ]]
    base += ["-I" + str(out)]
    base += ["-I" + str(actor.parent)]
    if sanitize:
        base += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                 "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
    cc, cxx = os.environ.get("CC", "cc"), os.environ.get("CXX", "c++")
    for combo in (False, True):
        mode = "combo" if combo else "soh"
        flags = base + (["-DCOMBO_BUILD"] if combo else [])
        obj = out / (mode + ".o")
        water_obj = out / (mode + "-water.o")
        subprocess.run([cc, "-std=gnu11", *flags, "-Werror=implicit-function-declaration",
                        "-Wno-incompatible-pointer-types", "-Wno-int-conversion", "-c",
                        str(ROOT / "soh/tests/zoras_domain_thaw_test.c"), "-o", str(obj)], check=True)
        subprocess.run([cxx, "-std=c++20", "-I" + str(ROOT / "soh/tests/rain_engine_stubs"),
                        *flags, "-c", str(out / "water.cpp"),
                        "-o", str(water_obj)], check=True)
        binary = out / mode
        subprocess.run([cxx, *flags, str(obj), str(water_obj), "-lm", "-o", str(binary)], check=True)
        env = {**os.environ, "ASAN_OPTIONS": "detect_leaks=0"}
        subprocess.run([str(binary)], check=True, env=env)
        print("PASS " + mode + " native Domain thaw checks", flush=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", nargs="?", type=Path)
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    if args.output:
        run(args.output.resolve(), args.sanitize)
    else:
        with tempfile.TemporaryDirectory(prefix="zoras-domain-") as directory:
            run(Path(directory), args.sanitize)
