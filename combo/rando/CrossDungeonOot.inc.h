// Included by OTRGlobals.cpp after native oracle initialization helpers.
#include "rando/CrossDungeonState.h"
#include "ComboGameMode.h"
#include "rando/CrossDungeonCatalogue.h"
#include "rando/CrossEntranceOotShops.h"

namespace ComboCrossOot {
using namespace ComboRando::CrossDungeon;
static HostState state(0);
static std::map<int32_t, RandomizerRegion> regions;
static std::map<int32_t, int32_t> nativeOverrides;
static bool searching = false;
static bool evaluatingExits = false;
static int Resolved(Rando::Entrance* e) {
    return ResolveNativeIndex(e->GetIndex(), e->GetReplacement() ? e->GetReplacement()->GetIndex() : e->GetIndex(),
                              nativeOverrides);
}
static void IndexRegions() {
    // RegionTable_Init recreates untyped vanilla edges when native shuffling is off. Already
    // shuffled graphs retain metadata; SetAllEntrancesData requires vanilla connected regions.
    auto edges = GetShuffleableEntrances(Rando::EntranceType::All, false);
    if (edges.empty()) {
        Rando::SetAllEntrancesData();
        edges = GetShuffleableEntrances(Rando::EntranceType::All, false);
    }
    regions.clear();
    // Native graph replays may leave only some typed edges. The immutable descriptors also
    // cover boss doors and synthetic grotto indices without reinitializing a shuffled graph.
    for (const auto& pair : Rando::GetEntranceShuffleTable()) {
        for (const auto& link : { pair.first, pair.second })
            if (link.index >= 0 && link.connectedRegion != RR_NONE)
                regions[link.index] = link.connectedRegion;
    }
    nativeOverrides.clear();
    for (const auto& entry : OTRGlobals::Instance->gRandoContext->GetEntranceShuffler()->entranceOverrides) {
        if (entry.type == 0 && entry.index == 0 && entry.override == 0)
            break;
        nativeOverrides[entry.index] = entry.override;
    }
    for (auto* e : edges) {
        if (e->GetIndex() >= 0)
            regions[e->GetIndex()] = e->GetOriginalConnectedRegionKey();
    }
}
// Cross routing moves dungeon mouths even when the native permutation is off. Native
// conditions must apply the same age/door safety rules while evaluating that graph.
struct DungeonRuleScope {
    uint8_t previous = RO_DUNGEON_ENTRANCE_SHUFFLE_OFF;
    bool changed = false;
    DungeonRuleScope() {
        if (!state.HasPool(Pool::Dungeon))
            return;
        auto& option = OTRGlobals::Instance->gRandoContext->GetOption(RSK_SHUFFLE_DUNGEON_ENTRANCES);
        previous = option.Get();
        if (previous == RO_DUNGEON_ENTRANCE_SHUFFLE_OFF) {
            option.Set(RO_DUNGEON_ENTRANCE_SHUFFLE_ON);
            changed = true;
        }
    }
    ~DungeonRuleScope() {
        if (changed)
            OTRGlobals::Instance->gRandoContext->GetOption(RSK_SHUFFLE_DUNGEON_ENTRANCES).Set(previous);
    }
};
struct SearchScope {
    DungeonRuleScope rules;
    std::list<Rando::Entrance> saved;
    size_t added = 0;
    bool previousSearching;
    SearchScope() : previousSearching(searching) {
        if (state.routes.empty()) {
            // Native baseline probes must not read completion from the playing slot either.
            searching = true;
            return;
        }
        IndexRegions(); // Replay may have replaced the native graph since this map was preflighted.
        auto* root = RegionTable(RR_ROOT);
        for (int index : state.arrivals)
            if (!regions.contains(index))
                throw std::invalid_argument("unknown OoT cross-dungeon arrival");
        if (!state.overworld)
            saved.splice(saved.end(), root->exits);
        for (int index : state.arrivals) {
            root->exits.emplace_back(
                regions.at(index), [] { return true; }, "cross-dungeon arrival");
            root->exits.back().SetParentRegion(RR_ROOT);
            ++added;
        }
        searching = true;
    }
    ~SearchScope() {
        searching = previousSearching;
        auto* root = RegionTable(RR_ROOT);
        while (added > 0) {
            root->exits.pop_back();
            --added;
        }
        root->exits.splice(root->exits.end(), saved);
    }
};
} // namespace ComboCrossOot

