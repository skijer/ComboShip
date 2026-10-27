#!/usr/bin/env python3
"""Real host collision + exact native door recognition; absent blocker is RED."""
from pathlib import Path
import os, re, subprocess, sys, tempfile
ROOT = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(ROOT/'tools/black_market'), str(ROOT/'scripts/diagnostics')]
import native_market
from run_time_pedestal_tests import block_from

with tempfile.TemporaryDirectory(prefix='market-door-collision-') as directory:
    work=Path(directory)
    room=work/'room.bin'
    room.write_bytes(native_market.read_bundle(ROOT/'tools/native_market/bundled')[native_market.COLLISION])
    for host in ((os.environ['DOOR_TEST_HOST'],) if 'DOOR_TEST_HOST' in os.environ else ('mm','soh')):
        oot=host=='soh'
        paths=('.',host,host+'/include',host+'/include/PR',host+'/src',host+'/assets',
               host+('/soh' if oot else '/2s2h'),host+('/soh' if oot else '/2s2h')+'/Enhancements/Gameplay','libultraship/include','combo')
        flags=['-w','-DCVAR_PREFIX_ENHANCEMENT="gEnhancements"','-DCVAR_PREFIX_SETTING="gSettings"','-DLOG_LEVEL_GAME_PRINTS=6','-DCOMBO_BUILD','-DF3DEX_GBI_2','-g','-O1','-ffunction-sections','-fdata-sections',
               '-fsanitize=address,undefined','-fno-sanitize=shift','-fno-sanitize-recover=all',
               '-fno-omit-frame-pointer',*(['-DDOOR_COLLISION_OOT'] if oot else []),
               *['-I'+str(ROOT/p) for p in paths]]
        source=ROOT/(host+'/soh/Enhancements/Gameplay/ComboBlackMarketNative.cpp' if oot else
                     host+'/2s2h/Enhancements/Gameplay/BlackMarket.cpp')
        text=(subprocess.check_output(['git','show',os.environ['DOOR_SOURCE_REF']+':'+str(source.relative_to(ROOT))],cwd=ROOT,text=True) if 'DOOR_SOURCE_REF' in os.environ else source.read_text())
        production=(block_from(text,text.index('void SyncShopDoorCollision(')) if 'void SyncShopDoorCollision(' in text
                    else 'void SyncShopDoorCollision() {}')
        fixture=(ROOT/'tests/black_market/door_collision_native_test.cpp').read_text().replace('/* PRODUCTION SYNC */',production)
        cpp=work/(host+'.cpp'); cpp.write_text(fixture)
        names=['z_bgcheck.c','sys_math3d.c','z_lib.c','z_skin_matrix.c','z_bg_item.c']
        sources=[ROOT/host/'src/code'/name for name in names]
        if not oot:
            sources.append(ROOT/'mm/src/code/stubs.c')
        else:
            sources += [ROOT/'soh/src/libultra/gu'/n for n in ('sins.c','coss.c')]
        objects=[]
        for index,path in enumerate(sources):
            obj=work/f'{host}-{index}.o'
            subprocess.run([os.environ.get('CC','cc'),'-std=gnu17',*flags,'-include','global.h','-c',str(path),'-o',str(obj)],check=True)
            objects.append(str(obj))
        binary=work/host
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++20',*flags,str(cpp),*objects,
                        '-Wl,--gc-sections','-fsanitize=address,undefined','-no-pie','-lm','-o',str(binary)],check=True)
        subprocess.run([str(binary),str(room)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
