#!/usr/bin/env python3
"""Compile the real spin GI recipe/renderer against both engines' headers/GBI.

Matrix, resource and cosmetic services are fixture boundaries; no rasterization
or model-pack runtime acceptance is implied.
"""
import os
from pathlib import Path
import re
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


recipe = function((ROOT / 'mm/2s2h/Rando/SpinAttackGi.h').read_text(), 'MM_FillSpinAttackGi')
oot_draw = (ROOT / 'soh/soh/Enhancements/randomizer/draw.cpp').read_text()
alias = '\n'.join(function(oot_draw, name) for name in ['OOT_DescribeMmSpinAttackDraw',
                                                      'Randomizer_DrawMmGreatSpinAttack'])
items = (ROOT / 'soh/soh/Enhancements/randomizer/item_list.cpp').read_text()
assert 'itemTable[RG_MM_GREAT_SPIN_ATTACK].SetCustomDrawFunc(Randomizer_DrawMmGreatSpinAttack);' in items
flags = ['-DF3DEX_GBI_2', '-DCOMBO_BUILD', '-DLOG_LEVEL_GAME_PRINTS=0',
         '-DCONTROLLERBUTTONS_T=uint32_t', '-DNON_EQUIVALENT', '-DNON_MATCHING']
if '--sanitize' in sys.argv:
    flags += ['-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
              '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
with tempfile.TemporaryDirectory(prefix='spin-gi-') as td:
    build = Path(td)
    source = (ROOT / 'tests/spin_gi/presentation_test.cpp').read_text().replace('/* PRODUCTION_RECIPE */', recipe)
    source = source.replace('/* PRODUCTION_OOT_ALIAS */', alias)
    (build / 'test.cpp').write_text(source)
    for host, game in [('mm', 'mm'), ('oot', 'soh')]:
        includes = [game, game + '/include', game + '/include/PR', game + '/src', game + '/assets', game + '/2s2h', game + '/soh', game + '/mods',
                    'libultraship/include', 'libultraship/src', 'combo/menu']
        extra = ['-DHOST_MM'] if host == 'mm' else []
        binary = build / host
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=gnu++20', *flags, *extra,
                        *['-I' + str(ROOT / p) for p in includes], str(build / 'test.cpp'), '-o', str(binary)],
                       check=True)
        env = os.environ.copy()
        env['ASAN_OPTIONS'] = env.get('ASAN_OPTIONS', '') + ':detect_leaks=0'
        subprocess.run([str(binary)], check=True, env=env)
