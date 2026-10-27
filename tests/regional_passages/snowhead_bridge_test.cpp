#include "entrances/FountainSnowheadBridge.h"
#include "render/RegionalSnow.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#ifndef SNOWHEAD_NATIVE_TEST
int main(int argc, char **) {
  using namespace ComboFountainSnowhead;
  auto mesh = BuildMesh();
  assert(mesh.count > 12 && mesh.count < 256);
  assert(kMouth.x == 3600 && std::fabs(kMouth.y - 225.984f) < .01f &&
         kMouth.z == 0 && kYaw == -16384);
  for (size_t i = 1; i < kStationCount; ++i) {
    auto a = kStations[i - 1], b = kStations[i];
    float run = std::hypot(b.x - a.x, b.z - a.z);
    assert(run > 0 && std::fabs(b.y - a.y) / run < .55f);
  }
  // Both ages have the same route. The first ramp meets water at y=0;
  // its upper landing joins the open valley, separately from Ice Cavern.
  assert(kStations[0].y == 0 && kHalfWidth >= 70 && kHeadroom >= 140);
  for (size_t i = 0; i < mesh.count; ++i) {
    const auto &t = mesh.triangles[i];
    assert(std::isfinite(t.a.x + t.b.y + t.c.z));
    assert(t.a.z > -1786 && t.b.z > -1786 && t.c.z > -1786);
  }
  if (argc > 1) {
    for (size_t i = 0; i < mesh.count; ++i) {
      const auto &t = mesh.triangles[i];
      std::printf("%ld %ld %ld %ld %ld %ld %ld %ld %ld\n", std::lround(t.a.x),
                  std::lround(t.a.y), std::lround(t.a.z), std::lround(t.b.x),
                  std::lround(t.b.y), std::lround(t.b.z), std::lround(t.c.x),
                  std::lround(t.c.y), std::lround(t.c.z));
    }
    return 0;
  }
  std::puts("PASS Snowhead authored geometry: fixed pose, continuous shallow "
            "waterline/ascent, entrance separation");
}

