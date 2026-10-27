#!/usr/bin/env python3
"""Exercise actual MM host adapter functions with native constants and narrow engine boundaries."""
import argparse
import pathlib
import re
import subprocess
import tempfile

ROOT=pathlib.Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser()
parser.add_argument('--sanitize',action='store_true')
args=parser.parse_args()
oracle=(ROOT/'combo/rando/CrossDungeonMm.inc.h').read_text()
runtime=(ROOT/'combo/rando/CrossDungeonMmRuntime.inc.h').read_text()
logic=(ROOT/'mm/2s2h/Rando/Logic/Logic.cpp').read_text()
boss=(ROOT/'combo/rando/CrossEntranceMmBoss.inc.h').read_text()
assert 'MM_DumpCrossDungeonCatalogueForOptions' in oracle, 'MM options catalogue export missing'
assert 'MM_CanEnterCrossEntrance' in oracle, 'MM native time admission gate missing'
assert 'Combo_MM_CrossDungeonEntranceFromRegion(exitId, currentRegion)' in logic, 'MM traversal lacks source region context'

def block(text, marker):
    start=text.index(marker);opening=text.index('{',start);depth=1;end=opening+1
    while depth:
        depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return text[start:end]

# Reuse native enum/respawn boundary declarations, without executing the other runner.
base=ROOT/'tests/cross_dungeon/mm_catalogue_behavior_test.py'
env={'__file__':str(base)}
exec(compile(base.read_text().split('# Compile the native respawn table')[0],str(base),'exec'),env)
constants=env['constants']
oot_entrances=(ROOT/'soh/include/tables/entrance_table.h').read_text()
for value,name in re.findall(r'/\*\s*(0x[0-9A-Fa-f]+)\s*\*/\s*DEFINE_ENTRANCE\((ENTR_\w+)',oot_entrances):
    if name in {'ENTR_BAZAAR_1','ENTR_MARKET_DAY_OUTSIDE_BAZAAR','ENTR_POTION_SHOP_KAKARIKO_FRONT',
                'ENTR_KAKARIKO_VILLAGE_OUTSIDE_POTION_SHOP_FRONT','ENTR_POTION_SHOP_KAKARIKO_BACK',
                'ENTR_KAKARIKO_VILLAGE_OUTSIDE_POTION_SHOP_BACK'}:
        constants+=f'\nconstexpr int OOT_{name}={value};'

constants=constants.replace('int respawnFlag=0;', 'int gameMode=0,fileNum=0,nextCutsceneIndex=0xFFEF,nextTransitionType=0; int respawnFlag=0;')
transitions=(ROOT/'mm/include/z64transition.h').read_text()
fade_black=re.search(r'/\*\s*(\d+)\s*\*/\s*TRANS_TYPE_FADE_BLACK,',transitions).group(1)
constants+='\nconstexpr int TRANS_TYPE_FADE_BLACK='+fade_black+';\n'
constants=constants.replace('struct { int entrance=0; } save;', 'struct { int entrance=0,cutsceneIndex=0; bool isOwlSave=false; struct { int pauseSaveEntrance=-1; } shipSaveInfo; } save;')
scenes=(ROOT/'mm/include/tables/scene_table.h').read_text()
for value,name in re.findall(r'/\*\s*(0x[0-9a-fA-F]+)\s*\*/\s*DEFINE_SCENE\([^,]+,\s*(SCENE_\w+)',scenes):
    if name!='SCENE_KAKUSIANA':constants+=f'\nconstexpr int {name}={value};'
