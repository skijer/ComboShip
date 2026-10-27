#!/usr/bin/env python3
"""Execute native/foreign Twinrova head surgery with actual owner recipes."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
from run_mm_scene_randomization_tests import function
ROOT = Path(__file__).resolve().parents[2]
owner = (ROOT/'combo/menu/ComboItemDrawOOT.h').read_text()
native = (ROOT/'soh/soh/Enhancements/randomizer/draw.cpp').read_text()
fixture = r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>
#include "combo/menu/ComboItemDrawABI.h"
using s32=int32_t;
struct Gfx {int unused;};
struct Vec3f {float x,y,z;};
struct Vec3s {int16_t x,y,z;};
struct GraphicsContext {};
struct PlayState {struct {GraphicsContext* gfxCtx;} state;};
#define CVAR_RANDOMIZER_ENHANCEMENT(x) x
bool custom=false;
int OOT_MagicJarUsesCustomAsset(const char* path) {assert(!strcmp(path,gTwinrovaKotakeSkel));return custom;}
int CVarGetInteger(const char*,int fallback) {return fallback;}
constexpr int BOSSGOMA_LIMB_EYE=23,BOSSGOMA_LIMB_IRIS=24;
std::vector<const char*> submitted;
Gfx command;
#define OPEN_DISPS(g) ((void)(g))
#define CLOSE_DISPS(g) ((void)(g))
#define POLY_XLU_DISP (&command)
#define G_MTX_NOPUSH 0
#define G_MTX_LOAD 0
#define G_MTX_MODELVIEW 0
#define gSPMatrix(...) ((void)0)
#define gSPDisplayList(p,dl) submitted.push_back((const char*)(dl))
'''
includes='\n'.join('#include "objects/'+name+'/'+name+'.h"' for name in (
 'object_bv','object_gi_fire','object_goma','object_kingdodongo','object_gnd','object_fd','object_sst','object_tw','object_ganon2'))
fixture=fixture.replace('#include "combo/menu/ComboItemDrawABI.h"', '#include "combo/menu/ComboItemDrawABI.h"\n'+includes)
a=owner.index('static const uint8_t kBossSoulFlameColors');b=owner.index('\n};',a)+3
fixture += owner[a:b]+'\n'
fixture += '\n'.join(function(owner,n) for n in ('OOT_AnimBossSoulFlame','OOT_AnimSeg','OOT_AnimLimbEnv','OOT_FillBossSoulAnim'))+'\n'
fixture += 'static bool sKotakeGiCustomSkeleton = false;\n'
fixture += function(native,'OverrideLimbDrawKotake')+'\n'+function(native,'PostLimbDrawKotake')+'\n'
fixture += r'''
int main() {
 GraphicsContext gfx;PlayState play{{&gfx}};
 for(bool selected:{false,true,false}) {
  custom=selected;sKotakeGiCustomSkeleton=selected;
  CwItemAnimDrawInfo info{};assert(OOT_FillBossSoulAnim(7,&info)==1);
  assert(!strcmp(info.skelPath,gTwinrovaKotakeSkel));
  assert(!strcmp(info.animPath,gTwinrovaKotakeKoumeFlyAnim));
  assert(info.limbCount==27 && info.segCount==3);
  assert(info.limbDLCount==1);
  if(custom) assert(info.limbDLs[0].dlPath==nullptr);
  if(!custom) assert(info.limbDLs[0].limbIndex==21 &&
    !strcmp(info.limbDLs[0].dlPath,gTwinrovaKotakeHeadDL) &&
    !strcmp(info.limbDLs[0].postDlPath,gTwinrovaKotakeIceHairDL));
  Gfx* head=(Gfx*)"__OTR__replacement/sistersHead";
  OverrideLimbDrawKotake(&play,21,&head,nullptr,nullptr,nullptr);
  assert(!strcmp((const char*)head,custom?"__OTR__replacement/sistersHead":gTwinrovaKotakeHeadDL));
  submitted.clear();PostLimbDrawKotake(&play,21,&head,nullptr,nullptr);
  assert(submitted.size()==1);
  assert(!strcmp(submitted[0],gTwinrovaKotakeIceHairDL));
 }
}
'''
with tempfile.TemporaryDirectory(prefix='twinrova-gi-') as td:
 path=Path(td)/'test.cpp';path.write_text(fixture);binary=Path(td)/'test'
 flags=['-std=c++20','-Wall','-Wextra','-Wno-unused-parameter','-I'+str(ROOT),'-I'+str(ROOT/'soh/assets'),'-I'+str(ROOT/'soh/include')]
 if '--sanitize' in sys.argv:flags+=['-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie']
 subprocess.run([os.environ.get('CXX','c++'),*flags,str(path),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
print('PASS Twinrova native head/hair, replacement geometry preservation and shared flying clip')
