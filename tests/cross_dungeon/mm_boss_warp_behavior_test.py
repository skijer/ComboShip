#!/usr/bin/env python3
"""Compile and execute extracted production MM boss/skip/route functions with engine boundary stubs."""
import argparse
import os
import pathlib
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument('--sanitize', action='store_true')
args = parser.parse_args()
root = pathlib.Path(__file__).resolve().parents[2]

def block(text, marker):
    start = text.index(marker)
    opening = text.index('{', start)
    depth = 1
    end = opening + 1
    # These selected native functions contain no braces inside string literals.
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]

skip = (root / 'mm/2s2h/Enhancements/Cutscenes/StoryCutscenes/SkipGiantsChamber.cpp').read_text()
door = (root / 'mm/src/overlays/actors/ovl_Door_Warp1/z_door_warp1.c').read_text()
rando_door = (root / 'mm/2s2h/Rando/ActorBehavior/DoorWarp1.cpp').read_text()
runtime = (root / 'combo/rando/CrossDungeonMmRuntime.inc.h').read_text()
offer = block(rando_door, 'COND_VB_SHOULD(VB_GIVE_ITEM_FROM_OFFER, IS_RANDO, {')
offer = offer[offer.index('{') + 1:-1]
skip_hook = block(skip, 'COND_VB_SHOULD(VB_PLAY_TRANSITION_CS, CVAR || IS_RANDO, {')
skip_hook = skip_hook[skip_hook.index('{') + 1:-1]
prefix = r'''
#define COMBO_BUILD 1
#include "combo/ComboGameMode.h"
#include "combo/rando/CrossDungeonRuntime.h"
extern "C" { int gComboGameMode = COMBO_GAME_BOTH; }
#include <cassert>
#include <cstdarg>
#include <functional>
#include <iostream>
#include <variant>
using u8 = uint8_t; using u16 = uint16_t; using s8 = int8_t; using s32 = int32_t;
using SceneId = int; using GetItemId = int;
enum { SCENE_MITURIN_BS=31, SCENE_HAKUGIN_BS=68, SCENE_SEA_BS=95, SCENE_INISIE_BS=54 };
enum { WOODFALL_TEMPLE=40, MOUNTAIN_VILLAGE_SPRING=53, ZORA_CAPE=72, IKANA_CANYON=83,
       GIANTS_CHAMBER=90, WOODFALL=39, STONE_TOWER=86, STONE_TOWER_TEMPLE=87,
       SNOWHEAD_TEMPLE=56, GREAT_BAY_TEMPLE=71, STONE_TOWER_TEMPLE_INVERTED=88 };
#define ENTRANCE(scene, spawn) (((scene) << 9) | ((spawn) << 4))
enum { WEEKEVENTREG_CLEARED_WOODFALL_TEMPLE, WEEKEVENTREG_ENTERED_WOODFALL_TEMPLE_PRISON,
       WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE, WEEKEVENTREG_CLEARED_GREAT_BAY_TEMPLE,
       WEEKEVENTREG_CLEARED_STONE_TOWER_TEMPLE };
static bool week[5];
#define SET_WEEKEVENTREG(flag) (week[flag] = true)
#define CHECK_WEEKEVENTREG(flag) (week[flag])
enum { RC_WOODFALL_TEMPLE_BOSS_WARP, RC_SNOWHEAD_TEMPLE_BOSS_WARP,
       RC_GREAT_BAY_TEMPLE_BOSS_WARP, RC_STONE_TOWER_TEMPLE_INVERTED_BOSS_WARP,
       RC_GIANTS_CHAMBER_OATH_TO_ORDER };
using RandoCheckId = int;
struct Check { bool eligible=false, cycleObtained=false; };
static Check checks[5];
#define RANDO_SAVE_CHECKS checks
#define IS_RANDO true
struct Vec { float x=0, z=0; };
struct Actor { int id=0; struct { Vec pos; } world; };
struct PlayState { int sceneId=0; u16 nextEntrance=0; int transitionTrigger=0, transitionType=0; int setupExitList[1]{}; };
struct Player { Vec unk_3A0; };
struct DoorWarp1 { struct { Actor actor; } dyna; int unk_202=0, unk_1CA=0; void (*actionFunc)(DoorWarp1*, PlayState*)=nullptr; void* unk_1A0=nullptr; };
struct DmHina { bool isDrawn=true; };
static PlayState play; static PlayState* gPlayState=&play; static Player nativePlayer;
struct SaveContext { struct { int entrance=0, cutsceneIndex=0; struct { uint32_t unk_EA8[2]{}; } saveInfo;
    struct { int pauseSaveEntrance=-1; } shipSaveInfo; } save;
    int nextCutsceneIndex=0xFFEF, nextTransitionType=0, gameMode=0, fileNum=0, respawnFlag=0; };
static SaveContext gSaveContext;
static int gDungeonBossWarpSceneId=-1;
static int Combo_MM_CrossBossWarp(int) { return 0; }
enum { GAMEMODE_NORMAL=0, TRANS_TRIGGER_START=1, TRANS_TYPE_FADE_WHITE=2, ACTOR_DOOR_WARP1=3, PLAYER_CSACTION_9=9 };
#define GET_PLAYER(play) (&nativePlayer)
#define DOORWARP1_GET_FF00_1(actor) 0xFF
#define DOORWARP1_GET_FF00_3(actor) 0
#define DOORWARP1_GET_FF(actor) 0
#define ENDOORWARP1_FF_6 6
static void Player_SetCsActionWithHaltedActors(PlayState*, Actor*, int) {}
static void func_808B9524(DoorWarp1*, PlayState*) {}
static void Scene_SetExitFade(PlayState*) {}
static void func_80169FDC(PlayState*) {}
struct GIEventTransition { u16 entrance=0, cutsceneIndex=0; s8 transitionTrigger=0; u8 transitionType=0; };
struct GIEventGiveItem { bool showGetItemCutscene=false; std::function<void(Actor*,PlayState*)> giveItem, drawItem; };
using GIEvent = std::variant<std::monostate, GIEventTransition, GIEventGiveItem>;
struct GameInteractor { std::vector<GIEvent> events; static GameInteractor* Instance; };
static GameInteractor gi; GameInteractor* GameInteractor::Instance=&gi;
static int CVarGetInteger(const char*, int) { return 0; }
namespace CustomItem { enum { GIVE_ITEM_CUTSCENE=1 }; }
#define CUSTOM_ITEM_FLAGS 0
namespace CustomMessage { struct Args { int textboxType=0; }; static void SetActiveCustomMessage(const char*,Args) {} static void StartTextbox(const char*,Args) {} }
static void Item_Give(PlayState*,int) {} enum { ITEM_SONG_OATH, MTXMODE_APPLY, RI_SONG_OATH };
static void Matrix_Scale(float,float,float,int) {}
namespace Rando { static void DrawItem(int) {} }
using namespace ComboRando::CrossDungeon;
namespace ComboCrossMmRuntime { static RuntimeState state(1); static std::optional<RuntimeState::Transition> pending;
    static bool departureFadeComplete=false;
    static std::optional<Endpoint> pendingBossRecovery; static bool bossDeathRecoveryRequested=false;
    static CheckpointState checkpoint; static int32_t localDungeonArrival=-1; }
static int queuedTarget=-1;
static int QueuePeer(int32_t index) {
    assert(checks[RC_GIANTS_CHAMBER_OATH_TO_ORDER].eligible || checks[RC_GIANTS_CHAMBER_OATH_TO_ORDER].cycleObtained);
    assert(std::none_of(gi.events.begin(),gi.events.end(),[](const GIEvent& event){return std::holds_alternative<GIEventTransition>(event);})); queuedTarget=index; return 1;
}
static void ReturnPeer(int) {}
static void (*gComboReturnCallback)(int)=ReturnPeer;
static void* Combo_ResolveSym(const char*,const char*) { return reinterpret_cast<void*>(QueuePeer); }
#define SPDLOG_ERROR(...) ((void)0)
#define SPDLOG_INFO(...) ((void)0)
// This fixture exercises boss targets only. Grotto boundaries are independently covered by
// mm_adapter_behavior_test.py using the real registry and materializer.
static int32_t ComboMmCanonicalGrottoExit(int32_t index,int) { return index; }
static int32_t ComboMmCanonicalGrottoLoad(int32_t index) { return index; }
static int32_t ComboMmMaterializeGrotto(int32_t index) { return index; }
namespace ComboCrossMmCatalogue { static bool FindGrottoByExit(int32_t) { return false; } }
'''
functions = block(skip, 'void HandleGiantsCutsceneSkip()') + '\n' + block(skip, 'void handleGiantsCheck(SceneId sceneId)')
if 'extern "C" int Combo_MM_GiantsEscape' in skip:
    functions += '\n' + block(skip, 'extern "C" int Combo_MM_GiantsEscape')
