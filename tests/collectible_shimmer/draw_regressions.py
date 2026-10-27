#!/usr/bin/env python3
"""Execute production native drop functions and foreign metadata/cache boundaries.

Engine resource/GPU boundaries are stand-ins; original model submissions and
matrix operations execute. This cannot establish in-game effect size or layout.
"""
from pathlib import Path
import os
import re
import subprocess
import tempfile
from native_regressions import ROOT, function


def compile_run(code, name):
    with tempfile.TemporaryDirectory(prefix='collectible-draw-') as tmp:
        src=Path(tmp)/'draw.cpp'; src.write_text(code)
        out=Path(tmp)/'draw'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-Wno-unused-variable','-Wno-unused-parameter','-I',str(ROOT/'combo/menu'),str(src),'-o',str(out)],check=True)
        subprocess.run([str(out)],check=True)
    print(name)


def native(mm):
    game='mm' if mm else 'soh'
    text=(ROOT/game/'src/code/z_en_item00.c').read_text()
    names=['EnItem00_DrawRupee','EnItem00_DrawHeartContainer','EnItem00_DrawHeartPiece']
    body=re.sub(r'\bthis\b', 'self', '\n'.join(function(text,n) for n in names))
    code=r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
using s32=int32_t; using u8=uint8_t;
struct Color_RGB8 { uint8_t r,g,b; };
struct PlayState { struct { void* gfxCtx=nullptr; } state; int objectCtx; };
struct EnItem00 { struct { int params; int objectSlot; } actor; };
int enabled=0, stars=0, draws=0, palette=-1;
float matrix=1, starMatrix=0; std::vector<float> stack;
int buffer[1000]; int* POLY_OPA_DISP=buffer; int* POLY_XLU_DISP=buffer+500;
void Matrix_Push() { stack.push_back(matrix); }
void Matrix_Pop() { assert(!stack.empty()); matrix=stack.back(); stack.pop_back(); }
void Matrix_Scale(float a,float b,float c,int) { assert(a==b && b==c); matrix*=a; }
enum { MTXMODE_APPLY, G_MTX_MODELVIEW, G_MTX_LOAD, OBJECT_GI_HEARTS };
enum { GID_RUPEE_GREEN=10,GID_RUPEE_BLUE,GID_RUPEE_RED,GID_RUPEE_PURPLE,GID_RUPEE_GOLD,GID_RUPEE_HUGE,GID_RUPEE_SILVER,GID_HEART,GID_RECOVERY_HEART,GID_HEART_CONTAINER,GID_HEART_PIECE };
enum { ITEM00_RUPEE_GREEN=0,ITEM00_RUPEE_BLUE=1,ITEM00_RUPEE_RED=2,ITEM00_RUPEE_ORANGE=0x13,ITEM00_RUPEE_HUGE=0x13,ITEM00_RUPEE_PURPLE=0x14 };
int CVarGetInteger(const char* key,int fallback) { return std::string(key)=="gEnhancements.CollectibleShimmer"?enabled:fallback; }
Color_RGB8 CVarGetColor24(const char*,Color_RGB8 c) { return c; }
#define OPEN_DISPS(ctx)
#define CLOSE_DISPS(ctx)
#define Gfx_SetupDL_25Opa(ctx) ((void)0)
#define Gfx_SetupDL_25Xlu(ctx) ((void)0)
#define Gfx_SetupDL25_Opa(ctx) ((void)0)
#define Gfx_SetupDL25_Xlu(ctx) ((void)0)
#define func_8002EBCC(a,p,i) ((void)0)
#define func_8002ED80(a,p,i) ((void)0)
#define func_800B8050(a,p,i) ((void)0)
#define func_800B8118(a,p,i) ((void)0)
#define MATRIX_NEWMTX(ctx) 0
#define MATRIX_FINALIZE_AND_LOAD(p,ctx) ((void)(p))
#define gSPMatrix(p,m,f) ((void)(p))
#define gDPSetGrayscaleColor(p,r,g,b,a) ((void)(p))
#define gSPGrayscale(p,e) ((void)(p))
#define gSPSegment(p,s,t) ((void)(p),(void)(t))
#define gSPDisplayList(p,d) ((void)(p),++draws)
#define SEGMENTED_TO_VIRTUAL(t) (t)
#define Lib_SegmentedToVirtual(t) (t)
#define CVAR_COSMETIC(x) "gCosmetics." x
int gRupeeDL,gHeartPieceExteriorDL,gHeartContainerInteriorDL,gHeartPieceInteriorDL,gGiHeartBorderDL,gGiHeartContainerDL;
int sRupeeTex[5],sRupeeTextures[5];
int Object_GetSlot(int*,int) { return 7; }
int* GetItem_DrawDListWithCosmetics(int* p,int,int) { ++draws; return p; }
'''
    code='#include <string>\n'+code
    if mm: code+='#define COMBO_COLLECTIBLE_SHIMMER_HOST_MM\n'
    code+='#include "ComboCollectibleShimmerHost.h"\n'
    code+=r'''
