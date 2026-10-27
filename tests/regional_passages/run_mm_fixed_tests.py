#!/usr/bin/env python3
"""Compile actual fixed MM endpoints against native host headers; no game runtime claim."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
ROOT = Path(__file__).resolve().parents[2]
paths = ['mm','mm/2s2h','mm/include','mm/include/PR','mm/src','mm/assets','libultraship/include','combo','combo/menu','mm/2s2h/Enhancements/Gameplay']
env = dict(os.environ)
if env.get('COMBO_NEI_FIXTURE_CPATH'):
    env['CPATH'] = os.pathsep.join((env['COMBO_NEI_FIXTURE_CPATH'], env.get('CPATH','')))
with tempfile.TemporaryDirectory(prefix='fixed-mm-regional-') as temporary:
    cpp, binary = Path(temporary)/'test.cpp', Path(temporary)/'test'
    player_source=(ROOT/'mm/src/overlays/actors/ovl_player_actor/z_player.c').read_text()
    begin=player_source.index('    startMode = PLAYER_GET_START_MODE(&this->actor);')
    end=player_source.index('    sStartModeFuncs[startMode](play, this);',begin)
    native_start='static s32 NativeStartMode(Player* player) {\n s32 startMode;\n'+player_source[begin:end].replace('this->','player->')+'\n return startMode;\n}\n'
    cpp.write_text((ROOT/'tests/regional_passages/mm_native_test.cpp').read_text().replace('/* PRODUCTION */', (ROOT/'mm/2s2h/Enhancements/Gameplay/ComboRegionalPassages.cpp').read_text()).replace('/* NATIVE_START */',native_start))
    command = [os.environ.get('CXX','c++'),'-std=c++20','-w','-DF3DEX_GBI_2','-DCOMBO_BUILD','-DCOMBO_MM_NATIVE_START_TEST','-DFMT_HEADER_ONLY','-ffunction-sections','-fdata-sections','-Wl,--gc-sections', *['-I'+str(ROOT/p) for p in paths]]
    if '--sanitize' in sys.argv:
        command += ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-no-pie']
    subprocess.run([*command,str(cpp),'-o',str(binary)], check=True, env=env)
    subprocess.run([str(binary)],check=True,env={**env,'ASAN_OPTIONS':'detect_leaks=0'})
