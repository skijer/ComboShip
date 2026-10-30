// fleet-seedgen: generates one combined OoT + MM seed from the installed Ship and 2Ship builds, with no window.

#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>

#include <nlohmann/json.hpp>
#include <ship/Context.h>
#include <ship/GameRegistry.h>

#include "GameModule.h"
#include "rando/ComboPlaythrough.h"
#include "rando/CrossDungeonCoordinator.h"
#include "rando/CrossDungeonGeneration.h"
#include "rando/CrossForeign.h"
#include "rando/CrossHints.h"
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
    std::string dumpDirectory;
    std::optional<ComboRando::CrossDungeon::PoolOptions> crossPools;
    std::string playthroughPath;
};

struct Fill {
    ComboRando::CombinedFillResult result;
    uint32_t masterSeed;
    bool mmStart;
    std::string ootDump;
    std::string mmDump;
    std::string ootHintDump;
    ComboRando::RequirednessResult requiredness;
    nlohmann::json crossDungeonSeed;
    // Routed through the cross-game entrances once those are laid out.
    ComboRando::OracleFns ootOracle;
    ComboRando::OracleFns mmOracle;
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

// The cross-dungeon coordinator also takes plain function pointers; an empty reply is a refusal.
template <GameIndex Game> struct CrossHost {
    static const char* Reply(const std::string& op, const std::string& json) {
        static std::string reply;
        reply = sGames[Game]->Request(op, json);
        return reply.empty() ? nullptr : reply.c_str();
    }

    static const char* DumpCatalogue() {
        return Reply("DumpCrossDungeonCatalogue", "{}");
    }

    static const char* DumpCatalogueForOptions(const char* options) {
        return Reply("DumpCrossDungeonCatalogue", options);
    }

    static const char* DumpSeedCatalogue(const char* spoiler) {
        return Reply("DumpCrossDungeonCatalogueForSeed", spoiler);
    }

    static int SetSeed(const char* seed) {
        return Reply("SetCrossDungeonSeed", seed) != nullptr ? 1 : 0;
    }

    static void SetAccess(const char* access) {
        Reply("SetCrossDungeonAccess", access);
    }

    static const char* GetExits() {
        return Reply("GetCrossDungeonExits", "{}");
    }

    static int CanEnterArrival(int32_t entrance) {
        const char* reply = Reply("CanEnterCrossEntrance", nlohmann::json{ { "entrance", entrance } }.dump());
        return reply != nullptr && nlohmann::json::parse(reply).value("allowed", false) ? 1 : 0;
    }

    static ComboRando::CrossDungeon::HostFns Fns() {
        ComboRando::CrossDungeon::HostFns fns;
        fns.DumpCatalogue = DumpCatalogue;
        fns.SetSeed = SetSeed;
        fns.SetAccess = SetAccess;
        fns.GetExits = GetExits;
        fns.DumpSeedCatalogue = DumpSeedCatalogue;
        fns.DumpCatalogueForOptions = DumpCatalogueForOptions;
        if (Game == Mm) {
            fns.CanEnterArrival = CanEnterArrival;
        }
        return fns;
    }
};

ComboRando::CrossDungeon::Coordinator sCrossDungeons;

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

// The cross-game entrance pools to shuffle, e.g. "dungeons,interiors,grottos".
bool ParseCrossPools(const std::string& value, std::optional<ComboRando::CrossDungeon::PoolOptions>& pools) {
    const nlohmann::json known = ComboRando::CrossDungeon::OptionsJson({});
    nlohmann::json flags = { { "dungeons", false } };
    std::stringstream names(value);
    std::string name;
    while (std::getline(names, name, ',')) {
        if (!known.contains(name)) {
            return false;
        }
        flags[name] = true;
    }
    pools = ComboRando::CrossDungeon::ParseOptions(flags);
    return pools->Any();
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
        } else if (flag == "--cross") {
            if (!ParseCrossPools(value, options.crossPools)) {
                return false;
            }
        } else if (flag == "--playthrough") {
            options.playthroughPath = value;
        } else if (flag == "--dumps") {
            options.dumpDirectory = value;
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
    sCrossDungeons.hosts = { CrossHost<Oot>::Fns(), CrossHost<Mm>::Fns() };
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
    const nlohmann::json crossSeed = ComboRando::CrossDungeon::Coordinator::SeedOf(spoiler);
    if (!options.crossPools && ComboRando::CrossDungeon::Coordinator::Enabled(crossSeed)) {
        options.crossPools = ComboRando::CrossDungeon::OptionsFromSeed(crossSeed);
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

const ComboRando::OracleFns kOotOracle = { .Reset = Oracle<Oot>::Reset,
                                           .SetOwnedItems = Oracle<Oot>::SetOwnedItems,
                                           .GetReachableChecks = Oracle<Oot>::GetReachableChecks,
                                           .PlaceItem = Oracle<Oot>::PlaceItem,
                                           .GetPortalOpen = Oracle<Oot>::GetPortalOpen };
const ComboRando::OracleFns kMmOracle = { .Reset = Oracle<Mm>::Reset,
                                          .SetOwnedItems = Oracle<Mm>::SetOwnedItems,
                                          .GetReachableChecks = Oracle<Mm>::GetReachableChecks,
                                          .PlaceItem = Oracle<Mm>::PlaceItem };

// What each game handed the engine, for inspecting a seed by hand.
void WriteDumps(const Fill& fill, const std::string& directory) {
    if (directory.empty()) {
        return;
    }
    std::filesystem::create_directories(directory);
    std::ofstream(std::filesystem::path(directory) / "oot.json") << fill.ootDump;
    std::ofstream(std::filesystem::path(directory) / "mm.json") << fill.mmDump;
    std::ofstream(std::filesystem::path(directory) / "oot-hints.json") << fill.ootHintDump;
    std::ofstream(std::filesystem::path(directory) / "fill.json") << fill.result.spoilerJson;
}

// Which placements the goal truly needs, for Way of the Hero and Foolish hints. Runs while the games still
// hold this attempt's seed. Without logic only MM's half is checked, since OoT may be unbeatable by design.
ComboRando::RequirednessResult FindRequiredItems(const Fill& fill, const Options& options, bool noLogic) {
    if (options.goal.hunt && noLogic) {
        return {};
    }
    if (!ComboRando::NeedsRequirednessPareDown(fill.ootHintDump, fill.mmDump)) {
        return {};
    }
    std::unordered_map<std::string, std::string> ootAreas;
    const nlohmann::json ootHints = nlohmann::json::parse(fill.ootHintDump, nullptr, false);
    for (const nlohmann::json& check : ootHints.value("checks", nlohmann::json::array())) {
        ootAreas.emplace(check.value("name", ""), check.value("area", ""));
    }
    std::unordered_map<std::string, std::string> mmAreas;
    const nlohmann::json mmLocationHints =
        nlohmann::json::parse(fill.mmDump).value("locationHints", nlohmann::json::object());
    for (const auto& [check, area] : mmLocationHints.items()) {
        mmAreas.emplace(check, area.get<std::string>());
    }
    ComboRando::GoalPredicate goal = ComboRando::DefaultGanonMajoraGoal;
    if (options.goal.hunt) {
        goal = ComboRando::MakeTriforceHuntGoal(options.goal.required);
    } else if (noLogic) {
        goal = ComboRando::MmOnlyMajoraGoal;
    }
    return ComboRando::PareDownPlaythrough(fill.result.spoilerJson, fill.ootOracle, fill.mmOracle, nullptr,
                                           fill.ootDump, fill.mmDump, ootAreas, mmAreas, goal, !noLogic, fill.mmStart,
                                           nullptr, options.sharedMask);
}

// The shared entrance pools across both games, structurally validated against both oracles, which it leaves
// routed through the chosen layout.
bool LayOutCrossEntrances(Fill& fill, const Options& options) {
    const bool enabled = options.crossPools.has_value();
    if (enabled) {
        sCrossDungeons.SetOptions(*options.crossPools);
    }
    try {
        fill.crossDungeonSeed =
            ComboRando::CrossDungeon::GenerateValidatedSeed(sCrossDungeons, enabled, fill.masterSeed, fill.ootDump,
                                                            fill.mmDump, fill.ootOracle, fill.mmOracle, fill.mmStart);
    } catch (const std::exception& e) {
        fill.result.error = e.what();
        return false;
    }
    return true;
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
                  .mmDump = Oracle<Mm>::DumpStaticData(),
                  .ootOracle = kOotOracle,
                  .mmOracle = kMmOracle };
    if (fill.ootDump.empty() || fill.mmDump.empty()) {
        fill.result.error = "a game could not prepare its seed (see its log)";
        return fill;
    }
    if (goal.hunt && ComboRando::CountPoolTriforcePieces(fill.ootDump, fill.mmDump) < goal.required) {
        fill.result.error = "the combined pool holds fewer Triforce pieces than the hunt requires";
        return fill;
    }
    const std::string forcedOot = sGames[Oot]->Request("GetForcedPlacements");
    if (!LayOutCrossEntrances(fill, options)) {
        return fill;
    }
    const ComboRando::OotAccess ootAccess = ComboRando::OotAccessFromDump(fill.ootDump);
    fill.result = ComboRando::CrossWorldCombinedFill(fill.ootDump, fill.mmDump, masterSeed, fill.ootOracle,
                                                     fill.mmOracle, nullptr, forcedOot, ootAccess, goal,
                                                     mmStart ? ComboRando::GAME_MM : ComboRando::GAME_OOT, "",
                                                     options.sharedMask);
    if (fill.result.success) {
        fill.ootHintDump = sGames[Oot]->Request("DumpHintData");
        WriteDumps(fill, options.dumpDirectory);
        fill.requiredness = FindRequiredItems(fill, options, ootAccess == ComboRando::OotAccess::NO_LOGIC);
    }
    return fill;
}

nlohmann::json GoalJson(const ComboRando::CwGoal& goal) {
    if (!goal.hunt) {
        return { { "type", "bosses" } };
    }
    return { { "type", "triforceHunt" }, { "requiredPieces", goal.required }, { "totalPieces", goal.total } };
}

// ComboShip's consolidated layout: per game its settings snapshot, placements and the prices the seed rolled.
// Item names both games use get a game tag, cross-game traps get their disguises, and the hints are composed
// over both games. OoT's entrance layout moves to the top-level "entrances" block, where ComboShip's validator
// reads it.
nlohmann::json BuildSpoiler(const Fill& fill, uint32_t seed, const ComboRando::CwGoal& goal) {
    const nlohmann::json fillSpoiler = nlohmann::json::parse(fill.result.spoilerJson);
    const nlohmann::json sharedItems = fillSpoiler.value("sharedItems", nlohmann::json::array());
    const uint32_t sharedMask = ComboRando::SharedMaskFromKeys(sharedItems);
    nlohmann::json placements[GameCount] = { fillSpoiler.value("oot", nlohmann::json::object()),
                                              fillSpoiler.value("mm", nlohmann::json::object()) };
    const nlohmann::json rawForeign = fillSpoiler.value("foreign", nlohmann::json::array());
    ComboRando::SuffixCrossGameItems(placements[Oot], placements[Mm], rawForeign, fill.ootDump, fill.mmDump,
                                     ComboRando::SharedUntaggedNames(sharedMask));
    nlohmann::json disguised = rawForeign;
    ComboRando::AssignTrapDisguises(disguised, fillSpoiler.value("oot", nlohmann::json::object()),
                                    fillSpoiler.value("mm", nlohmann::json::object()), fill.ootDump, fill.mmDump,
                                    fill.masterSeed);
    const nlohmann::json foreign = ComboRando::BuildForeignArray(disguised, sharedMask);

    nlohmann::json spoiler = { { "fileType", "ComboShipRandomizer" },
                               { "seed", seed },
                               { "masterSeed", fill.masterSeed },
                               { "goal", GoalJson(goal) },
                               { "startingGame", fill.mmStart ? "MM" : "OOT" },
                               { "sharedItems", sharedItems },
                               { "foreign", foreign },
                               { "hints", ComboRando::Generate(fill.masterSeed, fill.ootDump, fill.ootHintDump,
                                                               fill.mmDump, foreign, fill.result.spoilerJson,
                                                               fill.requiredness) } };
    const std::string dumps[GameCount] = { fill.ootDump, fill.mmDump };
    for (int game = 0; game < GameCount; game++) {
        nlohmann::json block = nlohmann::json::parse(sGames[game]->Request("DumpSettings"), nullptr, false);
        if (!block.is_object()) {
            block = nlohmann::json::object();
        }
        block["placements"] = std::move(placements[game]);
        block["prices"] = nlohmann::json::parse(dumps[game]).value("prices", nlohmann::json::object());
        spoiler[kGameKeys[game]] = std::move(block);
    }
    if (ComboRando::CrossDungeon::Coordinator::Enabled(fill.crossDungeonSeed)) {
        spoiler["crossDungeonEntrances"] = fill.crossDungeonSeed;
    }
    spoiler["entrances"] = { { "oot", spoiler["oot"].value("entrances", nlohmann::json::array()) } };
    spoiler["oot"].erase("entrances");
    return spoiler;
}

// The placements a playthrough walks: the spoiler's own plus every confined one the dumps fix in place, since a
// saved file lists only shuffled checks while logic still gates on the others (vanilla shops, unshuffled remains).
nlohmann::json PlaythroughPlacements(const nlohmann::json& spoiler, const std::string dumps[GameCount]) {
    nlohmann::json flat = { { "foreign", spoiler.value("foreign", nlohmann::json::array()) } };
    for (int game = 0; game < GameCount; game++) {
        nlohmann::json placements =
            spoiler.value(kGameKeys[game], nlohmann::json::object()).value("placements", nlohmann::json::object());
        const nlohmann::json fixed = nlohmann::json::parse(dumps[game]).value("fixed", nlohmann::json::array());
        for (const nlohmann::json& entry : fixed) {
            const std::string check = entry.value("check", "");
            if (!check.empty() && !placements.contains(check)) {
                placements[check] = entry.value("item", "");
            }
        }
        flat[kGameKeys[game]] = std::move(placements);
    }
    return flat;
}

// Rebuilds both games' logic for a finished spoiler, with its own prices and cross layout, and walks it.
int ValidateSpoiler(Options& options) {
    const nlohmann::json spoiler = nlohmann::json::parse(std::ifstream(options.playthroughPath), nullptr, false);
    if (!spoiler.is_object() || !RestoreSettingsFrom(options.playthroughPath, options, false)) {
        std::cerr << "[fleet-seedgen] could not read " << options.playthroughPath << "\n";
        return 2;
    }
    const uint32_t masterSeed = spoiler.value("masterSeed", 0u);
    const bool mmStart = spoiler.value("startingGame", std::string("OOT")) == "MM";
    const std::string seed = nlohmann::json{ { "seed", masterSeed } }.dump();
    for (const auto& game : sGames) {
        game->Request("SetSeed", seed);
    }
    SendComboOptions(options, mmStart);
    const std::string dumps[GameCount] = { Oracle<Oot>::DumpStaticData(), Oracle<Mm>::DumpStaticData() };
    for (int game = 0; game < GameCount; game++) {
        const nlohmann::json prices =
            spoiler.value(kGameKeys[game], nlohmann::json::object()).value("prices", nlohmann::json::object());
        sGames[game]->Request("SetCheckPrices", prices.dump());
    }
    ComboRando::OracleFns ootOracle = kOotOracle;
    ComboRando::OracleFns mmOracle = kMmOracle;
    const nlohmann::json crossSeed = ComboRando::CrossDungeon::Coordinator::SeedOf(spoiler);
    if (options.crossPools) {
        sCrossDungeons.SetOptions(*options.crossPools);
    }
    ComboRando::CrossDungeon::ValidateSavedStructure(sCrossDungeons, crossSeed, dumps[Oot], dumps[Mm], ootOracle,
                                                     mmOracle, mmStart);

    const bool portalGated = ComboRando::OotAccessFromDump(dumps[Oot]) != ComboRando::OotAccess::NO_LOGIC;
    const ComboRando::PlaythroughResult result = ComboRando::RunPlaythrough(
        PlaythroughPlacements(spoiler, dumps).dump(), ootOracle, mmOracle, std::to_string(masterSeed), nullptr,
        nullptr, dumps[Oot], dumps[Mm], portalGated, false, options.goal, mmStart, options.sharedMask);
    sGames[Mm]->Request("Finish");
    std::cout << "[fleet-seedgen] " << options.playthroughPath << (result.beatable ? " BEATABLE" : " NOT BEATABLE")
              << ": OoT " << result.reachableOot << " reachable / " << result.unreachableOot << " not, MM "
              << result.reachableMm << " / " << result.unreachableMm << "\n";
    return result.beatable ? 0 : 1;
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
                     "                     [--start oot|mm|random] [--shared all|bows,wallets,...]\n"
                     "                     [--cross dungeons,expanded,interiors,shops,doorInteriors,grottos]\n"
                     "                     [--dumps <folder>]\n"
                     "       fleet-seedgen --oot <Ship folder> --mm <2Ship folder> --playthrough spoiler.json\n";
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
    if (!options.playthroughPath.empty()) {
        return ValidateSpoiler(options);
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
    int status = 1;
    try {
        status = GenerateSeed(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "[fleet-seedgen] generation aborted: " << e.what() << "\n";
    }
    std::cout.flush();
    std::cerr.flush();
    // The games have no headless shutdown yet: a normal exit would run their static destructors after
    // their resource workers were killed mid-task, so the process ends without tearing anything down.
#ifdef _WIN32
    TerminateProcess(GetCurrentProcess(), static_cast<UINT>(status));
#endif
    std::_Exit(status);
}
