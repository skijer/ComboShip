#include "soh/Enhancements/randomizer/NeiAirMagicPolicy.h"
#include <fstream>
#include <iostream>
using namespace NeiAirMagic;
int main(int argc, char **argv) {
  if (argc != 2)
    return 2;
  std::ofstream out(argv[1], std::ios::binary);
  const float x = 24 * Tau / 360, y = -18 * Tau / 360;
  const Basis camera{
      {std::cos(y), 0, std::sin(y)},
      {std::sin(x) * std::sin(y), std::cos(x), -std::sin(x) * std::cos(y)},
      {-std::cos(x) * std::sin(y), std::sin(x), std::cos(x) * std::cos(y)}};
  size_t maxVertices = 0;
  for (unsigned frame = 0; frame < 180; ++frame)
    for (unsigned panel = 0; panel < 4; ++panel) {
      Layers layers;
      if (panel == 0)
        layers = SampleLightning(frame, {1, 0, 0}, camera);
      else if (panel == 1)
        layers = SampleEnvelope(frame, 44, {}, camera);
      else
        layers = SampleGust(frame, {1, 0, 0}, 200, 100, panel == 3,
                            panel == 3 ? 0xDDE5EF : 0xFFFFFF, camera);
      const Point offset = panel >= 2 ? Point{-100, 0, 0} : Point{};
      maxVertices =
          std::max(maxVertices, layers.surface.count + layers.detail.count);
      const uint32_t layerCount = 2;
      out.write(reinterpret_cast<const char *>(&layerCount), 4);
      for (int layer = 0; layer < 2; ++layer) {
        const auto &m = layer ? layers.detail : layers.surface;
        const uint32_t material = layer        ? 0
                                  : panel == 0 ? 2
                                               : 1,
                       count = m.count;
        out.write(reinterpret_cast<const char *>(&material), 4);
        out.write(reinterpret_cast<const char *>(&count), 4);
        for (size_t i = 0; i < m.count; ++i) {
          auto v = m.vertices[i];
          v.p = v.p + offset;
          const float p[] = {std::round(v.p.x * 16) / 16,
                             std::round(v.p.y * 16) / 16,
                             std::round(v.p.z * 16) / 16};
          const uint8_t rgba[] = {uint8_t(v.rgb >> 16), uint8_t(v.rgb >> 8),
                                  uint8_t(v.rgb), v.alpha};
          const float uv[] = {v.u, v.v};
          out.write(reinterpret_cast<const char *>(p), 12);
          out.write(reinterpret_cast<const char *>(rgba), 4);
          out.write(reinterpret_cast<const char *>(uv), 8);
        }
      }
    }
  std::cout << "Exported 180 exact effect phases; maximum " << maxVertices
            << " vertices in one effect\n";
  return !out;
}
