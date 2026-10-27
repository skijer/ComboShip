#!/usr/bin/env python3
"""Run the complete OoT audio worker against its actual graphics wait loop."""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
from run_mm_audio_runtime_test import function_body

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline-ref")
    parser.add_argument("--case", choices=("synthesis", "capacity", "shutdown", "preservation", "serialization"))
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    path = "soh/soh/OTRGlobals.cpp"
    source = subprocess.check_output(["git", "show", f"{args.baseline_ref}:{path}"],
                                     cwd=ROOT, text=True) if args.baseline_ref else (ROOT / path).read_text()
    start, end = function_body(source, "OTRAudio_Thread")
    declarations = re.sub(r'extern "C"', lambda match: " " * len(match[0]), source)
    render_start, render_end = function_body(declarations, "Graph_ProcessGfxCommands")
    wait = re.search(r"while\s*\(audio\.processing\)\s*\{\s*"
                     r"audio\.cv_from_thread\.wait\(Lock\);\s*\}",
                     source[render_start:render_end])
    if wait is None:
        raise RuntimeError("production graphics completion wait was not found")
    with tempfile.TemporaryDirectory(prefix="soh-audio-worker-") as directory:
        build = Path(directory)
        (build / "soh_audio_worker.inc").write_text(source[start:end])
        (build / "soh_audio_wait.inc").write_text(wait[0])
        binary = build / "worker_test"
        flags = ["-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                 "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"] if args.sanitize else []
        emotes = not args.baseline_ref or "ComboEmotes_MixAudio" in source[start:end]
        flags += ["-DTEST_AUDIO_WORKER_EMOTES=" + str(int(emotes))]
        subprocess.run([*shlex.split(os.environ.get("CXX", "c++")), "-std=c++20", "-O2", "-g",
                        "-pthread", "-Wall", "-Wextra", "-Werror", *flags,
                        *shlex.split(os.environ.get("SOH_AUDIO_WORKER_CXXFLAGS", "")),
                        "-I" + str(build), str(ROOT / "tests/audio_mixer/soh_worker_test.cpp"),
                        "-o", str(binary)], check=True)
        for case in [args.case] if args.case else ("synthesis", "capacity", "shutdown", "preservation", "serialization"):
            subprocess.run([str(binary), case], check=True, timeout=15)


if __name__ == "__main__":
    main()
