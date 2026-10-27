#!/usr/bin/env python3
"""Execute the launcher's real slot-persistence/notification function at native boundaries."""
import argparse
from pathlib import Path
import subprocess
import tempfile

p=argparse.ArgumentParser()
p.add_argument('--sanitize',action='store_true')
a=p.parse_args()
root=Path(__file__).resolve().parents[2]
source=(root/'combo/ComboShip.cpp').read_text()
begin=source.index('static void Combo_OnCrossBossDefeated(')
end=source.index('// Generation pushes',begin)
fixture=r'''
#include "combo/rando/CrossDungeonCoordinator.h"
#include <cassert>
#include <iostream>
#include <mutex>
using namespace ComboRando::CrossDungeon;
static Coordinator g_CrossDungeons;
static int g_comboCompletionSlot=0,g_MmSaveInMemorySlot=0;
static uint16_t g_crossBossClears=0,hostMasks[2]={};
static std::mutex g_containerMutex;
static std::map<int,nlohmann::json> containers;
static std::vector<int> effects;
static int flushes=0,loads=0;
static bool loadFails=false;
static bool ComboIsValidSlot(int slot){return slot>=0&&slot<3;}
static nlohmann::json& LoadOrCreateContainer(int slot){return containers[slot];}
static void FlushContainer(int){++flushes;}
static void SetOot(uint16_t mask){hostMasks[0]=mask;}
static void SetMm(uint16_t mask){hostMasks[1]=mask;}
static void Effect(int source,int dormant){
    assert(g_containerMutex.try_lock());g_containerMutex.unlock();
    assert(dormant); // This fixture exchanges both bosses across games.
    effects.push_back(source);
}
static int LoadMm(int slot){++loads;assert(slot==g_comboCompletionSlot);return loadFails?1:0;}
static int (*MM_LoadSaveForCombo)(int)=LoadMm;
static void (*g_SetCrossBossMask[2])(uint16_t)={SetOot,SetMm};
static void (*g_ApplyCrossBossClear[2])(int,int)={Effect,Effect};
'''+source[begin:end]+r'''
int main(){
    Dungeon oot{"oot:b",{0,110},{0,111},{0,112},{{0,120}}};
    Dungeon mm{"mm:b",{1,210},{1,211},{1,212},{{1,220}}};
    oot.pool=mm.pool=Pool::Boss;oot.boss=0;mm.boss=8;
    oot.owner="oot:d";mm.owner="mm:d";
    oot.warpReturn=Endpoint{0,130};mm.warpReturn=Endpoint{1,230};
    const auto seed=Encode({oot,mm},{1,0});
    for(auto& host:g_CrossDungeons.hosts)host.SetRuntimeSeed=[](const char*){return 1;};
    g_CrossDungeons.BindRuntime({{"crossDungeonEntrances",seed}});
    containers[0]={{"oot",{{"inventory",1}}},{"mm",{{"inventory",2}}}};
    Combo_OnCrossBossDefeated(0);
    assert(effects==std::vector<int>{8});
    assert(containers[0]["combo"]["crossBossClears"]==(1u<<8));
    assert(hostMasks[0]==(1u<<8)&&hostMasks[1]==(1u<<8));
    assert(containers[0]["oot"]["inventory"]==1&&containers[0]["mm"]["inventory"]==2);
    Combo_OnCrossBossDefeated(0); // Repeat after MM reset reapplies source effect.
    assert(effects.size()==2&&effects.back()==8&&g_crossBossClears==(1u<<8));
    Combo_OnCrossBossDefeated(8);
    assert(effects.back()==0&&g_crossBossClears==((1u<<8)|1));
    const auto previous=containers;
    Combo_OnCrossBossDefeated(-1);Combo_OnCrossBossDefeated(12);
    g_comboCompletionSlot=-1;Combo_OnCrossBossDefeated(0);
    assert(containers==previous);
    // Loading the dormant owner occurs before either persistence or a native effect.
    g_comboCompletionSlot=1;g_crossBossClears=0;g_MmSaveInMemorySlot=0;
    loadFails=true;const auto priorEffects=effects.size();
    Combo_OnCrossBossDefeated(0);
    assert(g_crossBossClears==0&&effects.size()==priorEffects&&!containers.contains(1));
    loadFails=false;Combo_OnCrossBossDefeated(0);
    assert(loads==2&&g_MmSaveInMemorySlot==1&&containers[1]["combo"]["crossBossClears"]==(1u<<8));
    assert(containers[0]==previous.at(0));
    g_CrossDungeons.BindRuntime(nullptr);
    const int before=flushes;Combo_OnCrossBossDefeated(0);
    assert(flushes==before);
    std::cout<<"launcher bosses: slot persistence, repeat-cycle effects, dormant ownership and disabled isolation PASS\n";
}
'''
with tempfile.TemporaryDirectory(prefix='boss-launcher-') as directory:
    cpp=Path(directory)/'test.cpp';binary=Path(directory)/'test'
    cpp.write_text(fixture)
    flags=['-std=c++20','-Wall','-Wextra','-Werror','-I',str(root),'-I',str(root.parent/'build-deps/usr/include')]
    if a.sanitize:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-g']
    subprocess.run(['g++',*flags,str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
