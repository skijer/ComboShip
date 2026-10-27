"""Run MM's production item draws and check their emitted color state.

The resource service, CVars and graphics allocation are fixture boundaries.
Real engine types, GBI commands and unchanged draw bodies run in the fixture;
complete affected C files are also syntax checked with their real headers.
"""
import os
import argparse
from pathlib import Path
import shlex
import subprocess
import tempfile

from run_mm_scene_randomization_tests import function

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    selected = []
    paths = {
        "mm/src/code/z_draw.c": ["GetItem_DrawDListWithCosmetics", "GetItem_DrawRecoveryHeart",
                                "GetItem_DrawOpa0", "GetItem_DrawXlu01"],
        "mm/src/code/z_en_item00.c": ["EnItem00_DrawHeartContainer", "EnItem00_DrawHeartPiece"],
        "mm/src/overlays/actors/ovl_Item_B_Heart/z_item_b_heart.c": ["ItemBHeart_Draw"],
        "mm/2s2h/Enhancements/Graphics/3DItemDrops.cpp": ["EnItem00_3DItemsDraw", "DrawSlime3DItem"],
        "mm/2s2h/Rando/DrawItem.cpp": ["DrawDoubleDefense"],
    }
    for path, names in paths.items():
        source = (ROOT / path).read_text()
        for name in names:
            if name == "GetItem_DrawDListWithCosmetics" and name not in source:
                continue  # A pre-fix run fails on missing tint, not missing symbols.
            selected.append(function(source, name))

    flags = ["-DNDEBUG", "-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DMM_BUILD_DLL",
             "-DCONTROLLERBUTTONS_T=uint32_t", "-DNON_EQUIVALENT", "-DNON_MATCHING",
             "-Wno-int-conversion", "-Wno-incompatible-pointer-types"]
    flags += ["-I" + str(ROOT / p) for p in
              ("mm/include", "mm/include/PR", "mm/src", "mm", "mm/2s2h", "mm/assets",
               "libultraship/include", "libultraship/src", "combo", "combo/menu")]
    if args.sanitize:
        flags += ["-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                  "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
    cc = shlex.split(os.environ.get("CC", "cc"))
    with tempfile.TemporaryDirectory(prefix="mm-custom-item-color-") as temporary:
        build = Path(temporary)
        (build / "custom_item_color_production.inc").write_text("\n".join(selected))
        binary = build / "custom_item_color_test"
        subprocess.run([*cc, "-std=gnu11", *flags, "-I" + str(build),
                        str(ROOT / "mm/tests/custom_item_color_test.c"), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})

        for path in list(paths)[:3]:
            result = subprocess.run([*cc, "-std=gnu11", *flags, "-fsyntax-only",
                                     "-Werror=implicit-function-declaration", str(ROOT / path)],
                                    capture_output=True, text=True)
            if result.returncode:
                raise RuntimeError(result.stdout + result.stderr)
            print("PASS real-header item syntax:", path)

        body = selected[-1]
        cpp = build / "double_defense.cpp"
        cpp.write_text('extern "C" {\n#include "global.h"\n'
                       '#include "assets/objects/object_gi_hearts/object_gi_hearts.h"\n'
                       'void NeiGi_ReportBlackMarketModel(const char*, const char*, const char*);\n}\n' + body)
        subprocess.run([*shlex.split(os.environ.get("CXX", "c++")), "-std=gnu++20",
                        *[f for f in flags if f not in ("-Wno-int-conversion", "-Wno-incompatible-pointer-types")],
                        "-fsyntax-only", str(cpp)], check=True)
        print("PASS Double Defense C++ body and shared C declaration")


if __name__ == "__main__":
    main()
