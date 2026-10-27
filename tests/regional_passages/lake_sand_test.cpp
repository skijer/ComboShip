#include "entrances/LakeHyliaBridge.h"
#if __has_include("entrances/LakeHyliaSurface.h")
#include "entrances/LakeHyliaSurface.h"
#else
// Read-only adapter for the old provider's single-material draw output.
namespace ComboLakeHylia {
struct SurfaceGeometry {
  std::vector<Quad> stone, sand;
};
inline SurfaceGeometry BuildSurfaceGeometry() {
  return {BuildGeometry().solid, {}};
}
} // namespace ComboLakeHylia
#endif
#include <cassert>
#include <cstdio>
#include <cstring>
#include <limits>
using namespace ComboLakeHylia;
static bool Floor(const Quad &q) {
  return q.a.y == -1305 && q.b.y == -1305 && q.c.y == -1305 && q.d.y == -1305;
}
static float Area(const Quad &q) {
  const Point p[] = {q.a, q.b, q.c, q.d};
  float result = 0;
  for (int i = 0; i < 4; ++i)
    result += p[i].x * p[(i + 1) % 4].z - p[(i + 1) % 4].x * p[i].z;
  return std::fabs(result) * .5f;
}
static bool Contains(const Quad &q, float x, float z) {
  const Point p[] = {q.a, q.b, q.c, q.d};
  bool positive = false, negative = false;
  for (int i = 0; i < 4; ++i) {
    const auto a = p[i], b = p[(i + 1) % 4];
    const float cross = (b.x - a.x) * (z - a.z) - (b.z - a.z) * (x - a.x);
    positive |= cross > .001f;
    negative |= cross < -.001f;
  }
  return !(positive && negative);
}
int main(int argc, char **argv) {
  const auto original = BuildGeometry();
  const auto surface = BuildSurfaceGeometry();
  if (argc > 1 && std::strcmp(argv[1], "cap") == 0) {
    const Quad *cap = nullptr;
    for (const auto &q : surface.stone)
      if (q.shade == 0)
        cap = &q;
    assert(cap);
    float lowZ = 1e9f, highZ = -1e9f, lowY = 1e9f, highY = -1e9f;
    for (auto p : {cap->a, cap->b, cap->c, cap->d}) {
      lowZ = std::min(lowZ, p.z);
      highZ = std::max(highZ, p.z);
      lowY = std::min(lowY, p.y);
      highY = std::max(highY, p.y);
    }
    assert(lowZ < 3627 && highZ > 3807 && lowY < -1305 && highY > -1089 &&
           "black backdrop must overlap the complete opening instead of "
           "leaving a right-edge sliver");
    puts("PASS visual backdrop overlaps opening; original collision cap "
         "remains separate");
    return 0;
  }
  assert(!surface.sand.empty() && "stream banks need a distinct sand region");
  float originalArea = 0, sandArea = 0, stoneFloorArea = 0;
  for (const auto &q : original.solid)
    if (Floor(q))
      originalArea += Area(q);
  for (const auto &q : surface.sand) {
    assert(Floor(q) && Area(q) > 0);
    sandArea += Area(q);
  }
  for (const auto &q : surface.stone)
    if (Floor(q))
      stoneFloorArea += Area(q);
  assert(std::fabs(sandArea + stoneFloorArea - originalArea) < .05f &&
         "split must preserve the original floor area");
  assert(sandArea / originalArea > .42f && sandArea / originalArea < .58f &&
         "sand should occupy about half of the dry banks");
  unsigned samples = 0;
  for (float x = -2236.73f; x < -1825; x += 3.731f) {
    const auto channel = ChannelAt(x);
    for (float z = 3615.31f; z < 3815; z += 2.371f) {
      bool wasFloor = false;
      for (const auto &q : original.solid)
        wasFloor |= Floor(q) && Contains(q, x, z);
      unsigned hits = 0, sandHits = 0;
      for (const auto &q : surface.stone)
        hits += Floor(q) && Contains(q, x, z);
      for (const auto &q : surface.sand) {
        const bool hit = Contains(q, x, z);
        hits += hit;
        sandHits += hit;
      }
      assert(hits == unsigned(wasFloor) &&
             "surface partition must not create gaps, overlaps or cover the "
             "stream");
      if (z > channel.nearZ + .01f && z < channel.farZ - .01f)
        assert(!sandHits);
      ++samples;
    }
  }
  float minEdge = 1e9f, maxEdge = -1e9f;
  for (const auto &q : surface.sand)
    for (auto p : {q.a, q.b, q.c, q.d})
      if (p.z < 3730) {
        minEdge = std::min(minEdge, p.z);
        maxEdge = std::max(maxEdge, p.z);
      }
  assert(
      maxEdge - minEdge > 15 &&
      "sand/stone boundary must wander instead of forming a straight stripe");
  printf("PASS %.1f%% sand, %u dry-bank/stream samples, no gaps or overlap, "
         "irregular boundary\n",
         sandArea / originalArea * 100, samples);
}
