#!/usr/bin/env python3
"""Compile production seasonal/market paths against small engine boundaries.

This is a partial-source regression fixture, not a game build. Soil executes the
real latch/spawner and native parameter/collected-token/chime branches. The well
executes its complete native init/update. Market executes native entry guards,
the complete target search, and the complete A-talk offer functions. The large
unchanged graphics bodies and ordinary companion update tails are sentinels.
"""
from pathlib import Path
import argparse
import os
import re
import subprocess
import tempfile


def block(src, marker):
    start = src.index(marker)
    opening = src.index('{', start)
    while ';' in src[start:opening]:
        start = src.index(marker, start + len(marker))
        opening = src.index('{', start)
    depth, end = 1, opening + 1
    while depth:
        depth += (src[end] == '{') - (src[end] == '}')
        end += 1
    return src[start:end]


def prefix(src, marker, stop, tail):
    fn = block(src, marker)
    return fn[:fn.index(stop)] + tail + '\n}'


COMMON = r'''
#include <algorithm>
#include <cassert>
#include <cfloat>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <string>
using s16 = int16_t; using u16 = uint16_t; using s32 = int32_t;
using u32 = uint32_t; using u8 = uint8_t; using f32 = float;
struct Vec3f { float x, y, z; }; struct Vec3s { s16 x,y,z; };
struct PosRot { Vec3f pos; Vec3s rot; };
struct PlayState;
struct Actor {
    s16 id{}, params{}; PosRot world{}, home{}, focus{}; struct { Vec3s rot; } shape{};
    void (*init)(Actor*, PlayState*){}; u32 flags{};
    bool isTargeted{}, isLockedOn{}; float yDistToPlayer{}, playerHeightRel{}, xzDistToPlayer{};
};
static std::set<std::string> failures;
static void check(bool pass, const char* message) {
    if (!pass && failures.insert(message).second) std::fprintf(stderr, "FAIL: %s\n", message);
}
enum { SEASON_OFF, SEASON_SPRING, SEASON_SUMMER, SEASON_AUTUMN, SEASON_WINTER };
static int season = SEASON_WINTER, seasonCount = 1;
static int Seasons_SeasonCount() { return seasonCount; }
static int Seasons_GetSeason() { return season; }
'''


