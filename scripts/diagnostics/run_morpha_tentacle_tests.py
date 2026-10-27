#!/usr/bin/env python3
"""Exercise the shared Morpha tentacle using both hosts' real engine/GBI headers.

Matrix conversion and resource availability are fixture boundaries; no ROM,
archive replacement, graphics interpreter or in-game visual proof is claimed.
"""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
from run_mm_scene_randomization_tests import function

ROOT = Path(__file__).resolve().parents[2]
flags = ['-std=gnu++20', '-DF3DEX_GBI_2', '-DCOMBO_BUILD', '-DLOG_LEVEL_GAME_PRINTS=0',
         '-DCONTROLLERBUTTONS_T=uint32_t', '-DNON_EQUIVALENT', '-DNON_MATCHING']
if '--sanitize' in sys.argv:
    flags += ['-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
              '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
with tempfile.TemporaryDirectory(prefix='morpha-tentacle-') as temporary:
    for game in ['soh', 'mm']:
        includes = [game, game+'/include', game+'/include/PR', game+'/src', game+'/assets',
                    game+'/2s2h', game+'/soh', 'libultraship/include', 'libultraship/src', 'combo/menu', 'soh', 'soh/assets']
        binary = Path(temporary) / game
        if game == 'soh':
            draw = (ROOT / 'soh/soh/Enhancements/randomizer/draw.cpp').read_text()
            integration = '\n'.join(function(draw, name) for name in ['DrawMorpha', 'Randomizer_DrawBossSoul'])
        else:
            draw = (ROOT / 'mm/2s2h/Rando/DrawFuncs.cpp').read_text()
            foreign = (ROOT / 'combo/menu/ComboForeignDrawMM.h').read_text()
            integration = '\n'.join(function(draw, name) for name in ['DrawSoulFlame', 'DrawOotSoulFlame'])
            start=foreign.index('#define MM_FOREIGN_PIN_XLU')
            end=foreign.index('// Restore the segments',start)
            integration += '\n'+foreign[start:end]+function(foreign,'MM_RestoreForeignSegs')
            integration += '\n' + function(foreign, 'MM_DrawForeignMorphaSoul')
        source = Path(temporary) / (game + '.cpp')
        source.write_text((ROOT / 'tests/morpha_tentacle/presentation_test.cpp').read_text().replace(
            '/* PRODUCTION_INTEGRATION */', integration))
        subprocess.run([os.environ.get('CXX', 'c++'), *flags,
                        *(['-DHOST_MM'] if game == 'mm' else []),
                        *['-I'+str(ROOT / p) for p in includes],
                        str(source), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
