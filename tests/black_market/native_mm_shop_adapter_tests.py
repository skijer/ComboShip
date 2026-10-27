#!/usr/bin/env python3
"""Compare actual native MM shop flow with its private-market adapter boundary."""
from pathlib import Path
import argparse
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=ROOT / "mm/src/overlays/actors/ovl_En_Ossan/z_en_ossan.c")
    parser.add_argument("--vanilla-trace", action="store_true", help="emit the ordinary-shop frame trace for source comparisons")
    args = parser.parse_args()
    from run_shop_camera_mm_tests import native_engine_source, array, compile_native_shop
    source = args.source.read_text()
    fixture = (ROOT / "tests/black_market/native_mm_shop_actor_test.cpp").read_text()
    fixture = fixture.replace("/* NATIVE MM SUPPORT */", (ROOT / "tests/black_market/native_mm_shop_support.h").read_text())
    fixture = fixture.replace("/* NATIVE CAMERA AND MANAGER */", native_engine_source(observe=True))
    fixture = fixture.replace("/* NATIVE STOCK DATA */", array(source, "static ShopItem sShops["))
    with tempfile.TemporaryDirectory(prefix="market-mm-native-shop-") as temporary:
        native_object, includes = compile_native_shop(temporary, args.source)
        cpp = Path(temporary) / "test.cpp"
        cpp.write_text(fixture)
        binary = Path(temporary) / "test"
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-w", "-DCOMBO_BUILD", "-DF3DEX_GBI_2",
                        "-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-no-pie",
                        "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections",
                        *includes, str(cpp), str(native_object), "-o", str(binary)], check=True)
        subprocess.run([str(binary), *(["vanilla-trace"] if args.vanilla_trace else [])], check=True,
                       env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})


if __name__ == "__main__":
    main()
