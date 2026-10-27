"""Execute unchanged production launch/draw-phase/owner-query function bodies.

Unrelated unity gameplay dependencies are replaced at the collision and graphics
boundaries; all MM state types and item constants are the production headers.
"""
import os
import re
from pathlib import Path
import subprocess
import sys
import tempfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_mm_nei_tests import flags
from run_time_pedestal_tests import functions
prefix='''
#include "mods/items/custom_items.h"
#include "mods/items/logic/item_rod_common.h"
#include "mods/items/logic/item_rod_fire.h"
#include "mods/items/logic/item_rod_ice.h"
#include "mods/items/logic/item_rod_light.h"
#include <cassert>
#include <iostream>
#include <vector>
CustomItemState gCustomItemState{};
static RodProjSet slots[2];
static u32 sFireDrawEpoch, sIceDrawEpoch;
static RodConfig sFireRodConfig{};
static RodProjSet* FireRod_FindFreeSet(PlayState*) { return &slots[0]; }
static RodProjSet* IceRod_FindFreeSet(PlayState*) { return &slots[1]; }
static void FireRod_InitSetColliders(RodProjSet*, Player*, PlayState*) {}
static void IceRod_InitSetColliders(RodProjSet*, Player*, PlayState*) {}
void RodCommon_CalcVelocity(const RodConfig*, Vec3f*, s16, s16) {}
static void IceRod_CalcVelocity(Vec3f*, s16, s16) {}
std::vector<int> spins,charges,bursts;
void NeiUsedMagic_DrawSpin(PlayState*, Player*, int e, float radius, bool big) { assert(radius==123 && big); spins.push_back(e); }
void NeiUsedMagic_DrawCharge(PlayState*, Player*, int e, float charge) { assert(charge==.5f); charges.push_back(e); }
void NeiUsedMagic_DrawBurst(PlayState*, int e, const Vec3f*, float size, float life) { assert(size>0 && life==.5f); bursts.push_back(e); }
'''
parts=[]
for element in ['fire','ice','light']:
    source=(ROOT/f'mm/mods/items/logic/item_rod_{element}.c').read_text();f=functions(source)
    prefix += '\n'.join(re.findall(r'^#define \w+ gCustomItemState\.\w+.*$',source,re.M))+'\n'
    cap=element.capitalize()
    if element!='light':
        parts += [f[cap+'Rod_InitSingleProjectile'],f[cap+'Rod_InitTripleProjectile']]
    parts.append(f['CustomItems_Draw'+cap+'RodEffects'])
    # Graphics must stay exclusively in the draw-phase function.
    for name,body in f.items():
        if not name.startswith('CustomItems_Draw'):
            assert 'NeiUsedMagic_Draw' not in body, (name,'rendering during update')
