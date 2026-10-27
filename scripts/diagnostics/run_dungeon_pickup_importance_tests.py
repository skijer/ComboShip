#!/usr/bin/env python3
"""Execute both production pickup queues against native/foreign dungeon items."""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
from run_mm_scene_randomization_tests import function

ROOT = Path(__file__).resolve().parents[2]
items = (ROOT / 'mm/2s2h/Rando/StaticData/Items.cpp').read_text()
types = (ROOT / 'mm/2s2h/Rando/Types.h').read_text()
mm_enum = re.search(r'typedef enum \{[^{}]*\} RandoItemId;', types, re.S)[0]
type_enum = re.search(r'typedef enum \{[^{}]*\} RandoItemType;', types, re.S)[0]
catalog = re.findall(r'RI\((RI_\w+),\s*"[^"]*",\s*"[^"]*",\s*(RITYPE_\w+)', items)
rg_ids = re.findall(r'RANDO_ENUM_ITEM\((RG_\w+)\)',
                   (ROOT / 'soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerGet.h').read_text())
hook = (ROOT / 'soh/soh/Enhancements/randomizer/hook_handlers.cpp').read_text()
queue = (ROOT / 'mm/2s2h/Rando/MiscBehavior/CheckQueue.cpp').read_text()
helper = '#include "combo/menu/ComboDungeonPickup.h"\n' if (ROOT / 'combo/menu/ComboDungeonPickup.h').exists() else ''
source = r'''
#include <cassert>
#include <iostream>
#include <map>
#include <queue>
#include <string>
#include "combo/rando/CrossForeign.h"
/* HELPER */
/* ENUMS */
using RandoCheckId=int;
using RandomizerCheck=int;
int skipLevel=0,ootSkip=1,drops=0;
int CVarGetInteger(const char* name,int fallback) {
 return std::string(name).find("SkipGetItemAnimation")!=std::string::npos ? ootSkip : skipLevel;
}
ComboRando::ForeignItem foreign{ComboRando::GAME_OOT,"Forest Temple Compass",""};
namespace Rando::StaticData {
struct Item { RandoItemType randoItemType; };
std::map<RandoItemId,Item> Items={ /* CATALOG */ };
/* NATIVE_POLICY */
}
namespace Rando::MiscBehavior {
const ComboRando::ForeignItem* MM_LookupForeign(int) { return &foreign; }
bool ShouldShowForeignCutscene(int);
}
/* FOREIGN_POLICY */
enum GetItemCategory { ITEM_CATEGORY_JUNK,ITEM_CATEGORY_LESSER,ITEM_CATEGORY_HEALTH,
 ITEM_CATEGORY_SKULLTULA_TOKEN,ITEM_CATEGORY_MAJOR };
enum {MOD_NONE,MOD_RANDOMIZER,SGIA_JUNK=1,SGIA_ALL=2};
using GetItemID=int;
struct GetItemEntry { int modIndex,getItemId,itemId; GetItemCategory getItemCategory; };
GetItemEntry entry{MOD_RANDOMIZER,RG_DEKU_TREE_MAP,0,ITEM_CATEGORY_LESSER};
GetItemCategory Randomizer_AdjustItemCategory(GetItemEntry e) {return e.getItemCategory;}
struct Location {
 int placed=RG_DEKU_TREE_MAP;
 int GetPlacedRandomizerGet() {return placed;}
 RandomizerGet GetVanillaItem() {return static_cast<RandomizerGet>(placed);}
 std::string GetName() {return "fixture check";}
 bool HasObtained() {return false;}
} location;
namespace Rando {
struct Context {
 static Context* GetInstance() {static Context instance;return &instance;}
 Location* GetItemLocation(int) {return &location;}
 GetItemEntry GetFinalGIEntry(int,bool,GetItemID) {return entry;}
};
namespace StaticData {
 Location* GetLocation(int) {return &location;}
 struct ItemId {int GetItemID(){return 0;}};
 ItemId RetrieveItem(int) {return {};}
}
}
struct {int fileNum=0;} gSaveContext;
struct Player {int stateFlags1=0;} fixturePlayer;
struct {} play;
auto* gPlayState=&play;
#define GET_PLAYER(p) (&fixturePlayer)
#define IS_RANDO true
#define CVAR_RANDOMIZER_ENHANCEMENT(x) "gRandoEnhancements." x
#define SPDLOG_INFO(...) ((void)0)
enum {PLAYER_STATE1_IN_ITEM_CS=1,PLAYER_STATE1_GETTING_ITEM=2,PLAYER_STATE1_CARRYING_ACTOR=4,
 RC_UNKNOWN_CHECK,RC_HF_OCARINA_OF_TIME_ITEM,RC_SPIRIT_TEMPLE_SILVER_GAUNTLETS_CHEST,
 RC_MARKET_BOMBCHU_BOWLING_FIRST_PRIZE,RC_MARKET_BOMBCHU_BOWLING_SECOND_PRIZE,
 ITEM00_SOH_GIVE_ITEM_ENTRY};
bool Player_InBlockingCsMode(decltype(gPlayState),Player*) {return false;}
namespace ComboCapeReceiptChoice {bool IsCape(const char*) {return false;}}
const ComboRando::ForeignItem* OOT_LookupForeign(int,const std::string&) {return &foreign;}
std::queue<int> randomizerQueuedChecks;
int randomizerQueuedCheck=RC_UNKNOWN_CHECK;
GetItemEntry randomizerQueuedItemEntry{};
float iceTrapScale=0;
struct {float x,y,z;} spawnPos{};
void Item_DropCollectible(decltype(gPlayState),decltype(&spawnPos),int16_t) {++drops;}
/* OOT_QUEUE */
int main() {
 const RandoItemId native[] = {RI_WOODFALL_MAP,RI_SNOWHEAD_MAP,RI_GREAT_BAY_MAP,RI_STONE_TOWER_MAP,
  RI_WOODFALL_COMPASS,RI_SNOWHEAD_COMPASS,RI_GREAT_BAY_COMPASS,RI_STONE_TOWER_COMPASS,
  RI_OOT_MAP_DEKU_TREE,RI_OOT_MAP_FOREST_TEMPLE,RI_OOT_COMPASS_DEKU_TREE,RI_OOT_COMPASS_FOREST_TEMPLE};
 for(auto id:native) for(int level=0;level<4;++level) {
  skipLevel=level;
  assert(Rando::StaticData::ShouldShowGetItemCutscene(id)==(level<2));
 }
 const char* names[]={"Great Deku Tree Map","Forest Temple Compass","Ice Cavern Compass",
  "Woodfall Map","Stone Tower Compass"};
 for(auto name:names) for(int level=0;level<4;++level) {
  foreign.itemName=name;foreign.advancement=false;foreign.category="";skipLevel=level;
  assert(Rando::MiscBehavior::ShouldShowForeignCutscene(1)==(level<2) &&
         "foreign map/compass hints disappeared under Skip Junk");
  foreign.category="lesser";
  assert(Rando::MiscBehavior::ShouldShowForeignCutscene(1)==(level<2));
 }
 foreign.itemName="Blue Rupee";foreign.category="junk";
 for(int level=0;level<4;++level) {
  skipLevel=level;
  assert(Rando::MiscBehavior::ShouldShowForeignCutscene(1)==(level<1));
  foreign.fakeItemName="Forest Temple Compass";foreign.trap=true;
  assert(Rando::MiscBehavior::ShouldShowForeignCutscene(1)==(level<3));
  foreign.fakeItemName="";foreign.trap=false;
 }
 const int dungeonIds[]={RG_DEKU_TREE_MAP,RG_ICE_CAVERN_MAP,RG_DEKU_TREE_COMPASS,RG_ICE_CAVERN_COMPASS};
 for(int id:dungeonIds) {
  location.placed=id;entry.getItemId=id;
  for(int mode:{SGIA_JUNK,SGIA_ALL}) {
   ootSkip=mode;randomizerQueuedCheck=RC_UNKNOWN_CHECK;randomizerQueuedChecks.push(1);drops=0;
   RandomizerOnPlayerUpdateForRCQueueHandler();
   assert(drops==(mode==SGIA_ALL) && "native SoH compass was dropped with its hint receipt skipped");
  }
 }
 location.placed=RG_COMBO_FOREIGN;entry.getItemId=RG_COMBO_FOREIGN;
 for(auto name:names) for(int mode:{SGIA_JUNK,SGIA_ALL}) {
  foreign.itemName=name;foreign.advancement=false;foreign.category="lesser";
  ootSkip=mode;randomizerQueuedCheck=RC_UNKNOWN_CHECK;randomizerQueuedChecks.push(1);drops=0;
  RandomizerOnPlayerUpdateForRCQueueHandler();
  assert(drops==(mode==SGIA_ALL) && "foreign map/compass skipped its SoH hint receipt");
 }
 foreign.itemName="Blue Rupee";foreign.category="junk";ootSkip=SGIA_JUNK;
 randomizerQueuedCheck=RC_UNKNOWN_CHECK;randomizerQueuedChecks.push(1);drops=0;
 RandomizerOnPlayerUpdateForRCQueueHandler();assert(drops==1);
 std::cout<<"PASS native and foreign dungeon map/compass receipts survive Skip Junk in MM and SoH; stronger skips, rupees and trap disguises preserved\n";
}
'''
source = source.replace('/* HELPER */', helper)
source = source.replace('/* ENUMS */', mm_enum + '\n' + type_enum + '\nenum RandomizerGet {' + ','.join(rg_ids) + '};')
source = source.replace('/* CATALOG */', ','.join('{'+item+',{'+kind+'}}' for item,kind in catalog))
source = source.replace('/* NATIVE_POLICY */', function(items,'ShouldShowGetItemCutscene'))
source = source.replace('/* FOREIGN_POLICY */', function(queue,'Rando::MiscBehavior::ShouldShowForeignCutscene'))
source = source.replace('/* OOT_QUEUE */', function(hook,'RandomizerOnPlayerUpdateForRCQueueHandler'))
with tempfile.TemporaryDirectory(prefix='dungeon-pickup-') as td:
    src = Path(td) / 'test.cpp'
    src.write_text(source)
    flags=['-std=c++20','-DCOMBO_BUILD','-Wall','-Wextra','-Wno-unused-parameter']
    if '--sanitize' in sys.argv:
        flags += ['-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie']
    exe=Path(td)/'test'
    subprocess.run([os.environ.get('CXX','c++'),*flags,'-I'+str(ROOT),'-I'+str(ROOT/'combo'),str(src),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
