#!/usr/bin/env python3
"""Run the real launcher reload callback against isolated native-state fixtures."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--json-include', type=Path, default=ROOT.parent / 'build-deps/usr/include')
parser.add_argument('--source-rev', help='Extract a prior launcher revision to demonstrate the reload regression')
args = parser.parse_args()
source = (subprocess.check_output(['git', 'show', f'{args.source_rev}:combo/ComboShip.cpp'], cwd=ROOT,
                                  env={**os.environ, 'GIT_NO_LAZY_FETCH': '1'}).decode()
          if args.source_rev else (ROOT / 'combo/ComboShip.cpp').read_text())
begin = source.index('static int Combo_OnReloadRequest(const char* path) {')
end = source.index('\n}', begin) + 2
with tempfile.TemporaryDirectory(prefix='cross_dungeon_reload_') as tmp:
    directory = Path(tmp)
    (directory / 'combo_reload_function.h').write_text(source[begin:end])
    binary = directory / 'combo_reload_test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-Wall', '-Wextra', '-Werror', '-O0',
                    '-I', str(ROOT / 'combo'), '-I', str(args.json_include), '-I', str(directory),
                    str(ROOT / 'combo/tests/combo_cross_dungeon_reload_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], cwd=directory, check=True)
