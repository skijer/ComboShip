#!/usr/bin/env python3
import os,sys,tempfile,subprocess
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'scripts/diagnostics'))
from run_mm_nei_tests import ROOT,flags,write_graph_helpers
from rig_fit_query import write_query
# Connected-callsite audit supplements execution of the actual object/component code.
common=(ROOT/'mm/mods/items/custom_items_common.c').read_text()
for name in ['BallChain','Beetle','CaneOfSomaria','DekuLeaf','GustJar','MogmaMitts','Shovel','Spinner','TimeGate','TimeGatePortal','Whip','Lantern']:
 assert 'CustomItems_Draw'+name+'(' in common,name
assert 'GustJarPot_Draw(this, play)' in (ROOT/'mm/mods/items/logic/item_gustjar.c').read_text()
assert 'NeiLantern_DrawHeld(p, play,' in (ROOT/'mm/mods/items/logic/item_lantern.c').read_text()
assert 'NeiArticulated_DrawSwitchHookTip(play, player, &this->actor)' in (ROOT/'mm/src/overlays/actors/ovl_Arms_Hook/z_arms_hook.c').read_text()
with tempfile.TemporaryDirectory(prefix='mm-nei-held-') as td:
 write_graph_helpers(td)
 query=Path(td)/'rig_fit_query.cpp'
 write_query(ROOT,query)
 objects=[]
 for s in ['objects/object_'+n+'.c' for n in ['ballchain','beetle','cane_of_somaria','dekuleaf','gustjar_pot','mogma_mitts','shovel','spinner','timegate','whip']] + ['helpers/equip_helper.c']:
  obj=str(Path(td)/(Path(s).stem+'.o'))
  source=ROOT/'mm/mods/items'/s
  if s=='objects/object_gustjar_pot.c':
   source=Path(td)/'jar.c';source.write_text('#include "'+str(ROOT/'mm/mods/items'/s)+'"\nvoid CustomItems_DrawGustJar(Player* p,PlayState* play){GustJarPot_Draw(p,play);}')
  subprocess.run(['cc','-std=gnu2x',*flags(),'-include',str(ROOT/'mm/mods/actors/somaria_cubes.h'),'-include',str(ROOT/'mm/mods/actors/pacci_flip_vfx.h'),'-ffunction-sections','-fdata-sections','-c',str(source),'-o',obj],check=True);objects.append(obj)
 for name in ['NeiArticulatedPresentation','NeiResourceRouting']:
  obj=str(Path(td)/(name+'.o'))
  subprocess.run(['c++','-std=c++20',*flags(),'-ffunction-sections','-fdata-sections','-c',str(ROOT/'mm/2s2h/Rando'/(name+'.cpp')),'-o',obj],check=True);objects.append(obj)
 binary=str(Path(td)/'held')
 subprocess.run(['c++','-std=c++20',*flags(),'-I'+td,str(ROOT/'tests/mm_nei/held_runtime_test.cpp'),str(query),*objects,'-Wl,--gc-sections','-o',binary],check=True)
 subprocess.run([binary],check=True)
