// Included by BenPort.cpp next to the native MM reachability oracle.
#include "rando/CrossDungeonState.h"
#include "ComboGameMode.h"
#include "rando/CrossEntranceMmCatalogue.h"
#include "rando/CrossEntranceTime.h"
namespace ComboCrossMm {
using namespace ComboRando::CrossDungeon;
static HostState state(1);
static bool searching = false;
static std::set<RandoRegionId> reachable;
static std::unordered_map<RandoRegionId, Rando::Logic::RegionTimeState> timeStates;
} // namespace ComboCrossMm
extern "C" int Combo_MM_CrossBossEnabled(void);
#include "rando/CrossEntranceMmBoss.inc.h"
namespace ComboCrossMm {
static std::vector<Dungeon> Catalogue(const PoolOptions& options) {
    std::vector<Dungeon> dungeons = { { "mm:woodfall",
                                        { 1, ENTRANCE(WOODFALL_TEMPLE, 0) },
                                        { 1, ENTRANCE(WOODFALL, 1) },
                                        { 1, ENTRANCE(WOODFALL, 1) } },
                                      { "mm:snowhead",
                                        { 1, ENTRANCE(SNOWHEAD_TEMPLE, 0) },
                                        { 1, ENTRANCE(SNOWHEAD, 1) },
                                        { 1, ENTRANCE(SNOWHEAD, 1) } },
                                      { "mm:greatbay",
                                        { 1, ENTRANCE(GREAT_BAY_TEMPLE, 0) },
                                        { 1, ENTRANCE(ZORA_CAPE, 7) },
                                        { 1, ENTRANCE(ZORA_CAPE, 7) } },
                                      { "mm:stonetower",
                                        { 1, ENTRANCE(STONE_TOWER_TEMPLE, 0) },
                                        { 1, ENTRANCE(STONE_TOWER, 2) },
                                        { 1, ENTRANCE(STONE_TOWER, 2) } },
                                      { "mm:inverted",
                                        { 1, ENTRANCE(STONE_TOWER_TEMPLE_INVERTED, 0) },
                                        { 1, ENTRANCE(STONE_TOWER_INVERTED, 1) },
                                        { 1, ENTRANCE(STONE_TOWER_INVERTED, 1) } } };
    dungeons[0].escapeExits = { { 1, ENTRANCE(WOODFALL, 3) } };
    dungeons[1].escapeExits = { { 1, ENTRANCE(MOUNTAIN_VILLAGE_SPRING, 7) } };
    dungeons[2].escapeExits = { { 1, ENTRANCE(ZORA_CAPE, 9) } };
    dungeons[4].escapeExits = { { 1, ENTRANCE(IKANA_CANYON, 15) } };
    if (!options.dungeons)
        dungeons.clear();
    ComboCrossMmCatalogue::Append(dungeons, options);
    if (options.bosses)
        AppendBossCatalogue(dungeons);
    Validate(dungeons);
    return dungeons;
}
} // namespace ComboCrossMm
extern "C" int Combo_MM_CrossDungeonEntranceFromRegion(int index, int region) {
    using namespace ComboCrossMm;
    if (!Combo_IsCombinedGame() || !searching)
        return index;
    const int grottoExit = ComboCrossMmCatalogue::GrottoExitForRegion(static_cast<RandoRegionId>(region));
    const int original = grottoExit >= 0 ? grottoExit : index;
    const auto* route = state.Lookup(original, index);
    return !route ? index : route->to.game == 1 ? route->to.index : -1;
}
extern "C" int Combo_MM_CrossDungeonEntrance(int index) {
    return Combo_MM_CrossDungeonEntranceFromRegion(index, RR_MAX);
}
extern "C" COMBO_EXPORT const char* MM_DumpCrossDungeonCatalogueForOptions(const char* raw) {
    static std::string result;
    try {
        auto out = nlohmann::json::array();
        for (const auto& dungeon :
             ComboCrossMm::Catalogue(ComboRando::CrossDungeon::ParseOptions(nlohmann::json::parse(raw ? raw : "{}"))))
            out.push_back(ComboRando::CrossDungeon::DungeonJson(dungeon));
        result = out.dump();
    } catch (const std::exception& e) {
        SPDLOG_ERROR("[CrossEntrance] MM catalogue options rejected: {}", e.what());
        result = "[]";
    }
    return result.c_str();
}
extern "C" COMBO_EXPORT const char* MM_DumpCrossDungeonCatalogueForSeed(const char* raw) {
    static std::string result;
    try {
        const auto seed = nlohmann::json::parse(raw ? raw : "{}");
        const auto options =
            ComboRando::CrossDungeon::OptionsFromSeed(seed.value("crossDungeonEntrances", nlohmann::json{}));
        auto out = nlohmann::json::array();
        for (const auto& dungeon : ComboCrossMm::Catalogue(options))
            out.push_back(ComboRando::CrossDungeon::DungeonJson(dungeon));
        result = out.dump();
    } catch (const std::exception& e) {
        SPDLOG_ERROR("[CrossEntrance] MM seed catalogue rejected: {}", e.what());
        result = "[]";
    }
    return result.c_str();
}
extern "C" COMBO_EXPORT const char* MM_DumpCrossDungeonCatalogue(void) {
    return MM_DumpCrossDungeonCatalogueForOptions("{}");
}
extern "C" COMBO_EXPORT int MM_CanEnterCrossEntrance(int32_t index) {
    if (index < 0 || index > 0xFFFF || Rando::Logic::GetRegionIdFromEntrance(index) == RR_MAX)
        return 0;
    uint8_t owned = 0;
    for (int halfDay = 0; halfDay < 6; ++halfDay)
        if (Rando::Logic::OwnsClockHalfDay(halfDay))
            owned |= 1u << halfDay;
    // Raw owned flags deliberately exclude TimeLogic's synthetic day-one fallback.
    return ComboRando::CrossDungeon::MmForeignArrivalAllowed(CAN_PLAY_SONG(TIME), Rando::Logic::SettingClocks(), owned,
                                                             false);
}
extern "C" int Combo_MM_CrossBossLogicEnabled(void) {
    return Combo_IsCombinedGame() && ComboCrossMm::state.HasPool(ComboRando::CrossDungeon::Pool::Boss);
}
extern "C" COMBO_EXPORT int MM_SetCrossDungeonSeed(const char* raw) {
    if (!Combo_IsCombinedGame())
        return 0;
    using namespace ComboCrossMm;
    try {
        auto seed = nlohmann::json::parse(raw ? raw : "null");
        auto routes = Decode(seed);
        for (const auto& route : routes) {
            // The vanilla one-way Goht exit has a COMBO arrival-region alias for boss-only maps.
            // Preserve the legacy source exception while validating all actual destinations.
            const bool gohtEscape =
                route.phase == Phase::BeforeNative && route.from == Endpoint{ 1, ENTRANCE(MOUNTAIN_VILLAGE_SPRING, 7) };
            if (route.from.game == 1 && !gohtEscape &&
                Rando::Logic::GetRegionIdFromEntrance(route.from.index) == RR_MAX)
                throw std::invalid_argument("cross-dungeon seed names unknown MM source " +
                                            std::to_string(route.from.index));
            if (route.to.game == 1 && Rando::Logic::GetRegionIdFromEntrance(route.to.index) == RR_MAX)
                throw std::invalid_argument("cross-dungeon seed names unknown MM destination " +
                                            std::to_string(route.to.index));
        }
        state.Install(seed);
        return 1;
    } catch (const std::exception& e) {
        SPDLOG_ERROR("[CrossDungeon] MM seed rejected: {}", e.what());
        return 0;
    }
}
extern "C" COMBO_EXPORT void MM_SetCrossDungeonAccess(const char* raw) {
    ComboCrossMm::state.SetAccess(nlohmann::json::parse(raw));
}
extern "C" COMBO_EXPORT const char* MM_GetCrossDungeonExits(void) {
    using namespace ComboCrossMm;
    struct SearchScope {
        bool previous = searching;
        SearchScope() {
            searching = true;
        }
        ~SearchScope() {
            searching = previous;
        }
    } searchScope;
    static std::string result;
    auto out = nlohmann::json::array();
    for (auto region : reachable) {
        Rando::Logic::SetCurrentRegionTime(timeStates, region);
        for (const auto& [index, exit] : Rando::Logic::Regions.at(region).exits) {
            const int grottoExit = ComboCrossMmCatalogue::GrottoExitForRegion(region);
            const auto* route = state.Lookup(grottoExit >= 0 ? grottoExit : index, index);
            if (route && route->to.game != 1 && exit.condition())
                out.push_back(EndpointJson(route->to));
        }
    }
    result = out.dump();
    return result.c_str();
}
