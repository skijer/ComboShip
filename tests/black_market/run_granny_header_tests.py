#!/usr/bin/env python3
"""Compile the shared Granny adapter with UI and native engine GBI settings."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix="granny-header-") as temporary:
    source = Path(temporary) / "header.cpp"
    source.write_text(
        '#include "combo/BlackMarketGranny.h"\n'
        'int main() { BlackMarketGranny::Room room; return room.Opaque() != nullptr; }\n'
    )
    for name, defines in [("shared UI", []), ("native engines", ["-DF3DEX_GBI_2"])]:
        subprocess.run(
            [os.environ.get("CXX", "c++"), "-std=c++20", *defines,
             "-I" + str(ROOT), "-I" + str(ROOT / "libultraship/include"),
             "-fsyntax-only", str(source)],
            check=True,
        )
        print("PASS: Granny header compiles for " + name, flush=True)

    binary = Path(temporary) / "refinement-filter"
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-DF3DEX_GBI_2", "-g",
                    "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-no-pie",
                    "-I" + str(ROOT), "-I" + str(ROOT / "libultraship/include"),
                    str(ROOT / "tests/black_market/granny_refinement_unit_test.cpp"), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})

subprocess.run([sys.executable, '-B', str(ROOT / 'tests/black_market/run_granny_availability_tests.py')], check=True)
