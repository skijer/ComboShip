// fleet-seedgen: generates one combined OoT + MM seed from the installed Ship and 2Ship builds, with no window.

#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include <nlohmann/json.hpp>
#include <ship/Context.h>
#include <ship/GameRegistry.h>

#include "GameModule.h"
#include "rando/CrossWorldRando.h"

#ifdef _WIN32
#include <Windows.h>
#endif

namespace {

enum GameIndex { Oot, Mm, GameCount };

std::array<std::unique_ptr<GameModule>, GameCount> sGames;

struct Options {
    std::string ootDirectory;
    std::string mmDirectory;
    uint32_t seed = 1;
    std::string outputPath;
};

void LogFromGame(const char* game, FleetLogLevel level, const char* message) {
    std::cerr << "[" << game << "] " << message << "\n";
}

const FleetHostApi sHost = { .structSize = sizeof(FleetHostApi),
                             .major = FLEET_GAME_API_MAJOR,
                             .minor = FLEET_GAME_API_MINOR,
                             .Log = LogFromGame };

// The combined fill takes plain function pointers, so each game gets its own set of entry points.
template <GameIndex Game> struct Oracle {
    static const GameModule& Module() {
        return *sGames[Game];
    }

    static void Reset() {
        Ship::GameScope scope(Module().Name());
        Module().Api().RandoReset();
    }

    static void SetOwnedItems(const char* itemsJson) {
        Ship::GameScope scope(Module().Name());
        Module().Api().RandoSetOwnedItems(itemsJson);
    }

    // The fill reads the result before the next query, so one buffer per game is enough.
    static const char* GetReachableChecks() {
        static std::string reachable;
        Ship::GameScope scope(Module().Name());
        reachable = Module().TakeString(Module().Api().RandoGetReachableChecks());
        return reachable.c_str();
    }

    static void PlaceItem(const char* check, const char* item) {
        Ship::GameScope scope(Module().Name());
        Module().Api().RandoPlaceItem(check, item);
    }

    static uint8_t GetPortalOpen() {
        const nlohmann::json reply = nlohmann::json::parse(Module().Request("GetPortalOpen"), nullptr, false);
        return reply.is_object() && reply.value("open", false) ? 1 : 0;
    }

    static std::string DumpStaticData() {
        Ship::GameScope scope(Module().Name());
        return Module().TakeString(Module().Api().DumpRandoStaticData());
    }
};

bool ParseOptions(int argc, char* argv[], Options& options) {
    for (int i = 1; i + 1 < argc; i += 2) {
        const std::string flag = argv[i];
        const std::string value = argv[i + 1];
        if (flag == "--oot") {
            options.ootDirectory = value;
        } else if (flag == "--mm") {
            options.mmDirectory = value;
        } else if (flag == "--seed") {
            options.seed = static_cast<uint32_t>(std::strtoul(value.c_str(), nullptr, 10));
        } else if (flag == "--out") {
            options.outputPath = value;
        } else {
            return false;
        }
    }
    return !options.ootDirectory.empty() && !options.mmDirectory.empty();
}

bool AttachGames(const Options& options) {
    sGames[Oot] = GameModule::Load(options.ootDirectory, "soh", sHost);
    sGames[Mm] = GameModule::Load(options.mmDirectory, "2ship", sHost);
    if (sGames[Oot] == nullptr || sGames[Mm] == nullptr) {
        return false;
    }
    for (const auto& game : sGames) {
        game->Request("InitRandoHeadless");
    }
    return true;
}

// One whole fill: both games roll their own pools from masterSeed, then the combined fill places the rest.
ComboRando::CombinedFillResult FillOnce(uint32_t masterSeed) {
    const std::string seed = nlohmann::json{ { "seed", masterSeed } }.dump();
    for (const auto& game : sGames) {
        game->Request("SetSeed", seed);
    }
    const std::string ootDump = Oracle<Oot>::DumpStaticData();
    const std::string mmDump = Oracle<Mm>::DumpStaticData();
    const std::string forcedOot = sGames[Oot]->Request("GetForcedPlacements");
    const ComboRando::OracleFns ootOracle = { .Reset = Oracle<Oot>::Reset,
                                              .SetOwnedItems = Oracle<Oot>::SetOwnedItems,
                                              .GetReachableChecks = Oracle<Oot>::GetReachableChecks,
                                              .PlaceItem = Oracle<Oot>::PlaceItem,
                                              .GetPortalOpen = Oracle<Oot>::GetPortalOpen };
    const ComboRando::OracleFns mmOracle = { .Reset = Oracle<Mm>::Reset,
                                             .SetOwnedItems = Oracle<Mm>::SetOwnedItems,
                                             .GetReachableChecks = Oracle<Mm>::GetReachableChecks,
                                             .PlaceItem = Oracle<Mm>::PlaceItem };
    return ComboRando::CrossWorldCombinedFill(ootDump, mmDump, masterSeed, ootOracle, mmOracle, nullptr, forcedOot,
                                              ComboRando::OotAccessFromDump(ootDump));
}

int GenerateSeed(int argc, char* argv[]) {
    Options options;
    if (!ParseOptions(argc, argv, options)) {
        std::cerr << "usage: fleet-seedgen --oot <Ship folder> --mm <2Ship folder> [--seed N] [--out spoiler.json]\n";
        return 2;
    }

    Ship::Context* context = Ship::Context::CreateUninitializedInstance("Fleet", "fleet", "fleetship.json");
    context->InitLogging();
    context->InitConfiguration();
    context->InitConsoleVariables();
    if (!AttachGames(options)) {
        return 2;
    }

    // Same re-derived seeds as ComboShip's generator, so a seed number names the same layout in both.
    ComboRando::CombinedFillResult result = {};
    for (int attempt = 0; attempt < ComboRando::kFillAttempts && !result.success; attempt++) {
        result = FillOnce(options.seed + attempt * 0x9E3779B9u);
    }
    sGames[Mm]->Request("Finish");

    if (!result.success) {
        std::cerr << "[fleet-seedgen] seed " << options.seed << " failed: " << result.error << "\n";
        return 1;
    }
    const nlohmann::json spoiler = nlohmann::json::parse(result.spoilerJson);
    std::cout << "[fleet-seedgen] seed " << options.seed << " PASS: "
              << spoiler.value("oot", nlohmann::json::object()).size() << " OoT and "
              << spoiler.value("mm", nlohmann::json::object()).size() << " MM placements, "
              << spoiler.value("foreign", nlohmann::json::array()).size() << " across games\n";
    if (!options.outputPath.empty()) {
        std::ofstream(options.outputPath) << spoiler.dump(2);
    }
    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    const int status = GenerateSeed(argc, argv);
    std::cout.flush();
    std::cerr.flush();
    // The games have no headless shutdown yet: a normal exit would run their static destructors after
    // their resource workers were killed mid-task, so the process ends without tearing anything down.
#ifdef _WIN32
    TerminateProcess(GetCurrentProcess(), static_cast<UINT>(status));
#endif
    std::_Exit(status);
}
