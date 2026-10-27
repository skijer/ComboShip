#include "soh/Enhancements/randomizer/NeiUsedMagicPolicy.h"
#include <cassert>
#include <iostream>
using namespace NeiUsedMagic;
int main() {
  for (unsigned frame = 0; frame < 180; ++frame) {
    auto ice = SampleProjectile(Kind::Ice, frame, 2, {1, 0, 0});
    float xmin = 100, xmax = -100, width = 0;
    for (size_t i = 0; i < ice.count; ++i) {
      auto v = ice.vertices[i];
      xmin = std::min(xmin, v.p.x);
      xmax = std::max(xmax, v.p.x);
      width = std::max(width, std::max(std::abs(v.p.y), std::abs(v.p.z)));
    }
    assert(ice.count > 0 && ice.count < ice.vertices.size());
    assert(xmax - xmin > 35 && xmax - xmin < 60 &&
           width >
               15); // Compact bright head leaves the actual history visible.
    const Point history[] = {{0, 0, 0},   {-15, 0, 0}, {-30, 0, 0},
                             {-45, 0, 0}, {-60, 0, 0}, {-75, 0, 0}};
    const auto wake = SampleTrail(Kind::Ice, frame, history, 6, 2);
    bool oldIce = false;
    for (size_t i = 0; i < wake.count; ++i) {
      const auto &v = wake.vertices[i];
      oldIce |= v.p.x < -50 && v.alpha > 90 &&
                (std::abs(v.p.y) > 3 || std::abs(v.p.z) > 3);
    }
    assert(oldIce); // Substantial visible frost behind the body, not an
                    // occluded smooth line.
    assert(SampleProjectile(Kind::Fire, frame, 2, {1, 0, 0}).count > 0);
    auto release = SampleSpin(Kind::Fire, frame, 150, true);
    assert(release.count > 0 && release.count < release.vertices.size());
    assert(SampleSpinSurface(Kind::Light, frame, 150, true).count == 0);
    assert(SampleSpinFlow(Kind::Light, frame, 150, true, 1).count == 0);
    for (auto kind : {Kind::Fire, Kind::Ice}) {
      auto spin = SampleSpinSurface(kind, frame, 150, true);
      float height = 0;
      for (size_t i = 0; i < spin.count; ++i)
        height = std::max(height, spin.vertices[i].p.y);
      assert(spin.count < spin.vertices.size());
      assert(height == 192); // Native full-height wall; texture alpha defines
                             // the elemental crest.
      // The tall envelope must not become a uniformly opaque white curtain.
      bool tinted = false, transparentTop = false;
      for (size_t i = 0; i < spin.count; ++i) {
        const auto& v = spin.vertices[i];
        assert(v.alpha < 190);
        tinted |= v.alpha > 60 && v.rgb != 0xFFFFFF;
        transparentTop |= v.p.y >= 191.9f && v.alpha < 20;
      }
      assert(tinted && transparentTop);
      if (kind != Kind::Light) {
        const auto flow = SampleSpinFlow(kind, frame, 150, true, 1);
        const auto next = SampleSpinFlow(kind, frame + 1, 150, true, 1);
        assert(flow.count > 0 && flow.count + spin.count <= 864);
        assert(flow.vertices[0].u != next.vertices[0].u);
        for (size_t i = 0; i < flow.count; ++i) {
          assert(flow.vertices[i].v >= 0 && flow.vertices[i].v <= 1);
          assert(flow.vertices[i].p.y <= 36.001f); // Low outward sweep, separate from tall crest.
        }
        assert(flow.vertices[0].u !=
               spin.vertices[0].u); // Independent material motion.
      }
    }
    // Fire has its own warm, rising ember buildup, independent of Light's rays.
    for (const auto sampler : {SampleCharge, SampleChargeSparks, SampleChargeSurface}) {
      const auto fire = sampler(Kind::Fire, frame, 1, {});
      assert(fire.count > 0 && fire.count < fire.vertices.size());
      size_t warm = 0, visible = 0;
      for (size_t i = 0; i < fire.count; ++i) {
        const auto& v = fire.vertices[i];
        if (v.alpha > 30) {
          ++visible;
          const auto red = (v.rgb >> 16) & 255, green = (v.rgb >> 8) & 255;
          warm += red > green * 1.25f && (v.rgb & 255) < 120;
        }
      }
      assert(visible > 5 && warm > visible / 2);
    }
    const auto iceRelease = SampleSpin(Kind::Ice, frame, 150, true);
    size_t elevatedFaces = 0;
    for (size_t i = 0; i < iceRelease.count; i += 3)
      elevatedFaces += iceRelease.vertices[i].p.y > 12 &&
                       iceRelease.vertices[i + 1].p.y > 12 &&
                       iceRelease.vertices[i + 2].p.y > 12;
    assert(elevatedFaces > 12); // Actual ice facets above the floor rim.
  }
  std::cout << "PASS accepted bolts, warm Fire charge, translucent native-height "
               "release fronts and elevated Ice facets\n";
}
