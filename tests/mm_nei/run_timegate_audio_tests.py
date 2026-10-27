"""Run the real MM Time Gate handler against the native MM sound-request/bank engine."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_mm_nei_tests import flags


with tempfile.TemporaryDirectory(prefix="mm-timegate-audio-") as td:
    binary = Path(td) / "timegate-audio"
    subprocess.run([
        os.environ.get("CC", "cc"), "-std=gnu11", *flags(),
        "-Werror=implicit-function-declaration", "-Wno-incompatible-pointer-types",
        "-Wno-discarded-qualifiers", "-ffunction-sections", "-fdata-sections", "-g",
        "-fsanitize=address,undefined", str(ROOT / "tests/mm_nei/timegate_audio_test.c"),
        str(ROOT / "mm/src/audio/sfx.c"), str(ROOT / "mm/src/audio/sfx_params.c"),
        "-Wl,--gc-sections", "-lm", "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True, timeout=10,
                   env=dict(os.environ, ASAN_OPTIONS="detect_leaks=0"))
