"""Exercise actual native receipt rows and exported bottle draw recipes."""
import os
from pathlib import Path
import re
import subprocess
import tempfile
from run_mm_scene_randomization_tests import function

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT/'mm/src/code/z_draw.c').read_text()
a=source.index('static DrawItemTableEntry sDrawItemTable[]')
b=source.index('\n};',a)+3
table=source[a:b]
export=function(source,'GetItem_GetDrawTableEntry')
paths={}
for header in (ROOT/'mm/assets/objects').rglob('*.h'):
    paths.update(re.findall(r'#define d(\w+) "([^"\n]+)"',header.read_text()))
# Markers also have native C declarations; their names map to the bundled roots.
paths.update({'gComboMushroomBottleGi':'__OTR__objects/combo_bottle_gi/MushroomBottle',
              'gComboPrincessBottleGi':'__OTR__objects/combo_bottle_gi/PrincessBottle',
              'gComboGoldDustBottleGi':'__OTR__objects/combo_bottle_gi/GoldDustBottle'})
resources=sorted(set(re.findall(r'\b(g\w+)\b',table)))
callbacks=sorted(set(re.findall(r'GetItem_Draw\w+',table+export)))
code='#include <cassert>\n#include <cstdint>\n#include <cstring>\n#include "mm/include/z64item.h"\nusing s8=int8_t;using s16=int16_t;using s32=int32_t;using f32=float;\nstruct PlayState{};struct Gfx{};\nstruct DrawItemTableEntry {void (*drawFunc)(PlayState*,s16);void* drawResources[8];};\n#define ARRAY_COUNT(x) (sizeof(x)/sizeof((x)[0]))\nconst char* selectedModPath=nullptr;\nbool compositionAvailable=true;int compositionDraws=0,nativeDraws=0,chosenContent=0;\nint ResourceMgr_IsModAsset(const char* path) {return selectedModPath && !strcmp(path,selectedModPath);}\nint ResourceMgr_IsModAssetForGame(const char*,const char*) {return 0;}\nint ResourceMgr_IsCustomAssetForGame(const char*,const char*) {return 0;}\nint ResourceMgr_FileExists(const char*) {return 0;}\nGfx* ResourceMgr_LoadGfxByName(const char*) {return nullptr;}\nint ComboBottleGi_HasPotionRecipe(const char*,const char*,const char*,Gfx*(*)(const char*)) {return 0;}\n#include "combo/menu/ComboFairyBottle.h"\n'
code+='\n'.join('char '+name+'[]="'+paths.get(name,name)+'";' for name in resources)
code+='\n#include "combo/menu/ComboBottleContents.h"\nextern "C" int ComboBottleContents_Draw(PlayState*,int content){++compositionDraws;chosenContent=content;return compositionAvailable;}\n'
code+='\n'+'\n'.join('void '+name+'(PlayState*,s16) '+(';' if name=='GetItem_DrawSeahorseBottle' else '{++nativeDraws;}' if name=='GetItem_DrawOpa0Xlu1' else '{}') for name in callbacks)
code+='\n'+table+'\n'+function(source,'GetItem_EmptyBottleShell')+'\n'+function(source,'GetItem_FairyBottleShell')+'\n'+export+'\n'+function(source,'GetItem_DrawSeahorseBottle')+'\nint main(){\n'
items=(ROOT/'mm/2s2h/Rando/StaticData/Items.cpp').read_text()
player=(ROOT/'mm/src/overlays/actors/ovl_player_actor/z_player.c').read_text()
for gi,want in [('GI_MUSHROOM','MushroomBottle'),('GI_DEKU_PRINCESS','PrincessBottle'),('GI_GOLD_DUST','GoldDustBottle'),('GI_SEAHORSE','SeahorseBottle')]:
    row=re.search(r'// '+gi+r'\n\s*GET_ITEM\([^,]+,[^,]+,\s*(GID_\w+)',player)
    assert row,gi
    gid=row.group(1)
    code+='{void* dl[8]{};int xlu=-1,scroll=0,kind=-1;float scale=0;\nassert(GetItem_GetDrawTableEntry(%s,dl,8,&xlu,&scale,&scroll,&kind)>0);\nassert(kind==40 && "bottled receipt incorrectly exports a loose item or empty bottle");\nassert(strstr((const char*)dl[0],"/%s") && "receipt bottle contents identity was lost");}\n' % (gid,want)
for ri,want in [('RI_MUSHROOM','MushroomBottle'),('RI_OOT_BOTTLE_MAGIC_MUSHROOM','MushroomBottle'),('RI_BOTTLE_GOLD_DUST','GoldDustBottle')]:
    row=re.search(r'RI\('+ri+r',[^\n]+,\s*(GID_\w+)\)',items)
    assert row,ri
    code+='{void* dl[8]{};int xlu=-1,scroll=0,kind=-1;float scale=0;\nassert(GetItem_GetDrawTableEntry(%s,dl,8,&xlu,&scale,&scroll,&kind)>0);\nassert(kind==40 && "randomizer bottle still uses its old empty/loose alias");\nassert(strstr((const char*)dl[0],"/%s"));}\n' % (row.group(1),want)
