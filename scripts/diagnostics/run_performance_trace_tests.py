#!/usr/bin/env python3
"""Compile the shared-engine trace service and exercise bounds and worker lifetime."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix="performance-trace-") as temporary:
    binary = Path(temporary) / "trace_test"
    subprocess.run([
        *shlex.split(os.environ.get("CXX", "c++")), "-std=c++20", "-Wall", "-Wextra", "-Werror",
        *shlex.split(os.environ.get("PERFORMANCE_TRACE_CXXFLAGS", "")),
        "-I" + str(ROOT / "libultraship/include"),
        str(ROOT / "libultraship/tests/performance_trace_test.cpp"),
        str(ROOT / "libultraship/src/ship/diagnostics/PerformanceTrace.cpp"),
        "-pthread", "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True)
