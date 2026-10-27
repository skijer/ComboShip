"""Real engine graphics ABI exercises new rigid-resource selection in both hosts."""
import subprocess,sys,tempfile,re
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from run_articulated_tests import ROOT,flags as oot_flags
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_mm_nei_tests import flags as mm_flags
subprocess.run([sys.executable, '-B', str(ROOT/'tests/nei_held/run_equipment_regression_tests.py')], check=True)
header=ROOT/'soh/mods/equipment/nei_equipment_presentation.h'
assert header.exists(),'Rigid equipment has no authored gameplay model selection'
with tempfile.TemporaryDirectory(prefix='nei-equipment-') as td:
 for host,flags in [('oot',['-DCOMBO_BUILD',*oot_flags()]),('mm',['-std=c++20','-DNEI_EQUIPMENT_MM',*mm_flags()])]:
  out=str(Path(td)/host)
  def function(source,name):
   match=re.search(r'(?:static )?(?:Gfx\*|u8) '+name+r'\([^)]*\) \{',source)
   assert match,name
   begin=match.start();i=match.end();depth=1
   while depth:
    depth+=(source[i]=='{')-(source[i]=='}');i+=1
   return source[begin:i]
  engine='mm' if host=='mm' else 'soh'
  ext=(ROOT/engine/'mods/extended_equipment.c').read_text()
  four=(ROOT/engine/'mods/equipment/behaviors/equip_foursword.c').read_text()
  functions='\n'.join(function(ext,n) for n in ['ExtEquip_GetCachedDL','Byrna_GetCaneDL','Trident_GetLanceDL','ExtEquip_GetKiteShieldDL','ExtEquip_GetDivineShieldDL'])
  if host=='mm':functions+='\n'+function(four,'FourSword_HeldSwordDLForFrame')
  functions+='\n'+function(four,'FourSword_HeldSwordDL')
  (Path(td)/'equipment_bindings.inc').write_text(functions)
  subprocess.run(['c++',*flags,'-I'+td,str(ROOT/'tests/nei_held/equipment_runtime_test.cpp'),'-o',out],check=True)
  subprocess.run([out],check=True)
  subprocess.run([out,"missing-archive"],check=True)
subprocess.run([sys.executable,'-B',str(ROOT/'tests/nei_held/equipment_geometry_test.py')],check=True)
subprocess.run([sys.executable,'-B',str(ROOT/'tests/nei_held/equipment_binding_test.py')],check=True)
print('PASS: rigid equipment selection, missing resources, pair fallback, active Alt priority, OoT/MM deferred DL ABI and source geometry')
subprocess.run([sys.executable,'-B',str(ROOT/'tests/nei_held/run_held_sword_tests.py')],check=True)
subprocess.run([sys.executable,'-B',str(ROOT/'tests/nei_held/run_oot_sword_limb_tests.py')],check=True)
