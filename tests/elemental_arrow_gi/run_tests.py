"""Compile the sampled animation and actual native renderer/GBI boundary."""
from pathlib import Path
import os, re, subprocess, sys, tempfile
ROOT=Path(__file__).resolve().parents[2]
subprocess.run([sys.executable,'-B',str(ROOT/'tests/elemental_arrow_gi/rainbow_test.py')],check=True)
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_time_pedestal_tests import functions
from run_mm_nei_tests import flags as mm_flags, write_graph_helpers
with tempfile.TemporaryDirectory(prefix='arrow-gi-') as folder:
    temp=Path(folder)
    graph=functions((ROOT/'soh/src/code/graph.c').read_text())
    (temp/'nei_gi_graph.inc').write_text(graph['Graph_OpenDisps']+'\n'+graph['Graph_CloseDisps'])
    (temp/'nei_gi_dispatch.inc').write_text('')
    flags=['-std=c++20','-DF3DEX_GBI_2','-DLOG_LEVEL_GAME_PRINTS=0']
    flags+=['-I'+str(ROOT/p) for p in ('soh','soh/include','soh/src','soh/assets','soh/mods','libultraship/include','combo/menu')]
    flags+=['-I'+str(temp)]
    for config in ('CMake/soh-cvars.cmake','CMake/lus-cvars.cmake'):
        for key,value in re.findall(r'set\((CVAR_PREFIX_\w+)\s+"?([^\s"\)]+)',(ROOT/config).read_text()):
            flags.append(f'-D{key}="{value}"')
    for test in ('policy','polish','colors','renderer'):
        binary=temp/test
        subprocess.run([os.environ.get('CXX','c++'),*flags,'-O2','-ffunction-sections','-fdata-sections',str(ROOT/'tests/elemental_arrow_gi'/f'{test}_test.cpp'),'-Wl,--gc-sections','-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)
    write_graph_helpers(folder)
    (temp/'mm_song_draw.inc').write_text(functions((ROOT/'mm/2s2h/Rando/DrawItem.cpp').read_text())['DrawSong'])
    native=mm_flags()+['-I'+str(temp),'-O2','-ffast-math','-ffunction-sections','-fdata-sections']
    objects=[]
    for source in ('NeiGiPresentation.cpp','NeiResourceRouting.cpp'):
        obj=temp/(source+'.o')
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++20',*native,'-c',str(ROOT/'mm/2s2h/Rando'/source),'-o',str(obj)],check=True)
        objects.append(str(obj))
    binary=temp/'mm-renderer'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20',*native,str(ROOT/'tests/elemental_arrow_gi/mm_renderer_test.cpp'),*objects,
                    '-Wl,--gc-sections','-Wl,--export-dynamic-symbol=OOT_NeiResourceExists','-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