checks='''
int main(){
 Player p{};PlayState play{};Vec3f start{10,20,30};
 for(int i=1;i<8;i++){
   FireRod_InitSingleProjectile(&p,&play,&start,100,200,1000);IceRod_InitSingleProjectile(&p,&play,&start,100,200,1000);
   for(auto& slot:slots){assert(slot.drawEpoch==unsigned(i*2-1));assert(slot.active&&slot.count==1&&slot.yaw==100&&slot.pitch==200);}
   FireRod_InitTripleProjectile(&p,&play,&start,300,400);IceRod_InitTripleProjectile(&p,&play,&start,300,400);
   for(auto& slot:slots){assert(slot.drawEpoch==unsigned(i*2));assert(slot.active&&slot.count==3&&slot.yaw==300&&slot.pitch==400);}
 }
 void(*draw[])(Player*,PlayState*)={CustomItems_DrawFireRodEffects,CustomItems_DrawIceRodEffects,CustomItems_DrawLightRodEffects};
 for(auto fn:draw)fn(&p,&play);assert(spins.empty()&&charges.empty()&&bursts.empty());
 fireRodSpinActive=iceRodSpinActive=lightRodSpinActive=1;
 fireRodCharging=iceRodCharging=lightRodCharging=1;
 fireRodSpinIsBig=iceRodSpinIsBig=lightRodSpinIsBig=1;
 fireRodSpinRadius=iceRodSpinRadius=lightRodSpinRadius=123;
 fireRodChargeLevel=iceRodChargeLevel=lightRodChargeLevel=.5f;
 iceRodWaveActive=lightRodBeamActive=1;
 iceRodWaveTimer=lightRodBeamTimer=15;
 CustomItemState before=gCustomItemState;
 for(auto fn:draw)fn(&p,&play);
 assert((spins==std::vector<int>{0,1,2})&&(charges==std::vector<int>{0,1,2}));
 assert(bursts.size()==ICE_ROD_WAVE_COUNT+LIGHT_ROD_BEAM_COUNT);
 assert(memcmp(&before,&gCustomItemState,sizeof(before))==0);
 std::cout<<"PASS native MM single/triple launch/reuse epochs, draw-phase charge/spin/burst dispatch and unchanged gameplay state\\n";
}
'''
owner=(ROOT/'soh/soh/ResourceManagerHelpers.cpp').read_text()
ownerfn=functions(owner.replace('extern "C" COMBO_EXPORT ','')).get('OOT_NeiResourceExists')
assert ownerfn
ownerprefix='''#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <set>
#include <iostream>
std::set<std::string> base,alt; bool registered=true,stockRegistered=true,activeMmAlt=false;
namespace Ship {
struct Archive {
 bool HasFile(const std::string& path) {
  return path.starts_with("alt/") ? alt.contains("__OTR__"+path.substr(4)) : base.contains("__OTR__"+path);
 }
};
struct ResourceManager {
 bool alt=false; bool IsAltAssetsEnabled(){return alt;}
 std::shared_ptr<Archive> GetArchiveManager(){return std::make_shared<Archive>();}
 bool LoadResource(const char* path,bool exact){assert(exact);return base.contains(std::string("__OTR__")+path);}
};
std::shared_ptr<ResourceManager> owner=std::make_shared<ResourceManager>();
struct CrossRMRegistry {
  static std::shared_ptr<ResourceManager> Get(const char* name){
    if(std::string(name)=="oot-gi-base")return stockRegistered?owner:nullptr;
    assert(std::string(name)=="oot");return registered?owner:nullptr;
  }
};
}
// OoT's legacy ExtensionCache is cold during this MM-first owner query.
bool ResourceMgr_FileExists(const char*){return false;}
bool ResourceMgr_FileAltExists(const char*){return false;}
'''
ownerchecks='''int main(){
 const char* p="__OTR__objects/nei/test"; assert(!OOT_NeiResourceExists(nullptr));assert(!OOT_NeiResourceExists(p));
 alt.insert(p);activeMmAlt=true;assert(!OOT_NeiResourceExists(p));Ship::owner->alt=true;activeMmAlt=false;assert(OOT_NeiResourceExists(p));
 alt.clear();assert(!OOT_NeiResourceExists(p));base.insert(p);Ship::owner->alt=false;assert(OOT_NeiResourceExists(p));registered=false;assert(!OOT_NeiResourceExists(p));
 const char* stock="__OTR__@oot-gi-base:objects/nei/test";
 assert(OOT_NeiResourceExists(stock));base.clear();alt.insert(p);Ship::owner->alt=true;
 assert(!OOT_NeiResourceExists(stock));base.insert(p);stockRegistered=false;assert(!OOT_NeiResourceExists(stock));
 std::cout<<"PASS actual owner export: inactive owner, divergent MM/OoT Alt, exact shipped lookup, late absence and missing registration\\n";
}'''
with tempfile.TemporaryDirectory(prefix='mm-nei-dispatch-') as td:
    for name,source in [('dispatch','#include <cstring>\n'+prefix+'\n'.join(parts)+checks),('owner',ownerprefix+ownerfn+ownerchecks)]:
        path=Path(td)/(name+'.cpp');path.write_text(source);binary=Path(td)/name
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++20',*flags(),str(path),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)
