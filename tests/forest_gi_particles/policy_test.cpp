#include "../../soh/soh/Enhancements/randomizer/NeiGiSongEffectPolicy.h"
#include "actors/ForestGiParticles.h"
#include <cassert>
#include <cstdio>

float Distance(NeiGi::Point a, NeiGi::Point b) {
  const auto d = a - b;
  return std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
}

// Only center and uniform size may differ from the actual NEI item twinkle.
void CheckCanonical(const NeiGi::Mesh &mesh, uint32_t frame, int motes,
                    float scale, const NeiGi::Basis &camera,
                    NeiGi::Kind kind = NeiGi::Kind::Leaf, uint32_t tint = 0) {
  constexpr size_t vertices = 72;
  assert(mesh.count == size_t(motes) * vertices &&
         "must use complete NEI twinkles, without added hexes");
  assert(mesh.count <= mesh.vertices.size());
  for (int i = 0; i < motes; ++i) {
    const auto reference =
        NeiGi::SampleShimmer(frame + uint32_t(i / 5) * 29u, true, camera, kind);
    assert(reference.count == 5 * vertices);
    const auto origin = mesh.vertices[i * vertices].p;
    const auto oldOrigin = reference.vertices[(i % 5) * vertices].p;
    for (size_t j = 0; j < vertices; ++j) {
      const auto &actual = mesh.vertices[i * vertices + j];
      const auto &expected = reference.vertices[(i % 5) * vertices + j];
      assert(Distance((actual.p - origin) * (1.f / scale),
                      expected.p - oldOrigin) < .0001f);
      const auto rgb =
          tint && expected.rgb == NeiGi::ColorHex(kind) ? tint : expected.rgb;
      assert(actual.rgb == rgb && actual.alpha == expected.alpha);
      assert(std::isfinite(actual.p.x) && std::isfinite(actual.p.y) &&
             std::isfinite(actual.p.z));
    }
  }
}

void CheckFaceCoverage(const NeiGi::Mesh &mesh, float radius, float front) {
  int central = 0, lower = 0, upper = 0, left = 0, right = 0;
  for (size_t i = 0; i < mesh.count; i += 72) {
    const auto p = mesh.vertices[i].p;
    central += std::fabs(p.x) < radius * .4f && p.y > 20 && p.y < 90;
    lower += p.y < 35;
    upper += p.y > 75;
    left += p.x < -radius * .3f;
    right += p.x > radius * .3f;
    assert(p.z >= front);
  }
  assert(central >= 4 &&
         "twinkles must cover the black entrance face, not only its rim");
  assert(lower && upper && left && right);
}

int main() {
  const NeiGi::Basis cameras[] = {
      {},
      {{0, 0, -1}, {0, 1, 0}, {1, 0, 0}},
      {{1, 0, 0}, {0, .70710678f, .70710678f}, {0, -.70710678f, .70710678f}}};
  for (const auto &camera : cameras) {
    for (uint32_t frame : {0u, 1u, 30u, 160u, 192u, 100000u, 0xffffffffu}) {
      for (bool seated : {false, true}) {
        const auto mesh = ForestGiParticles::Saria(frame, seated, camera);
        CheckCanonical(mesh, frame, ForestGiParticles::kSariaMotes, .4f,
                       camera);
        for (size_t i = 0; i < mesh.count; ++i) {
          const auto p = mesh.vertices[i].p;
          assert(std::fabs(p.x) < 31 && std::fabs(p.z) < 31);
          assert(p.y > 1 && p.y < (seated ? 35 : 56));
        }
      }
      for (bool packed : {false, true}) {
        const auto mesh = ForestGiParticles::Portal(frame, packed, camera);
        CheckCanonical(mesh, frame, ForestGiParticles::kPortalMotes, .8f,
                       camera, NeiGi::Kind::Shadow, 0x581880);
        CheckFaceCoverage(mesh, packed ? 68.f : 44.f, packed ? 14.f : 8.f);
      }
      const auto mystery = ForestGiParticles::Mystery(frame, camera);
      CheckCanonical(mystery, frame, ForestGiParticles::kMysteryMotes, .8f,
                     camera);
      CheckFaceCoverage(mystery, 60.f, 10.f);
      const auto mirror = ForestGiParticles::Mirror(frame, camera);
      const auto song = NeiGi::SampleSong(CW_SONG_OOT_SARIA, frame, camera);
      assert(mirror.count == song.count && mirror.count > 0);
      for (size_t i = 0; i < mirror.count; ++i) {
        const auto p = mirror.vertices[i].p;
        const auto &leaf = song.vertices[i];
        assert(std::fabs(p.x - leaf.p.x * .55f) < .00001f);
        assert(std::fabs(p.y - (6.f + leaf.p.y * .55f)) < .00001f);
        assert(std::fabs(p.z - leaf.p.z * .55f) < .00001f);
        assert(mirror.vertices[i].rgb == leaf.rgb &&
               mirror.vertices[i].alpha == leaf.alpha);
      }
    }
  }
  puts("PASS exact NEI twinkle geometry/pulse/palette, bounded camera-facing "
       "meshes and full entrance-face coverage");
}
