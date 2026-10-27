#ifndef COMBO_LAKE_NATIVE_TEST
#include "entrances/LakeHyliaBridge.h"
#include "lake_scene_evidence.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>
using namespace ComboLakeHylia;
static float Floor(const std::vector<Triangle> &mesh, float x, float z,
                   float ceiling) {
  float result = -32768;
  for (const auto &t : mesh) {
    const auto a = t.a, b = t.b, c = t.c;
    const float det = (b.x - a.x) * (c.z - a.z) - (c.x - a.x) * (b.z - a.z);
    if (std::fabs(det) < .001f)
      continue;
    const float u = ((x - a.x) * (c.z - a.z) - (c.x - a.x) * (z - a.z)) / det;
    const float v = ((b.x - a.x) * (z - a.z) - (x - a.x) * (b.z - a.z)) / det;
    const float y = a.y + u * (b.y - a.y) + v * (c.y - a.y);
    if (u >= -.001f && v >= -.001f && u + v <= 1.001f && y <= ceiling &&
        y > result)
      result = y;
  }
  return result;
}
static Point Sub(Point a, Point b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
static Point Cross(Point a, Point b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
static float Dot(Point a, Point b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static bool Clear(const std::vector<Triangle> &mesh, Point from, Point to) {
  auto direction = Sub(to, from);
  for (const auto &triangle : mesh) {
    auto e1 = Sub(triangle.b, triangle.a), e2 = Sub(triangle.c, triangle.a),
         h = Cross(direction, e2);
    float det = Dot(e1, h);
    if (std::fabs(det) < .0001f)
      continue;
    auto s = Sub(from, triangle.a), q = Cross(s, e1);
    float u = Dot(s, h) / det, v = Dot(direction, q) / det,
          t = Dot(e2, q) / det;
    if (u >= -.0001f && v >= -.0001f && u + v <= 1.0001f && t > .0001f &&
        t < .9999f)
      return false;
  }
  return true;
}
static void Walk(const std::vector<Triangle> &native,
                 const std::vector<Triangle> &injected, Point a, Point b) {
  const int steps = std::ceil(std::hypot(b.x - a.x, b.z - a.z) / 8);
  const float length = std::hypot(b.x - a.x, b.z - a.z),
              rx = (b.z - a.z) / length, rz = -(b.x - a.x) / length;
  for (float width : {-20.f, 0.f, 20.f}) {
    float previous = a.y;
    Point last{};
    for (int i = 0; i <= steps; ++i) {
      float t = float(i) / steps, x = a.x + (b.x - a.x) * t + rx * width,
            z = a.z + (b.z - a.z) * t + rz * width;
      float expected = a.y + (b.y - a.y) * t;
      float y = std::fmax(Floor(native, x, z, expected + 30),
                          Floor(injected, x, z, expected + 30));
      if (!(std::fabs(y - expected) < 22 &&
            (!i || std::fabs(y - previous) < 16)))
        std::fprintf(stderr, "walk fail %f,%f expected%f got%f previous%f\n", x,
                     z, expected, y, previous);
      assert(std::fabs(y - expected) < 22 &&
             (!i || std::fabs(y - previous) < 16));
      assert(Clear(native, {x, y + 1, z}, {x, y + 140, z}));
      assert(Clear(injected, {x, y + 1, z}, {x, y + 140, z}));
      if (i)
        for (float height : {20.f, 80.f, 140.f}) {
          assert(Clear(native, {last.x, last.y + height, last.z},
                       {x, y + height, z}));
          assert(Clear(injected, {last.x, last.y + height, last.z},
                       {x, y + height, z}));
        }
      previous = y;
      last = {x, y, z};
    }
  }
}
int main() {
  assert(kMouth.x == -2185 && kMouth.y == -1305 && kMouth.z == 3717 &&
         kYaw == 16384);
  assert(OpeningTopAt(0) == 216 && OpeningTopAt(52) == 190 &&
         OpeningTopAt(-52) == 190 && OpeningTopAt(90) == 150 &&
         OpeningTopAt(-90) == 150 && OpeningTopAt(91) == 0);
  assert(kMouth.x + kShimmerFront == kBackX + 10);
  auto mesh = BuildGeometry();
  assert(mesh.solid.size() > 60 && mesh.water.size() >= 6);
  assert(mesh.water.front().a.y > mesh.water.back().c.y + 1 &&
         "the stream must descend from the cave to the receiving lake");
  bool outsideSpill = false;
  bool verticalCurtain = false;
  for (const auto& q : mesh.water) {
    assert(q.d.x >= q.a.x && q.d.y <= q.a.y + .0001f && q.c.y == q.d.y && q.a.y == q.b.y);
    outsideSpill |= q.a.x >= -1825 && q.d.y < q.a.y - .01f;
    verticalCurtain |= q.a.x > -1825 && std::fabs(q.d.x - q.a.x) < .001f && q.d.y < q.a.y - 1;
    assert(q.a.x <= -1823 && q.d.x <= -1823 && "the outlet must fall at the edge, not extend into a runout");
  }
  assert(outsideSpill && "a falling outflow must extend beyond the authored front lip");
  assert(verticalCurtain && "the water must turn downward over the front edge into a vertical curtain");
  assert(std::fabs(mesh.water.back().c.y - kFullWaterY) < .001f &&
         "the outflow must join the actual lake level");
  std::vector<Triangle> injected;
  for (const auto &q : mesh.solid)
    if (q.collides) {
      injected.push_back({q.a, q.b, q.c});
      injected.push_back({q.a, q.c, q.d});
    }
  assert(Clear(injected, {-1800, -1311, 3757}, {-1840, -1311, 3757}) &&
         "the outward stream must pass through an open outlet to the lake");
  for (float z : {3739.f, 3809.f})
    assert(!Clear(injected, {-1824, -1350, z}, {-1826, -1350, z}) &&
           "the intake bevel must close against the solid front below the water slot");
  float widestWater = 0;
  for (const auto& q : mesh.water)
    widestWater = std::max(widestWater, std::fabs(q.b.z - q.a.z));
  assert(widestWater >= 36 && "the inlet must read as a watercourse rather than a twenty-unit stripe");
  for (const auto& q : mesh.water)
    for (float along : {0.f, .25f, .5f, .75f, 1.f})
      for (float across : {.01f, .99f}) {
        const float x = q.a.x + (q.d.x - q.a.x) * along;
        const float nearZ = q.a.z + (q.d.z - q.a.z) * along;
        const float farZ = q.b.z + (q.c.z - q.b.z) * along;
        const float z = nearZ + (farZ - nearZ) * across;
        assert(Floor(injected, x, z, kMouth.y + 1) <= q.a.y + .1f &&
               "water must remain inside its actual solid banks at every bend");
      }
  assert(Floor(injected, -1815, 3975, -1000) == -32768 &&
         "bank switchback and giant side-support assembly must be absent");
  for (const auto &q : mesh.solid)
    for (auto p : {q.a, q.b, q.c, q.d})
      assert(p.z <= 3840 && p.y >= -1405 &&
             "only the modest cave/shore plinth may remain");
  bool frontIsVertical = false;
  for (const auto &q : mesh.solid)
    frontIsVertical |=
        q.a.x == -1825 && q.b.x == -1825 && q.c.x == -1825 && q.d.x == -1825 &&
        std::fmin(std::fmin(q.a.y, q.b.y), std::fmin(q.c.y, q.d.y)) == -1405;
  assert(frontIsVertical && "native ledge-grab requires a vertical front edge");
  bool collarJoined = false;
  for (const auto &q : mesh.solid)
    if (q.shade == 140)
      for (const auto p : {q.a, q.b, q.c, q.d})
        collarJoined |= p.x <= -2270;
  assert(collarJoined &&
         "outer rock must overlap native bank instead of leaving rear gap");
  float lowestFootingSupport = 32767;
  int footingSamples = 0;
  // Real native and Alt collision, including the marked bank and native lab.
  for (const auto *evidence : kSceneVariants) {
    assert(evidence->size() >= 33);
    // Check every footing base edge, not just one representative center.
    for (const auto &q : mesh.solid) {
      const Point points[] = {q.a, q.b, q.c, q.d, q.a};
      for (int edge = 0; edge < 4; ++edge)
        if (points[edge].y == kPlinthBaseY &&
            points[edge + 1].y == kPlinthBaseY) {
          auto a = points[edge], b = points[edge + 1];
          int count = std::ceil(std::hypot(b.x - a.x, b.z - a.z) / 8);
          for (int i = 0; i <= count; ++i) {
            float t = count ? float(i) / count : 0;
            const float support = Floor(*evidence, a.x + (b.x - a.x) * t,
                                        a.z + (b.z - a.z) * t, -800);
            assert(support > kPlinthBaseY);
            lowestFootingSupport = std::fmin(lowestFootingSupport, support);
            ++footingSamples;
          }
        }
    }

    for (const auto &q : mesh.solid)
      if (q.shade == 140)
        for (const auto p : {q.a, q.b, q.c, q.d})
          if (p.x == kRockJoinX && p.y > kMouth.y + 20)
            assert(!Clear(*evidence, p, {-2190, p.y, p.z}) &&
                   "outer collar must cross measured bank collision");
    assert(std::fabs(Floor(*evidence, -2270, 3770, -1000) + 1033) < 1);
    // A shallow native north-shore lip remains available in drained setups.
    // This is measured source clearance, not a claim of runtime swim/climb
    // proof.
    for (float x : {-1900.f, -1875.f, -1850.f, -1825.f}) {
      const float basin = Floor(*evidence, x, 3615, kMouth.y);
      assert(kMouth.y - basin > 10 && kMouth.y - basin < 20);
      const float shore = Floor(*evidence, x, 3590, kMouth.y + 30);
      assert(Clear(*evidence, {x, shore + 1, 3590}, {x, shore + 141, 3590}));
      assert(Floor(injected, x, 3630, kMouth.y + 1) == kMouth.y);
    }
    Walk(*evidence, injected, kApron, kMouth);
    Walk(*evidence, injected, kMouth, {kMouth.x + 100, kMouth.y, kMouth.z});
    // The widened south-side channel occupies the former decorative rim.
    // Both directions of the real dry route retain a full body-width reserve.
    Walk(*evidence, injected, kMouth, kApron);
    // Deep tunnel is wholly lakeward of intact bank x=-2245; the lab is above.
    assert(kBackX > -2245 && kFacadeX - kBackX >= 300);
    for (float x : {-2185.f, -2085.f, -1985.f}) {
      assert(Floor(*evidence, x, 3717, -1000) < kMouth.y);
      assert(Floor(injected, x, 3717, kMouth.y + 1) == kMouth.y);
    }
  }
  std::printf(
      "PASS grounded footing: %d source base-edge samples, lowest native "
      "support %.2f above base -1405; rear collar overlaps bank\n",
      footingSamples, lowestFootingSupport);
  // Full/refilled lake has an8-unit dry ledge. Drained access meets the
  // measured native shore; the former direct lab-bank ramp no longer exists.
  assert(kMouth.y - kFullWaterY == 8);
  assert(kMouth.y > kDrainedWaterY && kBank.y > kFullWaterY);
  assert(HasInlet(kFullWaterY));
  assert(!HasInlet(kChannelFloorY) &&
         !HasInlet(std::numeric_limits<float>::quiet_NaN()));
  assert(!HasInlet(kDrainedWaterY));
  assert(StreamHeight(kFullWaterY) == kFullWaterY);
  assert(StreamHeight(kDrainedWaterY) < kMouth.y &&
         StreamHeight(kDrainedWaterY) > kChannelFloorY);
  assert(StreamHeight(kFullWaterY) < kMouth.y);
  for (const auto &q : mesh.water)
    assert(!q.collides);
  std::printf(
      "PASS lake bridge: %s, no bank ramps, measured shallow shore lip, "
      "dry width40 return,140-unit headroom,322-unit cave, both "
      "water levels/drained inlet gate and safe return\n",
      kSceneEvidenceLabel);
}

#else

#include "ComboGameMode.h"
#include "entrances/LakeHyliaBridge.h"
#include "entrances/LakeHyliaSurface.h"
#include "entrances/LostWoodsEntrance.h"
#include "global.h"
#include "soh/Enhancements/Gameplay/ComboLakeHyliaBridge.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string>
#include <type_traits>
#include <vector>
SaveContext gSaveContext{};
PlayState *gPlayState;
uintptr_t gSegments[16]{};
extern "C" {
int gComboGameMode = COMBO_GAME_BOTH;
}
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
  } entry{};
  ActorDBInit profile;
  static ActorDB *Instance;
  Entry &AddEntry(const ActorDBInit &p) {
    profile = p;
    entry.entry.id = 9001;
    return entry;
  }
};
ActorDB database;
ActorDB *ActorDB::Instance = &database;
struct GameInteractor {
  struct OnSceneInit {};
  struct OnPlayDestroy {};
  struct OnPlayerUpdate {};
  static GameInteractor *Instance;
  std::function<void(s16)> scene;
  std::function<void()> destroy, update;
  template <class Hook, class F> void RegisterGameHook(F f) {
    if constexpr (std::is_same_v<Hook, OnSceneInit>)
      scene = f;
    if constexpr (std::is_same_v<Hook, OnPlayDestroy>)
      destroy = f;
    if constexpr (std::is_same_v<Hook, OnPlayerUpdate>)
      update = f;
  }
};
GameInteractor interactor;
GameInteractor *GameInteractor::Instance = &interactor;
struct RegisterShipInitFunc {
  explicit RegisterShipInitFunc(void (*f)()) { f(); }
};
#undef osSyncPrintf
#define osSyncPrintf(...) ((void)0)
#define SS_NULL 0xFFFF
#define DYNA_RAYCAST_FLOORS 1
#define DYNA_RAYCAST_WALLS 2
#define DYNA_RAYCAST_CEILINGS 4
extern "C" {
// Exact trigonometry for the authored identity actor transform.
f32 Math_SinS(s16 angle) {
  assert(angle == 0);
  return 0;
}
f32 Math_CosS(s16 angle) {
  assert(angle == 0);
  return 1;
}
/* NATIVE_DYNA */
void Actor_SetScale(Actor *a, f32 scale) { a->scale = {scale, scale, scale}; }
void Actor_Kill(Actor *a) { a->update = nullptr; }
}
/* NATIVE_PROVIDER */
static std::vector<Actor *> allocated;
extern "C" Actor *Actor_Spawn(ActorContext *, PlayState *play, s16 id, f32 x,
                              f32 y, f32 z, s16, s16, s16, s16) {
  assert(id == 9001);
  const auto &profile = database.profile;
  auto *actor = static_cast<Actor *>(std::calloc(1, profile.instanceSize));
  actor->world.pos = {x, y, z};
  actor->update = profile.update;
  actor->destroy = profile.destroy;
  actor->draw = profile.draw;
  allocated.push_back(actor);
  profile.init(actor, play);
  return actor;
}
static void Destroy(PlayState &play) {
  for (auto *actor : allocated) {
    actor->destroy(actor, &play);
    std::free(actor);
  }
  allocated.clear();
  interactor.destroy();
}
static void CameraControl(PlayState &play, CollisionHeader *collision) {
  Player player{};
  Camera camera{};
  camera.play = &play;
  camera.player = &player;
  s32 bg = 0;
  const auto index =
      SurfaceType_GetCamDataIndex(&play.colCtx, &collision->polyList[0], bg);
  assert(index == 0);
  // Actual native camera path, deliberately no fixture null guard.
  assert(Camera_GetDataIdxForPoly(&camera, &bg, &collision->polyList[0]) == -1);
  assert(BgCheck_GetBgCamSettingImpl(&play.colCtx, index, bg) == CAM_SET_NONE);
  assert(collision->cameraDataListLen == 1 &&
         collision->cameraDataList[0].cameraSType == CAM_SET_NONE);
  puts("PASS native camera floor query: index0 -> CAM_SET_NONE -> "
       "Camera_GetDataIdxForPoly -1");
}
static void StoneUvControl() {
  const auto &rock = RockVertices();
  const auto& surface = SurfaceGeometry();
  assert(rock.size() == (surface.stone.size() + surface.sand.size()) * 4);
  // Adjacent north-wall rings share exact texture coordinates. Fragment-style
  // per-quad resets create repeated moss bands at every46-unit ring boundary.
  int shared = 0;
  for (size_t i = 0; i < surface.stone.size(); ++i) {
    const auto &q = surface.stone[i];
    if (q.a.z != 3627 || q.b.z != 3627 || q.c.z != 3627 || q.d.z != 3627)
      continue;
    for (size_t j = i + 1; j < surface.stone.size(); ++j) {
      const auto &r = surface.stone[j];
      if (r.a.z != 3627 || r.b.z != 3627 || r.c.z != 3627 || r.d.z != 3627)
        continue;
      for (int a = 0; a < 4; ++a)
        for (int b = 0; b < 4; ++b) {
          const auto &va = rock[i * 4 + a].v, &vb = rock[j * 4 + b].v;
          if (va.ob[0] == vb.ob[0] && va.ob[1] == vb.ob[1] &&
              va.ob[2] == vb.ob[2]) {
            assert(va.tc[0] == vb.tc[0] && va.tc[1] == vb.tc[1] &&
                   "native stone UVs must continue across cave rings");
            ++shared;
          }
        }
    }
  }
  assert(shared >= 12);
  puts("PASS native stone material: continuous UVs across adjacent cave rings");
}
static void ShellControl(PlayState &play, CollisionHeader *collision) {
  auto &dyna = play.colCtx.dyna;
  auto &lookup = dyna.bgActors[0].dynaLookup;
  std::vector<CollisionPoly> polys(collision->numPolygons);
  std::vector<Vec3s> vertices(collision->numVertices);
  std::vector<SSNode> nodes(collision->numPolygons * 2);
  dyna.polyList = polys.data();
  dyna.polyListMax = polys.size();
  dyna.vtxList = vertices.data();
  dyna.vtxListMax = vertices.size();
  dyna.polyNodes.tbl = nodes.data();
  dyna.polyNodes.max = nodes.size();
  auto contains = [&](const SSList &list, int poly) {
    for (auto id = list.head; id != SS_NULL; id = dyna.polyNodes.tbl[id].next)
      if (dyna.polyNodes.tbl[id].polyId == poly)
        return true;
    return false;
  };
  for (int cached = 0; cached < 2; ++cached) {
    lookup.floor.head = lookup.wall.head = lookup.ceiling.head = SS_NULL;
    dyna.polyNodes.count = 0;
    s32 vtx = 0, poly = 0;
    if (cached) {
      dyna.bitFlag &= ~DYNAPOLY_INVALIDATE_LOOKUP;
      dyna.bgActors[0].prevTransform = dyna.bgActors[0].curTransform;
    }
    // Actual native cold transform/recomputed normals and cached classifier.
    DynaPoly_ExpandSRT(&play, &dyna, 0, &vtx, &poly);
    assert(vtx == collision->numVertices && poly == collision->numPolygons);
    for (const Vec3f point :
         {Vec3f{-2100, -1230, 3631}, Vec3f{-2100, -1230, 3803},
          Vec3f{-2233, -1230, 3717}}) {
      Vec3f pos = point;
      float x = point.x, z = point.z;
      CollisionPoly *hit = nullptr;
      s32 bg = -1;
      const bool collided = BgCheck_SphVsDynaWallInBgActor(
          &play.colCtx, 0, &dyna, &lookup.wall, &x, &z, &hit, &bg, &pos, 18, 0);
      if (!collided)
        std::fprintf(stderr, "native contact ignored at %f,%f,%f\n", point.x,
                     point.y, point.z);
      assert(collided);
      assert(hit && bg == 0);
      // Native SRT truncates signed plane distance; allow1.2 world units
      // against authored coordinates while requiring an inward radius18
      // response.
      if (point.z == 3631) {
        if (!(z >= 3643.8f && z < 3717))
          std::fprintf(stderr, "native north contact pushed z%f to %f\n",
                       point.z, z);
        assert(z >= 3643.8f && z < 3717);
      }
      if (point.z == 3803)
        assert(z <= 3790.2f && z > 3717);
      if (point.x == -2233)
        assert(x >= -2220.2f && x < -2100);
    }
    int polygon = 0, innerCeilings = 0, outerFloors = 0;
    for (const auto &q : Geometry().solid) {
      if (!q.collides)
        continue;
      const float lowX =
          std::fmin(std::fmin(q.a.x, q.b.x), std::fmin(q.c.x, q.d.x));
      const float highX =
          std::fmax(std::fmax(q.a.x, q.b.x), std::fmax(q.c.x, q.d.x));
      const float lowY =
          std::fmin(std::fmin(q.a.y, q.b.y), std::fmin(q.c.y, q.d.y));
      const float highY =
          std::fmax(std::fmax(q.a.y, q.b.y), std::fmax(q.c.y, q.d.y));
      const bool vault = highX <= Lake::kFacadeX && highX > lowX &&
                         highY > Lake::kMouth.y && highY > lowY;
      const bool exterior =
          lowX == Lake::kRockJoinX && highX == Lake::kFacadeX + 18;
      const bool reveal =
          lowX == Lake::kFacadeX && highX == Lake::kFacadeX + 18;
      const bool trough = lowY == Lake::kChannelFloorY &&
                          highY == Lake::kMouth.y && q.a.z == q.b.z &&
                          q.b.z == q.c.z;
      const bool footing =
          lowY == Lake::kPlinthBaseY && highY == Lake::kMouth.y;
      const bool cap =
          lowX == Lake::kBackX && highX == lowX && highY > Lake::kMouth.y;
      const float cy = (q.a.y + q.b.y + q.c.y + q.d.y) / 4,
                  cz = (q.a.z + q.b.z + q.c.z + q.d.z) / 4;
      for (int i = 0; i < 2; ++i) {
        const auto &p = polys[polygon + i];
        if (vault || cap) {
          const float facing = p.normal.x * 80 * (cap ? 1 : 0) +
                               p.normal.y * (Lake::kMouth.y + 80 - cy) +
                               p.normal.z * (Lake::kMouth.z - cz);
          assert(facing > 0 && "inner shell must face cavity free space");
          if (p.normal.y < -.8f * 32767) {
            assert(contains(lookup.ceiling, polygon + i));
            ++innerCeilings;
          }
          assert(!contains(lookup.floor, polygon + i));
        }
        if (reveal)
          assert(p.normal.x > 0 && "entrance reveal must face the approach");
        if (trough)
          assert(p.normal.z * (3757 - cz) > 0 &&
                 "trough side must face water channel");
        if (footing) {
          if (q.a.z == q.b.z && q.b.z == q.c.z)
            assert(p.normal.z * (cz - Lake::kMouth.z) > 0);
          else {
            assert(p.normal.x > 0 && "front footing must face the lake");
            assert(std::abs(p.normal.y) < 600 &&
                   "native ledge grab requires a nearly vertical wall");
          }
        }
        if (exterior) {
          const float facing =
              p.normal.y * 100 + p.normal.z * (cz - Lake::kMouth.z);
          assert(facing > 0 && "outer rock must face external free space");
          if (p.normal.y > .5f * 32767) {
            assert(contains(lookup.floor, polygon + i));
            ++outerFloors;
          }
        }
      }
      polygon += 2;
    }
    assert(innerCeilings > 20 && outerFloors >= 4);
    Vec3f probe{-2100, -1230, 3717};
    CollisionPoly *floor = nullptr;
    s32 bg = 0;
    DynaRaycast ray{};
    ray.play = &play;
    ray.colCtx = &play.colCtx;
    ray.resultPoly = &floor;
    ray.yIntersect = BGCHECK_Y_MIN;
    ray.pos = &probe;
    ray.bgId = &bg;
    ray.chkDist = 1;
    ray.dyna = &dyna;
    ray.ssList = &lookup.floor;
    const float dryFloor =
        BgCheck_RaycastFloorDynaList(&ray, DYNA_RAYCAST_FLOORS);
    assert(floor && std::fabs(dryFloor - Lake::kMouth.y) < 1.2f);
    probe.y = -1000;
    floor = nullptr;
    const float externalRoof =
        BgCheck_RaycastFloorDynaList(&ray, DYNA_RAYCAST_FLOORS);
    assert(floor && externalRoof > -1070 && externalRoof < -1045);
    probe.y = -1110;
    float displacedY = probe.y;
    CollisionPoly *ceiling = nullptr;
    assert(BgCheck_CheckDynaCeilingList(&play.colCtx, 0, &dyna, &lookup.ceiling,
                                        &displacedY, &probe, 40, &ceiling));
    assert(ceiling && displacedY < probe.y && displacedY > -1132 &&
           displacedY < -1126);
  }
  dyna.polyList = nullptr;
  dyna.vtxList = nullptr;
  dyna.polyNodes.tbl = nullptr;
  puts("PASS native shell: cold/cached floor-ceiling-wall classification, "
       "actual floor/ceiling queries and inward north/south/back wall "
       "displacement");
}
int main() {
  PlayState play{};
  CollisionHeader native{};
  WaterBox waters[4]{};
  SurfaceType sceneSurface{};
  native.surfaceTypeList = &sceneSurface;
  waters[2] = {-4577, -1313, 3663, 7680, 5627, 258};
  native.waterBoxes = waters;
  native.numWaterBoxes = 4;
  play.sceneNum = SCENE_LAKE_HYLIA;
  play.colCtx.colHeader = &native;
  gPlayState = &play;
  gSaveContext.gameMode = GAMEMODE_NORMAL;
  Vec3f mouth{};
  s16 yaw = 0;
  assert(ComboLakeHyliaBridge_Get(&play, &mouth, &yaw));
  assert(mouth.x == -2185 && mouth.y == -1305 && mouth.z == 3717 &&
         yaw == 16384);
  assert(allocated.size() == 1 && sCave->dyna.bgId == 0);
  interactor.scene(SCENE_LAKE_HYLIA);
  assert(ComboLakeHyliaBridge_Get(&play, &mouth, &yaw) &&
         allocated.size() == 1);
  StoneUvControl();
  auto *collision = play.colCtx.dyna.bgActors[0].colHeader;
  assert(collision == &Collision().header && collision->numPolygons > 100);
  if (COMBO_LAKE_NATIVE_CASE != 2)
    CameraControl(play, collision);
  if (COMBO_LAKE_NATIVE_CASE != 1)
    ShellControl(play, collision);
  assert(!collision->numWaterBoxes && native.waterBoxes == waters &&
         native.numWaterBoxes == 4);
  assert(sCave->dyna.actor.scale.x == 1 &&
         play.colCtx.dyna.bgActors[0].curTransform.pos.x == 0);
  for (int i = 0; i < collision->numVertices; ++i) {
    const auto v = collision->vtxList[i];
    assert(v.x >= collision->minBounds.x && v.x <= collision->maxBounds.x);
    assert(v.y >= collision->minBounds.y && v.y <= collision->maxBounds.y);
    assert(v.z >= collision->minBounds.z && v.z <= collision->maxBounds.z);
  }
  int floors = 0;
  for (int i = 0; i < collision->numPolygons; ++i) {
    const auto &p = collision->polyList[i];
    assert(p.type == 0 && p.vtxData[0] < collision->numVertices &&
           p.vtxData[1] < collision->numVertices &&
           p.vtxData[2] < collision->numVertices);
    if (p.normal.y > 23170)
      ++floors;
  }
  assert(floors > 30);
  for (int frame = 0; frame < 30; ++frame)
    interactor.update();
  assert(allocated.size() == 1); // No duplicate actor/collision registration.
  Destroy(play);
  assert(!sCave && (play.colCtx.dyna.bgActorFlags[0] & 2));
  // Native teardown clears the flagged slot; a new scene gets a fresh context.
  for (int age : {0, 1})
    for (int level : {-1313, -1993, -1313}) {
      play = {};
      play.sceneNum = SCENE_LAKE_HYLIA;
      play.colCtx.colHeader = &native;
      gSaveContext.linkAge = age;
      waters[2].ySurface = level;
      interactor.scene(SCENE_LAKE_HYLIA);
      assert(ComboLakeHyliaBridge_Get(&play, &mouth, &yaw) &&
             allocated.size() == 1);
      assert(play.colCtx.dyna.bgActors[0].colHeader == collision);
      float inletY = -3000;
      WaterBox *selected = nullptr;
      assert(WaterBox_GetSurfaceImpl(&play, &play.colCtx, -1900, 3757, &inletY,
                                     &selected));
      assert(selected == &waters[2] && inletY == level);
      assert(Lake::HasInlet(inletY) == (level == Lake::kFullWaterY));
      assert(waters[2].ySurface == level);
      Destroy(play);
    }
  play = {};
  play.sceneNum = SCENE_LAKE_HYLIA;
  play.colCtx.colHeader = &native;
  for (int mode : {COMBO_GAME_OOT, COMBO_GAME_MM}) {
    gComboGameMode = mode;
    assert(!ComboLakeHyliaBridge_Get(&play, nullptr, nullptr));
  }
  gComboGameMode = COMBO_GAME_BOTH;
  gSaveContext.fileNum = 0xFF;
  assert(!ComboLakeHyliaBridge_Get(&play, nullptr, nullptr));
  gSaveContext.fileNum = 0;
  play.sceneNum = SCENE_GRAVEYARD;
  assert(!ComboLakeHyliaBridge_Get(&play, nullptr, nullptr));
  // Full native dynamic actor pool: provider must not advertise
  // invisible/noncolliding geometry.
  play.sceneNum = SCENE_LAKE_HYLIA;
  for (auto &flag : play.colCtx.dyna.bgActorFlags)
    flag = 1;
  assert(!ComboLakeHyliaBridge_Get(&play, nullptr, nullptr));
  Destroy(play);
  puts("PASS lake native provider: real DynaPoly register/delete, owned "
       "collision, repeated child/adult full/drained/refilled entry, native "
       "water preserved, guards and exhausted pool");
}
#endif
