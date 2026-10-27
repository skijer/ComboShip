#!/usr/bin/env python3
"""Behavior gate for solo modes. Supply the project's third-party headers normally."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile

ROOT=Path(__file__).resolve().parents[2]
HERE=Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='solo-modes-') as temp:
    directory=Path(temp)
    for combined in (False,True):
        binary=directory/('combo-mode' if combined else 'native-mode')
        command=[os.environ.get('CC','cc'),'-std=c11','-Wall','-Wextra','-Werror','-I'+str(ROOT)]
        if combined:command+=['-DCOMBO_BUILD']
        subprocess.run([*command,str(HERE/'mode_test.c'),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True,cwd=directory)
    binary=directory/'launch-options'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I'+str(ROOT),
                    str(HERE/'launch_options_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,cwd=directory)
    print('CLI selection, argument preservation and preference persistence PASS',flush=True)

names=[p for p in sorted(HERE.glob('*_test.py')) if 'syntax' not in p.name]
for test in names:
    print('RUN '+test.name,flush=True)
    subprocess.run([sys.executable,'-B',str(test)],check=True,cwd=ROOT,timeout=180)
print(f'Solo game behavior gate PASS: 3 compiled mode/options probes and {len(names)} production-boundary suites')
