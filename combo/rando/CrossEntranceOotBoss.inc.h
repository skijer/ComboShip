// Included after ComboCrossOot's oracle HostState is defined.
#include "rando/CrossEntranceBoss.h"
namespace ComboCrossOot {
static ComboRando::CrossDungeon::BossCompletionState bossCompletion;
static const int32_t bossEntrances[8] = { ENTR_DEKU_TREE_BOSS_ENTRANCE,     ENTR_DODONGOS_CAVERN_BOSS_ENTRANCE,
                                          ENTR_JABU_JABU_BOSS_ENTRANCE,     ENTR_FOREST_TEMPLE_BOSS_ENTRANCE,
                                          ENTR_FIRE_TEMPLE_BOSS_ENTRANCE,   ENTR_WATER_TEMPLE_BOSS_ENTRANCE,
                                          ENTR_SPIRIT_TEMPLE_BOSS_ENTRANCE, ENTR_SHADOW_TEMPLE_BOSS_ENTRANCE };
static int BossFromScene(int scene) {
    switch (scene) {
        case SCENE_DEKU_TREE_BOSS:
            return COMBO_BOSS_GOHMA;
        case SCENE_DODONGOS_CAVERN_BOSS:
            return COMBO_BOSS_DODONGO;
        case SCENE_JABU_JABU_BOSS:
            return COMBO_BOSS_BARINADE;
        case SCENE_FOREST_TEMPLE_BOSS:
            return COMBO_BOSS_PHANTOM_GANON;
        case SCENE_FIRE_TEMPLE_BOSS:
            return COMBO_BOSS_VOLVAGIA;
        case SCENE_WATER_TEMPLE_BOSS:
            return COMBO_BOSS_MORPHA;
        case SCENE_SPIRIT_TEMPLE_BOSS:
            return COMBO_BOSS_TWINROVA;
        case SCENE_SHADOW_TEMPLE_BOSS:
            return COMBO_BOSS_BONGO_BONGO;
        default:
            return -1;
    }
}
static bool BossLogicEnabled() {
    return Combo_IsCombinedGame() && state.HasPool(Pool::Boss);
}
// Immutable native descriptors: seed preflight must not inspect a live shuffled entrance table.
static int OriginalEntranceScene(int32_t index) {
    static const int16_t scenes[] = {
#define DEFINE_ENTRANCE(name, scene, ...) scene,
#include "tables/entrance_table.h"
#undef DEFINE_ENTRANCE
    };
    return index >= 0 && static_cast<size_t>(index) < std::size(scenes) ? scenes[index] : -1;
}
static void AppendBossCatalogue(std::vector<Dungeon>& out, const std::map<int32_t, int32_t>& overrides) {
    const int32_t reverse[] = { ENTR_DEKU_TREE_BOSS_DOOR,     ENTR_DODONGOS_CAVERN_BOSS_DOOR,
                                ENTR_JABU_JABU_BOSS_DOOR,     ENTR_FOREST_TEMPLE_BOSS_DOOR,
                                ENTR_FIRE_TEMPLE_BOSS_DOOR,   ENTR_WATER_TEMPLE_BOSS_DOOR,
                                ENTR_SPIRIT_TEMPLE_BOSS_DOOR, ENTR_SHADOW_TEMPLE_BOSS_DOOR };
    const int32_t dungeon[] = { ENTR_DEKU_TREE_ENTRANCE,     ENTR_DODONGOS_CAVERN_ENTRANCE, ENTR_JABU_JABU_ENTRANCE,
                                ENTR_FOREST_TEMPLE_ENTRANCE, ENTR_FIRE_TEMPLE_ENTRANCE,     ENTR_WATER_TEMPLE_ENTRANCE,
                                ENTR_SPIRIT_TEMPLE_ENTRANCE, ENTR_SHADOW_TEMPLE_ENTRANCE };
    const int32_t warp[] = {
        ENTR_KOKIRI_FOREST_DEKU_TREE_BLUE_WARP,           ENTR_DEATH_MOUNTAIN_TRAIL_DODONGO_BLUE_WARP,
        ENTR_ZORAS_FOUNTAIN_JABU_JABU_BLUE_WARP,          ENTR_SACRED_FOREST_MEADOW_FOREST_TEMPLE_BLUE_WARP,
        ENTR_DEATH_MOUNTAIN_CRATER_FIRE_TEMPLE_BLUE_WARP, ENTR_LAKE_HYLIA_WATER_TEMPLE_BLUE_WARP,
        ENTR_DESERT_COLOSSUS_SPIRIT_TEMPLE_BLUE_WARP,     ENTR_GRAVEYARD_SHADOW_TEMPLE_BLUE_WARP
    };
    const auto checkpointFor = [&](int32_t resolved) -> std::optional<Endpoint> {
        for (int i = 0; i < 8; ++i) {
            if (resolved == reverse[i])
                return Endpoint{ 0, dungeon[i] };
            if (resolved == dungeon[i])
                return Endpoint{ 0, resolved };
        }
        // Exact catalogue identities retain interior/grotto ownership. An arbitrary
        // alternate interior spawn has no durable owner in the runtime catalogue.
        for (const auto& owned : out)
            if (owned.entrance == Endpoint{ 0, resolved } && (IsInteriorPool(owned.pool) || owned.pool == Pool::Grotto))
                return owned.entrance;
        const int scene = OriginalEntranceScene(resolved);
        if (scene < 0)
            return std::nullopt;
        // Match the runtime's native dungeon/boss scene lifetime even when that
        // dungeon is not selected for a cross pool (native mixed ER can reach it).
        const bool dungeonScene = (scene >= SCENE_DEKU_TREE && scene <= SCENE_GANONS_TOWER_COLLAPSE_INTERIOR &&
                                   scene != SCENE_THIEVES_HIDEOUT) ||
                                  scene == SCENE_INSIDE_GANONS_CASTLE_COLLAPSE ||
                                  (scene >= SCENE_DEKU_TREE_BOSS && scene <= SCENE_GANONS_TOWER_COLLAPSE_EXTERIOR) ||
                                  scene == SCENE_GANON_BOSS;
        if (dungeonScene)
            return Endpoint{ 0, resolved };
        return std::nullopt; // Exterior native returns release checkpoint ownership.
    };
    for (int id = 0; id < 8; ++id) {
        Dungeon boss{ "oot:boss:" + std::to_string(id),
                      { 0, bossEntrances[id] },
                      { 0, reverse[id] },
                      { 0, ResolveNativeIndex(reverse[id], reverse[id], overrides) },
                      { { 0, warp[id] } } };
        boss.pool = Pool::Boss;
        boss.boss = id;
        boss.owner = "oot:" + std::to_string(dungeon[id]);
        boss.warpReturn = Endpoint{ 0, ResolveNativeIndex(warp[id], warp[id], overrides) };
        boss.reverseCheckpoint = checkpointFor(boss.returnTo.index);
        boss.warpCheckpoint = checkpointFor(boss.warpReturn->index);
        // Native Entrance_Init assigns death/save ownership from the INCOMING
        // override. The reverse door is independently shuffled in decoupled ER.
        int32_t parent = dungeon[id];
        for (const auto& [from, to] : overrides) {
            if (to != bossEntrances[id])
                continue;
            for (const auto& pair : Rando::GetEntranceShuffleTable()) {
                if (from == pair.first.index && pair.second.index >= 0)
                    parent = pair.second.index;
                else if (from == pair.second.index)
                    parent = pair.first.index;
            }
            for (int source = 0; source < 8; ++source)
                if (from == bossEntrances[source])
                    parent = dungeon[source];
        }
        // A native BossReverse may lead to an arena. Recover at that arena's
        // canonical temple instead, so a later load cannot cross-shuffle it again.
        for (int source = 0; source < 8; ++source)
            if (parent == bossEntrances[source])
                parent = dungeon[source];
        boss.deathCheckpoint = Endpoint{ 0, parent };
        out.push_back(std::move(boss));
    }
}
} // namespace ComboCrossOot

