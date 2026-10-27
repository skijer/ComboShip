#!/usr/bin/env python3
"""Exercise the actual production readiness/authored-override/retry state machine."""
import os
from pathlib import Path
import subprocess
import tempfile
from run_time_pedestal_tests import functions
ROOT = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix='solo-forest-actors-') as folder:
    binary = Path(folder) / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=undefined', '-fno-sanitize-recover=all', '-Isoh',
                    'soh/tests/solo_forest_actors_test.cpp', '-o', str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True)
source = functions((ROOT / 'soh/src/overlays/actors/ovl_En_Horse/z_en_horse.c').read_text())
fixture = (ROOT / 'soh/tests/solo_forest_actors_test.c').read_text().replace(
    '/* PRODUCTION_HOST */', '\n'.join(source[name] for name in ('EnHorse_ForestInit', 'EnHorse_ForestUpdate')))
with tempfile.TemporaryDirectory(prefix='solo-forest-horse-') as folder:
    cfile, binary = Path(folder) / 'test.c', Path(folder) / 'test'
    cfile.write_text(fixture)
    subprocess.run([os.environ.get('CC', 'cc'), '-std=c11', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=undefined', '-fno-sanitize-recover=all', str(cfile), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('PASS: production solo forest visit routing and stationary horse host')

source = functions((ROOT / 'soh/soh/Enhancements/Gameplay/ComboForestActors.cpp').read_text())
fixture = (ROOT / 'soh/tests/solo_forest_actors_runtime_test.cpp').read_text().replace(
    '/* PRODUCTION_ROUTING */', '\n'.join(source[name] for name in ('ForestVisit', 'Safe', 'Find', 'Spawn', 'Update', 'OpenText')))
with tempfile.TemporaryDirectory(prefix='solo-forest-routing-') as folder:
    cfile, binary = Path(folder) / 'test.cpp', Path(folder) / 'test'
    cfile.write_text(fixture)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-DCOMBO_BUILD', '-Isoh', '-Icombo',
                    '-Wall', '-Wextra', '-Werror', '-fsanitize=undefined', '-fno-sanitize-recover=all',
                    str(cfile), '-o', str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True)
print('PASS: production solo forest spawn/placement/dialogue and mode guards')