def soil(root):
    scene = (root / 'soh/mods/season_scene.cpp').read_text()
    src = (root / 'soh/src/overlays/actors/ovl_Obj_Makekinsuta/z_obj_makekinsuta.c').read_text()
    sw = (root / 'soh/src/overlays/actors/ovl_En_Sw/z_en_sw.c').read_text()
    wrapper = block(sw, 'Actor* EnSw_SpawnFromSoil(') if 'Actor* EnSw_SpawnFromSoil(' in sw else ''
    init = block(sw, 'void EnSw_Init(Actor* thisx, PlayState* play) {')
    # Keep the native conversion/collected-token early return and exact chime
    # branch; exclude the unchanged skeleton, collision and launch setup.
    end = init.index('    SkelAnime_Init(')
    native_init = init[:end].replace('void EnSw_Init(', 'void NativeInit(', 1)
    native_init += block(init, 'if (((thisx->params & 0xE000) >> 0xD) >= 3') + '\n}'
    return COMMON + r'''
enum { ACTOR_EN_SW = 0x95, SOFT_SOIL_TRIGGER_SEASONAL = 2, VB_SPAWN_BEAN_SKULLTULA,
       ENSW_SOIL_QUIET_ROT_Z=0x7FFF };
struct ActorContext {};
struct PlayState { ActorContext actorCtx; };
struct ObjMakekinsuta { Actor actor; void (*actionFunc)(ObjMakekinsuta*,PlayState*); s16 timer, unk_152; };
struct EnSw { Actor actor; };
static int chimes, spawnCalls, collected, killed;
static bool failSpawn, deferred, nestedManual, preInitDirect;
static EnSw spawned, unrelated;
static int spawnDepth;
static Actor* lastChimeActor;
static float spawnX, spawnY, spawnZ; static s16 spawnYaw, spawnParams;
static int gSfxDefaultPos, gSfxDefaultFreqAndVolScale, gSfxDefaultReverb;
#define NA_SE_SY_CORRECT_CHIME 1
#define GET_GS_FLAGS(index) collected
#define GET_PLAYER(play) nullptr
#define GameInteractor_Should(v, condition) (condition)
#define func_8002DEEC(player) false
#define Audio_PlaySfxGeneral(...) (++chimes, lastChimeActor=thisx)
static void Actor_Kill(Actor*) { ++killed; }
void NativeInit(Actor*,PlayState*);
Actor* EnSw_SpawnFromSoil(Actor*,PlayState*,s32);
static Actor* Actor_Spawn(ActorContext* ctx,PlayState* play,s16,float x,float y,float z,s16,s16 yaw,s16 rotZ,s16 params) {
    ++spawnCalls;
    if(!spawnDepth) { spawnX=x; spawnY=y; spawnZ=z; spawnYaw=yaw; spawnParams=params; }
    if (failSpawn) return nullptr;
    EnSw* target=spawnDepth?&unrelated:&spawned; *target={}; target->actor.params=params;
    target->actor.home.rot.z=target->actor.world.rot.z=target->actor.shape.rot.z=target->actor.focus.rot.z=rotZ;
    ++spawnDepth;
    // Actor_Init's real ShouldActorInit callback occurs before the native init.
    if(preInitDirect) { preInitDirect=false; Actor_Spawn(ctx,play,ACTOR_EN_SW,1,2,3,0,0,0,s16(0xC102)); }
    if (deferred) target->actor.init=NativeInit;
    else NativeInit(&target->actor,play);
    if (nestedManual) {
        nestedManual=false;
        Actor manual{}; manual.params=0x4101;
        EnSw_SpawnFromSoil(&manual,play,true);
    }
    --spawnDepth;
    return &target->actor;
}
void ObjMakekinsuta_DoNothing(ObjMakekinsuta*, PlayState*) {}
''' + '\n' + native_init + '\n' + (wrapper or r'''
Actor* EnSw_SpawnFromSoil(Actor* soil,PlayState* play,s32) {
 return Actor_Spawn(&play->actorCtx,play,ACTOR_EN_SW,soil->world.pos.x,soil->world.pos.y,
 soil->world.pos.z,0,soil->shape.rot.y,0,soil->params|0x8000);
}
''') + '\n'.join(block(scene, m) for m in (
        'static u8 SeasonScene_Active(void) {',
        'static void SeasonScene_LatchSoftSoil(Actor* self) {',
        'static void SeasonScene_OnInitSoftSoil(void* refActor) {')) + '\n' + block(
            src, 'void func_80B98320(ObjMakekinsuta* this, PlayState* play) {') + r'''
int main() {
 PlayState play{};
 auto make = [] { ObjMakekinsuta soil{}; soil.actor.params=0x4101;
   soil.actor.world.pos={12,34,56}; soil.actor.shape.rot.y=1234; soil.timer=60; return soil; };
 for (bool delay : {false,true}) {
   ObjMakekinsuta soil=make(); deferred=delay;
   SeasonScene_OnInitSoftSoil(&soil.actor);
   chimes=0; func_80B98320(&soil,&play);
   if (spawned.actor.init) { spawned.actor.init(&spawned.actor,&play); spawned.actor.init=nullptr; }
   check(chimes==0,"Winter automatic soil reveal played the puzzle chime");
   check(spawned.actor.home.rot.z==0 && spawned.actor.world.rot.z==0 && spawned.actor.shape.rot.z==0 && spawned.actor.focus.rot.z==0,
         "quiet spawn marker escaped into native actor orientation");
   check(spawnX==12 && spawnY==34 && spawnZ==56 && spawnYaw==1234 && spawnParams==s16(0xC101),
         "seasonal spawn changed placement, yaw or Skulltula identity");
   check(soil.actionFunc==ObjMakekinsuta_DoNothing,"soil did not keep one-shot behavior");
   for (int nativeTrigger : {1,1}) { // Bugs and shovel both write the native value 1.
     soil=make(); soil.unk_152=nativeTrigger; chimes=0; func_80B98320(&soil,&play);
     if (spawned.actor.init) spawned.actor.init(&spawned.actor,&play);
     check(chimes==1,"genuine bugs/shovel soil reveal lost its puzzle chime");
   }
 }
 deferred=false;
 ObjMakekinsuta soil=make(); SeasonScene_OnInitSoftSoil(&soil.actor);
 failSpawn=true; func_80B98320(&soil,&play); failSpawn=false;
 chimes=0; EnSw_SpawnFromSoil(&soil.actor,&play,true);
 check(chimes==1,"failed seasonal spawn leaked chime suppression");
 chimes=0; nestedManual=true; EnSw_SpawnFromSoil(&soil.actor,&play,false);
 check(chimes==1,"nested genuine spawn inherited seasonal chime suppression");
 chimes=0; preInitDirect=true; EnSw_SpawnFromSoil(&soil.actor,&play,false);
 check(chimes==1 && lastChimeActor==&unrelated.actor,
       "pre-init reentrant Actor_Spawn stole suppression from the seasonal actor");
 EnSw placed{}; placed.actor.params=0x6102;
 placed.actor.home.rot.z=placed.actor.world.rot.z=placed.actor.shape.rot.z=placed.actor.focus.rot.z=ENSW_SOIL_QUIET_ROT_Z;
 chimes=0; NativeInit(&placed.actor,&play);
 check(chimes==1 && placed.actor.home.rot.z==ENSW_SOIL_QUIET_ROT_Z,
       "non-soil actor interpreted its genuine rotation as a quiet-birth marker");
 collected=1; chimes=0; killed=0; EnSw_SpawnFromSoil(&soil.actor,&play,true);
 check(killed==1 && chimes==0,"collected-token early return changed"); collected=0;
 for(int s : {SEASON_OFF,SEASON_SPRING,SEASON_SUMMER,SEASON_AUTUMN}) {
   season=s; soil=make(); SeasonScene_OnInitSoftSoil(&soil.actor);
   check(soil.unk_152==0,"non-Winter soil was automatically triggered");
 }
 season=SEASON_WINTER; seasonCount=0; soil=make(); SeasonScene_OnInitSoftSoil(&soil.actor);
 check(soil.unk_152==0,"unowned/inactive season triggered soil");
 if(failures.empty()) std::puts("PASS soil: quiet automatic Winter reveal, immediate/deferred init, genuine reveals, identity, failure/nesting and collected flags");
 return !failures.empty();
}
'''


