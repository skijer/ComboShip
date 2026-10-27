#!/usr/bin/env python3
"""Focused repeatable gate for the bundled Chateau model and shimmer."""
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
subprocess.run([sys.executable, '-B', str(ROOT / 'scripts/assets/prepare_chateau_romani.py')], check=True)


def run(*args):
    subprocess.run(list(map(str, args)), cwd=ROOT, check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})


with tempfile.TemporaryDirectory(prefix='chateau-romani-') as temporary:
    output = Path(temporary)
    first, second = output / 'first.o2r', output / 'second.o2r'
    for path in (first, second):
        run(sys.executable, '-B', 'scripts/assets/build_chateau_romani_3ds.py', '--output', path)
    assert first.read_bytes() == second.read_bytes(), 'Archive rebuild is not deterministic'
    run(sys.executable, '-B', 'tools/chateau_romani_3ds/verify.py', '--archive', first)
    binary = output / 'shimmer'
    run(*shlex.split(os.environ.get('CXX', 'c++')), '-std=c++20', '-I' + str(ROOT),
        'tests/nei_gi/chateau_shimmer_test.cpp', '-o', binary)
    run(binary)
    tutorial = output / 'tutorial'
    run(*shlex.split(os.environ.get('CXX', 'c++')), '-std=c++20', '-I' + str(ROOT),
        'tests/item_receipts/chateau_tutorial_test.cpp', '-o', tutorial)
    run(tutorial)
    run(sys.executable, '-B', 'scripts/diagnostics/run_chateau_3ds_draw_tests.py')
    run(sys.executable, '-B', 'scripts/diagnostics/run_mask_shimmer_tests.py', '--sanitize')
print('PASS Chateau resources, palettes, tutorial receipt, host routing and both native GBI boundaries')
