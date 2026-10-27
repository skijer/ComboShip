#!/usr/bin/env python3
"""Compile the actual geometry header with native Windows SDK macro exposure."""
from pathlib import Path
import argparse
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--msvc', action='store_true')
parser.add_argument('--compiler')
args = parser.parse_args()
with tempfile.TemporaryDirectory(prefix='regional-header-') as temporary:
    folder = Path(temporary)
    binary = folder / ('header.exe' if args.msvc else 'header')
    source = ROOT / 'tests/regional_passages/header_portability_test.cpp'
    if args.msvc:
        command = [args.compiler or 'cl', '/nologo', '/std:c++20', '/EHsc',
                   '/I' + str(ROOT / 'combo'), str(source), '/Fe:' + str(binary)]
    else:
        command = [args.compiler or os.environ.get('CXX', 'c++'), '-std=c++20',
                   '-Wall', '-Wextra', '-Werror', '-I' + str(ROOT / 'combo'), str(source), '-o', str(binary)]
    subprocess.run(command, cwd=folder, check=True)
    subprocess.run([str(binary)], cwd=folder, check=True)
