#!/usr/bin/env python3
"""Compile both production host adapters with explicit engine-boundary fixtures."""
import pathlib,re,subprocess,tempfile
import os, sys
sanitize_flags = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-g'] if '--sanitize' in sys.argv else []
if sanitize_flags:
    os.environ['ASAN_OPTIONS'] = 'detect_leaks=0'
root=pathlib.Path(__file__).resolve().parents[2]
common=r'''
#define COMBO_BUILD 1
#include "combo/ComboGameMode.h"
extern "C" { int gComboGameMode = COMBO_GAME_BOTH; }
#define COMBO_EXPORT
#include "combo/rando/CrossDungeonState.h"
#include "combo/rando/CrossDungeonRuntime.h"
#include "combo/rando/CrossEntranceBoss.h"
#include <cassert>
#define SPDLOG_ERROR(...) ((void)0)
static bool active=false; static int saves=0; static void* gPlayState=nullptr;
static struct { int fileNum=0; } gSaveContext;
static bool nativeFlags[1000]{};
static void Flags_SetEventChkInf(int flag) { nativeFlags[flag]=true; }
#define SET_WEEKEVENTREG(flag) (nativeFlags[flag]=true)
using RandomizerRegion=int; using RandoRegionId=int; using ActorId=int; using LogicVal=int;
using namespace ComboRando::CrossDungeon;
'''
def native_function(text,name):
 match=re.search(r'^const std::vector<EntranceInfoPair>& '+name+r'\(\) \{',text,re.M)
 assert match
 opening=text.index('{',match.start());depth=1;end=opening+1
 while depth:
  depth+=(text[end]=='{')-(text[end]=='}');end+=1
 return text[match.start():end]
