#!/usr/bin/env python3
"""Exercise production dungeon palettes, MM export and OoT resolve/cache/draw.

Resource/UI/graphics services are modeled; this is not a full game build.
"""
import os
import re
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, name):
    match = re.search(r'^[A-Za-z_][^\n;{}]*\b' + re.escape(name) + r'\([^;{}]*\)\s*\{', source, re.M)
    if match is None:
        raise ValueError('Missing production definition: ' + name)
    start = match.start()
    brace = source.index('{', start)
    depth, end = 1, brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


native = (ROOT / 'mm/src/code/z_draw.c').read_text()
mm = (ROOT / 'combo/menu/ComboItemDrawMM.h').read_text()
oot = (ROOT / 'combo/menu/ComboForeignDrawOOT.h').read_text()
owner = (ROOT / 'mm/2s2h/Rando/DungeonItemVisuals.h').read_text()
owner = owner[owner.index('static inline int'):owner.rindex('#endif')]
owner_sword = (ROOT / 'combo/menu/ComboItemDrawOOT.h').read_text()
master_case = owner_sword[owner_sword.index('        case RG_TRUE_MASTER_SWORD:',owner_sword.index('static int32_t OOT_DescribeCustomDraw')):
                          owner_sword.index('        case RG_DOUBLE_DEFENSE:',owner_sword.index('static int32_t OOT_DescribeCustomDraw'))]
parts = {
    'SPIN': function((ROOT / 'mm/2s2h/Rando/SpinAttackGi.h').read_text(), 'MM_FillSpinAttackGi'),
    'OWNER': owner,
    'OPS': '\n'.join(function(mm, n) for n in ['MM_Op', 'MM_OpV', 'MM_OpColor', 'MM_OpDL']),
    'CROSS': '\n'.join(function(mm, n) for n in ['MM_FillDungeonKeyModelInfo', 'MM_FillDungeonTintInfo']),
    'OWNER_SWORD': '\n'.join(function(owner_sword,n) for n in ['CwSimple','CwCustomGi','CwAltSwordGi']),
    'OWNER_MASTER_CASE': master_case,
    'PRODUCER': '\n'.join(function(mm, n) for n in ['MM_IsProgressiveItem', 'MM_IsStateDependentDraw',
                                                   'MM_FillImportedSwordFallback', 'MM_IsSwordAppearanceDependent',
                                                   'MM_FillElementalSpellDrawInfo', 'MM_FillItemDrawInfo', 'MM_GetItemDrawInfo']),
    'CACHE': oot[oot.index('namespace {'):oot.index('} // namespace') + len('} // namespace')],
    'HOST': function(oot, 'OOT_DrawForeignOps'),
}
collectible = function((ROOT / 'combo/menu/ComboCollectibleShimmerHost.h').read_text(),
                       'ComboCollectibleShimmer_ProfileForDrawId')
# This bounded dungeon fixture models only key/bottle draw IDs. Give the real
# collectible classifier distinct IDs outside those rows, as with other
# out-of-fixture item families, and retain the shared integer shimmer modes.
collectible_ids = sorted(set(re.findall(r'GID_\w+', collectible)))
parts['PRODUCER'] = ('#include "ComboCollectibleShimmer.h"\nenum {' +
                     ','.join(name + '=' + str(1000+i) for i, name in enumerate(collectible_ids)) +
                     '};\n#define COMBO_COLLECTIBLE_SHIMMER_HOST_MM\n' + collectible +
                     '\n#undef COMBO_COLLECTIBLE_SHIMMER_HOST_MM\n' + parts['PRODUCER'])
native_names = ['GetItem_EmptyBottleShell', 'GetItem_FairyBottleShell', 'GetItem_GetDungeonItemTint', 'GetItem_GetDungeonKeyEmblemTint',
                'GetItem_GetDungeonKeyModel', 'GetItem_TryDrawDungeonKey', 'GetItem_DrawDungeonItem', 'GetItem_DrawOpa0',
                'GetItem_DrawOpa0Xlu1', 'GetItem_DrawCompass']
fixture = ROOT / 'tests/dungeon_keys'
flags = ['-I' + str(ROOT), '-Wall', '-Wextra', '-Werror', '-Wno-unused-function', '-Wno-unused-parameter', '-Wno-unused-variable']
if '--sanitize' in sys.argv:
    flags += ['-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer',
              '-fno-pie', '-no-pie']
with tempfile.TemporaryDirectory(prefix='mm-dungeon-keys-') as td:
    build = Path(td)
    (build / 'fixture_api.h').write_text((fixture / 'fixture_api.h').read_text())
    source = (fixture / 'native_test.c').read_text().replace(
        '/* PRODUCTION_NATIVE */', '\n'.join(function(native, n) for n in native_names))
    (build / 'native.c').write_text(source)
    cc, cxx = os.environ.get('CC', 'cc'), os.environ.get('CXX', 'c++')
    subprocess.run([cc, '-std=c11', *flags, str(build / 'native.c'), '-o', str(build / 'native')], check=True)
    subprocess.run([str(build / 'native')], check=True)
    subprocess.run([cc, '-std=c11', *flags, '-DTEST_NO_MAIN', '-c', str(build / 'native.c'),
                    '-o', str(build / 'native.o')], check=True)
    source = (fixture / 'cache_test.cpp').read_text()
    for name, body in parts.items():
        source = source.replace('/* PRODUCTION_' + name + ' */', body)
    (build / 'cache.cpp').write_text(source)
    subprocess.run([cxx, '-std=c++20', *flags, '-I' + str(ROOT / 'combo/menu'), str(build / 'cache.cpp'),
                    str(build / 'native.o'), '-o', str(build / 'cache')], check=True)
    subprocess.run([str(build / 'cache')], check=True)
