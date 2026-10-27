#!/usr/bin/env python3
"""Execute real OoT grotto and cross runtime bodies against native structs without ROMs."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser();parser.add_argument('--sanitize',action='store_true');args=parser.parse_args()
grotto=(ROOT/'soh/soh/Enhancements/randomizer/randomizer_grotto.c').read_text()
grotto=grotto[grotto.index('static const GrottoLoadInfo'):]
runtime=(ROOT/'combo/rando/CrossDungeonOotRuntime.inc.h').read_text()
runtime=runtime[runtime.index('namespace ComboCrossOotRuntime {'):runtime.index('extern "C" void Combo_SOH_AfterNativeLoadGame')]
source=r'''
#include "ComboGameMode.h"
extern "C" { int gComboGameMode = COMBO_GAME_BOTH; }
#include "global.h"
#include "variables.h"
#include "soh/Enhancements/randomizer/randomizerTypes.h"
#include "soh/Enhancements/randomizer/randomizer_grotto.h"
#include "soh/Enhancements/randomizer/randomizer_entrance.h"
#include "rando/CrossDungeonRuntime.h"
#include <cassert>
#include <iostream>
#ifndef COMBO_EXPORT
#define COMBO_EXPORT
#endif
#define SPDLOG_ERROR(...) ((void)0)
#define SPDLOG_INFO(...) ((void)0)
SaveContext gSaveContext{};
EntranceInfo gEntranceTable[ENTR_MAX]{};
PlayState* gPlayState=nullptr;
static std::map<int,u8> settings;
u8 Randomizer_GetSettingValue(RandomizerSettingKey key) { return settings[key]; }
void EntranceTracker_SetCurrentGrottoID(s16) {}
void EntranceTracker_SetLastEntranceOverride(s16) {}
extern "C" void Entrance_SetEntranceDiscovered(uint16_t,uint8_t) {}
extern "C" int32_t Combo_SOH_CrossDungeonRoute(int32_t,int);
extern "C" int Combo_SOH_CrossDungeonRuntimeEnabled(void);
extern "C" int32_t Combo_SOH_GrottoScene(int32_t);
extern "C" int32_t Combo_SOH_CrossDungeonGrottoId(void);
static bool gComboSceneSwitchCallback=true;
void* Combo_ResolveSym(const char*,const char*) { return nullptr; }
'''+grotto+'\n'+runtime+r'''
using namespace ComboRando::CrossDungeon;
using namespace ComboCrossOotRuntime;
int main() {
    PlayState play{}; gPlayState=&play;gSaveContext.fileNum=1;
    Grotto_InitExitAndLoadLists();
    // Disabled parity: native-off does not reinterpret a synthetic ID.
    assert(Grotto_OverrideSpecialEntrance(0x701)==0x701);
    std::vector<Dungeon> grottos;
    for(int i=0;i<3;++i) {
        Dungeon d{"g"+std::to_string(i),{0,0x700+i},{0,0x800+i},{0,0x800+i}};
        d.pool=Pool::Grotto;grottos.push_back(d);
        gEntranceTable[grottoLoadTable[i].entranceIndex].scene=SCENE_GROTTOS;
        gEntranceTable[grottoReturnTable[i].entranceIndex].scene=grottoLoadTable[i].scene;
    }
    auto seed=Encode(grottos,{1,2,0});
    assert(SOH_SetCrossDungeonRuntimeSeed(seed.dump().c_str()));
    Actor hole{};hole.params=grottoLoadTable[0].content;
    play.sceneNum=grottoLoadTable[0].scene;play.nextEntranceIndex=ENTR_LOAD_OPENING;
    Grotto_OverrideActorEntrance(&hole);
    assert(play.nextEntranceIndex==grottoLoadTable[1].entranceIndex);
    assert(gSaveContext.respawn[RESPAWN_MODE_RETURN].data==grottoLoadTable[1].content);
    assert(Grotto_CurrentGrotto()==1);
    ObserveScene(SCENE_GROTTOS);
    assert(Combo_SOH_SaveCrossDungeonCheckpoint()==0x701); // No synthetic entrance table indexing.
    ObserveScene(SCENE_GROTTOS);
    assert(Combo_SOH_SaveCrossDungeonCheckpoint()==0x701);
    // One native permutation, one cross leg: native0->1 then cross1->2.
    Grotto_SetLoadOverride(0x700,0x701);
    Grotto_OverrideActorEntrance(&hole);
    assert(play.nextEntranceIndex==grottoLoadTable[2].entranceIndex);
    assert(gSaveContext.respawn[RESPAWN_MODE_RETURN].data==grottoLoadTable[2].content);
    ObserveScene(SCENE_GROTTOS);
    assert(Combo_SOH_SaveCrossDungeonCheckpoint()==0x702);
    Grotto_SetExitOverride(0x802,0x800);
    play.nextEntranceIndex=ENTR_GROTTOS_0;
    const int exited=Grotto_OverrideSpecialEntrance(ENTR_RETURN_GROTTO);
    assert(exited==grottoReturnTable[1].entranceIndex); // Cross reverse precedes native exit lookup.
    assert(gSaveContext.respawn[RESPAWN_MODE_RETURN].pos.x==grottoReturnTable[1].pos.x);
    ObserveScene(grottoLoadTable[1].scene);
    assert(Combo_SOH_SaveCrossDungeonCheckpoint()==-1);
    Combo_SOH_LoadCrossDungeonCheckpoint(0x701);
    assert(Combo_SOH_ConsumeCrossDungeonRecovery()==grottoLoadTable[1].entranceIndex);
    ObserveScene(SCENE_GROTTOS);
    assert(Combo_SOH_SaveCrossDungeonCheckpoint()==0x701);
    // A queued peer arrival supersedes any unconsumed local route to the same generic scene.
    Combo_SOH_CrossDungeonRoute(0x700,false);
    assert(SOH_QueueCrossDungeonArrival(0x702));
    assert(Combo_SOH_ConsumeCrossDungeonArrival()==grottoLoadTable[2].entranceIndex);
    ObserveScene(SCENE_GROTTOS);
    assert(Combo_SOH_SaveCrossDungeonCheckpoint()==0x702);
    // Farore's native real-index transition clears transient grotto ID, but not logical ownership.
    Combo_SOH_RestoreCrossDungeonCheckpoint(0x701);
    Grotto_OverrideSpecialEntrance(grottoLoadTable[1].entranceIndex);
    play.sceneNum=SCENE_GROTTOS;
    ObserveScene(SCENE_GROTTOS);
    assert(Combo_SOH_SaveCrossDungeonCheckpoint()==0x701);
    assert(Grotto_CurrentGrotto()==1);
    play.nextEntranceIndex=ENTR_GROTTOS_0;
    assert(Grotto_OverrideSpecialEntrance(ENTR_RETURN_GROTTO)==grottoReturnTable[0].entranceIndex);
    // Interior ownership lasts only inside that interior, even if other scenes are visited.
    Dungeon a{"a",{0,ENTR_MIDOS_HOUSE_0},{0,ENTR_KOKIRI_FOREST_OUTSIDE_MIDOS_HOUSE},{0,ENTR_KOKIRI_FOREST_OUTSIDE_MIDOS_HOUSE}};
    Dungeon b{"b",{0,ENTR_LINKS_HOUSE_1},{0,ENTR_KOKIRI_FOREST_OUTSIDE_LINKS_HOUSE},{0,ENTR_KOKIRI_FOREST_OUTSIDE_LINKS_HOUSE}};
    for (auto pool : {Pool::Interior,static_cast<Pool>(4),static_cast<Pool>(5)}) {
    a.pool=b.pool=pool;
    gEntranceTable[a.entrance.index].scene=SCENE_MIDOS_HOUSE;
    gEntranceTable[b.entrance.index].scene=SCENE_LINKS_HOUSE;
    seed=Encode({a,b},{1,0});assert(SOH_SetCrossDungeonRuntimeSeed(seed.dump().c_str()));
    assert(Combo_SOH_CrossDungeonRoute(a.entrance.index,false)==b.entrance.index);
    ObserveScene(SCENE_LINKS_HOUSE);
    assert(Combo_SOH_SaveCrossDungeonCheckpoint()==b.entrance.index);
    ObserveScene(SCENE_MIDOS_HOUSE);
    assert(Combo_SOH_SaveCrossDungeonCheckpoint()==-1);
    // Accepted Farore restore cancels a pending local arrival that would overwrite its ownership.
    Combo_SOH_CrossDungeonRoute(a.entrance.index,false);
    Combo_SOH_RestoreCrossDungeonCheckpoint(a.entrance.index);
    ObserveScene(SCENE_MIDOS_HOUSE);
    assert(Combo_SOH_SaveCrossDungeonCheckpoint()==a.entrance.index);
    }
    // An active unrelated pool must preserve vanilla physical grotto return data on no route.
    Grotto_InitExitAndLoadLists();
    play.sceneNum=grottoLoadTable[0].scene;play.nextEntranceIndex=ENTR_LOAD_OPENING;
    gSaveContext.respawn[RESPAWN_MODE_RETURN].entranceIndex=123;
    gSaveContext.respawn[RESPAWN_MODE_RETURN].pos.x=777;
    Grotto_OverrideActorEntrance(&hole);
    play.nextEntranceIndex=ENTR_GROTTOS_0;
    assert(Grotto_OverrideSpecialEntrance(ENTR_RETURN_GROTTO)==123);
    assert(gSaveContext.respawn[RESPAWN_MODE_RETURN].pos.x==777);
    // Native-off ordinary Farore returns must not arm stale grotto-return state.
    Grotto_OverrideSpecialEntrance(ENTR_MIDOS_HOUSE_0);
    gSaveContext.fw.playerParams=0x6FF;
    gSaveContext.respawnFlag=0;
    Grotto_SetupReturnInfoOnFWReturn();
    Grotto_ForceGrottoReturn();
    assert(gSaveContext.respawnFlag==0);
    // Boss reverse has an explicit owning dungeon checkpoint absent from boss-only catalogue.
    a={"ba",{0,10},{0,11},{0,12}};b={"bb",{0,20},{0,21},{0,22}};
    a.pool=b.pool=Pool::Boss;a.boss=0;b.boss=1;a.owner="owner-a";b.owner="owner-b";
    a.warpReturn=Endpoint{0,30};b.warpReturn=Endpoint{0,40};
    a.reverseCheckpoint=Endpoint{0,123};b.reverseCheckpoint=Endpoint{0,124};
    a.warpCheckpoint=a.reverseCheckpoint;b.warpCheckpoint=b.reverseCheckpoint;
    seed=Encode({a,b},{1,0});assert(SOH_SetCrossDungeonRuntimeSeed(seed.dump().c_str()));
    gEntranceTable[12].scene=gEntranceTable[123].scene=SCENE_DEKU_TREE;
    assert(Combo_SOH_CrossDungeonRoute(21,true)==12);
    ObserveScene(SCENE_DEKU_TREE);
    assert(Combo_SOH_SaveCrossDungeonCheckpoint()==123);
    Combo_SOH_LoadCrossDungeonCheckpoint(123);
    assert(Combo_SOH_ConsumeCrossDungeonRecovery()==123);
    assert(Combo_SOH_SaveCrossDungeonCheckpoint()==123);
    std::cout<<"OoT pool runtime: native-off grotto loading, single composition, physical reverse, synthetic recovery, interior scopes, boss owner checkpoint pass\n";
}
'''
with tempfile.TemporaryDirectory(prefix='oot-pool-runtime-') as tmp:
    path=Path(tmp);(path/'test.cpp').write_text(source)
    cmd=[os.environ.get('CXX','c++'),'-std=c++20','-O1','-Wno-narrowing','-DCOMBO_BUILD','-DF3DEX_GBI_2','-DCONTROLLERBUTTONS_T=uint32_t','-DLOG_LEVEL_GAME_PRINTS=0']
    for p in ('combo','soh','soh/include','soh/include/PR','soh/src','soh/assets','soh/mods','soh/soh','libultraship/include'):
        cmd+=['-I'+str(ROOT/p)]
    cmd+=['-I'+str(ROOT.parent/'build-deps/usr/include')]
    if args.sanitize:cmd+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-g']
    subprocess.run(cmd+[str(path/'test.cpp'),'-o',str(path/'test')],check=True)
    subprocess.run([str(path/'test')],check=True)
