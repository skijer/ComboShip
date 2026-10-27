"""Focused production resource traversal and native OoT/MM renderer fixtures."""
from pathlib import Path
import os, re, subprocess, sys, tempfile
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_time_pedestal_tests import functions
from run_mm_nei_tests import flags as mm_flags, write_graph_helpers
with tempfile.TemporaryDirectory(prefix='reward-gi-') as folder:
    temp = Path(folder)
    graph = functions((ROOT/'soh/src/code/graph.c').read_text())
    (temp/'nei_gi_graph.inc').write_text(graph['Graph_OpenDisps']+'\n'+graph['Graph_CloseDisps'])
    (temp/'nei_gi_dispatch.inc').write_text('')
    recipes=functions((ROOT/'soh/src/code/z_draw.c').read_text())
    (temp/'reward_native_recipes.inc').write_text(recipes['GetItem_DrawEggOrMedallion']+'\n'+recipes['GetItem_DrawJewel'])
    flags = ['-std=c++20','-DF3DEX_GBI_2','-DLOG_LEVEL_GAME_PRINTS=0']
    flags += ['-I'+str(ROOT/p) for p in ('.','soh','soh/include','soh/src','soh/assets','soh/mods','libultraship/include','combo/menu')]
    flags += ['-I'+str(temp)]
    for config in ('CMake/soh-cvars.cmake','CMake/lus-cvars.cmake'):
        for key,value in re.findall(r'set\((CVAR_PREFIX_\w+)\s+"?([^\s"\)]+)',(ROOT/config).read_text()):
            flags.append(f'-D{key}="{value}"')
    for test in ('policy','surface','renderer','native_recipe'):
        binary = temp/test
        subprocess.run([os.environ.get('CXX','c++'),*flags,'-O2','-ffunction-sections','-fdata-sections',str(ROOT/'tests/reward_gi'/f'{test}_test.cpp'),'-Wl,--gc-sections','-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)
    write_graph_helpers(folder)
    mm_recipes=functions((ROOT/'mm/2s2h/Rando/DrawItem.cpp').read_text())
    (temp/'mm_song_draw.inc').write_text(mm_recipes['DrawSong'])
    (temp/'reward_mm_recipes.inc').write_text(mm_recipes['DrawOotMedallion']+'\n'+mm_recipes['DrawOotStone'])
    foreign_source=(ROOT/'combo/menu/ComboForeignDrawMM.h').read_text()
    foreign=functions(foreign_source)
    pins=foreign_source[foreign_source.index('#define MM_FOREIGN_PIN_OPA'):foreign_source.index('// Restore the segments')]
    (temp/'reward_foreign_jewel.inc').write_text(pins+'\n'+foreign['MM_RestoreForeignSegs']+'\n'+foreign['MM_DrawForeignJewel'])
    native=mm_flags()+['-I'+str(temp),'-O2','-ffast-math','-ffunction-sections','-fdata-sections']
    objects=[]
    for source in ('NeiGiPresentation.cpp','NeiResourceRouting.cpp'):
        obj=temp/(source+'.o')
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++20',*native,'-c',str(ROOT/'mm/2s2h/Rando'/source),'-o',str(obj)],check=True)
        objects.append(str(obj))
    binary=temp/'mm-renderer'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20',*native,str(ROOT/'tests/reward_gi/mm_renderer_test.cpp'),*objects,
                    '-Wl,--gc-sections','-Wl,--export-dynamic-symbol=OOT_NeiResourceExists','-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