extern "C" COMBO_EXPORT void SOH_SetCrossBossNotify(void (*notify)(int)) {
    ComboCrossOot::bossCompletion.SetNotify(notify);
}
extern "C" COMBO_EXPORT void SOH_SetCrossBossCompletedMask(uint16_t mask) {
    ComboCrossOot::bossCompletion.SetCompletedMask(mask);
}
extern "C" int Combo_SOH_CrossBossLogicalClear(int id, int nativeValue) {
    using namespace ComboCrossOot;
    if ((searching || evaluatingExits) && BossLogicEnabled())
        return id >= 0 && id < COMBO_BOSS_COUNT && (state.logicalClears & (1u << id));
    return bossCompletion.LogicalClear(id, nativeValue != 0,
                                       !searching && !evaluatingExits && Combo_SOH_CrossBossEnabled());
}
extern "C" int Combo_SOH_CrossBossWarp(int scene) {
    return ComboCrossOot::bossCompletion.Warp(ComboCrossOot::BossFromScene(scene), Combo_SOH_CrossBossEnabled());
}
extern "C" COMBO_EXPORT void SOH_ApplyCrossBossClear(int sourceBossId, int dormant) try {
    using namespace ComboCrossOot;
    if (sourceBossId < 0 || sourceBossId >= 8 || gSaveContext.fileNum > 2)
        return;
    bossCompletion.SetCompletedMask(bossCompletion.CompletedMask() | (1u << sourceBossId));
    // These are world effects only. Used-bluewarp flags collect physical rewards and stay native.
    switch (sourceBossId) {
        case COMBO_BOSS_GOHMA:
            Flags_SetEventChkInf(EVENTCHKINF_OBTAINED_KOKIRI_EMERALD_DEKU_TREE_DEAD);
            Flags_SetEventChkInf(EVENTCHKINF_SPOKE_TO_MIDO_AFTER_DEKU_TREES_DEATH);
            break;
        case COMBO_BOSS_PHANTOM_GANON:
            Flags_SetEventChkInf(EVENTCHKINF_SPOKE_TO_DEKU_TREE_SPROUT);
            break;
        case COMBO_BOSS_VOLVAGIA:
            Flags_SetEventChkInf(EVENTCHKINF_DEATH_MOUNTAIN_ERUPTED);
            break;
        case COMBO_BOSS_MORPHA:
            Flags_SetEventChkInf(EVENTCHKINF_RAISED_LAKE_HYLIA_WATER);
            break;
    }
    // A parked host can retain its PlayState; the launcher supplies explicit dormancy.
    if (dormant || !gPlayState)
        SaveManager::Instance->SaveFile(gSaveContext.fileNum);
} catch (const std::exception& e) {
    SPDLOG_ERROR("[CrossBoss] OoT source clear could not be saved: {}", e.what());
} catch (...) { SPDLOG_ERROR("[CrossBoss] OoT source clear failed"); }

