#include "2s2h/Rando/Rando.h"
#include "NeiGracePolicy.h"
#include "NeiSeasonsPolicy.h"
#include "combo/rando/MagicMirrorShuffle.h"
#include "tests/test_require.h"
#include <cstdio>
#include <cstring>
#include <cctype>
#include <algorithm>
#include <array>
#include <memory>
#include <sstream>
// Only the native settings key enum is shared with this MM-side fixture.
#define RANDO_ENUM_BEGIN(name) enum name {
#define RANDO_ENUM_ITEM(name) name,
#define RANDO_ENUM_END(name) };
#include "soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerSettingKey.h"
#undef RANDO_ENUM_BEGIN
#undef RANDO_ENUM_ITEM
#undef RANDO_ENUM_END
#define SPDLOG_WARN(...) ((void)0)

#define COMBO_EXPORT
#define CVAR_RANDOMIZER_SETTING(s) "gRandoSettings." s
static nlohmann::json config = nlohmann::json::object();
static int appliedMode, appliedRewards, creationResult, g_MmSaveInMemorySlot = -1;

extern "C" {
int32_t CVarGetInteger(const char* key, int32_t fallback) { return config.value(key, fallback); }
void CVarSetInteger(const char* key, int32_t value) { config[key] = value; }
void CVarSetString(const char* key, const char* value) { config[key] = value; }
void CVarClear(const char* key) { config.erase(key); }
const char* CVarGetString(const char* key, const char* fallback) {
    return config.contains(key) && config[key].is_string()
               ? config[key].get_ref<const std::string&>().c_str() : fallback;
}
}
namespace Rando {
// Native option metadata and shared CVar storage are the input/service seam.
// GetInstance/GetAllOptions/GetCVarName below are copied from production.
struct Option {
    std::string cvarName;
    const std::string& GetCVarName() const;
};
struct Settings {
    static std::shared_ptr<Settings> mInstance;
    std::array<Option, RSK_MAX> mOptions{};
    static std::shared_ptr<Settings> GetInstance();
    const std::array<Option, RSK_MAX>& GetAllOptions() const;
};
std::shared_ptr<Settings> Settings::mInstance;
void SetExcludedChecksInConfig(std::vector<RandoCheckId>&) {}
void SetStartingItemsInConfig(std::vector<RandoItemId>&) {}
namespace StaticData {
RandoCheckId GetCheckIdFromName(const char*) { return RC_UNKNOWN; }
RandoItemId GetItemIdFromName(const char*) { return RI_UNKNOWN; }
}
}

namespace ComboRando {
enum { GAME_MM = 1 };
// Placement translation is unrelated to settings; the engine boundary receives
// this seed's own MM placement object, unchanged by the fixture.
nlohmann::json ApplyPayloadFromConsolidated(const nlohmann::json& seed, int) { return seed["mm"]["placements"]; }
}

static const char* MM_DumpRandoSettings() {
    static std::string snapshot; snapshot = config.dump(); return snapshot.c_str();
}
static void MM_SetCheckPrices(const char*) {}
static void SOH_GetCurrentPlayerName(unsigned char* name) { std::memset(name, 0x3E, 8); }
static int MM_InitRandoSaveFile(int, const char*, const unsigned char*) {
    appliedMode = CVarGetInteger("gRando.Options.RO_HYLIAS_GRACE", NEI_GRACE_OFF);
    appliedRewards = CVarGetInteger("gRando.Options.RO_HYLIAS_GRACE_REWARDS", 4);
    return creationResult;
}
// Runtime seed binding is a launcher service; the dedicated cross-entrance
// fixtures execute its production host binding and rollback paths.
static bool runtimeBindingAccepted = true;
static bool Combo_BindCrossDungeonRuntime(const nlohmann::json&) {
    return runtimeBindingAccepted;
}

#include "settings_production.inc"

static void SeedApplication() {
    config["gRando.Options.RO_HYLIAS_GRACE"] = NEI_GRACE_ON;
    config["gRando.Options.RO_HYLIAS_GRACE_REWARDS"] = 1;
    nlohmann::json seed = {{"mm", {{"placements", nlohmann::json::object()},
        {"settings", {{"gRando.Options.RO_HYLIAS_GRACE", NEI_GRACE_GATED},
                      {"gRando.Options.RO_HYLIAS_GRACE_REWARDS", 7}}}}}};
    runtimeBindingAccepted = false;
    appliedMode = -99;
    REQUIRE(!Combo_WriteMMSaveForSlot(2, seed) && appliedMode == -99);
    REQUIRE(CVarGetInteger("gRando.Options.RO_HYLIAS_GRACE", -1) == NEI_GRACE_ON);
    runtimeBindingAccepted = true;
    REQUIRE(Combo_WriteMMSaveForSlot(2, seed));
    REQUIRE(appliedMode == NEI_GRACE_GATED && appliedRewards == 7 && g_MmSaveInMemorySlot == 2);
    REQUIRE(CVarGetInteger("gRando.Options.RO_HYLIAS_GRACE", -1) == NEI_GRACE_ON);
    REQUIRE(CVarGetInteger("gRando.Options.RO_HYLIAS_GRACE_REWARDS", -1) == 1);
    creationResult = -1;
    REQUIRE(!Combo_WriteMMSaveForSlot(1, seed));
    REQUIRE(appliedMode == NEI_GRACE_GATED && appliedRewards == 7);
    REQUIRE(CVarGetInteger("gRando.Options.RO_HYLIAS_GRACE", -1) == NEI_GRACE_ON);
    creationResult = 0;
    seed["mm"]["settings"] = nlohmann::json::object(); // Legacy consolidated seed.
    config["gRando.Options.RO_HYLIAS_GRACE"] = NEI_GRACE_OFF;
    REQUIRE(Combo_WriteMMSaveForSlot(1, seed));
    REQUIRE(appliedMode == NEI_GRACE_ON && appliedRewards == 4);
    REQUIRE(CVarGetInteger("gRando.Options.RO_HYLIAS_GRACE", -1) == NEI_GRACE_OFF);
    puts("PASS fresh/repaired MM save uses seed policy after UI edits; UI restores on success/failure, legacy seeds remain On");
}

