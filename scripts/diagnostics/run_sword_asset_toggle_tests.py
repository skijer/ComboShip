#!/usr/bin/env python3
"""Run live sword GI selection with independently toggled host and donor assets."""
import os,re,subprocess,sys,tempfile
from pathlib import Path
from run_time_pedestal_tests import functions
from run_mm_scene_randomization_tests import function
ROOT=Path(__file__).resolve().parents[2]
oot=(ROOT/'soh/soh/Enhancements/randomizer/NeiGiPresentation.cpp').read_text()
mm=(ROOT/'mm/2s2h/Rando/NeiGiPresentation.cpp').read_text()
owner=functions((ROOT/'combo/menu/ComboItemDrawOOT.h').read_text())
of,mf=functions(oot),functions(mm)
source=(ROOT/'tests/sword_fallback/asset_toggle_test.cpp').read_text()
ids=sorted(set(re.findall(r'\bRI_[A-Z0-9_]+\b',mm)))
source=source.replace('/* ITEM_IDS */','enum RandoItemId{'+','.join(ids)+'};')
names=['SelectedSwordPath','HasSelectedSword']
if 'NeiGi_BaseSwordPath' in of:names.append('NeiGi_BaseSwordPath')
names += ['PresentationShimmer', 'NeiGi_FillCrossGameInfo']
source=source.replace('/* OWNER_SELECTION */','\n'.join(of[n] for n in names))
source=source.replace('/* OWNER_CUSTOM_RECIPE */','\n'.join(owner[n] for n in ('CwSimple','CwCustomGi','CwAltSwordGi')))
source=source.replace('/* OWNER_DEPENDENCY */','\n'.join(owner[n] for n in ('OOT_IsStateDependentDraw','OOT_DrawDependency')))
source=source.replace('/* OWNER_QUERY */',function(oot,'OOT_GetNeiGiDrawInfo'))
query='OOT_GetNeiGiDrawInfoForAssets'
source=source.replace('/* OWNER_ASSET_QUERY */',function(oot,query) if query in oot else '')
symbol='if(!strcmp(name,"'+query+'"))return reinterpret_cast<void*>('+query+');' if query in oot else ''
source=source.replace('/* ASSET_QUERY_SYMBOL */',symbol)
begin=mm.index('struct Binding {');end=mm.index('// Only roots',begin)
source=source.replace('/* MM_BINDINGS */',mm[begin:end])
foreign=(ROOT/'combo/menu/ComboForeignDrawMM.h').read_text()
info=foreign[foreign.index('struct ComboForeignDrawInfoOOT {'):foreign.index('\n};',foreign.index('struct ComboForeignDrawInfoOOT {'))+3]
source=source.replace('/* FOREIGN_RESOLVER */',info+'\n'+function(foreign,'ComboFillForeignDrawInfoOOT'))
cache=foreign[foreign.index('struct ComboForeignDrawCacheOOT {'):foreign.index('} // namespace')]
source=source.replace('/* FOREIGN_CACHE */',cache)
source=source.replace('/* MM_SELECTION */','\n'.join(mf[n] for n in ('HasMmLegacyGiMod','GetSelectedOwnerGi','MM_DescribeNeiGi')))
with tempfile.TemporaryDirectory(prefix='sword-asset-toggle-') as tmp:
    cpp=Path(tmp)/'toggle.cpp';cpp.write_text(source);binary=Path(tmp)/'toggle'
    flags=['-std=c++20','-DCOMBO_BUILD','-I'+str(ROOT)]
    if '--sanitize' in sys.argv:flags+=['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie']
    subprocess.run([os.environ.get('CXX','c++'),*flags,str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
