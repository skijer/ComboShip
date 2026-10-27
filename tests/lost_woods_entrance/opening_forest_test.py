#!/usr/bin/env python3
"""Execute the opening-forest route and native prologue boundaries with real save types.

Disk/DLL dispatch and scene-resource payloads are boundaries. Boot, arrival,
scene-command and cutscene-entry code is extracted from the production paths.
This is not an installed-game or rendered-scene test.
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
parser.add_argument('--case', choices=['mm', 'oot', 'scene', 'cutscene', 'saving', 'season'])
parser.add_argument('--sanitize', action='store_true')
args = parser.parse_args()


def function(text, signature):
    return block_from(text, text.index(signature))


def build(game, source, name):
    with tempfile.TemporaryDirectory(prefix='opening-forest-' + name + '-') as directory:
        cpp = Path(directory) / 'test.cpp'
        binary = Path(directory) / 'test'
        cpp.write_text('#include "ComboGameMode.h"\nextern "C" { int gComboGameMode = COMBO_GAME_BOTH; }\n' + source)
        includes = [game, game + ('/2s2h' if game == 'mm' else '/soh'), game + '/include',
                    game + '/include/PR', game + '/src', game + '/assets', 'libultraship/include', 'combo']
        command = [os.environ.get('CXX', 'c++'), '-std=c++20', '-w', '-DF3DEX_GBI_2',
                   '-DCOMBO_BUILD', '-DFMT_HEADER_ONLY', '-ffunction-sections', '-fdata-sections',
                   '-Wl,--gc-sections', *['-I' + str(ROOT / path) for path in includes]]
        if args.sanitize:
            command += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-no-pie']
        result = subprocess.run([*command, str(cpp), '-o', str(binary)], capture_output=True, text=True)
        if result.returncode:
            raise SystemExit(result.stderr[:7000])
        env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0')
        subprocess.run([str(binary)], check=True, env=env)


port = (ROOT / 'mm/2s2h/BenPort.cpp').read_text()
oot = (ROOT / 'soh/soh/OTRGlobals.cpp').read_text(encoding='utf-8-sig')

if args.case in (None, 'mm'):
    source = (ROOT / 'tests/lost_woods_entrance/mm_arrival_test.cpp').read_text().split('int main()')[0]
    source = source.replace('/* ARRIVAL_BODY */', function(port, 'extern "C" void MM_ApplyComboPortalArrival('))
    source = source.replace('/* ROUTE_BODY */', '\n'.join(function(port, signature) for signature in (
        'static void Combo_CancelLostWoodsPassage(', 'static void Combo_ClearReturnRequests(',
        'extern "C" COMBO_EXPORT void MM_SetComboEntryIsResume(',
        'extern "C" COMBO_EXPORT void MM_SetComboPortalRoute(')))
    body = function((ROOT / 'mm/src/code/title_setup.c').read_text(), 'void Setup_InitImpl(')
    source = source.replace('/* BOOT_BODY */', re.sub(r'\bthis\b', 'self', body))
    source += r'''
int main() {
    SetupState state{};
    for (int slot = 0; slot < 3; ++slot) {
        gComboStartFileNum = slot;
        MM_SetComboEntryIsResume(0);
        MM_SetComboPortalRoute(2);
        Setup_InitImpl(&state);
        require(queuedCameraRoute == COMBO_PORTAL_OPENING_FOREST && queuedCameraCount == slot + 1,
                "opening forest must queue one camera presentation after the matching save load");
        require(gSaveContext.save.entrance == ENTRANCE(LOST_WOODS, 0),
                "Fairy tunnel must load the playable opening forest, never the title or first-cycle intro");
        require(gSaveContext.fileNum == slot && gSaveContext.save.day == 2 &&
                gSaveContext.save.time == 0x8000 && gSaveContext.save.saveInfo.playerData.health == 0x50,
                "opening entry must load the matching existing slot and retain cycle/time/health");
        require(gSaveContext.respawn[RESPAWN_MODE_TOP].pos.y == 222,
                "opening entry must retain Farore's Wind");
    }
    for (int form = 0; form < PLAYER_FORM_MAX; ++form) {
        std::memset(&gSaveContext.save, 0x5A, sizeof(gSaveContext.save));
        gSaveContext.save.playerForm = form;
        gSaveContext.save.day = 3;
        gSaveContext.save.time = 0x1234;
        const auto progress = gSaveContext.save.saveInfo;
        const auto day = gSaveContext.save.day;
        const auto time = gSaveContext.save.time;
        const auto firstCycle = gSaveContext.save.isFirstCycle;
        const auto hasTatl = gSaveContext.save.hasTatl;
        const auto linkAge = gSaveContext.save.linkAge;
        MM_SetComboPortalRoute(2);
        MM_ApplyComboPortalArrival();
        require(std::memcmp(&progress, &gSaveContext.save.saveInfo, sizeof(progress)) == 0,
                "opening placement must preserve every inventory, quest, dungeon, weekly, rando and NEI save byte");
        require(gSaveContext.save.playerForm == form && gSaveContext.save.day == day &&
                gSaveContext.save.time == time && gSaveContext.save.isFirstCycle == firstCycle &&
                gSaveContext.save.hasTatl == hasTatl && gSaveContext.save.linkAge == linkAge,
                "opening placement must not grant, consume, transform, or reset story/cycle state");
        require(gSaveContext.save.cutsceneIndex == 0 && gSaveContext.nextCutsceneIndex == 0xFFEF &&
                gSaveContext.cutsceneTrigger == 0 && gSaveContext.respawnFlag == 0,
                "opening placement must clear pending prologue control and use an ordinary scene spawn");
    }
    std::puts("PASS opening forest: paired boot and complete progress-byte preservation for every form");
}
'''
    build('mm', source, 'mm')

if args.case in (None, 'oot'):
    source = (ROOT / 'tests/lost_woods_entrance/oot_arrival_test.cpp').read_text().split('int main()')[0]
    source = source.replace('/* ARRIVAL_BODY */', function(oot, 'extern "C" void SOH_ApplyComboPortalArrival('))
    source = source.replace('/* BOOT_BODY */', function((ROOT / 'soh/src/code/title_setup.c').read_text(),
                                                      'void TitleSetup_InitImpl('))
    source += r'''
int main() {
    GameState state{};
    sComboReturnPortalRoute = 2;
    TitleSetup_InitImpl(&state);
    const auto& position = gSaveContext.respawn[RESPAWN_MODE_RETURN];
    require(gSaveContext.entranceIndex == ENTR_LOST_WOODS_SOUTH_EXIT && position.roomIndex == 1 &&
            position.pos.x == 800 && position.pos.z == 160 && position.yaw == (s16)0x8000,
            "opening forest return must leave the same pink Fairy tunnel, outside its trigger");
    require(gSaveContext.linkAge == 1 && gSaveContext.health == 0x50 &&
            gSaveContext.respawn[RESPAWN_MODE_TOP].pos.y == 222,
            "opening forest return must retain the paired OoT age, health and Farore point");
    std::puts("PASS opening forest: OoT returns to the Fairy tunnel with paired save state intact");
}
'''
    build('soh', source, 'oot')

base = r'''
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "global.h"
#include "regs.h"
#include "gameplay/CrossGameTitleCard.h"
#if __has_include("2s2h/Enhancements/Gameplay/ComboOpeningForest.h")
#include "2s2h/Enhancements/Gameplay/ComboOpeningForest.h"
#include "entrances/SoloForest.h"
#endif
SaveContext gSaveContext{};
PlayState* gPlayState;
GameState* gGameState;
static void require(bool value, const char* message) {
    if (!value) { std::fprintf(stderr,"FAIL: %s\n",message); std::exit(1); }
}
#define GameInteractor_Should(...) true
'''

if args.case in (None, 'scene'):
    scene = (ROOT / 'mm/2s2h/z_scene_2SH.cpp').read_text(encoding='utf-8-sig')
    spawn = function(scene, 'void Scene_CommandSpawnList(')
    spawn = spawn[:spawn.index('    if ((PLAYER_GET_START_MODE')] + '}\n'
    time = function(scene, 'void Scene_CommandTimeSettings(')
    play = function((ROOT / 'mm/src/code/z_play.c').read_text(), 'void Play_Init(')
    entry = play[play.index('    s32 pad2;') + len('    s32 pad2;'):play.index('    MMWinterAtmosphere_Reset();')]
    source = base + r'''
RegEditor editor{};
RegEditor* gRegEditor = &editor;
extern "C" void ComboBlackMarket_NormalizeEntry() {} // independently covered by black_market/native_test.cpp
extern "C" f32 Math_SinS(s16) { return 0; }
extern "C" f32 Math_CosS(s16) { return 1; }
namespace SOH {
struct ISceneCommand {};
struct SetStartPositionList : ISceneCommand {
    ActorEntry* entries;
    unsigned numStartPositions = 1;
    void* GetRawPointer() { return entries; }
};
struct SetTimeSettings : ISceneCommand {
    struct { u8 hour, minute, timeIncrement; } settings;
};
}
// The separately tested native-solo lifecycle controller is a boundary here.
extern "C" void ComboWoodfallPassage_ReadSpawns(PlayState*, const ActorEntry*, unsigned) {}
extern "C" void ComboRegionalPassagesMm_ReadSpawns(PlayState*, const ActorEntry*, unsigned) {}
// This fixture exercises the unchanged combined opening/prologue entry path.
extern "C" void ComboSoloForestMm_NormalizeEntry() {
    require(Combo_IsCombinedGame(), "combined scene fixture must keep the solo controller inactive");
}
''' + spawn + time + '\nvoid NativePlayEntry() {\n' + entry + '\n}\n' + r'''
int main() {
    PlayState play{};
    gPlayState = &play; gGameState = &play.state;
    gSaveContext.gameMode = GAMEMODE_NORMAL;
    gSaveContext.fileNum = 0;
    gSaveContext.save.entrance = ENTRANCE(LOST_WOODS, 0);
    play.sceneId = SCENE_LOST_WOODS;
    ActorEntry original{};
    original.pos = { 100, 0, -100 }; original.rot.y = 1234;
    original.params = PLAYER_PARAMS(3, 0);
    const auto immutable = original;
    EntranceEntry entrance{};
    play.setupEntranceList = &entrance;
    SOH::SetStartPositionList spawns{}; spawns.entries = &original;
    Scene_CommandSpawnList(&play, &spawns);
    require(PLAYER_GET_START_MODE(play.linkActorEntry) == PLAYER_START_MODE_E &&
            PLAYER_GET_BG_CAM_INDEX(play.linkActorEntry) == 0xFF,
            "opening visit must use ordinary player control without a prologue action or fixed camera");
    require(std::memcmp(&original, &immutable, sizeof(original)) == 0 &&
            play.linkActorEntry->pos.x == 100 && play.linkActorEntry->rot.y == 1234,
            "safe spawn must preserve source resources, native location and yaw");
    SOH::SetTimeSettings settings{}; settings.settings = {8, 0, 0};
    gSaveContext.save.day = 3; gSaveContext.save.time = 0xABCD;
    Scene_CommandTimeSettings(&play, &settings);
    require(gSaveContext.save.day == 3 && gSaveContext.save.time == 0xABCD,
            "the prologue's fixed morning scene command must not replace the current cycle time");
    gSaveContext.save.entrance = ENTRANCE(OPENING_DUNGEON, 0);
    gSaveContext.nextCutsceneIndex = 0xFFF0;
    NativePlayEntry();
    require(gSaveContext.save.entrance == ENTRANCE(CLOCK_TOWER_INTERIOR, 0) &&
            gSaveContext.nextCutsceneIndex == 0xFFEF && gSaveContext.save.day == 3 &&
            gSaveContext.save.time == 0xABCD,
            "onward travel must bypass the curse prologue and reach the normal Clock Tower without resetting time");
    play.sceneId = SCENE_26SARUNOMORI;
    gSaveContext.save.entrance = ENTRANCE(WOODS_OF_MYSTERY, 0);
    Scene_CommandTimeSettings(&play, &settings);
    require(gSaveContext.save.time == (u16)CLOCK_TIME_ALT2_F(8,0),
            "ordinary scenes must retain their native time commands");
    std::puts("PASS opening forest: live spawn/time commands and Play entry preserve progress and normal scene behavior");
}
'''
    build('mm', source, 'scene')

if args.case in (None, 'cutscene'):
    demo = (ROOT / 'mm/src/code/z_demo.c').read_text()
    manager = (ROOT / 'mm/src/code/z_eventmgr.c').read_text()
    start = function(manager, 's16 CutsceneManager_Start(')
    # The camera/script allocator is the boundary. Run the actual preceding
    # start/rejection logic and count whether it reaches that boundary.
    start = start[:start.index('    ActorCutscene* csEntry;')] + '\n++starts; return csId;\n}\n'
    title = (ROOT / 'mm/src/code/title_setup.c').read_text()
    request = re.search(r'static s32 sComboCrossGameTitleCardPending[^;]*;', title)[0] + '\n'
    request += function(title, 'int Combo_MM_ConsumeCrossGameTitleCard(') + '\n'
    source = base + request + r'''
static int starts, spawnQueries;
extern "C" s16 CutsceneManager_FindEntranceCsId() { ++spawnQueries; return 0; }
extern "C" s16 CutsceneManager_GetCutsceneScriptIndex(s16 id) { return id == CS_ID_GLOBAL_TALK ? CS_SCRIPT_ID_NONE : 0; }
extern "C" s16 CutsceneManager_StartWithPlayerCs(s16,Actor*) { ++starts; return 0; }
extern "C" s32 Entrance_GetTransitionFlags(u16) { return 0; }
extern "C" void Message_DisplaySceneTitleCard(PlayState*,u16) {}
''' + start + function(demo, 'void Cutscene_HandleEntranceTriggers(') + r'''
int main() {
    PlayState play{}; gPlayState = &play;
    SceneTableEntry loadedScene{}; play.loadedScene = &loadedScene;
    play.sceneId = SCENE_LOST_WOODS;
    CutsceneScriptEntry scripts[1]{};
    scripts[0].spawnFlags = CS_SPAWN_FLAG_ALWAYS;
    play.csCtx.scriptList = scripts;
    gSaveContext.gameMode = GAMEMODE_NORMAL; gSaveContext.fileNum = 0;
    gSaveContext.save.entrance = ENTRANCE(LOST_WOODS,0);
    sComboCrossGameTitleCardPending = true;
    const auto progress = gSaveContext.save;
    Cutscene_HandleEntranceTriggers(&play);
    require(starts == 0 && spawnQueries == 0 &&
            std::memcmp(&progress,&gSaveContext.save,sizeof(progress)) == 0,
            "forest arrival must not query/start Skull Kid's scene cutscene or set its story flags");
    require(!Combo_MM_ConsumeCrossGameTitleCard(),
            "opening forest must consume the crossing request before its prologue bypass");
    require(CutsceneManager_Start(0,nullptr) == CS_ID_NONE && starts == 0,
            "walking deeper must not start a scripted prologue cutscene");
    require(CutsceneManager_Start(CS_ID_GLOBAL_TALK,nullptr) == CS_ID_GLOBAL_TALK && starts == 1,
            "ordinary player interaction must remain available");
    play.sceneId = SCENE_CLOCKTOWER;
    require(CutsceneManager_Start(0,nullptr) == 0 && starts == 2,
            "normal scenes must retain their cutscenes");
    std::puts("PASS opening forest: arrival and later scene scripts cannot start Skull Kid's cutscene; interactions remain native");
}
'''
    build('mm', source, 'cutscene')

if args.case in (None, 'saving'):
    saving = (ROOT / 'mm/2s2h/Enhancements/Saving/SavingEnhancements.cpp').read_text()
    source = base + r'''
static bool blocking;
extern "C" bool Player_InBlockingCsMode(PlayState*,Player*) { return blocking; }
''' + function(saving, 'extern "C" bool SavingEnhancements_CanSave(') + r'''
int main() {
    PlayState play{}; Player player{}; gPlayState = &play;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
    play.sceneId = SCENE_LOST_WOODS;
    gSaveContext.gameMode = GAMEMODE_NORMAL; gSaveContext.fileNum = 0;
    gSaveContext.flashSaveAvailable = true;
    gSaveContext.save.entrance = ENTRANCE(LOST_WOODS,0);
    require(SavingEnhancements_CanSave(),
            "a playable forest visit must allow normal pause/autosaving");
    gSaveContext.save.cutsceneIndex = 0xFFF0;
    require(!SavingEnhancements_CanSave(), "the actual prologue must retain its native save restriction");
    gSaveContext.save.cutsceneIndex = 0;
    blocking = true;
    require(!SavingEnhancements_CanSave(), "blocking player actions must still prevent saves");
    blocking = false; play.msgCtx.msgMode = 1;
    require(!SavingEnhancements_CanSave(), "dialog must still prevent saves");
    play.msgCtx.msgMode = 0; play.sceneId = SCENE_OPENINGDAN;
    require(!SavingEnhancements_CanSave(), "the curse/tutorial scene must retain its native save restriction");
    play.sceneId = SCENE_CLOCKTOWER;
    require(SavingEnhancements_CanSave(), "normal gameplay saving must remain available");
    std::puts("PASS opening forest: pause/autosave eligibility preserves native blocking and prologue restrictions");
}
'''
    build('mm', source, 'saving')

if args.case in (None, 'season'):
    weather = (ROOT / 'mm/2s2h/Enhancements/Audio/MMWeather.cpp').read_text()
    source = base + r'''
#include "2s2h/Enhancements/Audio/MMWeather.h"
#include "mods/extended_inventory.h"
static int manual=-2;
extern "C" int32_t CVarGetInteger(const char*,int32_t) { return manual; }
extern "C" uint8_t Seasons_GetSeason() { return SEASON_WINTER; }
''' + function(weather, 'bool OutdoorEligible(')
    if 'int SelectedSeason(' in weather:
        source += function(weather, 'int SelectedSeason(')
    source += function(weather, 'extern "C" int MMWeather_SeasonForPlay(')
    if 'extern "C" int MMWeather_FoliageSeasonForPlay(' in weather:
        source += function(weather, 'extern "C" int MMWeather_FoliageSeasonForPlay(')
    else:
        source += 'int MMWeather_FoliageSeasonForPlay(const PlayState* p) { return MMWeather_SeasonForPlay(p); }\n'
    source += r'''
int main() {
    PlayState play{}; Camera camera{};
    play.cameraPtrs[0]=&camera; play.skyboxId=SKYBOX_2;
    play.sceneId=SCENE_LOST_WOODS;
    play.envCtx.lightSettingOverride=LIGHT_SETTING_OVERRIDE_NONE;
    gSaveContext.gameMode=GAMEMODE_NORMAL; gSaveContext.fileNum=0;
    gSaveContext.save.entrance=ENTRANCE(LOST_WOODS,0);
    const auto progress=gSaveContext.save;
    require(MMWeather_SeasonForPlay(&play)==SEASON_WINTER,
            "playable Lost Woods must receive Rod weather with its actual SKYBOX_2");
    require(MMWeather_FoliageSeasonForPlay(&play)==SEASON_WINTER,
            "playable intro forest foliage must receive the selected season with its actual SKYBOX_2");
    play.roomCtx.curRoom.num=2;
    require(MMWeather_SeasonForPlay(&play)==-1,"the title-only room must retain native weather");
    for (int room : {0,1}) for (int selected=0;selected<SEASON_COUNT;++selected) {
        play.roomCtx.curRoom.num=room; manual=selected;
        require(MMWeather_FoliageSeasonForPlay(&play)==selected,"both playable rooms must follow manual seasons");
        require(MMWeather_SeasonForPlay(&play)==selected,"both playable rooms must receive seasonal weather");
    }
    play.roomCtx.curRoom.num=0;
    manual=-1;
    require(MMWeather_SeasonForPlay(&play)==-1,"Native/Off must bypass seasonal weather");
    require(MMWeather_FoliageSeasonForPlay(&play)==-1,"Native/Off must bypass the foliage treatment");
    manual=-2; gSaveContext.sceneLayer=1;
    require(MMWeather_SeasonForPlay(&play)==-1,"the real intro/flashback weather must remain native");
    require(MMWeather_FoliageSeasonForPlay(&play)==-1,"the real intro/flashback layer must remain native");
    gSaveContext.sceneLayer=0; play.csCtx.state=1;
    require(MMWeather_SeasonForPlay(&play)==-1,"cutscenes must retain native weather");
    require(MMWeather_FoliageSeasonForPlay(&play)==-1,"cutscene presentation must retain priority");
    play.csCtx.state=0; camera.stateFlags=CAM_STATE_UNDERWATER;
    require(MMWeather_SeasonForPlay(&play)==-1,"underwater weather must remain native");
    require(MMWeather_FoliageSeasonForPlay(&play)==-1,"underwater presentation must retain priority");
    camera.stateFlags=0; play.envCtx.lightSettingOverride=1;
    require(MMWeather_SeasonForPlay(&play)==-1,"story lighting must retain priority");
    play.envCtx.lightSettingOverride=LIGHT_SETTING_OVERRIDE_NONE; play.envCtx.customSkyboxFilter=true;
    require(MMWeather_SeasonForPlay(&play)==-1,"story sky filters must retain priority");
    play.envCtx.customSkyboxFilter=false; play.envCtx.skyboxDisabled=true;
    require(MMWeather_SeasonForPlay(&play)==-1,"disabled skies must retain priority");
    play.envCtx.skyboxDisabled=false; play.gameOverCtx.state=GAMEOVER_DEATH_START;
    require(MMWeather_SeasonForPlay(&play)==-1,"game-over must retain priority");
    play.gameOverCtx.state=GAMEOVER_INACTIVE; gSaveContext.fileNum=0xFF;
    require(MMWeather_SeasonForPlay(&play)==-1,"non-gameplay slots must retain native weather");
    gSaveContext.fileNum=0; gSaveContext.save.entrance=ENTRANCE(LOST_WOODS,1);
    require(MMWeather_SeasonForPlay(&play)==-1,"native prologue spawns must retain native weather");
    gSaveContext.save.entrance=ENTRANCE(LOST_WOODS,0); gSaveContext.save.cutsceneIndex=1;
    require(MMWeather_SeasonForPlay(&play)==-1,"saved cutscenes must retain native weather");
    gSaveContext.save.cutsceneIndex=0; play.sceneId=SCENE_SONCHONOIE;
    require(MMWeather_SeasonForPlay(&play)==-1,"other SKYBOX_2 interiors must remain ineligible");
    require(MMWeather_FoliageSeasonForPlay(&play)==-1,"other interiors must remain ineligible");
    require(memcmp(&progress,&gSaveContext.save,sizeof(progress))==0,"presentation selection must not mutate progression");
    std::puts("PASS opening forest: Rod/manual weather and foliage in both rooms; story/interior/save policy preserved");
}
'''
    source = source.replace('#include <cstdio>', '#include <cstdio>\n#include <initializer_list>')
    build('mm', source, 'season')
