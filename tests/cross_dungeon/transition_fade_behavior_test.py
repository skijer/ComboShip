#!/usr/bin/env python3
"""Exercise native cross-entrance transition boundaries without rendering or game assets.

Real route/PrepareSwitch bodies own pending state and checkpoints. Native transition
cases and fade arithmetic execute; scene replacement is counted at SET_NEXT_GAMESTATE.
The fixture reads fade alpha rather than sending display lists to a GPU. Registered
handoff hooks use counters for disk I/O and DLL callbacks. The native camera, Link
animation, and final peer presentation require game testing.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_time_pedestal_tests import block_from

parser = argparse.ArgumentParser()
parser.add_argument('--sanitize', action='store_true')
parser.add_argument('--game', choices=['oot', 'mm', 'oot_handoff', 'mm_handoff'])
args = parser.parse_args()
if args.sanitize:
    os.environ['ASAN_OPTIONS'] = 'detect_leaks=0'


def fixture(name, variable):
    path = ROOT / 'tests/cross_dungeon' / name
    env = {'__file__': str(path)}
    previous = sys.argv
    try:
        sys.argv = [str(path)]
        prefix = path.read_text().split('with tempfile.TemporaryDirectory(')[0]
        exec(compile(prefix, str(path), 'exec'), env)
    finally:
        sys.argv = previous
    source = env[variable]
    return source[:source.index('int main()')]


def function(text, marker):
    return block_from(text, text.index(marker))


def fade_functions(game):
    text = (ROOT / game / 'src/code/z_fbdemo_fade.c').read_text()
    names = ['Start', 'Init', 'Destroy', 'Update', 'IsDone', 'SetColor', 'SetType']
    return '\n'.join(re.sub(r'\bthis\b', 'self', function(text, ('void* ' if name == 'Init' else
                                                             's32 ' if name == 'IsDone' else 'void ') +
                                                            'TransitionFade_' + name + '('))
                     for name in names)


def destroy_beginning(text):
    destroy = function(text, 'void Play_Destroy(GameState* thisx)')
    start = destroy.index('#ifdef COMBO_BUILD')
    end = destroy.index('#endif', start) + len('#endif')
    return '\nvoid NativeDestroyBeginning() {\n' + destroy[start:end] + '\n}\n'


def non_instance(text, pointer):
    start = text.index('case TRANS_MODE_FILL_WHITE_INIT:', text.index('// update non-instance transitions'))
    end = text.index('case TRANS_MODE_INSTANCE_WAIT:', start)
    cases = re.sub(r'\bthis\b', 'self', text[start:end])
    start = text.index('case TRANS_MODE_SANDSTORM:', end)
    end = text.index('case TRANS_MODE_SANDSTORM_END_INIT:', start)
    cases += re.sub(r'\bthis\b', 'self', text[start:end])
    return '\nvoid NativeNonInstance(PlayState* ' + pointer + ') {\n switch(' + pointer + '->transitionMode) {\n' + cases + '\n default: break;\n }\n}\n'


oot_play = (ROOT / 'soh/src/code/z_play.c').read_text()
oot_cases = oot_play[oot_play.index('                case TRANS_MODE_SETUP:'):
                      oot_play.index('                case TRANS_MODE_FILL_WHITE_INIT:')]
oot_native = r'''
#undef osSyncPrintf
#define osSyncPrintf(...) ((void)0)
#define VT_COL(...) ""
#define VT_RST ""
#define Audio_PlaySfxGeneral(...) ((void)0)
#undef SET_NEXT_GAMESTATE
#define SET_NEXT_GAMESTATE(...) (++nativeLoads)
static int nativeLoads=0, transitionEnds=0;
s16 sTransitionFillTimer=0;
static GameInfo fixtureGameInfo{};
GameInfo* gGameInfo=&fixtureGameInfo;
s32 gTrnsnUnkState=0;
TransitionUnk sTrnsnUnk{};
s32 Math_StepToS(s16* value,s16 target,s16 step) {
    if (*value<target) *value=std::min<int>(*value+step,target);
    else *value=std::max<int>(*value-step,target);
    return *value==target;
}
void Interface_ChangeHudVisibilityMode(u16) {}
s32 Environment_IsForcedSequenceDisabled() { return 0; }
void func_800F6964(u16) {}
void TransitionUnk_Destroy(TransitionUnk*) {}
void GameInteractor_ExecuteOnTransitionEndHooks(int16_t) { ++transitionEnds; }
''' + fade_functions('soh') + '\n' + function(oot_play, 'void func_800BC88C(') + r'''
void Gameplay_SetupTransition(PlayState* play,s32 type) {
    auto& ctx=play->transitionCtx;
    ctx={};ctx.transitionType=type;
    ctx.init=TransitionFade_Init;ctx.destroy=TransitionFade_Destroy;
    ctx.start=TransitionFade_Start;ctx.isDone=TransitionFade_IsDone;
    ctx.update=TransitionFade_Update;ctx.setType=TransitionFade_SetType;
    ctx.setColor=TransitionFade_SetColor;ctx.setEnvColor=nullptr;
}
void NativeStep(PlayState* play) {
    switch(play->transitionMode) {
''' + oot_cases + r'''
    default: break;
    }
}
static TransitionFade& NativeFade(PlayState& play) {
    return *reinterpret_cast<TransitionFade*>(&play.transitionCtx.data);
}
''' + destroy_beginning(oot_play) + non_instance(oot_play, 'play')

oot = fixture('oot_pool_runtime_behavior_test.py', 'source')
oot = oot.replace('void* Combo_ResolveSym(const char*,const char*) { return nullptr; }', r'''
static int queued=-1, queues=0;
static int QueuePeer(int entrance) { queued=entrance;++queues;return 1; }
void* Combo_ResolveSym(const char*,const char*) { return reinterpret_cast<void*>(QueuePeer); }
''')
oot += oot_native + r'''
int main() {
    PlayState play{};gPlayState=&play;gSaveContext.fileNum=1;
    R_UPDATE_RATE=3;
    Dungeon source{"source",{0,10},{0,11},{0,12}};
    Dungeon peer{"peer",{1,20},{1,21},{1,22}};
    auto seed=Encode({source,peer},{1,0});
    assert(SOH_SetCrossDungeonRuntimeSeed(seed.dump().c_str()));
    gSaveContext.entranceIndex=777;
    play.state.running=true;play.nextEntranceIndex=10;
    play.transitionTrigger=TRANS_TRIGGER_START;play.transitionMode=TRANS_MODE_SETUP;
    play.transitionType=TRANS_TYPE_FADE_BLACK;
    assert(Combo_SOH_CrossDungeonRoute(source.entrance.index,false)==source.entrance.index);
    assert(!PrepareSwitch() && "OoT must not prepare a foreign handoff before the native fade reaches black");
    assert(queues==0 && queued==-1 && gSaveContext.entranceIndex==777);
    assert(play.transitionTrigger==TRANS_TRIGGER_START && play.transitionMode==TRANS_MODE_SETUP);
    NativeStep(&play);
    assert(play.transitionType==TRANS_TYPE_FADE_BLACK && play.transitionMode==TRANS_MODE_INSTANCE_RUNNING);
    for(int frame=0;frame<30 && !TransitionFade_IsDone(&play.transitionCtx.data);++frame) {
        assert(!PrepareSwitch() && queues==0 && nativeLoads==0 && gSaveContext.entranceIndex==777);
        NativeStep(&play);
    }
    assert(TransitionFade_IsDone(&play.transitionCtx.data) && NativeFade(play).fadeColor.a==255);
    assert(!PrepareSwitch() && "black must be acknowledged by the native completion branch");
    NativeStep(&play);
    assert(nativeLoads==0 && play.state.running && play.transitionTrigger==TRANS_TRIGGER_START);
    assert(play.transitionMode==TRANS_MODE_INSTANCE_RUNNING && gSaveContext.entranceIndex==777);
    assert(PrepareSwitch() && queues==1 && queued==20 && gSaveContext.entranceIndex==12);
    assert(play.transitionTrigger==TRANS_TRIGGER_OFF && play.transitionMode==TRANS_MODE_OFF);
    assert(!PrepareSwitch() && queues==1);

    // Native entrance effects remain authoritative for foreign routes too.
    Combo_SOH_CrossDungeonRoute(10,false);
    play.transitionMode=TRANS_MODE_SETUP;play.transitionTrigger=TRANS_TRIGGER_START;
    play.transitionType=TRANS_TYPE_FADE_WHITE;
    NativeStep(&play);assert(play.transitionType==TRANS_TYPE_FADE_WHITE);
    NativeDestroyBeginning();assert(!Combo_SOH_CrossDungeonTransitionPending() && !PrepareSwitch());
    Combo_SOH_CrossDungeonRoute(10,false);assert(!PrepareSwitch());
    Combo_SOH_CompleteCrossDungeonTransition();
    NativeDestroyBeginning();assert(!PrepareSwitch() && queues==1);
    Combo_SOH_CrossDungeonRoute(10,false);assert(!PrepareSwitch());
    Combo_SOH_CompleteCrossDungeonTransition();
    assert(SOH_SetCrossDungeonRuntimeSeed(seed.dump().c_str()));
    assert(!Combo_SOH_CrossDungeonTransitionPending() && !PrepareSwitch());
    Combo_SOH_CrossDungeonRoute(10,false);Combo_SOH_CompleteCrossDungeonTransition();
    Combo_SOH_CrossDungeonRoute(10,false);assert(!PrepareSwitch());
    Combo_SOH_CancelCrossDungeonTransition();

    // A completed wipe uses the same native completion branch without replacing Play.
    Combo_SOH_CrossDungeonRoute(10,false);play.state.running=true;
    play.transitionMode=TRANS_MODE_INSTANCE_RUNNING;play.transitionTrigger=TRANS_TRIGGER_START;
    play.transitionCtx.transitionType=TRANS_TYPE_WIPE;NativeFade(play).isDone=true;
    NativeStep(&play);assert(nativeLoads==0 && play.state.running);
    assert(PrepareSwitch() && queues==2 && !PrepareSwitch());

    // Native non-instance white fill completes without a GPU callback or user input.
    Combo_SOH_CrossDungeonRoute(10,false);play.state.running=true;
    play.transitionTrigger=TRANS_TRIGGER_START;play.transitionMode=TRANS_MODE_FILL_WHITE_INIT;
    NativeNonInstance(&play);
    for(int frame=0;frame<20;++frame) {
        assert(!PrepareSwitch());NativeNonInstance(&play);
    }
    assert(!PrepareSwitch());NativeNonInstance(&play);
    assert(play.envCtx.screenFillColor[3]==255 && nativeLoads==0 && play.state.running);
    assert(PrepareSwitch() && queues==3 && !PrepareSwitch());

    Combo_SOH_CrossDungeonRoute(10,false);play.state.running=true;
    play.transitionTrigger=TRANS_TRIGGER_START;play.transitionMode=TRANS_MODE_INSTANT;
    NativeNonInstance(&play);assert(nativeLoads==0 && play.state.running);
    assert(PrepareSwitch() && queues==4 && !PrepareSwitch());

    Combo_SOH_CrossDungeonRoute(10,false);play.state.running=true;
    play.transitionTrigger=TRANS_TRIGGER_START;play.transitionMode=TRANS_MODE_SANDSTORM;
    play.envCtx.sandstormEnvA=254;NativeNonInstance(&play);
    assert(!PrepareSwitch() && nativeLoads==0);
    play.envCtx.sandstormEnvA=255;NativeNonInstance(&play);
    assert(nativeLoads==0 && play.state.running && PrepareSwitch() && queues==5 && !PrepareSwitch());

    // A local route retains its native transition style and loads the native destination once.
    assert(SOH_SetCrossDungeonRuntimeSeed("null"));
    play.state.running=true;play.nextEntranceIndex=99;
    play.transitionTrigger=TRANS_TRIGGER_START;play.transitionMode=TRANS_MODE_SETUP;
    play.transitionType=TRANS_TYPE_FADE_WHITE;
    NativeStep(&play);
    assert(play.transitionType==TRANS_TYPE_FADE_WHITE);
    for(int frame=0;frame<31 && nativeLoads==0;++frame) NativeStep(&play);
    assert(nativeLoads==1 && !play.state.running && gSaveContext.entranceIndex==99 && queues==5);
    std::cout<<"OoT: native fade/wipe/fill/instant/sandstorm completion, one handoff, native local load and cancellation PASS\n";
}
'''

mm_transition = (ROOT / 'mm/include/z64transition.h').read_text()
mm_color = (ROOT / 'mm/include/color.h').read_text()
mm_play = (ROOT / 'mm/src/code/z_play.c').read_text()
mm_fade = (ROOT / 'mm/src/code/z_fbdemo_fade.c').read_text()
mm_types = r'''
#include <cstdint>
#include <cstring>
using u8=uint8_t;using u16=uint16_t;using u32=uint32_t;using s16=int16_t;using s32=int32_t;
''' + function(mm_color, 'typedef union Color_RGBA8_u32 {') + ' Color_RGBA8_u32;\n'
mm_types += function(mm_transition, 'typedef struct TransitionFade {') + ' TransitionFade;\n'
mm_types += mm_transition[mm_transition.index('#define TRANS_TRIGGER_OFF'):
                          mm_transition.index('#define DEFINE_TRANSITION(enumValue,')]
mm_types += mm_transition[mm_transition.index('typedef enum {\n    /* 1 */ TRANS_INSTANCE_TYPE_FILL_OUT'):
                          mm_transition.index('typedef struct TransitionFade {')]
mm_types += mm_fade[mm_fade.index('typedef enum TransitionFadeDirection'):
                    mm_fade.index('static Gfx sTransFadeSetupDL')]
mm_types += r'''
#define RGBA8(r,g,b,a) ((uint32_t(r)<<24)|(uint32_t(g)<<16)|(uint32_t(b)<<8)|uint32_t(a))
struct TransitionContext {
    TransitionFade instanceData{};int transitionType=0;
    void* (*init)(void*)=nullptr;void (*destroy)(void*)=nullptr;
    void (*start)(void*)=nullptr;int (*isDone)(void*)=nullptr;
    void (*update)(void*,s32)=nullptr;void (*setType)(void*,s32)=nullptr;
    void (*setColor)(void*,u32)=nullptr;void (*setEnvColor)(void*,u32)=nullptr;
};
'''
mm = fixture('mm_adapter_behavior_test.py', 'program')
mm = mm.replace(function(mm, 'void CheckNativeBossCatalogue()'), '')
mm = re.sub(r'constexpr int TRANS_TYPE_FADE_BLACK\s*=\s*\d+;', '', mm)
mm = mm_types + mm
mm = mm.replace('int gameMode=0,', 'int minigameStatus=0,seqId=0,ambienceId=0,transWipeSpeed=0,transFadeDuration=0;int gameMode=0,')
mm = mm.replace('struct Play { int sceneId=0;', r'''struct Play { int sceneId=0,nextEntrance=0;
    int transitionTrigger=0,transitionMode=0,transitionType=0;
    struct { bool running=true;int framerateDivisor=3; } state;
    struct { bool fillScreen=false;u8 screenFillColor[4]{};u8 sandstormPrimA=0,sandstormEnvA=0; } envCtx;
    TransitionContext transitionCtx;''')
mm = mm.replace('int queued=-1;', 'int queued=-1,queues=0;')
mm = mm.replace('int QueuePeer(int index){queued=index;return 1;}',
                'int QueuePeer(int index){queued=index;++queues;return 1;}')
mm_cases = mm_play[mm_play.index('        case TRANS_MODE_SETUP: {'):
                   mm_play.index('    // update non-instance transitions')]
mm_cases = mm_cases[:mm_cases.rfind('    }')]
mm_native = r'''
using PlayState=Play;
static int nativeLoads=0;
static bool D_801D0D54=false;
static int gTransitionTileState=0,sTransitionTile=0;
static s16 sTransitionFillTimer=0;
static s16 fadeFlashStep=0;
#define R_TRANS_FADE_FLASH_ALPHA_STEP fadeFlashStep
#define STOP_GAMESTATE(state) ((state)->running=false)
#define SET_NEXT_GAMESTATE(...) (++nativeLoads)
#define LUSLOG_ERROR(...) ((void)0)
#define Audio_PlaySfx_2(...) ((void)0)
constexpr int GAMEMODE_OWL_SAVE=2,GAMEMODE_FILE_SELECT=3;
constexpr int TRANS_TILE_READY=3,TRANS_TILE_OFF=0,HUD_VISIBILITY_NONE=0;
constexpr int MINIGAME_STATUS_ACTIVE=1,MINIGAME_STATUS_END=2;
constexpr int SEQ_PLAYER_BGM_MAIN=0,NA_BGM_FINAL_HOURS=1,NA_BGM_DISABLED=2,AMBIENCE_ID_DISABLED=0;
constexpr int WEEKEVENTREG_CLEARED_GREAT_BAY_TEMPLE=3,WEEKEVENTREG_CLEARED_STONE_TOWER_TEMPLE=4;
struct SceneEntranceTableEntry { void* table;int tableCount; };
s32 gSceneEntranceTableCount=128;
SceneEntranceTableEntry sSceneEntranceTable[128];
void Interface_SetHudVisibility(int) {}
int Entrance_GetTransitionFlags(int) { return 0x8000; }
bool Environment_IsFinalHours(PlayState*) { return false; }
int AudioSeq_GetActiveSeqId(int) { return 0; }
void Audio_MuteAllSeqExceptSystemAndOcarina(int) {}
bool Environment_IsForcedSequenceDisabled() { return false; }
void Audio_MuteSfxAndAmbienceSeqExceptSysAndOca(int) {}
bool GameInteractor_Should(int,bool value) { return value; }
constexpr int VB_SETUP_TRANSITION=0;
void Combo_RequestOwlSaveQuit() { assert(false); }
void Play_ClearTransition(PlayState*) {}
void TransitionTile_Destroy(int*) {}
template<class State> void GameState_SetFramerateDivisor(State*,int) {}
s32 Math_StepToS(s16* value,s16 target,s16 step) {
    if (*value<target) *value=std::min<int>(*value+step,target);
    else *value=std::max<int>(*value-step,target);
    return *value==target;
}
''' + fade_functions('mm') + r'''
int Play_ChooseDynamicTransition(PlayState*,int type) { return type; }
void Play_SetupTransition(PlayState* play,int type) {
    auto& ctx=play->transitionCtx;ctx={};ctx.transitionType=type;
    ctx.init=TransitionFade_Init;ctx.destroy=TransitionFade_Destroy;
    ctx.start=TransitionFade_Start;ctx.isDone=TransitionFade_IsDone;
    ctx.update=TransitionFade_Update;ctx.setType=TransitionFade_SetType;
    ctx.setColor=TransitionFade_SetColor;ctx.setEnvColor=nullptr;
}
void NativeStep(PlayState* self) {
    switch(self->transitionMode) {
''' + re.sub(r'\bthis\b', 'self', mm_cases) + r'''
    default: break;
    }
}
''' + destroy_beginning(mm_play) + non_instance(mm_play, 'self')
mm += mm_native + r'''
int main() {
    using namespace ComboCrossMmRuntime;
    InitRegions();
    auto seed=ForeignGrottoSeed(18);
    assert(MM_SetCrossDungeonRuntimeSeed(seed.dump().c_str()));
    play.sceneId=1000;gSaveContext.respawn[3].data=0x9A;
    gSaveContext.save.entrance=777;gSaveContext.save.shipSaveInfo.pauseSaveEntrance=778;
    for(auto& entry:sSceneEntranceTable) entry={&entry,32};
    play.nextEntrance=ENTRANCE(GROTTOS,4);play.state.running=true;
    play.transitionTrigger=TRANS_TRIGGER_START;play.transitionMode=TRANS_MODE_SETUP;
    play.transitionType=TRANS_TYPE_FADE_BLACK;
    NativeStep(&play);
    assert(Combo_MM_CrossDungeonTransitionPending());
    assert(!PrepareSwitch() && "MM must not prepare a foreign handoff before the native fade reaches black");
    assert(queues==0 && queued==-1 && gSaveContext.save.entrance==777);
    assert(gSaveContext.save.shipSaveInfo.pauseSaveEntrance==778);
    assert(play.transitionType==TRANS_TYPE_FADE_BLACK && play.transitionMode==TRANS_MODE_INSTANCE_RUNNING);
    for(int frame=0;frame<30 && !TransitionFade_IsDone(&play.transitionCtx.instanceData);++frame) {
        assert(!PrepareSwitch() && queues==0 && nativeLoads==0 && gSaveContext.save.entrance==777);
        NativeStep(&play);
    }
    assert(TransitionFade_IsDone(&play.transitionCtx.instanceData) && play.transitionCtx.instanceData.color.a==255);
    assert(!PrepareSwitch());
    NativeStep(&play);
    assert(nativeLoads==0 && play.state.running && play.transitionTrigger==TRANS_TRIGGER_START);
    assert(play.transitionMode==TRANS_MODE_INSTANCE_RUNNING && gSaveContext.save.entrance==777);
    assert(PrepareSwitch() && queues==1 && queued==100);
    assert(gSaveContext.save.entrance!=777 && gSaveContext.save.shipSaveInfo.pauseSaveEntrance!=778);
    assert(!PrepareSwitch() && queues==1);

    auto route=[&]() { gSaveContext.respawnFlag=0;gSaveContext.respawn[3].data=0x9A;
        int32_t index=ENTRANCE(GROTTOS,4);assert(Combo_MM_CrossDungeonTransition(&index)); };
    route();play.transitionMode=TRANS_MODE_SETUP;play.transitionTrigger=TRANS_TRIGGER_START;
    play.transitionType=TRANS_TYPE_FADE_WHITE;NativeStep(&play);
    assert(play.transitionType==TRANS_TYPE_FADE_WHITE);
    NativeDestroyBeginning();assert(!Combo_MM_CrossDungeonTransitionPending() && !PrepareSwitch());
    route();assert(!PrepareSwitch());Combo_MM_CompleteCrossDungeonTransition();
    NativeDestroyBeginning();assert(!PrepareSwitch() && queues==1);
    route();assert(!PrepareSwitch());Combo_MM_CompleteCrossDungeonTransition();
    assert(MM_SetCrossDungeonRuntimeSeed(seed.dump().c_str()));
    assert(!Combo_MM_CrossDungeonTransitionPending() && !PrepareSwitch());
    route();Combo_MM_CompleteCrossDungeonTransition();route();assert(!PrepareSwitch());
    Combo_MM_CancelCrossDungeonTransition();

    route();play.state.running=true;play.transitionMode=TRANS_MODE_INSTANCE_RUNNING;
    play.transitionTrigger=TRANS_TRIGGER_START;play.transitionCtx.transitionType=TRANS_TYPE_WIPE;
    play.transitionCtx.instanceData.isDone=true;
    NativeStep(&play);assert(nativeLoads==0 && play.state.running);
    assert(PrepareSwitch() && queues==2 && !PrepareSwitch());

    route();play.state.running=true;play.transitionTrigger=TRANS_TRIGGER_START;
    play.transitionMode=TRANS_MODE_FILL_WHITE_INIT;NativeNonInstance(&play);
    for(int frame=0;frame<20;++frame) { assert(!PrepareSwitch());NativeNonInstance(&play); }
    assert(!PrepareSwitch());NativeNonInstance(&play);
    assert(play.envCtx.screenFillColor[3]==255 && nativeLoads==0 && play.state.running);
    assert(PrepareSwitch() && queues==3 && !PrepareSwitch());
    route();play.state.running=true;play.transitionTrigger=TRANS_TRIGGER_START;
    play.transitionMode=TRANS_MODE_INSTANT;NativeNonInstance(&play);
    assert(nativeLoads==0 && play.state.running);
    assert(PrepareSwitch() && queues==4 && !PrepareSwitch());
    route();play.state.running=true;play.transitionTrigger=TRANS_TRIGGER_START;
    play.transitionMode=TRANS_MODE_SANDSTORM;play.envCtx.sandstormEnvA=254;
    NativeNonInstance(&play);assert(!PrepareSwitch() && nativeLoads==0);
    play.envCtx.sandstormEnvA=255;NativeNonInstance(&play);
    assert(nativeLoads==0 && play.state.running && PrepareSwitch() && queues==5 && !PrepareSwitch());
    assert(MM_SetCrossDungeonRuntimeSeed("null"));
    play.state.running=true;play.nextEntrance=ENTRANCE(SOUTH_CLOCK_TOWN,0);
    play.transitionTrigger=TRANS_TRIGGER_START;play.transitionMode=TRANS_MODE_SETUP;
    play.transitionType=TRANS_TYPE_FADE_WHITE;
    NativeStep(&play);assert(play.transitionType==TRANS_TYPE_FADE_WHITE);
    for(int frame=0;frame<31 && nativeLoads==0;++frame) NativeStep(&play);
    assert(nativeLoads==1 && !play.state.running && queues==5);
    std::cout<<"MM: native fade/wipe/fill/instant/sandstorm completion, one handoff, native local load and cancellation PASS\n";
}
'''


def handoff_fixture(game):
    """Retain registered production handoff bodies; stub only dispatch, DLLs and disk IO."""
    is_oot = game == 'oot'
    ns = 'ComboCrossOotRuntime' if is_oot else 'ComboCrossMmRuntime'
    prefix = 'Combo_SOH' if is_oot else 'Combo_MM'
    name = 'oot_handoff_test.cpp' if is_oot else 'mm_handoff_test.cpp'
    text = (ROOT / 'tests/lost_woods_entrance' / name).read_text()
    text = text[:text.index('int main()')]
    if not is_oot and 'struct OnGameStateMainFinish ' not in text:
        text = text.replace('    struct OnPlayDestroy ',
                            '    struct OnGameStateMainFinish { using Function = std::function<void()>; };\n    struct OnPlayDestroy ')
    text = text.replace(function(text, 'namespace ' + ns + ' {'), '')
    for marker in ['extern "C" int ' + prefix + '_CrossDungeonTransitionPending(',
                   'extern "C" void ' + prefix + '_CancelCrossDungeonTransition(']:
        text = text.replace(function(text, marker), '')
    text = text.replace(function(text, 'static void* Combo_ResolveSym('), r'''
static int queues=0,queued=-1;
static int QueuePeer(int index) { ++queues;queued=index;return 1; }
static void* Combo_ResolveSym(const char*,const char* name) {
    if (std::string(name).find("QueueCrossDungeonArrival")!=std::string::npos)
        return reinterpret_cast<void*>(QueuePeer);
    return reinterpret_cast<void*>(SET_ROUTE);
}
'''.replace('SET_ROUTE', 'SetMmRoute' if is_oot else 'SetOotRoute'))
    runtime = (ROOT / 'combo/rando' / ('CrossDungeonOotRuntime.inc.h' if is_oot else
                                      'CrossDungeonMmRuntime.inc.h')).read_text()
    body = r'''
#include "ComboGameMode.h"
extern "C" { int gComboGameMode = COMBO_GAME_BOTH; }
#include "rando/CrossDungeonRuntime.h"
#define SPDLOG_ERROR(...) ((void)0)
#define SPDLOG_INFO(...) ((void)0)
using namespace ComboRando::CrossDungeon;
'''
    if is_oot:
        body += 'EntranceInfo gEntranceTable[ENTR_MAX]{};\n'
    else:
        body += r'''
static int32_t ComboMmMaterializeGrotto(int32_t index) { return index; }
static int32_t ComboMmCanonicalGrottoLoad(int32_t index) { return index; }
static int32_t ComboMmCanonicalGrottoExit(int32_t index,int) { return index; }
namespace ComboCrossMmCatalogue { static bool FindGrottoByExit(int32_t) { return false; } }
extern "C" int Combo_MM_GiantsEscape(int32_t*,int32_t*,int) { return 0; }
'''
    body += 'namespace ' + ns + ' {\n' + r'''
static RuntimeState state(GAME);
static std::optional<RuntimeState::Transition> pending;
static std::optional<Endpoint> pendingBossRecovery;
static bool departureFadeComplete=false,bossDeathRecoveryRequested=false;
static CheckpointState checkpoint;
static int32_t localDungeonArrival=-1,localArrivalCheckpoint=-1;
static int32_t Materialize(int32_t index) { return index; }
static void ObserveScene(int) {}
'''.replace('GAME', '0' if is_oot else '1')
    if not is_oot:
        body += function(runtime, 'static int32_t RecoverBoss(') + '\n'
    body += function(runtime, 'static bool PrepareSwitch()') + '\n}\n'
    for marker in [prefix + '_CrossDungeonTransitionPending(', prefix + '_CompleteCrossDungeonTransition(',
                   prefix + '_CancelCrossDungeonTransition(']:
        declaration = ('extern "C" void ' if 'Cancel' in marker else 'extern "C" int ') + marker
        body += function(runtime, declaration) + '\n'
    body += function(runtime, 'extern "C" int32_t Combo_SOH_CrossDungeonRoute(' if is_oot else
                              'extern "C" int Combo_MM_CrossDungeonTransition(') + '\n'
    body += destroy_beginning(oot_play if is_oot else mm_play)
    insertion = 'void RegisterCandidate()' if is_oot else '/* RETURN_STATE */'
    text = text.replace(insertion, body + '\n' + insertion)
    if is_oot:
        host = (ROOT / 'soh/soh/OTRGlobals.cpp').read_text(encoding='utf-8-sig')
        text = text.replace('/* MIRROR_QUEUE */', re.search(
            r'static int sComboMirrorSwitchSlot = [^;]+;', host)[0])
        begin = host.index('    // Flag set when we want to switch to MM.')
        end = host.index('    // ComboShip release gate:', begin)
        text = text.replace('/* HANDOFF_HOOKS */', host[begin:end])
        source_game = (ROOT / 'soh/src/code/game.c').read_text()
        frame_calls = ['gameState->main(gameState);', 'GameInteractor_ExecuteOnGameFrameUpdate();']
        frame_order = '\n'.join(sorted(frame_calls, key=source_game.index))
        fill_draw = function(oot_play, 'switch (play->envCtx.fillScreen)')
        text += r'''
static int lastRenderedFillAlpha=0;
void Environment_FillScreen(GraphicsContext*,u8,u8,u8,u8 alpha,u8) { lastRenderedFillAlpha=alpha; }
static void NativeFrameBody(GameState*) {
    PlayState* play=gPlayState;GraphicsContext* gfxCtx=nullptr;lastRenderedFillAlpha=0;
FILL_DRAW
}
extern "C" void GameInteractor_ExecuteOnGameFrameUpdate() {
    GameInteractor::Fire<GameInteractor::OnGameFrameUpdate>();
}
static void NativeFrame() {
    GameState* gameState=&play.state;
FRAME_ORDER
}
'''.replace('FILL_DRAW', fill_draw).replace('FRAME_ORDER', frame_order)
    else:
        host = (ROOT / 'mm/2s2h/BenPort.cpp').read_text()
        begin = host.index('    // Reverse MM->OOT trigger:')
        end = host.index('    } // Combo-only handoff hooks:', begin)
        state_begin = host.index('static bool sComboReturnPending = false;')
        state_end = host.index('// MM\'s own ResourceManager', state_begin)
        text = text.replace('/* RETURN_STATE */', host[state_begin:state_end])
        text = text.replace('/* HANDOFF_HOOKS */', host[begin:end])
    if not is_oot:
        native_game = (ROOT / 'mm/src/code/game.c').read_text()
        begin = native_game.index('    GameInteractor_ExecuteOnGameStateMainStart();')
        end = native_game.index('    GameInteractor_ExecuteOnGameStateMainFinish();', begin) + len(
            '    GameInteractor_ExecuteOnGameStateMainFinish();')
        frame_order = native_game[begin:end]
        begin = mm_play.index('            if (((this->transitionMode == TRANS_MODE_INSTANCE_RUNNING)')
        end = mm_play.index('{', begin)
        draw_condition = re.sub(r'\bthis\b', 'self', mm_play[begin:end])
        text += r'''
static bool expectCovered=false,nativeEffectDrawn=false;
static int lastRenderedFillAlpha=0;
static bool D_801D0D54=false;
static void CaptureNativeEffect(void*,Gfx**) { nativeEffectDrawn=true; }
static void NativeFrameBody(GameState*) {
    PlayState* self=&play;Gfx* sp218=nullptr;nativeEffectDrawn=false;
    lastRenderedFillAlpha=self->envCtx.fillScreen?self->envCtx.screenFillColor[3]:0;
DRAW_CONDITION {
    self->transitionCtx.draw(&self->transitionCtx.instanceData,&sp218);
    }
    require(!expectCovered || nativeEffectDrawn,"final MM source draw must retain its native transition effect");
}
extern "C" void GameInteractor_ExecuteOnGameStateMainStart() {
    GameInteractor::Fire<GameInteractor::OnGameStateMainStart>();
}
extern "C" void GameInteractor_ExecuteOnGameStateMainFinish() {
    GameInteractor::Fire<GameInteractor::OnGameStateMainFinish>();
}
static void NativeFrame() {
    GameState* gameState=&play.state;
FRAME_ORDER
}
'''.replace('DRAW_CONDITION', draw_condition).replace('FRAME_ORDER', frame_order)
    text += r'''
int main() {
    RegisterCandidate();gPlayState=&play;gGameState=&play.state;
    gSaveContext.fileNum=0;gSaveContext.gameMode=GAMEMODE_NORMAL;play.state.running=true;
    Dungeon oot{"oot",{0,10},{0,11},{0,12}},mm{"mm",{1,20},{1,21},{1,22}};
    NS::state.Install(Encode({oot,mm},{1,0}));
    auto route=[]() { ROUTE;play.state.running=true;
        play.transitionTrigger=TRANS_TRIGGER_START;play.transitionMode=TRANS_MODE_INSTANCE_RUNNING; };
    auto frame=[]() { GameInteractor::Fire<GameInteractor::FRAME>(); };
    route();
    for(int i=0;i<8;++i) frame();
    require(queues==0 && saves==0 && switches==0,"native departure must finish before queue/save/switch");
    require(play.state.running && play.transitionTrigger==TRANS_TRIGGER_START,"pending native animation remains live");
    EXPECT_COVERED
    PREFIX_CompleteCrossDungeonTransition();frame();
    END_EXPECT_COVERED
    require(queues==1 && saves==1 && switches==1,"one native completion must save and hand off once");
    require(queued==TARGET && play.transitionMode==TRANS_MODE_OFF && play.transitionTrigger==TRANS_TRIGGER_OFF,
            "accepted handoff owns peer destination and clears source transition modes");
    for(int i=0;i<4;++i) frame();
    require(queues==1 && saves==1 && switches==1,"the next frame must not repeat handoff");
    route();PREFIX_CompleteCrossDungeonTransition();NativeDestroyBeginning();
    GameInteractor::Fire<GameInteractor::OnPlayDestroy>();frame();
    require(queues==1 && saves==1 && switches==1,"Play destruction discards a ready departure");
    route();PREFIX_CompleteCrossDungeonTransition();RESET;frame();
    require(queues==1 && saves==1 && switches==RESET_SWITCHES,"reset supersedes the ready ordinary departure");
    FOREST_CHECK
    std::puts("GAME: production handoff hooks gate queue/save/switch and honor Play destruction/reset PASS");
}
'''.replace('NS::', ns + '::').replace('ROUTE', 'Combo_SOH_CrossDungeonRoute(10,false)' if is_oot else
                            'int32_t index=20;assert(Combo_MM_CrossDungeonTransition(&index))').replace(
                                'FRAME', 'OnGameFrameUpdate' if is_oot else 'OnGameStateMainStart').replace(
                                    'PREFIX', prefix).replace('TARGET', '20' if is_oot else '10').replace(
                                        'RESET_SWITCHES', '1' if is_oot else '2').replace(
                                            'RESET', 'GameInteractor::Fire<GameInteractor::OnExitGame>(0)' if is_oot
                                            else 'MM_RequestComboReturn()').replace('GAME:', game + ':').replace(
                                                'END_EXPECT_COVERED', '' if is_oot else 'expectCovered=false;').replace(
                                                    'EXPECT_COVERED', '' if is_oot else 'expectCovered=true;')
    text = text.replace('FOREST_CHECK', r'''
    play.transitionMode=TRANS_MODE_OFF;play.transitionTrigger=TRANS_TRIGGER_OFF;
    EnterTunnel(0);play.state.main=NativeFrameBody;
    for(int i=0;i<20 && switches==1;++i) frame();
    require(switches==2 && saves==2 && queues==1,"forest handoff remains independent of ordinary entrance routing");
    require(lastRenderedFillAlpha==255,"forest handoff needs a fully covered source draw before commit");
''' if is_oot else r'''
    EnterPassage(0);play.state.main=NativeFrameBody;play.transitionCtx.draw=CaptureNativeEffect;
    for(int i=0;i<20 && switches==2;++i) frame();
    require(switches==3 && saves==2 && queues==1,"forest handoff remains independent of ordinary entrance routing");
    require(lastRenderedFillAlpha==255,"forest handoff needs a fully covered source draw before commit");
''')
    if not is_oot:
        text = text.replace('auto frame=[]() { GameInteractor::Fire<GameInteractor::OnGameStateMainStart>(); };',
                            'play.state.main=NativeFrameBody;play.transitionCtx.draw=CaptureNativeEffect;\n    auto frame=[]() { NativeFrame(); };')
    else:
        text = text.replace('auto frame=[]() { GameInteractor::Fire<GameInteractor::OnGameFrameUpdate>(); };',
                            'play.state.main=NativeFrameBody;\n    auto frame=[]() { NativeFrame(); };')
    return '#include <cassert>\n#include <string>\n' + text


handoffs = [(game + '_handoff', handoff_fixture(game)) for game in ('oot', 'mm')]

with tempfile.TemporaryDirectory(prefix='transition-fade-') as tmp:
    path = Path(tmp)
    failures = []
    for game, source in [('oot', oot), ('mm', mm), *handoffs]:
        if args.game and args.game != game:
            continue
        cpp = path / (game + '.cpp')
        binary = path / game
        cpp.write_text(source)
        cmd = [os.environ.get('CXX', 'c++'), '-std=c++20', '-O1', '-Wno-narrowing',
               '-DF3DEX_GBI_2', '-DCONTROLLERBUTTONS_T=uint32_t',
               '-DLOG_LEVEL_GAME_PRINTS=0', '-I' + str(ROOT),
               '-I' + str(ROOT.parent / 'build-deps/usr/include')]
        if game == 'oot' or game.endswith('_handoff'):
            cmd += ['-DCOMBO_BUILD']
        if game == 'mm_handoff':
            # The native MM header set deliberately redefines the libultraship button macros.
            # The imported host block also contains unrelated arrival entrypoints.
            # Match the forest fixture's section GC so only exercised paths must link.
            cmd += ['-w', '-ffunction-sections', '-fdata-sections', '-Wl,--gc-sections']
        native_game = 'mm' if game == 'mm_handoff' else 'soh'
        for p in ('combo', native_game, native_game + '/include', native_game + '/include/PR', native_game + '/src',
                  native_game + '/assets', native_game + '/mods', native_game + '/soh', native_game + '/2s2h', 'libultraship/include'):
            cmd += ['-I' + str(ROOT / p)]
        if args.sanitize:
            cmd += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-g1']
        subprocess.run(cmd + [str(cpp), '-o', str(binary)], check=True)
        result = subprocess.run([str(binary)], check=False)
        if result.returncode:
            failures.append(game)
    assert not failures, 'native fade boundary failed: ' + ', '.join(failures)
