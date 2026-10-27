#!/usr/bin/env python3
"""Characterize the actual Woodfall return owner, without changing its styling.

Native GBI macros emit commands from the production builder/draw function. Only
the matrix service, endpoint and resource lookup boundary are substituted.
Optional native archive verification reads metadata/pixels without exporting any.
"""
import argparse
import hashlib
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mm-archive", type=Path)
    args = parser.parse_args()
    renderer = (ROOT / "mm/2s2h/Enhancements/Graphics/ComboLostWoodsPassage.cpp").read_text()
    rcp = (ROOT / "mm/src/code/z_rcp.c").read_text()
    begin = rcp.index("        /* SETUPDL_25 */")
    setup_list = rcp[begin:rcp.index("    },", begin)]
    geometry = renderer[renderer.index("constexpr int kArchSegments"):renderer.index("} // namespace")]
    fixture = Path(__file__).with_name("woodfall_draw_test.cpp").read_text()
    fixture = fixture.replace("/* LIST */", setup_list).replace("/* BUILD */", geometry)
    fixture = fixture.replace("/* DRAW */", block_from(renderer, renderer.index("static void DrawWoodfallPassage(")))
    fixture = fixture.replace("/* SETUP */", block_from(rcp, rcp.index("void Gfx_SetupDL25_Opa(")))
    includes = ["mm", "mm/2s2h", "mm/include", "mm/include/PR", "mm/src", "mm/assets",
                "libultraship/include", "combo"]
    with tempfile.TemporaryDirectory(prefix="woodfall-draw-") as temporary:
        cpp, binary = Path(temporary) / "test.cpp", Path(temporary) / "test"
        cpp.write_text(fixture)
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-w", "-DCOMBO_BUILD", "-DF3DEX_GBI_2",
                        "-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-no-pie",
                        *["-I" + str(ROOT / p) for p in includes], str(cpp), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
    if args.mm_archive:
        path = "scenes/nonmq/Z2_26SARUNOMORI/Z2_26SARUNOMORITex_00BAC0"
        with zipfile.ZipFile(args.mm_archive) as archive:
            data = archive.read(path)
        assert struct.unpack_from("<IIII", data, 64) == (2, 32, 32, 2048)
        assert len(data) == 80 + 2048
        assert all(pixel & 1 for pixel in struct.unpack(">1024H", data[80:]))
        print("PASS read-only native bark: RGBA16 32x32, all 1024 alpha bits opaque; resource SHA256",
              hashlib.sha256(data).hexdigest())


if __name__ == "__main__":
    main()
