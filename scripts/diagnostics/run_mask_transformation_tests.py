#!/usr/bin/env python3
"""Compile actual mask routing against production item IDs and form enumeration."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, name):
    match = re.search(r'^.*\b' + name + r'\([^;{}]*\)\s*\{', source, re.M)
    assert match, f'Missing production function: {name}'
    start = match.start()
    end = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


def main():
    form = (ROOT / 'soh/mods/transformation_masks/mm_player_form.cpp').read_text()
    routing = (ROOT / 'soh/mods/transformation_masks/transformation_masks.c').read_text()
    header = (ROOT / 'soh/mods/transformation_masks/transformation_masks.h').read_text()
    enums = []
    for text, name in ((header, 'TransformMaskId'), (header, 'MmPlayerTransformation'), (form, 'MmFormStateId')):
        enum = re.search(r'typedef enum ' + name + r'\s*\{.*?\}\s*' + name + ';', text, re.S)
        assert enum, f'Missing production enum: {name}'
        enums.append(enum.group(0))
    crystal = re.search(r'^#define EXT_ITEM_SHADOW_CRYSTAL .*$',
                        (ROOT / 'soh/mods/extended_inventory.h').read_text(), re.M)
    assert crystal
    source = (ROOT / 'tests/mask_transformations/routing_test.cpp').read_text()
    source = source.replace('/* PRODUCTION_FORM_ENUM */', '\n'.join(enums))
    source = source.replace('/* PRODUCTION_SHADOW_CRYSTAL_ID */', crystal.group(0))
    actual = [function(form, name) for name in ('MmForm_MaskIdToForm', 'MmForm_GetMaskType', 'MmForm_IsEnabled',
                                              'MmForm_FleetApplyForm')]
    actual += [function(routing, name) for name in ('TransformMasks_GetMaskType', 'TransformMasks_IsEnabled',
                                                   'MaskShipsOutsideMmAssets', 'TransformMasks_TryFormFromItem')]
    source = source.replace('/* PRODUCTION_MASK_ROUTING */', '\n'.join(actual))
    with tempfile.TemporaryDirectory(prefix='mask-routing-') as folder:
        p = Path(folder)
        (p / 'routing.cpp').write_text(source)
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=gnu++20', '-Wall', '-Wextra', '-Werror',
                        '-I' + str(ROOT / 'soh/include'), str(p / 'routing.cpp'), '-o', str(p / 'routing')], check=True)
        subprocess.run([str(p / 'routing')], check=True)


if __name__ == '__main__':
    main()
