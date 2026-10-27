#!/usr/bin/env python3
"""Exercise the production policy, not a reimplementation of the generator."""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix="black-market-stock-") as directory:
    binary = Path(directory) / "stock"
    command = [os.environ.get("CXX", "c++"), "-std=c++20", "-Wall", "-Wextra", "-Werror", "-O1",
               "-I" + str(ROOT / "combo"), str(ROOT / "tests/black_market/stock_test.cpp"), "-o", str(binary)]
    subprocess.run(command, check=True)
    subprocess.run([str(binary)], check=True)
