#!/usr/bin/env python3
"""Execute foreign heart body policy and donor native/custom material boundary."""
from pathlib import Path
import os, subprocess, sys, tempfile
ROOT=Path(__file__).resolve().parents[2]
def source(path):
    if '--baseline' in sys.argv:return subprocess.check_output(['git','show','HEAD:'+path],cwd=ROOT).decode()
    return (ROOT/path).read_text()
def function(text,name):
    start=text.rfind('\n',0,text.index(name+'('))+1
    brace=text.index('{',start);end=brace+1;depth=1
    while depth:depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return text[start:end]
owner=source('combo/menu/ComboItemDrawOOT.h')
resource=source('soh/soh/ResourceManagerHelpers.cpp')
fixture=r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>
#include "combo/menu/ComboItemDrawABI.h"
using s16=int16_t;
struct Color_RGB8 {uint8_t r,g,b;};
constexpr int GID_HEART_PIECE=3,GID_HEART_CONTAINER=4;
#define CVAR_COSMETIC(x) x
#define COMBO_EXPORT
bool changed=false,custom=false,present=true;
int scopes=0;
struct Gfx {int op,r,g,b;};
struct Patch {std::string path,name;int index;Gfx command;};
std::vector<Patch> patches;
std::vector<std::string> removed;
namespace Ship {
 struct CrossRMRegistry {static std::shared_ptr<int> Get(const char* owner) {assert(!strcmp(owner,"oot"));return present?std::make_shared<int>(1):nullptr;}};
 struct ResourceManagerScope {ResourceManagerScope(std::shared_ptr<int>) {++scopes;}~ResourceManagerScope(){--scopes;}};
}
int32_t OOT_MagicJarUsesCustomAsset(const char*) {return custom;}
int CVarGetInteger(const char*,int) {return changed;}
Color_RGB8 CwLiveCosmeticColor(const char*,Color_RGB8) {return {53,167,225};}
#define gsDPSetPrimColor(m,l,r,g,b,a) Gfx{1,r,g,b}
#define gsDPSetEnvColor(r,g,b,a) Gfx{2,r,g,b}
void ResourceMgr_PatchGfxByName(const char* p,const char* name,int index,Gfx command) {assert(scopes==1);patches.push_back({p,name,index,command});}
void ResourceMgr_UnpatchGfxByName(const char*,const char* name) {assert(scopes==1);removed.push_back(name);}
'''
abi=(ROOT/'combo/menu/ComboItemDrawABI.h').read_text()
if 'CW_DRAW_KIND_GRAYSCALE_LAYERS' not in abi:fixture+='constexpr int CW_DRAW_KIND_GRAYSCALE_LAYERS=35;\n'
fixture+=function(owner,'CwLayerPrim')+'\n'
if 'OOT_ApplyGiHeartCosmetics(' in resource:fixture+=function(resource,'OOT_ApplyGiHeartCosmetics')+'\n'
else:fixture+='int32_t OOT_ApplyGiHeartCosmetics(const char*,int32_t,uint8_t,uint8_t,uint8_t,int32_t){return 0;}\n'
if 'OOT_DescribeHeartCosmetics(' in owner:fixture+=function(owner,'OOT_DescribeHeartCosmetics')+'\n'
else:fixture+='void OOT_DescribeHeartCosmetics(s16,CwItemDrawInfo*) {}\n'
fixture+=r'''
int main() {
 for(int id:{GID_HEART_PIECE,GID_HEART_CONTAINER}) for(bool edit:{false,true}) for(bool alt:{false,true}) {
  changed=edit;custom=alt;patches.clear();removed.clear();
  CwItemDrawInfo info{};info.dlistCount=2;info.xluStartIndex=0;
  info.dlists[0]="__OTR__border";info.dlists[1]="__OTR__body";
  OOT_DescribeHeartCosmetics(id,&info);
  assert(info.stateDependent==2 && scopes==0);
  assert(!info.layerEnvMask && info.xluStartIndex==0);
  if(custom) {
   assert(patches.empty()&&removed.empty());
   if(changed) {assert(info.drawKind==CW_DRAW_KIND_GRAYSCALE_LAYERS&&info.layerPrimMask==2);
    assert(info.layerPrimColor[1][0]==53&&info.layerPrimColor[1][1]==167&&info.layerPrimColor[1][2]==225&&info.layerPrimColor[1][3]==255);}
   else assert(!info.layerPrimMask && info.drawKind==CW_DRAW_KIND_SIMPLE);
  } else {
   assert(!info.layerPrimMask && info.drawKind==CW_DRAW_KIND_SIMPLE);
   const char* prim=id==GID_HEART_PIECE?"Consumable_Hearts5":"Consumable_Hearts7";
   const char* env=id==GID_HEART_PIECE?"Consumable_Hearts6":"Consumable_Hearts8";
   if(changed) {assert(removed.empty()&&patches.size()==2);
    assert(patches[0].name==prim&&patches[0].index==2&&patches[0].command.r==53&&patches[0].command.g==167&&patches[0].command.b==225);
    assert(patches[1].name==env&&patches[1].index==6&&patches[1].command.r==26&&patches[1].command.g==83&&patches[1].command.b==112);
    for(auto& patch:patches)assert(patch.path=="__OTR__body");}
   else assert(patches.empty()&&removed==std::vector<std::string>({prim,env}));
  }
 }
 present=false;custom=false;changed=true;patches.clear();removed.clear();CwItemDrawInfo info{};
 info.dlistCount=2;info.dlists[1]="__OTR__body";OOT_DescribeHeartCosmetics(GID_HEART_PIECE,&info);
 assert(patches.empty()&&removed.empty()&&scopes==0);
 info={};OOT_DescribeHeartCosmetics(9,&info);assert(info.stateDependent==0);
}
'''
with tempfile.TemporaryDirectory(prefix='foreign-hearts-') as td:
    path=Path(td)/'test.cpp';path.write_text(fixture);binary=Path(td)/'test'
    flags=['-std=c++20','-Wall','-Wextra','-I'+str(ROOT)]
    if '--sanitize' in sys.argv:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-g']
    subprocess.run([os.environ.get('CXX','c++'),*flags,str(path),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
print('PASS native/custom heart piece/container donor tint, native offsets, border separation and reset')
