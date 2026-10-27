#include "soh/Enhancements/randomizer/NeiGiEffectPolicy.h"
#include "soh/Enhancements/randomizer/NeiGiEnergyTexture.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <set>
#include <utility>

int main() {
  using namespace NeiGi;
  // Concrete cane hues and a two-tone Pokeball never inherit neutral blue.
  assert(ColorHex(static_cast<Kind>(33)) == 0xFF3C3C);
  assert(ColorHex(static_cast<Kind>(34)) == 0xFFD746);
  const auto pokeball = SampleShimmer(47, true, {}, static_cast<Kind>(35));
  for (int cluster = 0; cluster < 5; ++cluster)
    assert(pokeball.vertices[cluster * 72].rgb == (cluster % 2 ? 0xFFFFFFu : 0xE73842u));
  // Each blade has its own intrinsic effect even when the optional shimmer is
  // disabled. These numeric identities append to the existing native contract.
  const Kind swords[] = {Kind::KokiriSword, Kind::MmKokiriSword,
                         Kind::RazorSword, Kind::GildedSword,
                         Kind::MasterSword, Kind::BiggoronSword,
                         Kind::GreatFairySword, Kind::FourSword,
                         Kind::SwordAura, Kind::GiantsKnife};
  static_assert(int(Kind::SwordAura) == 17 && int(Kind::SlateSensor) == 24 &&
                int(Kind::KokiriSword) == 25 && int(Kind::FourSword) == 32 &&
                int(Kind::Gold) == 37 && int(Kind::GiantsKnife) == 38);
  static_assert(int(Kind::DivineShield)==39 && int(Kind::IronKnuckleAxe)==49);
  for(int profile=int(Kind::DivineShield);profile<=int(Kind::IronKnuckleAxe);++profile) {
    const auto kind=static_cast<Kind>(profile);
    assert(IsEquipment(kind)&&!IsSword(kind)&&!IsRod(kind)&&!IsSpell(kind)&&!IsSpecial(kind));
    for(uint32_t frame:{0u,42u,179u,360u,std::numeric_limits<uint32_t>::max()}) {
      assert(SampleOrb(kind).count==0&&SampleEnergy(kind,frame).count==0&&SampleSpecial(kind,frame).count==0);
      const auto stars=SampleShimmer(frame,true,{},kind);
      assert(stars.count==360&&SampleShimmer(frame,false,{},kind).count==0);
      for(size_t cluster=0;cluster<5;++cluster) {
        const auto hue=cluster%2?EquipmentShimmerSecondary(kind):ColorHex(kind);
        for(size_t vertex=0;vertex<72;++vertex) {
          const bool core=vertex>=48&&(vertex-48)%3==0;
          assert(stars.vertices[cluster*72+vertex].rgb==(core?0xFFFFFF:hue));
        }
      }
    }
  }
  const uint32_t swordHues[] = {0x78C850, 0xA46CFF, 0xBDD6EA, 0xFFD45A,
                                0x6F8FFF, 0xFF9A42, 0x79BE84, 0x315B2F,
                                0xFFF4D6, 0xCFD9E6};
  const Basis sideCamera{{0, 0, 1}, {0, 1, 0}, {-1, 0, 0}};
  auto near = [](Point a, Point b) {
    return std::abs(a.x - b.x) < .0001f &&
           std::abs(a.y - b.y) < .0001f &&
           std::abs(a.z - b.z) < .0001f;
  };
  // Restoring the black/orange mesh must also replace its old violet energy.
  // Removing either shadow wisps or orange embers breaks the two-tone effect.
  for (uint32_t frame : {0u, 19u, 47u, 179u, 180u, 65535u,
                        std::numeric_limits<uint32_t>::max()}) {
    for (const auto &mesh : {SampleSpecial(Kind::DarkCrystal, frame),
                            SampleShimmer(frame, true, {}, Kind::DarkCrystal)}) {
      bool dark = false, ember = false;
      for (size_t index = 0; index < mesh.count; ++index) {
        const auto &vertex = mesh.vertices[index];
        const unsigned red = vertex.rgb >> 16;
        const unsigned green = (vertex.rgb >> 8) & 255;
        const unsigned blue = vertex.rgb & 255;
        if (!vertex.alpha)
          continue;
        dark |= red <= 48 && green <= 48 && blue <= 48;
        ember |= red >= 150 && green >= 40 && green < red && blue < green / 2;
        assert(vertex.rgb != 0x9E38DA && vertex.rgb != 0xD5B6FF);
      }
      assert(dark && ember && "Shadow Crystal needs visible charcoal and burnt-orange particles");
    }
  }
  for (size_t sword = 0; sword < std::size(swords); ++sword) {
    const Kind kind = swords[sword];
    assert(IsSword(kind));
    assert(IsSpecial(kind));
    assert(ColorHex(kind) == swordHues[sword]);
    float highestBladeEffect = -1000.f;
    for (uint32_t frame : {0u, 19u, 47u, 179u, 180u, 65535u,
                           std::numeric_limits<uint32_t>::max()}) {
      const auto mesh = SampleSpecial(kind, frame);
      const auto again = SampleSpecial(kind, frame);
      const auto side = SampleSpecial(kind, frame, sideCamera);
      assert(mesh.count > 0 && mesh.count < mesh.vertices.size() &&
             mesh.count % 3 == 0 && mesh.count == side.count);
      size_t anchors = 0;
      for (size_t i = 0; i < mesh.count; ++i) {
        const auto &v = mesh.vertices[i];
        highestBladeEffect = std::max(highestBladeEffect, v.p.y);
        assert(std::isfinite(v.p.x) && std::isfinite(v.p.y) &&
               std::isfinite(v.p.z));
        assert(std::abs(v.p.x) < 25 && v.p.y > -23 &&
               v.p.y < (kind == Kind::BiggoronSword ? 112.f : 103.f) &&
               std::abs(v.p.z) < 15);
        assert(near(v.p, again.vertices[i].p) &&
               v.rgb == again.vertices[i].rgb &&
               v.alpha == again.vertices[i].alpha);
        // Transparent skirts may face the camera; lit blade centers and local
        // flecks must stay attached as the camera moves around the item.
        if (v.rgb == swordHues[sword] && v.alpha >= 80) {
          assert(near(v.p, side.vertices[i].p));
          if (kind == Kind::RazorSword || kind == Kind::BiggoronSword || kind == Kind::GiantsKnife) {
            // The corrected meshes stand upright. Their lit particle centers
            // must stay around the blade axis without the former baked lean.
            assert(std::abs(v.p.x) < 6.8f);
          }
          ++anchors;
        }
      }
      assert(anchors >= 6);
      assert(SampleShimmer(frame, false, {}, kind).count == 0);
    }
    // Energy must reach each blade's upper section. Gilded's shortened model
    // ends at native Y=61.78, so its motes must not trail far beyond that tip.
    if (kind == Kind::MasterSword) assert(highestBladeEffect > 69.f);
    if (kind == Kind::SwordAura) assert(highestBladeEffect > 65.f);
    // Independent native tip landmarks: Biggoron 109.55, Knife 100.31.
    if (kind == Kind::BiggoronSword) assert(highestBladeEffect > 108.f && highestBladeEffect < 112.f);
    if (kind == Kind::GiantsKnife) assert(highestBladeEffect > 99.f && highestBladeEffect < 103.f);
    if (kind == Kind::GildedSword) {
      assert(highestBladeEffect > 60.f);
      assert(highestBladeEffect < 63.f);
    }
    const auto a = SampleSpecial(kind, 19), b = SampleSpecial(kind, 47);
    bool moves = a.count != b.count;
    for (size_t i = 0; i < a.count && i < b.count; ++i)
      moves |= !near(a.vertices[i].p, b.vertices[i].p);
    assert(moves);
    // The native animation phase may wrap, but visible energy must not snap.
    const auto last = SampleSpecial(kind, 179), first = SampleSpecial(kind, 180);
    assert(last.count == first.count);
    for (size_t i = 0; i < last.count; ++i) {
      if (last.vertices[i].alpha < 40 || first.vertices[i].alpha < 40)
        continue;
      const auto delta = last.vertices[i].p - first.vertices[i].p;
      assert(std::hypot(delta.x, std::hypot(delta.y, delta.z)) < 2);
    }
  }
  // Four Sword always carries all four exact blade colors, and the five
  // optional shimmer clusters are green, red, blue, violet, green.
  const Kind fourSword = Kind::FourSword;
  const uint32_t fourHues[] = {0x315B2F, 0xD8232D, 0x2289CF, 0x6D3593};
  for (uint32_t frame : {0u, 47u, 179u, 180u}) {
    const auto trails = SampleSpecial(fourSword, frame);
    for (uint32_t hue : fourHues) {
      bool visible = false;
      for (size_t i = 0; i < trails.count; ++i)
        visible |= trails.vertices[i].rgb == hue &&
                   trails.vertices[i].alpha >= 80;
      assert(visible);
    }
    const auto shimmer = SampleShimmer(frame, true, {}, fourSword);
    // Each independent cluster has a soft 16-triangle halo and 8-triangle star.
    assert(shimmer.count == 5 * (16 + 8) * 3);
    const uint32_t clusterHues[] = {fourHues[0], fourHues[1], fourHues[2],
                                    fourHues[3], fourHues[0]};
    for (size_t cluster = 0; cluster < 5; ++cluster)
      assert(shimmer.vertices[cluster * 72].rgb == clusterHues[cluster]);
  }
  // Fairy petals progress through the two approved hues across the animation.
  const Kind fairySword = Kind::GreatFairySword;
  bool green = false, violet = false, hueChanges = false;
  const auto fairyStart = SampleSpecial(fairySword, 0);
  for (uint32_t frame : {0u, 47u, 90u, 137u}) {
    const auto petals = SampleSpecial(fairySword, frame);
    for (size_t i = 0; i < petals.count; ++i) {
      const auto &v = petals.vertices[i];
      assert(v.rgb == 0x79BE84 || v.rgb == 0x9382C4);
      green |= v.rgb == 0x79BE84 && v.alpha >= 80;
      violet |= v.rgb == 0x9382C4 && v.alpha >= 80;
      hueChanges |= i < fairyStart.count && v.rgb != fairyStart.vertices[i].rgb;
    }
  }
  assert(green && violet && hueChanges);
  // Fairy shimmer follows a smooth shared green/violet cycle while its bright
  // star centers stay white. A native 180-frame boundary cannot reset the hue.
  for (const auto &[frame, expected] :
       {std::pair{0u, 0x79BE84u}, std::pair{90u, 0x86A0A4u},
        std::pair{180u, 0x9382C4u}, std::pair{360u, 0x79BE84u}}) {
    const auto shimmer = SampleShimmer(frame, true, {}, fairySword);
    bool whiteCore = false;
    for (size_t i = 0; i < shimmer.count; ++i) {
      const auto &v = shimmer.vertices[i];
      assert(v.rgb == expected || v.rgb == 0xFFFFFF);
      whiteCore |= v.rgb == 0xFFFFFF && v.alpha == 255;
    }
    assert(shimmer.vertices[0].rgb == expected && whiteCore);
  }
  for (uint32_t frame = 0; frame < 720; ++frame) {
    const uint32_t a = SampleShimmer(frame, true, {}, fairySword).vertices[0].rgb;
    const uint32_t b = SampleShimmer(frame + 1, true, {}, fairySword).vertices[0].rgb;
    for (int shift : {0, 8, 16})
      assert(std::abs(int((a >> shift) & 255) - int((b >> shift) & 255)) <= 2);
  }
  assert(!IsSword(Kind::Neutral) && !IsSword(Kind::SlateSensor));
  // True Master is an ivory blessing with gold motes. Any blue energy would
  // obscure its identity and make it resemble the ordinary Master Sword.
  assert(ColorHex(Kind::SwordAura) == 0xFFF4D6);
  for (uint32_t frame : {0u, 19u, 47u, 179u, 180u}) {
    const auto blessing = SampleSpecial(Kind::SwordAura, frame);
    bool ivory = false, gold = false;
    for (size_t i = 0; i < blessing.count; ++i) {
      const auto &v = blessing.vertices[i];
      assert(v.rgb == 0xFFF4D6 || v.rgb == 0xF4C95D);
      ivory |= v.rgb == 0xFFF4D6 && v.alpha > 0;
      gold |= v.rgb == 0xF4C95D && v.alpha > 0;
    }
    assert(ivory && gold);
  }
  const Kind kinds[] = {Kind::Fire,  Kind::Ice,   Kind::Light,
                        Kind::Hylia, Kind::Zonai, Kind::Demise};
  for (Kind kind : kinds) {
    for (uint32_t tick : {0u, 1u, 45u, 89u, 179u, 180u, 65535u,
                          std::numeric_limits<uint32_t>::max()}) {
      const auto mesh = SampleEnergy(kind, tick);
      const auto again = SampleEnergy(kind, tick);
      assert(mesh.count > 30 && mesh.count <= mesh.vertices.size() &&
             mesh.count % 3 == 0);
      size_t bright = 0;
      for (size_t i = 0; i < mesh.count; ++i) {
        const auto &v = mesh.vertices[i];
        assert(std::isfinite(v.p.x) && std::isfinite(v.p.y) &&
               std::isfinite(v.p.z));
        assert(std::abs(v.p.x) < 45 && std::abs(v.p.y) < 45 &&
               std::abs(v.p.z) < 45);
        assert(v.p.x == again.vertices[i].p.x &&
               v.alpha == again.vertices[i].alpha);
        bright += v.alpha >= 200;
        if (kind == Kind::Hylia || kind == Kind::Zonai ||
            kind == Kind::Demise) {
          // Crystal's actual octahedral interior (30/50 author units * .74).
          assert((std::abs(v.p.x) + std::abs(v.p.z)) / 22.2f +
                     std::abs(v.p.y) / 37.f <=
                 1.001f);
        }
      }
      assert(bright >= 6); // Energy must never fade out entirely.
    }
    const auto a = SampleEnergy(kind, 0), b = SampleEnergy(kind, 19);
    bool moves = false;
    for (size_t i = 0; i < a.count && i < b.count; ++i) {
      moves |= std::abs(a.vertices[i].p.x - b.vertices[i].p.x) > .25f;
    }
    assert(moves);
  }
  for(Kind k : {Kind::Sand,Kind::Tornado,Kind::Water,Kind::Meteor,Kind::Storm,Kind::Shadow,
                 Kind::Slate,Kind::Hourglass,Kind::DarkCrystal,Kind::SwordAura,Kind::CaneBlue}) {
    assert(IsSpecial(k));
    for(uint32_t frame : {0u,19u,47u,179u,180u,65535u}) {
      auto a=SampleSpecial(k,frame), b=SampleSpecial(k,frame);
      assert(a.count>0 && a.count<=a.vertices.size() && a.count%3==0);
      for(size_t i=0;i<a.count;++i) {
        const auto& v=a.vertices[i];
        assert(std::isfinite(v.p.x) && std::isfinite(v.p.y) && std::isfinite(v.p.z));
        const float heightBound = k == Kind::SwordAura ? 74.f : 48.f;
        assert(std::abs(v.p.x)<48 && std::abs(v.p.y)<heightBound && std::abs(v.p.z)<48);
        assert(v.p.x==b.vertices[i].p.x && v.rgb==b.vertices[i].rgb && v.alpha==b.vertices[i].alpha);
      }
    }
  }
  assert(!IsSpecial(Kind::Neutral) && SampleSpecial(Kind::Neutral,0).count==0);
  assert(SampleEnergy(Kind::Neutral, 0).count == 0);
  assert(SampleShimmer(0, false).count == 0);
  const auto glints = SampleShimmer(45, true);
  assert(glints.count > 0 && glints.count <= glints.vertices.size());
  assert(ColorHex(Kind::Demise) == 0x000000);
  // Elemental shimmer must carry the item's hue, rather than a universal cyan.
  for (Kind kind : {Kind::Fire, Kind::Ice, Kind::Light, Kind::Hylia,
                    Kind::Zonai, Kind::Demise, Kind::Shadow, Kind::DarkCrystal}) {
    const auto shimmer = SampleShimmer(45, true, {}, kind);
    bool carriesHue = false;
    for (size_t i = 0; i < shimmer.count; ++i)
      carriesHue |= shimmer.vertices[i].rgb == ColorHex(kind);
    assert(carriesHue);
    assert(SampleShimmer(45, false, {}, kind).count == 0);
  }
  // Summer must remain a recognizable, steady sun rather than rising motes.
  // Removing the disk or reintroducing embers breaks this silhouette contract.
  for (uint32_t frame : {0u, 47u, 179u, 180u, 719u,
                         std::numeric_limits<uint32_t>::max()}) {
    const auto sun = SampleSeason(frame, 2);
    size_t diskVertices = 0;
    for (size_t i = 0; i < sun.count; ++i) {
      const auto &v = sun.vertices[i];
      if (v.alpha >= 230 && std::hypot(v.p.x, v.p.y) <= 11.6f)
        ++diskVertices;
    }
    assert(diskVertices >= 48);
  }
  // Spring's only particles are rain; petals can resemble a second item effect.
  const auto rain = SampleSeason(47, 1);
  for (size_t i = 0; i < rain.count; ++i)
    assert((rain.vertices[i].rgb & 255) >= (rain.vertices[i].rgb >> 16));
  const auto &sunRays = SeasonSunRayTexture();
  assert(sunRays.size() == 1024 && &sunRays == &SeasonSunRayTexture());
  assert(sunRays[0] == 255 && sunRays[16] == 0);
  for (size_t i = 992; i < 1024; ++i) assert(sunRays[i] == 0);
  for (int profile : {1, 3, 4}) {
    // A native frame boundary must not snap visible flakes/leaves around.
    const auto a = SampleSeason(179, profile), b = SampleSeason(180, profile);
    assert(a.count == b.count);
    for (size_t i = 0; i < a.count; ++i) {
      if (a.vertices[i].alpha < 40 || b.vertices[i].alpha < 40) continue;
      assert(std::abs(a.vertices[i].p.y - b.vertices[i].p.y) < 2);
      assert(std::abs(a.vertices[i].p.x - b.vertices[i].p.x) < 1);
    }
  }
  // Native orb texels must be stable for deferred GPU interpretation, animated,
  // bounded to one TMEM load, and empty at their border (no square billboard).
  for (Kind kind :
       {Kind::Fire, Kind::Ice, Kind::Hylia, Kind::Zonai, Kind::Demise}) {
    const auto &a = OrbTexture(kind, 0);
    assert(a.size() == 4096 && &a == &OrbTexture(kind, OrbFrameCount));
    assert(&a != &OrbTexture(kind, 1));
    size_t opaque = 0, moving = 0;
    for (size_t i = 0; i < a.size(); ++i) {
      opaque += a[i] > 170;
      moving += std::abs(int(a[i]) - int(OrbTexture(kind, 9)[i])) > 20;
      if (i < 64 || i >= 4032 || i % 64 == 0 || i % 64 == 63)
        assert(a[i] == 0);
    }
    assert(opaque > 40 && moving > 100);
    if (kind == Kind::Demise)
      assert(a[32 * 64 + 32] == 0); // Approved black center stays visible.
    for (const Basis camera : {Basis{}, Basis{{.7071f, 0, .7071f},
                                              {.4082f, .8165f, -.4082f},
                                              {-.5774f, .5774f, .5774f}}}) {
      auto orb = SampleOrb(kind, camera);
      assert(orb.count > 30 && orb.count <= orb.vertices.size());
      for (size_t i = 0; i < orb.count; ++i) {
        const auto &v = orb.vertices[i];
        assert(v.u >= 0 && v.u <= 1 && v.v >= 0 && v.v <= 1);
        if (IsSpell(kind)) {
          const float length =
              std::sqrt(v.p.x * v.p.x + v.p.y * v.p.y + v.p.z * v.p.z);
          assert(length > 14.06f); // Do not disappear behind the opaque core.
          assert((std::abs(v.p.x) + std::abs(v.p.z)) / 22.2f +
                     std::abs(v.p.y) / 37.f <
                 1.f);
        }
      }
    }
  }
  assert(SampleOrb(Kind::Light).count == 0);
  assert(SampleOrb(Kind::Leaf).count == 0);
  // Ice crystals have hard, differently colored planar faces, not soft star
  // fans. Cull their rear faces before submission because effects do not write
  // depth; rear faces must never paint over the visible crystal surface.
  for (const Basis camera :
       {Basis{}, Basis{{0, 0, 1}, {0, 1, 0}, {-1, 0, 0}}}) {
    Mesh crystal;
    IceCrystal(crystal, {}, {0.3f, 1, 0.2f}, 9, 2, .4f, 230, camera);
    assert(crystal.count > 12 && crystal.count <= 72 && crystal.count % 3 == 0);
    std::set<uint32_t> faceColors;
    for (size_t i = 0; i < crystal.count; i += 3) {
      const auto &a = crystal.vertices[i], &b = crystal.vertices[i + 1],
                 &c = crystal.vertices[i + 2];
      const auto n = Cross(b.p - a.p, c.p - a.p);
      assert(n.x * camera.forward.x + n.y * camera.forward.y +
                 n.z * camera.forward.z >
             0);
      assert(a.rgb == b.rgb && a.rgb == c.rgb);
      assert(a.alpha == 230 && b.alpha == 230 && c.alpha == 230);
      assert((a.rgb & 255) > (a.rgb >> 16));
      faceColors.insert(a.rgb);
    }
    assert(faceColors.size() >= 3);
  }
  std::cout << "NEI energy: continuous, bounded, animated, contained; "
               "independent optional shimmer passed\n";
}
