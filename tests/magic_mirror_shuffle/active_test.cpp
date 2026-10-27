#include "combo/rando/ComboPlaythrough.h"
#include "soh/tests/test_require.h"
#include <cstring>
using namespace ComboRando;
using Json = nlohmann::json;
static std::vector<std::string> owned[2];
static bool forestFree = true, woodsFree = false, overworld[2] = {true, true};
static bool Has(int game, const char *name) {
  return std::find(owned[game].begin(), owned[game].end(), name) !=
         owned[game].end();
}
static void Reset() {}
static void Set(int game, const char *raw) {
  owned[game] = Json::parse(raw).get<std::vector<std::string>>();
}
static const char *Get(int game) {
  static std::string cache[2];
  Json checks = Json::array();
  if (game == 0) {
    checks = {"Start 1", "Start 2", "Start 3", "Start 4"};
    if (forestFree || Has(0, "Forest Key"))
      checks.push_back(MagicMirrorShuffle::kOotAccessProbe);
    if (Has(0, "Portal Key") && Has(0, "Forest Key"))
      checks.push_back("Ganon");
  } else if (overworld[1]) {
    checks = {"MM Chest"};
    if (woodsFree)
      checks.push_back(MagicMirrorShuffle::kMmStumpCheck);
    if (Has(1, "MM Goal Key"))
      checks.push_back("Moon Majora Pot 01");
  }
  cache[game] = checks.dump();
  return cache[game].c_str();
}
static uint8_t Portal() { return Has(0, "Portal Key"); }
static void Place(const char *, const char *) {}
static void Access(int game, const char *raw) {
  overworld[game] = Json::parse(raw).at("overworld").get<bool>();
}
static const char *Exits() { return "[]"; }
static void Restore() { overworld[0] = overworld[1] = true; }
static std::pair<OracleFns, OracleFns> Oracles(bool cross) {
  OracleFns oot{Reset, [](const char *s) { Set(0, s); }, [] { return Get(0); },
                Place, Portal};
  OracleFns mm{Reset, [](const char *s) { Set(1, s); }, [] { return Get(1); },
               Place};
  if (cross) {
    oot.SetDungeonAccess = [](const char *s) { Access(0, s); };
    oot.GetDungeonExits = Exits;
    mm.SetDungeonAccess = [](const char *s) { Access(1, s); };
    mm.GetDungeonExits = Exits;
  }
  return {oot, mm};
}
static Json Dump(bool mm) {
  Json dump = {{"checks", Json::array()},
               {"pool", Json::array()},
               {"fixed", Json::array()},
               {"items", Json::array()}};
  auto item = [&](const char *name, bool adv) {
    Json row = {{"name", name},
                {"advancement", adv},
                {"category", adv ? "major" : "junk"}};
    dump["pool"].push_back(row);
    dump["items"].push_back(row);
  };
  if (mm) {
    dump["checks"].push_back({{"name", MagicMirrorShuffle::kMmStumpCheck}});
    dump["checks"].push_back({{"name", "MM Chest"}});
    item("Magic Mirror", true);
    item("MM Goal Key", true);
  } else {
    for (const char *name : {"Start 1", "Start 2", "Start 3", "Start 4"})
      dump["checks"].push_back({{"name", name}});
    item("Forest Key", true);
    item("Portal Key", true);
    item("Blue Rupee", false);
    item("Blue Rupee", false);
  }
  return dump;
}
int main() {
  const auto ootDump = Dump(false).dump(), mmDump = Dump(true).dump();
  for (bool cross : {false, true}) {
    auto [oot, mm] = Oracles(cross);
    for (bool free : {false, true})
      for (bool woods : {false, true}) {
        forestFree = free;
        woodsFree = woods;
        for (uint32_t seed = 1; seed <= 4; ++seed) {
          const auto result =
              CrossWorldCombinedFill(ootDump, mmDump, seed, oot, mm);
          REQUIRE(result.success);
          const auto placements =
              ParseSpoilerPlacements(result.spoilerJson, ootDump, mmDump);
          REQUIRE(placements.size() == 6);
          int mirrors = 0, stumps = 0;
          for (const auto &p : placements) {
            mirrors += p.item == "Magic Mirror";
            if (p.checkGame == GAME_MM &&
                p.check == MagicMirrorShuffle::kMmStumpCheck) {
              ++stumps;
              if (!free && !woods)
                REQUIRE(p.item != "Forest Key");
            }
            REQUIRE(p.check != MagicMirrorShuffle::kOotAccessProbe);
          }
          REQUIRE(mirrors == 1 && stumps == 1);
          const auto replay =
              RunPlaythrough(result.spoilerJson, oot, mm, "Mirror active path",
                             Restore, nullptr, ootDump, mmDump);
          REQUIRE(replay.beatable && replay.unreachableOot == 0 &&
                  replay.unreachableMm == 0);
        }
      }
    // Direct/plando proof: stump can hold the key to the MAIN portal and be
    // collected through Lost Woods first, without making all Termina reachable.
    forestFree = true;
    woodsFree = false;
    Json spoiler = {
        {"oot",
         {{"Start 1", "Magic Mirror"},
          {"Start 2", "Forest Key"},
          {"Start 3", "Blue Rupee"},
          {"Start 4", "Blue Rupee"}}},
        {"mm",
         {{MagicMirrorShuffle::kMmStumpCheck, "Portal Key"},
          {"MM Chest", "MM Goal Key"}}},
        {"foreign",
         Json::array({{{"checkGame", "oot"},
                       {"checkName", "Start 1"},
                       {"itemGame", "mm"},
                       {"itemName", "Magic Mirror"}},
                      {{"checkGame", "mm"},
                       {"checkName", MagicMirrorShuffle::kMmStumpCheck},
                       {"itemGame", "oot"},
                       {"itemName", "Portal Key"}}})}};
    auto replay = RunPlaythrough(spoiler.dump(), oot, mm,
                                 "stump before main portal", Restore);
    REQUIRE(replay.beatable && replay.unreachableMm == 0);
    const auto required = PareDownPlaythrough(spoiler.dump(), oot, mm, Restore);
    REQUIRE(required.requiredByCheck.at(
        "mm:" + std::string(MagicMirrorShuffle::kMmStumpCheck)));
    if (cross) {
      const auto joint =
          CrossDungeon::QueryJoint(oot, mm, {}, {}, false, QueryReachable);
      REQUIRE(joint.mm.contains(MagicMirrorShuffle::kMmStumpCheck));
      REQUIRE(!joint.mm.contains("MM Chest"));
    }
    // Negative: the missing entrance requirement cannot justify itself on the
    // stump.
    forestFree = false;
    spoiler["oot"]["Start 2"] = "Portal Key";
    spoiler["mm"][MagicMirrorShuffle::kMmStumpCheck] = "Forest Key";
    spoiler["foreign"][1]["itemName"] = "Forest Key";
    replay =
        RunPlaythrough(spoiler.dump(), oot, mm, "self lock must fail", Restore);
    REQUIRE(!replay.beatable);
    // The genuine alternative MM Woods of Mystery route resolves that same
    // arrangement.
    woodsFree = true;
    replay = RunPlaythrough(spoiler.dump(), oot, mm,
                            "native Woods of Mystery route", Restore);
    REQUIRE(replay.beatable);
  }
  puts("PASS actual CrossWorldCombinedFill/RunPlaythrough/PareDown/QueryJoint: "
       "32 seeds, one Mirror/check, both forest routes, closed portal, "
       "candidate self-lock rejected");
}
