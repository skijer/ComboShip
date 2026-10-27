#include "rando/CrossDungeonCoordinator.h"
#include "rando/CrossWorldRando.h"
#include "rando/CrossForeign.h"
#include "rando/OotDungeonLayouts.h"
#include <atomic>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <map>
#include <stdexcept>

using Json = nlohmann::json;
using namespace ComboRando::CrossDungeon;
static Coordinator g_CrossDungeons;
static std::atomic<bool> g_GenerateBusy{ false }, g_ComboPendingFinalize{ false };
static std::string g_ConsolidatedJson, g_PendingMMSettingsJson, g_UserMMSettingsSnapshot;
static bool g_ComboReloadRestoreUserMM = false;
static uint32_t g_FinalizeDisplaySeed = 0;
static ComboRando::ComboGenProgress g_ComboProgress;
static Json native = Json::object(), installed[2];
static std::map<std::string, Json> files;
static std::string remembered;
static bool rejectMm = false;
static int pureCatalogueCalls = 0;

static std::filesystem::path ResolveComboSeedPath(const char* path) {
    return path;
}
static std::filesystem::path ResolveComboSeedPath(const std::string& path) {
    return path;
}
static bool TryLoadComboSeedFile(const std::filesystem::path& path, Json& out) {
    auto it = files.find(path.string());
    if (it == files.end())
        return false;
    out = it->second;
    return true;
}
static std::filesystem::path FindNewestComboSeed(Json&) {
    return {};
}
static void RememberComboSpoiler(const std::filesystem::path& path) {
    remembered = path.string();
}
static std::filesystem::path WriteComboSpoiler(const Json&, const std::string&) {
    return "written";
}
static const char* StubSOH_GetComboSpoilerPath() {
    return remembered.c_str();
}
static void ResetCrossItemDedupForSeed(uint32_t seed) {
    native["dedup"] = seed;
}
static void StubSOH_SetComboGoal(int hunt, int required, int total) {
    native["ootGoal"] = { hunt, required, total };
}
static void StubMM_SetComboGoal(int hunt, int required, int total) {
    native["mmGoal"] = { hunt, required, total };
}
static void StubSOH_SetComboStartingGame(int mm) {
    native["start"] = mm;
}
static void StubSOH_SetComboSharedItems(uint32_t mask) {
    native["shared"] = mask;
}
static void StubSOH_SetCheckPrices(const char* raw) {
    native["ootPrices"] = Json::parse(raw);
}
static void StubMM_SetCheckPrices(const char* raw) {
    native["mmPrices"] = Json::parse(raw);
}
static const char* DumpSettings(const char* game) {
    static std::string raw;
    raw = native.at(game).dump();
    return raw.c_str();
}
static const char* StubSOH_DumpRandoSettings() {
    return DumpSettings("ootSettings");
}
static const char* StubMM_DumpRandoSettings() {
    return DumpSettings("mmSettings");
}
static void StubSOH_RestoreRandoSettings(const char* raw) {
    native["ootSettings"] = Json::parse(raw);
}
static void StubMM_RestoreRandoSettings(const char* raw) {
    native["mmSettings"] = Json::parse(raw);
}
static void StubSOH_SetComboRandoSeed(uint64_t seed) {
    native["ootSeed"] = seed;
}
static void StubMM_SetComboRandoSeed(uint64_t seed) {
    native["mmSeed"] = seed;
}
static int StubSOH_ValidateDungeonLayoutsForCombo(const char* raw) {
    return !raw || std::string(raw).find("unknown") == std::string::npos;
}
static int StubSOH_SetDungeonLayoutsForCombo(const char* raw) {
    if (!StubSOH_ValidateDungeonLayoutsForCombo(raw))
        return 0;
    native["layouts"] = raw ? Json::parse(raw) : Json();
    return 1;
}
static void StubSOH_PrepRandoContext() {
    native["context"] = { native["ootSettings"], native["ootGoal"], native["start"], native["shared"],
                          native["ootSeed"] };
}
static int StubSOH_ApplyEntranceOverridesForCombo(const char* raw) {
    native["entrances"] = Json::parse(raw);
    return 1;
}
static int StubSOH_ShuffleEntrancesForCombo(uint64_t seed) {
    native["entrances"] = Json::array({ { { "index", 2 }, { "override", static_cast<int>(seed) } } });
    return 1;
}
static void StubSOH_SetComboHintsPresent(int on) {
    native["hintsPresent"] = on;
}
static void StubSOH_ApplyRandoPlacements(const char* raw) {
    native["placements"] = Json::parse(raw);
}
static void StubSOH_SetComboSeedHash(uint32_t hash) {
    native["hash"] = hash;
}
static void StubSOH_ApplyComboHints(const char* raw) {
    native["hints"] = Json::parse(raw);
}
static void Combo_FireGenRollHooksOnce(uint64_t seed) {
    native["hooks"] = seed;
}