functions += '\nvoid FirstBossOffer(bool* should, ...) { va_list args; va_start(args, should);\n' + offer + '\nva_end(args); }\n'
functions += '\nvoid NativeSkipLoadHook() {\n' + skip_hook + '\n}\n'
functions += '\n' + block(door, 'void func_808BA10C(DoorWarp1* this, PlayState* play) {').replace('this', 'self')
functions += '\nnamespace ComboCrossMmRuntime {\n' + block(runtime, 'static int32_t RecoverBoss(') + '\n}\n'
functions += '\n' + block(runtime, 'extern "C" int Combo_MM_CrossDungeonTransition(')
functions += '\nnamespace ComboCrossMmRuntime {\n' + block(runtime, 'static bool PrepareSwitch()') + '\n}\n'
functions += '\n' + block(runtime, 'extern "C" int Combo_MM_CompleteCrossDungeonTransition')
main = r'''
static void Reset() {
    play={}; gSaveContext={}; nativePlayer={}; gi.events.clear(); std::fill(std::begin(week),std::end(week),false);
    std::fill(std::begin(checks),std::end(checks),Check{}); ComboCrossMmRuntime::pending.reset();
    ComboCrossMmRuntime::pendingBossRecovery.reset(); ComboCrossMmRuntime::bossDeathRecoveryRequested=false;
    ComboCrossMmRuntime::checkpoint.Load(-1); ComboCrossMmRuntime::localDungeonArrival=-1; queuedTarget=-1;
}
static const int bossScenes[]={SCENE_MITURIN_BS,SCENE_HAKUGIN_BS,SCENE_SEA_BS,SCENE_INISIE_BS};
static const int bossEntries[]={ENTRANCE(WOODFALL_TEMPLE,0),ENTRANCE(SNOWHEAD_TEMPLE,0),ENTRANCE(GREAT_BAY_TEMPLE,0),ENTRANCE(STONE_TOWER_TEMPLE_INVERTED,0)};
static const int bossExits[]={ENTRANCE(WOODFALL,1),30000,ENTRANCE(ZORA_CAPE,7),30016};
static const int escapeTargets[]={ENTRANCE(WOODFALL,3),ENTRANCE(MOUNTAIN_VILLAGE_SPRING,7),ENTRANCE(ZORA_CAPE,9),ENTRANCE(IKANA_CANYON,15)};
static const int clearFlags[]={WEEKEVENTREG_CLEARED_WOODFALL_TEMPLE,WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE,WEEKEVENTREG_CLEARED_GREAT_BAY_TEMPLE,WEEKEVENTREG_CLEARED_STONE_TOWER_TEMPLE};
static void Install(int boss, int game, bool enabled=true, bool identity=false) {
    Dungeon source{"source",{game,2000},{game,2001},{game,identity?escapeTargets[boss]:ENTRANCE(STONE_TOWER,2)}};
    Dungeon target{"boss",{1,bossEntries[boss]},{1,bossExits[boss]},{1,bossExits[boss]},{{1,escapeTargets[boss]}}};
    ComboCrossMmRuntime::state.Install(enabled?Encode({source,target},{1,0}):nlohmann::json{});
}
static void FirstWarp(int boss) {
    play.sceneId=bossScenes[boss]; gSaveContext.save.saveInfo.unk_EA8[0]=0xAABBCCDD; gSaveContext.save.saveInfo.unk_EA8[1]=2;
    DoorWarp1 actor{}; DmHina model{}; actor.dyna.actor.id=ACTOR_DOOR_WARP1; actor.unk_1A0=&model; actor.unk_202=1;
    bool should=true; GetItemId item=0; FirstBossOffer(&should,&item,&actor.dyna.actor);
    assert(!should && !model.isDrawn && checks[boss].eligible && !checks[boss].cycleObtained);
    assert(week[clearFlags[boss]] && gi.events.size()==1);
    func_808BA10C(&actor,&play); assert(play.nextEntrance==ENTRANCE(GIANTS_CHAMBER,0));
}
static void CheckFirstBookkeeping(int boss) {
    assert(std::none_of(gi.events.begin(),gi.events.end(),[](const GIEvent& event){return std::holds_alternative<GIEventTransition>(event);}));
    assert(gi.events.size()==1 && std::holds_alternative<GIEventGiveItem>(gi.events.front()));
    assert(checks[boss].eligible && !checks[boss].cycleObtained);
    assert(checks[RC_GIANTS_CHAMBER_OATH_TO_ORDER].eligible);
    assert(gSaveContext.save.saveInfo.unk_EA8[1]==3);
    const int shift[]={0,8,24,16};
    const uint32_t mask=0xFFu<<shift[boss];
    assert(gSaveContext.save.saveInfo.unk_EA8[0]==((0xAABBCCDDu&~mask)|(2u<<shift[boss])));
}
int main() {
    for(int boss=0;boss<4;++boss) for(int game : {0,1}) {
        Reset(); Install(boss,game); FirstWarp(boss);
        gi.events.emplace_back(GIEventGiveItem{}); // Existing reward presentation must survive event consumption.
        int32_t index=play.nextEntrance;
        int foreign=Combo_MM_CrossDungeonTransition(&index);
        if(boss==0) {
            // Odolwa keeps its native prison route and bookkeeps in the original skip hook.
            assert(!foreign && index==ENTRANCE(GIANTS_CHAMBER,0) && gi.events.size()==2);
            gSaveContext.save.entrance=index; NativeSkipLoadHook();
            assert(gSaveContext.save.entrance==ENTRANCE(WOODFALL_TEMPLE,1)); CheckFirstBookkeeping(boss);
            index=escapeTargets[boss]; foreign=Combo_MM_CrossDungeonTransition(&index);
        } else CheckFirstBookkeeping(boss);
        assert(foreign==(game==0)); assert(gSaveContext.nextCutsceneIndex==0xFFEF && gSaveContext.save.cutsceneIndex==0);
        if(game==0) {
            assert(ComboCrossMmRuntime::pending); assert(!ComboCrossMmRuntime::PrepareSwitch());
            assert(Combo_MM_CompleteCrossDungeonTransition()); assert(ComboCrossMmRuntime::PrepareSwitch());
            assert(queuedTarget==ENTRANCE(STONE_TOWER,2));
            assert(ComboCrossMmRuntime::checkpoint.Saved()==bossEntries[boss]);
        } else {
            assert(index==ENTRANCE(STONE_TOWER,2));
            // This actual target has one entrance layer: a stale Gyorg/Twinmold source layer is invalid.
            int singleLayer[1]={123}; const int layer=gSaveContext.nextCutsceneIndex>=0xFFF0?(gSaveContext.nextCutsceneIndex&0xF)+1:0;
            assert(layer==0 && singleLayer[layer]==123);
        }
    }
    for(int boss=0;boss<4;++boss) for(int game : {0,1}) {
        Reset(); Install(boss,game); play.sceneId=bossScenes[boss]; week[clearFlags[boss]]=true;
        checks[boss].cycleObtained=true; checks[RC_GIANTS_CHAMBER_OATH_TO_ORDER].cycleObtained=true;
        gSaveContext.save.saveInfo.unk_EA8[0]=0xAABBCCDD; gSaveContext.save.saveInfo.unk_EA8[1]=3;
        DoorWarp1 actor{}; func_808BA10C(&actor,&play); int32_t index=play.nextEntrance;
        if(boss==0) {
            assert(index==ENTRANCE(WOODFALL_TEMPLE,1)); index=escapeTargets[boss];
        }
        assert(Combo_MM_CrossDungeonTransition(&index)==(game==0));
        assert(gSaveContext.nextCutsceneIndex==0xFFEF && gSaveContext.save.cutsceneIndex==0);
        assert(gi.events.empty() && gSaveContext.save.saveInfo.unk_EA8[0]==0xAABBCCDD && gSaveContext.save.saveInfo.unk_EA8[1]==3);
        assert(!checks[boss].eligible && !checks[RC_GIANTS_CHAMBER_OATH_TO_ORDER].eligible);
        if(game==0) {
            assert(!ComboCrossMmRuntime::PrepareSwitch());
            assert(Combo_MM_CompleteCrossDungeonTransition()); assert(ComboCrossMmRuntime::PrepareSwitch());
        } else assert(index==ENTRANCE(STONE_TOWER,2));
    }
    // Disabled first-warps remain native; unchanged-index routes retain source staging.
    for(int boss=0;boss<4;++boss) {
        Reset(); Install(boss,1,false); FirstWarp(boss); int32_t index=play.nextEntrance;
        const int staged=gSaveContext.nextCutsceneIndex;
        assert(!Combo_MM_CrossDungeonTransition(&index) && index==ENTRANCE(GIANTS_CHAMBER,0));
        assert(gSaveContext.nextCutsceneIndex==staged && gi.events.size()==1 && gSaveContext.save.saveInfo.unk_EA8[1]==2);
    }
    for(int boss=0;boss<4;++boss) {
        Reset(); Install(boss,1,false); play.sceneId=bossScenes[boss]; week[clearFlags[boss]]=true;
        DoorWarp1 actor{}; func_808BA10C(&actor,&play); int32_t index=play.nextEntrance;
        const int nativeIndex=index, staged=gSaveContext.nextCutsceneIndex;
        gSaveContext.save.cutsceneIndex=0xFFF4;
        assert(!Combo_MM_CrossDungeonTransition(&index) && index==nativeIndex);
        assert(gSaveContext.nextCutsceneIndex==staged && gSaveContext.save.cutsceneIndex==0xFFF4);
    }
    for(int boss : {1,2,3}) {
        Reset(); Install(boss,1,true,true); FirstWarp(boss);
        int32_t index=play.nextEntrance;
        assert(!Combo_MM_CrossDungeonTransition(&index) && index==escapeTargets[boss]);
        assert(gSaveContext.nextCutsceneIndex==(boss==1?0:boss==2?0xFFF0:0xFFF2));
        assert(gSaveContext.save.saveInfo.unk_EA8[1]==3 && checks[RC_GIANTS_CHAMBER_OATH_TO_ORDER].eligible);
    }
    for(int boss : {2,3}) {
        Reset(); Install(boss,1,true,true); int32_t index=escapeTargets[boss];
        gSaveContext.nextCutsceneIndex=boss==2?0xFFF0:0xFFF2; gSaveContext.save.cutsceneIndex=0xFFF4;
        assert(!Combo_MM_CrossDungeonTransition(&index) && index==escapeTargets[boss]);
        assert(gSaveContext.nextCutsceneIndex==(boss==2?0xFFF0:0xFFF2) && gSaveContext.save.cutsceneIndex==0xFFF4);
    }
    std::cout<<"MM production boss warps: first/repeat all four, local/cross, bookkeeping and layer safety pass\n";
}
'''
with tempfile.TemporaryDirectory(prefix='mm-boss-warp-') as temp:
    source = pathlib.Path(temp) / 'behavior.cpp'
    binary = pathlib.Path(temp) / 'behavior'
    source.write_text(prefix + functions + main)
    cmd = ['g++','-std=c++20','-Wno-unused-function','-I',str(root),'-I',str(root.parent/'build-deps/usr/include'),str(source),'-o',str(binary)]
    if args.sanitize:
        cmd += ['-fsanitize=address,undefined','-fno-omit-frame-pointer','-g']
    subprocess.run(cmd,check=True)
    env=os.environ.copy()
    if args.sanitize:
        env['ASAN_OPTIONS']='detect_leaks=0'
    subprocess.run([str(binary)],check=True,env=env)
