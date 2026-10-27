#include "ComboGameMode.h"
#include "entrances/IkanaGravePlacement.h"
#include "entrances/RegionalPassages.h"
#include "global.h"
#include "objects/gameplay_keep/gameplay_keep.h"
#include "objects/object_haka/object_haka.h"
#include "overlays/actors/ovl_Bg_Haka/z_bg_haka.h"
#include "overlays/actors/ovl_Door_Ana/z_door_ana.h"
#include "soh/Enhancements/Gameplay/ComboRegionalPassages.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <type_traits>
#include <vector>
SaveContext gSaveContext{};
PlayState *gPlayState;
EntranceInfo gEntranceTable[ENTR_MAX]{};
extern "C" {
int gComboGameMode = COMBO_GAME_BOTH;
int ComboLakeHyliaBridge_Get(PlayState *, Vec3f *, s16 *);
int ComboFountainSnowheadBridge_Get(PlayState *, Vec3f *, s16 *);
}
#define CVAR_PREFIX_ENHANCEMENT "gEnhancements"
static u32 seed = 77;
static bool mq;
extern "C" u32 ResourceMgr_IsSceneMasterQuest(s16) { return mq; }
static bool heldSaveMutex, wet, exitFloor, dynamicFloor, missingWall,
    apronObstacle, lowCeiling, inCs;
#undef IS_RANDO
#define IS_RANDO 1
namespace Rando {
struct Context {
  static Context *GetInstance() {
    static Context c;
    return &c;
  }
  u32 GetSeed() { return seed; }
};
} // namespace Rando
struct SaveManager {
  static SaveManager *Instance;
  std::function<void(bool)> init;
  std::function<void()> load;
  std::function<void(const SaveContext &, int, bool)> save;
  nlohmann::json disk, written;
  void ThreadPoolWait() { assert(!heldSaveMutex); }
  void AddInitFunction(decltype(init) c) { init = c; }
  void AddLoadFunction(const std::string &, int, decltype(load) c) { load = c; }
  void AddSaveFunction(const std::string &, int, decltype(save) c, bool, int) {
    save = c;
  }
  void SaveData(const std::string &, const nlohmann::json &value) {
    written = value;
  }
  void LoadData(const std::string &, nlohmann::json &value) { value = disk; }
};
static SaveManager manager;
SaveManager *SaveManager::Instance = &manager;
constexpr int SECTION_PARENT_NONE = -1;
struct GameInteractor {
  struct OnLoadGame {};
  struct OnPlayerUpdate {};
  struct OnPlayDestroy {};
  struct OnExitGame {};
  static GameInteractor *Instance;
  std::function<void(int)> load, exit;
  std::function<void()> update, destroy;
  template <class Hook, class F> void RegisterGameHook(F f) {
    if constexpr (std::is_same_v<Hook, OnLoadGame>)
      load = f;
    if constexpr (std::is_same_v<Hook, OnPlayerUpdate>)
      update = f;
    if constexpr (std::is_same_v<Hook, OnPlayDestroy>)
      destroy = f;
    if constexpr (std::is_same_v<Hook, OnExitGame>)
      exit = f;
  }
};
static GameInteractor interactor;
GameInteractor *GameInteractor::Instance = &interactor;
struct RegisterShipInitFunc {
  explicit RegisterShipInitFunc(void (*f)()) { f(); }
};
struct ActorDBInit {
  std::string name, desc;
  int category = 0, objectId = 0;
  size_t instanceSize = 0;
  u32 flags = 0;
  ActorFunc init = nullptr, destroy = nullptr, update = nullptr, draw = nullptr;
};
struct ActorDB {
  struct Entry {
    struct {
      int id;
    } entry;
  };
  static ActorDB *Instance;
  std::vector<ActorDBInit> profiles;
  Entry entry{};
  Entry &AddEntry(const ActorDBInit &p) {
    entry.entry.id = 9000 + profiles.size();
    profiles.push_back(p);
    return entry;
  }
};
static ActorDB database;
ActorDB *ActorDB::Instance = &database;
static std::vector<Actor *> allocated;
static bool deferHoleInit;
static int collisionRegistrations, collisionDeletes, chimes, slides,
    titleRequests;
