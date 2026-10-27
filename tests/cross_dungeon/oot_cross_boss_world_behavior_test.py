#!/usr/bin/env python3
"""Execute production OoT logical dungeon counts and live gameover checkpoint selection."""
import pathlib,re,subprocess,tempfile
import os, sys
sanitize_flags = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-g'] if '--sanitize' in sys.argv else []
if sanitize_flags:
    os.environ['ASAN_OPTIONS'] = 'detect_leaks=0'
root=pathlib.Path(__file__).resolve().parents[2]
def fn(text,marker):
 a=text.index(marker);b=text.index('{',a);d,e=1,b+1
 while d:d+=(text[e]=='{')-(text[e]=='}');e+=1
 return text[a:e]
logic=fn((root/'soh/soh/Enhancements/randomizer/logic.cpp').read_text(),'uint8_t Logic::DungeonCount()').replace('Logic::DungeonCount','CountLogic')
play=fn((root/'soh/src/code/z_play.c').read_text(),'u8 CheckDungeonCount()')
death=fn((root/'soh/soh/Enhancements/randomizer/randomizer_entrance.c').read_text(),'void Entrance_SetGameOverEntrance()') if False else fn((root/'soh/soh/Enhancements/randomizer/randomizer_entrance.c').read_text(),'void Entrance_SetGameOverEntrance(void)')
code=logic+'\n'+play+'\n'+death
names=sorted(set(re.findall(r'\b(?:ENTR_|SCENE_|RSK_|RO_|LOGIC_|RAND_INF_|EVENTCHKINF_)[A-Z0-9_]+',code)))
prefix='''#include <cassert>
#include <cstdint>
#define COMBO_BUILD 1
#include "combo/ComboGameMode.h"
extern "C" { int gComboGameMode = COMBO_GAME_BOTH; }
#include "combo/rando/CrossEntranceBoss.h"
using u8=uint8_t;using s16=int16_t;
'''+ 'enum {'+','.join(names)+'};\n'+r'''
static bool active=false, CalculatingAvailableChecks=false;
static uint16_t logical=0;static int32_t checkpoint=-1;
static int flags[1000]{};static int Get(int id){return flags[id];}
static int CheckEventChkInf(int id){return flags[id];}static int CheckRandoInf(int id){return flags[id];}
static int Flags_GetEventChkInf(int id){return flags[id];}static int Flags_GetRandomizerInf(int id){return flags[id];}
int Combo_SOH_CrossBossEnabled(){return active;}
int Combo_SOH_CrossBossLogicalClear(int id,int native){return active?bool(logical&(1u<<id)):native;}
int32_t Combo_SOH_CrossDungeonDeathCheckpoint(){return checkpoint;}
static int Randomizer_GetSettingValue(int){return RO_BOSS_ROOM_ENTRANCE_SHUFFLE_OFF;}
static int Grotto_OverrideSpecialEntrance(int index){return index;}
static int bossSceneSaveDeathWarps[1000]{};
static struct {int entranceIndex=0;} gSaveContext;
static struct {int sceneNum=0;} playState;
static auto* gPlayState=&playState;
'''
main=r'''
int main(){
 for(auto& flag:flags)flag=1;flags[RAND_INF_DUNGEONS_DONE_GANONS_TOWER]=0;
 active=true;logical=(1u<<2)|(1u<<9);
 assert(CountLogic()==1 && "physical target victories cannot count as source dungeons");
 CalculatingAvailableChecks=true;assert(CountLogic()==1);assert(CheckDungeonCount()==1);
 flags[RAND_INF_DUNGEONS_DONE_GANONS_TOWER]=1;assert(CheckDungeonCount()==2);
 active=false;assert(CountLogic()==8 && CheckDungeonCount()==9);
 active=true;playState.sceneNum=SCENE_DEKU_TREE_BOSS;gSaveContext.entranceIndex=ENTR_DEKU_TREE_BOSS_ENTRANCE;checkpoint=12345;
 Entrance_SetGameOverEntrance();assert(gSaveContext.entranceIndex==12345 && "live death must retain physical destination checkpoint");
 active=false;gSaveContext.entranceIndex=ENTR_DEKU_TREE_BOSS_ENTRANCE;Entrance_SetGameOverEntrance();assert(gSaveContext.entranceIndex==ENTR_DEKU_TREE_ENTRANCE);
}
'''
with tempfile.TemporaryDirectory(prefix='boss-world-') as tmp:
 source=pathlib.Path(tmp)/'test.cpp';binary=pathlib.Path(tmp)/'test';source.write_text(prefix+code+main)
 subprocess.run(['g++',*sanitize_flags,'-std=c++20','-I',str(root),str(source),'-o',str(binary)],check=True);subprocess.run([str(binary)],check=True)
