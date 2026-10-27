#!/usr/bin/env python3
"""Execute both keeper asset-selection, initialization and draw paths."""
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
with tempfile.TemporaryDirectory(prefix="market-keeper-native-") as temporary:
    for host in (("mm", "soh") if args.host == "all" else (args.host,)):
        source = (ROOT / host / ("2s2h" if host == "mm" else "soh") / "Enhancements/Gameplay" /
                  ("BlackMarket.cpp" if host == "mm" else "ComboBlackMarketNative.cpp")).read_text()
        constants = "\n".join(re.findall(r'(?:alignas\(2\) )?constexpr char k(?:Keeper|Idle|Card)\[\] = [^;]+;', source))
        helpers = re.finditer(r"(?:bool|void) (?:CustomKeeperAvailable|LoadNativeKeeper)\([^;{}]*\)\s*\{", source)
        bodies = [block_from(source, match.start()) for match in helpers]
        if host == "mm":
            native = (ROOT / "mm/2s2h/Enhancements/Gameplay/BlackMarketMmNativeShop.h").read_text()
            bodies.append(block_from(native, native.index("void NativeKeeperBlink(")))
        bodies += [block_from(source, source.index(s)) for s in
                   ("s32 KeeperLimb(", "void KeeperDraw(", "void InitKeeper(" if host == "mm" else "void KeeperInit(")]
        if "void PresentTitle(" in source:
            bodies.append(block_from(source, source.index("void PresentTitle(")))
        fixture = (ROOT / "tests/black_market/keeper_native_test.cpp").read_text().replace("/* PRODUCTION */", constants + "\n" + "\n".join(bodies))
        cpp = Path(temporary) / (host + ".cpp"); cpp.write_text(fixture)
        binary = Path(temporary) / host
        paths = [host, host + "/include", host + "/include/PR", host + "/src", host + "/assets",
                 host + ("/2s2h" if host == "mm" else "/soh"), "libultraship/include", "combo"]
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-w", "-DCOMBO_BUILD", "-DF3DEX_GBI_2",
                        *(["-DOOT_KEEPER_TEST"] if host == "soh" else []), "-g", "-fsanitize=address,undefined",
                        "-fno-sanitize-recover=all", "-no-pie", "-I" + str(ROOT),
                        *["-I" + str(ROOT / path) for path in paths], str(cpp), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