static CollisionPoly floorPoly{}, wallPoly{};
static CollisionHeader nativeGraveCollision{};
extern "C" {
void (*gComboSceneSwitchCallback)(int) = [](int) {};
s32 Player_InCsMode(PlayState *) { return inCs; }
f32 BgCheck_EntityRaycastFloor5(PlayState *, CollisionContext *,
                                CollisionPoly **poly, s32 *bg, Actor *,
                                Vec3f *) {
  floorPoly.normal.y = 32767;
  *poly = &floorPoly;
  *bg = dynamicFloor ? 0 : BGCHECK_SCENE;
  return 0;
}
u32 SurfaceType_GetSceneExitIndex(CollisionContext *, CollisionPoly *, s32) {
  return exitFloor;
}
s32 WaterBox_GetSurface1(PlayState *, CollisionContext *, f32, f32, f32 *y,
                         WaterBox **) {
  *y = wet ? 10 : -100;
  return true;
}
s32 BgCheck_EntityLineTest1(CollisionContext *, Vec3f *a, Vec3f *b, Vec3f *hit,
                            CollisionPoly **poly, s32 checkWall, s32 checkFloor,
                            s32 checkCeiling, s32 oneFace, s32 *bg) {
  if (checkFloor || checkCeiling) {
    assert(checkWall && checkFloor && checkCeiling && !oneFace);
    if (apronObstacle) {
      *poly = &wallPoly;
      *bg = 0;
      *hit = *a;
      return true;
    }
    if (lowCeiling && std::fmin(a->y, b->y) < 40 &&
        std::fmax(a->y, b->y) > 40) {
      *poly = &wallPoly;
      *bg = BGCHECK_SCENE;
      wallPoly.normal = {0, -32767, 0};
      *hit = *a;
      return true;
    }
  }
  if (missingWall || (a->z - 200) * (b->z - 200) >= 0)
    return false;
  const float t = (200 - a->z) / (b->z - a->z);
  *hit = {a->x + (b->x - a->x) * t, a->y, 200};
  wallPoly.normal = {0, 0, -32767};
  *poly = &wallPoly;
  *bg = BGCHECK_SCENE;
  return true;
}
void Actor_SetScale(Actor *a, f32 scale) { a->scale = {scale, scale, scale}; }
f32 Math_SinS(s16 yaw) { return std::sin(yaw * ComboRegional::kAngle); }
f32 Math_CosS(s16 yaw) { return std::cos(yaw * ComboRegional::kAngle); }
s32 Math_StepToF(f32 *value, f32 target, f32 step) {
  if (std::fabs(target - *value) <= step + .000001f) {
    *value = target;
    return true;
  }
  *value += target > *value ? step : -step;
  return false;
}
void Actor_PlaySfx_Flagged(Actor *, u16 sfx) {
  assert(sfx == NA_SE_EV_ROCK_SLIDE - SFX_FLAG);
  ++slides;
}
void Sfx_PlaySfxCentered(u16 sfx) {
  assert(sfx == NA_SE_SY_CORRECT_CHIME);
  ++chimes;
}
void Actor_ProcessInitChain(Actor *a, InitChainEntry *) {
  a->minVelocityY = 0;
  Actor_SetScale(a, .1f);
}
void DynaPolyActor_Init(DynaPolyActor *, s32 flags) {
  assert(flags == DPM_UNK);
}
void CollisionHeader_GetVirtual(void *resource, CollisionHeader **out) {
  assert(
      !std::strcmp(reinterpret_cast<const char *>(resource), gGravestoneCol));
  *out = &nativeGraveCollision;
}
s32 DynaPoly_SetBgActor(PlayState *, DynaCollisionContext *, Actor *,
                        CollisionHeader *col) {
  assert(col == &nativeGraveCollision);
  ++collisionRegistrations;
  return 23;
}
void DynaPoly_DeleteBgActor(PlayState *, DynaCollisionContext *, s32 bg) {
  assert(bg == 23);
  ++collisionDeletes;
}
void Actor_WorldToActorCoords(Actor *a, Vec3f *out, Vec3f *p) {
  const float x = p->x - a->world.pos.x, z = p->z - a->world.pos.z;
  const float si = Math_SinS(a->shape.rot.y), co = Math_CosS(a->shape.rot.y);
  *out = {x * co - z * si, p->y - a->world.pos.y, x * si + z * co};
}
s32 Collider_InitCylinder(PlayState *, ColliderCylinder *) { return 0; }
s32 Collider_SetCylinder(PlayState *, ColliderCylinder *, Actor *,
                         ColliderCylinderInit *) {
  return 0;
}
s32 Collider_DestroyCylinder(PlayState *, ColliderCylinder *) { return 0; }
void BgHaka_IdleClosed(BgHaka *, PlayState *);
void BgHaka_Pull(BgHaka *, PlayState *);
void BgHaka_IdleOpened(BgHaka *, PlayState *);
void BgHaka_IdleLockedClosed(BgHaka *, PlayState *);
void BgHaka_Update(Actor *, PlayState *);
s32 CVarGetInteger(const char *, s32 fallback) { return fallback; }
s32 Play_InCsMode(PlayState *) { return inCs; }
s32 Flags_GetSwitch(PlayState *, s32) { return false; }
void Message_StartTextbox(PlayState *, u16, Actor *) {}
void func_800F5ACC(u16) {}
#define GameInteractor_Should(hook, defaultValue, ...) (defaultValue)
void BgHaka_Draw(Actor *, PlayState *) {}
void DoorAna_WaitClosed(DoorAna *, PlayState *) {}
void DoorAna_WaitOpen(DoorAna *, PlayState *) {}
void DoorAna_Draw(Actor *, PlayState *) {}
void Combo_SOH_RequestCrossGameTitleCard(void) { ++titleRequests; }
void Actor_Kill(Actor *actor) { actor->update = nullptr; }
}
static void *Combo_ResolveSym(const char *, const char *) {
  return reinterpret_cast<void *>(1);
}
#define CollisionHeader_GetVirtual(resource, out)                              \
  CollisionHeader_GetVirtual(                                                  \
      const_cast<void *>(static_cast<const void *>(resource)), out)
