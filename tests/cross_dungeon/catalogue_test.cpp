#include "combo/rando/CrossDungeonCatalogue.h"
#include <cassert>
#include <iostream>
using namespace ComboRando::CrossDungeon;
int main() {
  const std::vector<Dungeon> base = {{"oot:0", {0, 0}, {0, 0x209}, {0, 0x209}},
                                     {"oot:4", {0, 4}, {0, 0x221}, {0, 0x221}}};
  const std::map<int32_t, int32_t> warps = {{0x457, 0x209}, {0x47a, 0x221}};
  const std::map<int32_t, int> types = {{0, 5},     {4, 5},     {0x209, 5},
                                        {0x221, 5}, {0x457, 4}, {0x47a, 4},
                                        {0x701, 20}};
  const auto before = DungeonJson(base[0]);
  auto vanilla = ComposeNativeCatalogue(base, warps, {});
  assert(vanilla[0].escapeExits == std::vector<Endpoint>({{0, 0x457}}));
  auto raw = nlohmann::json::array({{{"type", 5},
                                     {"index", 0x209},
                                     {"override", 0x701},
                                     {"destination", -1},
                                     {"overrideDestination", -1}},
                                    {{"type", 4},
                                     {"index", 0x47a},
                                     {"override", 0x701},
                                     {"destination", -1},
                                     {"overrideDestination", -1}}});
  auto overrides = ParseNativeOverrides(raw, types, 400);
  auto mixed = ComposeNativeCatalogue(base, warps, overrides);
  assert(mixed[0].returnTo.index == 0x701);
  assert(mixed[0].escapeExits == std::vector<Endpoint>({{0, 0x47a}}));
  assert(DungeonJson(base[0]) == before);
  for (int mutation = 0; mutation < 6; ++mutation) {
    auto bad = raw;
    if (mutation == 0)
      bad[0]["index"] = 0x7fff;
    if (mutation == 1)
      bad[0]["override"] = 0x7fff;
    if (mutation == 2)
      bad[0]["type"] = 99;
    if (mutation == 3)
      bad[0]["destination"] = 0x7fff;
    if (mutation == 4)
      bad.push_back(bad[0]);
    if (mutation == 5)
      bad[0]["destination"] = UINT64_MAX;
    bool rejected = false;
    try {
      ParseNativeOverrides(bad, types, 400);
    } catch (const std::exception &) {
      rejected = true;
    }
    assert(rejected);
  }
  std::cout << "pure catalogue: vanilla, mixed exit/warp composition, "
               "immutable input, invalid native override rejection pass\n";
}
