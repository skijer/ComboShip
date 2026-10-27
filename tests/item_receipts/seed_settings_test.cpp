// Native settings/storage probe. run_seed_settings_tests.py supplies verbatim
// production boundaries. This does not run the fill worker or either game loop.
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <future>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include "z64.h"
#include "soh/SohGui/UIWidgets.hpp"
#include "soh/cvar_prefixes.h"
#include "soh/Enhancements/randomizer/entrance.h"
#include "soh/Enhancements/randomizer/logic.h"
#include "soh/Enhancements/randomizer/dungeon.h"
#include "soh/Enhancements/randomizer/trial.h"
#include "soh/Enhancements/randomizer/fishsanity.h"
#include "soh/Enhancements/randomizer/rng.h"
#include "soh/Enhancements/randomizer/randomizerEnumStrings.h"
#include "soh/Enhancements/Lang/Lang.h"
#include "rando/SharedItems.h"
#include "rando/OotDungeonLayouts.h"
#include "ComboExport.h"
#include "ComboGameMode.h"
#include "NeiGracePolicy.h"
#include "NeiSeasonsPolicy.h"
// Expose only native registration/section state to this synchronous fixture.
#define private public
#include "soh/Enhancements/randomizer/settings.h"
#include "soh/Enhancements/randomizer/static_data.h"
#include "soh/SaveManager.h"
#undef private

using json = nlohmann::json;
// Language loading belongs to the GUI; retain stable references for the native
// metadata dispatch while testing the authored integration settings below.
namespace Lang {
const std::string& Translate(const char* path) {
    static std::map<std::string, std::string> values;
    return values.try_emplace(path, path).first->second;
}
std::variant<std::reference_wrapper<const std::string>, Lang::Error> TryTranslate(const char* path) {
    return std::cref(Translate(path));
}
}
extern "C" {
SaveContext gSaveContext{};
int gComboGameMode = COMBO_GAME_BOTH;
int gComboGoalHunt = 0, gComboGoalRequired = 0, gComboGoalPieces = 0;
int gComboStartingGameMM = 1, gComboSharedMask = 0;
}

// CVar storage stands in for the shared console service; native Option reads
// and writes it through its real configured name/default.
static std::map<std::string, int> integerCVars;
static std::map<std::string, std::string> stringCVars;
extern "C" int32_t CVarGetInteger(const char* key, int32_t fallback) {
    const auto found = integerCVars.find(key);
    return found == integerCVars.end() ? fallback : found->second;
}
extern "C" void CVarSetInteger(const char* key, int32_t value) { integerCVars[key] = value; }
extern "C" const char* CVarGetString(const char* key, const char* fallback) {
    const auto found = stringCVars.find(key);
    return found == stringCVars.end() ? fallback : found->second.c_str();
}
extern "C" void CVarSetString(const char* key, const char* value) { stringCVars[key] = value; }
extern "C" void CVarClear(const char* key) {
    integerCVars.erase(key);
    stringCVars.erase(key);
}

// Pack availability is a fixture input to the complete native finalizer.
struct OTRGlobals {
    static OTRGlobals* Instance;
    std::shared_ptr<Rando::Context> gRandoContext;
    bool original = true, masterQuest = true;
    bool HasOriginal() const { return original; }
    bool HasMasterQuest() const { return masterQuest; }
};
static OTRGlobals globals;
OTRGlobals* OTRGlobals::Instance = &globals;
#ifdef COMBO_MQ_LAYOUT_TESTS
extern "C" int Combo_SOH_GenerationQuestAssets(void);
#else
extern "C" int Combo_SOH_GenerationQuestAssets(void) {
    return (globals.original ? 1 : 0) | (globals.masterQuest ? 2 : 0);
}
#endif

