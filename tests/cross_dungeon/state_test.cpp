#include "combo/rando/CrossDungeonState.h"
#include <cassert>
#include <iostream>
using namespace ComboRando::CrossDungeon;
int main() {
  HostState host(0);
  auto seed = Encode({{"oot", {0, 0}, {0, 0x209}, {0, 0x701}},
                      {"mm", {1, 0x5000}, {1, 0x8000}, {1, 0x8000}}},
                     {1, 0});
  host.Install(seed);
  assert(host.Lookup(0x111, 0)->to ==
         (Endpoint{1, 0x5000})); // native mixed ingress
  assert(host.Lookup(0x209, 0x222)->to ==
         (Endpoint{1, 0x8000})); // pre-native exit wins
  host.SetAccess({{"overworld", false}, {"arrivals", {0, 0x701}}});
  assert(!host.overworld && host.arrivals.size() == 2);
  auto old = host.routes;
  auto broken = seed;
  broken["version"] = 9;
  bool rejected = false;
  try {
    host.Install(broken);
  } catch (const std::exception &) {
    rejected = true;
  }
  assert(rejected && host.routes == old); // parse is transactional
  host.Install(nullptr);
  assert(host.routes.empty() && host.arrivals.empty() && host.overworld);
  assert(host.Lookup(0x111, 0) == nullptr);
  std::cout << "cross-dungeon host state: native composition, restricted "
               "roots, transaction and clear pass\n";
}
