#!/usr/bin/env python3
"""Compile the whole production dungeon-mask module with engine boundary fixtures."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    module = ROOT / 'soh/mods/transformation_masks/mask_progression.cpp'
    assert module.is_file(), 'Child-dungeon mask reward implementation is missing'
    code = re.sub(r'^#include[^\n]*\n', '', module.read_text(), flags=re.M)
    fixture = (ROOT / 'tests/mask_transformations/progression_test.cpp').read_text()
    header = (ROOT / 'soh/include/z64save.h').read_text()
    flags = []
    for name in ('DEKU_TREE', 'DODONGOS_CAVERN', 'JABU_JABUS_BELLY'):
        define = re.search(r'^#define EVENTCHKINF_USED_' + name + r'_BLUE_WARP .*$', header, re.M)
        assert define, name
        flags.append(define.group(0))
    fixture = fixture.replace('/* PRODUCTION_DUNGEON_FLAGS */', '\n'.join(flags))
    fixture = fixture.replace('/* PRODUCTION_PROGRESSION */', code)
    with tempfile.TemporaryDirectory(prefix='mask-progression-') as folder:
        p = Path(folder)
        (p / 'progression.cpp').write_text(fixture)
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=gnu++20', '-Wall', '-Wextra', '-Werror',
                        '-I' + str(ROOT / 'soh/include'), '-I' + str(ROOT / 'soh'),
                        str(p / 'progression.cpp'), '-o', str(p / 'progression')], check=True)
        subprocess.run([str(p / 'progression')], check=True)


if __name__ == '__main__':
    main()
