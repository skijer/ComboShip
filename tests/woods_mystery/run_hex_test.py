#!/usr/bin/env python3
"""Verify the active GI hex particle sampler; old ring previews are not runtime proof."""
from pathlib import Path
import subprocess
import sys
ROOT = Path(__file__).resolve().parents[2]
subprocess.run([sys.executable, "-B", str(ROOT / "scripts/diagnostics/run_forest_gi_particles_tests.py"),
                "--policy-only"], check=True)
subprocess.run([sys.executable, "-B", str(ROOT / "tests/woods_mystery/run_shimmer_placement_test.py")], check=True)
