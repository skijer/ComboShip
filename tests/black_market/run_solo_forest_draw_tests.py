#!/usr/bin/env python3
"""Execute both native forest draw bodies at the graphics submission boundary."""
from pathlib import Path
import argparse
import os
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--case", choices=("fan", "state", "all"), default="all")
parser.add_argument("--host", choices=("mm", "soh", "all"), default="all")
args = parser.parse_args()
results = []
with tempfile.TemporaryDirectory(prefix="solo-forest-draw-") as temporary:
    for host in (("mm", "soh") if args.host == "all" else (args.host,)):
        path = ROOT / host / ("2s2h" if host == "mm" else "soh") / "Enhancements/Gameplay/ComboSoloForest.cpp"
        source = path.read_text()
        body = block_from(source, source.index("void DrawForestPortal("))
        if "bool NativePortal(" in source:
            body=block_from(source,source.index("bool NativePortal("))+"\n"+body
        fixture = (ROOT / "tests/black_market/solo_forest_draw_test.cpp").read_text().replace("/* PRODUCTION */", body)
        cpp = Path(temporary) / (host + ".cpp")
        cpp.write_text(fixture)
        binary = Path(temporary) / host
        includes = [host, host + "/include", host + "/include/PR", host + "/src", host + "/assets",
                    host + ("/2s2h" if host == "mm" else "/soh"), "libultraship/include", "combo", "combo/menu"]
        warnings = ["-Werror=write-strings"] if host == "soh" else ["-w"]
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", *warnings, "-DCOMBO_BUILD", "-DF3DEX_GBI_2",
                        *(["-DOOT_FOREST_TEST"] if host == "soh" else []), "-g", "-fsanitize=address,undefined",
                        "-fno-sanitize-recover=all", "-no-pie", "-I" + str(ROOT), *["-I" + str(ROOT / p) for p in includes],
                        str(cpp), "-o", str(binary)], check=True)
        print("Native forest draw host:", host, flush=True)
        results.append(subprocess.run([str(binary), args.case], env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"}).returncode)
raise SystemExit(any(results))
