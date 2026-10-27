#!/usr/bin/env python3
"""Native viewer fixture; no engine execution or archive mutation."""
from pathlib import Path
import os,re,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2]
def function(source,name):
    m=re.search(r'^[^\n;{}]*\b'+name+r'\([^;{}]*\)\s*\{',source,re.M)
    if not m: raise RuntimeError(name)
    i=m.end(); depth=1
    while depth:
        depth+=(source[i]=='{')-(source[i]=='}');i+=1
    return source[m.start():i]+'\n'
viewer=(ROOT/'soh/src/overlays/actors/ovl_En_Viewer/z_en_viewer.c').read_text()
fixture=(ROOT/'soh/tests/static_story_mm_viewer_test.c').read_text()
loader=(ROOT/'soh/mods/transformation_masks/assets/mm_asset_loader.cpp').read_text()
table=re.search(r'static Gfx sMmOpaqueRenderModeDL\[\] = \{.*?\n\};',loader,re.S).group(0)
fixture=fixture.replace('/* PRODUCTION_OPAQUE_RENDER_MODE */',table+'\n'+function(loader,'MmAssets_GetOpaqueRenderMode'))
names=['EnViewer_Update','EnViewer_Destroy','EnViewer_StaticSelectSkullKidModel','EnViewerStatic_WaitForObjects','EnViewerStatic_Update','EnViewer_StaticGreatFairyEyeIndex','EnViewer_StaticGreatFairyOverrideLimbDraw','EnViewer_DrawStaticGreatFairy','EnViewer_StaticTreasureChestShopGalOverrideLimbDraw','EnViewer_StaticOrdinaryMmOverrideLimbDraw','EnViewer_StaticAnjuPostLimbDraw','EnViewer_DrawStaticMmActor','EnViewer_DrawStaticSkullKid','EnViewerStatic_Draw']
names[names.index('EnViewer_DrawStaticMmActor'):names.index('EnViewer_DrawStaticMmActor')] = ['EnViewer_StaticMikauPostLimbDraw', 'EnViewer_StaticRegionalPostLimbDraw', 'EnViewer_StaticRegionalPostLimbDrawXlu']
table=re.search(r'static EnViewerDrawFunc sDrawFuncs\[\] = \{.*?\n\};',viewer,re.S).group(0)
fixture=fixture.replace('/* PRODUCTION_VIEWER_FUNCTIONS */','\n'.join(function(viewer,n) for n in names)+table+function(viewer,'EnViewer_Draw'))
with tempfile.TemporaryDirectory(prefix='regional-actors-') as d:
    path=Path(d)/'viewer.c';path.write_text(fixture)
    flags=['-std=gnu11','-DNDEBUG','-w','-DF3DEX_GBI_2','-DCONTROLLERBUTTONS_T=uint32_t','-DLOG_LEVEL_GAME_PRINTS=0','-ffunction-sections','-fdata-sections','-Wl,--gc-sections']
    includes=['soh/include','soh/src','soh/assets','soh','libultraship/include','combo','combo/menu']
    for combo in (False,True):
        command=['cc',*flags,*(['-DCOMBO_BUILD'] if combo else []),*['-I'+str(ROOT/p) for p in includes],str(path),*[str(ROOT/('soh/src/overlays/actors/ovl_En_Viewer/static_story_'+n+'.c')) for n in ['actor','mm_actor','ganon']],'-lm','-o',d+'/viewer']
        subprocess.run(command,cwd=ROOT,check=True)
        subprocess.run([d+'/viewer'],check=True)
        print('PASS regional/native viewer: '+('combo' if combo else 'solo'))

source=(ROOT/'soh/soh/Enhancements/Gameplay/ComboRegionalActors.cpp').read_text()
fixture=(ROOT/'tests/regional_actors/oot_placement_test.cpp').read_text().replace('/* PRODUCTION_RUNTIME */',function(source,'ComboRegionalActorsOot_Update'))
with tempfile.TemporaryDirectory(prefix='regional-placement-') as d:
    path=Path(d)/'placement.cpp';path.write_text(fixture)
    obj=Path(d)/'catalog.o'
    subprocess.run(['cc','-std=c11','-I'+str(ROOT/'soh/include'),'-c',str(ROOT/'soh/src/overlays/actors/ovl_En_Viewer/static_story_actor.c'),'-o',str(obj)],check=True)
    subprocess.run(['c++','-std=c++20','-w','-DCOMBO_BUILD','-DF3DEX_GBI_2',*['-I'+str(ROOT/p) for p in includes],str(path),str(obj),'-o',d+'/placement'],check=True)
    subprocess.run([d+'/placement'],check=True)
