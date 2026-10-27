#include "combo/rando/CrossDungeonEntrances.h"
#include <cassert>
#include <iostream>
#include <map>

using namespace ComboRando::CrossDungeon;

int main() {
  const std::map<int32_t, int32_t> blueWarps = {{0x457, 0x209}};
  assert(CanonicalNativeReturn(0x457, blueWarps) == 0x209);
  assert(CanonicalNativeReturn(0x701, blueWarps) == 0x701);
  const std::map<int32_t, int32_t> recorded = {{0x209, 0x701}};
  assert(ResolveNativeIndex(0x209, 0x209, recorded) == 0x701);
  assert(ResolveNativeIndex(0x209, 0x123, recorded) == 0x701);
  assert(ResolveNativeIndex(0x222, 0x333, recorded) == 0x333);
  // OoT's native mixed shuffle has already moved this dungeon's exit to a
  // grotto.
  const Dungeon oot{"oot.deku", {0, 0x0000}, {0, 0x0209}, {0, 0x0701}};
  const Dungeon mm{"mm.woodfall", {1, 0x3000}, {1, 0x8610}, {1, 0x8610}};
  auto disabled = Generate({oot, mm}, 42, false);
  assert(disabled.empty());

  auto routes = FromPermutation({oot, mm}, {1, 0});
  assert(routes.size() == 4);
  assert(Resolve(routes, Phase::AfterNative, oot.entrance) == mm.entrance);
  assert(Resolve(routes, Phase::BeforeNative, mm.exit) == oot.returnTo);
  assert(Resolve(routes, Phase::AfterNative, mm.entrance) == oot.entrance);
  assert(Resolve(routes, Phase::BeforeNative, oot.exit) == mm.returnTo);
  assert(Resolve(routes, Phase::AfterNative, {0, 0x1234}) ==
         (Endpoint{0, 0x1234}));
  assert(Resolve(routes, Phase::BeforeNative, oot.entrance) == oot.entrance);
  auto withEscape = mm;
  withEscape.escapeExits = {{1, 0x8620}};
  auto bossRoutes = FromPermutation({oot, withEscape}, {1, 0});
  assert(Resolve(bossRoutes, Phase::BeforeNative, {1, 0x8620}) == oot.returnTo);

  for (unsigned seed = 0; seed < 100; ++seed) {
    assert(Generate({oot, mm}, seed, true) == Generate({oot, mm}, seed, true));
  }
  bool rejected = false;
  try {
    FromPermutation({oot, mm}, {0, 0});
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  assert(rejected);
  rejected = false;
  try {
    Generate({oot, oot}, 1, true);
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  assert(rejected);
  auto bad = mm;
  bad.entrance.game = 2;
  rejected = false;
  try {
    Generate({oot, bad}, 1, true);
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  assert(rejected);
  std::cout << "cross-dungeon routing: disabled, composition, reverse, phase, "
               "replay and rejection pass\n";
}
