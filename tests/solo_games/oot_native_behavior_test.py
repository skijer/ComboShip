#!/usr/bin/env python3
"""Compile actual OoT function bodies to exercise native/combined session boundaries.

Fixtures replace only unrelated engine/window services. Save paths use a real
filesystem; spoiler routing parses real JSON; creation and win checks execute the
production bodies. --source-rev gives a repeatable baseline red control.
"""
from pathlib import Path
import argparse
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source-rev')
args = parser.parse_args()
def read(path):
    if args.source_rev:
        return subprocess.check_output(['git','show',args.source_rev+':'+path],cwd=ROOT,text=True)
    return (ROOT/path).read_text(encoding='utf-8-sig')
def function(path, marker):
    source=read(path); start=source.index(marker); brace=source.index('{',start); depth=1; i=brace+1
    # Existing native functions contain balanced braces in their comments/strings.
    while depth:
        depth += (source[i]=='{')-(source[i]=='}'); i+=1
    return source[start:i]+'\n'

source=r'''
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <map>
#include <queue>
#include <nlohmann/json.hpp>
#include "ComboGameMode.h"
extern "C" { int gComboGameMode=COMBO_GAME_BOTH; }
using u8=uint8_t; using u16=uint16_t; using s16=int16_t;
#define CVAR_ENHANCEMENT(x) "enh." x
#define CVAR_GENERAL(x) "gen." x
#define SPDLOG_INFO(...) ((void)0)
std::map<std::string,int> integers;
std::map<std::string,std::string> strings;
int CVarGetInteger(const char* s,int fallback) { auto i=integers.find(s); return i==integers.end()?fallback:i->second; }
void CVarSetInteger(const char* s,int v) { integers[s]=v; }
void CVarSetString(const char* s,const char* v) { strings[s]=v; }
namespace SohUtils { bool IsStringEmpty(const char* s) { return !s || !*s; } }
bool original=true,mq=true;
bool ResourceMgr_GameHasOriginal(){ return original; }
bool ResourceMgr_GameHasMasterQuest(){ return mq; }
/* native enum inserted below */
struct { int entranceIndex,linkAge,dayTime,cutsceneIndex,language,n64ddFlag,fileNum;
  u8 playerName[8]; struct { int filenameLanguage; struct { int id; } quest; } ship;
  struct { int dungeonItems[32]; } inventory; } gSaveContext{};
struct FileChooseContext { int buttonIndex=0,n64ddFlag=0,speedrunIndex=0; u8 questType[3]{}; };
struct { u8 playerName[8]; } metadata{};
auto Save_GetSaveMetaInfo(int) { return &metadata; }
int randos=0,peers=0,saves=0,created=0;
bool seed=true,spoiler=false;
void Sram_InitNewSave(){} void Sram_InitDebugSave(){}
bool Ship_QuestDebugEnabled(int){ return false; }
int ResourceMgr_GetGameRegion(int){ return 0; }
constexpr int GAME_REGION_PAL=1,LANGUAGE_JPN=1,NAME_LANGUAGE_PAL=1,NAME_LANGUAGE_NTSC_JPN=2,NAME_LANGUAGE_NTSC_ENG=3;
constexpr int ENTR_LINKS_HOUSE_CHILD_SPAWN=100;
bool Randomizer_IsSeedGenerated(){ return seed; } bool Randomizer_IsSpoilerLoaded(){ return spoiler; }
void Randomizer_InitSaveFile(){ ++randos; }
void FleetComboFS_OnCreateSave(int){ ++peers; }
void Speedrun_InitSaveFile(int){}
#define IS_SPEEDRUN (false)
void Save_SaveFile(){ ++saves; }
void SaveManager_ThreadPoolWait(){}
void CountCreate(int){ ++created; }
void (*gComboSaveInitCallback)(int)=CountCreate;
void (*gComboGenerateCallback)(int)=nullptr;
namespace Ship { struct Context { static std::string GetPathRelativeToAppDirectory(const std::string& p) { return p; } }; }
struct SaveManager { std::filesystem::path GetFileName(int); };
struct PlayState { int sceneNum; }; PlayState* gPlayState=nullptr;
constexpr int SCENE_CASTLE_COURTYARD_GUARDS_DAY=1,SCENE_CASTLE_COURTYARD_GUARDS_NIGHT=2,ENTR_CASTLE_GROUNDS_RAINBOW_BRIDGE_EXIT=30;
int crossCalls=0;
void Entrance_SetEntranceDiscovered(s16,bool){} void EntranceTracker_SetLastEntranceOverride(s16){}
s16 Entrance_GetOverride(s16 p){ return p+1; }
s16 Grotto_OverrideSpecialEntrance(s16 p){ return p; }
int32_t Combo_SOH_CrossDungeonRoute(int32_t,bool){ ++crossCalls; return 999; }
namespace GameInteractor { bool IsSaveLoaded(){ return true; } }
constexpr int SCENE_GANONS_TOWER=0,RAND_INF_GANON_SOUL=1,RC_GANONS_BOSS_KEY=2,RC_GANON_SOUL=3,RC_WINCON=4;
std::queue<int> randomizerQueuedChecks;
bool MeetsGBKRequirements(){ return false; } bool MeetsGanonsSoulRequirements(){ return false; }
bool MeetsWinconRequirements(){ return true; } bool Flags_GetRandomizerInf(int){ return true; }
'''
quest_source=read('soh/include/z64save.h')
quest_start=quest_source.rindex('typedef enum',0,quest_source.index('QUEST_NORMAL'))
quest_end=quest_source.index(';',quest_start)+1
source=source.replace('/* native enum inserted below */',quest_source[quest_start:quest_end])
# An extracted static helper is included when production shares the native save directory.
save=read('soh/soh/SaveManager.cpp')
if 'static std::filesystem::path GetNativeSaveDirectory(' in save:
    source+=function('soh/soh/SaveManager.cpp','static std::filesystem::path GetNativeSaveDirectory(')
