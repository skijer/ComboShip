#include "render/RegionalBridgeMaterials.h"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <set>
#include <string>
int main() {
  using namespace RegionalBridgeMaterials;
  std::set<std::string> paths;
  for (auto m :
       {Material::LakeStone, Material::SnowheadStone, Material::SnowheadSnow,
        Material::SnowheadIce, Material::ValleySnow, Material::LakeWater, Material::LakeSand}) {
    assert((reinterpret_cast<uintptr_t>(OverridePath(m)) & 1) == 0);
    assert((reinterpret_cast<uintptr_t>(FallbackPath(m)) & 1) == 0);
    assert(paths.insert(OverridePath(m)).second);
    assert(
        std::strstr(OverridePath(m), "__OTR__textures/comboship/regional/") ==
        OverridePath(m));
    assert(std::strcmp(Resolve(m, [](const char *) { return false; }),
                       FallbackPath(m)) == 0);
    assert(std::strcmp(Resolve(m,
                               [&](const char *p) {
                                 return std::strcmp(p, OverridePath(m) + 7) ==
                                        0;
                               }),
                       OverridePath(m)) == 0);
  }
}