#ifdef COMBO_BUILD
#include "rando/CrossEntranceOotBoss.inc.h"
#endif

extern "C" int Combo_SOH_CrossDungeonRegion(int index, int resolved, int fallback) {
    using namespace ComboCrossOot;
    if (!Combo_IsCombinedGame() || !searching)
        return fallback;
    const auto* route = state.Lookup(index, ResolveNativeIndex(index, resolved, nativeOverrides));
    if (!route)
        return fallback;
    if (route->to.game != 0)
        return RR_NONE;
    auto it = regions.find(route->to.index);
    return it == regions.end() ? RR_NONE : it->second;
}

namespace ComboCrossOot {
// Native ingress identities, not destination scene names: alternate doors remain separate.
// Retail membership is independent of the physical opening. Ordinary open houses,
// tents, fairy caves and the Temple of Time remain Passage. The Mask Shop stays the portal.
static InteriorKind InteriorKindForEntrance(int32_t entrance) {
    if (IsOotRetailShopEntrance(entrance) && entrance != ENTR_KOKIRI_SHOP_0 && entrance != ENTR_GORON_SHOP_0 &&
        entrance != ENTR_ZORA_SHOP_0)
        return InteriorKind::ShopDoor;
    switch (entrance) {
        case ENTR_KOKIRI_SHOP_0:
        case ENTR_GORON_SHOP_0:
        case ENTR_ZORA_SHOP_0:
            return InteriorKind::ShopPassage;
        case ENTR_LAKESIDE_LABORATORY_0:
        case ENTR_FISHING_POND_0:
        case ENTR_MARKET_GUARD_HOUSE_0:
        case ENTR_BOMBCHU_BOWLING_ALLEY_0:
        case ENTR_TREASURE_BOX_SHOP_0:
        case ENTR_BACK_ALLEY_MAN_IN_GREEN_HOUSE:
        case ENTR_KAKARIKO_CENTER_GUEST_HOUSE_0:
        case ENTR_HOUSE_OF_SKULLTULA_0:
        case ENTR_IMPAS_HOUSE_FRONT:
        case ENTR_IMPAS_HOUSE_BACK:
        case ENTR_GRAVEKEEPERS_HUT_0:
        case ENTR_LON_LON_BUILDINGS_TALONS_HOUSE:
        case ENTR_STABLE_0:
        case ENTR_LON_LON_BUILDINGS_TOWER:
        case ENTR_SHOOTING_GALLERY_0:
        case ENTR_SHOOTING_GALLERY_1:
        case ENTR_WINDMILL_AND_DAMPES_GRAVE_WINDMILL:
            return InteriorKind::Door;
        default:
            return InteriorKind::Passage;
    }
}
static std::vector<Dungeon> BaseCatalogue(bool includeGanon, const PoolOptions& options,
                                          const std::map<int32_t, int32_t>& overrides) {
    std::vector<Dungeon> base;
    std::map<int32_t, InteriorKind> physicalKinds;
    if (!options.interiors && (options.doorInteriors || (options.legacyInteriors && options.shops))) {
        const auto record = [&](int32_t source, InteriorKind kind) {
            if (source < 0)
                return;
            const int32_t resolved = ResolveNativeIndex(source, source, overrides);
            auto [it, inserted] = physicalKinds.emplace(resolved, kind);
            if (!inserted && it->second != kind)
                it->second = InteriorKind::Passage; // Never grant an open/ambiguous source a narrow leg.
        };
        // Routing runs after native ER. Classify the physical source that now leads
        // to each interior, including non-interior and reverse sources in mixed ER.
        for (const auto& pair : Rando::GetEntranceShuffleTable()) {
            const auto& source = pair.first;
            const bool ingress =
                pair.second.index >= 0 && source.index != ENTR_HAPPY_MASK_SHOP_0 &&
                (source.type == Rando::EntranceType::Interior || source.type == Rando::EntranceType::SpecialInterior);
            const auto kind = ingress ? InteriorKindForEntrance(source.index) : InteriorKind::Passage;
            record(source.index,
                   options.legacyInteriors && kind == InteriorKind::ShopPassage ? InteriorKind::Passage : kind);
            record(pair.second.index, InteriorKind::Passage);
        }
    }
    for (const auto& pair : Rando::GetEntranceShuffleTable()) {
        const auto& entry = pair.first;
        if (entry.index == ENTR_HAPPY_MASK_SHOP_0 || pair.second.index < 0)
            continue; // This entrance remains the existing cross-game portal.
        Pool pool;
        if (options.dungeons && (entry.type == Rando::EntranceType::Dungeon ||
                                 (includeGanon && entry.type == Rando::EntranceType::GanonDungeon))) {
            pool = Pool::Dungeon;
        } else if (entry.type == Rando::EntranceType::Interior || entry.type == Rando::EntranceType::SpecialInterior) {
            const auto physical = physicalKinds.find(entry.index);
            auto kind = physical == physicalKinds.end() ? InteriorKind::Passage : physical->second;
            if (!options.legacyInteriors) {
                if (IsOotRetailShopEntrance(entry.index))
                    kind = InteriorKindForEntrance(entry.index);
                else if (kind == InteriorKind::ShopDoor || kind == InteriorKind::ShopPassage)
                    kind = InteriorKind::Passage; // A recorded native shop source cannot make a house retail.
            }
            const auto selected = InteriorPool(kind, options);
            if (!selected)
                continue;
            pool = *selected;
        } else if (options.grottos && entry.type == Rando::EntranceType::GrottoGrave) {
            pool = Pool::Grotto;
        } else {
            continue;
        }
        Dungeon portal{
            "oot:" + std::to_string(entry.index), { 0, entry.index }, { 0, pair.second.index }, { 0, pair.second.index }
        };
        portal.pool = pool;
        base.push_back(std::move(portal));
    }
    return base;
}
static const std::map<int32_t, int32_t>& BlueWarpExits() {
    static const std::map<int32_t, int32_t> warps = {
        { ENTR_KOKIRI_FOREST_DEKU_TREE_BLUE_WARP, ENTR_KOKIRI_FOREST_OUTSIDE_DEKU_TREE },
        { ENTR_DEATH_MOUNTAIN_TRAIL_DODONGO_BLUE_WARP, ENTR_DEATH_MOUNTAIN_TRAIL_OUTSIDE_DODONGOS_CAVERN },
        { ENTR_ZORAS_FOUNTAIN_JABU_JABU_BLUE_WARP, ENTR_ZORAS_FOUNTAIN_OUTSIDE_JABU_JABU },
        { ENTR_SACRED_FOREST_MEADOW_FOREST_TEMPLE_BLUE_WARP, ENTR_SACRED_FOREST_MEADOW_OUTSIDE_TEMPLE },
        { ENTR_DEATH_MOUNTAIN_CRATER_FIRE_TEMPLE_BLUE_WARP, ENTR_DEATH_MOUNTAIN_CRATER_OUTSIDE_TEMPLE },
        { ENTR_LAKE_HYLIA_WATER_TEMPLE_BLUE_WARP, ENTR_LAKE_HYLIA_OUTSIDE_TEMPLE },
        { ENTR_DESERT_COLOSSUS_SPIRIT_TEMPLE_BLUE_WARP, ENTR_DESERT_COLOSSUS_OUTSIDE_TEMPLE },
        { ENTR_GRAVEYARD_SHADOW_TEMPLE_BLUE_WARP, ENTR_GRAVEYARD_OUTSIDE_TEMPLE },
        { ENTR_OUTSIDE_GANONS_CASTLE_1_2, ENTR_CASTLE_GROUNDS_RAINBOW_BRIDGE_EXIT }
    };
    return warps;
}
static std::string CatalogueJson(bool includeGanon, const std::map<int32_t, int32_t>& overrides,
                                 const PoolOptions& options) {
    // Compose the native layout first. Boss records have distinct reverse/warp ownership and
    // must retain the metadata appended by their adapter rather than generic bluewarp inference.
    auto catalogue = BaseCatalogue(includeGanon, options, overrides);
    std::vector<Dungeon> dungeons;
    for (const auto& entry : catalogue)
        if (entry.pool == Pool::Dungeon)
            dungeons.push_back(entry);
    auto composedDungeons = ComposeNativeCatalogue(std::move(dungeons), BlueWarpExits(), overrides);
    size_t dungeon = 0;
    for (auto& entry : catalogue) {
        if (entry.pool == Pool::Dungeon)
            entry = std::move(composedDungeons[dungeon++]);
        else
            entry.returnTo.index = ResolveNativeIndex(entry.exit.index, entry.exit.index, overrides);
    }
    if (options.bosses) {
#ifdef COMBO_BUILD
        AppendBossCatalogue(catalogue, overrides);
#else
        throw std::invalid_argument("cross-game boss entrances require the combined build");
#endif
    }
    Validate(catalogue);
    auto out = nlohmann::json::array();
    for (const auto& dungeon : catalogue)
        out.push_back(DungeonJson(dungeon));
    return out.dump();
}
} // namespace ComboCrossOot

