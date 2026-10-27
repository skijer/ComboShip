"""Compile exact production quest drawers and helpers for each native host."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tests/nei_held'))
from run_articulated_tests import flags as oot_flags

def run_oot():
    with tempfile.TemporaryDirectory(prefix='nei-quest-oot-') as td:
        objects=[]
        for source in ['helpers/equip_helper.c','objects/object_elemental_wand.c','objects/object_sheikah_slate.c']:
            output=str(Path(td)/(Path(source).stem+'.o'))
            subprocess.run(['cc','-std=gnu2x',*oot_flags()[1:],'-Wno-implicit-function-declaration','-ffunction-sections','-fdata-sections','-c',str(ROOT/'soh/mods/items'/source),'-o',output],check=True)
            objects.append(output)
        binary=str(Path(td)/'quest')
        subprocess.run(['c++',*oot_flags(),'-ffunction-sections','-fdata-sections',str(ROOT/'tests/nei_quest_held/oot_runtime_test.cpp'),*objects,'-Wl,--gc-sections','-o',binary],check=True)
        for args in [['--partial-mod'],['--mod'],['--cache'],['--partial'],[],['--fallback'],['--missing']]:subprocess.run([binary,*args],check=True)

def run_mm():
    sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
    from run_mm_nei_tests import flags as mm_flags, write_graph_helpers
    from rig_fit_query import write_query
    with tempfile.TemporaryDirectory(prefix='nei-quest-mm-') as td:
        write_graph_helpers(td)
        query=Path(td)/'query.cpp';write_query(ROOT,query)
        text=(ROOT/'mm/mods/items/logic/item_elemental_wand.c').read_text()
        start=text.index('void Wand_Draw(');body=text[start:text.index('\n}',start)+2]
        callback=Path(td)/'wand_callback.c'
        callback.write_text('#include "z64.h"\nvoid WandShadow_Draw(PlayState*);\nvoid WandStorm_Draw(PlayState*);\nvoid WandWind_Draw(Player*,PlayState*);\nvoid CustomItems_DrawElementalWand(Player*,PlayState*);\n'+body)
        objects=[]
        sources=[ROOT/'mm/mods/items'/p for p in ['helpers/equip_helper.c','objects/object_elemental_wand.c','objects/object_sheikah_slate.c']]+[callback]
        for source in sources:
            output=str(Path(td)/(source.stem+'.o'))
            subprocess.run(['cc','-std=gnu2x',*mm_flags(),'-ffunction-sections','-fdata-sections','-c',str(source),'-o',output],check=True);objects.append(output)
        binary=str(Path(td)/'quest')
        subprocess.run(['c++','-std=c++20',*mm_flags(),'-I'+td,'-ffunction-sections','-fdata-sections',str(ROOT/'tests/nei_quest_held/mm_runtime_test.cpp'),str(query),*objects,'-Wl,--gc-sections','-o',binary],check=True)
        for args in [['--partial-mod'],['--mod'],[],['--fallback'],['--missing']]:subprocess.run([binary,*args],check=True)

if __name__=='__main__':
    run_oot();run_mm()
    for source in ['geometry_test.py','mm_dispatch_test.py','mm_gi_loader_test.py','mm_gi_boot_mod_test.py','mm_cane_gi_test.py']:subprocess.run([sys.executable,str(ROOT/'tests/nei_quest_held'/source)],check=True)
