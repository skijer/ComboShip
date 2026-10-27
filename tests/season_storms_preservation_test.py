#!/usr/bin/env python3
"""Audit/test seasonal presentation isolation from native Storms checks.
Actual source functions run with explicit engine/render/audio adapters; no game
runtime or full weather/audio integration proof. --snippets permits a narrow
captured-source fixture without a source checkout.
"""
import argparse
import json
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
ROD = "soh/mods/items/logic/item_rod_of_seasons.c"
WEATHER = "soh/soh/Enhancements/audio/SeasonWeather.cpp"
RAIN = "soh/soh/Enhancements/audio/GlobalOutdoorRain.cpp"
FLAGS = "soh/src/code/code_8006C3A0.c"
ACTOR = "soh/src/code/z_actor.c"
GROTTO = "soh/src/overlays/actors/ovl_Door_Ana/z_door_ana.c"
STORM = "soh/src/overlays/actors/ovl_En_Okarina_Effect/z_en_okarina_effect.c"
BEAN = "soh/src/overlays/actors/ovl_Obj_Bean/z_obj_bean.c"
FU = "soh/src/overlays/actors/ovl_En_Fu/z_en_fu.c"
GATES = "soh/soh/Enhancements/randomizer/NeiSeasons.cpp"


def function(source, name, native_c=False):
    match = re.search(r'^[\w* "\\]+\b' + re.escape(name) + r'\([^;]*?\)\s*\{.*?^}',
                      source, re.M | re.S)
    if not match:
        raise AssertionError("missing actual function " + name)
    body = match[0]
    # Native C uses a parameter named this; only rename that C token for C++.
    return re.sub(r"\bthis\b", "self", body) if native_c else body