extern "C" COMBO_EXPORT const char* SOH_DumpCrossDungeonCatalogue(void) {
    using namespace ComboCrossOot;
    static std::string result;
    EnsureOracleInit();
    IndexRegions();
    const bool ganon = OTRGlobals::Instance->gRandoContext->GetOption(RSK_SHUFFLE_DUNGEON_ENTRANCES)
                           .Is(RO_DUNGEON_ENTRANCE_SHUFFLE_ON_PLUS_GANON);
    result = CatalogueJson(ganon, nativeOverrides, PoolOptions{});
    return result.c_str();
}

extern "C" COMBO_EXPORT const char* SOH_DumpCrossDungeonCatalogueForOptions(const char* raw) {
    using namespace ComboCrossOot;
    static std::string result;
    try {
        const auto options = ParseOptions(nlohmann::json::parse(raw ? raw : "{}"));
        EnsureOracleInit();
        IndexRegions();
        const bool ganon = OTRGlobals::Instance->gRandoContext->GetOption(RSK_SHUFFLE_DUNGEON_ENTRANCES)
                               .Is(RO_DUNGEON_ENTRANCE_SHUFFLE_ON_PLUS_GANON);
        result = CatalogueJson(ganon, nativeOverrides, options);
        return result.c_str();
    } catch (const std::exception& e) {
        SPDLOG_ERROR("[CrossDungeon] OoT pool catalogue rejected: {}", e.what());
        return nullptr;
    }
}

