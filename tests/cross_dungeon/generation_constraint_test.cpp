#include "combo/rando/CrossDungeonGeneration.h"
#include <cassert>
#include <iostream>

using namespace ComboRando;
using namespace ComboRando::CrossDungeon;
using Json = nlohmann::json;

static std::vector<Dungeon> entries;
static Json installed[2], accesses[2];
static bool impossible = false;
static int runtimeCalls = 0;
static const char* Catalogue(int game) {
    static std::string raw[2];
    auto list = Json::array();
    for (const auto& entry : entries)
        if (entry.entrance.game == game)
            list.push_back(DungeonJson(entry));
    raw[game] = list.dump();
    return raw[game].c_str();
}
static int Install(int game, const char* raw) {
    installed[game] = Json::parse(raw);
    return 1;
}
static const char* Checks(int game) {
    // A mandatory native starting entrance must stay in place. Other entrances
    // may still cross games; rejecting entire random maps rarely finds this.
    return game == 0 && !installed[0].is_null() &&
                   (impossible || installed[0].at("permutation").at(0) != 0)
               ? "[]" : "[\"Native check\"]";
}
int main() {
    for (int i = 0; i < 4; ++i) {
        Dungeon entry{"interior" + std::to_string(i), {i / 2, i * 3}, {i / 2, i * 3 + 1},
                      {i / 2, i * 3 + 1}};
        entry.pool = Pool::Interior;
        entries.push_back(entry);
    }
    Coordinator coordinator;
    coordinator.selectedPools.dungeons = false;
    coordinator.selectedPools.interiors = true;
    for (int game = 0; game < 2; ++game) {
        auto& host = coordinator.hosts[game];
        host.GetExits = [] { return "[]"; };
        host.SetRuntimeSeed = [](const char*) { ++runtimeCalls; return 1; };
    }
    coordinator.hosts[0].DumpCatalogue = [] { return Catalogue(0); };
    coordinator.hosts[1].DumpCatalogue = [] { return Catalogue(1); };
    coordinator.hosts[0].DumpCatalogueForOptions = [](const char*) { return Catalogue(0); };
    coordinator.hosts[1].DumpCatalogueForOptions = [](const char*) { return Catalogue(1); };
    coordinator.hosts[0].SetSeed = [](const char* raw) { return Install(0, raw); };
    coordinator.hosts[1].SetSeed = [](const char* raw) { return Install(1, raw); };
    coordinator.hosts[0].SetAccess = [](const char* raw) { accesses[0] = Json::parse(raw); };
    coordinator.hosts[1].SetAccess = [](const char* raw) { accesses[1] = Json::parse(raw); };
    OracleFns oot{[] {}, [](const char*) {}, [] { return Checks(0); }, nullptr, []() -> uint8_t { return 1; }};
    OracleFns mm{[] {}, [](const char*) {}, [] { return Checks(1); }, nullptr};
    const auto dump = Json{{"checks", Json::array({{{"name", "Native check"}}})},
                           {"pool", Json::array()}}.dump();
    uint64_t master = 0;
    while (true) {
        const auto proposed = GenerateSeed(entries, master).at("permutation").get<std::vector<size_t>>();
        bool noncriticalCross = false;
        for (size_t i = 1; i < proposed.size(); ++i)
            noncriticalCross |= proposed[i] != 0 && entries[i].entrance.game != entries[proposed[i]].entrance.game;
        if (proposed[0] != 0 && noncriticalCross)
            break;
        ++master;
    }
    const auto seed = GenerateValidatedSeed(coordinator, true, master, dump, dump, oot, mm, false, 1);
    assert(seed.at("permutation").at(0) == 0);
    const auto permutation = seed.at("permutation").get<std::vector<size_t>>();
    bool crossed = false;
    for (size_t i = 0; i < permutation.size(); ++i)
        crossed |= entries[i].entrance.game != entries[permutation[i]].entrance.game;
    assert(crossed && "a constrained layout must still shuffle across games");
    assert(installed[0] == seed && installed[1] == seed && coordinator.LogicSeed() == seed);
    assert(runtimeCalls == 0 && accesses[0].at("arrivals").empty() && accesses[1].at("arrivals").empty());
    assert(GenerateValidatedSeed(coordinator, true, master, dump, dump, oot, mm, false, 1) == seed);
    ValidateSavedStructure(coordinator, seed, dump, dump, oot, mm, false);
    impossible = true;
    bool rejected = false;
    try {
        GenerateValidatedSeed(coordinator, true, master, dump, dump, oot, mm, false, 1);
    } catch (const StructureExhausted&) {
        rejected = true;
    }
    assert(rejected && coordinator.LogicSeed() == seed && installed[0] == seed && installed[1] == seed);
    assert(runtimeCalls == 0 && accesses[0].at("arrivals").empty() && accesses[1].at("arrivals").empty());
    std::cout << "constrained v2 generation: real cross-game shuffle, deterministic replay and fail-closed rollback PASS\n";
}