program='''
#include <cassert>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <unordered_map>
#define COMBO_EXPORT
#define COMBO_BUILD
#include "combo/ComboGameMode.h"
extern "C" { int gComboGameMode = COMBO_GAME_BOTH; }
#define SPDLOG_ERROR(...) ((void)0)
#define SPDLOG_INFO(...) ((void)0)
using s32=int32_t;
''' .replace('using s32=int32_t;','#include <cstdint>\nusing s32=int32_t;')+constants+'''
#include "combo/rando/CrossDungeonState.h"
#include "combo/rando/CrossEntranceMmCatalogue.h"
#include "combo/rando/CrossEntranceMmGrottos.inc.h"
#include "combo/rando/CrossEntranceTime.h"
#include "combo/rando/CrossEntranceBoss.h"
constexpr int GAMEMODE_NORMAL=0,ONE_WAY_EXIT=-1;
bool canPlayTime=false,clockShuffle=false;uint8_t rawClocks=0;
#define CAN_PLAY_SONG(song) (canPlayTime)
namespace Rando::Logic {
struct Exit { int returnEntrance; std::function<bool()> condition=[] {return true;}; };
struct Region { int sceneId=0;std::map<int,Exit> exits;std::set<int> oneWayEntrances; };
struct RegionTimeState {};
std::map<RandoRegionId,Region> Regions;
bool SettingClocks(){return clockShuffle;}
bool OwnsClockHalfDay(int i){return rawClocks&(1u<<i);}
void SetCurrentRegionTime(const std::unordered_map<RandoRegionId,RegionTimeState>&,RandoRegionId){}
'''+block(logic,'RandoRegionId GetRegionIdFromEntrance')+'''
}
int Entrance_GetSceneId(int index){
    assert(index>=0 && index<0xF000);
    auto region=Rando::Logic::GetRegionIdFromEntrance(index);
    return Rando::Logic::Regions[region].sceneId;
}
int queued=-1;
int QueuePeer(int index){queued=index;return 1;}
void ReturnPeer(int){}
void (*gComboReturnCallback)(int)=ReturnPeer;
void* Combo_ResolveSym(const char*,const char*){return reinterpret_cast<void*>(QueuePeer);}
extern "C" int Combo_MM_GiantsEscape(int32_t*,int32_t*,int){return 0;}
'''+runtime+'''
namespace ComboCrossMm {
using namespace ComboRando::CrossDungeon;
static HostState state(1);static bool searching=false;
static BossCompletionState bossCompletion;
static std::set<RandoRegionId> reachable;
static std::unordered_map<RandoRegionId,Rando::Logic::RegionTimeState> timeStates;
'''+re.search(r'static const int32_t bossEntrances\[4\] = \{.*?\};',boss,re.S).group(0)+'\n'+block(boss,'static void AppendBossCatalogue(')+'''
'''+block(boss,'static bool BossLogicEnabled()')+'\n'+block(oracle,'static std::vector<Dungeon> Catalogue(')+'''
}
'''
for marker in ['extern "C" int Combo_MM_CrossDungeonEntranceFromRegion', 'extern "C" int Combo_MM_CrossDungeonEntrance(',
               'extern "C" COMBO_EXPORT const char* MM_DumpCrossDungeonCatalogueForOptions',
               'extern "C" COMBO_EXPORT const char* MM_DumpCrossDungeonCatalogueForSeed',
               'extern "C" COMBO_EXPORT const char* MM_DumpCrossDungeonCatalogue(void)',
               'extern "C" COMBO_EXPORT int MM_CanEnterCrossEntrance',
               'extern "C" COMBO_EXPORT int MM_SetCrossDungeonSeed',
               'extern "C" COMBO_EXPORT const char* MM_GetCrossDungeonExits']:
    program+='\n'+block(oracle,marker)+'\n'
