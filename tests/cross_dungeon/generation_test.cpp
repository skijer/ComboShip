#include "combo/rando/CrossDungeonGeneration.h"
#include <cassert>
#include <iostream>

using namespace ComboRando;
using namespace ComboRando::CrossDungeon;
using Json = nlohmann::json;
static std::vector<Dungeon> dungeons{{"oot:adult", {0, 1}, {0, 2}, {0, 3}},
                                     {"mm:temple", {1, 4}, {1, 5}, {1, 6}}};
static Json installed[2], accesses[2];
static std::vector<std::string> lastOwned[2];
static Json rejected;
static bool rejectEveryMap = false;
static int hostCalls = 0, queries = 0, runtimeCalls = 0;
static int lastQueryGame = 0;
static ComboGenProgress cancelledProgress;
static bool cancelOnQuery = false;
static const char *Catalogue(int game) {
  static std::string raw[2];
  ++hostCalls;
  raw[game] = Json::array({DungeonJson(dungeons[game])}).dump();
  return raw[game].c_str();
}
static int Install(int game, const char *raw) {
  ++hostCalls;
  installed[game] = Json::parse(raw);
  return 1;
}
static void Access(int game, const char *raw) {
  accesses[game] = Json::parse(raw);
}
static const char *Exits() {
  return lastQueryGame == 0 ? "[{\"game\":1,\"index\":4}]"
                            : "[{\"game\":0,\"index\":1}]";
}
static void Reset() {
  if (cancelOnQuery)
    cancelledProgress.cancel = true;
}
static void Owned(int game, const char *raw) {
  lastOwned[game] = Json::parse(raw).get<std::vector<std::string>>();
}
static const char *Checks(int game) {
  lastQueryGame = game;
  static std::string raw[2];
  ++queries;
  Json checks = Json::array({"Native check", "Inventory gate"});
  const auto &owned = lastOwned[game];
  const bool song = std::count(owned.begin(), owned.end(), "Saria's Song") > 0;
  const bool fixedSongLock = game == 0 && !installed[game].is_null() &&
                             (rejectEveryMap || installed[game] == rejected);
  if (!fixedSongLock || song)
    checks.push_back("Confined song chest");
  if (song) {
    checks.push_back("Confined key chest 01");
    checks.push_back("Confined key chest 02");
  }
  if (song && std::count(owned.begin(), owned.end(), "Small Key") >= 2)
    checks.push_back("Adult dungeon check");
  raw[game] = checks.dump();
  return raw[game].c_str();
}
static uint8_t Portal() { return 1; }
static OracleFns Oracle(int game) {
  return game == 0 ? OracleFns{Reset, [](const char *raw) { Owned(0, raw); },
                               [] { return Checks(0); }, nullptr, Portal}
                   : OracleFns{Reset, [](const char *raw) { Owned(1, raw); },
                               [] { return Checks(1); }, nullptr};
}
static Json Dump() {
  return {{"checks", Json::array({{{"name", "Native check"}},
                                  {{"name", "Inventory gate"}},
                                  {{"name", "Adult dungeon check"}},
                                  {{"name", "Native unavailable check"}}})},
          {"pool", Json::array({{{"name", "Progressive Hookshot"}},
                                {{"name", "Progressive Hookshot"}}})},
          {"fixed",
           Json::array(
               {{{"check", "Confined song chest"}, {"item", "Saria's Song"}},
                {{"check", "Confined key chest 01"}, {"item", "Small Key"}},
                {{"check", "Confined key chest 02"}, {"item", "Small Key"}}})}};
}
static void CheckCancellation(Coordinator &coordinator, OracleFns &oot,
                              OracleFns &mm, const std::string &dump) {
  const auto previous = coordinator.LogicSeed();
  cancelledProgress.Reset();
  cancelOnQuery = true;
  bool cancelled = false;
  try {
    GenerateValidatedSeed(coordinator, true, 123, dump, dump, oot, mm, false, 4,
                          &cancelledProgress);
  } catch (const std::runtime_error &error) {
    cancelled =
        std::string(error.what()).find("cancelled") != std::string::npos;
  }
  cancelOnQuery = false;
  assert(cancelled && "Reset must cancel during a structural oracle query");
  assert(coordinator.LogicSeed() == previous && installed[0] == previous &&
         installed[1] == previous);
  assert(accesses[0]["arrivals"].empty() && accesses[1]["arrivals"].empty());
}

