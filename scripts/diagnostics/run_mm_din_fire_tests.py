#!/usr/bin/env python3
"""Compile the real MM Din modules with production headers/GBI and engine boundaries."""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
FLAGS = ['-std=gnu11', '-O1', '-g', '-DF3DEX_GBI_2', '-DCOMBO_BUILD', '-DMM_BUILD_DLL',
         '-DCONTROLLERBUTTONS_T=uint32_t', '-DNON_EQUIVALENT', '-DNON_MATCHING', '-DNDEBUG',
         '-DLOG_LEVEL_GAME_PRINTS=0', '-Wno-incompatible-pointer-types', '-Wno-int-conversion',
         '-Wno-discarded-qualifiers', '-Werror=implicit-function-declaration']
FLAGS += ['-I' + str(ROOT / p) for p in ('mm/include', 'mm/include/PR', 'mm/src', 'mm', 'mm/tests',
          'mm/2s2h', 'mm/assets', 'libultraship/include', 'libultraship/src', 'combo')]

def run(*args):
    subprocess.run(args, cwd=ROOT, check=True)

def main():
    with tempfile.TemporaryDirectory(prefix='mm-din-fire-') as directory:
        binary = str(Path(directory) / 'render')
        run(os.environ.get('CC', 'cc'), *FLAGS, 'mm/tests/din_fire_render_test.c',
            'mm/src/code/din_fire_sword.c', 'mm/src/code/din_fire_shield.c', '-lm', '-o', binary)
        run(binary)

if __name__ == '__main__':
    main()
