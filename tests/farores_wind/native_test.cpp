#include "global.h"
#include "variables.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include "gameplay/ComboFaroresWindJson.h"
#include "ComboGameMode.h"
extern "C" { int gComboGameMode = COMBO_GAME_BOTH; }
static int32_t activeCheckpoint = -1;
static int checkpointRestores = 0;
extern "C" int32_t Combo_SOH_SaveCrossDungeonCheckpoint(void) { return activeCheckpoint; }
extern "C" int32_t Combo_MM_SaveCrossDungeonCheckpoint(void) { return activeCheckpoint; }
extern "C" void Combo_SOH_RestoreCrossDungeonCheckpoint(int32_t index) { activeCheckpoint = index; ++checkpointRestores; }
extern "C" void Combo_MM_RestoreCrossDungeonCheckpoint(int32_t index) { activeCheckpoint = index; ++checkpointRestores; }
// NATIVE_FLAG_CAPTURE
#ifdef TEST_MM
#include "mods/nei_save.h"
#include "../../mm/2s2h/ComboFaroresWind.cpp"
static NeiSaveData nativeNei{};
extern "C" NeiSaveData* Nei_Save(void) { return &nativeNei; }
extern "C" {
s32 gSceneEntranceTableCount = 10;
SceneEntranceTableEntry sSceneEntranceTable[10]{};
}
extern "C" s16 Play_GetOriginalSceneId(s16 scene) { return scene; }
constexpr int ownGame = 1;
static int nativeSet() { return nativeNei.fwSet; }
static void setNativePoint() {
    nativeNei.fwSet = 1;
    nativeNei.fwEntrance = 0x1230;
    nativeNei.fwRoomIndex = 3;
    nativeNei.fwYaw = -300;
    nativeNei.fwPosX = 111;
    nativeNei.fwPosY = 222;
    nativeNei.fwPosZ = -333;
}
#define SET_CALLBACKS MM_SetFwCallbacks
#define SET_ARRIVAL MM_SetFwArrival
#else
#include "../../soh/soh/ComboFaroresWind.cpp"
constexpr int ownGame = 0;
static int nativeSet() { return gSaveContext.fw.set; }
static void setNativePoint() {
    gSaveContext.fw.set = 1;
    gSaveContext.fw.entranceIndex = 0x123;
    gSaveContext.fw.roomIndex = 3;
    gSaveContext.fw.yaw = -300;
    gSaveContext.fw.pos = {111, 222, -333};
    gSaveContext.linkAge = 0;
}
#define SET_CALLBACKS SOH_SetFwCallbacks
#define SET_ARRIVAL SOH_SetFwArrival
#endif

SaveContext gSaveContext{};
#ifndef TEST_MM
EntranceInfo gEntranceTable[ENTR_MAX]{};
#endif
PlayState* gPlayState = nullptr;
static ComboFwPoint points[3]{};
static int states[3]{-1, -1, -1};
static int queued = 0;
static bool available = true;
extern "C" void SOH_QueueFwHandoff(int slot) { assert(slot == 1); ++queued; }
extern "C" void MM_QueueFwHandoff(void) { ++queued; }
static int readPoint(int slot, ComboFwPoint* p) {
    assert(slot >= 0 && slot < 3);
    *p = points[slot];
    return states[slot];
}
static int writePoint(int slot, const ComboFwPoint* p) {
    assert(slot >= 0 && slot < 3);
    if (p) { points[slot] = *p; states[slot] = 1; }
    else states[slot] = 0;
    return 1;
}
static int request(int game, int slot) {
    assert(game == ownGame && slot == 1);
    return available;
}

