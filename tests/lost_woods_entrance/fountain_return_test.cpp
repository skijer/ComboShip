#include "global.h"
#include "ComboGameMode.h"
#include "entrances/LostWoodsEntrance.h"
#include "gameplay/CrossGameTitleCard.h"
#include "soh/Enhancements/game-interactor/vanilla-behavior/GIVanillaBehavior.h"
#include <nlohmann/json.hpp>
#include <array>
#include <cassert>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <functional>
#include <type_traits>

SaveContext gSaveContext{};
PlayState* gPlayState;
/* PRODUCTION_RETURN_SLOT */
EntranceInfo gEntranceTable[ENTR_MAX];
extern "C" { int gComboGameMode = COMBO_GAME_BOTH; }
bool isRando = false, heldSaveMutex = false, inCutscene = false;
u32 seed = 77;
#undef IS_RANDO
#define IS_RANDO isRando
namespace Rando {
struct Context {
    static Context* GetInstance() { static Context value; return &value; }
    u32 GetSeed() { return seed; }
};
}
int shuffled = 0, dynamicShuffled = 0;
extern "C" s32 Player_InCsMode(PlayState*) { return inCutscene; }
extern "C" u32 SurfaceType_GetSceneExitIndex(CollisionContext*, CollisionPoly* poly, s32) { return poly->type; }
extern "C" u32 SurfaceType_GetFloorEffect(CollisionContext*, CollisionPoly*, s32) { return 0; }
extern "C" void Scene_SetTransitionForNextEntrance(PlayState* play) { play->transitionType = TRANS_TYPE_FADE_BLACK; }
extern "C" void Play_TriggerVoidOut(PlayState*) { assert(false && "authored fountain routes must not void out"); }
bool GameInteractor_Should(GIVanillaBehavior, bool result, ...) { return result; }
s16 Entrance_OverrideNextIndex(s16 entrance) {
    ++shuffled;
    return entrance < ENTR_MAX ? ENTR_HAPPY_MASK_SHOP_0 : entrance;
}
s16 Entrance_OverrideDynamicExit(s16) { ++dynamicShuffled; return ENTR_MARKET_DAY_4; }

struct SaveManager {
    static SaveManager* Instance;
    std::function<void(bool)> init;
    std::function<void()> load;
    std::function<void(const SaveContext&, int, bool)> save;
    nlohmann::json disk, written;
    void ThreadPoolWait() { assert(!heldSaveMutex && "save callback must not wait while holding saveMtx"); }
    void AddInitFunction(std::function<void(bool)> callback) { init = callback; }
    void AddLoadFunction(const std::string&, int, std::function<void()> callback) { load = callback; }
    void AddSaveFunction(const std::string&, int, decltype(save) callback, bool, int) { save = callback; }
    void SaveData(const std::string&, const nlohmann::json& value) { written = value; }
    void LoadData(const std::string&, nlohmann::json& value) { value = disk; }
};
SaveManager manager;
SaveManager* SaveManager::Instance = &manager;
constexpr int SECTION_PARENT_NONE = -1;
struct GameInteractor {
    struct OnLoadGame {};
    struct OnSceneInit {};
    struct OnPlayerUpdate {};
    struct OnPlayDestroy {};
    struct OnExitGame {};
    static GameInteractor* Instance;
    std::function<void(int)> onLoad, onScene, onExit;
    std::function<void()> onUpdate, onDestroy;
    template<class Hook, class Callback> void RegisterGameHook(Callback callback) {
        if constexpr (std::is_same_v<Hook, OnLoadGame>) onLoad = callback;
        if constexpr (std::is_same_v<Hook, OnSceneInit>) onScene = callback;
        if constexpr (std::is_same_v<Hook, OnExitGame>) onExit = callback;
        if constexpr (std::is_same_v<Hook, OnPlayerUpdate>) onUpdate = callback;
        if constexpr (std::is_same_v<Hook, OnPlayDestroy>) onDestroy = callback;
    }
};
GameInteractor interactor;
GameInteractor* GameInteractor::Instance = &interactor;
struct RegisterShipInitFunc { explicit RegisterShipInitFunc(void (*callback)()) { callback(); } };

