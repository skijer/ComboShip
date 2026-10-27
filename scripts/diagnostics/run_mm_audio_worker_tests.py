#!/usr/bin/env python3
"""Run the complete native MM worker with a controlled audio-device boundary."""
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
from run_mm_audio_runtime_test import function_body

ROOT = Path(__file__).resolve().parents[2]


def main():
    source = (ROOT / "mm/2s2h/BenPort.cpp").read_text()
    start, end = function_body(source, "OTRAudio_Thread")
    with tempfile.TemporaryDirectory(prefix="mm-audio-worker-") as directory:
        build = Path(directory)
        (build / "mm_audio_worker.inc").write_text(source[start:end])
        binary = build / "worker_test"
        subprocess.run([*shlex.split(os.environ.get("CXX", "c++")), "-std=c++20", "-O2", "-g", "-DCOMBO_BUILD",
                        "-DTEST_AUDIO_WORKER_EMOTES=1",
                        "-pthread", "-Wall", "-Wextra", "-Werror", "-Wno-unused-variable",
                        *shlex.split(os.environ.get("MM_AUDIO_WORKER_CXXFLAGS", "")),
                        "-I" + str(build), str(ROOT / "tests/audio_mixer/mm_worker_test.cpp"),
                        "-o", str(binary)], check=True)
        for enabled in ['0', '1']:
            subprocess.run([str(binary), enabled], check=True, timeout=10)


if __name__ == "__main__":
    main()
