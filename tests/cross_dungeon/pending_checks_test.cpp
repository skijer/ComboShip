#include "combo/rando/CrossDungeonPendingChecks.h"
#include <cassert>
#include <iostream>
using ComboRando::CrossDungeon::PendingChecks;
int main() {
  PendingChecks pending;
  pending.Capture({"Queen Gohma", "King Dodongo", "Queen Gohma"});
  auto saved = pending.Save();
  assert(saved == nlohmann::json({"Queen Gohma", "King Dodongo"}));
  assert(pending.RestoreOnce() ==
         (std::vector<std::string>{"Queen Gohma", "King Dodongo"}));
  assert(pending.RestoreOnce().empty());
  // Restoring is not delivery: a second save/restart must still contain the
  // pending reward.
  pending.Acknowledge("Queen Gohma");
  assert(pending.Save() == nlohmann::json({"King Dodongo"}));
  PendingChecks restart;
  restart.Load(pending.Save());
  assert(restart.RestoreOnce() == (std::vector<std::string>{"King Dodongo"}));
  assert(restart.RestoreOnce().empty());
  restart.Acknowledge("King Dodongo");
  assert(restart.Save().empty());
  pending.Load(nullptr); // Legacy saves/default-off cannot inherit a previous
                         // slot queue.
  assert(pending.RestoreOnce().empty() && pending.Save().empty());
  pending.Load(nlohmann::json::array({"Queen Gohma", 100}));
  assert(pending.RestoreOnce().empty());
  std::cout << "cross-dungeon pending checks: ok\n";
}
