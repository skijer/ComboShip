#include "fast/resource/type/DisplayList.h"
#include "fast/resource/type/Vertex.h"
#include "ship/resource/ResourceManager.h"
#include "global.h"
#include "soh/ActorDB.h"
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <vector>
#undef osSyncPrintf
#define osSyncPrintf(...) ((void)0)
/* NATIVE_ENGINE */
/* NATIVE_PRODUCTION */
static std::vector<std::string> reports;
extern "C" void lusprintf(const char*, int32_t, int32_t, const char* format, ...) {
  char message[2048];
  va_list arguments;
  va_start(arguments, format);
  std::vsnprintf(message, sizeof(message), format, arguments);
  va_end(arguments);
  reports.emplace_back(message);
}
SaveContext gSaveContext{};
PlayState *gPlayState = nullptr;
extern "C" {
int gComboGameMode = COMBO_GAME_BOTH;
}
static std::map<std::string, std::shared_ptr<Ship::IResource>> resources;
static std::map<std::string, std::shared_ptr<Ship::IResource>> foreignResources;
static bool foreignActive = false;
std::shared_ptr<Ship::IResource> ResourceLoad(const char *p) {
  return (foreignActive ? foreignResources : resources)[p];
}
// Keep the registered-owner boundary distinct from the globally active manager.
// No archives or loader workers are needed to test the production selection.
Ship::ResourceManager::ResourceManager() {}
Ship::ResourceManager::~ResourceManager() {}
static auto ootOwner = std::make_shared<Ship::ResourceManager>();
std::shared_ptr<Ship::ResourceManager> Ship::CrossRMRegistry::GetOrActive(const std::string& name) {
  assert(name == "oot");
  return ootOwner;
}
std::shared_ptr<Ship::IResource> Ship::ResourceManager::LoadResource(
    const std::string& path, bool, std::shared_ptr<Ship::ResourceInitData>) {
  assert(this == ootOwner.get());
  return resources[path];
}
// Keep the actual renderer's lookup and vertex submission boundaries small;
// its command parsing and in-place cache mutation remain production code.
struct VertexLookup {
  void *GetResourceRawPointer(uint64_t hash) {
    assert(hash == UINT64_C(0x5e7003a09feb4255));
    return resources.at("scenes/shared/spot08_scene/spot08_room_0Vtx_0023F8")
        ->GetRawPointer();
  }
};
static VertexLookup *ActiveResMgr() {
  static VertexLookup lookup;
  return &lookup;
}
using F3DGfx = Gfx;
using F3DVtx = Vtx;
struct Interpreter {
  void GfxSpVertex(unsigned count, unsigned offset, const F3DVtx *vertices) {
    assert(count == 6 && offset == 0 && vertices);
  }
};
#define C0(shift, width) ((cmd->words.w0 >> (shift)) & ((1U << (width)) - 1))
/* NATIVE_RENDERER */
#undef C0
static void RenderBackdropVertices(Fast::DisplayList &list) {
  Interpreter renderer;
  for (size_t slot : {25, 40}) {
    auto *command = &list.Instructions[slot];
    gfx_vtx_hash_handler_custom(&renderer, &command);
    assert(command == &list.Instructions[slot + 1]);
  }
}
ActorDB::ActorDB() {}
ActorDB::Entry::Entry() {}
static ActorDB database;
ActorDB *ActorDB::Instance = &database;
ActorDB::Entry &ActorDB::AddEntry(const ActorDBInit &) {
  static ActorDB::Entry e;
  e.entry.id = 400;
  return e;
}
extern "C" void Actor_SetScale(Actor *a, float scale) {
  a->scale = {scale, scale, scale};
}
extern "C" void Actor_Kill(Actor *a) { a->update = nullptr; }
static ValleyActor actors[32]{};
static int spawns = 0;
extern "C" Actor *Actor_Spawn(ActorContext *, PlayState *p, s16, f32, f32, f32,
                              s16, s16, s16, s16) {
  assert(spawns < 32);
  auto *a = &actors[spawns++];
  a->dyna.actor.update = Update;
  Init(&a->dyna.actor, p);
  return &a->dyna.actor;
}
static std::shared_ptr<Fast::DisplayList> NativeBackdrop() {
  auto l = std::make_shared<Fast::DisplayList>();
  l->Instructions.resize(45);
  auto set = [&](size_t i, uintptr_t a, uintptr_t b) {
    l->Instructions[i].words.w0 = a;
    l->Instructions[i].words.w1 = b;
  };
  set(25, 0x3200600c, 0);
  set(26, 0x5e7003a0, 0x9feb4255);
  set(40, 0x3200600c, 0x60);
  set(41, 0x5e7003a0, 0x9feb4255);
  set(27, 0x06000204, 0x00000406);
  set(28, 0x06080a00, 0x00080006);
  set(42, 0x06000204, 0x00000406);
  set(43, 0x0604080a, 0x00040a06);
  set(44, 0xdf000000, 0);
  return l;
}
// Exercise the actual Build output: preserve 8 raw UV units per world unit,
// one shared S origin, and the original 32-pixel repeating tile phase. Derive
// the origin from the first packed vertex so this accepts any safe whole-tile
// origin, but catches overflow or independently wrapping individual vertices.
static void CheckDrawUVs() {
  const int originS = int(kTriangles[0].a.x * 8) - sDrawVertices[0].v.tc[0];
  assert(originS % (32 * 32) == 0);
  for (size_t i = 0; i < kTriangles.size(); ++i) {
    const auto &t = kTriangles[i];
    const Point points[] = {t.a, t.b, t.c};
    for (size_t j = 0; j < 3; ++j) {
      const auto &v = sDrawVertices[i * 3 + j].v;
      const float raw[] = {points[j].x * 8 - originS,
                           (t.floor ? points[j].z : points[j].y) * 8};
      for (size_t axis = 0; axis < 2; ++axis) {
        if (raw[axis] < std::numeric_limits<s16>::min() ||
            raw[axis] > std::numeric_limits<s16>::max())
          std::cerr << "UV overflow: triangle " << i << " vertex " << j
                    << " axis " << axis << " raw " << raw[axis] << " packed "
                    << v.tc[axis] << std::endl;
        assert(raw[axis] >= std::numeric_limits<s16>::min() &&
               raw[axis] <= std::numeric_limits<s16>::max());
        assert(v.tc[axis] == raw[axis]);
      }
      const size_t next = (j + 1) % 3;
      const auto &other = sDrawVertices[i * 3 + next].v;
      assert(int(other.tc[0]) - v.tc[0] == (points[next].x - points[j].x) * 8);
      assert(int(other.tc[1]) - v.tc[1] ==
             (t.floor ? points[next].z - points[j].z
                      : points[next].y - points[j].y) *
                 8);
      for (size_t previous = 0; previous < i * 3 + j; ++previous) {
        const auto &shared = sDrawVertices[previous].v;
        if (shared.ob[0] == v.ob[0] && shared.ob[1] == v.ob[1] &&
            shared.ob[2] == v.ob[2]) {
          assert(shared.tc[0] == v.tc[0]);
          // Floors project T onto Z; walls project it onto Y.
          if (kTriangles[previous / 3].floor == t.floor)
            assert(shared.tc[1] == v.tc[1]);
        }
      }
    }
  }
  // Both terminal surfaces share the x3684 -> x4121 edge. Its signed
  // interpolation delta is +3496, never the overflowed -62040.
  assert(!kTriangles[17].floor && kTriangles[41].floor);
  assert(int(sDrawVertices[17 * 3 + 2].v.tc[0]) -
             sDrawVertices[17 * 3 + 1].v.tc[0] ==
         3496);
  assert(int(sDrawVertices[41 * 3].v.tc[0]) -
             sDrawVertices[41 * 3 + 1].v.tc[0] ==
         3496);
  std::cout
      << "PASS actual valley Build UVs: all 129 vertices in signed16 range, "
         "all edge deltas and shared vertices, terminal side17/floor41, "
         "whole-tile phase preserved\n";
}
int main() {
  constexpr Point pts[] = {
      {2008, 145, 50},   {2049, 250, 448},  {2052, 718, 488},
      {2008, 614, 50},   {1969, 716, -382}, {1967, 250, -348},
      {1931, 961, -408}, {1928, 565, -370}, {1968, 462, 50},
      {1968, 862, 50},   {2012, 566, 488},  {2013, 966, 497}};
  auto vertices = std::make_shared<Fast::Vertex>();
  vertices->VertexList.resize(12);
  for (size_t i = 0; i < 12; ++i) {
    auto &v = vertices->VertexList[i].v;
    v.ob[0] = pts[i].x;
    v.ob[1] = pts[i].y;
    v.ob[2] = pts[i].z;
  }
  resources["scenes/shared/spot08_scene/spot08_room_0Vtx_0023F8"] = vertices;
  auto first = NativeBackdrop();
  resources[kBackdrop] = first;
  foreignActive = true;
  assert(PrepareBackdrop() && !sDetailedRender &&
         "OoT backdrop preparation must use its registered owner while MM is active");
  assert(foreignActive && "owner lookup must not change the globally active manager");
  foreignActive = false;
  for (auto slot : kBackdropSlots)
    assert((first->Instructions[slot].words.w0 >> 24) == G_SPNOOP);
  assert(PrepareBackdrop());
  // MM mounts a low-priority OoT companion in real combined sessions. It can
  // own an identically named but distinct native list: never patch that copy.
  RestoreBackdrop();
  auto companionBackdrop = NativeBackdrop();
  foreignResources[kBackdrop] = companionBackdrop;
  foreignResources["scenes/shared/spot08_scene/spot08_room_0Vtx_0023F8"] = vertices;
  foreignActive = true;
  assert(PrepareBackdrop() && NativeBackdropLayout(companionBackdrop->Instructions));
  for (auto slot : kBackdropSlots)
    assert((first->Instructions[slot].words.w0 >> 24) == G_SPNOOP);
  foreignActive = false;
  auto second = NativeBackdrop();
  resources[kBackdrop] = second;
  assert(PrepareBackdrop());
  assert(NativeBackdropLayout(first->Instructions));
  RestoreBackdrop();
  assert(NativeBackdropLayout(second->Instructions));
  RenderBackdropVertices(*second);
  const auto vertexBase = reinterpret_cast<uintptr_t>(vertices->GetPointer());
  assert(second->Instructions[25].words.w1 == vertexBase);
  assert(second->Instructions[40].words.w1 == vertexBase + 0x60);
  assert(PrepareBackdrop() && !sDetailedRender &&
         "the renderer's cached vertex operands must not hide Snowhead");
  RestoreBackdrop();
  vertices->VertexList[0].v.ob[0]++;
  assert(!PrepareBackdrop() && NativeBackdropLayout(second->Instructions, vertexBase));
  vertices->VertexList[0].v.ob[0]--;
  auto foreign = NativeBackdrop();
  foreign->Instructions[27].words.w0++;
  resources[kBackdrop] = foreign;
  assert(!PrepareBackdrop());
  auto detailed = std::make_shared<Fast::DisplayList>();
  Gfx call{};
  const char *leaf = "scenes/shared/spot08_scene/spot08_room_0DL_002538_tri_0";
  call.words.w0 = 0x27000000;
  call.words.w1 = reinterpret_cast<uintptr_t>(leaf);
  detailed->Instructions.push_back(call);
  auto distant = std::make_shared<Fast::Vertex>();
  distant->VertexList.resize(3);
  for (auto &v : distant->VertexList)
    v.v.ob[1] = 1843;
  resources["scenes/shared/spot08_scene/spot08_room_0DL_002538_vtx_0"] =
      distant;
  resources[kBackdrop] = detailed;
  foreignActive = true;
  assert(PrepareBackdrop() && sDetailedRender);
  assert(foreignActive);
  foreignActive = false;
  distant->VertexList[0].v.ob[1] = 300;
  assert(!PrepareBackdrop());
  resources[kBackdrop] = NativeBackdrop();
  assert(PrepareBackdrop());
  CollisionHeader native{};
  Vec3s nativeVertices[3] = {
      {-1000, 0, -1000}, {-1100, 0, -1000}, {-1000, 0, -1100}};
  CollisionPoly nativePoly{};
  nativePoly.flags_vIB = 1;
  nativePoly.vIC = 2;
  nativePoly.type = 7;
  SurfaceType nativeSurfaces[8]{};
  nativeSurfaces[7].data[0] = 0x12345678;
  WaterBox nativeWater{};
  nativeWater.ySurface = 20;
  CamData nativeCamera{};
  nativeCamera.cameraSType = CAM_SET_NORMAL0;
  native.numVertices = 3;
  native.vtxList = nativeVertices;
  native.numPolygons = 1;
  native.polyList = &nativePoly;
  native.surfaceTypeList = nativeSurfaces;
  native.numWaterBoxes = 1;
  native.waterBoxes = &nativeWater;
  native.cameraDataList = &nativeCamera;
  native.cameraDataListLen = 1;
  const auto savedNative = native;
  const auto savedPoly = nativePoly;
  const auto savedWater = nativeWater;
  const auto savedCamera = nativeCamera;
  Build(&native);
  CheckDrawUVs();
  assert(sCollision.numPolygons == 43 && sCollision.numVertices == 129);
  assert(sCollision.numWaterBoxes == 0 && sCollision.waterBoxes == nullptr);
  assert(sCollision.cameraDataListLen == 1 &&
         sCollision.cameraDataList[0].cameraSType == CAM_SET_NONE);
  for (size_t i = 0; i < 43; ++i) {
    assert(sPolys[i].type == 0);
    for (size_t j = 0; j < 3; ++j) {
      assert(sVertices[i * 3 + j].y == sDrawVertices[i * 3 + j].v.ob[1]);
    }
  }
  std::vector<Vec3s> donorVertices(sVertices.begin(), sVertices.end());
  std::vector<CollisionPoly> donorPolys(sPolys.begin(), sPolys.end());
  CollisionHeader donor = sCollision;
  donor.vtxList = donorVertices.data();
  donor.polyList = donorPolys.data();
  const auto before = donor;
  Build(&donor);
  assert(sCollision.numPolygons == 0);
  assert(std::memcmp(&before, &donor, sizeof donor) == 0);
  donor.numPolygons = 21;
  Build(&donor);
  assert(sCollision.numPolygons == 22);
  donor.numPolygons = 43;
  PlayState play{};
  play.sceneNum = SCENE_ZORAS_FOUNTAIN;
  play.colCtx.colHeader = &native;
  gSaveContext.gameMode = GAMEMODE_NORMAL;
  gSaveContext.fileNum = 0;
  assert(ComboFountainValley_Ready(&play));
  assert(!reports.empty() && reports.back().find("status=native-patched") != std::string::npos);
  const auto reportCount = reports.size();
  assert(ComboFountainValley_Ready(&play) && reports.size() == reportCount &&
         "stable readiness must not log every frame");
  assert(spawns == 1 && sCollision.numPolygons == 43);
  assert(ComboFountainValley_Ready(&play) && spawns == 1);
  auto *collisionBuffer = sCollision.polyList;
  auto *cameraBuffer = sCollision.cameraDataList;
  Destroy(&sActor->dyna.actor, &play);
  assert(!sActor);
  resources[kBackdrop] = foreign;
  assert(!ComboFountainValley_Ready(&play));
  assert(reports.back().find("status=unrecognized-display-list") != std::string::npos);
  assert(spawns == 1 && "a layout rejection must remain non-destructive");
  resources[kBackdrop] = NativeBackdrop();
  assert(collisionBuffer[42].type == 0 &&
         cameraBuffer[0].cameraSType == CAM_SET_NONE);
  PlayState alt{};
  alt.sceneNum = SCENE_ZORAS_FOUNTAIN;
  alt.colCtx.colHeader = &donor;
  assert(ComboFountainValley_Ready(&alt));
  assert(sCollision.numPolygons == 0 && sActor->dyna.bgId == BG_ACTOR_MAX);
  Destroy(&sActor->dyna.actor, &alt);
  PlayState full{};
  full.sceneNum = SCENE_ZORAS_FOUNTAIN;
  full.colCtx.colHeader = &native;
  for (auto &flags : full.colCtx.dyna.bgActorFlags)
    flags = 1;
  auto allocationFailureBackdrop = NativeBackdrop();
  resources[kBackdrop] = allocationFailureBackdrop;
  assert(!ComboFountainValley_Ready(&full));
  assert(!sActor);
  assert(reports.back().find("status=collision-actor-unavailable") != std::string::npos);
  assert(NativeBackdropLayout(allocationFailureBackdrop->Instructions) &&
         "do not expose valley without registered collision");
  gComboGameMode = COMBO_GAME_OOT;
  assert(!ComboFountainValley_Ready(&play));
  assert(std::memcmp(&native, &savedNative, sizeof native) == 0);
  assert(std::memcmp(&nativePoly, &savedPoly, sizeof nativePoly) == 0);
  assert(std::memcmp(&nativeWater, &savedWater, sizeof nativeWater) == 0);
  assert(std::memcmp(&nativeCamera, &savedCamera, sizeof nativeCamera) == 0);
  assert(nativeSurfaces[7].data[0] == 0x12345678);
  RestoreBackdrop();
  resources.clear();
  foreignResources.clear();
  std::cout << "PASS registered OoT owner selection with foreign active manager, "
               "state-change readiness diagnostics, native collision registration, subset deduplication, "
               "unchanged scene data, load/destroy/reload, allocation failure "
               "and backdrop restore/reload/foreign guards\n";
}
