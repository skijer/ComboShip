#include "actors/ForestGiParticles.h"
#include "actors/RegionalPassagePresentation.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>

int main() {
  const NeiGi::Basis cameras[] = {
      {},
      {{0, 0, -1}, {0, 1, 0}, {1, 0, 0}},
      {{1, 0, 0}, {0, .70710678f, .70710678f}, {0, -.70710678f, .70710678f}}};
  struct Palette {
    int route;
    uint32_t outer;
  };
  // User palette requests change the outer hue only, including on tilted
  // cameras.
  for (auto palette :
       {Palette{5, 0xE8A13A}, Palette{6, 0x174A9C}, Palette{7, 0xA8DFFF}}) {
    for (uint32_t frame : {0u, 1u, 75u, 179u, 100000u, 0xffffffffu}) {
      for (const auto &camera : cameras) {
        const auto forest = ForestGiParticles::Mystery(frame, camera);
        const auto regional =
            RegionalPassagePresentation::Shimmer(frame, palette.route, camera);
        assert(
            regional.count == 1440 &&
            "regional passage must emit the actual 20 complete NEI twinkles");
        assert(regional.count == forest.count);
        unsigned outer = 0, core = 0;
        for (size_t i = 0; i < forest.count; ++i) {
          const auto &old = forest.vertices[i];
          const auto &now = regional.vertices[i];
          assert(now.p.x == old.p.x && now.p.y == old.p.y &&
                 now.p.z == old.p.z);
          assert(now.alpha == old.alpha && now.u == old.u && now.v == old.v);
          const bool colored = old.rgb == 0x5AC85A;
          assert(now.rgb == (colored ? palette.outer : old.rgb));
          outer += colored;
          core += !colored;
        }
        assert(outer && core);
        const auto untouched = ForestGiParticles::Mystery(frame, camera);
        for (size_t i = 0; i < forest.count; ++i)
          assert(untouched.vertices[i].rgb == forest.vertices[i].rgb);
      }
    }
  }
  assert(RegionalPassagePresentation::Shimmer(0, 0, {}).count == 0 &&
         "foreign routes must stay passive");
  // The native stone must beckon before pulling. Move only twinkle centers;
  // their camera-facing offsets keep the exact existing shape and pulse.
  const auto upright = RegionalPassagePresentation::Shimmer(75, 5, cameras[2]);
  const auto grave = RegionalPassagePresentation::GraveShimmer(75, cameras[2]);
  assert(grave.count == 1440);
  for (size_t first = 0; first < grave.count; first += 72) {
    const auto center = grave.vertices[first].p;
    assert(center.y > 54 || std::fabs(center.x) > 40 ||
           std::fabs(center.z) > 42);
    const auto oldCenter = upright.vertices[first].p;
    for (size_t j = 0; j < 72; ++j) {
      const auto &old = upright.vertices[first + j];
      const auto &now = grave.vertices[first + j];
      const auto a = old.p - oldCenter, b = now.p - center;
      assert(std::fabs(a.x - b.x) < .00001f && std::fabs(a.y - b.y) < .00001f &&
             std::fabs(a.z - b.z) < .00001f);
      assert(now.rgb == old.rgb && now.alpha == old.alpha);
    }
  }
  const auto shell = RegionalPassagePresentation::BuildShell();
  assert(shell.size() >= 92);
  float minX = 999, minY = 999, minZ = 999, maxX = -999, maxY = -999,
        maxZ = -999;
  for (const auto &v : shell) {
    minX = std::min(minX, v.p.x);
    maxX = std::max(maxX, v.p.x);
    minY = std::min(minY, v.p.y);
    maxY = std::max(maxY, v.p.y);
    minZ = std::min(minZ, v.p.z);
    maxZ = std::max(maxZ, v.p.z);
  }
  assert(minX == -68 && maxX == 68 && minY == 0 && maxY == 133 && minZ == -45 &&
         maxZ == 0);
  unsigned dark = 0;
  for (const auto &v : shell) {
    if (v.shade != 0)
      continue;
    ++dark;
    assert(v.p.z == -1 && std::fabs(v.p.x) <= 60 && v.p.y >= 0);
    assert(v.p.y <=
               66 + std::sqrt(std::max(0.f, 3600 - v.p.x * v.p.x)) + .001f &&
           "a rectangular black cap must not spill outside the arched opening");
  }
  assert(dark > 4);
  assert(RegionalPassagePresentation::BuildShell()[2].p.x == -60);
  puts("PASS regional outer palettes preserve every NEI vertex, UV, core, "
       "alpha, phase and forest default");
}
