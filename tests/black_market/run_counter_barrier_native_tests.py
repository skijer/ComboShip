#!/usr/bin/env python3
"""Exercise the MM barrier with native collision, room geometry, and talk offers.

The collision/index/triangle/matrix translation units are compiled unchanged.
Only the arena allocator, optional cheats, and talk cutscene queue are external
test boundaries. --without-barrier is the negative control: it must fail the
same native customer-to-counter collision probe.
"""
from pathlib import Path
import argparse
import os
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/black_market"))
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
import native_market
from run_time_pedestal_tests import block_from


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--without-barrier", action="store_true")
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="market-counter-barrier-") as directory:
        work = Path(directory)
        collision = work / "room.bin"
        collision.write_bytes(native_market.read_bundle(ROOT / "tools/native_market/bundled")[native_market.COLLISION])
        paths = (".", "mm", "mm/2s2h", "mm/include", "mm/include/PR", "mm/src", "mm/assets",
                 "libultraship/include", "combo")
        common = ["-w", "-DCOMBO_BUILD", "-DF3DEX_GBI_2", "-g", "-O1", "-ffunction-sections", "-fdata-sections",
                  "-fsanitize=address,undefined", "-fno-sanitize=shift", "-fno-sanitize-recover=all",
                  "-fno-omit-frame-pointer", *["-I" + str(ROOT / path) for path in paths]]
        sources = [ROOT / "mm/src/code" / name for name in (
            "z_bgcheck.c", "sys_math3d.c", "z_lib.c", "z_skin_matrix.c", "z_bg_item.c", "stubs.c")]
        actor = (ROOT / "mm/src/code/z_actor.c").read_text()
        native_talk = work / "native_talk.c"
        native_talk.write_text('#include "global.h"\n' + "\n".join(
            block_from(actor, actor.index(signature)) for signature in (
                "s32 Actor_OfferTalkExchange(", "s32 Actor_OfferTalkExchangeEquiCylinder(", "s32 Actor_OfferTalk(")))
        sources.append(native_talk)
        objects = []
        for index, source in enumerate(sources):
            obj = work / f"native-{index}.o"
            subprocess.run([os.environ.get("CC", "cc"), "-std=gnu17", *common, "-include", "global.h",
                            "-c", str(source), "-o", str(obj)], check=True)
            objects.append(str(obj))
        test = work / "test.o"
        production = (ROOT / "mm/2s2h/Enhancements/Gameplay/BlackMarket.cpp").read_text()
        guard = block_from(production, production.index('extern "C" int ComboBlackMarket_IsCounterBarrier('))
        fixture = (ROOT / "tests/black_market/counter_barrier_native_test.cpp").read_text().replace(
            "/* PRODUCTION BARRIER GUARD */", guard)
        test_source = work / "test.cpp"
        test_source.write_text(fixture)
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", *common,
                        *(["-DCOUNTER_BARRIER_DISABLED"] if args.without_barrier else []),
                        "-c", str(test_source), "-o", str(test)], check=True)
        binary = work / "test"
        subprocess.run([os.environ.get("CXX", "c++"), *objects, str(test), "-Wl,--gc-sections",
                        "-fsanitize=address,undefined", "-no-pie", "-lm", "-o", str(binary)], check=True)
        subprocess.run([str(binary), str(collision)], check=True,
                       env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})


if __name__ == "__main__":
    main()