/* PRODUCTION_REQUEST_STATE */
/* PRODUCTION_ROUTE */
/* NATIVE_RETURN_TABLES */

static Player player;
static PlayState play;
static CollisionPoly floorPoly;
void Require(bool condition, const char* message) {
    if (!condition) { std::fprintf(stderr, "FAIL %s\n", message); std::exit(1); }
}
void Normalize() {
#ifdef HAS_FOUNTAIN_ROUTE
    ComboLostWoodsFountain_NormalizeEntry();
#endif
}
void Tick() { if (interactor.onUpdate) interactor.onUpdate(); }
void LoadFile(const nlohmann::json& document = nullptr, int slot = 0) {
    manager.disk = document;
    heldSaveMutex = true;
    if (manager.init) manager.init(false);
    if (document.is_object() && manager.load) manager.load();
    heldSaveMutex = false;
    gSaveContext.fileNum = slot;
    if (interactor.onLoad) interactor.onLoad(slot);
}
void Reset(bool randomized = false) {
    play = {}; player = {}; floorPoly = {}; gSaveContext = {};
    gPlayState = &play; play.state.running = true;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
    floorPoly.normal.y = 32767;
    player.actor.floorPoly = &floorPoly;
    player.actor.bgCheckFlags = BGCHECKFLAG_GROUND;
    gSaveContext.gameMode = GAMEMODE_NORMAL;
    gSaveContext.health = 48; gSaveContext.rupees = 321;
    std::memset(gSaveContext.respawn, 0x35, sizeof(gSaveContext.respawn));
    gSaveContext.respawnFlag = 0;
    seed = 77; isRando = randomized; inCutscene = false; shuffled = dynamicShuffled = 0;
    gComboGameMode = COMBO_GAME_BOTH;
    gComboReturnFileNum = -1;
    LoadFile();
}
void Scene(int scene, int entrance, int room = 0, int spawn = 0) {
    if (interactor.onDestroy) interactor.onDestroy();
    gSaveContext.entranceIndex = entrance;
    Normalize();
    play.sceneNum = scene; play.curSpawn = spawn; play.roomCtx.curRoom.num = room;
    play.transitionMode = TRANS_MODE_OFF; play.transitionTrigger = TRANS_TRIGGER_OFF;
    if (interactor.onScene) interactor.onScene(scene);
    // Play_Init consumes the staged respawn before the first player update.
    gSaveContext.respawnFlag = 0;
}
void Ground(float x, float z, int room = 4, s16 yaw = 0) {
    play.roomCtx.curRoom.num = room;
    player.actor.world.pos = {x, 0, z};
    player.actor.floorHeight = 0; player.actor.shape.rot.y = yaw;
    floorPoly.type = 0;
    Tick();
}
void Resolve(int entrance) {
    s16 exits[] = {static_cast<s16>(entrance)};
    play.setupExitList = exits;
    Player* current = &player;
    PlayState* play = &::play;
    CollisionPoly* poly = &floorPoly;
    const int exitIndex = 1, bgId = BGCHECK_SCENE;
    /* NATIVE_COLLISION_EXIT */
    play->transitionTrigger = TRANS_TRIGGER_START;
}
void Enter(int fountain = ENTR_GREAT_FAIRYS_FOUNTAIN_MAGIC_DMT, int room = 4) {
    Scene(SCENE_LOST_WOODS, ENTR_LOST_WOODS_SOUTH_EXIT, room);
    Ground(-1200, 800, room);
    Ground(-1200, 840, room);
    Ground(-1200, 880, room);
    player.actor.world.pos = {-1200, 0, 920}; floorPoly.type = 11;
    Resolve(fountain);
    Require(play.nextEntranceIndex == fountain, "custom Lost Woods entry must enter its authored fountain");
    Require(shuffled == 0, "custom fountain entry must bypass entrance shuffle");
    Scene(gEntranceTable[fountain].scene, fountain, 0, gEntranceTable[fountain].spawn);
}
int main() {
    // Use the real entrance table, including native fountain spawns and variants.
#define DEFINE_ENTRANCE(name, sceneValue, spawnValue, ...) \
    gEntranceTable[name].scene = sceneValue; gEntranceTable[name].spawn = spawnValue;
#include "tables/entrance_table.h"
#undef DEFINE_ENTRANCE

    // The unmodified room-7 north floor uses exit-list slot 0 (Kokiri),
    // not the authored archive's additional Fairy exit. Exercise that input.
    for (const auto mode : {COMBO_GAME_BOTH, COMBO_GAME_OOT}) {
        for (const bool randomized : {false, true}) {
            for (const int age : {LINK_AGE_CHILD, LINK_AGE_ADULT}) {
                Reset(randomized); gComboGameMode = mode; gSaveContext.linkAge = age;
                Scene(SCENE_LOST_WOODS, ENTR_LOST_WOODS_SOUTH_EXIT, 7);
                Ground(1600, -2700, 7, static_cast<s16>(0x8000));
                Ground(1600, -2740, 7, static_cast<s16>(0x8000));
                Ground(1600, -2780, 7, static_cast<s16>(0x8000));
                player.actor.world.pos = {1600, 0, -2820}; floorPoly.type = 1;
                Resolve(ENTR_KOKIRI_FOREST_UPPER_EXIT);
                Require(play.nextEntranceIndex == ENTR_GREAT_FAIRYS_FOUNTAIN_MAGIC_DMT && shuffled == 0,
                        "unmodified room-7 straight tunnel must reach the Fairy without any scene archive");
                Require(Combo_SOH_ConsumeCrossGameTitleCard(),
                        "a claimed custom fountain entrance must request its native destination card");
                Scene(SCENE_GREAT_FAIRYS_FOUNTAIN_MAGIC, play.nextEntranceIndex);
                Resolve(ENTR_RETURN_DAIYOUSEI_IZUMI);
                const auto& local = gSaveContext.respawn[RESPAWN_MODE_RETURN];
                Require(play.nextEntranceIndex == ENTR_LOST_WOODS_SOUTH_EXIT && local.roomIndex == 7 &&
                            local.pos.x == 1600 && local.pos.z > -2800 && local.pos.z <= -2600 && local.yaw == 0,
                        "native tunnel must return to a safe approach outside its own exit floor");
                Require(shuffled == 0 && dynamicShuffled == 0,
                        "native Fairy round trip must stay fixed with entrance shuffle enabled");
                Require(Combo_SOH_ConsumeCrossGameTitleCard() && !Combo_SOH_ConsumeCrossGameTitleCard(),
                        "the positioned fountain return must request one Lost Woods destination card");
            }
        }
    }
    // Debug warps or a first update directly on the exit floor have no sampled
    // approach. The native mesh still provides a known safe room-7 return.
    Reset(true); Scene(SCENE_LOST_WOODS, ENTR_LOST_WOODS_SOUTH_EXIT, 7);
    player.actor.world.pos = {1600, 0, -3100}; floorPoly.type = 1;
    Resolve(ENTR_KOKIRI_FOREST_UPPER_EXIT);
    Require(play.nextEntranceIndex == ENTR_GREAT_FAIRYS_FOUNTAIN_MAGIC_DMT,
            "native Fairy entry must not depend on prior sampled frames");
    Scene(SCENE_GREAT_FAIRYS_FOUNTAIN_MAGIC, play.nextEntranceIndex);
    manager.save(gSaveContext, 0, false);
    LoadFile(nlohmann::json::parse(manager.written.dump()));
    gSaveContext.entranceIndex = ENTR_LINKS_HOUSE_CHILD_SPAWN; Normalize();
    Require(gSaveContext.entranceIndex == ENTR_GREAT_FAIRYS_FOUNTAIN_MAGIC_DMT,
            "saving inside the native tunnel's fountain must resume the same visit");
    Scene(SCENE_GREAT_FAIRYS_FOUNTAIN_MAGIC, gSaveContext.entranceIndex);
    Resolve(ENTR_RETURN_DAIYOUSEI_IZUMI);
    const auto& nativeReturn = gSaveContext.respawn[RESPAWN_MODE_RETURN];
    Require(nativeReturn.roomIndex == 7 && nativeReturn.pos.x == 1600 && nativeReturn.pos.y == 0 &&
                nativeReturn.pos.z == -2760 && nativeReturn.yaw == 0,
            "unsampled native entry must return on the known dry floor facing south into room 7");

    for (const auto& position : std::array<Vec3f, 4>{Vec3f{2020, 0, -2400}, Vec3f{1180, 0, -2400},
                                                    Vec3f{1600, 0, -1980}, Vec3f{1600, 200, -2820}}) {
        Reset(); Scene(SCENE_LOST_WOODS, ENTR_LOST_WOODS_SOUTH_EXIT, 7);
        player.actor.world.pos = position; Resolve(ENTR_KOKIRI_FOREST_UPPER_EXIT);
        Require(play.nextEntranceIndex == ENTR_KOKIRI_FOREST_UPPER_EXIT,
                "the Woodfall, left, south and upper paths must not become Fairy entrances");
    }
    for (const int room : {0, 1, 6, 8}) {
        Reset(); Scene(SCENE_LOST_WOODS, ENTR_LOST_WOODS_SOUTH_EXIT, room);
        player.actor.world.pos = {1600, 0, -2820}; Resolve(ENTR_KOKIRI_FOREST_UPPER_EXIT);
        Require(play.nextEntranceIndex == ENTR_KOKIRI_FOREST_UPPER_EXIT,
                "only room 7 may claim the native Fairy exit");
    }
    Reset(); Scene(SCENE_LOST_WOODS, ENTR_LOST_WOODS_SOUTH_EXIT, 7);
    player.actor.world.pos = {1600, 0, -2820}; Resolve(ENTR_HYRULE_FIELD_CENTER_EXIT);
    Require(play.nextEntranceIndex == ENTR_HYRULE_FIELD_CENTER_EXIT,
            "an archive's explicit non-Kokiri exit must keep its authored destination");
    Reset(true); gComboGameMode = COMBO_GAME_MM;
    Scene(SCENE_LOST_WOODS, ENTR_LOST_WOODS_SOUTH_EXIT, 7);
    player.actor.world.pos = {1600, 0, -2820}; Resolve(ENTR_KOKIRI_FOREST_UPPER_EXIT);
    Require(play.nextEntranceIndex == ENTR_HAPPY_MASK_SHOP_0 && shuffled == 1,
            "the OoT native tunnel controller must be inactive in MM-only mode");

    Reset(true); Enter();
    const auto returnBefore = gSaveContext.respawn[RESPAWN_MODE_RETURN];
    const auto topBefore = gSaveContext.respawn[RESPAWN_MODE_TOP];
    const auto downBefore = gSaveContext.respawn[RESPAWN_MODE_DOWN];
    Resolve(ENTR_RETURN_DAIYOUSEI_IZUMI);
    Require(play.nextEntranceIndex == ENTR_LOST_WOODS_SOUTH_EXIT,
            "fountain reached through Lost Woods must return to the captured Lost Woods tunnel");
    const auto arrival = gSaveContext.respawn[RESPAWN_MODE_RETURN];
    Require(gSaveContext.respawnFlag == 2 && arrival.roomIndex == 4 && arrival.pos.x == -1200 &&
                arrival.pos.z <= 880 && arrival.pos.z >= 800,
            "return must use a grounded approach point in the relocated tunnel's room");
    Require(arrival.yaw == static_cast<s16>(0x8000) && arrival.playerParams == ((PLAYER_START_MODE_IDLE << 8) | 0xFF),
            "return must face out of the tunnel and start idle");
    Require(shuffled == 0 && dynamicShuffled == 0, "local return must bypass ordinary/randomized fairy exits");
    Require(!std::memcmp(&topBefore, &gSaveContext.respawn[RESPAWN_MODE_TOP], sizeof(topBefore)) &&
                !std::memcmp(&downBefore, &gSaveContext.respawn[RESPAWN_MODE_DOWN], sizeof(downBefore)),
            "local return must preserve Farore and void respawn points");
    Require(gSaveContext.health == 48 && gSaveContext.rupees == 321, "travel must preserve player progress");
    Scene(SCENE_LOST_WOODS, play.nextEntranceIndex, arrival.roomIndex); Tick();
    Require(!std::memcmp(&returnBefore, &gSaveContext.respawn[RESPAWN_MODE_RETURN], sizeof(returnBefore)),
            "temporary arrival respawn must be restored after spawning");
    Scene(SCENE_GREAT_FAIRYS_FOUNTAIN_MAGIC, ENTR_GREAT_FAIRYS_FOUNTAIN_MAGIC_DMT);
    Resolve(ENTR_RETURN_DAIYOUSEI_IZUMI);
    Require(play.nextEntranceIndex == ENTR_MARKET_DAY_4 && dynamicShuffled == 1,
            "later normal visit must retain its randomized exit");

    for (const auto mode : {COMBO_GAME_BOTH, COMBO_GAME_OOT}) {
        for (const int fountain : {ENTR_GREAT_FAIRYS_FOUNTAIN_MAGIC_DMT, ENTR_GREAT_FAIRYS_FOUNTAIN_MAGIC_DMC,
                                  ENTR_GREAT_FAIRYS_FOUNTAIN_MAGIC_OGC_DD, ENTR_GREAT_FAIRYS_FOUNTAIN_SPELLS_FARORES_ZF,
                                  ENTR_GREAT_FAIRYS_FOUNTAIN_SPELLS_DINS_HC, ENTR_GREAT_FAIRYS_FOUNTAIN_SPELLS_NAYRUS_COLOSSUS}) {
            Reset(); gComboGameMode = mode; Enter(fountain, 7);
            const bool spells = play.sceneNum == SCENE_GREAT_FAIRYS_FOUNTAIN_SPELLS;
            Resolve(spells ? ENTR_RETURN_YOUSEI_IZUMI_YOKO : ENTR_RETURN_DAIYOUSEI_IZUMI);
            Require(play.nextEntranceIndex == ENTR_LOST_WOODS_SOUTH_EXIT &&
                        gSaveContext.respawn[RESPAWN_MODE_RETURN].roomIndex == 7,
                    "all native Great Fairy variants must support a moved Lost Woods tunnel in both OoT modes");
        }
    }
    Reset(); Enter();
    Scene(SCENE_HYRULE_FIELD, ENTR_HYRULE_FIELD_CENTER_EXIT);
    Scene(SCENE_GREAT_FAIRYS_FOUNTAIN_MAGIC, ENTR_GREAT_FAIRYS_FOUNTAIN_MAGIC_DMT);
    Resolve(ENTR_RETURN_DAIYOUSEI_IZUMI);
    Require(play.nextEntranceIndex == ENTR_DEATH_MOUNTAIN_TRAIL_GREAT_FAIRY_EXIT,
            "warping away must clear the local return before a normal visit");

    Reset(); Enter();
    if (manager.save) manager.save(gSaveContext, 0, false);
    const auto savedInside = manager.written;
    LoadFile(nlohmann::json::parse(savedInside.dump()));
    gSaveContext.entranceIndex = ENTR_LINKS_HOUSE_CHILD_SPAWN; Normalize();
    Require(gSaveContext.entranceIndex == ENTR_GREAT_FAIRYS_FOUNTAIN_MAGIC_DMT,
            "save/reload inside the fountain must resume this local visit");
    Scene(SCENE_GREAT_FAIRYS_FOUNTAIN_MAGIC, gSaveContext.entranceIndex);
    Resolve(ENTR_RETURN_DAIYOUSEI_IZUMI);
    Require(play.nextEntranceIndex == ENTR_LOST_WOODS_SOUTH_EXIT, "saved local visit must retain its return");
    // TitleSetup loads the paired save before composing an explicit portal,
    // dungeon/boss or Farore arrival, while this flag still names that slot.
    gComboReturnFileNum = 0;
    LoadFile(savedInside);
    gSaveContext.entranceIndex = ENTR_HYRULE_FIELD_CENTER_EXIT;
    gSaveContext.respawnFlag = 3;
    const auto explicitReturn = gSaveContext.respawn[RESPAWN_MODE_RETURN];
    gComboReturnFileNum = -1;
    Normalize();
    Require(gSaveContext.entranceIndex == ENTR_HYRULE_FIELD_CENTER_EXIT && gSaveContext.respawnFlag == 3 &&
                !std::memcmp(&explicitReturn, &gSaveContext.respawn[RESPAWN_MODE_RETURN], sizeof(explicitReturn)),
            "saved fountain metadata must not override an explicit combined-game or Farore arrival");
    Scene(SCENE_GREAT_FAIRYS_FOUNTAIN_MAGIC, ENTR_GREAT_FAIRYS_FOUNTAIN_MAGIC_DMT);
    Resolve(ENTR_RETURN_DAIYOUSEI_IZUMI);
    Require(play.nextEntranceIndex == ENTR_DEATH_MOUNTAIN_TRAIL_GREAT_FAIRY_EXIT,
            "explicit arrival into the same fountain must not inherit an older Lost Woods visit");
    LoadFile();
    Scene(SCENE_GREAT_FAIRYS_FOUNTAIN_MAGIC, ENTR_GREAT_FAIRYS_FOUNTAIN_MAGIC_DMT);
    Resolve(ENTR_RETURN_DAIYOUSEI_IZUMI);
    Require(play.nextEntranceIndex == ENTR_DEATH_MOUNTAIN_TRAIL_GREAT_FAIRY_EXIT, "another file must not inherit the route");

    Reset(true); Enter();
    manager.save(gSaveContext, 0, false);
    auto malformed = manager.written;
    malformed["origin"]["room"] = 1000;
    LoadFile(malformed);
    Scene(SCENE_GREAT_FAIRYS_FOUNTAIN_MAGIC, ENTR_GREAT_FAIRYS_FOUNTAIN_MAGIC_DMT);
    Resolve(ENTR_RETURN_DAIYOUSEI_IZUMI);
    Require(play.nextEntranceIndex == ENTR_MARKET_DAY_4, "malformed saved origin must not redirect a normal fountain");
    seed = 88; LoadFile(manager.written);
    Scene(SCENE_GREAT_FAIRYS_FOUNTAIN_MAGIC, ENTR_GREAT_FAIRYS_FOUNTAIN_MAGIC_DMT);
    Resolve(ENTR_RETURN_DAIYOUSEI_IZUMI);
    Require(play.nextEntranceIndex == ENTR_MARKET_DAY_4, "a different seed must not reuse the return marker");
    std::puts("PASS native room-7 Fairy round trip: no archive, both ages/modes, shuffle, unsampled entry and reload");
    std::puts("PASS conditional fountain return: relocated rooms, six fountains, native/rando exits and stale-state cleanup");
}