namespace Rando {
// The graph pool is a fixture boundary; every edge below is a native Entrance.
// Generation and JSON dumping are the complete production function bodies.
static std::vector<Entrance*> receiptEntrancePool;
std::vector<Entrance*> GetShuffleableEntrances(EntranceType, bool) { return receiptEntrancePool; }
std::weak_ptr<Context> Context::mContext;
std::shared_ptr<Settings> Settings::mInstance;
std::array<Location, RC_MAX> StaticData::locationTable;
Location* StaticData::GetLocation(RandomizerCheck rc) { return &locationTable[rc]; }
std::unordered_map<uint32_t, RandomizerHintTextKey> StaticData::trialData = {
    { TK_LIGHT_TRIAL, RHT_LIGHT_TRIAL }, { TK_FOREST_TRIAL, RHT_FOREST_TRIAL }, { TK_FIRE_TRIAL, RHT_FIRE_TRIAL },
    { TK_WATER_TRIAL, RHT_WATER_TRIAL }, { TK_SHADOW_TRIAL, RHT_SHADOW_TRIAL }, { TK_SPIRIT_TRIAL, RHT_SPIRIT_TRIAL },
};
std::unordered_map<std::string, RandomizerSettingKey> StaticData::optionNameToEnum;
std::unordered_map<std::string, RandomizerCheck> StaticData::locationNameToEnum;
std::array<std::pair<RandomizerCheck, RandomizerCheck>, 17> StaticData::randomizerFishingPondFish{};
}

SaveManager* SaveManager::Instance = nullptr;
SaveManager::SaveManager() {}
#ifdef COMBO_MQ_LAYOUT_TESTS
// Graph/inventory construction is outside this layout-state boundary. Keep the
// actual Context, Settings, Dungeons, RNG, Combo prep/oracle and save code.
void RegionTable_Init() {}
void GenerateStartingInventory() {}
static void Combo_ApplyEnabledTricks() {}
static std::set<RandomizerCheck> Combo_ParseExcludedLocations() { return {}; }
void Rando::Logic::Reset(bool) {}
void Rando::Context::GenerateLocationPool() {}
#endif
#include "seed_settings_production.inc"

