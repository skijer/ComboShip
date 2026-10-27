#!/usr/bin/env python3
"""Round-trip the real MM portal writer/loader through the real JSON converters.

Container I/O, migration and item-audit logging are explicit boundaries. Native
scene-flag capture and SaveContext serialization/deserialization execute unchanged.
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
args = parser.parse_args()
production = (ROOT / 'mm/2s2h/SaveManager/SaveManager.cpp').read_text()
play = (ROOT / 'mm/src/code/z_play.c').read_text()


def function(text, marker):
    return block_from(text, text.index(marker))


source = r'''
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <type_traits>
#include "global.h"
#include "BenJsonConversions.hpp"
#include "ComboGameMode.h"
extern "C" {
int gComboGameMode = COMBO_GAME_BOTH;
char gGitCommitHash[] = "portal-save-fixture";
}
#define SPDLOG_INFO(...) ((void)0)
#define SPDLOG_ERROR(...) ((void)0)
SaveContext gSaveContext{};
PlayState* gPlayState;
GameState* gGameState;
int gComboOwlBlobSlot = -1;
static nlohmann::json disk;
static int writes, failures;
namespace ItemGrantAudit { struct Scope { explicit Scope(const char*) {} }; }
namespace ComboRando::CrossDungeon { int ReadCheckpoint(const nlohmann::json&) { return -1; } }
void Combo_MM_LoadCrossDungeonCheckpoint(int) {}
void ComboBlackMarket_LoadState(const nlohmann::json&, int) {} // market persistence has its own production fixture
static std::string SaveManager_GetFileName(int slot) { return "file" + std::to_string(slot) + ".json"; }
static int SaveManager_ReadSaveFile(const std::filesystem::path&,nlohmann::json& value) { value=disk; return 0; }
static int SaveManager_MigrateSave(nlohmann::json&) { return 0; }
static void SaveManager_WriteSaveFile(const std::filesystem::path&,nlohmann::json& value) { disk=value; ++writes; }
static int SaveManager_LoadFailedForCombo(int code) { ++failures; return code; }
static void require(bool value,const char* message) {
    if (!value) { std::fprintf(stderr,"FAIL: %s\n",message); std::exit(1); }
}
'''
source += re.search(r'const uint32_t CURRENT_SAVE_VERSION = \d+;', production).group(0) + '\n'
source += function(play, 's16 Play_GetOriginalSceneId(')
source += function(play, 'void Play_SaveCycleSceneFlags(').replace('PlayState* this', 'PlayState* self').replace('this->', 'self->')
source += function((ROOT / 'mm/src/code/z_sram_NES.c').read_text(), 'u16 Sram_CalcChecksum(').replace(
    'u8* dataPtr = data;', 'u8* dataPtr = (u8*)data;')
for marker in ('static bool SaveManager_CapturePortalSession(', 'static void SaveManager_RestorePortalRespawns('):
    if marker in production:
        source += function(production, marker)
source += function(production, 'void SaveManager_SaveCurrentForCombo(')
source += function(production, 'int SaveManager_LoadSaveFile(')
title = (ROOT / 'mm/src/code/title_setup.c').read_text()
source += 'void RestoreCycleFlags() {\n' + function(title, 'for (int i = 0; i < ARRAY_COUNT(gSaveContext.cycleSceneFlags); i++)') + '\n}\n'
source += function((ROOT / 'mm/2s2h/Enhancements/Saving/SavingEnhancements.cpp').read_text(), 'void LoadRespawnData(')
source += r'''
template<class Function> void SavePortalSession(Function save) {
    if constexpr (std::is_invocable_v<Function,bool>) save(true);
    else save(); // permits executing the actual pre-fix writer as the negative control
}
template<class Function> void SaveWithoutPortalSession(Function save) {
    if constexpr (std::is_invocable_v<Function,bool>) save(false);
    else save();
}
int main() {
    PlayState play{}; gPlayState=&play; gGameState=&play.state; play.state.running=true;
    for (bool remember : {false,true}) for (int slot=0;slot<3;++slot) {
        gSaveContext={}; disk=nlohmann::json{}; gComboOwlBlobSlot=-1;
        gSaveContext.fileNum=slot; gSaveContext.gameMode=GAMEMODE_NORMAL;
        gSaveContext.save.entrance=ENTRANCE(LOST_WOODS,0);
        gSaveContext.save.day=3; gSaveContext.save.time=0xABCD;
        gSaveContext.save.timeSpeedOffset=-2;
        gSaveContext.save.playerForm=PLAYER_FORM_GORON;
        gSaveContext.save.isFirstCycle=true; gSaveContext.save.hasTatl=true;
        gSaveContext.save.saveInfo.playerData.health=0x50;
        gSaveContext.save.saveInfo.playerData.rupees=147;
        gSaveContext.save.saveInfo.inventory.items[0]=ITEM_BOW;
        gSaveContext.save.saveInfo.weekEventReg[12]=0xA5;
        gSaveContext.eventInf[2]=0xA5;
        gSaveContext.jinxTimer=123;
        gSaveContext.respawn[RESPAWN_MODE_TOP].pos={11,22,33};
        gSaveContext.cycleSceneFlags[SCENE_MITURIN].chest=0xA501;
        gSaveContext.cycleSceneFlags[SCENE_MITURIN].switch0=0xDA12;
        play.sceneId=SCENE_LOST_WOODS;
        play.roomCtx.curRoom.num=0;
        play.actorCtx.sceneFlags.chest=0x1A;
        play.actorCtx.sceneFlags.switches[0]=0x5678;
        SavePortalSession(SaveManager_SaveCurrentForCombo);
        require(disk.contains("owlSave"),
                "portal departure must preserve a full current-session snapshot, even without an earlier owl save");
        require(disk.value("comboPortalSession",false), "the full session must identify its respawn-restoration policy");
        require(disk["owlSave"]["save"]["saveInfo"]["permanentSceneFlags"][SCENE_LOST_WOODS]["chest"]==0x1A,
                "portal snapshot must capture the current scene before teardown");
        const auto liveProgress=nlohmann::json(gSaveContext.save.saveInfo);
        auto snapshotProgress=disk["owlSave"]["save"]["saveInfo"];
        require(!gSaveContext.save.isOwlSave,
                "portal snapshot metadata must not turn the running game into an owl-save quit");
        gSaveContext={};
        require(SaveManager_LoadSaveFile(slot+1)==0 && gSaveContext.fileNum==slot && failures==0,
                "portal round trip must reload the same existing slot without initialization or repair");
        RestoreCycleFlags();
        if (remember) LoadRespawnData(slot);
        snapshotProgress["checksum"]=gSaveContext.save.saveInfo.checksum;
        require(nlohmann::json(gSaveContext.save.saveInfo)==snapshotProgress,
                "all native/randomizer/NEI progress must survive real JSON serialization and loading");
        require(gSaveContext.save.day==3 && gSaveContext.save.time==0xABCD &&
                gSaveContext.save.timeSpeedOffset==-2 && gSaveContext.save.playerForm==PLAYER_FORM_GORON &&
                gSaveContext.eventInf[2]==0xA5 && gSaveContext.jinxTimer==123 &&
                gSaveContext.respawn[RESPAWN_MODE_TOP].pos.y==22 &&
                gSaveContext.cycleSceneFlags[SCENE_MITURIN].switch0==0xDA12,
                "current cycle, form, temporary quest state, timers, scene switches and Farore point must survive");
    }
    require(writes==6,"each portal snapshot must persist once");
    gComboOwlBlobSlot=-1;
    SaveWithoutPortalSession(SaveManager_SaveCurrentForCombo);
    require(!disk.contains("owlSave") && !disk.contains("comboPortalSession"),
            "dropping a stale owl snapshot must also remove its portal restoration policy");
    disk["comboPortalSession"]=true;
    SaveWithoutPortalSession(SaveManager_SaveCurrentForCombo);
    require(!disk.contains("comboPortalSession"), "a bare save must not retain a stray portal marker");
    disk["owlSave"]=nlohmann::json::object(); disk["comboPortalSession"]=true;
    gComboOwlBlobSlot=(int)gSaveContext.fileNum+1;
    SaveWithoutPortalSession(SaveManager_SaveCurrentForCombo);
    require(!disk.contains("owlSave") && !disk.contains("comboPortalSession"),
            "dropping a malformed owl snapshot must also remove its portal restoration policy");
    const int completedWrites=writes;
    const auto completedDisk=disk;
    gGameState=nullptr;
    SavePortalSession(SaveManager_SaveCurrentForCombo);
    require(writes==completedWrites && disk==completedDisk,
            "a requested portal snapshot must refuse to save outside the active play state");
    gGameState=&play.state;
    gSaveContext.gameMode=GAMEMODE_TITLE_SCREEN;
    SavePortalSession(SaveManager_SaveCurrentForCombo);
    require(writes==completedWrites && disk==completedDisk,
            "a title-demo portal request must never persist its initialized state over progress");
    gSaveContext.gameMode=GAMEMODE_NORMAL;
    for (int invalidSlot : {-1,3,255}) {
        gSaveContext.fileNum=invalidSlot;
        SavePortalSession(SaveManager_SaveCurrentForCombo);
    }
    require(writes==completedWrites && disk==completedDisk, "invalid slots must never write a portal snapshot");
    std::puts("PASS portal save: three slots, both Remember settings, complete progress and invalid-state write guards");
}
'''

with tempfile.TemporaryDirectory(prefix='portal-save-') as directory:
    cpp=Path(directory)/'test.cpp'; binary=Path(directory)/'test'; cpp.write_text(source)
    includes=['mm','mm/2s2h','mm/include','mm/include/PR','mm/src','mm/assets','libultraship/include','combo']
    command=[os.environ.get('CXX','c++'),'-std=c++20','-w','-DCOMBO_BUILD','-DF3DEX_GBI_2',
             '-DFMT_HEADER_ONLY','-ffunction-sections','-fdata-sections','-Wl,--gc-sections',
             *['-I'+str(ROOT/path) for path in includes]]
    if args.sanitize:
        command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie']
    result=subprocess.run([*command,str(cpp),'-o',str(binary)],capture_output=True,text=True)
    if result.returncode: raise SystemExit(result.stderr[:6500])
    subprocess.run([str(binary)],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
