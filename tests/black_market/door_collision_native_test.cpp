#include "BlackMarketExitDoor.h"
#include "global.h"
#include "overlays/actors/ovl_En_Door/z_en_door.h"
#ifdef DOOR_COLLISION_OOT
#include "OotBlackMarketDoorNative.h"
#else
#include "2s2h/GameInteractor/GameInteractor.h"
#include "BlackMarketDoorNative.h"
#endif
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#if __has_include("BlackMarketDoorCollision.h")
#include "BlackMarketDoorCollision.h"
static BlackMarketDoorCollision::Fixture<
#ifdef DOOR_COLLISION_OOT
    true
#else
    false
#endif
    >
    sDoorCollision;
#endif
static PlayState play;
PlayState *gPlayState = &play;
GameState *gGameState = &play.state;
static bool interior = true, exterior = false;
bool Interior() { return interior; }
bool Exterior(PlayState *) { return exterior; }
static EnDoor door;
static Player player;
static TransitionActorEntry entry;
static std::vector<void *> allocations;
extern "C" void EnDoor_Idle(EnDoor *, PlayState *) {}
static void Opening(EnDoor *, PlayState *) {}
static void ActorCallback(Actor *, PlayState *) {}
SaveContext gSaveContext;
extern "C" {
uintptr_t gSegments[16]{};
u8 gMogmaMittsClimbActive = 0, gKeatonClimbActive = 0;
u8 Slate_IsClimbableBgId(s32) { return 0; }
#ifdef DOOR_COLLISION_OOT
void lusprintf(const char *, s32, s32, const char *, ...) {}
void osSyncPrintfUnused(const char *, ...) {}
static GameInfo gameInfo;
GameInfo *gGameInfo = &gameInfo;
void LogUtils_HungupThread(const char *, s32) { std::abort(); }
void *THA_AllocEndAlign(TwoHeadArena *, size_t n, size_t) {
  void *p = std::calloc(1, n);
  assert(p);
  allocations.push_back(p);
  return p;
}
void *THA_AllocEndAlign16(TwoHeadArena *a, size_t n) {
  return THA_AllocEndAlign(a, n, 0);
}
s32 ResourceMgr_OTRSigCheck(const void *) { return 0; }
CollisionHeader *ResourceMgr_LoadColByName(const char *) {
  assert(false);
  return nullptr;
}
s16 R_ROOM_MESH_CULL_DIST = 0;
s32 GameInteractor_Should(s32, u32 result, ...) { return result; }
void *GameState_Alloc(GameState *, size_t n, char *, s32) {
  void *p = std::calloc(1, n);
  allocations.push_back(p);
  return p;
}
void *GameState_AllocEndAlign16(GameState *, size_t n) {
  void *p = std::calloc(1, n);
  allocations.push_back(p);
  return p;
}
#else
bool GameInteractor_Should(GIVanillaBehavior, u32 result, ...) {
  return result;
}
int ComboBlackMarket_IsCounterBarrier(const CollisionContext *, int) {
  return false;
}
#endif
void *THA_AllocTailAlign(TwoHeadArena *, size_t n, uintptr_t) {
  void *p = std::calloc(1, n);
  assert(p);
  allocations.push_back(p);
  return p;
}
void *THA_AllocTailAlign16(TwoHeadArena *a, size_t n) {
  return THA_AllocTailAlign(a, n, 0);
}
void Fault_AddHungupAndCrash(const char *file,
#ifdef DOOR_COLLISION_OOT
                             u32
#else
                             s32
#endif
                                 line) {
  std::fprintf(stderr, "collision fault %s:%d\n", file, line);
  std::abort();
}
void Fault_AddHungupAndCrashImpl(const char *, const char *) { std::abort(); }
s32 BgCheck_SphVsDynaWall(CollisionContext *, u16, f32 *, f32 *, Vec3f *, f32,
                          CollisionPoly **, s32 *, Actor *);
}
/* PRODUCTION SYNC */
static u32 Read32(FILE *f) {
  u32 n;
  assert(std::fread(&n, 4, 1, f) == 1);
  return n;
}
struct NativeRoom {
  CollisionHeader header{};
  std::vector<Vec3s> vertices;
  std::vector<CollisionPoly> polygons;
  std::vector<SurfaceType> surfaces;
  explicit NativeRoom(const char *path) {
    FILE *f = std::fopen(path, "rb");
    assert(f && !std::fseek(f, 64, SEEK_SET));
    assert(std::fread(&header.minBounds, sizeof(Vec3s), 1, f) == 1 &&
           std::fread(&header.maxBounds, sizeof(Vec3s), 1, f) == 1);
    vertices.resize(Read32(f));
    assert(std::fread(vertices.data(), sizeof(Vec3s), vertices.size(), f) ==
           vertices.size());
    polygons.resize(Read32(f));
    assert(std::fread(polygons.data(), sizeof(CollisionPoly), polygons.size(),
                      f) == polygons.size());
    surfaces.resize(Read32(f));
    assert(std::fread(surfaces.data(), sizeof(SurfaceType), surfaces.size(),
                      f) == surfaces.size());
    std::fclose(f);
    header.numVertices = vertices.size();
    header.vtxList = vertices.data();
    header.numPolygons = polygons.size();
    header.polyList = polygons.data();
    header.surfaceTypeList = surfaces.data();
  }
};
static Actor &Door() {
#ifdef DOOR_COLLISION_OOT
  return door.actor;
#else
  return door.knobDoor.dyna.actor;
#endif
}
static void Index() {
#ifdef DOOR_COLLISION_OOT
  DynaPoly_Setup(&play, &play.colCtx.dyna);
#else
  DynaPoly_UpdateContext(&play, &play.colCtx.dyna);
#endif
}
static bool Line(Vec3f a, Vec3f b, bool camera = false) {
  Vec3f hit{};
  CollisionPoly *poly = nullptr;
  s32 bg = BGCHECK_SCENE;
#ifdef DOOR_COLLISION_OOT
  return camera ? BgCheck_CameraLineTest1(&play.colCtx, &a, &b, &hit, &poly,
                                          true, true, true, true, &bg)
                : BgCheck_EntityLineTest1(&play.colCtx, &a, &b, &hit, &poly,
                                          true, true, true, true, &bg);
#else
  return camera ? BgCheck_CameraLineTest1(&play.colCtx, &a, &b, &hit, &poly,
                                          true, true, true, true, &bg)
                : BgCheck_EntityLineTest1(&play.colCtx, &a, &b, &hit, &poly,
                                          true, true, true, true, &bg);
#endif
}
static void Prepare(NativeRoom &room) {
  for (void *p : allocations)
    std::free(p);
  allocations.clear();
  play = {};
  door = {};
  player = {};
  entry = {};
  play.state.running = true;
  play.roomCtx.curRoom.num = 0;
#ifdef DOOR_COLLISION_OOT
  play.sceneNum = SCENE_SKIEJER_MARKET;
  play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
  play.actorCtx.actorLists[ACTORCAT_DOOR].head = &Door();
  play.transiActorCtx.list = &entry;
  play.transiActorCtx.numActors = 1;
  entry.params = 384;
#else
  play.sceneId = COMBO_BLACK_MARKET_SCENE;
  play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
  play.actorCtx.actorLists[ACTORCAT_DOOR].first = &Door();
  play.transitionActors.list = &entry;
  play.transitionActors.count = 1;
  entry.params = 7;
  entry.rotY = 127;
#endif
  entry.id = -ACTOR_EN_DOOR;
  entry.sides[0].room = 0;
  entry.sides[1].room = -1;
  entry.pos = {-10, 0, 420};
  Door().id = ACTOR_EN_DOOR;
  Door().category = ACTORCAT_DOOR;
  Door().room = 0;
  Door().params = entry.params;
  Door().home.pos = Door().world.pos = {-10, 0, 420};
  Door().update = Door().draw = ActorCallback;
  door.actionFunc = EnDoor_Idle;
  BgCheck_Allocate(&play.colCtx, &play, &room.header);
}
int main(int argc, char **argv) {
  assert(argc == 2);
  NativeRoom room(argv[1]);
  const auto originalVertices = room.vertices;
  const auto originalPolygons = room.polygons;
  Prepare(room);
  assert(!Line({-10, 50, 390}, {-10, 50, 450}) &&
         "negative control: actual room doorway has no leaf collision");
  SyncShopDoorCollision();
  Index();
  assert(Line({-10, 50, 390}, {-10, 50, 450}) &&
         "a live closed shop door must block forward walking through its leaf");
  play.transitionTrigger = TRANS_TRIGGER_START;
  SyncShopDoorCollision();
  Index();
  assert(!Line({-10, 50, 390}, {-10, 50, 450}) &&
         "native A/Open transition must keep its leaf released");
  play.transitionTrigger = TRANS_TRIGGER_OFF;
  SyncShopDoorCollision();
  Index();
  assert(Line({-10, 50, 390}, {-10, 50, 450}));
  for (int side : {-1, 1})
    for (float radius : {10.f, 20.f, 40.f}) {
      Vec3f p{-10, 30, 420 + side * (radius + 5)};
      for (int frame = 0; frame < 120; ++frame) {
        Vec3f wanted = p;
        wanted.z -= side * 3;
        float x = wanted.x, z = wanted.z;
        CollisionPoly *wall = nullptr;
        s32 bg = BGCHECK_SCENE;
        BgCheck_SphVsDynaWall(&play.colCtx, 2, &x, &z, &wanted, radius, &wall,
                              &bg, &player.actor);
        p = {x, wanted.y, z};
        assert(side * (p.z - 420) >= radius - .1f &&
               "sustained forward cannot cross closed leaf");
      }
    }
  assert(Line({-10, 50, 450}, {-10, 50, 390}) &&
         "both sides stop at the closed leaf");
  assert(!Line({-10, 50, 390}, {-10, 50, 450}, true) &&
         "door blocker never changes camera collision");
#if __has_include("BlackMarketDoorCollision.h")
  const int owned = sDoorCollision.OwnedBgId(&play);
  assert(owned >= 0);
  SyncShopDoorCollision();
  Index();
  assert(sDoorCollision.OwnedBgId(&play) == owned);
  for (int reason = 0; reason < 5; ++reason) {
    if (reason == 0)
      door.actionFunc = Opening;
    if (reason == 1)
      Door().update = nullptr;
    if (reason == 2)
      Door().draw = nullptr;
    if (reason == 3)
      Door().home.pos.x += 5;
    if (reason == 4)
      entry.params ^= 1;
    SyncShopDoorCollision();
    Index();
    assert(sDoorCollision.OwnedBgId(&play) < 0);
    assert(!Line({-10, 50, 390}, {-10, 50, 450}) &&
           "opening/missing/destroyed/mismatched doors cannot trap the player");
    assert(BlackMarketExitDoor::WalkExit<
#ifdef DOOR_COLLISION_OOT
               true
#else
               false
#endif
               >(&play, {-10, 0, 410}) == (reason != 0));
    Prepare(room);
    SyncShopDoorCollision();
    Index();
  }
  for (s16 yaw : {s16(0), s16(16384), s16(-16384)}) {
    Prepare(room);
    interior = false;
    exterior = true;
    entry.sides[1].room = 0;
    Door().home.pos = Door().world.pos = {0, 0, 250};
    entry.pos = {0, 0, 250};
    Door().home.rot.y = Door().shape.rot.y = yaw;
#ifdef DOOR_COLLISION_OOT
    entry.rotY = yaw;
    entry.params = Door().params = 0;
#else
    entry.rotY = (yaw == 0 ? 0 : yaw > 0 ? 90 : 270) * 128 + 127;
#endif
    const float sx = std::sin(yaw * (6.28318530718f / 65536)),
                cz = std::cos(yaw * (6.28318530718f / 65536));
    const Vec3f a{-sx * 40, 50, 250 - cz * 40}, b{sx * 40, 50, 250 + cz * 40};
    assert(!Line(a, b));
    SyncShopDoorCollision();
    Index();
    assert(Line(a, b) && Line(b, a) &&
           "authored exterior yaw is the closed collision plane");
#ifdef DOOR_COLLISION_OOT
    door.playerIsOpening = true;
#else
    door.knobDoor.requestOpen = true;
#endif
    SyncShopDoorCollision();
    Index();
    assert(!Line(a, b) &&
           "native open request releases even before action changes");
#ifdef DOOR_COLLISION_OOT
    door.playerIsOpening = false;
#else
    door.knobDoor.requestOpen = false;
#endif
    SyncShopDoorCollision();
    Index();
    assert(Line(a, b));
    // A foreign/ambiguous same-room marker must not acquire a blocker.
    entry.pos.x += 10;
    SyncShopDoorCollision();
    Index();
    assert(!Line(a, b));
    entry.pos.x -= 10;
    SyncShopDoorCollision();
    Index();
    assert(Line(a, b));
    sDoorCollision.Release(&play);
    Index();
    assert(!Line(a, b));
    play.roomCtx.curRoom.num = 1;
    SyncShopDoorCollision();
    Index();
    assert(sDoorCollision.OwnedBgId(&play) < 0);
  }
  interior = true;
  exterior = false;
  Prepare(room);
  SyncShopDoorCollision();
  Index();
  sDoorCollision.Release(&play);
  Index();
  assert(!Line({-10, 50, 390}, {-10, 50, 450}));
  for (auto &flags : play.colCtx.dyna.bgActorFlags)
    flags = 1;
  SyncShopDoorCollision();
  assert(sDoorCollision.OwnedBgId(&play) < 0 &&
         "capacity exhaustion cannot install a partial blocker");
  for (auto &flags : play.colCtx.dyna.bgActorFlags)
    flags = 0;
  const auto max = play.colCtx.dyna.polyListMax;
  play.colCtx.dyna.polyListMax = 3;
  SyncShopDoorCollision();
  assert(sDoorCollision.OwnedBgId(&play) < 0);
  play.colCtx.dyna.polyListMax = max;
  SyncShopDoorCollision();
  Index();
  assert(Line({-10, 50, 390}, {-10, 50, 450}));
  sDoorCollision.Reset();
  SyncShopDoorCollision();
  Index();
  assert(Line({-10, 50, 390}, {-10, 50, 450}));

  assert(std::memcmp(room.vertices.data(), originalVertices.data(),
                     room.vertices.size() * sizeof(Vec3s)) == 0);
  assert(std::memcmp(room.polygons.data(), originalPolygons.data(),
                     room.polygons.size() * sizeof(CollisionPoly)) == 0);
#endif
  for (void *p : allocations)
    std::free(p);
  allocations.clear();
  std::puts("PASS native shop leaf collision: actual room/Dyna wall, both "
            "sides/radii/120 forward frames, camera pass-through, duplicate "
            "ownership and open/missing/kill/mismatch escape, immutable scene "
            "collision");
}
