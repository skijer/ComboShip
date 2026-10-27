#include "combo/rando/CrossDungeonSeed.h"
#include <cassert>
#include <iostream>
using namespace ComboRando::CrossDungeon;
using nlohmann::json;
static bool Rejects(const json &value) {
  try {
    Decode(value);
  } catch (const std::exception &) {
    return true;
  }
  return false;
}
int main() {
  const auto options = ParseOptions(
      {{"dungeons", false}, {"shops", true}, {"doorInteriors", true}});
  assert(options.Any() &&
         "narrow choices alone must enable entrance generation");
  assert(OptionsJson(options).at("shops") == true);
  assert(OptionsJson(options).at("doorInteriors") == true);
  std::vector<Dungeon> entries;
  for (int i = 0; i < 4; ++i) {
    Dungeon d{"door:" + std::to_string(i),
              {i % 2, 100 + i * 2},
              {i % 2, 101 + i * 2},
              {i % 2, 101 + i * 2}};
    d.pool = static_cast<Pool>(i < 2 ? 4 : 5);
    assert(Selected(d, options));
    entries.push_back(d);
  }
  const auto seed = Encode(entries, {1, 0, 3, 2});
  assert(seed.at("version") == 4);
  assert(Decode(json::parse(seed.dump())) ==
         FromPermutation(entries, {1, 0, 3, 2}));
  for (uint64_t i = 0; i < 50; ++i) {
    const auto routes = Decode(GenerateSeed(entries, i));
    for (const auto &r : routes)
      if (r.phase == Phase::AfterNative)
        assert((r.from.index < 104) == (r.to.index < 104) &&
               "shops and other interiors must stay separate");
  }
  auto invalid = seed;
  invalid["permutation"] = {2, 0, 3, 1};
  assert(Rejects(invalid));
  invalid = seed;
  invalid["options"]["shops"] = false;
  assert(Rejects(invalid));
  invalid = seed;
  invalid["options"]["doorInteriors"] = false;
  assert(Rejects(invalid));
  for (int version : {1, 2}) {
    invalid = seed;
    invalid["version"] = version;
    assert(Rejects(invalid));
  }
  bool badFlag = false;
  try {
    ParseOptions({{"shops", 1}});
  } catch (const std::exception &) {
    badFlag = true;
  }
  assert(badFlag);
  std::cout << "narrow pools: options, isolated permutations, v4 replay and "
               "old-version rejection PASS\n";
}
