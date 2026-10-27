#!/usr/bin/env python3
"""Run the shared default room effects with both hosts' real GBI/types."""
from pathlib import Path
import argparse
import os
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--host", choices=("mm", "soh", "all"), default="all")
args = parser.parse_args()
with tempfile.TemporaryDirectory(prefix="market-interior-effects-") as temporary:
    for host in (("mm", "soh") if args.host == "all" else (args.host,)):
        binary = Path(temporary) / host
        includes = [host, host + "/include", host + "/include/PR", host + "/src", host + "/assets",
                    host + ("/2s2h" if host == "mm" else "/soh"), "libultraship/include", "combo"]
        source = (ROOT / host / ("2s2h" if host == "mm" else "soh") / "Enhancements/Gameplay" /
                  ("BlackMarket.cpp" if host == "mm" else "ComboBlackMarketNative.cpp")).read_text()
        anchors = re.search(r"constexpr Vec3f kShelves\[8\] = \{.*?\};", source, re.S)[0]
        fixture = (ROOT / "tests/black_market/interior_effects_native_test.cpp").read_text()
        wrapper = (ROOT / ("soh/soh/GbiWrap.cpp" if host == "soh" else "mm/src/code/stubs.c")).read_text()
        signatures = (ROOT / ("soh/soh/ResourceManagerHelpers.cpp" if host == "soh" else "mm/2s2h/BenPort.cpp")).read_text()
        native_segment = re.search(r'^extern "C" int ResourceMgr_OTRSigCheck\([^\n]*\{.*?^\}', signatures, re.M | re.S)[0]
        segment_signature = 'extern "C" void gSPSegment(' if host == "soh" else 'void gSPSegment('
        native_segment += "\n" + block_from(wrapper, wrapper.index(segment_signature))
        fixture = fixture.replace("/* NATIVE SEGMENT WRAPPER */", native_segment)
        rcp = (ROOT / host / "src/code/z_rcp.c").read_text()
        setup = re.search(r"/\* SETUPDL_25 \*/(.*?)\n    \},", rcp, re.S)[1]
        fixture = fixture.replace("/* SHOP ANCHORS */", anchors).replace("/* NATIVE SETUP */", "static Gfx nativeSetup[] = {" + setup + "\n};")
        cpp = Path(temporary) / (host + ".cpp"); cpp.write_text(fixture)
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-w", "-DF3DEX_GBI_2", "-DCOMBO_BUILD",
                        *(["-DOOT_EFFECT_TEST"] if host == "soh" else []),
                        "-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-no-pie",
                        "-I" + str(ROOT), *["-I" + str(ROOT / path) for path in includes],
                        str(cpp), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
