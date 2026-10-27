// Gameplay route ownership is deliberately separate from CrossDungeonMm.inc.h's oracle state.
#include "rando/CrossDungeonRuntime.h"
#include "ComboGameMode.h"
#include "rando/CrossEntranceMmGrottos.inc.h"
extern "C" int Combo_MM_GiantsEscape(int32_t* entrance, int32_t* cutscene, int consume);
namespace ComboCrossMmRuntime {
using namespace ComboRando::CrossDungeon;
static RuntimeState state(1);
static std::optional<RuntimeState::Transition> pending;
static bool departureFadeComplete = false;
static std::optional<Endpoint> pendingBossRecovery;
static bool bossDeathRecoveryRequested = false;
static CheckpointState checkpoint;
static int32_t localDungeonArrival = -1;
static bool ValidEntrance(int32_t index) {
    if (index < 0 || index > 0xFFFF)
        return false;
    if (ComboCrossMmCatalogue::FindGrottoByEntry(index) || ComboCrossMmCatalogue::FindGrottoByExit(index))
        return true;
    return Rando::Logic::GetRegionIdFromEntrance(index) != RR_MAX;
}
static void ObserveScene(int scene) {
    const bool temple = scene == SCENE_MITURIN || scene == SCENE_HAKUGIN || scene == SCENE_SEA ||
                        scene == SCENE_INISIE_N || scene == SCENE_INISIE_R || scene == SCENE_MITURIN_BS ||
                        scene == SCENE_HAKUGIN_BS || scene == SCENE_SEA_BS || scene == SCENE_INISIE_BS ||
                        scene == SCENE_KYOJINNOMA;
    bool owned = temple;
    if (state.HasPool(Pool::Grotto) && scene == SCENE_KAKUSIANA)
        owned = true;
    for (const auto& portal : ComboCrossMmCatalogue::Interiors())
        if (state.FindEntrance(portal.entrance) && Rando::Logic::Regions.at(portal.region).sceneId == scene)
            owned = true;
    if (state.HasPool(Pool::Dungeon)) {
        for (const auto& portal : ComboCrossMmCatalogue::ExpandedDungeons())
            if (state.FindEntrance(portal.entrance) && Rando::Logic::Regions.at(portal.region).sceneId == scene)
                owned = true;
        if (state.FindEntrance(ENTRANCE(IKANA_CASTLE, 1)) && scene == SCENE_IKNINSIDE)
            owned = true;
        if (state.FindEntrance(ENTRANCE(PIRATES_FORTRESS_EXTERIOR, 0)) &&
            (scene == SCENE_KAIZOKU || scene == SCENE_PIRATE))
            owned = true;
    }
    if (localDungeonArrival >= 0) {
        checkpoint.Arrive(localDungeonArrival, owned);
        localDungeonArrival = -1;
    }
    checkpoint.ObserveScene(owned);
}
static int32_t RecoverBoss(int32_t index) {
    const auto recovery = state.RecoveryForBoss({ 1, index });
    if (!recovery)
        return -1;
    if (recovery->source.game != 1) {
        pendingBossRecovery = Endpoint{ 1, index };
        return index; // Native placeholder until the next clean-frame foreign resume.
    }
    checkpoint.Arrive(recovery->parent.index, true);
    return recovery->parent.index;
}
static bool PrepareSwitch() {
    if (pendingBossRecovery) {
        const auto arena = *pendingBossRecovery;
        auto queue = reinterpret_cast<int (*)(int, int32_t)>(Combo_ResolveSym("soh", "SOH_QueueCrossBossRecovery"));
        if (!queue || !gComboReturnCallback || !queue(arena.game, arena.index)) {
            SPDLOG_ERROR("[CrossDungeon] MM boss recovery rejected; source resume was not queued");
            return false;
        }
        pendingBossRecovery.reset();
        pending.reset();
        return true;
    }
    if (!pending || !departureFadeComplete)
        return false;
    const auto transition = *pending;
    pending.reset();
    departureFadeComplete = false;
    auto queue = reinterpret_cast<int (*)(int32_t)>(Combo_ResolveSym("soh", "SOH_QueueCrossDungeonArrival"));
    if (!queue || !gComboReturnCallback || !queue(transition.to.index)) {
        SPDLOG_ERROR("[CrossDungeon] MM handoff rejected; peer arrival was not queued");
        return false;
    }
    const int32_t nativeCheckpoint = ComboMmMaterializeGrotto(transition.checkpoint.index);
    gSaveContext.save.entrance = nativeCheckpoint;
    gSaveContext.save.shipSaveInfo.pauseSaveEntrance = nativeCheckpoint;
    checkpoint.SetSource(transition.checkpoint.index);
    SPDLOG_INFO("[CrossDungeon] MM -> OoT entrance {:#x}; source checkpoint {:#x}", transition.to.index,
                transition.checkpoint.index);
    return true;
}
} // namespace ComboCrossMmRuntime
extern "C" int Combo_MM_CrossDungeonTransitionPending(void) {
    return ComboCrossMmRuntime::pending.has_value();
}
extern "C" int Combo_MM_CompleteCrossDungeonTransition(void) {
    if (!Combo_IsCombinedGame())
        return 0;
    if (!ComboCrossMmRuntime::pending)
        return 0;
    ComboCrossMmRuntime::departureFadeComplete = true;
    return 1;
}
extern "C" void Combo_MM_CancelCrossDungeonTransition(void) {
    ComboCrossMmRuntime::pending.reset();
    ComboCrossMmRuntime::departureFadeComplete = false;
}
extern "C" COMBO_EXPORT int MM_SetCrossDungeonRuntimeSeed(const char* raw) try {
    if (!Combo_IsCombinedGame())
        return 0;
    auto seed = nlohmann::json::parse(raw ? raw : "null");
    const auto routes = ComboRando::CrossDungeon::Decode(seed);
    for (const auto& route : routes)
        for (auto endpoint : { route.from, route.to })
            if (endpoint.game == 1 && !ComboCrossMmRuntime::ValidEntrance(endpoint.index))
                throw std::invalid_argument("unknown MM runtime entrance");
    if (!routes.empty())
        for (const auto& rawDungeon : seed.at("dungeons")) {
            const auto dungeon = ComboRando::CrossDungeon::ParseDungeon(rawDungeon);
            if (dungeon.pool == ComboRando::CrossDungeon::Pool::Boss && dungeon.entrance.game == 1 &&
                (!ComboCrossMmRuntime::ValidEntrance(dungeon.returnTo.index) ||
                 (dungeon.reverseCheckpoint && !ComboCrossMmRuntime::ValidEntrance(dungeon.reverseCheckpoint->index)) ||
                 (dungeon.deathCheckpoint && !ComboCrossMmRuntime::ValidEntrance(dungeon.deathCheckpoint->index))))
                throw std::invalid_argument("unknown MM boss source recovery entrance");
        }
    ComboCrossMmRuntime::state.Install(seed);
    Combo_MM_CancelCrossDungeonTransition();
    ComboCrossMmRuntime::pendingBossRecovery.reset();
    ComboCrossMmRuntime::bossDeathRecoveryRequested = false;
    ComboCrossMmRuntime::localDungeonArrival = -1;
    return 1;
} catch (const std::exception& e) {
    SPDLOG_ERROR("[CrossDungeon] MM runtime seed rejected: {}", e.what());
    return 0;
}
extern "C" COMBO_EXPORT int MM_QueueCrossDungeonArrival(int32_t index) {
    if (!Combo_IsCombinedGame())
        return 0;
    if (!ComboCrossMmRuntime::state.QueueArrival(index))
        return 0;
    ComboCrossMmRuntime::pendingBossRecovery.reset();
    ComboCrossMmRuntime::bossDeathRecoveryRequested = false;
    return 1;
}
extern "C" COMBO_EXPORT int MM_QueueCrossBossRecovery(int game, int32_t index) {
    if (!Combo_IsCombinedGame())
        return 0;
    using namespace ComboCrossMmRuntime;
    const auto recovery = state.RecoveryForBoss({ game, index });
    if (!recovery || recovery->source.game != 1 || !ValidEntrance(recovery->parent.index) ||
        !state.QueueBossRecovery({ game, index }))
        return 0;
    pendingBossRecovery.reset();
    bossDeathRecoveryRequested = false;
    return 1;
}
extern "C" int32_t Combo_MM_ConsumeCrossBossRecoveryResume(void) {
    if (!Combo_IsCombinedGame())
        return -1;
    using namespace ComboCrossMmRuntime;
    const auto recovery = state.ConsumeBossRecovery();
    if (!recovery)
        return -1;
    checkpoint.Arrive(recovery->parent.index, true);
    localDungeonArrival = -1;
    gSaveContext.respawnFlag = 0;
    return ComboMmMaterializeGrotto(recovery->parent.index);
}
extern "C" int Combo_MM_CrossDungeonRuntimeEnabled(void) {
    return Combo_IsCombinedGame() && ComboCrossMmRuntime::state.Enabled();
}
extern "C" int Combo_MM_CrossBossEnabled(void) {
    return Combo_IsCombinedGame() && gSaveContext.gameMode == GAMEMODE_NORMAL && gSaveContext.fileNum >= 0 &&
           gSaveContext.fileNum <= 2 && ComboCrossMmRuntime::state.HasPool(ComboRando::CrossDungeon::Pool::Boss);
}
extern "C" void Combo_MM_RestoreCrossDungeonCheckpoint(int32_t index) {
    using namespace ComboCrossMmRuntime;
    const bool known = state.Enabled() && state.IsRecoveryEntrance(index) && ValidEntrance(index);
    checkpoint.Arrive(known ? index : -1, known);
    pendingBossRecovery.reset();
    bossDeathRecoveryRequested = false;
    state.ClearBossRecovery();
    localDungeonArrival = -1;
}
extern "C" int32_t Combo_MM_ConsumeCrossDungeonArrival(void) {
    if (!Combo_IsCombinedGame())
        return -1;
    using namespace ComboCrossMmRuntime;
    const int32_t logical = state.ConsumeArrival();
    if (logical < 0)
        return logical;
    const int32_t recovery = state.RecoveryForArrival(logical);
    checkpoint.Arrive(recovery, recovery >= 0);
    localDungeonArrival = -1;
    gSaveContext.respawnFlag = 0;
    return ComboMmMaterializeGrotto(logical);
}
extern "C" void Combo_MM_RequestCrossBossDeathRecovery(void) {
    using namespace ComboCrossMmRuntime;
    // Native automatic game-over and the optional Continue menu use different respawn modes.
    bossDeathRecoveryRequested =
        Combo_MM_CrossBossEnabled() && state.RecoveryForBoss({ 1, checkpoint.Saved() }).has_value();
}
// MM has no native entrance permutation; the central transition boundary takes one composed leg.
extern "C" int Combo_MM_CrossDungeonTransition(int32_t* index) {
    if (!Combo_IsCombinedGame())
        return 0;
    using namespace ComboCrossMmRuntime;
    if (!index || gSaveContext.gameMode != GAMEMODE_NORMAL || gSaveContext.fileNum < 0 || gSaveContext.fileNum > 2 ||
        !state.Enabled())
        return 0;
    if (bossDeathRecoveryRequested) {
        bossDeathRecoveryRequested = false;
        const int32_t recovery = RecoverBoss(checkpoint.Saved());
        if (recovery >= 0) {
            if (pendingBossRecovery)
                return 1;
            *index = ComboMmMaterializeGrotto(recovery);
            gSaveContext.respawnFlag = 0;
            gSaveContext.nextCutsceneIndex = 0xFFEF;
            gSaveContext.save.cutsceneIndex = 0;
            return 0;
        }
    }
    // Grotto exit mode4 is a physical portal return. Other respawn modes remain local death/void.
    const int32_t canonicalExit = ComboMmCanonicalGrottoExit(*index, gPlayState ? gPlayState->sceneId : -1);
    const bool knownGrottoExit = canonicalExit != *index && ComboCrossMmCatalogue::FindGrottoByExit(canonicalExit);
    if (gSaveContext.respawnFlag != 0 && !(gSaveContext.respawnFlag == 4 && knownGrottoExit))
        return 0;
    const int32_t source = knownGrottoExit ? canonicalExit : ComboMmCanonicalGrottoLoad(*index);
    auto transition = state.Lookup(Phase::BeforeNative, source);
    if (!transition && *index == ENTRANCE(GIANTS_CHAMBER, 0)) {
        int32_t finalEntrance = 0, finalCutscene = 0;
        if (Combo_MM_GiantsEscape(&finalEntrance, &finalCutscene, false)) {
            auto escape = state.Lookup(Phase::BeforeNative, finalEntrance);
            if (escape && Combo_MM_GiantsEscape(&finalEntrance, &finalCutscene, true)) {
                *index = finalEntrance;
                gSaveContext.nextCutsceneIndex = finalCutscene;
                transition = escape;
            }
        }
    }
    if (!transition)
        transition = state.Lookup(Phase::AfterNative, source);
    if (!transition)
        return 0;
    pendingBossRecovery.reset();
    if (transition->to != Endpoint{ 1, *index }) {
        gSaveContext.nextCutsceneIndex = 0xFFEF;
        gSaveContext.save.cutsceneIndex = 0;
    }
    if (transition->to.game == 1) {
        localDungeonArrival = transition->arrivalCheckpoint && transition->arrivalCheckpoint->game == 1
                                  ? transition->arrivalCheckpoint->index
                                  : state.RecoveryForArrival(transition->to.index);
        if (localDungeonArrival < 0)
            checkpoint.Arrive(-1, false); // A return may enter another selected interior's scene.
        // A selected route takes exactly one composed leg; synthetic identities stop here.
        *index = ComboMmMaterializeGrotto(transition->to.index);
        return 0;
    }
    pending = transition;
    departureFadeComplete = false;
    return 1;
}
extern "C" int32_t Combo_MM_SaveCrossDungeonCheckpoint(void) {
    return ComboCrossMmRuntime::checkpoint.Saved();
}
extern "C" void Combo_MM_LoadCrossDungeonCheckpoint(int32_t index) {
    ComboCrossMmRuntime::checkpoint.Load(ComboCrossMmRuntime::ValidEntrance(index) ? index : -1);
    ComboCrossMmRuntime::localDungeonArrival = -1;
    ComboCrossMmRuntime::pendingBossRecovery.reset();
    ComboCrossMmRuntime::bossDeathRecoveryRequested = false;
}
extern "C" int32_t Combo_MM_ConsumeCrossDungeonRecovery(void) {
    using namespace ComboCrossMmRuntime;
    if (state.HasBossRecovery())
        return -1; // The combo title path owns the frozen-source resume.
    const int32_t logical = checkpoint.ConsumeRecovery([](int32_t entry) { return state.IsRecoveryEntrance(entry); });
    if (logical < 0)
        return logical;
    const int32_t bossRecovery = RecoverBoss(logical);
    gSaveContext.respawnFlag = 0;
    return ComboMmMaterializeGrotto(bossRecovery >= 0 ? bossRecovery : logical);
}
