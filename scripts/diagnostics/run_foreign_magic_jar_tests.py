#!/usr/bin/env python3
"""Execute donor magic cosmetic policy and MM grayscale command submission."""
from pathlib import Path
import os, subprocess, sys, tempfile
ROOT = Path(__file__).resolve().parents[2]
def body(text, name):
    start = text.rfind('\n', 0, text.index(name+'('))+1
    brace = text.index('{', start); end=brace+1; depth=1
    while depth:
        depth += (text[end]=='{')-(text[end]=='}'); end+=1
    return text[start:end]
owner = (ROOT/'combo/menu/ComboItemDrawOOT.h').read_text()
host = (ROOT/'combo/menu/ComboForeignDrawMM.h').read_text()
assert 'OOT_DescribeMagicJar(' in owner, 'missing donor magic cosmetic export'
assert 'MM_DrawForeignMagicJarDList(' in host, 'missing MM magic cosmetic submission'
fixture = r'''
#include <cassert>
#include <cstdint>
#include <vector>
#include "combo/menu/ComboItemDrawABI.h"
using s16=int16_t;
struct Color_RGB8 {uint8_t r,g,b;};
struct Gfx {int op=0; uint8_t r=0,g=0,b=0;};
constexpr int GID_MAGIC_SMALL=1, GID_MAGIC_LARGE=2;
#define CVAR_COSMETIC(x) x
bool changed=false, custom=false, loaded=false;
Color_RGB8 liveColor{230,93,171};
int CVarGetInteger(const char*,int) {return changed;}
Color_RGB8 CVarGetColor24(const char*,Color_RGB8) {return {230,93,171};}
Color_RGB8 CwLiveCosmeticColor(const char*,Color_RGB8) {return liveColor;}
int32_t OOT_MagicJarUsesCustomAsset(const char*) {loaded=true;return custom;}
void grayColor(Gfx*p,uint8_t r,uint8_t g,uint8_t b) {*p={1,r,g,b};}
#define gDPSetGrayscaleColor(p,r,g,b,a) grayColor(p,r,g,b)
#define gSPGrayscale(p,on) (*(p)=Gfx{on?2:4})
#define gSPDisplayList(p,dl) ((void)(dl), *(p)=Gfx{3})
'''
checks = r'''
int main() {
 for(bool change:{false,true}) for(bool alt:{false,true}) for(int id:{GID_MAGIC_SMALL,GID_MAGIC_LARGE,9}) {
  changed=change;custom=alt;loaded=false;
  CwItemDrawInfo info{};info.dlists[0]="__OTR__magic";info.dlistCount=1;
  OOT_DescribeMagicJar(id,&info);
  bool tint=change&&alt&&id!=9;
  assert((info.primColorOpa[3]!=0)==tint);
  if(id!=9) {
   assert(info.drawKind==CW_DRAW_KIND_MAGIC_JAR&&info.stateDependent==2 && info.itemShimmer);
   assert(info.itemShimmerColor[0]==(change?230:0) && info.itemShimmerColor[1]==(change?93:200));
  }
  else assert(info.drawKind==CW_DRAW_KIND_SIMPLE);
  Gfx commands[8];Gfx*end=MM_DrawForeignMagicJarDList(commands,"__OTR__@oot:magic",info.primColorOpa);
  if(tint) {assert(end==commands+4);assert(commands[0].op==1&&commands[0].r==230&&commands[0].g==93&&commands[0].b==171);
   assert(commands[1].op==2&&commands[2].op==3&&commands[3].op==4);}
  else {assert(end==commands+1&&commands[0].op==3);}
 }
 changed=true; custom=true; CwItemDrawInfo animated{}; animated.dlists[0]="__OTR__magic";
 liveColor={12,210,250}; OOT_DescribeMagicJar(GID_MAGIC_LARGE,&animated);
 assert(animated.itemShimmerColor[0]==12 && animated.itemShimmerColor[1]==210 && animated.itemShimmerColor[2]==250);
 assert(animated.primColorOpa[0]==animated.itemShimmerColor[0]);
}
'''
with tempfile.TemporaryDirectory(prefix='foreign-magic-') as td:
    source=Path(td)/'test.cpp';source.write_text(fixture+body(owner,'OOT_DescribeMagicJar')+'\n'+body(host,'MM_DrawForeignMagicJarDList')+checks)
    flags=['-std=c++20','-Wall','-Wextra','-I'+str(ROOT)]
    if '--sanitize' in sys.argv:flags += ['-fsanitize=address,undefined','-fno-omit-frame-pointer','-g']
    binary=Path(td)/'test';subprocess.run([os.environ.get('CXX','c++'),*flags,str(source),'-o',str(binary)],check=True);subprocess.run([str(binary)],check=True)
print('PASS donor live magic cosmetic/Alt selection and MM grayscale tint scope')