def well(root):
    scene = (root / 'soh/mods/season_scene.cpp').read_text()
    src = (root / 'soh/src/overlays/actors/ovl_Bg_Spot01_Idomizu/z_bg_spot01_idomizu.c').read_text()
    header = (root / 'soh/src/overlays/actors/ovl_Bg_Spot01_Idomizu/z_bg_spot01_idomizu.h').read_text()
    declaration = header[header.index('typedef struct BgSpot01Idomizu {'):header.index('// size')]
    offset = block(scene, 'extern "C" f32 SeasonScene_WaterLevelOffset(void) {') if 'extern "C" f32 SeasonScene_WaterLevelOffset(void) {' in scene else 'extern "C" f32 SeasonScene_WaterLevelOffset(void) { return 0; }'
    return COMMON + r'''
enum { ACTOR_BG_SPOT01_IDOMIZU=1, ACTOR_BG_MORI_IDOMIZU=2, ACTOR_BG_HAKA_WATER=3,
 EVENTCHKINF_DRAINED_WELL_IN_KAKARIKO, YEARS_ADULT=17 };
struct WaterBox { s16 ySurface; }; struct CollisionHeader { WaterBox* waterBoxes; };
struct PlayState { struct { CollisionHeader* colHeader; } colCtx; };
struct BgSpot01Idomizu;
using BgSpot01IdomizuActionFunc=void(*)(BgSpot01Idomizu*,PlayState*);
static bool drained; static int age=10, sounds;
#define LINK_AGE_IN_YEARS age
#define Flags_GetEventChkInf(flag) drained
#define Actor_ProcessInitChain(...) ((void)0)
#define Audio_PlaySfxGeneral(...) (++sounds)
#define SEASON_WATER_DROP 600.0f
#define SEASON_WATER_RISE 100.0f
static void Math_ApproachF(float* y,float target,float,float step) {
 *y += std::clamp(target-*y,-step,step);
}
''' + declaration + '\nvoid BgSpot01Idomizu_UpdateWaterLevel(BgSpot01Idomizu*,PlayState*);\n' + block(
        scene, 'static u8 SeasonScene_Active(void) {') + '\n' + offset + '\n' + block(
            scene, 'static void SeasonScene_OnInitWaterActor(void* refActor) {') + '\n' + '\n'.join(
                block(src, m) for m in ('void BgSpot01Idomizu_Init(Actor* thisx, PlayState* play) {',
                                       'void BgSpot01Idomizu_UpdateWaterLevel(BgSpot01Idomizu* this, PlayState* play) {')) + r'''
int main() {
 WaterBox box{}; CollisionHeader header{&box}; PlayState play{{&header}};
 for (int s : {SEASON_OFF,SEASON_SPRING,SEASON_SUMMER,SEASON_AUTUMN,SEASON_WINTER})
  for (int owned : {0,1}) for(int years : {10,17}) for(bool solved : {false,true}) {
   season=s; seasonCount=owned; age=years; drained=solved; sounds=0;
   BgSpot01Idomizu water{}; water.actor.id=ACTOR_BG_SPOT01_IDOMIZU;
   BgSpot01Idomizu_Init(&water.actor,&play); SeasonScene_OnInitWaterActor(&water.actor);
   float delta=owned?(s==SEASON_SUMMER?-600.f:s==SEASON_AUTUMN?100.f:0.f):0.f;
   float expected=(solved||years==17?-550.f:52.f)+delta;
   check(water.actor.world.pos.y==expected && water.waterHeight==expected,
         "season shifted Kakariko well surface without its native target");
   for(int i=0;i<100;i++) BgSpot01Idomizu_UpdateWaterLevel(&water,&play);
   check(sounds==0 && water.actor.world.pos.y==expected && box.ySurface==s16(expected),
         "seasonal entry falsely drained the well or played its drain sound");
   BgSpot01Idomizu_Init(&water.actor,&play); SeasonScene_OnInitWaterActor(&water.actor);
   check(water.actor.world.pos.y==expected,"well offset stacked on re-entry");
  }
 seasonCount=1; age=10; drained=false; season=SEASON_SPRING; sounds=0;
 BgSpot01Idomizu water{}; water.actor.id=ACTOR_BG_SPOT01_IDOMIZU;
 BgSpot01Idomizu_Init(&water.actor,&play);
 for(int s : {SEASON_AUTUMN,SEASON_SUMMER,SEASON_WINTER,SEASON_OFF}) {
   season=s; BgSpot01Idomizu_UpdateWaterLevel(&water,&play);
   float expected=52.f+(s==SEASON_AUTUMN?100.f:s==SEASON_SUMMER?-600.f:0.f);
   check(water.actor.world.pos.y==expected && water.waterHeight==expected && sounds==0,
         "in-place season change caused a false drain before reload");
 }
 season=SEASON_AUTUMN; BgSpot01Idomizu_UpdateWaterLevel(&water,&play);
 drained=true; BgSpot01Idomizu_UpdateWaterLevel(&water,&play);
 check(sounds==1 && water.waterHeight==-450 && water.actor.world.pos.y==150,
       "real Song of Storms drain lost its sound, target or native 2-unit animation");
 for(int i=0;i<400;i++) BgSpot01Idomizu_UpdateWaterLevel(&water,&play);
 check(water.actor.world.pos.y==-450 && drained,"real well progression did not settle");
 for(int id : {ACTOR_BG_MORI_IDOMIZU,ACTOR_BG_HAKA_WATER}) {
   Actor other{}; other.id=id; other.world.pos.y=other.home.pos.y=10;
   SeasonScene_OnInitWaterActor(&other);
   check(other.world.pos.y==110 && other.home.pos.y==110,"other well seasonal adapter regressed");
 }
 struct OldLayout { Actor actor; BgSpot01IdomizuActionFunc actionFunc; f32 waterHeight; char unused[4]; };
 check(sizeof(OldLayout)==sizeof(BgSpot01Idomizu),"well actor instance size changed");
 if(failures.empty()) std::puts("PASS well: all seasons/ownership/ages/progression, reload and live offset changes are quiet; genuine drain remains audible");
 return !failures.empty();
}
'''