int main(int argc, char** argv) {
    auto settings = Rando::Settings::GetInstance();
    settings->mOptions[RSK_NO_LOGIC].cvarName = "gRandoSettings.NoLogic";
    settings->mOptions[RSK_ROD_OF_SEASONS].cvarName = "gRandoSettings.RodOfSeasons";
    settings->mOptions[RSK_HYLIAS_GRACE].cvarName = "gRandoSettings.HyliasGrace";
    config["gRandoSettings.NoLogic"] = 7;
    config["gUnrelatedSetting"] = 9;
    SOH_RestoreRandoSettings("{}");
    REQUIRE(!config.contains("gRandoSettings.NoLogic"));
    REQUIRE(config["gUnrelatedSetting"] == 9);
    SOH_RestoreRandoSettings("{\"gRandoSettings.LogicRules\":2,\"gRandoSettings.ExcludedLocations\":\"42\"}");
    REQUIRE(config["gRandoSettings.NoLogic"] == 2);
    REQUIRE(config["gRandoSettings.ExcludedLocations"] == "");
    SOH_RestoreRandoSettings("{\"gRandoSettings.ExcludedLocations\":\"Kokiri Sword Chest\"}");
    REQUIRE(config["gRandoSettings.ExcludedLocations"] == "Kokiri Sword Chest");
    if (argc > 1 && !std::strcmp(argv[1], "--apply-later")) { SeedApplication(); return 0; }
    for (int mode = NEI_GRACE_ON; mode <= NEI_GRACE_GATED; ++mode) {
        config["gCombo.Sync.Enabled"] = 0;
        config["gRando.Options.RO_HYLIAS_GRACE"] = mode;
        config["gRando.Options.RO_HYLIAS_GRACE_REWARDS"] = 8;
        config["gRandoSettings.HyliasGrace"] = (mode + 1) % 3;
        config["gRandoSettings.HyliasGraceRewards"] = 1;
        SOH_NormalizeComboGraceFromMM();
        REQUIRE(CVarGetInteger("gRandoSettings.HyliasGrace", -1) == mode);
        REQUIRE(CVarGetInteger("gRandoSettings.HyliasGraceRewards", -1) == 8);
        REQUIRE(CVarGetInteger("gCombo.Sync.Enabled", -1) == 0);
    }
    config["gRando.Options.RO_HYLIAS_GRACE"] = 99;
    config["gRando.Options.RO_HYLIAS_GRACE_REWARDS"] = 100;
    SOH_NormalizeComboGraceFromMM();
    REQUIRE(CVarGetInteger("gRandoSettings.HyliasGrace", -1) == NEI_GRACE_OFF);
    REQUIRE(CVarGetInteger("gRandoSettings.HyliasGraceRewards", -1) == 13);
    MM_RestoreRandoSettings("{}"); SOH_RestoreRandoSettings("{}");
    REQUIRE(CVarGetInteger("gRando.Options.RO_HYLIAS_GRACE", -1) == NEI_GRACE_ON);
    REQUIRE(CVarGetInteger("gRandoSettings.HyliasGrace", -1) == NEI_GRACE_ON);
    puts("PASS native combined generation normalization with sync Off, validation and legacy replay policy");
    for (int mode = NEI_SEASONS_INDIVIDUAL; mode <= NEI_SEASONS_GATED; ++mode) {
        config["gRando.Options.RO_ROD_OF_SEASONS"] = mode;
        config["gRando.Options.RO_STARTING_ROD_OF_SEASONS"] = 1;
        config["gRandoSettings.RodOfSeasons"] = 99;
        config["gRandoSettings.StartingRodOfSeasons"] = 0;
        SOH_NormalizeComboGraceFromMM();
        REQUIRE(CVarGetInteger("gRandoSettings.RodOfSeasons", -1) == mode);
        REQUIRE(CVarGetInteger("gRandoSettings.StartingRodOfSeasons", -1) == 1);
    }
    MM_RestoreRandoSettings("{}"); SOH_RestoreRandoSettings("{}");
    REQUIRE(CVarGetInteger("gRando.Options.RO_ROD_OF_SEASONS", -1) == NEI_SEASONS_INDIVIDUAL);
    REQUIRE(CVarGetInteger("gRando.Options.RO_STARTING_ROD_OF_SEASONS", -1) == 0);
    REQUIRE(CVarGetInteger("gRandoSettings.RodOfSeasons", -1) == NEI_SEASONS_INDIVIDUAL);
    REQUIRE(CVarGetInteger("gRandoSettings.StartingRodOfSeasons", -1) == 0);
    SeedApplication();
}