static void Check(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

static void ResetContext(std::shared_ptr<Rando::Context>& context) {
    context->GetLogic()->SetContext(nullptr);
    Rando::Settings::GetInstance()->ClearContext();
    context.reset();
    context = Rando::Context::CreateInstance();
    Rando::Settings::GetInstance()->AssignContext(context);
}

#ifdef COMBO_MQ_LAYOUT_TESTS
#include "../rom_setup/layout_cases.inc"
#endif

static void DumpReceiptEntranceFixtures(const std::shared_ptr<Rando::Context>& context, const char* filename) {
    using namespace Rando;
    globals.gRandoContext = context;
    context->GetOption(RSK_DECOUPLED_ENTRANCES).Set(0);
    context->GetOption(RSK_SHUFFLE_BOSS_ENTRANCES).Set(RO_BOSS_ROOM_ENTRANCE_SHUFFLE_FULL);
    context->GetEntranceShuffler()->SetNoRandomEntrances(false);
    Entrance forest(RR_FOREST_TEMPLE_BOSS_ROOM, [] { return true; }, "true");
    Entrance forestReverse(RR_FOREST_TEMPLE_BOSS_ENTRYWAY, [] { return true; }, "true");
    Entrance deku(RR_DEKU_TREE_BOSS_ROOM, [] { return true; }, "true");
    Entrance dekuReverse(RR_DEKU_TREE_BOSS_EXIT, [] { return true; }, "true");
    Entrance fire(RR_FIRE_TEMPLE_BOSS_ROOM, [] { return true; }, "true");
    Entrance fireReverse(RR_FIRE_TEMPLE_BOSS_ENTRYWAY, [] { return true; }, "true");
    Entrance lobby(RR_DEKU_TREE_ENTRYWAY, [] { return true; }, "true");
    Entrance lobbyReverse(RR_KF_OUTSIDE_DEKU_TREE, [] { return true; }, "true");
    Entrance forestLobby(RR_FOREST_TEMPLE_ENTRYWAY, [] { return true; }, "true");
    Entrance forestLobbyReverse(RR_SACRED_FOREST_MEADOW, [] { return true; }, "true");
    Entrance house(RR_KF_MIDOS_HOUSE, [] { return true; }, "true");
    Entrance houseReverse(RR_KOKIRI_FOREST, [] { return true; }, "true");
    auto setup = [](Entrance& forward, Entrance& reverse, int index, int reverseIndex, EntranceType type) {
        forward.SetIndex(index);
        reverse.SetIndex(reverseIndex);
        forward.BindTwoWay(&reverse);
        forward.SetType(type);
    };
    setup(forest, forestReverse, ENTR_FOREST_TEMPLE_BOSS_ENTRANCE, ENTR_FOREST_TEMPLE_BOSS_DOOR, EntranceType::AdultBoss);
    setup(deku, dekuReverse, ENTR_DEKU_TREE_BOSS_ENTRANCE, ENTR_DEKU_TREE_BOSS_DOOR, EntranceType::ChildBoss);
    setup(fire, fireReverse, ENTR_FIRE_TEMPLE_BOSS_ENTRANCE, ENTR_FIRE_TEMPLE_BOSS_DOOR, EntranceType::AdultBoss);
    setup(lobby, lobbyReverse, ENTR_DEKU_TREE_ENTRANCE, ENTR_KOKIRI_FOREST_OUTSIDE_DEKU_TREE, EntranceType::Dungeon);
    setup(forestLobby, forestLobbyReverse, ENTR_FOREST_TEMPLE_ENTRANCE,
          ENTR_SACRED_FOREST_MEADOW_OUTSIDE_TEMPLE, EntranceType::Dungeon);
    setup(house, houseReverse, ENTR_MIDOS_HOUSE_0, ENTR_KOKIRI_FOREST_OUTSIDE_MIDOS_HOUSE, EntranceType::Interior);
    forest.SetAsShuffled();
    deku.SetAsShuffled();
    receiptEntrancePool = { &forest, &deku };
    forest.SetReplacement(&lobby);
    deku.SetReplacement(&fire);
    auto dump = [&]() {
        context->GetEntranceShuffler()->CreateEntranceOverrides();
        return json::parse(SOH_DumpEntranceOverrides());
    };
    json fixtures;
    fixtures["nested"] = dump();
    Check(fixtures["nested"][0]["index"] == ENTR_FOREST_TEMPLE_BOSS_ENTRANCE &&
              fixtures["nested"][0]["override"] == ENTR_DEKU_TREE_ENTRANCE &&
              fixtures["nested"][0]["destination"] == ENTR_FOREST_TEMPLE_BOSS_DOOR &&
              fixtures["nested"][0]["overrideDestination"] == ENTR_KOKIRI_FOREST_OUTSIDE_DEKU_TREE,
          "native generated table preserves forward and reverse directions");
    forest.SetReplacement(&fire);
    deku.SetReplacement(&deku);
    fixtures["direct"] = dump();
    forest.SetReplacement(&lobby);
    deku.SetReplacement(&forestLobby);
    fixtures["cycle"] = dump();
    deku.SetReplacement(&house);
    fixtures["deadEnd"] = dump();
    std::ofstream(filename) << fixtures.dump(2);
    receiptEntrancePool.clear();
    globals.gRandoContext.reset();
    std::puts("PASS native Entrance construction, CreateEntranceOverrides and consolidated JSON dump for compass routes");
}

int main(int argc, char** argv) {
#ifdef COMBO_MQ_LAYOUT_TESTS
    return RunMqLayoutTests();
#endif
    SaveManager storage;
    SaveManager::Instance = &storage;
    auto settings = Rando::Settings::GetInstance();
    RegisterInformationOption(*settings);
    RegisterSeasonsOptions(*settings);
    auto context = Rando::Context::CreateInstance();
    settings->AssignContext(context);
    auto& option = settings->GetOption(RSK_MAPS_COMPASSES_GIVE_INFORMATION);
    Check(option.GetCVarName() == "gRandoSettings.MapsCompassesGiveInformation", "production CVar prefix/name");
    Check(option.GetMenuOptionDefault() == RO_GENERIC_OFF && option.GetOptionIndex() == RO_GENERIC_OFF,
          "native checkbox default is Off");
    Check(settings->PopulateOptionNameToEnum().at("Maps and Compasses Give Information") ==
              RSK_MAPS_COMPASSES_GIVE_INFORMATION, "native option-name translation includes appended option");
    static_assert(RSK_HYLIAS_GRACE == RSK_MAPS_COMPASSES_GIVE_INFORMATION + 1);
    static_assert(RSK_HYLIAS_GRACE_REWARDS + 1 == RSK_ROD_OF_SEASONS);
    static_assert(RSK_STARTING_ROD_OF_SEASONS + 1 == RSK_SHUFFLE_MAGIC_MIRROR);
    static_assert(RSK_SHUFFLE_MAGIC_MIRROR + 1 == RSK_MAX);

    auto& seasons = settings->GetOption(RSK_ROD_OF_SEASONS);
    auto& startingRod = settings->GetOption(RSK_STARTING_ROD_OF_SEASONS);
    Check(seasons.GetMenuOptionDefault() == NEI_SEASONS_INDIVIDUAL &&
              startingRod.GetMenuOptionDefault() == 0, "native Rod options preserve legacy defaults");
    for (int mode : {NEI_SEASONS_INDIVIDUAL, NEI_SEASONS_ROD, NEI_SEASONS_GATED}) {
        for (int start : {0, 1}) {
            CVarSetInteger(seasons.GetCVarName().c_str(), mode);
            CVarSetInteger(startingRod.GetCVarName().c_str(), start);
            settings->SetAllToContext();
            context->FinalizeSettings({}, {});
            gSaveContext.ship.quest.id = QUEST_RANDOMIZER;
            Check(Seasons_RandoMode() == mode &&
                      context->GetOption(RSK_STARTING_ROD_OF_SEASONS).Get() == start,
                  "native options survive MM-first finalizer and feed saved gameplay mode");
            json section = json::object();
            storage.currentJsonContext = &section;
            SaveNativeSettings();
            CVarSetInteger(seasons.GetCVarName().c_str(), (mode + 1) % 3);
            CVarSetInteger(startingRod.GetCVarName().c_str(), !start);
            ResetContext(context);
            storage.currentJsonContext = &section;
            LoadNativeSettings();
            Check(Seasons_RandoMode() == mode &&
                      context->GetOption(RSK_STARTING_ROD_OF_SEASONS).Get() == start,
                  "native save reload controls Rod mode independently of menu");
            ctx = context.get();
            for (int master : {0, 1}) {
                for (int density : {RO_ITEM_POOL_BALANCED, RO_ITEM_POOL_PLENTIFUL,
                                    RO_ITEM_POOL_SCARCE, RO_ITEM_POOL_MINIMAL}) {
                    context->GetOption(RSK_SKIJER_CUSTOM_ITEMS).Set(master);
                    context->GetOption(RSK_ITEM_POOL).Set(density);
                    itemPool.clear(); plentifulPool.clear();
                    GenerateNativeNeiPool();
                    Check(std::count(itemPool.begin(), itemPool.end(), RG_ROD_OF_SEASONS) ==
                              (master && mode != NEI_SEASONS_INDIVIDUAL && !start),
                          "native source pool shuffles one Rod only when needed");
                    for (auto season : {RG_SEASON_SPRING, RG_SEASON_SUMMER, RG_SEASON_AUTUMN, RG_SEASON_WINTER}) {
                        Check(std::count(itemPool.begin(), itemPool.end(), season) ==
                                  (master && mode == NEI_SEASONS_INDIVIDUAL),
                              "native individual pool keeps four independent pickups");
                        Check(std::count(plentifulPool.begin(), plentifulPool.end(), season) ==
                                  (master && mode == NEI_SEASONS_INDIVIDUAL && density == RO_ITEM_POOL_PLENTIFUL),
                              "native plentiful duplicate policy remains unchanged");
                    }
                }
                GenerateNativeStartingRod();
                Check(std::count(StartingInventory.begin(), StartingInventory.end(), RG_ROD_OF_SEASONS) == start,
                      "native starting Rod is independent of the NEI pool switch");
            }
            ctx = nullptr;
        }
    }
    gSaveContext.ship.quest.id = QUEST_NORMAL;
    Check(Seasons_RandoMode() == NEI_SEASONS_INDIVIDUAL, "non-rando Rod keeps existing individual behavior");
    std::puts("PASS native Rod settings, saved mode, MM-first preparation, source pool densities and starting inventory");

    // Fresh generation follows the actual critical order: native options copy,
    // native finalizer (including the MM-start forces), then settings snapshot
    // and native save array. Exercise multiple seeds in one process.
    for (int enabled : {0, 1, 0, 1}) {
        CVarSetInteger(option.GetCVarName().c_str(), enabled);
        settings->SetAllToContext();
        context->FinalizeSettings({}, {});
        context->SetSeedGenerated(true);
        Check(context->GetOption(RSK_MAPS_COMPASSES_GIVE_INFORMATION).Get() == enabled,
              "generation CVar survived real SetAllToContext and MM-start FinalizeSettings");
        Check(context->GetOption(RSK_SELECTED_STARTING_AGE).Is(RO_AGE_CHILD), "MM start selected child age");
        const auto baked = json::parse(SOH_DumpRandoSettings());
        Check(baked.at(option.GetCVarName()).get<int>() == enabled, "consolidated snapshot contains seed choice");
        json section = json::object();
        storage.currentJsonContext = &section;
        SaveNativeSettings();
        Check(section.at("randoSettings").size() == RSK_MAX &&
                  section.at("randoSettings").at(RSK_MAPS_COMPASSES_GIVE_INFORMATION) == enabled,
              "native save array includes authoritative appended value");
        // Opening a file recreates Context and loads the saved section. The
        // current menu can disagree; receipt queries must see the saved value.
        CVarSetInteger(option.GetCVarName().c_str(), !enabled);
        ResetContext(context);
        section = json::parse(section.dump());
        storage.currentJsonContext = &section;
        LoadNativeSettings();
        Check(context->GetOption(RSK_MAPS_COMPASSES_GIVE_INFORMATION).Get() == enabled,
              "save reload restores seed value independently of live menu");
    }

    // Reload/drop restore feeds the same native generation prep. An old blob
    // without this newly appended key must not inherit a local checked box.
    for (const auto& snapshot : {json{{option.GetCVarName(), 1}}, json::object(),
                                 json{{option.GetCVarName(), 0}}}) {
        CVarSetInteger(option.GetCVarName().c_str(), 1);
        SOH_RestoreRandoSettings(snapshot.dump().c_str());
        settings->SetAllToContext();
        context->FinalizeSettings({}, {});
        Check(context->GetOption(RSK_MAPS_COMPASSES_GIVE_INFORMATION).Get() ==
                  snapshot.value(option.GetCVarName(), 0), "restored/old seed choice remains authoritative");
    }
    json oldSection{{"randoSettings", json::array()}};
    oldSection["randoSettings"] = std::vector<int>(RSK_MAPS_COMPASSES_GIVE_INFORMATION, 0);
    context->GetOption(RSK_MAPS_COMPASSES_GIVE_INFORMATION).Set(1);
    context->GetOption(RSK_HYLIAS_GRACE).Set(NEI_GRACE_GATED);
    context->GetOption(RSK_ROD_OF_SEASONS).Set(NEI_SEASONS_GATED);
    context->GetOption(RSK_STARTING_ROD_OF_SEASONS).Set(1);
    storage.currentJsonContext = &oldSection;
    LoadNativeSettings();
    Check(context->GetOption(RSK_MAPS_COMPASSES_GIVE_INFORMATION).Is(0), "old save array defaults missing option Off");
    Check(context->GetOption(RSK_HYLIAS_GRACE).Is(NEI_GRACE_ON), "old save array preserves ungated Grace");
    Check(context->GetOption(RSK_ROD_OF_SEASONS).Is(NEI_SEASONS_INDIVIDUAL) &&
              context->GetOption(RSK_STARTING_ROD_OF_SEASONS).Is(0),
          "old save array preserves individual seasons and no starting Rod");
    for (int prior : {NEI_GRACE_OFF, NEI_GRACE_GATED}) {
        context->GetOption(RSK_HYLIAS_GRACE).Set(prior);
        context->GetOption(RSK_HYLIAS_GRACE_REWARDS).Set(7);
        context->GetOption(RSK_ROD_OF_SEASONS).Set(NEI_SEASONS_GATED);
        context->GetOption(RSK_STARTING_ROD_OF_SEASONS).Set(1);
        settings->ParseJson(json{{"seed", "legacy"}, {"finalSeed", 1}, {"settings", json::object()}});
        Check(context->GetOption(RSK_HYLIAS_GRACE).Is(NEI_GRACE_ON) &&
                  context->GetOption(RSK_HYLIAS_GRACE_REWARDS).Is(4),
              "old native spoiler clears prior Off/Gated Grace policy in reused Context");
        Check(NeiGrace_CanActivate(context->GetOption(RSK_HYLIAS_GRACE).Get(),
                                  context->GetOption(RSK_HYLIAS_GRACE_REWARDS).Get(), 0, 0),
              "legacy native spoiler permits Grace before any rewards");
        Check(context->GetOption(RSK_ROD_OF_SEASONS).Is(NEI_SEASONS_INDIVIDUAL) &&
                  context->GetOption(RSK_STARTING_ROD_OF_SEASONS).Is(0),
              "legacy native spoiler clears prior Rod gates and starting option");
    }
    std::puts("PASS full native spoiler parser restores legacy ungated Grace after Off/Gated seeds");
    if (argc > 1) DumpReceiptEntranceFixtures(context, argv[1]);
    context->GetLogic()->SetContext(nullptr);
    std::puts("PASS native CVar registration, MM-start generation prep, seed snapshot, save/reload, seed changes and old defaults");
}
