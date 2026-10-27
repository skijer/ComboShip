#!/usr/bin/env python3
"""Execute complete native MM and OoT Wolf update/combat at the audio boundary."""
import argparse
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_mm_nei_tests import flags as mm_flags
sys.path.insert(0, str(ROOT / 'tests/nei_held'))
from run_articulated_tests import flags as oot_flags


def run(command):
    result = subprocess.run(command, capture_output=True, text=True)
    if result.returncode:
        print(result.stdout + result.stderr)
        raise SystemExit(result.returncode)
    if result.stdout:
        print(result.stdout, end='')


parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--host', choices=('mm', 'soh'))
parser.add_argument('cases', nargs='*')
args = parser.parse_args()
if any(case not in ('voice', 'chain', 'transform', 'mixer', 'defaults', 'dash', 'exertion') for case in args.cases):
    parser.error('cases must be voice, chain, transform, mixer, defaults, dash or exertion')

with tempfile.TemporaryDirectory(prefix='wolf-audio-') as directory:
    build = Path(directory)
    for host, options in [('mm', mm_flags()), ('soh', oot_flags()[1:])]:
        if args.host and host != args.host:
            continue
        mode = ['-O1', '-g', '-fsanitize=undefined,float-cast-overflow', '-fno-sanitize-recover=all',
                '-ffunction-sections', '-fdata-sections']
        obj = str(build / f'{host}-math.o')
        run([os.environ.get('CC', 'cc'), '-std=gnu11', *options, *mode,
             '-include', str(ROOT / host / 'include/functions.h'),
             '-include', str(ROOT / host / 'include/variables.h'),
             '-c', str(ROOT / host / 'src/code/z_lib.c'), '-o', obj])
        binary = str(build / f'{host}-audio')
        run([os.environ.get('CXX', 'c++'), '-std=c++20', *options, *mode,
             *(['-DWOLF_AUDIO_MM'] if host == 'mm' else []), '-DFMT_HEADER_ONLY',
             '-DWOLF_IMPLEMENTATION="' + str(ROOT / host / 'mods/transformation_masks/wolf_link_form.cpp') + '"',
             str(ROOT / 'tests/mm_wolf/audio_runtime_test.cpp'),
             *([str(ROOT / 'soh/mods/transformation_masks/wolf_link_audio_hooks.cpp')] if host == 'soh' else []),
             obj, '-pthread', '-Wl,--gc-sections', '-o', binary])
        cases = args.cases or ['voice', 'chain', 'transform', 'mixer', 'defaults'] + (['dash', 'exertion'] if host == 'mm' else [])
        for case in cases:
            run([binary, case])