def market(root, host):
    oot = host == 'soh'
    elf = (root / host / 'src/overlays/actors/ovl_En_Elf/z_en_elf.c').read_text()
    actor = (root / host / 'src/code/z_actor.c').read_text()
    update = 'void func_80A053F0(Actor* thisx, PlayState* play) {' if oot else 'void func_8089010C(Actor* thisx, PlayState* play) {'
    stop = '    if (player->naviTextId == 0)' if oot else '    u16 temp_v0 = QuestHint_GetTatlTextId(play);'
    functions = block(elf, 'static s32 EnElf_BlackMarketQuiet(') + '\n'
    functions += prefix(elf, update, stop, '++normalUpdates; this->actionFunc(this,play);') + '\n'
    functions += prefix(elf, 'void EnElf_Draw(Actor* thisx, PlayState* play) {',
                        '    if ((this->unk_2A8' if oot else '    if (player->currentMask', '++bodyDraws;') + '\n'
    draw_marker = 'void Attention_Draw(TargetContext* targetCtx, PlayState* play) {' if oot else 'void Attention_Draw(Attention* attention, PlayState* play) {'
    functions += prefix(actor, draw_marker, '    OPEN_DISPS(' if oot else '    if (player->stateFlags1 &', '++attentionDraws;') + '\n'
    functions += block(actor, 'Actor* Attention_FindActor(' if oot else 'void Attention_FindActor(') + '\n'
    functions += '\n'.join(block(actor, m) for m in ('s32 Actor_OfferTalkExchange(', 's32 Actor_OfferTalkExchangeEquiCylinder(', 's32 Actor_OfferTalk('))
    defines = r'''
#include "BlackMarketFairy.h"
#include "entrances/BlackMarketEntrance.h"
extern "C" { int gComboGameMode=COMBO_GAME_BOTH; }
enum { SCENE_SKIEJER_MARKET=222, FAIRY_NAVI=0, FAIRY_TYPE_0=0,
 PLAYER_STATE2_NAVI_ALERT=1, PLAYER_STATE2_200000=1, ACTOR_FLAG_TALK_OFFER_AUTO_ACCEPTED=2,
 ACTOR_FLAG_TALK=4, EXCH_ITEM_NONE=0, PLAYER_IA_NONE=0, CS_ID_GLOBAL_TALK=1 };
using PlayerItemAction=int;
struct Player { Actor actor; Actor* talkActor{}; float talkActorDistance=FLT_MAX;
 u32 stateFlags2{}; s16 naviTextId{},tatlTextId{}; u32 exchangeItemId{}; int exchangeItemAction{}; };
struct TargetContext { Actor* targetedActor{}; Actor* bgmEnemy{}; };
struct Attention { Actor* reticleActor{}; Actor* bgmEnemy{}; };
struct ActorContext { TargetContext targetCtx; Attention attention; };
struct PlayState { int sceneNum{},sceneId{}; Player player; ActorContext actorCtx; };
#define GET_PLAYER(play) (&(play)->player)
using Gfx=int;
struct LightInfo { int radius=100; bool glow=true; };
struct EnElf { Actor actor; u32 fairyFlags{}; void* elfMsg{};
 void (*actionFunc)(EnElf*,PlayState*){}; int timer{};
 LightInfo lightInfoGlow,lightInfoNoGlow; };
static int normalUpdates,bodyDraws,attentionDraws,actionCalls,blinks,idleCalls,searches,talkQueues;
static void Action(EnElf*,PlayState*) { ++actionCalls; }
static void Lights_PointNoGlowSetInfo(LightInfo* light,float,float,float,int,int,int,int radius) {
 light->radius=radius; light->glow=false;
}
#define EnElf_UpdateMidnaBlink(...) (++blinks)
#define MidnaAudio_UpdateIdle(...) (++idleCalls)
#define MMMidnaAudio_UpdateIdle(...) (++idleCalls)
static bool Player_InCsMode(PlayState*) { return false; }
static void CutsceneManager_Queue(int) { ++talkQueues; }
static Actor merchandise;
#define ARRAY_COUNT(a) s32(sizeof(a)/sizeof((a)[0]))
'''
    if oot:
        defines += r'''
static Actor *D_8015BBE8,*D_8015BBEC;
static f32 D_8015BBF0,sbgmEnemyDistSq;
static s32 D_8015BBF8,D_8015BBFC;
static u8 D_801160A0[]={0,1,2,3,4,5};
static void Attention_FindActorInCategory(PlayState*,ActorContext* ctx,Player*,int) {
 ++searches; D_8015BBE8=&merchandise; ctx->targetCtx.bgmEnemy=&merchandise;
}
'''
    else:
        defines += r'''
static Actor *sNearestAttentionActor,*sPrioritizedAttentionActor,*sNearestCameraDriftActor,*sPrioritizedCameraDriftActor;
static f32 sNearestAttentionActorDistSq,sNearestCameraDriftActorDistSq,sBgmEnemyDistSq;
static s32 sHighestAttentionPriority,sHighestCameraDriftPriority,sAttentionPlayerRotY;
static u8 sAttentionCategorySearchOrder[]={0,1,2,3,4,5};
static void Attention_FindActorInCategory(PlayState*,ActorContext* ctx,Player*,int) {
 ++searches; sNearestAttentionActor=sNearestCameraDriftActor=&merchandise; ctx->attention.bgmEnemy=&merchandise;
}
'''
    main = r'''
int main() {
 PlayState play{}; EnElf fairy{}; fairy.actionFunc=Action;
 Actor keeper{}; keeper.xzDistToPlayer=100; keeper.flags=0x100;
 merchandise.flags=0x200;
 for(int mode : {COMBO_GAME_BOTH,COMBO_GAME_OOT,COMBO_GAME_MM})
  for(bool inside : {false,true,false}) for(int kind : {0,1}) {
   gComboGameMode=mode; fairy.actor.params=kind;
   play.sceneNum=inside?SCENE_SKIEJER_MARKET:1; play.sceneId=inside?COMBO_BLACK_MARKET_SCENE:99;
   bool quiet=false;
#ifdef COMBO_BUILD
   quiet=inside && (mode==COMBO_GAME_BOTH || mode==HOST);
#endif
   bodyDraws=attentionDraws=normalUpdates=actionCalls=blinks=idleCalls=searches=0;
   fairy.lightInfoGlow={100,true}; fairy.lightInfoNoGlow={100,false};
   fairy.fairyFlags=0x30; fairy.elfMsg=&keeper;
   fairy.actor.flags=ACTOR_FLAG_TALK_OFFER_AUTO_ACCEPTED;
   play.player.naviTextId=play.player.tatlTextId=-1; play.player.stateFlags2=1;
   UPDATE(&fairy.actor,&play); EnElf_Draw(&fairy.actor,&play);
   check(bodyDraws==(quiet && kind==0?0:1),"shop companion body (Midna/native fairy) still reached rendering");
   if(quiet && kind==0) {
     check(actionCalls==0 && blinks==0 && normalUpdates==0,"hidden shop companion still ran movement/targeting/blink");
     check(fairy.lightInfoGlow.radius==0 && !fairy.lightInfoGlow.glow && fairy.lightInfoNoGlow.radius==0,
           "hidden companion retained separately rendered halo or illumination");
     check(play.player.naviTextId==0 || play.player.tatlTextId==0,"shop companion text did not clear");
     check(play.player.stateFlags2==0 && fairy.elfMsg==nullptr,"stale companion prompt survived");
   } else check(normalUpdates==1 && actionCalls==1,"ordinary scene/fairy update did not pass through");
   ATTENTION_DRAW;
   check(attentionDraws==(quiet?0:1),"shop attention arrow/reticle still reached rendering");
   Actor* candidate=&merchandise; Actor* drift=&merchandise;
   play.actorCtx.targetCtx.bgmEnemy=play.actorCtx.attention.bgmEnemy=&merchandise;
   FIND;
   check(candidate==(quiet?nullptr:&merchandise),"shop merchandise remained a target candidate");
   if(quiet) check(searches==0,"shop still searched actor categories for targets");
   CHECK_DRIFT;
   play.player.actor.flags=0; play.player.talkActor=nullptr; play.player.talkActorDistance=FLT_MAX;
   check(Actor_OfferTalk(&keeper,&play,150),"untargeted keeper lost A-talk offer");
   check(play.player.talkActor==&keeper && !keeper.isLockedOn && !keeper.isTargeted,
         "keeper talk depended on lock-on");
   check(keeper.flags==0x100 && merchandise.flags==0x200,"shop policy mutated stock/keeper actor flags");
   keeper.xzDistToPlayer=200; play.player.talkActorDistance=FLT_MAX;
   check(!Actor_OfferTalk(&keeper,&play,150),"out-of-range A-talk behavior changed");
   keeper.xzDistToPlayer=100;
  }
 if(failures.empty()) std::puts("PASS market HOSTNAME: companion body/halo/update and targeting hidden only in shop; modes, other fairies, exit pass-through and native A-talk preserved");
 return !failures.empty();
}
'''
    main = main.replace('HOSTNAME', host).replace('HOST', 'COMBO_GAME_OOT' if oot else 'COMBO_GAME_MM')
    main = main.replace('UPDATE', 'func_80A053F0' if oot else 'func_8089010C')
    main = main.replace('ATTENTION_DRAW', 'Attention_Draw(&play.actorCtx.targetCtx,&play)' if oot else 'Attention_Draw(&play.actorCtx.attention,&play)')
    main = main.replace('FIND', 'Attention_FindActor(&play,&play.actorCtx,&candidate,&play.player)' if oot else 'Attention_FindActor(&play,&play.actorCtx,&candidate,&drift,&play.player)')
    main = main.replace('CHECK_DRIFT', '(void)drift' if oot else 'check(drift==(quiet?nullptr:&merchandise),"shop camera-drift target survived")')
    return COMMON + defines + functions + main


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument('--case', choices=('soil','well','market','all'), default='all')
    args = parser.parse_args()
    cases = []
    if args.case in ('soil','all'): cases.append(('soil',soil(args.root),True))
    if args.case in ('well','all'): cases.append(('well',well(args.root),True))
    if args.case in ('market','all'):
        for host in ('soh','mm'):
            for combo in (True,False): cases.append((f'market-{host}-{combo}',market(args.root,host),combo))
    failed = False
    with tempfile.TemporaryDirectory(prefix='season-shop-native-') as work:
        for name,source,combo in cases:
            print('Fixture:', name, flush=True)
            cpp=Path(work)/(name+'.cpp'); binary=Path(work)/name
            cpp.write_text(re.sub(r'\bthis\b','self',source))
            cmd=['c++','-std=c++20','-O1','-g','-Wall','-Wextra','-Werror',
                 '-Wno-unused-parameter','-Wno-unused-variable','-Wno-unused-function',
                 '-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-no-pie',
                 '-I'+str(args.root/'combo')]
            if combo: cmd.append('-DCOMBO_BUILD')
            subprocess.run(cmd+[str(cpp),'-o',str(binary)],check=True)
            run=subprocess.run([str(binary)],env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
            failed |= run.returncode != 0
    return int(failed)


if __name__=='__main__':
    raise SystemExit(main())