#endif
#ifdef SNOWHEAD_NATIVE_TEST
#include "ComboGameMode.h"
#include "entrances/LostWoodsEntrance.h"
#include "global.h"
#include "soh/ActorDB.h"
#include <cstring>
#undef osSyncPrintf
#define osSyncPrintf(...) ((void)0)
#define SS_NULL 0xFFFF
extern "C" void lusprintf(const char*, int32_t, int32_t, const char*, ...) {}
uintptr_t gSegments[16]{};
extern "C" PosRot *Actor_GetWorldPosShapeRot(PosRot *p, Actor *a) {
  p->pos = a->world.pos;
  p->rot = a->shape.rot;
  return p;
}
/* NATIVE_ENGINE */
SaveContext gSaveContext{};
PlayState *gPlayState = nullptr;
extern "C" {
int gComboGameMode = COMBO_GAME_BOTH;
}
static int adds = 0, deletes = 0, spawns = 0;
static bool failDyna = false, valleyReady = true, inCutscene = false;
extern "C" int ComboFountainValley_Ready(PlayState *) { return valleyReady; }
static ActorDB::Entry fakeEntry;
ActorDB::ActorDB() {}
static ActorDB fakeDB;
ActorDB *ActorDB::Instance = &fakeDB;
ActorDB::Entry::Entry() {}
ActorDB::Entry &ActorDB::AddEntry(const ActorDBInit &init) {
  fakeEntry.entry.id = 400;
  fakeEntry.entry.update = init.update;
  return fakeEntry;
}
extern "C" void Actor_SetScale(Actor *a, f32 v) { a->scale = {v, v, v}; }
extern "C" void Actor_Kill(Actor *a) { a->update = nullptr; }
extern "C" s32 DynaPoly_SetBgActor(PlayState *play, DynaCollisionContext *dyna,
                                   Actor *a, CollisionHeader *h) {
  assert(h->numWaterBoxes == 0);
  ++adds;
  if (failDyna) {
    for (auto &f : dyna->bgActorFlags)
      f = 1;
  }
  return Native_DynaPoly_SetBgActor(play, dyna, a, h);
}
extern "C" void DynaPoly_DeleteBgActor(PlayState *, DynaCollisionContext *dyna,
                                       s32 id) {
  assert(id == 0);
  dyna->bgActorFlags[id] = 0;
  ++deletes;
}
extern "C" s32 Player_InCsMode(PlayState *) { return inCutscene; }
/* NATIVE_PRODUCTION */
static SnowheadActor actors[8]{};
extern "C" Actor *Actor_Spawn(ActorContext *, PlayState *play, s16, f32, f32,
                              f32, s16, s16, s16, s16) {
  auto *a = &actors[spawns++];
  a->dyna.actor.update = Update;
  Init(&a->dyna.actor, play);
  return &a->dyna.actor;
}
int main() {
  PlayState play{};
  Player player{};
  Vec3f mouth{};
  s16 yaw = 0;
  play.sceneNum = SCENE_ZORAS_FOUNTAIN;
  play.roomCtx.curRoom.num = 0;
  play.state.running = true;
  play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
  gSaveContext.gameMode = GAMEMODE_NORMAL;
  gSaveContext.fileNum = 0;
  gPlayState = &play;
  valleyReady = false;
  Ensure(&play);
  assert(!ComboFountainSnowheadBridge_Get(&play, &mouth, &yaw));
  assert(spawns == 0 && adds == 0);
  valleyReady = true;
  assert(ComboFountainSnowheadBridge_Get(&play, &mouth, &yaw));
  assert(spawns == 1 && adds == 1);
  valleyReady = false;
  assert(!ComboFountainSnowheadBridge_Get(&play, &mouth, &yaw));
  valleyReady = true;
  assert(mouth.x == 3600 && std::fabs(mouth.y - 225.984f) < .01f &&
         mouth.z == 0 && yaw == -16384);
  // Native broad-phase bounds must include the entire final rounded mesh,
  // including the wider waterline cross-section and the high shoulder.
  for (size_t i = 0; i < sMesh.count * 3; ++i) {
    const auto &v = sVertices[i];
    const auto &min = sCollision.minBounds, &max = sCollision.maxBounds;
    assert(v.x >= min.x && v.x <= max.x && v.y >= min.y && v.y <= max.y &&
           v.z >= min.z && v.z <= max.z &&
           "native collision header must enclose every rounded vertex");
  }
  auto *header = sCollision.polyList;
  auto *vertices = sCollision.vtxList;
  auto *cameraTable = sCollision.cameraDataList;
  // Query the real registered native BgActor through the exact native
  // Camera_GetDataIdxForPoly -> SurfaceType -> BgCheck camera lookup chain.
  {
    Camera camera{};
    camera.play = &play;
    camera.player = &player;
    s32 bgId = sActor->dyna.bgId;
    for (size_t i = 0; i < sMesh.count; ++i)
      if (sPolys[i].normal.y > COLPOLY_SNORMAL(.5f))
        assert(Camera_GetDataIdxForPoly(&camera, &bgId, &sPolys[i]) == -1);
    assert(sCollision.cameraDataListLen == 1);
    assert(sCollision.cameraDataList[0].cameraSType == CAM_SET_NONE);
    puts("PASS native registered-floor camera query returns CAM_SET_NONE "
         "without dereferencing null");
  }
  // Native dynamic-wall tests use actual triangle intersection, sphere
  // one-face selection and wall displacement; no wall/camera query stubs.
  play.colCtx.colHeader = &sCollision;
  play.colCtx.dyna.vtxList = sVertices.data();
  play.colCtx.dyna.polyList = sPolys.data();
  for (size_t i = 0; i < sMesh.count; ++i) {
    assert(!sMesh.triangles[i].cave);
    if (i % 6 < 2)
      continue;
    auto &poly = sPolys[i];
    assert(std::abs(poly.normal.y) < 2);
    auto a = sVertices[i * 3], b = sVertices[i * 3 + 1],
         c = sVertices[i * 3 + 2];
    Vec3f normal{poly.normal.x / 32767.f, 0, poly.normal.z / 32767.f};
    Vec3f center{(a.x + b.x + c.x) / 3.f, (a.y + b.y + c.y) / 3.f,
                 (a.z + b.z + c.z) / 3.f};
    SSNode node{};
    node.polyId = i;
    node.next = SS_NULL;
    SSList list{};
    list.head = 0;
    play.colCtx.dyna.polyNodes.tbl = &node;
    Vec3f pos{center.x + normal.x * 5, center.y, center.z + normal.z * 5};
    CollisionPoly *hit = nullptr;
    s32 bgId = -1;
    float x = pos.x, z = pos.z;
    assert(BgCheck_SphVsDynaWallInBgActor(&play.colCtx, 0, &play.colCtx.dyna,
                                          &list, &x, &z, &hit, &bgId, &pos, 10,
                                          0));
    assert(hit == &poly && bgId == 0);
    assert((x - pos.x) * normal.x + (z - pos.z) * normal.z > 4.0f);
  }
  puts("PASS native ascent support wall push and floor classification");
  // The actual native polygon floor intersection visits the complete rounded
  // centerline in 12-unit steps, both walking lanes within the body reserve.
  for (float lane : {-4.f, 0.f, 4.f}) {
    float previous = 0;
    for (size_t station = 1; station < kStationCount; ++station) {
      auto a = kStations[station - 1], b = kStations[station];
      float length = std::hypot(b.x - a.x, b.z - a.z);
      int steps = int(std::ceil(length / 12));
      for (int step = 0; step <= steps; ++step) {
        float t = float(step) / steps,
              x = a.x + (b.x - a.x) * t + (b.z - a.z) / length * lane,
              z = a.z + (b.z - a.z) * t - (b.x - a.x) / length * lane,
              floor = -10000;
        for (size_t p = 0; p < sMesh.count; ++p)
          if (p % 6 < 2) {
            float y;
            if (CollisionPoly_CheckYIntersectApprox1(
                    &sPolys[p], sVertices.data(), x, z, &y, 1))
              floor = std::fmax(floor, y);
          }
        assert(floor > -1 && floor < 260);
        assert(std::fabs(floor - previous) < 16);
        previous = floor;
      }
    }
  }
  // Ordinary child exits water at depth22, adult36. At the un-lipped first
  // slope the native waterline20 is shallower than both standing thresholds.
  assert(20 - kStations[0].y < 22 && 20 - kStations[0].y < 36);
  puts("PASS native full-ascent floor intersections: 12-unit walking steps, "
       "body-reserved lanes and both-age swim exit depth");
  for (int age = 0; age < 2; ++age)
    for (int slot = 0; slot < 3; ++slot) {
      gSaveContext.linkAge = age;
      gSaveContext.fileNum = slot;
      assert(ComboFountainSnowheadBridge_Get(&play, &mouth, &yaw));
      assert(spawns == 1);
    }
  // Draw vertices preserve continuous top UVs and a nondegenerate mapping
  // on every rock/ice face. Distinct material IDs survive mesh generation.
  for (size_t i = 0; i < sMesh.count; ++i) {
    auto a = sDrawVertices[i * 3].v, b = sDrawVertices[i * 3 + 1].v,
         c = sDrawVertices[i * 3 + 2].v;
    assert((b.tc[0] - a.tc[0]) * (c.tc[1] - a.tc[1]) !=
           (c.tc[0] - a.tc[0]) * (b.tc[1] - a.tc[1]));
    if (i % 6 < 2)
      for (size_t j = 0; j < 3; ++j) {
        auto p = sDrawVertices[i * 3 + j].v;
        assert(std::abs(p.tc[0] - p.ob[0] * 4) <= 2 &&
               std::abs(p.tc[1] - p.ob[2] * 4) <= 2);
      }
  }
  for (size_t i = 0; i < sMesh.count; ++i)
    for (size_t j = 0; j < sMesh.count; ++j)
      if (!sMesh.triangles[i].snow &&
          sMesh.triangles[i].material == sMesh.triangles[j].material)
        for (size_t a = 0; a < 3; ++a)
          for (size_t b = 0; b < 3; ++b) {
            auto u = sDrawVertices[i * 3 + a].v, v = sDrawVertices[j * 3 + b].v;
            if (u.ob[0] == v.ob[0] && u.ob[1] == v.ob[1] && u.ob[2] == v.ob[2])
              assert(u.tc[0] == v.tc[0] && u.tc[1] == v.tc[1] &&
                     "stone/ice UVs must continue across station seams");
          }
  // Every collision plane is derived from its final rounded vertices.
  for (size_t i = 0; i < sMesh.count; ++i) {
    const auto &p = sPolys[i];
    auto v = sVertices[i * 3];
    float plane =
        (p.normal.x * v.x + p.normal.y * v.y + p.normal.z * v.z) / 32767.f +
        p.dist;
    assert(std::fabs(plane) < .7f);
    if (i % 6 < 2) {
      if (p.normal.y <= 28000)
        std::fprintf(stderr, "steep polygon%zu normalY%d\n", i, p.normal.y);
      assert(p.normal.y > 28000);
    }
  }
  for (int mode : {COMBO_GAME_OOT, COMBO_GAME_MM}) {
    gComboGameMode = mode;
    assert(!ComboFountainSnowheadBridge_Get(&play, &mouth, &yaw));
  }
  gComboGameMode = COMBO_GAME_BOTH;
  play.roomCtx.curRoom.num = 1;
  assert(!ComboFountainSnowheadBridge_Get(&play, &mouth, &yaw));
  play.roomCtx.curRoom.num = 0;
  player.actor.world.pos = {1000, 0, 850};
  Update(&sActor->dyna.actor, &play);
  assert(sActor->snowTick > 0);
  auto oldTick = sActor->snowTick;
  play.pauseCtx.state = 1;
  Update(&sActor->dyna.actor, &play);
  assert(sActor->snowTick == oldTick);
  play.pauseCtx.state = 0;
  inCutscene = true;
  Update(&sActor->dyna.actor, &play);
  assert(sActor->snowTick == oldTick);
  inCutscene = false;
  play.transitionMode = TRANS_MODE_INSTANCE_RUNNING;
  Update(&sActor->dyna.actor, &play);
  assert(sActor->snowTick == oldTick);
  play.transitionMode = TRANS_MODE_OFF;
  play.pauseCtx.state = 0;
  player.actor.world.pos = {0, 0, 2500};
  Update(&sActor->dyna.actor, &play);
  Destroy(&sActor->dyna.actor, &play);
  assert(deletes == 1 && !sActor);
  assert(ComboFountainSnowheadBridge_Get(&play, &mouth, &yaw));
  assert(spawns == 2 && adds == 2 && sActor->snowTick == 0);
  assert(header == sCollision.polyList && vertices == sCollision.vtxList &&
         cameraTable == sCollision.cameraDataList);
  Destroy(&sActor->dyna.actor, &play);
  failDyna = true;
  assert(!ComboFountainSnowheadBridge_Get(&play, &mouth, &yaw));
  assert(!sActor);
  assert(deletes == 2);
  failDyna = false;
  assert(!ComboFountainSnowheadBridge_Get(nullptr, &mouth, &yaw));
  puts("PASS native production Snowhead lifecycle: all ages/slots, "
       "repeatedGet/destroy/reentry, allocation failure, plane normals and "
       "local chill");
}
#endif