int main() {
#ifdef TEST_MM
    EntranceTableEntry entry{};
    EntranceTableEntry* spawns[32]{};
    spawns[3] = &entry;
    sSceneEntranceTable[9].tableCount = 4;
    sSceneEntranceTable[9].table = spawns;
#endif
    gSaveContext.fileNum = 1;
    assert(!ComboFw_HasPoint(0) && ComboFw_HasPoint(1)); // Compatibility without launcher callbacks.
    ComboFwCallbacks callbacks{readPoint, writePoint, request};
    SET_CALLBACKS(&callbacks);
    assert(!ComboFw_HasPoint(0) && states[1] == -1); // Don't erase the other game's legacy point.
    activeCheckpoint = 0x456; // Old point belongs elsewhere: migration must not capture this.
    setNativePoint();
    assert(ComboFw_HasPoint(1) && states[1] == 1 && points[1].game == ownGame);
    assert(states[0] == -1 && states[2] == -1);
    assert(points[1].x == 111 && points[1].z == -333 && points[1].room == 3);
    assert(ComboFw_EncodePoint(points[1])["dungeonCheckpoint"] == -1);
    assert(checkpointRestores == 0);
    // An actual new cast captures the active dungeon and persists independently of movement.
    activeCheckpoint = 0x123;
    ComboFw_PublishPoint();
    assert(ComboFw_EncodePoint(points[1])["dungeonCheckpoint"] == 0x123);
    auto own = points[1];
    own.tempSwitchFlags = 55;
    own.tempCollectFlags = 77;
    own.tempCollectFlagsLow = 123;
    auto foreign = own;
    foreign.game = 1 - ownGame;
    foreign.age = foreign.game == 0 ? 1 : -1;
    writePoint(1, &foreign);
    ComboFw_SyncPoint();
    assert(checkpointRestores == 0); // Merely syncing a point is not recall.
    assert(!nativeSet() && ComboFw_HasPoint(0)); // Foreign point opens menu without a false local pillar.
    assert(ComboFw_RequestReturn() == 1 && queued == 1);
    available = false;
    assert(ComboFw_RequestReturn() == -1 && queued == 1 && states[1] == 1);
    assert(checkpointRestores == 0 && activeCheckpoint == 0x123); // Handoff failure is non-mutating.
    assert(!SET_ARRIVAL(1, &foreign));
    assert(!SET_ARRIVAL(255, &own));
    auto invalid = own;
    invalid.entrance = 0xFFFF;
    assert(!SET_ARRIVAL(1, &invalid));
    writePoint(1, &invalid);
    assert(!ComboFw_HasPoint(1) && ComboFw_RequestReturn() == -1);
    ComboFw_SyncPoint();
    assert(!nativeSet());
#ifdef TEST_MM
    invalid.entrance = 0x12F0; // Valid scene, out-of-range spawn.
    assert(!SET_ARRIVAL(1, &invalid));
    invalid.entrance = own.entrance | 0xF; // Scene/spawn valid, but nonexistent alternate layer.
    assert(!SET_ARRIVAL(1, &invalid));
#else
    invalid.entrance = ENTR_MAX - 1;
    assert(!SET_ARRIVAL(1, &invalid));
    invalid.entrance = ENTR_UNUSED_6E;
    gEntranceTable[ENTR_UNUSED_6E + 3].scene = SCENE_UNUSED_6E;
    assert(!SET_ARRIVAL(1, &invalid));
#endif
    assert(SET_ARRIVAL(1, &own));
    writePoint(1, &own);
    // Save loading may reset respawns: apply after the load, never before it.
    gSaveContext.respawn[RESPAWN_MODE_TOP] = {};
#ifndef TEST_MM
    gSaveContext.linkAge = 1; // The point is no authority to transform Link or grant age-dependent gear.
#endif
#ifdef TEST_MM
    gSaveContext.save.day = 2;
    gSaveContext.save.time = 0x7788;
    gSaveContext.save.playerForm = PLAYER_FORM_ZORA;
#endif
    activeCheckpoint = 0x456; // Loaded recovery for B must be superseded by the point in A.
    assert(ComboFw_ApplyArrival() && !ComboFw_ApplyArrival());
    assert(activeCheckpoint == 0x123 && checkpointRestores == 1);
    auto& top = gSaveContext.respawn[RESPAWN_MODE_TOP];
    ComboFw_ValidateArrivalRoom(4);
    assert(top.pos.x == 111 && top.pos.y == 222 && top.pos.z == -333 && top.yaw == -300);
    assert(top.roomIndex == 3 && gSaveContext.respawnFlag == 3);
    assert(top.tempCollectFlags == 77);
#ifdef TEST_MM
    assert(top.entrance == own.entrance && gSaveContext.save.entrance == own.entrance);
    assert(top.playerParams == PLAYER_PARAMS(0xFF, PLAYER_START_MODE_D));
    assert(top.tempSwitchFlags == 55 && top.unk_18 == 123);
    assert(ComboFw_ConsumeArrivalAnimation() && !ComboFw_ConsumeArrivalAnimation());
#else
    assert(top.entranceIndex == own.entrance && gSaveContext.entranceIndex == own.entrance);
    assert(top.tempSwchFlags == 55);
    assert(gSaveContext.linkAge == 1 && top.playerParams == 0x6FF);
#endif
    activeCheckpoint = 0x456;
    assert(ComboFw_RequestReturn() == 0); // Local recall restores the identical checkpoint.
    assert(activeCheckpoint == 0x123 && checkpointRestores == 2);
    auto legacy = ComboFw_EncodePoint(own);
    legacy.erase("dungeonCheckpoint");
    ComboFwPoint oldPoint{};
    assert(ComboFw_DecodePoint(legacy, oldPoint));
    writePoint(1, &oldPoint);
    activeCheckpoint = 0x456;
    assert(ComboFw_RequestReturn() == 0);
    assert(activeCheckpoint == -1 && checkpointRestores == 3);
#ifdef TEST_MM
    assert(gSaveContext.save.day == 2 && gSaveContext.save.time == 0x7788);
    assert(gSaveContext.save.playerForm == PLAYER_FORM_ZORA);
#endif
    ComboFw_ClearPoint();
    setNativePoint(); // Stale native copy must not resurrect a dispelled shared point.
    assert(!ComboFw_HasPoint(1));
    ComboFw_SyncPoint();
    assert(!nativeSet() && states[1] == 0);
    writePoint(1, &foreign);
    ComboFw_ClearLocalPoint();
    assert(states[1] == 1 && points[1].game == foreign.game);
    writePoint(1, &own);
    ComboFw_ClearLocalPoint();
    assert(states[1] == 0 && !nativeSet());
    assert(SET_ARRIVAL(0, &own));
    assert(!ComboFw_ApplyArrival()); // Arrival addressed to a different file never touches this one.
    invalid = own;
    invalid.room = 255;
    writePoint(1, &invalid);
    assert(SET_ARRIVAL(1, &invalid) && ComboFw_ApplyArrival());
    ComboFw_ValidateArrivalRoom(4);
    assert(!nativeSet() && !gSaveContext.respawnFlag && states[1] == 0);
#ifdef TEST_MM
    PlayState play{};
    gPlayState = &play;
    play.sceneId = SCENE_CLOCKTOWER;
    auto& live = play.actorCtx.sceneFlags;
    live.chest = 17;
    live.switches[0] = 23;
    live.switches[1] = 31;
    live.collectible[0] = 41;
    live.clearedRoom = 47;
    gSaveContext.cycleSceneFlags[3].chest = 97; // Earlier scene in the same cycle.
    gSaveContext.save.saveInfo.permanentSceneFlags[3].rooms = 83;
    MM_CaptureFwDeparture();
    auto& captured = gSaveContext.save.saveInfo.permanentSceneFlags[SCENE_CLOCKTOWER];
    assert(captured.chest == 17 && captured.switch0 == 23 && captured.switch1 == 31);
    assert(captured.collectible == 41 && captured.clearedRoom == 47);
    assert(gSaveContext.save.saveInfo.permanentSceneFlags[3].chest == 97);
    assert(gSaveContext.save.saveInfo.permanentSceneFlags[3].rooms == 83);
    gPlayState = nullptr;
    MM_CaptureFwDeparture(); // No gameplay context must be harmless.
#endif
    std::cout << "native bridge: legacy adoption, both owners, return rejection, respawn, consumption, slot isolation passed\n";
}