/* NATIVE_ACTORS */
#undef CollisionHeader_GetVirtual
extern "C" Actor *Actor_Spawn(ActorContext *ctx, PlayState *play, s16 id, f32 x,
                              f32 y, f32 z, s16 rx, s16 ry, s16 rz,
                              s16 params) {
  if (id == ACTOR_EN_POH)
    return nullptr;
  assert(id >= 9000 && id < 9000 + database.profiles.size());
  const auto &p = database.profiles[id - 9000];
  auto *actor = static_cast<Actor *>(std::calloc(1, p.instanceSize));
  allocated.push_back(actor);
  actor->id = id;
  actor->room = play->roomCtx.curRoom.num;
  actor->params = params;
  actor->world.pos = actor->home.pos = {x, y, z};
  actor->world.rot = actor->home.rot = actor->shape.rot = {rx, ry, rz};
  actor->init = p.init;
  actor->update = p.update;
  actor->destroy = p.destroy;
  actor->draw = p.draw;
  actor->next = ctx->actorLists[p.category].head;
  ctx->actorLists[p.category].head = actor;
  if (!deferHoleInit) {
    p.init(actor, play);
    actor->init = nullptr;
  }
  return actor;
}
static int jumpCalls, cutsceneId, cutsceneDuration;
static float jumpVelocity, sWaterSpeedFactor;
static const char *jumpAnimation;
void Player_Action_8084F9C0(Player *, PlayState *) {}
s32 Player_SetupAction(PlayState *, Player *p, PlayerActionFunc action, s32) {
  p->actionFunc = action;
  return 1;
}
void func_808389E8(Player *, const void *animation, float velocity,
                   PlayState *) {
  ++jumpCalls;
  jumpAnimation = static_cast<const char *>(animation);
  jumpVelocity = velocity;
}
extern "C" s16 OnePointCutscene_Init(PlayState *, s16 id, s16 duration, Actor *,
                                     s16 camera) {
  cutsceneId = id;
  cutsceneDuration = duration;
  assert(camera == CAM_ID_MAIN);
  return 0;
}
/* NATIVE_GROTTO */
/* PRODUCTION */
static int lakeCalls, fountainCalls;
extern "C" int ComboLakeHyliaBridge_Get(PlayState *, Vec3f *p, s16 *yaw) {
  ++lakeCalls;
  if (p)
    *p = {100, 0, 200};
  if (yaw)
    *yaw = 0;
  return 1;
}
extern "C" int ComboFountainSnowheadBridge_Get(PlayState *, Vec3f *p,
                                               s16 *yaw) {
  ++fountainCalls;
  if (p)
    *p = {ComboFountainSnowhead::kMouth.x, ComboFountainSnowhead::kMouth.y,
          ComboFountainSnowhead::kMouth.z};
  if (yaw)
    *yaw = ComboFountainSnowhead::kYaw;
  return 1;
}
namespace SOH {
struct ISceneCommand {};
struct SetEntranceList : ISceneCommand {
  unsigned numEntrances;
  EntranceEntry *values;
  void *GetRawPointer() { return values; }
};
struct SetStartPositionList : ISceneCommand {
  unsigned numStartPositions;
  ActorEntry *values;
  void *GetRawPointer() { return values; }
};
} // namespace SOH
extern "C" {
s16 gLinkObjectIds[2]{};
s32 Object_Spawn(ObjectContext *, s16) { return 0; }
void Math_Vec3f_Copy(Vec3f *dest, Vec3f *source) { *dest = *source; }
}
/* NATIVE_SCENE_COMMANDS */
static void NativePlayerInitRespawn(PlayState *play, Player *self) {
  Actor *thisx = &self->actor;
  s32 respawnFlag = 0, respawnMode = 0;
  /* NATIVE_RESPAWN */
  if (PLAYER_GET_START_MODE(thisx) == PLAYER_START_MODE_GROTTO)
    Player_StartMode_Grotto(play, self);
  self->actor.init = nullptr;
  // Native Play_Init clears this only after Actor_InitContext / Player_Init.
  gSaveContext.respawnFlag = 0;
}
static PlayState play;
static Player player;
static BgHaka original;
static DoorAna originalHole;
static BgHaka neighbor;
static ActorEntry entries[5]{};
static EntranceEntry entrances[5]{};
static CollisionHeader collision;
static void Noop(Actor *, PlayState *) {}
static void LoadFile(const nlohmann::json &disk = nullptr, int slot = 0) {
  manager.disk = disk;
  heldSaveMutex = true;
  manager.init(false);
  if (disk.is_object())
    manager.load();
  heldSaveMutex = false;
  gSaveContext.fileNum = slot;
  interactor.load(slot);
}
static void Scene(int scene, bool deferGraveInit = false) {
  for (auto *actor : allocated) {
    if (actor->destroy)
      actor->destroy(actor, &play);
    std::free(actor);
  }
  allocated.clear();
  play = {};
  player = {};
  original = {};
  originalHole = {};
  gPlayState = &play;
  play.state.running = true;
  play.sceneNum = scene;
  play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
  play.colCtx.colHeader = &collision;
  play.setupEntranceList = entrances;
  for (int i = 0; i < 5; ++i) {
    entrances[i] = {};
    entrances[i].spawn = i;
    entries[i] = {};
  }
  if (scene == SCENE_GRAVEYARD) {
    play.roomCtx.curRoom.num = ComboRegional::IkanaGrave::kRoom;
    entrances[0].room = ComboRegional::IkanaGrave::kRoom;
    original.dyna.actor.home.pos =
        original.dyna.actor.world.pos = {22, 120, -336};
    original.dyna.actor.id = ACTOR_BG_HAKA;
    original.dyna.actor.update = BgHaka_Update;
    original.dyna.actor.draw = BgHaka_Draw;
    original.dyna.actor.room = ComboRegional::IkanaGrave::kRoom;
    original.dyna.actor.init = BgHaka_Init;
    if (!deferGraveInit) {
      BgHaka_Init(&original.dyna.actor, &play);
      original.dyna.actor.init = nullptr;
    }
    neighbor = {};
    neighbor.dyna.actor.id = ACTOR_BG_HAKA;
    neighbor.dyna.actor.home.pos = {-98, 120, -336};
    neighbor.dyna.actor.update = Noop;
    neighbor.dyna.actor.room = ComboRegional::IkanaGrave::kRoom;
    original.dyna.actor.next = &neighbor.dyna.actor;
    originalHole.actor.id = ACTOR_DOOR_ANA;
    originalHole.actor.update = Noop;
    play.actorCtx.actorLists[ACTORCAT_BG].head = &original.dyna.actor;
    play.actorCtx.actorLists[ACTORCAT_ITEMACTION].head = &originalHole.actor;
  }
  SOH::SetEntranceList entranceCommand{{}, 5, entrances};
  SOH::SetStartPositionList spawnCommand{{}, 5, entries};
  Scene_CommandEntranceList(&play, &entranceCommand);
  Scene_CommandSpawnList(&play, &spawnCommand);
}
int main() {
  gEntranceTable[ENTR_GRAVEYARD_ENTRANCE].scene = SCENE_GRAVEYARD;
  gEntranceTable[ENTR_LAKE_HYLIA_OUTSIDE_LAB].scene = SCENE_LAKE_HYLIA;
  gEntranceTable[ENTR_ZORAS_FOUNTAIN_OUTSIDE_ICE_CAVERN].scene =
      SCENE_ZORAS_FOUNTAIN;
  gSaveContext.gameMode = GAMEMODE_NORMAL;
  gSaveContext.health = 48;
  gSaveContext.rupees = 321;
  LoadFile();
  Vec3f mouth{};
  int route;
  s16 yaw;
  for (int age : {0, 1})
    for (bool day : {false, true}) {
      gSaveContext.linkAge = age;
      gSaveContext.dayTime = day ? 0x8000 : 0;
      gSaveContext.nightFlag = !day;
      gSaveContext.entranceIndex = ENTR_GRAVEYARD_ENTRANCE;
      Scene(SCENE_GRAVEYARD);
      const auto untouched = neighbor;
      assert(ComboRegionalPassagesOot_Get(&play, &route, &mouth, &yaw) &&
             route == 5);
      assert(mouth.x == 22 && mouth.y == 120 && mouth.z == -336 && yaw == 0);
      assert(sRegional.grave == &original &&
             original.dyna.actor.update == BgHaka_Update);
      assert(database.profiles.size() == 1 &&
             allocated.size() == 1); // aperture only
      assert(!ComboRegionalPassagesOot_AtDeparture(&play, &mouth));
      original.dyna.unk_150 = -1;
      player.stateFlags2 |= PLAYER_STATE2_MOVING_DYNAPOLY;
      original.dyna.actor.update(&original.dyna.actor, &play);
      if (age == 1 && day) {
        assert(!original.state &&
               original.actionFunc == BgHaka_IdleLockedClosed);
        assert(!std::memcmp(&untouched, &neighbor, sizeof(neighbor)));
        continue;
      }
      for (int frame = 0; frame < 120 && !original.state; ++frame)
        original.dyna.actor.update(&original.dyna.actor, &play);
      assert(original.state == 1 && original.dyna.actor.minVelocityY == 60);
      assert(!std::memcmp(&untouched, &neighbor, sizeof(neighbor)));
      player.actor.world.pos = mouth;
      for (int i = 0; i < 12; ++i)
        HoleUpdate(sRegional.hole, &play);
      assert(ComboRegionalPassagesOot_OwnsGraveFall(&play));
      player.actor.world.pos.y = mouth.y - 10;
      assert(ComboRegionalPassagesOot_AtDeparture(
                 &play, &player.actor.world.pos) == 5);
      ComboRegionalPassagesOot_CancelDeparture();
      assert(!(player.stateFlags1 & PLAYER_STATE1_FLOOR_DISABLED));
    }
  gSaveContext.linkAge = 0;
  manager.save(gSaveContext, 0, false);
  const auto disk = manager.written;
  assert(disk["origins"][0]["x"] == 22 && disk["origins"][0]["room"] == 1);
  for (int slot = 0; slot < 3; ++slot) {
    LoadFile(disk, slot);
    gSaveContext.respawn[RESPAWN_MODE_RETURN] = {};
    gSaveContext.respawn[RESPAWN_MODE_RETURN].pos = {-999, 7, 888};
    gSaveContext.respawn[RESPAWN_MODE_RETURN].playerParams = 0x0EFF;
    const auto preserved = gSaveContext.respawn[RESPAWN_MODE_RETURN];
    assert(ComboRegionalPassagesOot_ApplyArrival(5));
    Scene(SCENE_GRAVEYARD);
    assert(ComboRegionalPassagesOot_IsArrivalPending(&play));
    assert(!ComboRegionalPassagesOot_IsArrivalPending(nullptr));
    PlayState otherPlay{};
    otherPlay.sceneNum = SCENE_GRAVEYARD;
    assert(!ComboRegionalPassagesOot_IsArrivalPending(&otherPlay));
    interactor.update();
    assert(!ComboRegionalPassagesOot_IsArrivalPending(&play));
    const auto staged = gSaveContext.respawn[RESPAWN_MODE_RETURN];
    assert(staged.playerParams == 0x04FF && staged.pos.x == 22 &&
           staged.pos.y == 120);
    // Native Actor_UpdateAll keeps running throughout the source fade.
    play.transitionMode = TRANS_MODE_INSTANCE_RUNNING;
    for (int frame = 0; frame < 8; ++frame) {
      interactor.update();
      assert(!std::memcmp(&staged, &gSaveContext.respawn[RESPAWN_MODE_RETURN],
                          sizeof(staged)));
    }
    auto *oldAddress = &play;
    interactor.destroy();
    Scene(SCENE_GRAVEYARD);
    assert(&play ==
           oldAddress); // allocator may reuse exactly the source address
    // Scene commands ran, but native Player_Init has not consumed RETURN yet.
    player.actor.init = Noop;
    interactor.update();
    assert(!std::memcmp(&staged, &gSaveContext.respawn[RESPAWN_MODE_RETURN],
                        sizeof(staged)));
    NativePlayerInitRespawn(&play, &player);
    assert(player.actor.params == 0x04FF && player.actor.world.pos.x == 22 &&
           player.actor.world.pos.y == 120);
    assert(jumpVelocity == 12 &&
           !std::strcmp(jumpAnimation, gPlayerAnim_link_normal_jump));
    assert(cutsceneId == 5110 && cutsceneDuration == 40);
    interactor.update();
    assert(!std::memcmp(&preserved, &gSaveContext.respawn[RESPAWN_MODE_RETURN],
                        sizeof(preserved)));
    assert(ComboRegionalPassagesOot_Get(&play, nullptr, nullptr, nullptr));
    assert(original.state == 1 && original.actionFunc == BgHaka_IdleOpened);
    play.transitionTrigger = TRANS_TRIGGER_OFF;
    player.stateFlags1 = 0;
    for (int i = 0; i < 12; ++i)
      HoleUpdate(sRegional.hole, &play);
    assert(!ComboRegionalPassagesOot_OwnsGraveFall(&play));
    player.actor.world.pos.z += 70;
    HoleUpdate(sRegional.hole, &play);
    player.actor.world.pos = staged.pos;
    HoleUpdate(sRegional.hole, &play);
    assert(ComboRegionalPassagesOot_OwnsGraveFall(&play));
    ComboRegionalPassagesOot_CancelDeparture();
  }
  LoadFile();
  ++seed;
  assert(!ComboRegionalPassagesOot_ApplyArrival(5));
  assert(!ComboRegionalPassagesOot_Get(&play, nullptr, nullptr, nullptr));
  --seed;
  // Actor_Spawn publishes init/update before the native object bank is ready.
  LoadFile();
  assert(ComboRegionalPassagesOot_ApplyArrival(5));
  Scene(SCENE_GRAVEYARD, true);
  deferHoleInit = true;
  interactor.update();
  assert(sArrivalRoute == 5 && original.state == 0 && allocated.empty());
  original.dyna.actor.init(&original.dyna.actor, &play);
  original.dyna.actor.init = nullptr;
  interactor.update();
  assert(sArrivalRoute == 5 && original.state == 1 && allocated.size() == 1);
  auto *pendingHole = allocated[0];
  assert(pendingHole->init);
  interactor.update();
  assert(allocated.size() == 1 && sArrivalRoute == 5);
  pendingHole->init(pendingHole, &play);
  pendingHole->init = nullptr;
  deferHoleInit = false;
  interactor.update();
  assert(sArrivalRoute == 0 && original.state == 1);
  interactor.update();
  // Object/actor readiness may lag arrival dispatch; the request must survive.
  LoadFile();
  assert(ComboRegionalPassagesOot_ApplyArrival(5));
  Scene(SCENE_GRAVEYARD);
  auto *pendingGraves = play.actorCtx.actorLists[ACTORCAT_BG].head;
  play.actorCtx.actorLists[ACTORCAT_BG].head = nullptr;
  interactor.update();
  assert(sArrivalRoute == 5 && play.envCtx.fillScreen &&
         play.transitionTrigger == TRANS_TRIGGER_OFF);
  play.actorCtx.actorLists[ACTORCAT_BG].head = pendingGraves;
  interactor.update();
  assert(sArrivalRoute == 0 && play.transitionTrigger == TRANS_TRIGGER_START);
  interactor.update();
  // A genuinely missing fixed owner must not leave permanent black cover.
  LoadFile();
  assert(ComboRegionalPassagesOot_ApplyArrival(5));
  Scene(SCENE_GRAVEYARD);
  play.actorCtx.actorLists[ACTORCAT_BG].head = nullptr;
  for (int frame = 0; frame < 301; ++frame)
    interactor.update();
  assert(sArrivalRoute == 0 && !play.envCtx.fillScreen);
  // Scene-owned actors may unload and reload without a new PlayState.
  Scene(SCENE_GRAVEYARD);
  assert(ComboRegionalPassagesOot_Get(&play, nullptr, nullptr, nullptr));
  auto *oldHole = sRegional.hole;
  oldHole->update = nullptr;
  assert(ComboRegionalPassagesOot_Get(&play, nullptr, nullptr, nullptr) &&
         sRegional.hole != oldHole);
  // Legacy saved points cannot move the fixed endpoint or affect new layouts.
  auto oldCoordinates = disk;
  oldCoordinates["origins"][0]["x"] = 10000;
  LoadFile(oldCoordinates);
  mq = true;
  gSaveContext.linkAge = 1;
  assert(ComboRegionalPassagesOot_ApplyArrival(5));
  Scene(SCENE_GRAVEYARD);
  interactor.update();
  assert(gSaveContext.respawn[RESPAWN_MODE_RETURN].pos.x == 22);
  interactor.update();
  mq = false;
  gSaveContext.linkAge = 0;
  for (int wanted : {6, 7}) {
    Scene(wanted == 6 ? SCENE_LAKE_HYLIA : SCENE_ZORAS_FOUNTAIN);
    gSaveContext.entranceIndex = wanted == 6
                                     ? ENTR_LAKE_HYLIA_OUTSIDE_LAB
                                     : ENTR_ZORAS_FOUNTAIN_OUTSIDE_ICE_CAVERN;
    assert(ComboRegionalPassagesOot_Get(&play, &route, &mouth, &yaw) &&
           route == wanted);
    if (wanted == 7) {
      // Walking along the former entrance and the deep valley must not queue
      // a departure before the new grounded threshold.
      for (float x : {1820.f, 1840.f, 2400.f, 3200.f, 3500.f}) {
        float floor;
        assert(ComboFountainValley::FloorAt(x, 0, &floor));
        Vec3f walking{x, floor, 0};
        assert(ComboRegionalPassagesOot_AtDeparture(&play, &walking) == 0);
      }
    }
    auto f = ComboRegional::Forward(yaw);
    Vec3f p{mouth.x + f.x * 28, mouth.y, mouth.z + f.z * 28};
    assert(ComboRegionalPassagesOot_AtDeparture(&play, &p) == wanted);
    const auto preservedBridgeReturn =
        gSaveContext.respawn[RESPAWN_MODE_RETURN];
    assert(ComboRegionalPassagesOot_ApplyArrival(wanted));
    assert(gSaveContext.entranceIndex ==
           (wanted == 6 ? ENTR_LAKE_HYLIA_OUTSIDE_LAB
                        : ENTR_ZORAS_FOUNTAIN_OUTSIDE_ICE_CAVERN));
    Scene(wanted == 6 ? SCENE_LAKE_HYLIA : SCENE_ZORAS_FOUNTAIN);
    interactor.update();
    auto arrival = gSaveContext.respawn[RESPAWN_MODE_RETURN];
    assert(arrival.playerParams == 0x0EFF);
    assert(std::fabs(arrival.pos.x - (mouth.x + f.x * 100)) < .1f);
    assert(std::fabs(arrival.pos.z - (mouth.z + f.z * 100)) < .1f);
    if (wanted == 7) {
      float floor;
      assert(
          ComboFountainValley::FloorAt(arrival.pos.x, arrival.pos.z, &floor));
      assert(std::fabs(arrival.pos.y - floor) < .01f &&
             std::fabs(arrival.pos.y - mouth.y) > 1);
      ComboRegional::Point r{arrival.pos.x, arrival.pos.y, arrival.pos.z};
      assert(!ComboRegional::InSnowheadMouth(sRegional.mouth, r));
    }
    play.transitionMode = TRANS_MODE_INSTANCE_RUNNING;
    for (int frame = 0; frame < 8; ++frame) {
      interactor.update();
      assert(!std::memcmp(&arrival, &gSaveContext.respawn[RESPAWN_MODE_RETURN],
                          sizeof(arrival)));
    }
    interactor.destroy();
    Scene(wanted == 6 ? SCENE_LAKE_HYLIA : SCENE_ZORAS_FOUNTAIN);
    player.actor.init = Noop;
    interactor.update();
    assert(!std::memcmp(&arrival, &gSaveContext.respawn[RESPAWN_MODE_RETURN],
                        sizeof(arrival)));
    NativePlayerInitRespawn(&play, &player);
    assert(player.actor.params == 0x0EFF && player.actor.shape.rot.y == yaw);
    assert(std::fabs(player.actor.world.pos.x - arrival.pos.x) < .1f &&
           std::fabs(player.actor.world.pos.z - arrival.pos.z) < .1f);
    interactor.update();
    assert(!std::memcmp(&preservedBridgeReturn,
                        &gSaveContext.respawn[RESPAWN_MODE_RETURN],
                        sizeof(preservedBridgeReturn)));
  }
  assert(lakeCalls && fountainCalls);
  Scene(SCENE_ICE_CAVERN);
  assert(!ComboRegionalPassagesOot_Get(&play, nullptr, nullptr, nullptr));
  LoadFile();
  assert(ComboRegionalPassagesOot_ApplyArrival(5));
  gSaveContext.respawnFlag = 3;
  gSaveContext.respawn[RESPAWN_MODE_TOP].pos = {444, 55, 666};
  Scene(SCENE_GRAVEYARD);
  gSaveContext.respawnFlag = 0;
  interactor.update();
  assert(sArrivalRoute == 0 && play.transitionTrigger == TRANS_TRIGGER_OFF &&
         gSaveContext.respawn[RESPAWN_MODE_TOP].pos.x == 444);
  // A higher-priority native arrival replacing RETURN owns its newer record.
  LoadFile();
  assert(ComboRegionalPassagesOot_ApplyArrival(5));
  Scene(SCENE_GRAVEYARD);
  interactor.update();
  gSaveContext.respawn[RESPAWN_MODE_RETURN].pos = {7, 8, 9};
  const auto newerReturn = gSaveContext.respawn[RESPAWN_MODE_RETURN];
  gSaveContext.respawnFlag = 3;
  Scene(SCENE_GRAVEYARD);
  assert(!sReturnStaged && sArrivalRoute == 0);
  assert(!std::memcmp(&newerReturn, &gSaveContext.respawn[RESPAWN_MODE_RETURN],
                      sizeof(newerReturn)));
  for (int mode : {COMBO_GAME_OOT, COMBO_GAME_MM}) {
    gComboGameMode = mode;
    auto before = gSaveContext;
    assert(!ComboRegionalPassagesOot_Get(&play, nullptr, nullptr, nullptr));
    assert(!ComboRegionalPassagesOot_ApplyArrival(7));
    assert(!std::memcmp(&before, &gSaveContext, sizeof(before)));
  }
  gComboGameMode = COMBO_GAME_BOTH;
  interactor.destroy();
  assert(!ComboRegionalPassagesOot_Get(&play, nullptr, nullptr, nullptr));
  puts("PASS regional OoT native endpoint: exact existing grave, native "
       "age/day pull and grotto return, neighbor preservation, retrigger gate, "
       "provider delegation, slot/seed and Farore precedence");
}
