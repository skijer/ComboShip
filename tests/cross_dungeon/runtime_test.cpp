#include "combo/rando/CrossDungeonRuntime.h"
#include <cassert>
#include <iostream>
using namespace ComboRando::CrossDungeon;
int main() {
  const std::vector<Dungeon> dungeons = {
      {"oot:a", {0, 100}, {0, 101}, {0, 501}},
      {"oot:b", {0, 200}, {0, 201}, {0, 601}},
      {"mm:c", {1, 300}, {1, 301}, {1, 302}, {{1, 303}}}};
  auto seed = Encode(dungeons, {2, 0, 1});
  RuntimeState oot(0), mm(1);
  oot.Install(seed);
  mm.Install(seed);
  // Native mixed pools resolve a physical entrance to this slot first. Runtime
  // must take only one leg.
  auto entering = oot.Lookup(Phase::AfterNative, 100);
  assert(entering && entering->to == (Endpoint{1, 300}) &&
         entering->checkpoint == (Endpoint{0, 501}));
  auto leaving = mm.Lookup(Phase::BeforeNative, 301);
  assert(leaving && leaving->to == (Endpoint{0, 501}) &&
         leaving->checkpoint == (Endpoint{1, 300}));
  auto escape = mm.Lookup(Phase::BeforeNative, 303);
  assert(escape && escape->to == (Endpoint{0, 501}) &&
         escape->checkpoint == (Endpoint{1, 300}));
  auto sameHost = oot.Lookup(Phase::AfterNative, 200);
  assert(sameHost && sameHost->to == (Endpoint{0, 100}));
  assert(!oot.Lookup(Phase::BeforeNative, 999));
  // An arrival is already composed: consume exactly once without recursively
  // mapping 100 again.
  assert(oot.QueueArrival(100));
  oot.Install(seed);
  assert(oot.ConsumeArrival() == 100);
  assert(oot.ConsumeArrival() == -1);
  assert(!oot.QueueArrival(
      300)); // Never reinterpret a foreign index as a host index.
  assert(!oot.QueueArrival(-1));
  assert(mm.QueueArrival(300));
  try {
    auto broken = seed;
    broken["permutation"] = {0, 0, 2};
    mm.Install(broken);
    assert(false);
  } catch (const std::exception &) {
  }
  assert(mm.ConsumeArrival() ==
         300); // Invalid installs leave the entire runtime state intact.
  assert(mm.QueueArrival(300));
  mm.Install(nullptr);
  assert(mm.ConsumeArrival() == -1 && !mm.Lookup(Phase::AfterNative, 300));
  assert(ReadCheckpoint(nlohmann::json::object()) == -1);
  assert(ReadCheckpoint({{"crossDungeonCheckpoint", 501}}) == 501);
  assert(ReadCheckpoint({{"crossDungeonCheckpoint", -1}}) == -1);
  assert(ReadCheckpoint({{"crossDungeonCheckpoint", 0x10000}}) == -1);
  assert(ReadCheckpoint({{"crossDungeonCheckpoint", "501"}}) == -1);
  CheckpointState checkpoint;
  checkpoint.Arrive(300, true);
  assert(checkpoint.Saved() == 300);
  CheckpointState restart;
  restart.Load(checkpoint.Saved());
  assert(restart.ConsumeRecovery([](int32_t index) { return index == 300; }) ==
         300);
  assert(restart.Saved() ==
         300); // Default native remember-location cannot release the dungeon.
  restart.ObserveScene(true);
  assert(restart.Saved() == 300);
  restart.ObserveScene(false);
  assert(restart.Saved() ==
         -1); // Actual native departure releases the active ingress checkpoint.
  checkpoint.Arrive(501, false);
  assert(checkpoint.Saved() == -1);
  checkpoint.Load(501);
  assert(checkpoint.ConsumeRecovery([](int32_t) { return false; }) == 501);
  assert(checkpoint.Saved() == -1);
  const uint8_t nativeDungeonOff = 0;
  assert(EffectiveEntranceSetting(nativeDungeonOff, true, true) == 1);
  assert(nativeDungeonOff ==
         0); // Projection cannot rewrite the native seed setting.
  assert(EffectiveEntranceSetting(0, true, false) ==
         0); // Other shuffle types remain native.
  assert(EffectiveEntranceSetting(0, false, true) ==
         0); // Default-off path is identical.
  assert(EffectiveEntranceSetting(1, true, true) == 1);
  assert(EffectiveEntranceSetting(2, true, true) ==
         2); // Keep native On + Ganon, never downgrade.
  std::cout << "cross-dungeon runtime: ok\n";
}
