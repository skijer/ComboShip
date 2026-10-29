// fleet-seedgen: generates one combined OoT + MM seed from the installed Ship and 2Ship builds, with no window.

#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
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

enum class StartingGame { Oot, Mm, Random };

struct Options {
    std::string ootDirectory;
    std::string mmDirectory;
    uint32_t seed = 1;
    std::string outputPath;
    std::string settingsPath;
    ComboRando::CwGoal goal;
    StartingGame start = StartingGame::Oot;
    uint32_t sharedMask = 0;
};

struct Fill {
    ComboRando::CombinedFillResult result;
    uint32_t masterSeed;
    bool mmStart;
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

// "required:total" pieces across both games; total defaults to required.
bool ParseTriforceHunt(const std::string& value, ComboRando::CwGoal& goal) {
    const size_t colon = value.find(':');
    const int required = std::atoi(value.substr(0, colon).c_str());
    const int total = colon == std::string::npos ? required : std::atoi(value.substr(colon + 1).c_str());
    if (required < 1 || total < required || total > ComboRando::kMaxComboTriforcePieces) {
        return false;
    }
    goal = { .hunt = true, .required = required, .total = total };
    return true;
}

bool ParseStartingGame(const std::string& value, StartingGame& start) {
    if (value == "oot" || value == "OOT") {
        start = StartingGame::Oot;
    } else if (value == "mm" || value == "MM") {
        start = StartingGame::Mm;
    } else if (value == "random") {
        start = StartingGame::Random;
    } else {
        return false;
    }
    return true;
}

// FNV-1a over ComboShip's derivation string, so a Random start resolves the same way in both generators.
bool RollsMmStart(uint32_t masterSeed) {
    uint32_t hash = 2166136261u;
    for (unsigned char c : "startingGame:" + std::to_string(masterSeed)) {
        hash = (hash ^ c) * 16777619u;
    }
    return (hash & 1u) != 0;
}

// "all", or the family keys of combo/rando/SharedItems.h separated by commas.
bool ParseSharedItems(const std::string& value, uint32_t& mask) {
    if (value == "all") {
        mask = ComboRando::SharedMaskAll();
        return true;
    }
    nlohmann::json keys = nlohmann::json::array();
    std::stringstream names(value);
    std::string name;
    while (std::getline(names, name, ',')) {
        keys.push_back(name);
    }
    mask = ComboRando::SharedMaskFromKeys(keys);
    return mask != 0;
}

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
        } else if (flag == "--triforce") {
            if (!ParseTriforceHunt(value, options.goal)) {
                return false;
            }
        } else if (flag == "--start") {
            if (!ParseStartingGame(value, options.start)) {
                return false;
            }
        } else if (flag == "--shared") {
            if (!ParseSharedItems(value, options.sharedMask)) {
                return false;
            }
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
// Its goal and starting game come along, unless the command line names its own.
bool RestoreSettingsFrom(const std::string& spoilerPath, Options& options, bool commandLineGoal) {
    const nlohmann::json spoiler = nlohmann::json::parse(std::ifstream(spoilerPath), nullptr, false);
    if (!spoiler.is_object()) {
        std::cerr << "[fleet-seedgen] could not read settings from " << spoilerPath << "\n";
        return false;
    }
    for (int game = 0; game < GameCount; game++) {
        const nlohmann::json block = spoiler.value(kGameKeys[game], nlohmann::json::object());
        sGames[game]->Request("RestoreSettings", block.dump());
    }
    const nlohmann::json goal = spoiler.value("goal", nlohmann::json::object());
    if (!commandLineGoal && goal.value("type", std::string()) == "triforceHunt") {
        options.goal = { .hunt = true,
                         .required = goal.value("requiredPieces", 0),
                         .total = goal.value("totalPieces", -1) };
    }
    if (!commandLineGoal) {
        ParseStartingGame(spoiler.value("startingGame", std::string("OOT")), options.start);
    }
    if (options.sharedMask == 0) {
        options.sharedMask = ComboRando::SharedMaskFromKeys(spoiler.value("sharedItems", nlohmann::json::array()));
    }
    return true;
}

void SendComboOptions(const Options& options, bool mmStart) {
    const std::string startingGame = mmStart ? "MM" : "OOT";
    const ComboRando::CwGoal& goal = options.goal;
    const int pieces[GameCount] = { ComboRando::CwOotPieces(goal.total), ComboRando::CwMmPieces(goal.total) };
    const bool sharedWallets = (options.sharedMask & (1u << ComboRando::SF_WALLET)) != 0;
    for (int game = 0; game < GameCount; game++) {
        const nlohmann::json request = { { "hunt", goal.hunt },
                                         { "required", goal.required },
                                         { "pieces", pieces[game] },
                                         { "startingGame", startingGame },
                                         { "sharedWallets", sharedWallets } };
        sGames[game]->Request("SetComboOptions", request.dump());
    }
}

// One whole fill: both games roll their own pools from masterSeed, then the combined fill places the rest.
Fill FillOnce(uint32_t masterSeed, const Options& options, bool mmStart) {
    const std::string seed = nlohmann::json{ { "seed", masterSeed } }.dump();
    for (const auto& game : sGames) {
        game->Request("SetSeed", seed);
    }
    SendComboOptions(options, mmStart);
    const ComboRando::CwGoal& goal = options.goal;
    Fill fill = { .masterSeed = masterSeed,
                  .mmStart = mmStart,
                  .ootDump = Oracle<Oot>::DumpStaticData(),
                  .mmDump = Oracle<Mm>::DumpStaticData() };
    if (fill.ootDump.empty() || fill.mmDump.empty()) {
        fill.result.error = "a game could not prepare its seed (see its log)";
        return fill;
    }
    if (goal.hunt && ComboRando::CountPoolTriforcePieces(fill.ootDump, fill.mmDump) < goal.required) {
        fill.result.error = "the combined pool holds fewer Triforce pieces than the hunt requires";
        return fill;
    }
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
    fill.result = ComboRando::CrossWorldCombinedFill(
        fill.ootDump, fill.mmDump, masterSeed, ootOracle, mmOracle, nullptr, forcedOot,
        ComboRando::OotAccessFromDump(fill.ootDump), goal, mmStart ? ComboRando::GAME_MM : ComboRando::GAME_OOT, "",
        options.sharedMask);
    return fill;
}

nlohmann::json GoalJson(const ComboRando::CwGoal& goal) {
    if (!goal.hunt) {
        return { { "type", "bosses" } };
    }
    return { { "type", "triforceHunt" }, { "requiredPieces", goal.required }, { "totalPieces", goal.total } };
}

// ComboShip's consolidated layout: per game its settings snapshot, placements and the prices the seed rolled.
// OoT's entrance layout moves to the top-level "entrances" block, where ComboShip's validator reads it.
nlohmann::json BuildSpoiler(const Fill& fill, uint32_t seed, const ComboRando::CwGoal& goal) {
    const nlohmann::json placements = nlohmann::json::parse(fill.result.spoilerJson);
    const std::string dumps[GameCount] = { fill.ootDump, fill.mmDump };
    nlohmann::json spoiler = { { "fileType", "ComboShipRandomizer" },
                               { "seed", seed },
                               { "masterSeed", fill.masterSeed },
                               { "goal", GoalJson(goal) },
                               { "startingGame", fill.mmStart ? "MM" : "OOT" },
                               { "sharedItems", placements.value("sharedItems", nlohmann::json::array()) },
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
    spoiler["entrances"] = { { "oot", spoiler["oot"].value("entrances", nlohmann::json::array()) } };
    spoiler["oot"].erase("entrances");
    return spoiler;
}

// A Random start falls back to OoT on the last attempt, as ComboShip's generator does.
bool ResolvesMmStart(StartingGame start, uint32_t masterSeed, int attempt) {
    if (start == StartingGame::Random) {
        return attempt + 1 < ComboRando::kFillAttempts && RollsMmStart(masterSeed);
    }
    return start == StartingGame::Mm;
}

int GenerateSeed(int argc, char* argv[]) {
    Options options;
    if (!ParseOptions(argc, argv, options)) {
        std::cerr << "usage: fleet-seedgen --oot <Ship folder> --mm <2Ship folder> [--seed N] [--out spoiler.json]\n"
                     "                     [--settings spoiler.json] [--triforce required[:total]]\n"
                     "                     [--start oot|mm|random] [--shared all|bows,wallets,...]\n";
        return 2;
    }
    const bool commandLineGoal = options.goal.hunt || options.start != StartingGame::Oot;

    Ship::Context* context = Ship::Context::CreateUninitializedInstance("Fleet", "fleet", "fleetship.json");
    context->InitLogging();
    context->InitConfiguration();
    context->InitConsoleVariables();
    if (!AttachGames(options)) {
        return 2;
    }
    if (!options.settingsPath.empty() && !RestoreSettingsFrom(options.settingsPath, options, commandLineGoal)) {
        return 2;
    }

    // Same re-derived seeds as ComboShip's generator, so a seed number names the same layout in both.
    Fill fill = {};
    for (int attempt = 0; attempt < ComboRando::kFillAttempts && !fill.result.success; attempt++) {
        const uint32_t masterSeed = options.seed + attempt * 0x9E3779B9u;
        fill = FillOnce(masterSeed, options, ResolvesMmStart(options.start, masterSeed, attempt));
    }
    if (!fill.result.success) {
        sGames[Mm]->Request("Finish");
        std::cerr << "[fleet-seedgen] seed " << options.seed << " failed: " << fill.result.error << "\n";
        return 1;
    }
    // Finish drops the seed's restored lists, so the settings snapshot is taken first.
    const nlohmann::json spoiler = BuildSpoiler(fill, options.seed, options.goal);
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