PREFIX = r"""
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
using u8=uint8_t;using u16=uint16_t;using s8=int8_t;using s16=int16_t;using s32=int32_t;using u32=uint32_t;
constexpr int SEASON_SPRING=0,SEASON_SUMMER=1,SEASON_AUTUMN=2,SEASON_WINTER=3,SEASON_OFF=4,SEASON_COUNT=4,SEASON_SLOTS=5;
constexpr int CS_STATE_IDLE=0,GAMEOVER_INACTIVE=0,GAMEMODE_NORMAL=0,TRANS_MODE_OFF=0;
constexpr int ACTORCAT_PROP=0,ACTOR_EN_WEATHER_TAG=17,EN_WEATHER_TAG_TYPE_THUNDERSTORM_KAKARIKO=4;
constexpr int ACTOR_FLAG_UPDATE_CULLING_DISABLED=1,AC_HIT=1,CAM_ID_MAIN=0,BEAN_STATE_BEEN_WATERED=1;
constexpr int LIGHTNING_MODE_ON=1,LIGHTNING_MODE_LAST=2,SEQ_PLAYER_BGM_MAIN=0,NA_BGM_NATURE_AMBIENCE=1;
constexpr int NATURE_CHANNEL_LIGHTNING=0,NATURE_CHANNEL_RAIN=1,CHANNEL_IO_PORT_1=1,NA_SE_SY_CORRECT_CHIME=1;
constexpr int OCARINA_MODE_04=4,FLAG_EVENT_CHECK_INF=0;
#define SOH_SEASON_WEATHER_CVAR(name) "gAudioEditor.SeasonWeather." name
#define GET_ACTIVE_CAM(play) (&nativeCamera)
#define LUSLOG_INFO(...) ((void)0)
#define osSyncPrintf(...) ((void)0)
struct Actor{int id=0,params=0;u32 flags=ACTOR_FLAG_UPDATE_CULLING_DISABLED;float xyzDistToPlayerSq=0,xzDistToPlayer=0;Actor*next=nullptr;void(*update)()=nullptr;};
struct Camera{int unk_14C=0;}nativeCamera;
struct PlayState{
 struct{u32 frames=0;}state;
 struct{bool indoors=false,skyboxDisabled=false,customSkyboxFilter=false;int unk_1F=0,unk_17=0,unk_DE=0,gloomySkyMode=0,lightningMode=0;int unk_F2[4]{};}envCtx;
 struct{struct{Actor*head=nullptr;}actorLists[1];}actorCtx;
 struct{int state=0;}csCtx,gameOverCtx;
 struct{int state=0,debugState=0;}pauseCtx;
 struct{int msgLength=0,ocarinaMode=0;}msgCtx;
 struct{struct{int num=0;}curRoom;}roomCtx;
 int sceneNum=1,transitionMode=0,colChkCtx=0;u16 envFlags[20]{};
};
struct Save{int gameMode=0;u16 eventChkInf[16]{};}gSaveContext;
struct NeiSaveData{u8 seasonsGates=0;}saved;
NeiSaveData*Nei_Save(){return &saved;}
int savedSeason=0,selection=-2,rainMode=-1,seasonGetterCalls=0;
int CVarGetInteger(const char*key,int fallback){
 if(std::strcmp(key,SOH_SEASON_WEATHER_CVAR("Season"))==0)return selection;
 if(std::strstr(key,"RainMode"))return rainMode;
 return fallback;
}
u8 Seasons_SeasonCount(){return 4;}
u8 Seasons_GetSeason(){++seasonGetterCalls;return savedSeason;}
u8 Seasons_RandoMode(){return NEI_SEASONS_GATED;}
int Combo_SOH_CrossBossLogicalClear(int,int native){return native;}
constexpr int COMBO_BOSS_BARINADE=0,COMBO_BOSS_MORPHA=1,COMBO_BOSS_VOLVAGIA=2,COMBO_BOSS_PHANTOM_GANON=3;
constexpr int EVENTCHKINF_USED_JABU_JABUS_BELLY_BLUE_WARP=1,EVENTCHKINF_USED_WATER_TEMPLE_BLUE_WARP=2;
constexpr int EVENTCHKINF_USED_FIRE_TEMPLE_BLUE_WARP=3,EVENTCHKINF_USED_FOREST_TEMPLE_BLUE_WARP=4;
void GameInteractor_ExecuteOnFlagSet(int,int){}
bool FrameAdvance_IsEnabled(PlayState*){return false;}
int gWeatherMode=0,D_8011FB38=0,ambienceStarts=0,ambienceStops=0,chimes=0,beanWatered=0,cutscenes=0;
void Environment_PlayStormNatureAmbience(PlayState*){++ambienceStarts;}
void Environment_StopStormNatureAmbience(PlayState*){++ambienceStops;}
int func_800FA0B4(int){return 0;}
void Audio_SetNatureAmbienceChannelIO(int,int,int){}
void Actor_Kill(Actor*a){a->update=nullptr;}
float gSfxDefaultPos=0,gSfxDefaultFreqAndVolScale=1; s8 gSfxDefaultReverb=0;
void Audio_PlaySfxGeneral(int id,...){assert(id==NA_SE_SY_CORRECT_CHIME);++chimes;}
struct DoorAna;using DoorAnaActionFunc=void(*)(DoorAna*,PlayState*);
struct DoorAna{Actor actor;DoorAnaActionFunc actionFunc=nullptr;struct{struct{int acFlags=0;}base;}collider;};
void DoorAna_WaitOpen(DoorAna*,PlayState*){}
void Collider_DestroyCylinder(PlayState*,void*){}
void Collider_UpdateCylinder(Actor*,void*){}
void CollisionCheck_SetAC(PlayState*,void*,void*){}
void Actor_SetClosestSecretDistance(Actor*,PlayState*){}
struct EnOkarinaEffect;using StormAction=void(*)(EnOkarinaEffect*,PlayState*);
struct EnOkarinaEffect{Actor actor;int timer=0;StormAction actionFunc=nullptr;};
void EnOkarinaEffect_SetupAction(EnOkarinaEffect*self,StormAction action){self->actionFunc=action;}
void EnOkarinaEffect_ManageStorm(EnOkarinaEffect*,PlayState*);
struct ObjBean{u32 stateFlags=0;struct{Actor actor;}dyna;void(*transformFunc)(ObjBean*)=nullptr;};
ObjBean*D_80B90E30=nullptr;
void BeanTransform(ObjBean*){}
void ObjBean_SetupGrowWaterPhase1(ObjBean*){++beanWatered;}
void OnePointCutscene_Init(PlayState*,int,int,Actor*,int){++cutscenes;}
struct EnFu;using FuAction=void(*)(EnFu*,PlayState*);
struct EnFu{FuAction actionFunc=nullptr;};
void EnFu_WaitAdult(EnFu*,PlayState*){}
int requestedDensity=0;float requestedIntensity=0;bool requestedOvercast=false,skyAllowed=true;
void GlobalOutdoorRain_SetSeasonRequest(PlayState*play,int density,float intensity,int overcast){
 if(!play){requestedDensity=0;requestedIntensity=0;requestedOvercast=false;return;}
 requestedDensity=density;requestedIntensity=intensity;requestedOvercast=overcast!=0;
}
void GlobalOutdoorRain_SetSeasonSkyAllowed(PlayState*,int allowed){skyAllowed=allowed!=0;}
"""

