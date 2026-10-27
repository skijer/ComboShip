#!/usr/bin/env python3
"""Check the core's CPU pose against the native segment-01 billboard contract."""
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_mm_scene_randomization_tests import function

def main():
    glow = function((ROOT / 'combo/menu/ComboFairyBottleDraw.h').read_text(), 'ComboFairyBottle_GlowLimb')
    with tempfile.TemporaryDirectory(prefix='fairy-core-pose-') as temporary:
        build = Path(temporary)
        for host in ('soh', 'mm'):
            matrix = function((ROOT / host / 'src/code/sys_matrix.c').read_text(), 'Matrix_ReplaceRotation')
            (build / 'glow_production.inc').write_text(matrix + '\n' + glow)
            binary = build / host
            flags = ['-DCOMBO_FAIRY_HOST_MM'] if host == 'mm' else []
            subprocess.run([*shlex.split(os.environ.get('CXX', 'c++')), '-std=c++20', '-Wall', '-Wextra',
                            '-Werror', '-Wno-unused-parameter', '-Wno-missing-field-initializers', *flags, '-I'+str(build),
                            str(ROOT / 'tests/fairy_bottle/glow_pose_test.cpp'), '-o', str(binary)], check=True)
            print('CORE HOST ' + host, flush=True)
            subprocess.run([str(binary)], check=True)

if __name__ == '__main__':
    main()
