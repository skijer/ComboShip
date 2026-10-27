#!/usr/bin/env python3
"""Verify sampled effects and their native MM renderer under game compiler flags."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_mm_nei_tests import flags, write_graph_helpers
from run_time_pedestal_tests import functions


def run(command):
    result = subprocess.run(command, cwd=ROOT, text=True, capture_output=True)
    if result.returncode:
        sys.stderr.write(result.stdout + result.stderr)
        raise SystemExit(result.returncode)
    print(result.stdout, end='')


with tempfile.TemporaryDirectory(prefix='nei-air-') as directory:
    temp = Path(directory)
    compiler = os.environ.get('CXX', 'c++')
    for optimization in ['-O2', '-Ofast']:
        binary = temp / ('policy' + optimization)
        run([compiler, '-std=c++20', optimization, '-I' + str(ROOT / 'soh'),
             str(ROOT / 'tests/nei_air_magic/policy_test.cpp'), '-o', str(binary)])
        run([str(binary)])
    write_graph_helpers(directory)
    (temp / 'mm_song_draw.inc').write_text(functions((ROOT / 'mm/2s2h/Rando/DrawItem.cpp').read_text())['DrawSong'])
    native_flags = flags() + ['-I' + directory, '-O2', '-ffast-math', '-ffunction-sections', '-fdata-sections']
    if os.environ.get('NEI_TEST_DEPENDENCIES'):
        native_flags += ['-I' + os.environ['NEI_TEST_DEPENDENCIES']]
    objects = []
    for source in ['NeiGiPresentation.cpp', 'NeiResourceRouting.cpp', 'NeiAirMagicPresentation.cpp']:
        obj = temp / (source + '.o')
        run([compiler, '-std=c++20', *native_flags, '-c', str(ROOT / 'mm/2s2h/Rando' / source), '-o', str(obj)])
        objects.append(str(obj))
    binary = temp / 'renderer'
    run([compiler, '-std=c++20', *native_flags, str(ROOT / 'tests/nei_air_magic/renderer_test.cpp'), *objects,
         '-Wl,--gc-sections', '-Wl,--export-dynamic-symbol=OOT_NeiResourceExists', '-o', str(binary)])
    run([str(binary)])