native_table=native_function((root/'soh/soh/Enhancements/randomizer/entrance.cpp').read_text(),'GetEntranceShuffleTable')
entrance_types=(root/'soh/soh/Enhancements/randomizer/entrance.h').read_text()
native_types=re.search(r'enum class EntranceType \{.*?\};',entrance_types,re.S).group()
native_link=re.search(r'typedef struct \{\n    EntranceType type;.*?\} EntranceLinkInfo;',entrance_types,re.S).group()
for game,host in [(0,'Oot'),(1,'Mm')]:
 header=root/f'combo/rando/CrossEntrance{host}Boss.inc.h'; text=header.read_text()
 constants=sorted(set(re.findall(r'\b(?:ENTR_|SCENE_|RR_|LOGIC_|ACTOR_|EVENTCHKINF_|WEEKEVENTREG_)[A-Z0-9_]+',text)))
 if game: constants+=['ODOLWAS_LAIR','GOHTS_LAIR','GYORGS_LAIR','TWINMOLDS_LAIR','WOODFALL_TEMPLE','SNOWHEAD_TEMPLE','GREAT_BAY_TEMPLE','STONE_TOWER_TEMPLE_INVERTED','WOODFALL','MOUNTAIN_VILLAGE_SPRING','ZORA_CAPE','IKANA_CANYON']
 if not game:
  constants=sorted(set(constants+['RR_NONE']+re.findall(r'\bRR_[A-Z0-9_]+',native_table)))
  constants=[name for name in constants if not name.startswith(('SCENE_','ENTR_'))]
  native_enums = '#include "soh/include/z64scene_enum.h"\n#define SCENE_UNUSED_6E SCENE_ID_MAX\nenum NativeEntrance {\n#define DEFINE_ENTRANCE(name, ...) name,\n#include "soh/include/tables/entrance_table.h"\n#undef DEFINE_ENTRANCE\n};\n'
  misc=(root/'soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerMiscEnums.h').read_text()
  offsets=misc.split('RANDO_ENUM_BEGIN(GrottoEntranceOffsets)',1)[1].split('RANDO_ENUM_END(GrottoEntranceOffsets)',1)[0]
  native_enums+='enum NativeGrotto {'+','.join(re.findall(r'RANDO_ENUM_ITEM\((\w+)\)',offsets))+'};\n'
  native_enums+='\n'.join(re.findall(r'^#define ENTRANCE_GROTTO.*$',(root/'soh/soh/Enhancements/randomizer/randomizerTypes.h').read_text(),re.M))+'\n'
 else:
  native_enums = ''
 prefix=common+native_enums+'enum {'+','.join(constants)+'};\n'
 if not game:
  prefix+='namespace Rando {\n'+native_types+'\n'+native_link+'\nusing EntranceInfoPair=std::pair<EntranceLinkInfo,EntranceLinkInfo>;\nEntranceLinkInfo NO_RETURN_ENTRANCE={EntranceType::None,RR_NONE,RR_NONE,-1};\n'+native_table+'\n}\n'
  prefix+=r'''
static bool physical[1000]{};
struct Region { bool access=false; bool Child()const{return access;} bool Adult()const{return false;} };
static Region rooms[1000]; static Region* RegionTable(int id) { return &rooms[id]; }
struct NativeLogic { bool Get(int id) {return physical[id];} };
struct Context { NativeLogic logic; NativeLogic* GetLogic(){return &logic;} };
struct OTRGlobals { Context* gRandoContext; static OTRGlobals* Instance; };
static Context ctx; static OTRGlobals globals{&ctx}; OTRGlobals* OTRGlobals::Instance=&globals;
struct SaveManager { static SaveManager* Instance; void SaveFile(int){++saves;} };
static SaveManager manager; SaveManager* SaveManager::Instance=&manager;
extern "C" int Combo_SOH_CrossBossEnabled(){return active;}
namespace ComboCrossOot { using namespace ComboRando::CrossDungeon; static HostState state(0); static bool searching=false, evaluatingExits=false; }
'''
  main=r'''
int main() {
 using namespace ComboCrossOot;
 std::vector<Dungeon> records;
 AppendBossCatalogue(records,{{ENTR_DEKU_TREE_BOSS_DOOR,ENTR_FIRE_TEMPLE_BOSS_DOOR},{ENTR_KOKIRI_FOREST_DEKU_TREE_BLUE_WARP,ENTR_LAKE_HYLIA_WATER_TEMPLE_BLUE_WARP}});
 assert(records.size()==8 && records[0].pool==Pool::Boss && records[0].boss==0 && records[7].boss==7);
 assert(records[0].returnTo.index==ENTR_FIRE_TEMPLE_BOSS_DOOR && records[0].warpReturn->index==ENTR_LAKE_HYLIA_WATER_TEMPLE_BLUE_WARP);
 assert(records[0].owner=="oot:"+std::to_string(ENTR_DEKU_TREE_ENTRANCE));
 assert(records[0].reverseCheckpoint==std::optional<Endpoint>({0,ENTR_FIRE_TEMPLE_ENTRANCE}) && "native reverse shuffle must recover in its actual Fire Temple owner");
 assert(!records[0].warpCheckpoint && "an exterior bluewarp must not retain a physical Deku checkpoint");
 std::vector<Dungeon> coupled;
 AppendBossCatalogue(coupled,{{ENTR_DEKU_TREE_BOSS_DOOR,ENTR_FIRE_TEMPLE_BOSS_DOOR},{ENTR_FIRE_TEMPLE_BOSS_DOOR,ENTR_DEKU_TREE_BOSS_DOOR},
     {ENTR_KOKIRI_FOREST_DEKU_TREE_BLUE_WARP,ENTR_FIRE_TEMPLE_BOSS_DOOR}});
 assert(coupled[0].warpCheckpoint==std::optional<Endpoint>({0,ENTR_FIRE_TEMPLE_ENTRANCE}));
 assert(coupled[4].reverseCheckpoint==std::optional<Endpoint>({0,ENTR_DEKU_TREE_ENTRANCE}));

 std::vector<Dungeon> nativeEndpoints;
 AppendBossCatalogue(nativeEndpoints,{{ENTR_DEKU_TREE_BOSS_DOOR,ENTR_FIRE_TEMPLE_ENTRANCE},
     {ENTR_KOKIRI_FOREST_DEKU_TREE_BLUE_WARP,ENTR_FIRE_TEMPLE_0_1}});
 assert(nativeEndpoints[0].reverseCheckpoint==std::optional<Endpoint>({0,ENTR_FIRE_TEMPLE_ENTRANCE}));
 assert(nativeEndpoints[0].warpCheckpoint==std::optional<Endpoint>({0,ENTR_FIRE_TEMPLE_0_1}));
 std::vector<Dungeon> mixedDungeonEndpoint;
 AppendBossCatalogue(mixedDungeonEndpoint,{{ENTR_KOKIRI_FOREST_DEKU_TREE_BLUE_WARP,ENTR_INSIDE_GANONS_CASTLE_ENTRANCE}});
 assert(mixedDungeonEndpoint[0].warpCheckpoint==std::optional<Endpoint>({0,ENTR_INSIDE_GANONS_CASTLE_ENTRANCE}) && "native mixed returns inside an unpooled dungeon still own a physical checkpoint");

 Dungeon interior{"interior",{0,ENTR_KAKARIKO_CENTER_GUEST_HOUSE_0},{0,ENTR_KAKARIKO_VILLAGE_OUTSIDE_CENTER_GUEST_HOUSE},{0,ENTR_KAKARIKO_VILLAGE_OUTSIDE_CENTER_GUEST_HOUSE}};
 interior.pool=Pool::Interior;
 std::vector<Dungeon> interiorEndpoints{interior};
 AppendBossCatalogue(interiorEndpoints,{{ENTR_DEKU_TREE_BOSS_DOOR,ENTR_KAKARIKO_CENTER_GUEST_HOUSE_0},
     {ENTR_KOKIRI_FOREST_DEKU_TREE_BLUE_WARP,ENTR_KAKARIKO_CENTER_GUEST_HOUSE_0_1}});
 assert(interiorEndpoints[1].reverseCheckpoint==std::optional<Endpoint>(interior.entrance));
 assert(!interiorEndpoints[1].warpCheckpoint && "an alternate interior spawn lacks exact durable catalogue ownership");
 RuntimeState nativeRuntime(0);nativeRuntime.Install(Encode({coupled[0],coupled[4]},{0,1}));
 auto reverseRoute=nativeRuntime.Lookup(Phase::BeforeNative,ENTR_DEKU_TREE_BOSS_DOOR);
 assert(reverseRoute && reverseRoute->to==Endpoint({0,ENTR_FIRE_TEMPLE_BOSS_DOOR}));
 assert(reverseRoute->arrivalCheckpoint==std::optional<Endpoint>({0,ENTR_FIRE_TEMPLE_ENTRANCE}));
 CheckpointState recovery;
 const int nativeArrival=nativeRuntime.RecoveryForArrival(ENTR_FIRE_TEMPLE_BOSS_DOOR);
 recovery.Arrive(nativeArrival,nativeRuntime.IsRecoveryEntrance(nativeArrival));
 const int savedCheckpoint=recovery.Saved();recovery.Load(savedCheckpoint);
 assert(recovery.ConsumeRecovery([&](int index){return nativeRuntime.IsRecoveryEntrance(index);})==ENTR_FIRE_TEMPLE_ENTRANCE);

 assert(records[0].escapeExits.size()==1 && records[0].hasReverse);
 state.dungeons=records; state.logicalClears=1u<<5; searching=true;
 assert(Combo_SOH_CrossBossLogicalClear(5,0) && !Combo_SOH_CrossBossLogicalClear(0,1));
 physical[LOGIC_DEKU_TREE_CLEAR]=true; assert(SOH_GetCrossBossDefeated()==0);
 rooms[RR_DEKU_TREE_BOSS_ROOM].access=true; assert(SOH_GetCrossBossDefeated()==1);
 assert(!Combo_SOH_CrossBossLogicalClear(0,1)); // Physical event does not unlock source world.
 searching=false;evaluatingExits=true;
 assert(Combo_SOH_CrossBossLogicalClear(5,0) && !Combo_SOH_CrossBossLogicalClear(0,1) && "exit evaluation must retain oracle source-clear ownership");
 evaluatingExits=false;
 searching=false; active=true; SOH_SetCrossBossCompletedMask(1u<<5);
 assert(Combo_SOH_CrossBossLogicalClear(5,0) && !Combo_SOH_CrossBossLogicalClear(0,1));
 gPlayState=&active; // A parked host retains its last PlayState.
 SOH_ApplyCrossBossClear(COMBO_BOSS_GOHMA, 1);
 assert(nativeFlags[EVENTCHKINF_OBTAINED_KOKIRI_EMERALD_DEKU_TREE_DEAD] && saves==1);
 SOH_ApplyCrossBossClear(COMBO_BOSS_MORPHA, 1);assert(nativeFlags[EVENTCHKINF_RAISED_LAKE_HYLIA_WATER] && saves==2);
 SOH_ApplyCrossBossClear(COMBO_BOSS_VOLVAGIA, 0);assert(saves==2); // Active host defers ordinary saving.
 gPlayState=nullptr;SOH_ApplyCrossBossClear(COMBO_BOSS_VOLVAGIA, 0);assert(saves==3); // Native null fallback remains.
 active=false;assert(Combo_SOH_CrossBossLogicalClear(0,1) && !Combo_SOH_CrossBossLogicalClear(5,0));
}
'''
 else:
  prefix+=r'''
#define ENTRANCE(scene,spawn) (((scene)<<9)|((spawn)<<4))
static bool kill[1000]{};
namespace Rando::Logic { using RegionTimeState=int; static void SetCurrentRegionTime(const std::unordered_map<int,int>&,int){} static bool CanKillEnemy(int id){return kill[id];} }
static void SaveManager_SaveCurrentForCombo(){++saves;}
extern "C" int Combo_MM_CrossBossEnabled(){return active;}
namespace ComboCrossMm { using namespace ComboRando::CrossDungeon; static HostState state(1); static bool searching=false; static std::set<int> reachable; static std::unordered_map<int,int> timeStates; }
'''
  # unordered_map is an engine include in the real host.
  prefix='#include <unordered_map>\n'+prefix
  main=r'''
int main() {
 using namespace ComboCrossMm;
 std::vector<Dungeon> records;AppendBossCatalogue(records);
 assert(records.size()==4 && records[0].boss==8 && records[3].boss==11);
 assert(!records[0].hasReverse && records[0].escapeExits[0].index==ENTRANCE(WOODFALL_TEMPLE,1));
 assert(records[0].warpReturn->index==ENTRANCE(WOODFALL_TEMPLE,1) && records[3].owner=="mm:inverted");

 // Exercise production composition: a source Odolwa clear returns to its prison,
 // then the prison's outer door belongs to the dungeon, not to physical Odolwa.
 Dungeon gohma{"oot:boss:0",{0,30000},{0,30001},{0,30001},{{0,30002}}};
 gohma.pool=Pool::Boss;gohma.boss=0;gohma.owner="oot:source";gohma.warpReturn=Endpoint{0,30003};
 const Endpoint prison{1,ENTRANCE(WOODFALL_TEMPLE,1)}, prisonExit{1,ENTRANCE(WOODFALL,3)};
 auto bossOnly=FromPermutation({gohma,records[0]},{1,0});
 assert(Resolve(bossOnly,Phase::BeforeNative,{0,30002})==prison);
 assert(Resolve(bossOnly,Phase::BeforeNative,prisonExit)==prisonExit && "prison outer exit must remain native in boss-only mode");
 const Endpoint ordinaryWoodfall{1,ENTRANCE(WOODFALL,0)};
 assert(Resolve(bossOnly,Phase::BeforeNative,ordinaryWoodfall)==ordinaryWoodfall && "ordinary swamp-to-Woodfall travel is not a boss escape");
 Dungeon otherDungeon{"oot:source",{0,20000},{0,20001},{0,20001}};
 Dungeon woodfall{"mm:woodfall",{1,ENTRANCE(WOODFALL_TEMPLE,0)},{1,ENTRANCE(WOODFALL,1)},{1,ENTRANCE(WOODFALL,1)},{{1,ENTRANCE(WOODFALL,3)}}};
 auto dungeonAndBoss=FromPermutation({otherDungeon,woodfall,gohma,records[0]},{1,0,3,2});
 assert(Resolve(dungeonAndBoss,Phase::BeforeNative,{0,30002})==prison);
 assert(Resolve(dungeonAndBoss,Phase::BeforeNative,prisonExit)==otherDungeon.returnTo && "prison outer exit must follow its parent dungeon permutation");
 assert(Resolve(dungeonAndBoss,Phase::BeforeNative,ordinaryWoodfall)==ordinaryWoodfall);
 state.dungeons=records;state.logicalClears=1u<<9;searching=true;
 assert(Combo_MM_CrossBossLogicalClear(9,0) && !Combo_MM_CrossBossLogicalClear(8,1));
 kill[ACTOR_BOSS_02]=true;assert(MM_GetCrossBossDefeated()==0);
 reachable.insert(RR_STONE_TOWER_TEMPLE_BOSS_ROOM);assert(MM_GetCrossBossDefeated()==(1u<<11));
 searching=false;active=true;MM_SetCrossBossCompletedMask(1u<<9);
 assert(!Combo_MM_CrossBossLogicalClear(8,1) && !Combo_MM_CrossBossLogicalClear(9,0));
 assert(Combo_MM_CrossBossLogicalClear(9,1)); // Runtime requires this cycle's effect.
 gPlayState=&active; // A parked host retains its last PlayState.
 MM_ApplyCrossBossClear(8, 1);assert(nativeFlags[WEEKEVENTREG_CLEARED_WOODFALL_TEMPLE] && saves==1);
 MM_ApplyCrossBossClear(10, 1);assert(nativeFlags[WEEKEVENTREG_CLEARED_GREAT_BAY_TEMPLE] && saves==2);
 MM_ApplyCrossBossClear(9, 0);assert(saves==2); // Active host defers ordinary saving.
 gPlayState=nullptr;MM_ApplyCrossBossClear(9, 0);assert(saves==3); // Native null fallback remains.
 active=false;assert(Combo_MM_CrossBossLogicalClear(8,1));
}
'''
 with tempfile.TemporaryDirectory(prefix='boss-host-') as tmp:
  source=pathlib.Path(tmp)/'test.cpp';binary=pathlib.Path(tmp)/'test'
  source.write_text(prefix+'\n#include "'+str(header)+'"\n'+main)
  subprocess.run(['g++',*sanitize_flags,'-std=c++20','-Wall','-Wextra','-Wno-unused-function','-I',str(root),'-I',str(root/'combo'),'-I',str(root/'soh/include'),'-I',str(root.parent/'build-deps/usr/include'),str(source),'-o',str(binary)],check=True)
  subprocess.run([str(binary)],check=True)
