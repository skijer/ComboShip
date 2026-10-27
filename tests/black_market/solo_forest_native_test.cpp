#include "global.h"
#include "ComboGameMode.h"
#include "entrances/SoloForest.h"
#include "entrances/NativeForest.h"
#include "entrances/LostWoodsEntrance.h"
#include <nlohmann/json.hpp>
#include <cassert>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <functional>
#include <type_traits>
extern "C" { int gComboGameMode; }
SaveContext gSaveContext{};
// Native title policy is exercised by the two lost_woods_entrance title suites.
extern "C" void Combo_MM_RequestCrossGameTitleCard(void) {}
extern "C" void Combo_SOH_RequestCrossGameTitleCard(void) {}
PlayState* gPlayState;
#ifndef OOT_FOREST_TEST
GameState* gGameState;
#else
EntranceInfo gEntranceTable[ENTR_MAX];
#endif
struct ForestOrigin { int entrance; Vec3f pos; s16 yaw; s8 room; };
ForestOrigin sForestOrigin, sGroundedOrigin;
int sForestSlot = -1;
u32 sForestSeed;
bool sForestResume, sForestArmed, sOriginStaged, sLocalReturning, sForestTitle;
RespawnData sForestPreservedReturn;
bool pack = true, inCs = false;
nlohmann::json persisted;
ForestOrigin originAtWait;
int waits;
static PlayState play;
static Player player;
static CollisionPoly floorPoly;
u32 ForestSeed() { return 77; }
bool ForestPack() { return pack; }
#ifdef OOT_FOREST_TEST
extern "C" s32 Player_InCsMode(PlayState*) { return inCs; }
nlohmann::json sForestPending, nativeForestDisk;
bool sForestLoadPending = true, saveMutexHeld = false;
constexpr int SECTION_PARENT_NONE = -1;
struct SaveManager {
    static SaveManager* Instance;
    std::function<void(bool)> init;
    std::function<void()> load;
    void ThreadPoolWait() {
        assert(!saveMutexHeld && "waiting on native save worker while holding saveMtx would deadlock");
        ++waits; originAtWait = sForestOrigin;
    }
    void SaveData(const std::string&, const nlohmann::json& value) { persisted = value; }
    void LoadData(const std::string&, nlohmann::json& value) { value = nativeForestDisk; }
    void AddInitFunction(std::function<void(bool)> callback) { init = callback; }
    void AddLoadFunction(const std::string&, int, std::function<void()> callback) { load = callback; }
    template<class Callback> void AddSaveFunction(const std::string&, int, Callback, bool, int) {}
};
SaveManager manager;
SaveManager* SaveManager::Instance = &manager;
struct GameInteractor {
    struct OnLoadGame {};
    struct OnSceneInit {};
    struct OnPlayerUpdate {};
    struct OnPlayDrawEnd {};
    struct OnPlayDestroy {};
    static GameInteractor* Instance;
    std::function<void(int)> onLoad;
    template<class Hook, class Callback> void RegisterGameHook(Callback callback) {
        if constexpr (std::is_same_v<Hook, OnLoadGame>) onLoad = callback;
    }
};
GameInteractor interactor;
GameInteractor* GameInteractor::Instance = &interactor;
void DrawForestPortal() {}
#else
extern "C" bool Player_InCsMode(PlayState*) { return inCs; }
#endif
/* PRODUCTION */

void SetScene(int scene, int room) {
#ifdef OOT_FOREST_TEST
    play.sceneNum = scene;
#else
    play.sceneId = scene;
#endif
    play.roomCtx.curRoom.num = room;
}
int& Entrance() {
#ifdef OOT_FOREST_TEST
    return gSaveContext.entranceIndex;
#else
    return gSaveContext.save.entrance;
#endif
}
int CurrentCutscene() {
#ifdef OOT_FOREST_TEST
    return gSaveContext.cutsceneIndex;
#else
    return gSaveContext.save.cutsceneIndex;
#endif
}
constexpr int kForestScene =
#ifdef OOT_FOREST_TEST
    COMBO_SOLO_FOREST_OOT_SCENE;
#else
    COMBO_SOLO_FOREST_MM_SCENE;
#endif
constexpr int kForestEntrance =
#ifdef OOT_FOREST_TEST
    COMBO_SOLO_FOREST_OOT_ENTRANCE;
