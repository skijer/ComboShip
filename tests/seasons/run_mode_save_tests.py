"""Exercise MM's full native save serializer and missing-key defaults."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix="seasons-mode-save-") as td:
    binary = Path(td) / "save"
    result = subprocess.run([
        os.environ.get("CXX", "c++"), "-std=c++20", "-O1", "-w", "-DCOMBO_BUILD", "-DF3DEX_GBI_2",
        "-DCONTROLLERBUTTONS_T=uint32_t", "-DFMT_HEADER_ONLY",
        *["-I" + str(ROOT / path) for path in ("mm", "mm/2s2h", "mm/include", "mm/include/PR",
                                             "mm/src", "mm/assets", "libultraship/include", "combo")],
        str(ROOT / "tests/seasons/mode_save_test.cpp"), "-o", str(binary)
    ], capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    subprocess.run([str(binary)], check=True)