MAIN = r"""
static bool Case(int season,int presentation,int mode,bool indoors){
 PlayState play{};gSaveContext={};saved={};savedSeason=season;selection=presentation;rainMode=mode;
 // Unrelated nonzero saved word must survive presentation and native pulses.
 constexpr u16 savedSentinel=0xA55A;gSaveContext.eventChkInf[1]=savedSentinel;
 play.envCtx.indoors=indoors;SeasonWeather_Reset();Flags_SetEnv(&play,7);
 DoorAna grotto{};grotto.actor.params=0x100;grotto.actionFunc=DoorAna_WaitClosed;
 ObjBean bean{};bean.transformFunc=BeanTransform;D_80B90E30=nullptr;beanWatered=cutscenes=chimes=0;
 for(int frame=0;frame<120;++frame){
  ++play.state.frames;ROD_CALL;assert(gSaveContext.eventChkInf[1]==savedSentinel);
  DoorAna_WaitClosed(&grotto,&play);ObjBean_WaitForWater(&bean,&play);
  if(play.envFlags[0]!=(1<<7)||grotto.actionFunc!=DoorAna_WaitClosed||chimes||beanWatered){
   std::fprintf(stderr,"FAIL forged native song: saved=%d presentation=%d rain=%d indoors=%d flags=%u\n",season,presentation,mode,indoors,play.envFlags[0]);return false;
  }
  for(unsigned i=1;i<20;++i)assert(!play.envFlags[i]);
  assert(!Flags_GetEventChkInf(EVENTCHKINF_LEARNED_SONG_OF_STORMS));
 }
 const int expected=indoors?-1:presentation==-2?(season<SEASON_COUNT?season:-1):presentation;
 assert(SeasonWeather_SeasonForPlay(&play)==expected);
 if(expected==SEASON_WINTER)assert(requestedOvercast&&requestedDensity==0);
 if(expected==SEASON_AUTUMN&&(mode==-1||mode==1))assert(requestedDensity==30&&requestedOvercast);
 if(expected==SEASON_SUMMER||expected<0)assert(requestedDensity==0);
 // Actual native storm actor, not a fixture-created Flags_SetEnv shortcut.
 EnOkarinaEffect storm{};EnOkarinaEffect_TriggerStorm(&storm,&play);assert(storm.timer==400);
 storm.timer=309;EnOkarinaEffect_ManageStorm(&storm,&play);assert(storm.timer==308&&Flags_GetEnv(&play,5));
 ++play.state.frames;ROD_CALL;assert(gSaveContext.eventChkInf[1]==savedSentinel);
 assert(Flags_GetEnv(&play,5)&&Flags_GetEnv(&play,7));
 DoorAna_WaitClosed(&grotto,&play);ObjBean_WaitForWater(&bean,&play);
 assert(grotto.actionFunc==DoorAna_WaitOpen&&chimes==1&&beanWatered==1&&cutscenes==1);
 // A native played-song pulse does not complete the separate Windmill check.
 Seasons_UpdateGates();assert(saved.seasonsGates==0);assert(gSaveContext.eventChkInf[1]==savedSentinel);
 EnOkarinaEffect_ManageStorm(&storm,&play);assert(!Flags_GetEnv(&play,5));
 ++play.state.frames;ROD_CALL;assert(gSaveContext.eventChkInf[1]==savedSentinel);assert(!Flags_GetEnv(&play,5)&&Flags_GetEnv(&play,7));
 ObjBean_WaitForWater(&bean,&play);assert(!D_80B90E30);
 // Actual Windmill completion sets saved event0x5B; policy grants Spring bit0.
 EnFu windmill{};func_80A1DB60(&windmill,&play);
 assert(Flags_GetEventChkInf(EVENTCHKINF_LEARNED_SONG_OF_STORMS));
 Seasons_UpdateGates();assert(saved.seasonsGates==(1<<SEASON_SPRING));
 assert(!(saved.seasonsGates&(1<<SEASON_WINTER))&&!Flags_GetEnv(&play,5));
 Flags_SetEventChkInf(EVENTCHKINF_LEARNED_SERENADE_OF_WATER);Seasons_UpdateGates();
 assert(saved.seasonsGates==((1<<SEASON_SPRING)|(1<<SEASON_WINTER)));
 assert(gSaveContext.eventChkInf[1]==savedSentinel);
 return true;
}
int main(){
 unsigned cases=0;
 // Winter override first also diagnoses the historical Autumn-saved-state bug.
 for(int season=0;season<SEASON_SLOTS;++season)for(int presentation:{3,-2,-1,0,1,2})
 for(int mode:{-1,0,1,2})for(bool indoors:{false,true}){
  if(!Case(season,presentation,mode,indoors)){return 1;}
  ++cases;
 }
 std::printf("PASS %u seasonal/override/rain/indoor cases: no forged/consumed Storms; native actor opens grotto/bean; Windmill gate remains separate\n",cases);
}
"""