source+=function('soh/soh/SaveManager.cpp','std::filesystem::path SaveManager::GetFileName(')
source+=function('soh/soh/Enhancements/FileSelectEnhancements.cpp','bool SohFileSelect_IsQuestHidden(')
source+=function('soh/soh/Enhancements/FileSelectEnhancements.cpp','u8 SohFileSelect_CountVisibleQuests(')
source+=function('soh/src/code/z_sram.c','void Sram_InitSave(')
source+=function('soh/soh/Enhancements/randomizer/randomizer.cpp','bool Rando_HandleSpoilerDrop(')
source+=function('soh/soh/Enhancements/randomizer/randomizer_entrance.c','s16 Entrance_OverrideNextIndex(')
source+=function('soh/soh/Enhancements/randomizer/hook_handlers.cpp','void CheckTriggers(')
source+=function('soh/soh/FleetShipCombo/FleetShipCombo.cpp','int FleetCombo_BeatBothBosses(')
source+=r'''
int fails=0,checks=0;
void check(bool v,const char* label){ ++checks; if(!v){ ++fails;std::cerr<<"FAIL "<<label<<"\n"; } }
int main(){
  SaveManager manager;
  gComboGameMode=COMBO_GAME_OOT;
  check(manager.GetFileName(0)==std::filesystem::path("saves/oot/file1.sav"),"solo slot path is saves/oot");
  check(!SohFileSelect_IsQuestHidden(QUEST_NORMAL),"solo original visible");
  check(!SohFileSelect_IsQuestHidden(QUEST_MASTER),"solo MQ visible");
  check(SohFileSelect_IsQuestHidden(QUEST_OOTXMM),"solo hides the legacy paired quest inside native enum range");
  original=false; check(SohFileSelect_IsQuestHidden(QUEST_NORMAL),"missing original stays hidden"); original=true;
  mq=false;check(SohFileSelect_IsQuestHidden(QUEST_MASTER),"missing MQ stays hidden");mq=true;
  integers["enh.FileSelect.HideMasterQuest"]=1;
  check(SohFileSelect_IsQuestHidden(QUEST_MASTER),"solo native hide preference"); integers.clear();
  for (int quest : {QUEST_NORMAL,QUEST_MASTER,QUEST_RANDOMIZER}) {
    FileChooseContext fc; fc.questType[0]=quest; randos=peers=saves=created=0;
    Sram_InitSave(&fc);
    check(gSaveContext.ship.quest.id==quest,"solo creates chosen vanilla/MQ/rando quest");
    check(randos==(quest==QUEST_RANDOMIZER),"solo vanilla avoids randomizer initialization");
    check(saves==1 && peers==0 && created==0,"solo save never dispatches peer creation callback");
  }
  { FileChooseContext stale;stale.questType[0]=QUEST_OOTXMM;randos=peers=saves=created=0;Sram_InitSave(&stale);
    check(randos==0 && peers==0 && saves==0 && created==0,"solo rejects direct stale paired quest creation"); }
  std::ofstream("native.json")<<R"({"version":"x","finalSeed":123})";
  std::ofstream("combo.json")<<R"({"fileType":"ComboShipRandomizer","version":"x","finalSeed":123})";
  char native[]="native.json",combo[]="combo.json";
  check(Rando_HandleSpoilerDrop(native),"solo native spoiler accepted");
  check(integers["gen.RandomizerNewFileDropped"]==1,"solo native drop routed to native loader");
  check(!Rando_HandleSpoilerDrop(combo),"solo rejects combined spoiler");
  crossCalls=0;check(Entrance_OverrideNextIndex(10)==11 && crossCalls==0,"solo entrance ignores foreign adapter");
  CheckTriggers();check(!randomizerQueuedChecks.empty() && randomizerQueuedChecks.front()==RC_WINCON,"solo native win condition queues win");
  while(!randomizerQueuedChecks.empty())randomizerQueuedChecks.pop();
  integers["isFleetShipCombo.Enabled"]=1;integers["gFleetCombo.GoalMode"]=0;
  check(!FleetCombo_BeatBothBosses(),"stale combined goal cannot hijack solo Ganon");
  gComboGameMode=COMBO_GAME_BOTH;
  check(manager.GetFileName(0)==std::filesystem::path("Save/file1.sav"),"combined native fallback unchanged");
  check(SohFileSelect_IsQuestHidden(QUEST_NORMAL) && !SohFileSelect_IsQuestHidden(QUEST_RANDOMIZER),"combined remains randomizer only");
  randos=created=0;FileChooseContext fc;fc.questType[0]=QUEST_NORMAL;Sram_InitSave(&fc);
  check(gSaveContext.ship.quest.id==QUEST_RANDOMIZER && randos==1 && created==1,"combined creation callback unchanged");
  check(!Rando_HandleSpoilerDrop(native) && Rando_HandleSpoilerDrop(combo),"combined accepts only combined spoilers");
  crossCalls=0;check(Entrance_OverrideNextIndex(10)==999 && crossCalls==1,"combined entrance adapter retained");
  CheckTriggers();check(randomizerQueuedChecks.empty(),"combined suppresses native win trigger");
  check(FleetCombo_BeatBothBosses(),"combined legacy goal retained");
  std::cout<<checks<<" checks, "<<fails<<" failures\n";return fails?1:0;
}
'''
with tempfile.TemporaryDirectory(prefix='oot-solo-behavior-') as tmp:
    path=Path(tmp); (path/'case.cpp').write_text(source)
    deps=Path(os.environ.get('COMBO_TEST_DEPS','/workspace/scratch/4288e889429f/build-deps'))
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-DCOMBO_BUILD','-I'+str(ROOT/'combo'),'-I'+str(deps/'include'),str(path/'case.cpp'),'-o',str(path/'case')],check=True)
    subprocess.run([str(path/'case')],cwd=path,check=True)
