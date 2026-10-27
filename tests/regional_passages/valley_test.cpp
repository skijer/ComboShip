#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
#if __has_include("entrances/FountainValley.h")
#include "entrances/FountainValley.h"
#else
namespace ComboFountainValley {
inline bool FloorAt(float, float, float *) { return false; }
} // namespace ComboFountainValley
#endif
int main(int argc, char **argv) {
  if (argc > 1 && std::string(argv[1]) == "--bands") {
    for (const auto &band : ComboFountainValley::kBodyBands)
      std::cout << band.minX << " " << band.maxX << " " << band.halfWidth
                << "\n";
    return 0;
  }
  if (argc > 1) {
    for (const auto &t : ComboFountainValley::kTriangles) {
      for (const auto &p : {t.a, t.b, t.c})
        std::cout << p.x << " " << p.y << " " << p.z << " ";
      std::cout << t.floor << "\n";
    }
    return 0;
  }
  const float samples[][2] = {{1700, 256.72f}, {1840, 280.89f}, {1940, 286.53f},
                              {2400, 312.83f}, {3200, 260.26f}, {3600, 225.98f},
                              {4000, 188.02f}};
  for (const auto &p : samples) {
    float y = -9999;
    assert(ComboFountainValley::FloorAt(p[0], 0, &y) &&
           "missing shared valley floor");
    assert(std::abs(y - p[1]) < .02f &&
           "floor must fit existing valley triangles");
  }
  float y = 123;
#ifdef COMBO_VALLEY_BODY_CLEARANCE
  assert(ComboFountainValley::BodyClearAt(1840, 100));
  assert(ComboFountainValley::BodyClearAt(1740, -100));
  assert(ComboFountainValley::BodyClearAt(1600, 24));
  float terminalY;
  assert(ComboFountainValley::BodyClearAt(4062, 22, 18));
  assert(ComboFountainValley::FloorAt(4080, 40, &terminalY));
  assert(ComboFountainValley::FloorAt(4082, 0, &terminalY));
  assert(!ComboFountainValley::FloorAt(4099, 0, &terminalY));
  assert(!ComboFountainValley::BodyClearAt(4082, 0, 18) &&
         "a supported center cannot certify an unsupported adult footprint");
  assert(!ComboFountainValley::BodyClearAt(1579, 157));
  assert(!ComboFountainValley::BodyClearAt(1840, 110));
  assert(!ComboFountainValley::BodyClearAt(1580, 0));
#else
  assert(false &&
         "placement must reserve adult radius inside verified body envelope");
#endif
  assert(!ComboFountainValley::FloorAt(1200, 0, &y) && y == 123);
  assert(!ComboFountainValley::FloorAt(2000, 1000, &y));
  assert(!ComboFountainValley::FloorAt(2000, 0, nullptr));
  std::cout << "PASS shared valley grounded samples and outside rejection\n";
}
