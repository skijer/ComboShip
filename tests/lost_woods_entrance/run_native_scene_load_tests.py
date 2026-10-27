#!/usr/bin/env python3
"""Exercise the actual scene loader's exploration/prologue resource choice."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from

source = (ROOT / "mm/2s2h/z_play_2SH.cpp").read_text()
body = block_from(source, source.index('extern "C" void OTRPlay_SpawnScene('))
fixture = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <set>
#include <string>
#if __has_include("combo/entrances/NativeForest.h")
#include "combo/entrances/NativeForest.h"
#endif
using s32 = int;
using u8 = unsigned char;
struct RoomContext {};
struct SceneTableEntry { int unk_D; struct { const char* fileName; } segment; int drawConfig; };
struct PlayState { SceneTableEntry* loadedScene; int sceneId, sceneConfig; void* sceneSegment; RoomContext roomCtx; };
SceneTableEntry gSceneTable[] = {{9,{"Z2_LOST_WOODS"},7},{9,{"Z2_TOWN"},3}};
uintptr_t gSegments[16];
std::set<std::string> files;
std::string loaded;
bool exploration;
bool dependencyDataAvailable=true;
namespace NativeForest {
NativeForestDependencies::Graph LoadDependencies(const char*) {
 return dependencyDataAvailable ? NativeForestDependencies::Graph{{"custom/comboship/native_forest/geometry/ancestor"},
                                                                  {"scenes/native/dl"}} : NativeForestDependencies::Graph{};
}
}
int initCalls, roomCalls;
int ComboOpeningForest_IsExploration(const PlayState* play) { return exploration && play->sceneId == 0; }
u8 ResourceMgr_FileExists(const char* path) { return files.count(path); }
namespace StringHelper {
std::string Sprintf(const char*, const char* a, const char* b) { return std::string("scenes/nonmq/")+a+"/"+b; }
}
void* OTRPlay_LoadFile(PlayState*, const char* path) { loaded=path; return reinterpret_cast<void*>(0x1000); }
void OTRPlay_InitScene(PlayState*, s32) { ++initCalls; }
void Room_SetupFirstRoom(PlayState*, RoomContext*) { ++roomCalls; }
/* PRODUCTION */
int main() {
    files={"custom/comboship/native_forest/materials_v1",
           "scenes/nonmq/comboship_forest/comboship_forest",
           "scenes/nonmq/comboship_forest/comboship_forest_room_0",
           "scenes/nonmq/comboship_forest/comboship_forest_room_1",
           "scenes/nonmq/comboship_forest/comboship_forest_room_2"};
    files.insert("scenes/nonmq/comboship_forest/comboship_forest_collision");
    for(const char* core:NativeForestResources::kCommon)files.insert(core);
    files.insert("custom/comboship/native_forest/dependencies_combined_v1");
    files.insert("custom/comboship/native_forest/geometry/ancestor");
    files.insert("scenes/native/dl");
    files.insert("scenes/nonmq/Z2_LOST_WOODS/Z2_LOST_WOODS_room_00");
    files.insert("scenes/nonmq/Z2_26SARUNOMORI/Z2_26SARUNOMORITex_00BAC0");
    PlayState play{};
    exploration=true;
    OTRPlay_SpawnScene(&play,0,0);
    assert(loaded=="scenes/nonmq/comboship_forest/comboship_forest");
    assert(play.sceneId==0 && play.sceneConfig==7 && play.loadedScene==&gSceneTable[0]);
    assert(gSegments[2]==0x1000 && initCalls==1 && roomCalls==1);
    files.erase("custom/comboship/native_forest/geometry/ancestor");
    OTRPlay_SpawnScene(&play,0,0);
    assert(loaded=="scenes/nonmq/Z2_LOST_WOODS/Z2_LOST_WOODS");
    files.insert("custom/comboship/native_forest/geometry/ancestor");
    files.erase("scenes/native/dl");
    OTRPlay_SpawnScene(&play,0,0);
    assert(loaded=="scenes/nonmq/Z2_LOST_WOODS/Z2_LOST_WOODS");
    files.insert("scenes/native/dl");
    dependencyDataAvailable=false;
    OTRPlay_SpawnScene(&play,0,0);
    assert(loaded=="scenes/nonmq/Z2_LOST_WOODS/Z2_LOST_WOODS");
    dependencyDataAvailable=true;
    exploration=false;
    OTRPlay_SpawnScene(&play,0,0);
    assert(loaded=="scenes/nonmq/Z2_LOST_WOODS/Z2_LOST_WOODS");
    exploration=true;
    OTRPlay_SpawnScene(&play,1,0);
    assert(loaded=="scenes/nonmq/Z2_TOWN/Z2_TOWN");
    files.clear();
    OTRPlay_SpawnScene(&play,0,0);
    assert(loaded=="scenes/nonmq/Z2_LOST_WOODS/Z2_LOST_WOODS");
    puts("PASS native forest scene load: exploration-only private resources, unchanged scene ID/config, native prologue and missing-resource fallback");
}
'''.replace("/* PRODUCTION */", body)
with tempfile.TemporaryDirectory(prefix="native-forest-load-") as directory:
    path = Path(directory) / "test.cpp"
    binary = Path(directory) / "test"
    path.write_text(fixture)
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-DCOMBO_BUILD", "-I"+str(ROOT), str(path), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
