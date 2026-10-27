#!/usr/bin/env python3
"""Execute actual imported sword handlers/dispatch with OoT's real GBI headers.

Resource and matrix services are controlled boundaries; no archive or scene
execution is claimed. Export/cache coverage is in run_mm_dungeon_key_tests.py.
"""
import os
import re
from pathlib import Path
import subprocess
import sys
import tempfile
from run_mm_scene_randomization_tests import function

ROOT = Path(__file__).resolve().parents[2]
foreign = (ROOT / 'combo/menu/ComboForeignDrawOOT.h').read_text()
info = foreign[foreign.index('struct ComboForeignDrawInfo {'):foreign.index('\n};',foreign.index('struct ComboForeignDrawInfo {'))+3]
macros = foreign[foreign.index('#define COMBO_FOREIGN_MTX'):foreign.index('// Biggoron')]
mirror_helper = 'extern "C" void MagicMirror_DrawGI(PlayState*) { assert(false && "sword dispatch selected Mirror"); }\n'
mirror_helper += function((ROOT/'combo/menu/ComboMagicMirrorForeign.h').read_text(),'ComboDrawMagicMirrorForeign')
handlers = macros + '\n' + mirror_helper + '\n' + '\n'.join(function(foreign,name) for name in [
    'OOT_RestoreForeignSegs','OOT_DrawForeignGoronSword','OOT_DrawForeignWeaponFlame',
    'OOT_DrawForeignMasterSword','OOT_DrawForeignCustomGi','OOT_DrawForeignSimple','OOT_DrawComboForeign'])
flags=['-std=gnu++20','-DF3DEX_GBI_2','-DCOMBO_BUILD','-DLOG_LEVEL_GAME_PRINTS=0',
       '-DCONTROLLERBUTTONS_T=uint32_t','-DNON_EQUIVALENT','-DNON_MATCHING']
if '--sanitize' in sys.argv:
    flags+=['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie']
with tempfile.TemporaryDirectory(prefix='sword-fallback-') as temporary:
    includes=['.','soh','soh/include','soh/include/PR','soh/src','soh/assets','soh/soh',
              'combo/menu','libultraship/include','libultraship/src']
    template=(ROOT/'tests/sword_fallback/render_test.cpp').read_text()
    mm=(ROOT/'combo/menu/ComboForeignDrawMM.h').read_text()
    mm_info=mm[mm.index('struct ComboForeignDrawInfoOOT {'):mm.index('\n};',mm.index('struct ComboForeignDrawInfoOOT {'))+3]
    mm_dispatch=function(mm,'MM_DrawComboForeign')
    mm_handlers=sorted(set(re.findall(r'\b(MM_DrawForeign\w+)\(',mm_dispatch)) - {'MM_DrawForeignCustomGi','MM_DrawForeignSimple'})
    mm_prefix=r'''
using ComboForeignDrawInfoOOT=ComboForeignDrawInfo;
using RandoCheckId=int;constexpr int RC_UNKNOWN=0;
enum RandoItemId {RI_NONE,RI_OOT_NEI_CANE_OF_SOMARIA,RI_OOT_NEI_CANE_PACCI_FLIP,
 RI_OOT_NEI_CANE_SOMARIA_BLOCK,RI_OOT_NEI_CANE_PACCI_STONE,
 RI_OOT_NEI_CANE_SOMARIA_PLATFORM,RI_OOT_NEI_CANE_PACCI_ULTRAHAND};
namespace Rando {void DrawResolvedItem(RandoItemId,RandoCheckId,Actor*) {
 assert(false && "sword-only fixture unexpectedly dispatched a native MM alias");
}}
PlayState* gPlayState=&play;
const ComboForeignDrawInfoOOT* ComboResolveForeignDrawInfoOOT(int){return &recipe;}
void MM_DrawNeiGi(const CwItemDrawInfo&,bool,int=0){assert(false);}
void DrawOotNeiUltrahand(){assert(false);}
void DrawOotNeiCaneOfSomaria(RandoItemId){assert(false);}
#define Matrix_RotateXF Matrix_RotateX
#define Matrix_RotateYF Matrix_RotateY
#define Matrix_RotateZF Matrix_RotateZ
#define Gfx_SetupDL25_Opa Gfx_SetupDL_25Opa
#define Gfx_SetupDL25_Xlu Gfx_SetupDL_25Xlu
#define MATRIX_FINALIZE_AND_LOAD(pkt, gfx) gSPMatrix(pkt,Matrix_NewMtx(gfx,(char*)__FILE__,__LINE__),G_MTX_MODELVIEW|G_MTX_LOAD|G_MTX_NOPUSH)
'''
    mm_macros=mm[mm.index('#define MM_FOREIGN_PIN_OPA'):mm.index('inline void MM_RestoreForeignSegs')]
    mm_stubs='\n'.join('template<class...T> '+('bool ' if name=='MM_DrawForeignSilverRupee' else 'void ')+name+'(const ComboForeignDrawInfoOOT*,T...){assert(false);'+('return false;' if name=='MM_DrawForeignSilverRupee' else '')+'}' for name in mm_handlers)
    mm_handlers_text=macros+'\n'+mirror_helper+'\n'+function(foreign,'OOT_RestoreForeignSegs')+'\n'+function(foreign,'OOT_DrawForeignWeaponFlame')+'\n'+mm_prefix
    mm_handlers_text+='void DrawOotSlateRuneFlame(uint8_t r,uint8_t g,uint8_t b){const uint8_t c[]={r,g,b,255};OOT_DrawForeignWeaponFlame(&play,c);}\n'
    mm_handlers_text+=mm_macros+'\n'+mm_stubs+'\n'+function(mm,'MM_DrawForeignCustomGi')+'\n'+function(mm,'MM_DrawForeignSimple')+'\n'+mm_dispatch
    mm_handlers_text+='\nvoid OOT_DrawComboForeign(PlayState*,GetItemEntry*,bool shop=false){MM_DrawComboForeign(1,shop);}\n'
    for host,host_info,host_handlers in [('oot',info,handlers),('mm',mm_info.replace('ComboForeignDrawInfoOOT','ComboForeignDrawInfo'),mm_handlers_text)]:
        source=Path(temporary)/(host+'.cpp')
        candidate=template.replace('/* PRODUCTION_INFO */',host_info).replace('/* PRODUCTION_HANDLERS */',host_handlers)
        extra=[]
        if host=='mm':
            candidate=candidate.replace('for (int kind : {CW_DRAW_KIND_MASTER_SWORD, CW_DRAW_KIND_CUSTOM_GI})','for (int kind : {CW_DRAW_KIND_CUSTOM_GI})')
            candidate=candidate.replace('PASS actual OoT sword fallback dispatch','PASS actual MM sword fallback dispatch')
            extra=['-DHOST_MM_ROUTE']
        source.write_text(candidate);binary=Path(temporary)/(host+'-render')
        subprocess.run([os.environ.get('CXX','c++'),*flags,*extra,*['-I'+str(ROOT/p) for p in includes],str(source),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
