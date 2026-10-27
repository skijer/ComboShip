#!/usr/bin/env python3
"""Execute native MM NEI drawers with real MM structures and graphics ABI."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT/'tests/mm_nei'))
from rig_fit_query import write_query
def flags():
    # z_player.c's unity build gets the audio aliases through extended_player.c's
    # SoH redirect headers before equip_helper.c. Standalone fixtures need them too.
    return ['-DPLAYER_STATE1_LOADING=PLAYER_STATE1_200','-DPLAYER_STATE1_IN_ITEM_CS=0u','-DPLAYER_STATE1_GETTING_ITEM=PLAYER_STATE1_400','-DPLAYER_STATE1_DAMAGED=PLAYER_STATE1_4000000','-DPLAYER_STATE1_HANGING_OFF_LEDGE=0u','-DPLAYER_STATE1_CLIMBING_LEDGE=PLAYER_STATE1_4','-DF3DEX_GBI_2','-DLOG_LEVEL_GAME_PRINTS=0',*[f'-I{ROOT/p}' for p in ('mm','mm/2s2h','mm/include','mm/include/PR','mm/src','mm/assets','mm/mods','libultraship/include','combo','combo/menu')],'-include',str(ROOT/'mm/include/z64.h'),'-include',str(ROOT/'mm/mods/nei_oot_compat.h'),'-include',str(ROOT/'mm/soh/_nei_compat_core.h')]
def write_graph_helpers(directory):
    # MM_REAL_RENDERER fixtures execute the native marker allocations too.
    from run_time_pedestal_tests import functions
    graph=functions((ROOT/'mm/src/code/graph.c').read_text())
    (Path(directory)/'mm_nei_graph.inc').write_text(graph['Graph_OpenDisps']+'\n'+graph['Graph_CloseDisps'])
if __name__=='__main__':
    with tempfile.TemporaryDirectory(prefix='mm-nei-') as td:
        sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
        write_graph_helpers(td)
        from run_time_pedestal_tests import functions
        (Path(td)/'mm_song_draw.inc').write_text(functions((ROOT/'mm/2s2h/Rando/DrawItem.cpp').read_text())['DrawSong'])
        query=Path(td)/'rig_fit_query.cpp'
        write_query(ROOT, query)
        objects=[]
        sources=['objects/object_firerod.c','objects/object_icerod.c','objects/object_lightrod.c']
        helper=(ROOT/'mm/mods/items/helpers/equip_helper.c').read_text()
        if 'ItemEquip_CaptureLeftHandMatrix' in helper: sources.append('helpers/equip_helper.c')
        for source in sources:
            obj=str(Path(td)/(Path(source).stem+'.o'))
            subprocess.run([os.environ.get('CC','cc'),'-std=gnu2x',*flags(),'-Wno-implicit-function-declaration','-Wno-int-conversion','-ffunction-sections','-fdata-sections','-c',str(ROOT/'mm/mods/items'/source),'-o',obj],check=True)
            objects.append(obj)
        binary=str(Path(td)/'rod-runtime')
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++20',*flags(),'-ffunction-sections','-fdata-sections',str(ROOT/'tests/mm_nei/rod_runtime_test.cpp'),str(query),*objects,'-Wl,--gc-sections','-o',binary],check=True)
        subprocess.run([binary],check=True)

        native=[]
        for source in ['NeiGiPresentation.cpp','NeiHeldPresentation.cpp','NeiUsedMagicPresentation.cpp','NeiResourceRouting.cpp','NeiLanternPresentation.cpp']:
            obj=str(Path(td)/(source+'.o'))
            subprocess.run([os.environ.get('CXX','c++'),'-std=c++20',*flags(),'-ffunction-sections','-fdata-sections','-c',str(ROOT/'mm/2s2h/Rando'/source),'-o',obj],check=True)
            native.append(obj)
        binary=str(Path(td)/'renderer-runtime')
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++20',*flags(),'-I'+td,'-ffunction-sections','-fdata-sections',str(ROOT/'tests/mm_nei/renderer_runtime_test.cpp'),str(query),*native,'-Wl,--gc-sections','-Wl,--export-dynamic-symbol=OOT_NeiResourceExists','-Wl,--export-dynamic-symbol=OOT_GetNeiGiDrawInfo','-Wl,--export-dynamic-symbol=OOT_GetNeiGiDrawInfoForAssets','-o',binary],check=True)
        subprocess.run([binary],check=True)

    subprocess.run([sys.executable,'-B',str(ROOT/'tests/mm_nei/run_dispatch_tests.py')],check=True)
    subprocess.run([sys.executable,'-B',str(ROOT/'tests/mm_nei/run_interpolation_tests.py')],check=True)

    subprocess.run([sys.executable,'-B',str(ROOT/'tests/mm_nei/run_held_tests.py')],check=True)
    subprocess.run([sys.executable,'-B',str(ROOT/'tests/mm_nei/run_hand_selector_tests.py')],check=True)

    subprocess.run([sys.executable,'-B',str(ROOT/'tests/mm_nei/run_use_tests.py')],check=True)
    subprocess.run([sys.executable,'-B',str(ROOT/'tests/mm_nei/run_ice_projectile_tests.py')],check=True)
    subprocess.run([sys.executable,'-B',str(ROOT/'tests/mm_nei/run_action_tests.py')],check=True)
