"""Run MM's production legacy-GI resource loader against a changing archive selection."""
from pathlib import Path
import os, subprocess, sys, tempfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_time_pedestal_tests import functions
source=functions((ROOT/'mm/2s2h/Rando/DrawItem.cpp').read_text())
legacy=source.get('LoadNeiLegacyGfx','')
body=source['LoadNeiRealGfx']
callbacks="\n".join(source[name] for name in ['DrawNeiRealOpa','DrawNeiRealOpaXlu','DrawOotNeiFireRod','DrawOotNeiIceRod'])
prefix=r'''
#include <cassert>
#include <cstdint>
#include <map>
#include <string>
#include <iostream>
void NeiGi_ReportBlackMarketModel(const char*,const char*,const char* = nullptr) {}
#include <vector>
using u8=uint8_t;using f32=float;
constexpr int MTXMODE_APPLY=1;
struct PlayState{struct {void* gfxCtx;} state;};
PlayState play{};PlayState* gPlayState=&play;
std::vector<int> draws;int fallbackDraws=0;
#define OPEN_DISPS(ctx)
#define CLOSE_DISPS(ctx)
#define POLY_OPA_DISP ((Gfx*)nullptr)
#define POLY_XLU_DISP ((Gfx*)nullptr)
#define MATRIX_FINALIZE_AND_LOAD(pkt,ctx)
#define gSPDisplayList(pkt,dl) draws.push_back((dl)->id)
void Gfx_SetupDL25_Opa(void*){}
void Gfx_SetupDL25_Xlu(void*){}
void Matrix_Scale(float,float,float,int){}
void Matrix_RotateZF(float,int){}
struct Gfx {int id;};
std::map<std::string,Gfx> base,alt;bool activeAlt=false,blocked=false,mod=false;
Gfx companion{999};int directCalls=0;
u8 ResourceMgr_FileExists(const char* p){return base.contains(p);}
u8 ResourceMgr_FileAltExists(const char* p){return alt.contains(p);}
bool ResourceMgr_IsAltAssetsEnabled(){return activeAlt;}
int ResourceMgr_IsModAsset(const char*){return mod;}
u8 OotAssets_PathAllowed(const char*){return !blocked;}
Gfx* ResourceMgr_LoadGfxByName(const char* p){
 if(activeAlt&&alt.contains(p))return &alt.at(p);
 return base.contains(p)?&base.at(p):nullptr;
}
void* OotAssets_LoadGfx(const char* p){return ResourceMgr_LoadGfxByName(p);}
void* OotAssets_LoadGfxDirect(const char*){++directCalls;return &companion;}
'''
prefix+='void DrawOotRodStandIn(Gfx**,u8,u8,u8){++fallbackDraws;}\n'
checks=r'''
int main(){
 const char* p="__OTR__objects/object_nei_fire_rod/Cylinder_001_opaque_dl";
 Gfx* cache=nullptr;u8 tried=0;
 assert(!LoadNeiRealGfx(p,&cache,&tried));
 base[p]={1};assert(LoadNeiRealGfx(p,&cache,&tried)==&base.at(p));
 alt[p]={2};activeAlt=true;assert(LoadNeiRealGfx(p,&cache,&tried)==&alt.at(p));
 activeAlt=false;assert(LoadNeiRealGfx(p,&cache,&tried)==&base.at(p));
 base.clear();activeAlt=true;assert(LoadNeiRealGfx(p,&cache,&tried)==&alt.at(p));
 activeAlt=false;assert(!LoadNeiRealGfx(p,&cache,&tried));
 activeAlt=true;blocked=true;assert(!LoadNeiRealGfx(p,&cache,&tried));blocked=false;
'''
if legacy:
 checks+=r'''
 // The direct companion loader must never bypass a winning MM-local base/Alt mod.
 mod=true;assert(LoadNeiLegacyGfx(p,true)==&alt.at(p)&&directCalls==0);
 activeAlt=false;base[p]={3};assert(LoadNeiLegacyGfx(p,true)==&base.at(p)&&directCalls==0);
 mod=false;assert(LoadNeiLegacyGfx(p,true)==&companion&&directCalls==1);
 assert(LoadNeiLegacyGfx(p,false)==&base.at(p));
 activeAlt=true;assert(LoadNeiLegacyGfx(p,false)==&alt.at(p));
'''
checks+=r'''
 // Real fire and split ice GI callbacks reuse their static cache variables
 // across missing resources and live Alt changes; each frame must draw current.
 base.clear();alt.clear();activeAlt=false;DrawOotNeiFireRod();assert(fallbackDraws==1);
 base[p]={4};DrawOotNeiFireRod();assert(draws.back()==4);
 alt[p]={5};activeAlt=true;DrawOotNeiFireRod();assert(draws.back()==5);
 activeAlt=false;DrawOotNeiFireRod();assert(draws.back()==4);
 const char* ice="__OTR__objects/object_nei_ice_rod/ice_rod_opaque_dl";
 const char* glass="__OTR__objects/object_nei_ice_rod/ice_rod_transparent_dl";
 alt[ice]={6};alt[glass]={7};activeAlt=true;DrawOotNeiIceRod();
 assert(draws[draws.size()-2]==6&&draws.back()==7);
 alt.erase(glass);const auto count=draws.size();DrawOotNeiIceRod();
 assert(draws.size()==count&&fallbackDraws==2);
'''
checks+='std::cout<<"PASS MM production legacy GI loaders: missing/present, Alt-only, live toggles, blocked paths and local-mod precedence\\n";}'
with tempfile.TemporaryDirectory(prefix='mm-gi-load-') as td:
 p=Path(td)/'test.cpp';p.write_text(prefix+legacy+body+callbacks+checks)
 out=Path(td)/'test';subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',str(p),'-o',str(out)],check=True)
 subprocess.run([str(out)],check=True)