extern "C" COMBO_EXPORT const char* SOH_DumpCrossDungeonCatalogueForSeed(const char* raw) {
    using namespace ComboCrossOot;
    static std::string result;
    try {
        const auto seed = nlohmann::json::parse(raw);
        const auto& settings = seed.at("oot").at("settings");
        const int dungeonSetting = settings.value(CVAR_RANDOMIZER_SETTING("ShuffleDungeonsEntrances"), 0);
        if (dungeonSetting < RO_DUNGEON_ENTRANCE_SHUFFLE_OFF ||
            dungeonSetting > RO_DUNGEON_ENTRANCE_SHUFFLE_ON_PLUS_GANON)
            throw std::invalid_argument("invalid native dungeon shuffle setting");
        std::map<int32_t, int> knownTypes;
        for (const auto& pair : Rando::GetEntranceShuffleTable()) {
            knownTypes[pair.first.index] = static_cast<int>(pair.first.type);
            if (pair.second.index >= 0)
                knownTypes[pair.second.index] = static_cast<int>(pair.second.type);
        }
        const auto overrides =
            ParseNativeOverrides(seed.at("entrances").at("oot"), knownTypes, ENTRANCE_OVERRIDES_MAX_COUNT);
        const auto options = OptionsFromSeed(seed.value("crossDungeonEntrances", nlohmann::json{}));
        result = CatalogueJson(dungeonSetting == RO_DUNGEON_ENTRANCE_SHUFFLE_ON_PLUS_GANON, overrides, options);
        return result.c_str();
    } catch (const std::exception& e) {
        SPDLOG_ERROR("[CrossDungeon] OoT seed catalogue rejected: {}", e.what());
        return nullptr;
    }
}

