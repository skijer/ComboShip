#!/usr/bin/env python3
"""Run both real market presenters, native title update/draw and native GBI.

Resource availability, GPU setup, interpolation recording and default cosmetics
are boundaries. Native pointer framing, IA8 tiles, rectangles and fade commands
execute; these checks do not claim configured game rendering or art acceptance.
"""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_time_pedestal_tests import block_from


def function(source, signature):
    return block_from(source, source.index(signature))


def fixture(host):
    oot = host == 'soh'
    path = ('soh/soh/Enhancements/Gameplay/ComboBlackMarketNative.cpp' if oot else
            'mm/2s2h/Enhancements/Gameplay/BlackMarket.cpp')
    market = (ROOT / path).read_text()
    actor = (ROOT / host / 'src/code/z_actor.c').read_text()
    graph = (ROOT / host / 'src/code/graph.c').read_text()
    library = (ROOT / host / 'src/code/z_lib.c').read_text()
    native = function(library, 's32 Math_StepToS(')
    native += '\n' + '\n'.join(function(graph, 'void ' + name + '(') for name in ('Graph_OpenDisps', 'Graph_CloseDisps'))
    native += '\n' + '\n'.join(function(actor, 'void ' + name + '(') for name in ('TitleCard_Update', 'TitleCard_Draw'))
    scope = ''
    helper = ''
    if oot:
        boot = (ROOT / 'soh/src/code/title_setup.c').read_text()
        scope = re.search(r'static s32 sComboCustomTitleCardPresentation[^;]*;', boot)[0]
        scope += '\n' + function(boot, 'int Combo_SOH_SwapCustomTitleCardPresentation(')
    else:
        native += '\n' + function(actor, 'void TitleCard_InitBossName(')
        helper_source = (ROOT / 'mm/2s2h/Enhancements/Graphics/ComboOpeningForestTitleCard.cpp').read_text()
        helper = function(helper_source, 'extern "C" int ComboPlaceTitleCard_Show(')
    source = (ROOT / 'tests/native_market/title_draw_test.cpp').read_text()
    source = source.replace('/* CARD_RESOURCE */', re.search(r'alignas\(2\) constexpr char kCard\[\][^;]*;', market)[0])
    source = source.replace('/* NATIVE_PRESENTATION_SCOPE */', scope)
    source = source.replace('/* NATIVE_TITLE_CARD */', native + '\n' + helper)
    return source.replace('/* MARKET_PRESENTER */', function(market, 'void PresentTitle('))


def main():
    with tempfile.TemporaryDirectory(prefix='native-market-draw-') as directory:
        for host in ('mm', 'soh'):
            source, binary = Path(directory) / (host + '.cpp'), Path(directory) / host
            source.write_text(fixture(host))
            includes = [host, host + '/include', host + '/include/PR', host + '/src', host + '/assets',
                        host + ('/2s2h' if host == 'mm' else '/soh'), 'libultraship/include', 'combo']
            flags = ['-std=c++20', '-w', '-DCOMBO_BUILD', '-DF3DEX_GBI_2', '-DFMT_HEADER_ONLY', '-g',
                     '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-no-pie']
            if host == 'soh':
                flags += ['-DOOT_TITLE_DRAW_TEST']
            subprocess.run([os.environ.get('CXX', 'c++'), *flags, *['-I' + str(ROOT / path) for path in includes],
                            str(source), '-o', str(binary)], check=True)
            subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})


if __name__ == '__main__':
    main()