int main() {
  Coordinator coordinator;
  auto oot = Oracle(0), mm = Oracle(1);
  // This must return before reading malformed dumps, touching a host, querying,
  // or rolling maps.
  assert(GenerateValidatedSeed(coordinator, false, 123, "malformed",
                               "malformed", oot, mm, false)
             .is_null());
  assert(hostCalls == 0 && queries == 0);
  coordinator.hosts = {HostFns{[] { return Catalogue(0); },
                               [](const char *raw) { return Install(0, raw); },
                               [](const char *raw) { Access(0, raw); }, Exits,
                               [](const char *) {
                                 ++runtimeCalls;
                                 return 1;
                               }},
                       HostFns{[] { return Catalogue(1); },
                               [](const char *raw) { return Install(1, raw); },
                               [](const char *raw) { Access(1, raw); }, Exits,
                               [](const char *) {
                                 ++runtimeCalls;
                                 return 1;
                               }}};
  const auto dump = Dump().dump();
  const uint64_t seed = 123;
  rejected = GenerateSeed(dungeons, seed);
  // A blanket grant of fixed items certifies this self-locking song map. The
  // real probe must earn the song from its confined check before using it as a
  // key.
  installed[0] = rejected;
  lastOwned[0] = {"Progressive Hookshot", "Progressive Hookshot",
                  "Saria's Song", "Small Key", "Small Key"};
  const auto pregranted = Json::parse(Checks(0));
  assert(std::find(pregranted.begin(), pregranted.end(),
                   "Adult dungeon check") != pregranted.end());
  installed[0] = nullptr;
  const auto accepted = GenerateValidatedSeed(coordinator, true, seed, dump,
                                              dump, oot, mm, false, 16);
  assert(accepted != rejected && coordinator.LogicSeed() == accepted);
  assert(installed[0] == accepted && installed[1] == accepted &&
         runtimeCalls == 0);
  assert(accesses[0]["arrivals"].empty() && accesses[1]["arrivals"].empty() &&
         accesses[0]["overworld"] == true && accesses[1]["overworld"] == true);
  for (const auto &owned : lastOwned) {
    assert(std::count(owned.begin(), owned.end(), "Progressive Hookshot") == 2);
    assert(std::count(owned.begin(), owned.end(), "Small Key") == 2);
    assert(std::count(owned.begin(), owned.end(), "Saria's Song") == 1);
  }
  // Baseline's unavailable named check must not make every candidate
  // impossible.
  const auto replay = GenerateValidatedSeed(coordinator, true, seed, dump, dump,
                                            oot, mm, false, 16);
  assert(replay == accepted); // Retry stream is deterministic and independent
                              // of prior probes.
  const auto callsBeforeSaved = hostCalls;
  bool rejectedSaved = false;
  try {
    ValidateSavedStructure(coordinator, rejected, dump, dump, oot, mm, false);
  } catch (const std::invalid_argument &e) {
    rejectedSaved =
        std::string(e.what()).find("structurally invalid") != std::string::npos;
  }
  assert(rejectedSaved && hostCalls > callsBeforeSaved &&
         coordinator.LogicSeed() == accepted);
  assert(installed[0] == accepted &&
         installed[1] == accepted); // Exact saved map was never rerolled.
  assert(accesses[0]["arrivals"].empty() && accesses[1]["arrivals"].empty());
  ValidateSavedStructure(coordinator, accepted, dump, dump, oot, mm, false);
  assert(coordinator.LogicSeed() == accepted);
  assert(accesses[0]["arrivals"].empty() && accesses[1]["arrivals"].empty());
  rejectEveryMap = true;
  bool exhausted = false;
  try {
    GenerateValidatedSeed(coordinator, true, seed, dump, dump, oot, mm, false,
                          4);
  } catch (const StructureExhausted &e) {
    const auto error = std::string(e.what());
    exhausted = error.find("after 4 attempts") != std::string::npos &&
                error.find("Adult dungeon check") != std::string::npos;
  }
  assert(exhausted && coordinator.LogicSeed() == accepted);
  assert(installed[0] == accepted && installed[1] == accepted &&
         runtimeCalls == 0);
  rejectEveryMap = false;
  CheckCancellation(coordinator, oot, mm, dump);
  std::cout << "cross-dungeon structural generation and cancellation: PASS\n";
}
