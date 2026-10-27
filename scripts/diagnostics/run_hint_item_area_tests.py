#!/usr/bin/env python3
"""Exercise the production combo hint-area payload setter with real nlohmann JSON.

The setter and its storage declaration are extracted unchanged. Only warning
delivery is substituted, so payload lifetime, filtering and replacement are real.
"""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

from run_hint_item_name_tests import function

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root', type=Path, default=ROOT)
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    source = (args.source_root.resolve() /
              'soh/soh/Enhancements/randomizer/3drando/hints.cpp').read_text()
    storage = re.search(r'^static std::unordered_map<[^;]+> sComboItemAreas;', source, re.M)
    if storage is None:
        raise RuntimeError('Production hint-area storage declaration was not found')
    production = storage[0] + '\n' + function(source, 'void Combo_SetHintItemAreas(')
    flags = ['-std=c++20', '-O1', '-g', '-Wall', '-Wextra', '-fno-pie', '-no-pie']
    if args.sanitize:
        flags += ['-fsanitize=address,undefined', '-fsanitize-address-use-after-scope',
                  '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
    with tempfile.TemporaryDirectory(prefix='hint-item-areas-') as temporary:
        folder = Path(temporary)
        (folder / 'hint_item_area_production.inc').write_text(production)
        binary = folder / 'test'
        subprocess.run([*shlex.split(os.environ.get('CXX', 'c++')), *flags,
                        '-I' + str(folder), str(ROOT / 'soh/tests/hint_item_area_test.cpp'),
                        '-o', str(binary)], check=True)
        result = subprocess.run([str(binary)], env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
        print(f'hint_item_areas: returncode={result.returncode}', flush=True)
        raise SystemExit(result.returncode)


if __name__ == '__main__':
    main()
