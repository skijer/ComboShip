// Offline preview of the exact production seasonal samplers and native sun
// tile.
#include "soh/Enhancements/randomizer/NeiGiEffectPolicy.h"
#include "soh/Enhancements/randomizer/NeiGiEnergyTexture.h"
#include <fstream>
#include <iostream>

int main(int argc, char **argv) {
  if (argc != 2)
    return 2;
  std::ofstream out(argv[1], std::ios::binary);
  const uint32_t frames = 240, tileSize = 32;
  out.write(reinterpret_cast<const char *>(&frames), 4);
  out.write(reinterpret_cast<const char *>(&tileSize), 4);
  const auto &texture = NeiGi::SeasonSunRayTexture();
  out.write(reinterpret_cast<const char *>(texture.data()), texture.size());
  const float x = 12 * NeiGi::Tau / 360, y = 25 * NeiGi::Tau / 360;
  const NeiGi::Basis camera{
      {std::cos(y), 0, std::sin(y)},
      {std::sin(x) * std::sin(y), std::cos(x), -std::sin(x) * std::cos(y)},
      {-std::cos(x) * std::sin(y), std::sin(x), std::cos(x) * std::cos(y)}};
  auto mesh = [&](const NeiGi::Mesh &m, bool textured) {
    const uint32_t count = m.count;
    out.write(reinterpret_cast<const char *>(&count), 4);
    for (size_t i = 0; i < m.count; ++i) {
      const auto &v = m.vertices[i];
      const float p[] = {std::round(v.p.x * 16) / 16,
                         std::round(v.p.y * 16) / 16,
                         std::round(v.p.z * 16) / 16};
      const uint8_t rgba[] = {uint8_t(v.rgb >> 16), uint8_t(v.rgb >> 8),
                              uint8_t(v.rgb), v.alpha};
      auto uv = [](float value, int span) {
        const int packed = std::lround(value * span * 32);
        return (((packed * 65535) >> 16) / 32.f + .5f) / 32;
      };
      const float coords[] = {textured ? uv(v.u, 32) : 0,
                              textured ? uv(v.v, 31) : 0};
      out.write(reinterpret_cast<const char *>(p), sizeof(p));
      out.write(reinterpret_cast<const char *>(rgba), sizeof(rgba));
      out.write(reinterpret_cast<const char *>(coords), sizeof(coords));
    }
  };
  for (uint32_t frame = 0; frame < frames; ++frame) {
    for (int profile = 1; profile <= 4; ++profile) {
      mesh(NeiGi::SampleSeason(frame, profile, camera), false);
      mesh(NeiGi::SampleSeasonSunRays(frame, profile, camera), true);
    }
  }
  if (!out)
    return 1;
  std::cout << "Exported 240 frames of weather-only production geometry and "
               "immutable sun I8 tile\n";
}