def program(read):
    policy = read("combo/NeiSeasonsPolicy.h").replace("#pragma once", "")
    rain_header = read("soh/soh/Enhancements/audio/GlobalOutdoorRain.h").replace("#pragma once", "")
    weather, rain = read(WEATHER), read(RAIN)
    # Transitive native rain rendering/audio is an explicit fixture boundary.
    # Guard the complete inspected controllers against direct puzzle-flag writes.
    for source in (weather, rain):
        assert not re.search(r"\bFlags_(?:Set|Unset)Env\s*\(", source), "weather writes gameplay env flags"
        assert "envFlags" not in source, "weather directly mutates env flag storage"
    macros = read("soh/include/z64save.h")
    event_defines = "\n".join(re.search(r"^#define " + name + r"\s+[^\n]+", macros, re.M)[0] for name in (
        "EVENTCHKINF_LEARNED_SONG_OF_STORMS", "EVENTCHKINF_LEARNED_SERENADE_OF_WATER"))
    pure = "\n".join(function(rain, name) for name in (
        "GlobalOutdoorRain_ClampDensity", "GlobalOutdoorRain_ClampIntensity",
        "GlobalOutdoorRain_AdvanceCycle", "GlobalOutdoorRain_ScaleDensity"))
    flags = "\n".join(function(read(FLAGS), name, True) for name in (
        "Flags_SetEnv", "Flags_UnsetEnv", "Flags_GetEnv"))
    event = "\n".join(function(read(ACTOR), name, True) for name in (
        "Flags_GetEventChkInf", "Flags_SetEventChkInf"))
    state = weather[weather.index("constexpr int kLogicHz"):weather.index("int RandomFrames(")]
    controller = "\n".join(function(weather, name) for name in (
        "RandomFrames", "RainMode", "HasNativeSkyWeather", "ResolveSeasonForPlay",
        "SeasonWeather_SeasonForPlay", "SeasonWeather_Reset", "SeasonWeather_Update"))
    bridge = function(read(ROD), "Seasons_ApplyWeather", True)
    native = "\n".join(function(read(path), name, True) for path, name in (
        (GROTTO,"DoorAna_SetupAction"),(GROTTO,"DoorAna_WaitClosed"),
        (STORM,"EnOkarinaEffect_TriggerStorm"),(STORM,"EnOkarinaEffect_ManageStorm"),
        (BEAN,"ObjBean_WaitForWater"),(FU,"func_80A1DB60"),(GATES,"Seasons_UpdateGates")))
    call = "Seasons_ApplyWeather(&play, static_cast<u8>(savedSeason))" if ", u8 season)" in bridge else "Seasons_ApplyWeather(&play)"
    return ("#define GLOBAL_OUTDOOR_RAIN_TEST\n" + policy + rain_header + event_defines + "\n" +
            PREFIX + pure + flags + event + state + controller + bridge + native +
            MAIN.replace("ROD_CALL", call))


def run(read):
    with tempfile.TemporaryDirectory(prefix="season-storms-preservation-") as temporary:
        source=Path(temporary)/"test.cpp"
        binary=Path(temporary)/"test"
        source.write_text(program(read))
        subprocess.run(["c++","-std=c++20","-Wall","-Wextra","-Werror","-Wno-unused-parameter",
                        "-fsanitize=address,undefined","-fno-pie","-no-pie",
                        str(source),"-o",str(binary)],check=True)
        import os
        subprocess.run([str(binary)],check=True,env={**os.environ,"ASAN_OPTIONS":"detect_leaks=0"})


if __name__=="__main__":
    parser=argparse.ArgumentParser()
    parser.add_argument("--snippets",type=Path)
    args=parser.parse_args()
    snippets=json.loads(args.snippets.read_text()) if args.snippets else None
    run(lambda path:snippets[path] if snippets else (ROOT/path).read_text())