static std::vector<Dungeon> CatalogueFor(const Json& seed) {
    int destination = seed.at("entrances").at("oot").at(0).at("override").get<int>();
    return { { "oot:a", { 0, 1 }, { 0, 2 }, { 0, destination } }, { "mm:b", { 1, 4 }, { 1, 5 }, { 1, 6 } } };
}
static const char* PureOotCatalogue(const char* raw) {
    static std::string out;
    ++pureCatalogueCalls;
    out = Json::array({ DungeonJson(CatalogueFor(Json::parse(raw))[0]) }).dump();
    return out.c_str();
}
static const char* OotCatalogue() {
    static std::string out;
    const int destination = native.at("entrances").at(0).at("override").get<int>();
    out = Json::array({ DungeonJson({ "oot:a", { 0, 1 }, { 0, 2 }, { 0, destination } }) }).dump();
    return out.c_str();
}
static const char* MmCatalogue() {
    static std::string out = Json::array({ DungeonJson({ "mm:b", { 1, 4 }, { 1, 5 }, { 1, 6 } }) }).dump();
    return out.c_str();
}
static int Install(int game, const char* raw) {
    auto seed = Json::parse(raw);
    Decode(seed);
    if (game == 1 && rejectMm && !seed.is_null())
        return 0;
    installed[game] = seed;
    return 1;
}
static void Access(const char*) {
}
static const char* Exits() {
    return "[]";
}

static auto SOH_GetComboSpoilerPath = StubSOH_GetComboSpoilerPath;
static auto SOH_SetComboGoal = StubSOH_SetComboGoal;
static auto MM_SetComboGoal = StubMM_SetComboGoal;
static auto SOH_SetComboStartingGame = StubSOH_SetComboStartingGame;
static auto SOH_SetComboSharedItems = StubSOH_SetComboSharedItems;
static auto SOH_SetCheckPrices = StubSOH_SetCheckPrices;
static auto MM_SetCheckPrices = StubMM_SetCheckPrices;
static auto SOH_DumpRandoSettings = StubSOH_DumpRandoSettings;
static auto MM_DumpRandoSettings = StubMM_DumpRandoSettings;
static auto SOH_RestoreRandoSettings = StubSOH_RestoreRandoSettings;
static auto MM_RestoreRandoSettings = StubMM_RestoreRandoSettings;
static auto SOH_SetComboRandoSeed = StubSOH_SetComboRandoSeed;
static auto MM_SetComboRandoSeed = StubMM_SetComboRandoSeed;
static auto SOH_SetDungeonLayoutsForCombo = StubSOH_SetDungeonLayoutsForCombo;
[[maybe_unused]] static auto SOH_ValidateDungeonLayoutsForCombo = StubSOH_ValidateDungeonLayoutsForCombo;
static auto SOH_PrepRandoContext = StubSOH_PrepRandoContext;
static auto SOH_ApplyEntranceOverridesForCombo = StubSOH_ApplyEntranceOverridesForCombo;
static auto SOH_ShuffleEntrancesForCombo = StubSOH_ShuffleEntrancesForCombo;
static auto SOH_SetComboHintsPresent = StubSOH_SetComboHintsPresent;
static auto SOH_ApplyRandoPlacements = StubSOH_ApplyRandoPlacements;
static auto SOH_SetComboSeedHash = StubSOH_SetComboSeedHash;
static auto SOH_ApplyComboHints = StubSOH_ApplyComboHints;

// Extracted from ComboShip.cpp at test execution; the transaction ordering is production code.
#include "combo_reload_function.h"