extern "C" void OOT_SampleCollectibleShimmerColor(int32_t p,uint8_t c[4]) { palette=p; c[0]=17; c[1]=81; c[2]=204; c[3]=255; }
extern "C" void MM_SampleCollectibleShimmerColor(int32_t p,uint8_t c[4]) { OOT_SampleCollectibleShimmerColor(p,c); }
extern "C" void NeiGi_DrawTintedShimmerOverlay(PlayState*,const uint8_t c[4],const char*) { assert(c[0]==17 && c[1]==81 && c[2]==204); ++stars; starMatrix=matrix; }
'''+body+r'''
void near(float a,float b) { assert(a-b<0.00001f && b-a<0.00001f); }
int main() {
    PlayState play{}; EnItem00 actor{{ITEM00_RUPEE_GREEN,7}};
    for(int on : {0,1}) {
        enabled=on;
        for(const auto& pair : { std::pair<int,int>{ITEM00_RUPEE_GREEN,0},{ITEM00_RUPEE_BLUE,1},{ITEM00_RUPEE_RED,2},{ITEM00_RUPEE_PURPLE,3},{ITEM00_RUPEE_ORANGE,4} }) {
            matrix=.015f; stars=draws=0; actor.actor.params=pair.first;
            EnItem00_DrawRupee(&actor,&play);
            assert(stars==on && draws==1 && stack.empty()); near(matrix,.015f);
            if(on) { assert(palette==pair.second); near(starMatrix,pair.second<3?.375f:.2625f); }
        }
        matrix=.02f; stars=draws=0;
        EnItem00_DrawHeartPiece(&actor,&play);
        assert(stars==on && draws==1 && stack.empty()); near(matrix,.02f);
        if(on) { assert(palette==6); near(starMatrix,.42f); }
        matrix=.02f; stars=draws=0;
        EnItem00_DrawHeartContainer(&actor,&play);
        assert(stars==on && draws==2 && stack.empty());
'''
    code+=('        near(matrix,.4f);\n' if mm else '        near(matrix,.02f);\n')
    code+='        if(on) { assert(palette==6); near(starMatrix,.4f); }\n    }\n'
    if mm: code+='    actor.actor.objectSlot=3; enabled=1; stars=draws=0; matrix=.02f; EnItem00_DrawHeartContainer(&actor,&play); assert(stars==0 && draws==0); near(matrix,.02f);\n'
    code+='}\n'
    compile_run(code,game+': native drop type/color, single overlay, bounded scale/matrix and object guard passed')


def foreign(mm):
    host='MM' if mm else 'OOT'
    source=(ROOT/f'combo/menu/ComboForeignDraw{host}.h').read_text()
    field=re.search(r'(?m)^    (?:bool|int32_t) itemShimmer = [^;]+;',source).group()
    assign=re.search(r'(?m)^    info.itemShimmer = [^;]+;',source).group()
    # Execute the actual cached field declaration and transfer; bool narrowing
    # would erase the explicit-tint mode while still producing a visible effect.
    code='''#include <cassert>\n#include "ComboCollectibleShimmer.h"\nstruct Info {\n'''+field+'''\n};
int main() { Info info; struct { int32_t itemShimmer; } raw;
for(int32_t mode : {0,1,2}) { raw.itemShimmer=mode;
'''+assign+'''\nassert(info.itemShimmer==mode); } }\n'''
    code='#include <initializer_list>\n'+code
    compile_run(code,host+': foreign cache preserves disabled, legacy and explicit-tint modes')
    provider=(ROOT/f'combo/menu/ComboItemDraw{host}.h').read_text()
    start=provider.index('    const int32_t nativeShimmer = GetItem_GetShimmerColor(')
    if mm:
        stop=provider.index('    if (it->second.drawId == GID_CHATEAU)',start)
        setup='struct Entry { struct { int drawId=1; } second; } entry; auto* it=&entry;'
    else:
        stop=provider.index('    // Apply after native eligibility',start)
        setup='struct { int gid=1; } gi; uint8_t nativeShimmerColor[4]={};'
    boundary=provider[start:stop]
    code='#include <cassert>\n#include <cstdint>\n#include <cstring>\n#include "ComboCollectibleShimmer.h"\nusing s16=int16_t;\nint mode=0;\nint GetItem_GetShimmerColor(s16 id,uint8_t c[4]) { assert(id==1); c[0]=13; c[1]=97; c[2]=241; c[3]=255; return mode; }\nint ComboCollectibleShimmer_ProfileForDrawId(s16 id) { assert(id==1); return 0; }\nstruct Raw { int32_t itemShimmer=0; int32_t stateDependent=0; uint8_t itemShimmerColor[4]={}; };\nvoid fill(Raw* out) {\n'+setup+'\n'+boundary+'}\n'
    code+='int main() { Raw out; mode=0; fill(&out); assert(out.itemShimmer==0 && out.stateDependent==2);\nmode=2; fill(&out); assert(out.itemShimmer==2 && out.stateDependent==2 && out.itemShimmerColor[0]==13);\nmode=0; out=Raw{}; fill(&out); assert(out.itemShimmer==0 && out.stateDependent==2); }\n'
    compile_run(code,host+': foreign descriptor retains mode2 and refreshes off/on/owner-color')



if __name__=='__main__':
    native(False); native(True); foreign(False); foreign(True)