program+='\n'+block(boss,'extern "C" int Combo_MM_CrossBossLogicalClear')+'\n'
title=(ROOT/'mm/src/code/title_setup.c').read_text()
# Keep this fixture scoped to entrance recovery, before the independently tested
# Farore/title-card composition. The recall call now stores its return value.
title_end=title.rfind('\n',0,title.index('ComboFw_ApplyArrival()'))
title_block=title[title.index('        s32 crossDungeonArrival ='):title_end]
program+='\nstatic bool gComboEntryIsResume=false;\nvoid NativeTitleArrival() {\n'+title_block+'\n}\n'
# Execute the actual late native player-start override after title materialization.
# Routing/respawn assertions alone cannot detect a saved-location marker replacing
# the grotto jump with a stationary spawn over the same hole.
player=(ROOT/'mm/src/overlays/actors/ovl_player_actor/z_player.c').read_text()
pause_start=block(player,'    if (gSaveContext.save.shipSaveInfo.pauseSaveEntrance != -1)')
program+='\nint NativePlayerStartMode(int startMode) {\n'+pause_start+'\nreturn startMode;\n}\n'
# Seed validation must use the native one-way arrival declarations too: boss
# completion aliases can be absent from the ordinary paired EXIT index.
native_oneways=[]
native_exits=[]
for path in sorted((ROOT/'mm/2s2h/Rando/Logic/Regions').glob('*.cpp')):
    source=path.read_text()
    for match in re.finditer(r'Regions\[(RR_\w+)\]\s*=\s*RandoRegion\s*\{',source):
        region=block(source,match.group(0))
        exits=re.search(r'\.exits\s*=\s*\{(.*?)\n\s*\},',region,re.S)
        if exits:
            for to,back in re.findall(r'EXIT\(\s*(ENTRANCE\(\w+,\s*\d+\)),\s*(ENTRANCE\(\w+,\s*\d+\)|ONE_WAY_EXIT)',exits.group(1)):
                native_exits.append(f'Rando::Logic::Regions[{match.group(1)}].exits[{to}]={{{back}}};')
        one_way=re.search(r'\.oneWayEntrances\s*=\s*\{(.*?)\}',region,re.S)
        if one_way:
            for entrance in re.findall(r'ENTRANCE\(\w+,\s*\d+\)',one_way.group(1)):
                native_oneways.append(f'Rando::Logic::Regions[{match.group(1)}].oneWayEntrances.insert({entrance});')
program+='\nvoid CheckNativeBossCatalogue() {\nusing namespace ComboRando::CrossDungeon;\n'+'\n'.join(native_exits+native_oneways)+r'''
    PoolOptions options;options.dungeons=false;options.bosses=true;
    auto entries=ComboCrossMm::Catalogue(options);
    Dungeon oot{"oot:boss",{0,100},{0,101},{0,101}};
    oot.pool=Pool::Boss;oot.boss=0;oot.owner="oot:parent";oot.warpReturn=Endpoint{0,102};
    entries.push_back(oot);
    std::vector<size_t> permutation(entries.size());
    std::iota(permutation.begin(),permutation.end(),0);
    for(size_t boss=0;boss<4;++boss) {
        auto swap=permutation;std::swap(swap[boss],swap.back());
        auto seed=Encode(entries,swap);
        for(const auto& route:Decode(seed)) {
            for(auto endpoint:{route.from,route.to})
                if(endpoint.game==1 && Rando::Logic::GetRegionIdFromEntrance(endpoint.index)==RR_MAX)
                    std::cerr<<"native boss endpoint missing: "<<endpoint.index<<" boss "<<boss<<"\n";
        }
        assert(MM_SetCrossDungeonSeed(seed.dump().c_str()));
    }
    assert(MM_SetCrossDungeonSeed("null"));
}
'''
program+=(ROOT/'tests/cross_dungeon/mm_adapter_cases.inc').read_text().replace(
    '    InitRegions();','    InitRegions();\n    CheckNativeBossCatalogue();')
with tempfile.TemporaryDirectory(prefix='mm-host-adapter-') as tmp:
    path=pathlib.Path(tmp);(path/'test.cpp').write_text(program)
    cmd=['g++','-std=c++20','-Wall','-Wextra','-Werror','-Wno-unused-function','-I',str(ROOT),'-I',str(ROOT/'combo'),'-I',str(ROOT.parent/'build-deps/usr/include'),str(path/'test.cpp'),'-o',str(path/'test')]
    if args.sanitize:cmd+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-g']
    subprocess.run(cmd,check=True);subprocess.run([str(path/'test')],check=True)
