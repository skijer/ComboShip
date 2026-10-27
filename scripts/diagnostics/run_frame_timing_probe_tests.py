#!/usr/bin/env python3
"""Test timing accounting, C bridge, and actual asynchronous log output; no ROM needed."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix="oot-frame-timing-") as temporary:
    temporary = Path(temporary)
    output = temporary / "frame_timing_test"
    subprocess.run([
        os.environ.get("CXX", "g++"), "-std=c++20", "-Wall", "-Wextra", "-Werror", "-pedantic",
        *shlex.split(os.environ.get("FRAME_TIMING_CXXFLAGS", "")),
        "-I", str(ROOT / "soh"), str(ROOT / "soh/tests/frame_timing_probe_test.cpp"), "-o", str(output),
    ], check=True)
    subprocess.run([str(output)], check=True)
    flight_test = temporary / "flight_test"
    subprocess.run([os.environ.get("CXX", "g++"), "-std=c++20", "-Wall", "-Wextra", "-Werror", "-pedantic",
        *shlex.split(os.environ.get("FRAME_TIMING_CXXFLAGS", "")), "-I", str(ROOT / "soh"),
        str(ROOT / "soh/tests/frame_flight_recorder_test.cpp"), "-o", str(flight_test)], check=True)
    subprocess.run([str(flight_test)], check=True)
    render_test = temporary / "render_cost_test"
    subprocess.run([
        os.environ.get("CXX", "g++"), "-std=c++20", "-Wall", "-Wextra", "-Werror", "-pedantic",
        "-I", str(ROOT / "libultraship/include"), str(ROOT / "soh/tests/render_cost_probe_test.cpp"),
        "-o", str(render_test),
    ], check=True)
    subprocess.run([str(render_test)], check=True)
    source = temporary / "bridge.c"
    source.write_text('#include "soh/Enhancements/debugger/FrameTimingProbe.h"\n'
                      'void sample(void) { FrameTimingSpan span = FrameTiming_BeginSpan();\n'
                      'FrameTiming_EndSpan(FRAME_TIMING_PLAY_UPDATE, span); }\n')
    subprocess.run([os.environ.get("CC", "gcc"), "-std=c11", "-Wall", "-Werror", "-pedantic",
                    "-I", str(ROOT / "soh"), "-c", str(source), "-o", str(temporary / "bridge.o")], check=True)
    print("C bridge header compiled")
    output_test = temporary / "frame_timing_output_test"
    spdlog_flags = os.environ.get("FRAME_TIMING_SPDLOG_FLAGS")
    if spdlog_flags is None:
        spdlog_flags = subprocess.check_output(["pkg-config", "--cflags", "--libs", "spdlog"], text=True)
    subprocess.run([
        os.environ.get("CXX", "g++"), "-std=c++20", "-Wall", "-Wextra", "-Werror", "-pedantic",
        *shlex.split(os.environ.get("FRAME_TIMING_CXXFLAGS", "")),
        "-I", str(ROOT / "soh"), str(ROOT / "soh/tests/frame_timing_output_test.cpp"),
        "-I", str(ROOT / "libultraship/include"),
        str(ROOT / "soh/tests/frame_timing_context_fixture.cpp"),
        str(ROOT / "soh/soh/Enhancements/debugger/FrameTimingProbe.cpp"),
        str(ROOT / "libultraship/src/ship/diagnostics/PerformanceTrace.cpp"),
        *shlex.split(spdlog_flags), "-pthread", "-o", str(output_test),
    ], check=True)
    subprocess.run([str(output_test), str(temporary / "output")], check=True)
