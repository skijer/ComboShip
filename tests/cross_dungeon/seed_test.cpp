#include "combo/rando/CrossDungeonSeed.h"
#include <cassert>
#include <iostream>
using namespace ComboRando::CrossDungeon;
int main() {
  const std::vector<Dungeon> dungeons = {
      {"oot.deku", {0, 0}, {0, 0x209}, {0, 0x701}},
      {"mm.woodfall", {1, 0x3000}, {1, 0x8610}, {1, 0x8610}}};
  auto seed = Encode(dungeons, {1, 0});
  auto restored = Decode(nlohmann::json::parse(seed.dump()));
  assert(restored == FromPermutation(dungeons, {1, 0}));
  assert(Decode(nlohmann::json())
             .empty()); // older seeds do not enable the feature
  for (int mutation = 0; mutation < 6; ++mutation) {
    auto bad = seed;
    if (mutation == 0)
      bad["version"] = 2;
    if (mutation == 1)
      bad["permutation"] = {0, 0};
    if (mutation == 2)
      bad["dungeons"][0]["entrance"]["game"] = 2;
    if (mutation == 3)
      bad["dungeons"][0]["entrance"]["index"] = 0x100000000ULL;
    if (mutation == 4)
      bad["enabled"] = "true";
    if (mutation == 5)
      bad["dungeons"][1]["id"] = "oot.deku";
    bool rejected = false;
    try {
      Decode(bad);
    } catch (const std::exception &) {
      rejected = true;
    }
    assert(rejected);
  }
  std::cout << "cross-dungeon seed codec: replay, legacy off and malformed "
               "rejection pass\n";
}
