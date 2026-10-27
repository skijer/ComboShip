"""Run actual native Fire/Ice/Light rod drawers with the production rig query."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'scripts/diagnostics'))
from run_mm_nei_tests import ROOT, flags
from rig_fit_query import write_query

with tempfile.TemporaryDirectory(prefix='mm-nei-rod-fit-') as td:
    query = Path(td) / 'rig_fit_query.cpp'
    write_query(ROOT, query)
    objects = []
    for name in ('object_firerod.c', 'object_icerod.c', 'object_lightrod.c', 'equip_helper.c'):
        source = ROOT / 'mm/mods/items' / ('helpers' if name == 'equip_helper.c' else 'objects') / name
        obj = str(Path(td) / (name + '.o'))
        subprocess.run([os.environ.get('CC', 'cc'), '-std=gnu2x', *flags(),
                        '-Wno-implicit-function-declaration', '-Wno-int-conversion',
                        '-ffunction-sections', '-fdata-sections', '-c', str(source), '-o', obj], check=True)
        objects.append(obj)
    binary = str(Path(td) / 'rod-fit')
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', *flags(),
                    '-ffunction-sections', '-fdata-sections', str(ROOT / 'tests/mm_nei/rod_runtime_test.cpp'),
                    str(query), *objects, '-Wl,--gc-sections', '-o', binary], check=True)
    subprocess.run([binary], check=True)
