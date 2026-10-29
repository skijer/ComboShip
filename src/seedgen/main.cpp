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
    std::string settingsPath;
};

struct Fill {
    ComboRando::CombinedFillResult result;
    uint32_t masterSeed;
    std::string ootDump;
    std::string mmDump;
};

const char* const kGameKeys[GameCount] = { "oot", "mm" };

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
        } else if (flag == "--settings") {
            options.settingsPath = value;
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

// Each game block of a spoiler carries that game's settings snapshot, so another spoiler can seed this one.
bool RestoreSettingsFrom(const std::string& spoilerPath) {
    const nlohmann::json spoiler = nlohmann::json::parse(std::ifstream(spoilerPath), nullptr, false);
    if (!spoiler.is_object()) {
        std::cerr << "[fleet-seedgen] could not read settings from " << spoilerPath << "\n";
        return false;
    }
    for (int game = 0; game < GameCount; game++) {
        const nlohmann::json block = spoiler.value(kGameKeys[game], nlohmann::json::object());
        sGames[game]->Request("RestoreSettings", block.dump());
    }
    return true;
}

// One whole fill: both games roll their own pools from masterSeed, then the combined fill places the rest.
Fill FillOnce(uint32_t masterSeed) {
    const std::string seed = nlohmann::json{ { "seed", masterSeed } }.dump();
    for (const auto& game : sGames) {
        game->Request("SetSeed", seed);
    }
    Fill fill = { .masterSeed = masterSeed,
                  .ootDump = Oracle<Oot>::DumpStaticData(),
                  .mmDump = Oracle<Mm>::DumpStaticData() };
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
    fill.result = ComboRando::CrossWorldCombinedFill(fill.ootDump, fill.mmDump, masterSeed, ootOracle, mmOracle,
                                                     nullptr, forcedOot, ComboRando::OotAccessFromDump(fill.ootDump));
    return fill;
}

// ComboShip's consolidated layout: per game its settings snapshot, placements and the prices the seed rolled.
nlohmann::json BuildSpoiler(const Fill& fill, uint32_t seed) {
    const nlohmann::json placements = nlohmann::json::parse(fill.result.spoilerJson);
    const std::string dumps[GameCount] = { fill.ootDump, fill.mmDump };
    nlohmann::json spoiler = { { "fileType", "ComboShipRandomizer" },
                               { "seed", seed },
                               { "masterSeed", fill.masterSeed },
                               { "goal", { { "type", "bosses" } } },
                               { "startingGame", "OOT" },
                               { "foreign", placements.value("foreign", nlohmann::json::array()) } };
    for (int game = 0; game < GameCount; game++) {
        nlohmann::json block = nlohmann::json::parse(sGames[game]->Request("DumpSettings"), nullptr, false);
        if (!block.is_object()) {
            block = nlohmann::json::object();
        }
        block["placements"] = placements.value(kGameKeys[game], nlohmann::json::object());
        block["prices"] = nlohmann::json::parse(dumps[game]).value("prices", nlohmann::json::object());
        spoiler[kGameKeys[game]] = std::move(block);
    }
    return spoiler;
}

int GenerateSeed(int argc, char* argv[]) {
    Options options;
    if (!ParseOptions(argc, argv, options)) {
        std::cerr << "usage: fleet-seedgen --oot <Ship folder> --mm <2Ship folder> [--seed N] [--out spoiler.json] "
                     "[--settings spoiler.json]\n";
        return 2;
    }

    Ship::Context* context = Ship::Context::CreateUninitializedInstance("Fleet", "fleet", "fleetship.json");
    context->InitLogging();
    context->InitConfiguration();
    context->InitConsoleVariables();
    if (!AttachGames(options)) {
        return 2;
    }
    if (!options.settingsPath.empty() && !RestoreSettingsFrom(options.settingsPath)) {
        return 2;
    }

    // Same re-derived seeds as ComboShip's generator, so a seed number names the same layout in both.
    Fill fill = {};
    for (int attempt = 0; attempt < ComboRando::kFillAttempts && !fill.result.success; attempt++) {
        fill = FillOnce(options.seed + attempt * 0x9E3779B9u);
    }
    if (!fill.result.success) {
        sGames[Mm]->Request("Finish");
        std::cerr << "[fleet-seedgen] seed " << options.seed << " failed: " << fill.result.error << "\n";
        return 1;
    }
    // Finish drops the seed's restored lists, so the settings snapshot is taken first.
    const nlohmann::json spoiler = BuildSpoiler(fill, options.seed);
    sGames[Mm]->Request("Finish");
    std::cout << "[fleet-seedgen] seed " << options.seed << " PASS: " << spoiler["oot"]["placements"].size()
              << " OoT and " << spoiler["mm"]["placements"].size() << " MM placements, "
              << spoiler["foreign"].size() << " across games\n";
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