extern "C" COMBO_EXPORT int SOH_SetCrossDungeonSeed(const char* raw) {
    if (!Combo_IsCombinedGame())
        return 0;
    using namespace ComboCrossOot;
    try {
        auto seed = nlohmann::json::parse(raw ? raw : "null");
        auto routes = Decode(seed);
        std::set<int32_t> known;
        for (const auto& pair : Rando::GetEntranceShuffleTable()) {
            known.insert(pair.first.index);
            if (pair.second.index >= 0)
                known.insert(pair.second.index);
        }
        for (const auto& route : routes) {
            for (auto e : { route.from, route.to })
                if (e.game == 0 && !known.contains(e.index))
                    throw std::invalid_argument("cross-dungeon seed names unknown OoT entrance");
        }
        state.Install(seed);
        return 1;
    } catch (const std::exception& e) {
        SPDLOG_ERROR("[CrossDungeon] OoT seed rejected: {}", e.what());
        return 0;
    }
}
extern "C" COMBO_EXPORT void SOH_SetCrossDungeonAccess(const char* raw) {
    ComboCrossOot::state.SetAccess(nlohmann::json::parse(raw));
}
extern "C" COMBO_EXPORT const char* SOH_GetCrossDungeonExits(void) {
    using namespace ComboCrossOot;
    DungeonRuleScope rules;
    // Native ConditionsMet rejects foreign edges redirected to RR_NONE. Evaluate the
    // native source edge while boss/world predicates still read the logical oracle mask.
    struct ExitEvaluationScope {
        bool previousSearch = searching;
        bool previousEvaluation = evaluatingExits;
        ExitEvaluationScope() {
            searching = false;
            evaluatingExits = true;
        }
        ~ExitEvaluationScope() {
            searching = previousSearch;
            evaluatingExits = previousEvaluation;
        }
    } evaluationScope;
    const auto logic = Rando::Context::GetInstance()->GetLogic();
    struct ConditionRegionScope {
        Rando::Logic& logic;
        RandomizerRegion previous;
        ~ConditionRegionScope() {
            logic.CurrentRegionKey = previous;
        }
    } conditionRegion{ *logic, logic->CurrentRegionKey };
    static std::string result;
    auto out = nlohmann::json::array();
    for (auto* e : GetShuffleableEntrances(Rando::EntranceType::All, false)) {
        const auto* route = state.Lookup(e->GetIndex(), Resolved(e));
        if (route && route->to.game != 0) {
            // Native AnyAgeTime/Here predicates read the source region, just as in a normal crawl.
            logic->CurrentRegionKey = e->GetParentRegionKey();
            if (e->ConditionsMet())
                out.push_back(EndpointJson(route->to));
        }
    }
    result = out.dump();
    return result.c_str();
}

extern "C" COMBO_EXPORT int SOH_ReadCrossDungeonCVar(void) {
    return Combo_IsCombinedGame() && CVarGetInteger("gCombo.Rando.CrossDungeonEntrances", 0) != 0;
}

extern "C" COMBO_EXPORT const char* SOH_ReadCrossEntranceOptions(void) {
    using namespace ComboRando::CrossDungeon;
    static std::string result;
    PoolOptions options;
    const bool enabled = Combo_IsCombinedGame();
    options.dungeons = enabled && CVarGetInteger("gCombo.Rando.CrossDungeonEntrances", 0) != 0;
    options.bosses = enabled && CVarGetInteger("gCombo.Rando.CrossBossEntrances", 0) != 0;
    options.interiors = enabled && CVarGetInteger("gCombo.Rando.CrossInteriorEntrances", 0) != 0;
    options.shops = enabled && CVarGetInteger("gCombo.Rando.CrossShopEntrances", 0) != 0;
    options.doorInteriors = enabled && CVarGetInteger("gCombo.Rando.CrossDoorInteriorEntrances", 0) != 0;
    options.grottos = enabled && CVarGetInteger("gCombo.Rando.CrossGrottoEntrances", 0) != 0;
    options.expanded = options.dungeons && CVarGetInteger("gCombo.Rando.ExpandedDungeonPool", 0) != 0;
    result = OptionsJson(options).dump();
    return result.c_str();
}