code+='''
for(int selected=0;selected<3;++selected){
    selectedModPath=selected==1?gGiSeahorseBottleEmptyDL:selected==2?gGiSeahorseBottleGlassAndCorkDL:nullptr;
    void* dl[8]{};int xlu=-1,scroll=0,kind=-1;float scale=0;
    assert(GetItem_GetDrawTableEntry(GID_SEAHORSE,dl,8,&xlu,&scale,&scroll,&kind)==(selected?2:1));
    assert(kind==(selected?0:40) && xlu==(selected?1:0));
    if(selected)assert(dl[0]==gGiSeahorseBottleEmptyDL && dl[1]==gGiSeahorseBottleGlassAndCorkDL);
    else assert(strstr((const char*)dl[0],"/SeahorseBottle"));
    compositionDraws=nativeDraws=0;compositionAvailable=true;PlayState play;
    GetItem_DrawSeahorseBottle(&play,GID_SEAHORSE);
    assert(compositionDraws==(selected?0:1) && nativeDraws==(selected?1:0));
    if(!selected)assert(chosenContent==CW_BOTTLE_SEAHORSE);
}
selectedModPath=nullptr;compositionAvailable=false;compositionDraws=nativeDraws=0;PlayState play;
GetItem_DrawSeahorseBottle(&play,GID_SEAHORSE);assert(compositionDraws==1 && nativeDraws==1);
}
'''
with tempfile.TemporaryDirectory(prefix='bottle-contents-') as temp:
    path=Path(temp)/'test.cpp';path.write_text(code)
    binary=Path(temp)/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-DCOMBO_BUILD','-I'+str(ROOT),str(path),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('PASS native mushroom/princess/gold-dust/seahorse receipts and MM randomizer exports preserve distinct bottled contents')

renderer=(ROOT/'soh/soh/Enhancements/randomizer/NeiGiMeshRenderer.inc').read_text()
arena=renderer[renderer.index('struct NeiGi_ArenaBudget'):renderer.index('static bool NeiGi_ValidMesh')]
contents=(ROOT/'combo/menu/ComboBottleContentsDraw.h').read_text()
contents='\n'.join(line for line in contents.splitlines() if not line.startswith('#include'))
fixture=(ROOT/'tests/bottle_gi/contents_test.cpp').read_text()
fixture=fixture.replace('/* PRODUCTION_ARENA */',arena)
fixture=fixture.replace('/* PRODUCTION_MESH_TAIL */',function(renderer,'NeiGi_DrawMeshWithTail'))
fixture=fixture.replace('/* PRODUCTION_CONTENTS */',contents)
fixture=fixture.replace('/* PRODUCTION_IMPORTED_BOTTLE */',function((ROOT/'mm/2s2h/Rando/DrawItem.cpp').read_text(),'DrawOotBottleWithShimmer'))
with tempfile.TemporaryDirectory(prefix='bottle-contents-render-') as temp:
    source=Path(temp)/'test.cpp';source.write_text(fixture)
    for host in ('oot','mm'):
        binary=Path(temp)/host
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',
                        *(['-DCOMBO_BOTTLE_HOST_MM'] if host=='mm' else []),'-I'+str(ROOT),str(source),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)
        print('PASS '+host+' accepted static contents, selected replacement priority, host casing, exception/missing-resource handling, arena headroom and single imported shimmer')

    # Compile the exact composer and host aliases with both engines' real types.
    for host in ('soh','mm'):
        body='#include "global.h"\n'
        if host=='mm':
            body+='#include <nlohmann/json.hpp>\n#include "2s2h/BenPort.h"\n#include "2s2h/Rando/DrawFuncs.h"\nextern "C" {Gfx* ResourceMgr_LoadGfxByName(const char*);uint8_t ResourceMgr_FileExists(const char*);uint8_t ResourceMgr_FileAltExists(const char*);bool ResourceMgr_IsAltAssetsEnabled();int ResourceMgr_IsModAsset(const char*); }\n#define COMBO_FOREIGN_ANIM_HOST_MM\n#define COMBO_BOTTLE_HOST_MM\n'
            includes=['mm','mm/include','mm/include/PR','mm/src','mm/assets','mm/2s2h']
        else:
            body+='#include "soh/ResourceManagerHelpers.h"\n'
            includes=['soh','soh/include','soh/src','soh/assets','soh/mods']
        body+='#include "ComboForeignAnim.h"\n#include "ComboBottleContentsDraw.h"\n'
        source.write_text(body)
        includes+=['libultraship/include','libultraship/src','combo','combo/menu']
        command=[os.environ.get('CXX','c++'),'-std=c++20','-DF3DEX_GBI_2','-DCOMBO_BUILD',
                 '-DLOG_LEVEL_GAME_PRINTS=0','-DCONTROLLERBUTTONS_T=uint32_t','-DNON_EQUIVALENT','-DNON_MATCHING','-fsyntax-only']
        command+=['-I'+str(ROOT/path) for path in includes]+[str(source)]
        result=subprocess.run(command,capture_output=True,text=True)
        if result.returncode:
            raise RuntimeError(result.stdout+result.stderr)
        print('PASS real-header C++ bottle composer '+host)
        command=[os.environ.get('CC','cc'),'-std=gnu2x','-DF3DEX_GBI_2','-DCOMBO_BUILD',
                 '-DLOG_LEVEL_GAME_PRINTS=0','-DCONTROLLERBUTTONS_T=uint32_t','-DNON_EQUIVALENT','-DNON_MATCHING',
                 '-Werror=implicit-function-declaration','-Wno-incompatible-pointer-types','-fsyntax-only']
        for config in ('CMake/soh-cvars.cmake','CMake/lus-cvars.cmake'):
            for key,value in re.findall(r'set\((CVAR_PREFIX_\w+)\s+"?([^\s"\)]+)',(ROOT/config).read_text()):
                command.append(f'-D{key}="{value}"')
        command+=['-I'+str(ROOT/path) for path in includes]+[str(ROOT/host/'src/code/z_draw.c')]
        result=subprocess.run(command,capture_output=True,text=True)
        if result.returncode:
            raise RuntimeError(result.stdout+result.stderr)
        print('PASS real-header native C GI draw/export '+host)
