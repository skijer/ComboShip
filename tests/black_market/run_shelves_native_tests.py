#!/usr/bin/env python3
"""Run shelf GBI/collision code using each engine's actual registration functions."""
from pathlib import Path
import os,subprocess,sys,tempfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_time_pedestal_tests import block_from
with tempfile.TemporaryDirectory(prefix='shelves-native-') as temporary:
    for host in ('mm','soh'):
        collision=(ROOT/host/'src/code/z_bgcheck.c').read_text()
        actor=(ROOT/host/'src/code/z_bg_item.c').read_text()
        camera_index='SurfaceType_GetBgCamIndex' if host=='mm' else 'SurfaceType_GetCamDataIndex'
        bodies='\n'.join([block_from(collision,collision.index(s)) for s in (
            'void BgActor_SetActor(', 's32 DynaPoly_SetBgActor(',
            'CollisionHeader* BgCheck_GetCollisionHeader(', 'u32 SurfaceType_GetData(',
            'u32 '+camera_index+'(', 'u16 BgCheck_GetBgCamSettingImpl(')] +
            [block_from(actor,actor.index('void DynaPolyActor_Init('))])
        fixture=(ROOT/'tests/black_market/shelves_native_test.cpp').read_text().replace('/* NATIVE COLLISION */',bodies)
        fixture=fixture.replace('SHELF_CAMERA_INDEX',camera_index)
        cpp=Path(temporary)/(host+'.cpp');cpp.write_text(fixture);binary=Path(temporary)/host
        paths=[host,host+'/include',host+'/include/PR',host+'/src',host+'/assets',host+('/2s2h' if host=='mm' else '/soh'),'libultraship/include','combo']
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-w','-DCOMBO_BUILD','-DF3DEX_GBI_2','-g',
            '-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie','-I'+str(ROOT),
            *['-I'+str(ROOT/p) for p in paths],str(cpp),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