extern "C" COMBO_EXPORT uint16_t SOH_GetCrossBossDefeated(void) {
    const RandomizerRegion rooms[] = { RR_DEKU_TREE_BOSS_ROOM,        RR_DODONGOS_CAVERN_BOSS_ROOM,
                                       RR_JABU_JABUS_BELLY_BOSS_ROOM, RR_FOREST_TEMPLE_BOSS_ROOM,
                                       RR_FIRE_TEMPLE_BOSS_ROOM,      RR_WATER_TEMPLE_BOSS_ROOM,
                                       RR_SPIRIT_TEMPLE_BOSS_ROOM,    RR_SHADOW_TEMPLE_BOSS_ROOM };
    const LogicVal events[] = { LOGIC_DEKU_TREE_CLEAR,     LOGIC_DODONGOS_CAVERN_CLEAR, LOGIC_JABU_JABUS_BELLY_CLEAR,
                                LOGIC_FOREST_TEMPLE_CLEAR, LOGIC_FIRE_TEMPLE_CLEAR,     LOGIC_WATER_TEMPLE_CLEAR,
                                LOGIC_SPIRIT_TEMPLE_CLEAR, LOGIC_SHADOW_TEMPLE_CLEAR };
    uint16_t defeated = 0;
    const auto logic = OTRGlobals::Instance->gRandoContext->GetLogic();
    for (int id = 0; id < 8; ++id) {
        const auto* room = RegionTable(rooms[id]);
        if ((room->Child() || room->Adult()) && logic->Get(events[id]))
            defeated |= 1u << id;
    }
    return defeated;
}