#else
    COMBO_SOLO_FOREST_MM_ENTRANCE;
#endif
int NextEntrance() {
#ifdef OOT_FOREST_TEST
    return play.nextEntranceIndex;
#else
    return play.nextEntrance;
#endif
}
void Capture() {
#ifdef OOT_FOREST_TEST
    SaveForestState(gSaveContext, 0, false);
#else
    nlohmann::json document; SaveForestState(document, 0); persisted = document["soloForest"];
#endif
}
void Reset() {
    gSaveContext = {}; play = {}; player = {}; floorPoly = {};
    pack = true; inCs = false; waits = 0; persisted = nullptr;
    gPlayState = &play; play.state.running = true; floorPoly.normal.y = 32767;
    player.actor.floorPoly = &floorPoly; player.actor.floorHeight = 0;
#ifdef OOT_FOREST_TEST
    gComboGameMode = COMBO_GAME_OOT;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
    gEntranceTable[ENTR_LOST_WOODS_SOUTH_EXIT].scene = SCENE_LOST_WOODS;
#else
    gComboGameMode = COMBO_GAME_MM; gGameState = &play.state;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
    gSaveContext.save.shipSaveInfo.rando.finalSeed = 77;
#endif
    gSaveContext.gameMode = GAMEMODE_NORMAL; gSaveContext.fileNum = 0;
    LoadForestState(nlohmann::json(), 0);
    sForestArmed = sOriginStaged = sLocalReturning = sForestTitle = false;
}
void Approach() {
#ifdef OOT_FOREST_TEST
    SetScene(SCENE_LOST_WOODS, 1); Entrance() = ENTR_LOST_WOODS_SOUTH_EXIT;
    player.actor.world.pos = { 800, 0, 160 };
#else
    SetScene(SCENE_26SARUNOMORI, 1); Entrance() = ENTRANCE(WOODS_OF_MYSTERY, 0);
    player.actor.world.pos = { 0, 0, 0 };
#endif
    ForestPlayerUpdate();
}
void Enter() {
#ifdef OOT_FOREST_TEST
    player.actor.world.pos = { 800, 80, 340 };
#else
    player.actor.world.pos = { 160, 80, -160 };
#endif
    ForestPlayerUpdate();
}
int main() {
#ifndef OOT_FOREST_TEST
    Reset(); assert(ComboSoloForestMm_PassageAvailable());
    play.pauseCtx.state = 1; assert(ComboSoloForestMm_PassageAvailable()); // a paused route remains visible
    pack = false; assert(!ComboSoloForestMm_PassageAvailable()); pack = true;
    sForestSeed = 78; assert(!ComboSoloForestMm_PassageAvailable()); sForestSeed = 77;
    sForestSlot = -1; assert(!ComboSoloForestMm_PassageAvailable()); sForestSlot = 0;
    gSaveContext.fileNum = 1; assert(!ComboSoloForestMm_PassageAvailable()); gSaveContext.fileNum = 0;
    gSaveContext.gameMode = GAMEMODE_TITLE_SCREEN; assert(!ComboSoloForestMm_PassageAvailable());
    gSaveContext.gameMode = GAMEMODE_NORMAL;
    gComboGameMode = COMBO_GAME_BOTH; assert(!ComboSoloForestMm_PassageAvailable());
    gComboGameMode = COMBO_GAME_OOT; assert(!ComboSoloForestMm_PassageAvailable());
#endif
#ifdef OOT_FOREST_TEST
    Reset(); RegisterSoloForest();
    sForestOrigin.pos = { 800, 0, 175 };
    const auto beforeInit = sForestOrigin;
    const auto slotBeforeInit = sForestSlot;
    const auto seedBeforeInit = sForestSeed;
    const int waitsBeforeInit = waits;
    saveMutexHeld = true;
    manager.init(false); // real registered init callback executes under native saveMtx
    saveMutexHeld = false;
    assert(waits == waitsBeforeInit && sForestOrigin.pos.z == beforeInit.pos.z);
    assert(sForestSlot == slotBeforeInit && sForestSeed == seedBeforeInit);
    nativeForestDisk = { { "version", 1 }, { "seed", 77 }, { "savedInside", true },
        { "origin", { { "entrance", ENTR_LOST_WOODS_SOUTH_EXIT }, { "room", 1 },
            { "x", 800 }, { "y", 0 }, { "z", 170 }, { "yaw", -32768 } } } };
    manager.load();
    interactor.onLoad(0); // publication occurs after LoadFile releases saveMtx
    assert(waits > waitsBeforeInit && sForestResume && sForestOrigin.pos.z == 170);
    Entrance() = ENTR_TEMPLE_OF_TIME_WARP_PAD; // later unordered entrance-rando hook
    gSaveContext.respawn[RESPAWN_MODE_RETURN].pos = { 55, 66, 77 }; // later Remember Location hook
    const auto laterReturn = gSaveContext.respawn[RESPAWN_MODE_RETURN];
    NormalizeForestEntry();
    assert(Entrance() == kForestEntrance && !sForestResume && gSaveContext.respawnFlag == 0);
    assert(!std::memcmp(&laterReturn, &gSaveContext.respawn[RESPAWN_MODE_RETURN], sizeof(laterReturn)));
    sForestOrigin.pos.z = 195; // active entry publication, no worker in flight in this fixture
    const int waitsBeforeNotification = waits;
    interactor.onLoad(0); // age/warp notifications are not a file reload
    assert(sForestOrigin.pos.z == 195 && !sForestResume && waits == waitsBeforeNotification);
    manager.init(false); manager.load(); interactor.onLoad(0); // actual same-slot reload binds again
    assert(sForestOrigin.pos.z == 170 && sForestResume && waits > waitsBeforeNotification);
#endif
    Reset(); Approach();
    const auto publishedOrigin = sForestOrigin;
#ifdef OOT_FOREST_TEST
    player.actor.world.pos = { 800, 0, 190 };
#else
    player.actor.world.pos = { 20, 0, -20 };
#endif
    ForestPlayerUpdate();
    assert(sForestOrigin.pos.x == publishedOrigin.pos.x && sForestOrigin.pos.z == publishedOrigin.pos.z);
    const auto groundedCandidate = player.actor.world.pos;
    Enter();
    assert(sForestOrigin.pos.x == groundedCandidate.x && sForestOrigin.pos.z == groundedCandidate.z);
#ifdef OOT_FOREST_TEST
    assert(originAtWait.pos.x == publishedOrigin.pos.x && originAtWait.pos.z == publishedOrigin.pos.z);
#endif
    Reset(); Approach(); const auto safe = player.actor.world.pos;
    const auto top = gSaveContext.respawn[RESPAWN_MODE_TOP];
    const auto protectedSave = gSaveContext;
    Enter();
    assert(NextEntrance() == kForestEntrance && play.transitionTrigger == TRANS_TRIGGER_START);
    assert(sForestOrigin.pos.x == safe.x && sForestOrigin.pos.y == 0 && sForestOrigin.pos.z == safe.z);
#ifdef OOT_FOREST_TEST
    assert(originAtWait.pos.y == 0 && waits > 0);
#endif
    assert(!std::memcmp(&top, &gSaveContext.respawn[RESPAWN_MODE_TOP], sizeof(top)));
    play.transitionTrigger = TRANS_TRIGGER_OFF; SetScene(kForestScene, 0); Entrance() = kForestEntrance;
    player.actor.world.pos = { -486, 0, -227 }; ForestPlayerUpdate();
    assert(ForestVisit(&play) && play.transitionTrigger == TRANS_TRIGGER_OFF);
    player.actor.world.pos = { -695, 0, -313 }; ForestPlayerUpdate();
    assert(sLocalReturning && NextEntrance() == sForestOrigin.entrance);
    Entrance() = NextEntrance(); NormalizeForestEntry();
    assert(sOriginStaged && gSaveContext.respawnFlag == 2);
    assert(gSaveContext.respawn[RESPAWN_MODE_RETURN].roomIndex == 1);
    assert(!std::memcmp(&top, &gSaveContext.respawn[RESPAWN_MODE_TOP], sizeof(top)));
    Reset(); Approach(); Enter();
    play.transitionTrigger = TRANS_TRIGGER_OFF; SetScene(kForestScene, 1); Entrance() = kForestEntrance;
    Capture(); assert(persisted["savedInside"] == true);
    const auto saved = persisted;
    Reset(); LoadForestState(saved, 0); gSaveContext.fileNum = 1; Entrance() = 0;
    NormalizeForestEntry(); assert(Entrance() == 0 && gSaveContext.respawnFlag == 0);
    Reset(); LoadForestState(saved, 0); sForestSeed = 78; Entrance() = 0;
    NormalizeForestEntry(); assert(Entrance() == 0 && gSaveContext.respawnFlag == 0);
    Reset(); LoadForestState(saved, 0);
    Entrance() = 0; gSaveContext.respawn[RESPAWN_MODE_RETURN].pos = { 55, 66, 77 };
    // Later native load/rando hooks may choose another entrance/respawn.
    NormalizeForestEntry();
    assert(Entrance() == kForestEntrance && gSaveContext.respawnFlag == 0 && CurrentCutscene() == 0);
    Reset(); LoadForestState(saved, 0); pack = false;
    NormalizeForestEntry(); assert(Entrance() == sForestOrigin.entrance && sOriginStaged);
    Reset(); Entrance() = kForestEntrance; gSaveContext.respawnFlag = 1;
    NormalizeForestEntry(); assert(Entrance() == kForestEntrance && gSaveContext.respawnFlag == 0);
    const int variants =
#ifdef OOT_FOREST_TEST
        4;
#else
        1;
#endif
    for (int variant = 0; variant < variants; ++variant) {
        Reset(); Entrance() = kForestEntrance + variant; gSaveContext.respawnFlag = 2;
        auto& arrival = gSaveContext.respawn[RESPAWN_MODE_RETURN];
#ifdef OOT_FOREST_TEST
        arrival.entranceIndex = kForestEntrance + variant;
#else
        arrival.entrance = kForestEntrance;
#endif
        arrival.roomIndex = 0; arrival.pos = { 184, 0, -300 };
        const auto shopReturn = arrival;
        NormalizeForestEntry(); assert(gSaveContext.respawnFlag == 2);
        assert(!std::memcmp(&arrival, &shopReturn, sizeof(arrival)));
    }
#ifdef OOT_FOREST_TEST
    Reset(); Entrance() = COMBO_SOLO_FOREST_OOT_EXIT;
    NormalizeForestEntry(); assert(Entrance() == sForestOrigin.entrance && sOriginStaged);
    Reset(); Entrance() = kForestEntrance; gPlayState = nullptr;
    Capture(); assert(persisted["savedInside"] == true); // worker snapshot only
#endif
    for (int gate = 0; gate < 7; ++gate) {
        Reset(); Approach();
        if (gate == 0) pack = false;
        if (gate == 1) play.pauseCtx.state = 1;
        if (gate == 2) inCs = true;
        if (gate == 3) gComboGameMode = COMBO_GAME_BOTH;
        if (gate == 4) gSaveContext.fileNum = 255;
        if (gate == 5) play.roomCtx.curRoom.num = 0;
        if (gate == 6) sForestSeed = 78;
        Enter(); assert(play.transitionTrigger == TRANS_TRIGGER_OFF);
    }
    Reset(); SetScene(kForestScene, 0); Entrance()=kForestEntrance; sForestArmed=true;
    alignas(Gfx) static const char portalRoot[]="__OTR__custom/comboship/native_forest/portal_root";
#ifdef OOT_FOREST_TEST
    MeshHeader mesh{}; PolygonDlist2 entry{};
    mesh.base.type=2; mesh.polygon2.num=1; mesh.polygon2.start=&entry;
    play.roomCtx.curRoom.meshHeader=&mesh;
#else
    RoomShape mesh{}; RoomShapeCullableEntry entry{};
    mesh.base.type=ROOM_SHAPE_TYPE_CULLABLE; mesh.cullable.numEntries=1; mesh.cullable.entries=&entry;
    play.roomCtx.curRoom.roomShape=&mesh;
#endif
    entry.opa=(Gfx*)portalRoot;
    const float angle=12288*(6.28318530718f/65536.0f);
    player.actor.world.pos={-695+52*std::cos(angle)-20*std::sin(angle),0,
                            -313-52*std::sin(angle)-20*std::cos(angle)};
    ForestPlayerUpdate();
    assert(play.transitionTrigger==TRANS_TRIGGER_OFF && "native jamb must not teleport outside its 44-unit mouth");
    player.actor.world.pos={-695,0,-313}; ForestPlayerUpdate();
    assert(play.transitionTrigger==TRANS_TRIGGER_START);
    std::puts("PASS native solo forest route, grounded origin, Clock/stump return, reload/void/missing-pack recovery and isolation");
}
