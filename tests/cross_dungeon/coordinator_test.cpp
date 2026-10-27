#include "combo/rando/CrossDungeonCoordinator.h"
#include <cassert>
#include <iostream>

using namespace ComboRando::CrossDungeon;
using Json = nlohmann::json;
static Json logic[2], runtime[2];
static int calls = 0, dumps = 0;
static bool rejectMm = false;
static std::vector<Dungeon> dungeons{{"oot:a", {0, 1}, {0, 2}, {0, 3}},
                                     {"mm:b", {1, 4}, {1, 5}, {1, 6}}};
static const char *Catalogue(int game) {
  static std::string raw[2];
  ++dumps;
  raw[game] = Json::array({DungeonJson(dungeons[game])}).dump();
  return raw[game].c_str();
}
static int Set(int game, bool playing, const char *raw) {
  ++calls;
  auto seed = Json::parse(raw);
  Decode(seed);
  if (game == 1 && rejectMm && !seed.is_null())
    return 0;
  (playing ? runtime[game] : logic[game]) = seed;
  return 1;
}
static void Access(const char *) {}
static const char *Exits() { return "[]"; }
struct Oracle {
  void (*SetDungeonAccess)(const char *) = nullptr;
  const char *(*GetDungeonExits)() = nullptr;
};
template <class F> static void Rejects(F f) {
  bool rejected = false;
  try {
    f();
  } catch (const std::exception &) {
    rejected = true;
  }
  assert(rejected);
}
int main() {
  Coordinator c;
  c.hosts = {HostFns{[] { return Catalogue(0); },
                     [](const char *r) { return Set(0, false, r); }, Access,
                     Exits, [](const char *r) { return Set(0, true, r); }},
             HostFns{[] { return Catalogue(1); },
                     [](const char *r) { return Set(1, false, r); }, Access,
                     Exits, [](const char *r) { return Set(1, true, r); }}};
  // Disabled never consults native graph or uses native setters, even with
  // missing symbols.
  Coordinator absent;
  assert(absent.GenerateAndInstall(false, 123).is_null());
  assert(c.GenerateAndInstall(false, 123).is_null());
  Oracle oot, mm;
  c.ConfigureOracles(oot, mm);
  assert(!oot.SetDungeonAccess && !mm.GetDungeonExits && calls == 0 &&
         dumps == 0);

  auto seed = c.GenerateAndInstall(true, 123);
  assert(logic[0] == seed && logic[1] == seed && runtime[0].is_null());
  c.ConfigureOracles(oot, mm);
  assert(oot.SetDungeonAccess == Access && mm.GetDungeonExits == Exits);
  c.BindRuntime({{"crossDungeonEntrances", seed}});
  assert(runtime[0] == seed && runtime[1] == seed);
  const int boundCalls = calls;
  c.BindRuntime({{"crossDungeonEntrances", seed}});
  assert(calls == boundCalls); // Idempotent load hooks cannot clear a queued
                               // runtime arrival.
  // Exact replay installs recorded mapping regardless of the caller's later RNG
  // input.
  c.InstallLogic(nullptr);
  c.InstallLogic(seed);
  assert(c.LogicSeed() == seed);
  {
    Coordinator::LogicScope candidate(c);
    c.GenerateAndInstall(true, 999);
    assert(runtime[0] == seed && runtime[1] == seed);
  }
  assert(logic[0] == seed && logic[1] == seed);
  c.InstallLogic(nullptr);
  rejectMm = true;
  Rejects([&] { c.InstallLogic(seed); });
  assert(logic[0].is_null() && logic[1].is_null() && c.LogicSeed().is_null());
  rejectMm = false;
  c.BindRuntime(Json::object());
  rejectMm = true;
  Rejects([&] { c.BindRuntime({{"crossDungeonEntrances", seed}}); });
  assert(runtime[0].is_null() && runtime[1].is_null() &&
         c.RuntimeSeed().is_null());
  rejectMm = false;
  c.BindRuntime({{"crossDungeonEntrances", seed}});
  auto stale = seed;
  stale["dungeons"][0]["returnTo"]["index"] = 77;
  const int before = calls;
  Rejects([&] { c.InstallLogic(stale); });
  assert(calls == before); // Rejected before either host can change.
  auto oldEmptyAliases = seed;
  oldEmptyAliases["dungeons"][0].erase("escapeExits");
  c.InstallLogic(oldEmptyAliases);
  c.InstallLogic(nullptr);
  dungeons[0].escapeExits.push_back({0, 7});
  Rejects([&] { c.InstallLogic(oldEmptyAliases); });
  dungeons[0].escapeExits.clear();
  auto malformed = seed;
  malformed["permutation"][1] = malformed["permutation"][0];
  Rejects([&] { c.BindRuntime({{"crossDungeonEntrances", malformed}}); });
  assert(runtime[0] == seed && runtime[1] == seed);
  c.BindRuntime(Json::object()); // Old seed clears the last slot's routing.
  assert(runtime[0].is_null() && runtime[1].is_null());
  c.ConfigureOracles(oot, mm);
  assert(!oot.SetDungeonAccess && !mm.GetDungeonExits);
  std::cout << "cross-dungeon coordinator: PASS\n";
}
