#include "combo/rando/ComboPlaythrough.h"
#include <cassert>
#include <iostream>

using namespace ComboRando;
using Json = nlohmann::json;
static std::vector<std::string> owned[2];
static std::set<int> arrivals[2];
static bool overworld[2] = {true, true}, blockReturn = false;
static int queries = 0, portalReads = 0, restores = 0;
static void Reset() {}
static void Owned(int game, const char *raw) {
  owned[game] = Json::parse(raw).get<std::vector<std::string>>();
}
static void Access(int game, const char *raw) {
  const auto access = Json::parse(raw);
  overworld[game] = access.at("overworld").get<bool>();
  arrivals[game] = access.at("arrivals").get<std::set<int>>();
}
static bool MmAccessible() { return overworld[1] || arrivals[1].count(10); }
static int Hookshots() {
  return std::count(owned[1].begin(), owned[1].end(), "Progressive Hookshot");
}
static const char *Checks(int game) {
  static std::string raw[2];
  ++queries;
  auto checks = Json::array();
  if (game == 0) {
    checks = {"Entry chest", "Unused OOT chest"};
    if (arrivals[0].count(20))
      checks.push_back("Ganon");
  } else if (MmAccessible()) {
    checks = {"Hookshot chest 01", "Hookshot chest 02", "Unused MM chest"};
    if (Hookshots() >= 2)
      checks.push_back("Moon Majora Pot 01");
  }
  raw[game] = checks.dump();
  return raw[game].c_str();
}
static const char *OotExits() {
  return std::count(owned[0].begin(), owned[0].end(), "Entry Key")
             ? "[{\"game\":1,\"index\":10}]"
             : "[]";
}
static const char *MmExits() {
  return !blockReturn && MmAccessible() && Hookshots() >= 2
             ? "[{\"game\":0,\"index\":20}]"
             : "[]";
}
static uint8_t Portal() {
  ++portalReads;
  return 0;
}
static void Restore() { ++restores; }
static OracleFns oot{Reset,
                     [](const char *raw) { Owned(0, raw); },
                     [] { return Checks(0); },
                     nullptr,
                     Portal,
                     [](const char *raw) { Access(0, raw); },
                     OotExits};
static OracleFns mm{Reset,
                    [](const char *raw) { Owned(1, raw); },
                    [] { return Checks(1); },
                    nullptr,
                    nullptr,
                    [](const char *raw) { Access(1, raw); },
                    MmExits};

struct Placement {
  int game;
  const char *check;
  const char *item;
};
static const Placement placed[] = {
    {0, "Entry chest", "Entry Key"},
    {0, "Unused OOT chest", "Unused OOT"},
    {1, "Hookshot chest 01", "Progressive Hookshot"},
    {1, "Hookshot chest 02", "Progressive Hookshot"},
    {1, "Unused MM chest", "Unused MM"}};
static bool UncachedWin(int excluded) {
  std::vector<std::string> items[2];
  std::set<int> collected;
  for (;;) {
    const auto reach = CrossDungeon::QueryJoint(oot, mm, items[0], items[1],
                                                false, QueryReachable);
    if (DefaultGanonMajoraGoal(reach.oot, reach.mm, items[0], items[1]))
      return true;
    bool changed = false;
    for (int i = 0; i < 5; ++i)
      if (i != excluded && !collected.count(i) &&
          (placed[i].game ? reach.mm : reach.oot).count(placed[i].check)) {
        items[placed[i].game].push_back(placed[i].item);
        collected.insert(i);
        changed = true;
      }
    if (!changed)
      return false;
  }
}

int main() {
  assert(CanonicalJointOwnedKey({"A", "B"}, {}) !=
         CanonicalJointOwnedKey({"A\x1f"
                                 "B"},
                                {}));
  assert(CanonicalJointOwnedKey({"A"}, {"B"}) !=
         CanonicalJointOwnedKey({"A", "B"}, {}));
  assert(CanonicalJointOwnedKey({"1:A", "B"}, {}) !=
         CanonicalJointOwnedKey({"A", "1:B"}, {}));
  JointReachMemo memo(oot, mm, false);
  const auto first = memo.Query({"Entry Key", "Unused OOT"}, {});
  const int afterFirst = queries, portalAfterFirst = portalReads;
  const auto same = memo.Query({"Unused OOT", "Entry Key"}, {});
  assert(same.oot == first.oot && same.mm == first.mm &&
         queries == afterFirst && portalReads == portalAfterFirst);
  const auto one =
      memo.Query({"Entry Key", "Unused OOT"}, {"Progressive Hookshot"});
  assert(queries > afterFirst && !one.oot->count("Ganon"));
  const int afterOne = queries;
  const auto two = memo.Query({"Entry Key", "Unused OOT"},
                              {"Progressive Hookshot", "Progressive Hookshot"});
  assert(queries > afterOne && two.oot->count("Ganon") &&
         two.mm->count("Moon Majora Pot 01"));
  const int afterTwo = queries, portalAfterTwo = portalReads;
  memo.Query({"Entry Key", "Unused OOT"},
             {"Progressive Hookshot", "Progressive Hookshot"});
  assert(queries == afterTwo && portalReads == portalAfterTwo);

  Json spoiler = {{"oot", Json::object()}, {"mm", Json::object()}};
  for (const auto &p : placed)
    spoiler[p.game ? "mm" : "oot"][p.check] = p.item;
  queries = 0;
  const auto required = PareDownPlaythrough(spoiler.dump(), oot, mm, Restore);
  const int cachedQueries = queries;
  const std::unordered_map<std::string, bool> expected{
      {"oot:Entry chest", true},
      {"oot:Unused OOT chest", false},
      {"mm:Hookshot chest 01", true},
      {"mm:Hookshot chest 02", true},
      {"mm:Unused MM chest", false}};
  assert(required.requiredByCheck == expected && restores == 1);
  queries = 0;
  assert(UncachedWin(-1));
  for (int i = 0; i < 5; ++i) {
    const auto key =
        std::string(placed[i].game ? "mm:" : "oot:") + placed[i].check;
    assert(!UncachedWin(i) == expected.at(key));
  }
  assert(cachedQueries < queries); // The actual pare-down reuses joint sphere
                                   // prefixes across removals.
  blockReturn = true;
  queries = 0;
  const auto differentMap =
      PareDownPlaythrough(spoiler.dump(), oot, mm, Restore);
  assert(queries > 0 && restores == 2 &&
         differentMap.requiredByCheck.at("oot:Unused OOT chest"));
  std::cout << "cross-dungeon pare-down joint memo: PASS (" << cachedQueries
            << " cached native queries)\n";
}
