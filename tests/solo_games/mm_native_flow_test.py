#!/usr/bin/env python3
"""Execute MM native save creation/generation/spoiler, reset and owl-exit production bodies."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
DEPS = Path(os.environ.get('COMBO_TEST_DEPS', '/workspace/scratch/4288e889429f/build-deps'))

def block(source, marker):
    start=source.index(marker); opening=source.index('{',start); end=opening+1; depth=1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[start:end]

COMMON=r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>
#define COMBO_BUILD
#define COMBO_EXPORT
#include "combo/ComboGameMode.h"
extern "C" {int gComboGameMode=COMBO_GAME_MM;}
#define SPDLOG_INFO(...) ((void)0)
#define SPDLOG_WARN(...) ((void)0)
#define SPDLOG_ERROR(...) ((void)0)
'''
source=(ROOT/'mm/2s2h/Rando/MiscBehavior/OnFileCreate.cpp').read_text()
spoiler=(ROOT/'mm/2s2h/Rando/Spoiler/File.cpp').read_text()
apply=(ROOT/'mm/2s2h/Rando/Spoiler/Apply.cpp').read_text()
creation=COMMON+r'''
#include <nlohmann/json.hpp>
using s16=int16_t; using RandoItemId=int;using RandoCheckId=int;
enum {RO_SHUFFLE_GOLD_SKULLTULAS, RO_SKULLTULA_TOKENS_REQUIRED, RO_PLACEMENT_STRAY_FAIRIES,
RO_STRAY_FAIRIES_REQUIRED,RO_STRAY_FAIRIES_MAX,RO_CLOCK_SHUFFLE,RO_LOGIC};
enum {RO_DUNGEON_ITEM_START_WITH=4,RO_DUNGEON_ITEM_VANILLA=5};
enum {RO_LOGIC_VANILLA=0,RO_LOGIC_NO_LOGIC=1,RO_LOGIC_NEARLY_NO_LOGIC=2,RO_LOGIC_GLITCHLESS=3};
enum {SAVETYPE_VANILLA,SAVETYPE_RANDO};
enum {RC_UNKNOWN,RC_STARTING_ITEM_DEKU_MASK,RC_STARTING_ITEM_SONG_OF_HEALING};
enum {RI_UNKNOWN=-1,RI_JUNK,RI_HEART_PIECE,RI_HEART_CONTAINER,RI_SWORD_KOKIRI,RI_SHIELD_HERO,RI_KEY_SMALL};
constexpr int RITYPE_JUNK=0,RITYPE_SMALL_KEY=1,SPIDER_HOUSE_TOKENS_REQUIRED=30,STRAY_FAIRY_SCATTERED_TOTAL=15;
constexpr int PLAYER_FORM_HUMAN=4,SCENE_INSIDETOWER=0;
constexpr int EQUIP_TYPE_SWORD=0,EQUIP_VALUE_SWORD_NONE=0,EQUIP_TYPE_SHIELD=1,EQUIP_VALUE_SHIELD_NONE=0;
constexpr int ITEM_NONE=255,EQUIP_SLOT_B=0;
constexpr int NA_SE_SY_ATTENTION_SOUND=1,NA_SE_SY_QUIZ_INCORRECT=2;
#define ENTRANCE(scene,spawn) (0x1234+(spawn))
#define SET_WEEKEVENTREG(event) ((void)0)
#define SET_EQUIP_VALUE(type,value) ((void)0)
int button=1;
#define BUTTON_ITEM_EQUIP(form,slot) button
struct Check {int randoItemId=0;bool shuffled=false,eligible=false,skipped=false;uint16_t price=0;};
struct RandoSaveInfo {uint32_t finalSeed=0;int randoSaveOptions[8]={};int foundDungeonKeys[4]={};Check randoSaveChecks[4];};
struct Context {struct Save {int entrance=5,cutsceneIndex=17;bool hasTatl=false,isFirstCycle=false;int playerForm=3;
struct Info {struct Player {int threeDayResetCount=0;char playerName[8]={};char newf[6]={'Z','E','L','D','A','3'};} playerData;
struct Inventory {int dungeonKeys[4]={-1,-1,-1,-1};} inventory;
struct Flags {int switch0=0;} permanentSceneFlags[1];} saveInfo;
struct Ship {int saveType=SAVETYPE_VANILLA;RandoSaveInfo rando;} shipSaveInfo;} save;} gSaveContext;
#define RANDO_SAVE_OPTIONS gSaveContext.save.shipSaveInfo.rando.randoSaveOptions
#define RANDO_SAVE_CHECKS gSaveContext.save.shipSaveInfo.rando.randoSaveChecks
std::map<std::string,int> cvars;
int CVarGetInteger(const char* key,int def) {auto i=cvars.find(key);return i==cvars.end()?def:i->second;}
const char* CVarGetString(const char* key,const char*) {return std::string(key)=="gRando.SpoilerFile"?"chosen.json":"native-seed";}
std::string Ship_RemoveSpecialCharacters(const char* s) {return s;}
uint32_t Ship_Random(int,int) {return 42;}
uint32_t Ship_Hash(const std::string&) {return 123;}
void Ship_Random_Seed(uint32_t) {}
int sound=0;void Audio_PlaySfx(int s) {sound=s;}
struct ImVec4 {ImVec4(float,float,float,float){}};
namespace Notification {struct Notice {std::string prefix;ImVec4 prefixColor;std::string message;};void Emit(Notice){}}
struct OnRandoSeedGeneration {};
struct GameInteractor {using OnRandoSeedGeneration=::OnRandoSeedGeneration;static GameInteractor* Instance;
int generated=0;template<class T> void ExecuteHooks(){++generated;}} gi;
GameInteractor* GameInteractor::Instance=&gi;
const std::string appShortName="2ship";
namespace Ship {struct Context {static std::string GetPathRelativeToAppDirectory(const std::string& p,const std::string&){return p;}};}
int pools=0,fills=0,grants=0,applies=0,spoilers=0;
namespace Rando {
namespace StaticData {
struct Option {const char* cvar;int defaultValue;const char* name;};std::map<int,Option> Options={{RO_LOGIC,{"logic",RO_LOGIC_NO_LOGIC,"Logic"}}};
struct Item {int randoItemId=0,randoItemType=0;};std::map<int,Item> Items={{RI_KEY_SMALL,{RI_KEY_SMALL,RITYPE_SMALL_KEY}}};
struct StaticCheck {int randoCheckId=0,randoItemId=0;const char* name;};
std::map<int,StaticCheck> Checks={{1,{1,RI_SWORD_KOKIRI,"Native Check"}},{2,{2,RI_KEY_SMALL,"Small Key"}}};
int GetItemIdFromName(const char* name){return std::string(name)=="RI_SWORD_KOKIRI"?RI_SWORD_KOKIRI:RI_UNKNOWN;}
}
std::vector<int> GetStartingItemsFromConfig(){return {4};}
std::vector<int> GetStartingItemsFromSpoiler(nlohmann::json){++applies;return {4};}
std::vector<int> GetSariaPriorityItemsFromSpoiler(nlohmann::json){return {};}
void SetStartingItemsInSave(RandoSaveInfo&,const std::vector<int>&){}
std::vector<int> GetSariaPriorityItemsFromConfig(){return {};}
void SetSariaPriorityItemsInSave(RandoSaveInfo&,const std::vector<int>&){}
void GrantStartingItems(){++grants;}
void GiveItem(int){}
namespace Logic {
void GeneratePools(RandoSaveInfo&,std::vector<int>& checks,std::vector<int>& items){++pools;checks={1};items={1};}
void ApplyNoLogicToSaveContext(std::vector<int>&,std::vector<int>&){++fills;}
void ApplyNearlyNoLogicToSaveContext(std::vector<int>&,std::vector<int>&){++fills;}
void ApplyGlitchlessLogicToSaveContext(std::vector<int>&,std::vector<int>&){++fills;}
int ClockCount(){return 1;}
}
namespace Spoiler {
nlohmann::json GenerateFromSaveContext(){return {{"type","2S2H_RANDO_SPOILER"}};}
void SaveToFile(const std::string&,nlohmann::json){++spoilers;}
void RefreshOptions(){}
void SaveOotItemAreas(uint32_t,nlohmann::json){assert(Combo_IsCombinedGame());}
''' + block(apply,'void ApplyToSaveContext(')+r'''
''' + block(spoiler,'nlohmann::json LoadFromFile(')+r'''
}
namespace MiscBehavior {void OnFileCreate(s16);}
}
'''+block(source,'void Rando::MiscBehavior::OnFileCreate(')+r'''
int main(int argc,char** argv) {
    std::filesystem::current_path(argv[1]);
    Rando::MiscBehavior::OnFileCreate(0); // native vanilla
    assert(gSaveContext.save.shipSaveInfo.saveType==SAVETYPE_VANILLA);
    assert(gSaveContext.save.playerForm==3 && gSaveContext.save.cutsceneIndex==17);
    assert(pools==0 && fills==0 && grants==0 && gi.generated==0);
    cvars["gRando.Enabled"]=1;
    Rando::MiscBehavior::OnFileCreate(1); // native generation
    assert(gSaveContext.save.shipSaveInfo.saveType==SAVETYPE_RANDO);
    assert(gSaveContext.save.shipSaveInfo.rando.finalSeed==123);
    assert(pools==1 && fills==1 && grants==1 && spoilers==1 && gi.generated==1);
    assert(gSaveContext.save.shipSaveInfo.rando.foundDungeonKeys[0]==-1);
    assert(RANDO_SAVE_CHECKS[RC_STARTING_ITEM_DEKU_MASK].eligible);
    assert(RANDO_SAVE_CHECKS[RC_STARTING_ITEM_SONG_OF_HEALING].eligible);
    cvars["gRando.SpoilerFileIndex"]=1;
    std::filesystem::create_directory("randomizer");
    {std::ofstream f("randomizer/chosen.json");f<<R"({"type":"2S2H_RANDO_SPOILER","finalSeed":456,"options":{},"checks":{"Native Check":{"randoItemId":"RI_SWORD_KOKIRI"}}})";}
    Rando::MiscBehavior::OnFileCreate(2); // native spoiler loading
    assert(applies==1 && grants==2 && gi.generated==2 && pools==1);
    assert(gSaveContext.save.shipSaveInfo.rando.finalSeed==456);
    assert(RANDO_SAVE_CHECKS[1].randoItemId==RI_SWORD_KOKIRI && RANDO_SAVE_CHECKS[1].shuffled);
    assert(RANDO_SAVE_CHECKS[2].randoItemId==RI_KEY_SMALL && !RANDO_SAVE_CHECKS[2].shuffled && !RANDO_SAVE_CHECKS[2].skipped);
    {std::ofstream f("randomizer/chosen.json");f<<"{\"type\":\"COMBO_RANDO_SPOILER\"}";}
    Rando::MiscBehavior::OnFileCreate(2); // stale Combo seed cannot silently create a solo file
    assert(applies==1 && gi.generated==2 && sound==NA_SE_SY_QUIZ_INCORRECT);
    assert(gSaveContext.save.shipSaveInfo.saveType==SAVETYPE_VANILLA);
    assert(gSaveContext.save.saveInfo.playerData.newf[0]=='\0');
    std::cout<<"MM native vanilla/generation/spoiler/foreign-seed boundaries passed\n";
}
'''
reset_source=(ROOT/'mm/2s2h/DeveloperTools/DebugConsole.cpp').read_text()
reset=COMMON+r'''
struct State {bool running=true;int next=0;} state,*gGameState=&state;
struct ConsoleLogoState {};
namespace Ship {struct Console{};}
#define STOP_GAMESTATE(s) ((s)->running=false)
#define SET_NEXT_GAMESTATE(s,init,size) ((s)->next=1)
#define ERROR_MESSAGE(...) ((void)0)
int returns=0;void MM_RequestComboReturn(){++returns;}
'''+block(reset_source,'static bool ResetHandler(')+r'''
int main() {
    assert(!ResetHandler(nullptr,{},nullptr));
    assert(!state.running && state.next==1 && returns==0);
    state={};gComboGameMode=COMBO_GAME_BOTH;assert(!ResetHandler(nullptr,{},nullptr));
    assert(state.running && returns==1);
    std::cout<<"MM native reset boundary passed\n";
}
'''
play=(ROOT/'mm/src/code/z_play.c').read_text()
start=play.index('                    if (gSaveContext.gameMode == GAMEMODE_OWL_SAVE) {')
end=play.index('                    } else if (gSaveContext.gameMode != GAMEMODE_FILE_SELECT)',start)
owl=COMMON+r'''
struct TitleSetupState {};
constexpr int GAMEMODE_OWL_SAVE=3,TRANS_MODE_OFF=0;
struct {int gameMode=GAMEMODE_OWL_SAVE;} gSaveContext;
struct State {bool running=true;int next=0;};
struct Play {State state;int transitionMode=3;};
#define STOP_GAMESTATE(s) ((s)->running=false)
#define SET_NEXT_GAMESTATE(s,init,size) ((s)->next=1)
int returns=0;void Combo_RequestOwlSaveQuit(){++returns;}
void Quit(Play* self) {
'''+play[start:end].replace('this','self')+r'''
                    }
}
int main(){
    Play p;Quit(&p);assert(!p.state.running && p.state.next==1 && returns==0);
    p={};gComboGameMode=COMBO_GAME_BOTH;Quit(&p);assert(p.state.running && p.transitionMode==0 && returns==1);
    std::cout<<"MM native owl-save exit boundary passed\n";
}
'''
failed=[]
with tempfile.TemporaryDirectory(prefix='mm-native-flow-') as directory:
    tmp=Path(directory)
    for name,program in [('create',creation),('reset',reset),('owl',owl)]:
        cpp=tmp/(name+'.cpp');binary=tmp/name;cpp.write_text(program)
        result=subprocess.run(['g++','-std=c++20','-I',str(ROOT),'-I',str(DEPS/'include'),str(cpp),'-o',str(binary)])
        if result.returncode:failed.append(name+' (compile)');continue
        run=tmp/(name+'-run');run.mkdir()
        if subprocess.run([str(binary),str(run)]).returncode:failed.append(name)
if failed:raise SystemExit('MM native flow failures: '+', '.join(failed))
