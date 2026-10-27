#include "combo/rando/CrossDungeonReachability.h"
#include "combo/rando/CrossDungeonState.h"
#include <cassert>
#include <functional>
#include <iostream>
using namespace ComboRando::CrossDungeon;
struct Oracle {
  std::function<void(const char *)> SetDungeonAccess;
  std::function<const char *()> GetDungeonExits;
  std::function<uint8_t()> GetPortalOpen;
};
int main() {
  HostState hosts[] = {HostState(0), HostState(1)};
  std::string exits[] = {"[]", "[]"};
  Oracle oracles[2];
  for (int game = 0; game < 2; ++game) {
    oracles[game].SetDungeonAccess = [&, game](const char *raw) {
      hosts[game].SetAccess(nlohmann::json::parse(raw));
    };
    oracles[game].GetDungeonExits = [&, game]() { return exits[game].c_str(); };
  }
  oracles[0].GetPortalOpen = []() { return uint8_t(0); };
  auto query = [&](const Oracle &oracle, const std::vector<std::string> &) {
    int game = &oracle == &oracles[0] ? 0 : 1;
    std::unordered_set<std::string> checks;
    if (hosts[game].overworld)
      checks.insert(game == 0 ? "OoT overworld" : "MM overworld");
    if (game == 0 && hosts[0].overworld)
      exits[0] = "[{\"game\":1,\"index\":20480}]";
    if (game == 1 && hosts[1].arrivals.contains(20480)) {
      checks.insert("Woodfall chest");
      exits[1] = "[{\"game\":0,\"index\":1793}]";
    }
    if (game == 0 && hosts[0].arrivals.contains(1793))
      checks.insert("Shuffled return grotto");
    return checks;
  };
  auto reach = QueryJoint(oracles[0], oracles[1], {}, {}, false, query);
  assert(reach.oot.contains("OoT overworld"));
  assert(reach.oot.contains("Shuffled return grotto"));
  assert(reach.mm.contains("Woodfall chest"));
  assert(!reach.mm.contains("MM overworld"));
  auto startedMM = QueryJoint(oracles[0], oracles[1], {}, {}, true, query);
  assert(startedMM.mm.contains("MM overworld"));
  std::cout << "cross-dungeon joint oracle: foreign dungeon, chained return, "
               "overworld gate and MM start pass\n";
}