static void Check(bool ok, const char* message) {
    if (!ok)
        throw std::runtime_error(message);
}
static Json Seed(int number) {
    Json seed = {
        { "fileType", "ComboShipRandomizer" },
        { "seed", std::to_string(number) },
        { "masterSeed", number },
        { "displaySeed", number + 100 },
        { "startingGame", number % 2 ? "OOT" : "MM" },
        { "goal",
          { { "type", number % 2 ? "bosses" : "triforceHunt" }, { "requiredPieces", 1 }, { "totalPieces", 2 } } },
        { "sharedItems", number % 2 ? Json::array() : Json::array({ "bows" }) },
        { "oot",
          { { "settings", { { "setting", number } } },
            { "placements", Json::object() },
            { "prices", { { "shop", number } } } } },
        { "mm",
          { { "settings", { { "setting", number + 10 } } },
            { "placements", Json::object() },
            { "prices", { { "shop", number + 10 } } } } },
        { "entrances", { { "oot", Json::array({ { { "index", 2 }, { "override", number + 10 } } }) } } }
    };
    seed["crossDungeonEntrances"] = GenerateSeed(CatalogueFor(seed), number);
    return seed;
}
int main() {
    native["ootSettings"] = { { "original", 77 } };
    native["mmSettings"] = { { "original", 88 } };
    g_CrossDungeons.hosts[0] = { OotCatalogue, [](const char* raw) { return Install(0, raw); }, Access, Exits,
                                 nullptr };
    g_CrossDungeons.hosts[1] = { MmCatalogue, [](const char* raw) { return Install(1, raw); }, Access, Exits, nullptr };
    // The OOT preflight exporter computes from the file, while its active native world remains A.
    g_CrossDungeons.hosts[0].DumpSeedCatalogue = PureOotCatalogue;
    files["A"] = Seed(1);
    files["B"] = Seed(2);
    files["B"]["crossDungeonEntrances"]["dungeons"][0]["returnTo"]["index"] = 99; // Structurally valid, stale.
    Check(Combo_OnReloadRequest("A") == 1, "valid A must load");
    const auto acceptedNative = native;
    const auto acceptedJson = g_ConsolidatedJson;
    const auto acceptedMm = g_PendingMMSettingsJson;
    const auto acceptedDisplay = g_FinalizeDisplaySeed;
    Check(Combo_OnReloadRequest("B") == 0, "stale explicit B must reject");
    Check(native == acceptedNative && g_ConsolidatedJson == acceptedJson && g_PendingMMSettingsJson == acceptedMm &&
              g_FinalizeDisplaySeed == acceptedDisplay,
          "failed explicit drop must preserve A's settings, goals, seed, prices, graph, placements and pending state");
    remembered = "B";
    Check(Combo_OnReloadRequest(nullptr) == 0 && native == acceptedNative && g_ConsolidatedJson == acceptedJson,
          "failed silent reload must preserve the entire accepted native world");
    files["B"] = Seed(2);
    rejectMm = true;
    Check(Combo_OnReloadRequest("B") == 0 && native == acceptedNative && g_ConsolidatedJson == acceptedJson,
          "native route-setter rejection must happen before native seed state changes");
    Check(installed[0].is_null() && installed[1].is_null(), "failed preflight must restore both oracle hosts");
    rejectMm = false;
    for (bool cross : { false, true }) {
        files["badLayouts"] = Seed(2);
        if (!cross)
            files["badLayouts"].erase("crossDungeonEntrances");
        files["badLayouts"]["oot"]["masterQuestDungeons"] = Json::array({ "unknown" });
        Check(Combo_OnReloadRequest("badLayouts") == 0 && native == acceptedNative &&
                  g_ConsolidatedJson == acceptedJson && g_PendingMMSettingsJson == acceptedMm &&
                  g_FinalizeDisplaySeed == acceptedDisplay,
              "layout rejection must preserve accepted seed A before changing any native or pending state");
    }
    Check(Combo_OnReloadRequest("B") == 1 && g_ConsolidatedJson != acceptedJson, "valid B must commit after preflight");
    auto legacy = Seed(3);
    legacy.erase("crossDungeonEntrances");
    files["legacy"] = legacy;
    const int pureCalls = pureCatalogueCalls;
    Check(Combo_OnReloadRequest("legacy") == 1 && pureCatalogueCalls == pureCalls,
          "disabled legacy reload must preserve its path without querying cross-dungeon catalogues");
    std::cout << "cross-dungeon production reload preflight: PASS\n";
}
