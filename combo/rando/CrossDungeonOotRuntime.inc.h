// Gameplay route ownership is deliberately separate from CrossDungeonOot.inc.h's oracle state.
#include "rando/CrossDungeonRuntime.h"
#include "ComboGameMode.h"
#include "soh/Enhancements/randomizer/randomizer_grotto.h"
#include "soh/Enhancements/randomizer/randomizer_entrance.h"
void Combo_SOH_CapturePendingChecks();
void Combo_SOH_RestorePendingChecks();
extern "C" void Entrance_ApplyCrossDungeonSafety(void);
extern "C" int32_t Combo_SOH_GrottoScene(int32_t index);
namespace ComboCrossOotRuntime {
using namespace ComboRando::CrossDungeon;
static RuntimeState state(0);
static std::optional<RuntimeState::Transition> pending;
static bool departureFadeComplete = false;
static std::optional<Endpoint> pendingBossRecovery;
static CheckpointState checkpoint;
static int32_t localDungeonArrival = -1;
static int32_t localArrivalCheckpoint = -1;
static bool nativeLoadDispatch = false;
static bool ValidEntrance(int32_t index) {
    return (index >= 0 && index < ENTRANCE_TABLE_SIZE) ||
           (index >= ENTRANCE_GROTTO_LOAD_START && index < ENTRANCE_GROTTO_LOAD_START + NUM_GROTTOS) ||
           (index >= ENTRANCE_GROTTO_EXIT_START && index < ENTRANCE_GROTTO_EXIT_START + NUM_GROTTOS);
}
static int32_t Materialize(int32_t index) {
    return Grotto_OverrideSpecialEntrance(static_cast<int16_t>(index));
}
static bool SceneMatchesEntrance(int32_t index, int scene) {
    if (index >= ENTRANCE_GROTTO_LOAD_START)
        return Combo_SOH_GrottoScene(index) == scene;
    if (index < 0 || index >= ENTRANCE_TABLE_SIZE)
        return false;
    for (int layer = 0; layer < 4 && index + layer < ENTRANCE_TABLE_SIZE; ++layer)
        if (gEntranceTable[index + layer].scene == scene)
            return true;
    return false;
}
static void ObserveScene(int scene) {
    if (localDungeonArrival >= 0) {
        if (SceneMatchesEntrance(localDungeonArrival, scene))
            checkpoint.Arrive(localArrivalCheckpoint, localArrivalCheckpoint >= 0);
        localDungeonArrival = localArrivalCheckpoint = -1;
    }
    const auto* owner = state.FindEntrance(checkpoint.Saved());
    if (owner && (IsInteriorPool(owner->pool) || owner->pool == Pool::Grotto)) {
        // The logical checkpoint also survives native Farore/death transitions, which may
        // clear the transient grotto ID while retaining this exact scene/room respawn.
        checkpoint.ObserveScene(SceneMatchesEntrance(owner->entrance.index, scene));
        return;
    }
    const bool dungeon =
        (scene >= SCENE_DEKU_TREE && scene <= SCENE_GANONS_TOWER_COLLAPSE_INTERIOR && scene != SCENE_THIEVES_HIDEOUT) ||
        scene == SCENE_INSIDE_GANONS_CASTLE_COLLAPSE ||
        (scene >= SCENE_DEKU_TREE_BOSS && scene <= SCENE_GANONS_TOWER_COLLAPSE_EXTERIOR) || scene == SCENE_GANON_BOSS;
    checkpoint.ObserveScene(dungeon);
}
static int32_t RecoverBoss(int32_t index) {
    const auto recovery = state.RecoveryForBoss({ 0, index });
    if (!recovery)
        return -1;
    if (recovery->source.game != 0) {
        pendingBossRecovery = Endpoint{ 0, index };
        return index; // Native placeholder until the next clean-frame foreign resume.
    }
    checkpoint.Arrive(recovery->parent.index, true);
    return recovery->parent.index;
}
static bool PrepareSwitch() {
    if (pendingBossRecovery) {
        const auto arena = *pendingBossRecovery;
        auto queue = reinterpret_cast<int (*)(int, int32_t)>(Combo_ResolveSym("2ship", "MM_QueueCrossBossRecovery"));
        if (!queue || !gComboSceneSwitchCallback || !queue(arena.game, arena.index)) {
            SPDLOG_ERROR("[CrossDungeon] OoT boss recovery rejected; source resume was not queued");
            return false;
        }
        pendingBossRecovery.reset();
        pending.reset();
        if (gPlayState) {
            gPlayState->transitionTrigger = TRANS_TRIGGER_OFF;
            gPlayState->transitionMode = TRANS_MODE_OFF;
        }
        return true;
    }
    if (!pending || !departureFadeComplete)
        return false;
    const auto transition = *pending;
    pending.reset();
    departureFadeComplete = false;
    auto queue = reinterpret_cast<int (*)(int32_t)>(Combo_ResolveSym("2ship", "MM_QueueCrossDungeonArrival"));
    if (!queue || !gComboSceneSwitchCallback || !queue(transition.to.index)) {
        SPDLOG_ERROR("[CrossDungeon] OoT handoff rejected; peer arrival was not queued");
        return false;
    }
    // Source remains a valid native save even if the process closes before peer arrival. Grotto
    // destinations are already shuffled; materialize their spawn/respawn data without a second lookup.
    const int32_t nativeCheckpoint = Materialize(transition.checkpoint.index);
    if (nativeCheckpoint >= 0 && nativeCheckpoint < ENTRANCE_TABLE_SIZE) {
        gSaveContext.entranceIndex = nativeCheckpoint;
        gSaveContext.savedSceneNum = gEntranceTable[nativeCheckpoint].scene;
    }
    checkpoint.SetSource(transition.checkpoint.index);
    if (gPlayState) {
        gPlayState->transitionTrigger = TRANS_TRIGGER_OFF;
        gPlayState->transitionMode = TRANS_MODE_OFF;
    }
    SPDLOG_INFO("[CrossDungeon] OoT -> MM entrance {:#x}; source checkpoint {:#x}", transition.to.index,
                transition.checkpoint.index);
    return true;
}
} // namespace ComboCrossOotRuntime
// Native Play keeps drawing its entrance effect until completion.
extern "C" int Combo_SOH_CrossDungeonTransitionPending(void) {
    return ComboCrossOotRuntime::pending.has_value();
}
extern "C" int Combo_SOH_CompleteCrossDungeonTransition(void) {
    if (!Combo_IsCombinedGame())
        return 0;
    if (!ComboCrossOotRuntime::pending)
        return 0;
    ComboCrossOotRuntime::departureFadeComplete = true;
    return 1;
}
extern "C" void Combo_SOH_CancelCrossDungeonTransition(void) {
    ComboCrossOotRuntime::pending.reset();
    ComboCrossOotRuntime::departureFadeComplete = false;
}
extern "C" int32_t Combo_SOH_CrossDungeonGrottoId(void) {
    using namespace ComboCrossOotRuntime;
    if (!gPlayState || !state.Enabled())
        return -1;
    const auto* owner = state.FindEntrance(checkpoint.Saved());
    if (!owner || owner->pool != Pool::Grotto || owner->entrance.index < ENTRANCE_GROTTO_LOAD_START ||
        owner->entrance.index >= ENTRANCE_GROTTO_LOAD_START + NUM_GROTTOS ||
        !SceneMatchesEntrance(owner->entrance.index, gPlayState->sceneNum))
        return -1;
    return owner->entrance.index - ENTRANCE_GROTTO_LOAD_START;
}
extern "C" COMBO_EXPORT int SOH_SetCrossDungeonRuntimeSeed(const char* raw) try {
    if (!Combo_IsCombinedGame())
        return 0;
    auto seed = nlohmann::json::parse(raw ? raw : "null");
    const auto routes = ComboRando::CrossDungeon::Decode(seed);
    for (const auto& route : routes)
        for (auto endpoint : { route.from, route.to })
            if (endpoint.game == 0 && !ComboCrossOotRuntime::ValidEntrance(endpoint.index))
                throw std::invalid_argument("unknown OoT runtime entrance");
    if (!routes.empty())
        for (const auto& rawDungeon : seed.at("dungeons")) {
            const auto dungeon = ComboRando::CrossDungeon::ParseDungeon(rawDungeon);
            if (dungeon.pool == ComboRando::CrossDungeon::Pool::Boss && dungeon.entrance.game == 0 &&
                (!ComboCrossOotRuntime::ValidEntrance(dungeon.returnTo.index) ||
                 (dungeon.reverseCheckpoint &&
                  !ComboCrossOotRuntime::ValidEntrance(dungeon.reverseCheckpoint->index)) ||
                 (dungeon.deathCheckpoint && !ComboCrossOotRuntime::ValidEntrance(dungeon.deathCheckpoint->index))))
                throw std::invalid_argument("unknown OoT boss source recovery entrance");
        }
    ComboCrossOotRuntime::state.Install(seed);
    Combo_SOH_CancelCrossDungeonTransition();
    ComboCrossOotRuntime::pendingBossRecovery.reset();
    ComboCrossOotRuntime::localDungeonArrival = ComboCrossOotRuntime::localArrivalCheckpoint = -1;
    return 1;
} catch (const std::exception& e) {
    SPDLOG_ERROR("[CrossDungeon] OoT runtime seed rejected: {}", e.what());
    return 0;
}
extern "C" COMBO_EXPORT int Combo_SOH_CrossDungeonRuntimeEnabled(void) {
    return Combo_IsCombinedGame() && ComboCrossOotRuntime::state.Enabled();
}
extern "C" int Combo_SOH_CrossBossEnabled(void) {
    return Combo_IsCombinedGame() && gSaveContext.gameMode == GAMEMODE_NORMAL && gSaveContext.fileNum <= 2 &&
           ComboCrossOotRuntime::state.HasPool(ComboRando::CrossDungeon::Pool::Boss);
}
extern "C" int32_t Combo_SOH_CrossDungeonDeathCheckpoint(void) {
    using namespace ComboCrossOotRuntime;
    return RecoverBoss(checkpoint.Saved());
}
extern "C" void Combo_SOH_RestoreCrossDungeonCheckpoint(int32_t index) {
    using namespace ComboCrossOotRuntime;
    const bool known = state.Enabled() && state.IsRecoveryEntrance(index) && ValidEntrance(index);
    checkpoint.Arrive(known ? index : -1, known);
    pendingBossRecovery.reset();
    state.ClearBossRecovery();
    localDungeonArrival = localArrivalCheckpoint = -1;
}
extern "C" COMBO_EXPORT int SOH_QueueCrossDungeonArrival(int32_t index) {
    if (!Combo_IsCombinedGame())
        return 0;
    if (!ComboCrossOotRuntime::state.QueueArrival(index))
        return 0;
    ComboCrossOotRuntime::pendingBossRecovery.reset();
    return 1;
}
extern "C" COMBO_EXPORT int SOH_QueueCrossBossRecovery(int game, int32_t index) {
    if (!Combo_IsCombinedGame())
        return 0;
    using namespace ComboCrossOotRuntime;
    const auto recovery = state.RecoveryForBoss({ game, index });
    if (!recovery || recovery->source.game != 0 || !ValidEntrance(recovery->parent.index) ||
        !state.QueueBossRecovery({ game, index }))
        return 0;
    pendingBossRecovery.reset();
    return 1;
}
extern "C" int32_t Combo_SOH_ConsumeCrossBossRecoveryResume(void) {
    if (!Combo_IsCombinedGame())
        return -1;
    using namespace ComboCrossOotRuntime;
    const auto recovery = state.ConsumeBossRecovery();
    if (!recovery)
        return -1;
    checkpoint.Arrive(recovery->parent.index, true);
    localDungeonArrival = localArrivalCheckpoint = -1;
    return Materialize(recovery->parent.index);
}
extern "C" int32_t Combo_SOH_ConsumeCrossDungeonArrival(void) {
    if (!Combo_IsCombinedGame())
        return -1;
    const int32_t index = ComboCrossOotRuntime::state.ConsumeArrival();
    if (index >= 0) {
        const int32_t recovery = ComboCrossOotRuntime::state.RecoveryForArrival(index);
        ComboCrossOotRuntime::checkpoint.Arrive(recovery, recovery >= 0);
        ComboCrossOotRuntime::localDungeonArrival = ComboCrossOotRuntime::localArrivalCheckpoint = -1;
    }
    return index < 0 ? -1 : ComboCrossOotRuntime::Materialize(index);
}
// Called on each side of native lookup, never on a composed/queued arrival or death/save respawn.
extern "C" int32_t Combo_SOH_CrossDungeonRoute(int32_t index, int beforeNative) {
    if (!Combo_IsCombinedGame())
        return -1;
    using namespace ComboCrossOotRuntime;
    if (!gPlayState || gSaveContext.gameMode != GAMEMODE_NORMAL || gSaveContext.fileNum > 2)
        return -1;
    auto transition = state.Lookup(beforeNative ? Phase::BeforeNative : Phase::AfterNative, index);
    if (!transition)
        return -1;
    pendingBossRecovery.reset();
    if (transition->to.game == 0) {
        localDungeonArrival = transition->to.index;
        localArrivalCheckpoint =
            transition->arrivalCheckpoint
                ? (transition->arrivalCheckpoint->game == 0 ? transition->arrivalCheckpoint->index : -1)
                : state.RecoveryForArrival(transition->to.index);
        return transition->to.index;
    }
    pending = transition;
    departureFadeComplete = false;
    return index; // A native host index until the clean-frame handoff; never return a foreign index.
}
extern "C" int32_t Combo_SOH_SaveCrossDungeonCheckpoint(void) {
    return ComboCrossOotRuntime::checkpoint.Saved();
}
extern "C" void Combo_SOH_LoadCrossDungeonCheckpoint(int32_t index) {
    ComboCrossOotRuntime::checkpoint.Load(ComboCrossOotRuntime::ValidEntrance(index) ? index : -1);
    ComboCrossOotRuntime::localDungeonArrival = ComboCrossOotRuntime::localArrivalCheckpoint = -1;
    ComboCrossOotRuntime::pendingBossRecovery.reset();
}
extern "C" int32_t Combo_SOH_ConsumeCrossDungeonRecovery(void) {
    using namespace ComboCrossOotRuntime;
    // The native combo title path applies a queued frozen-source resume after load hooks.
    if (state.HasBossRecovery())
        return -1;
    const int32_t index = checkpoint.ConsumeRecovery([](int32_t entry) { return state.IsRecoveryEntrance(entry); });
    if (index < 0)
        return -1;
    const int32_t bossRecovery = RecoverBoss(index);
    return Materialize(bossRecovery >= 0 ? bossRecovery : index);
}

extern "C" void Combo_SOH_BeforeNativeLoadGame(void) {
    ComboCrossOotRuntime::nativeLoadDispatch = true;
}
// This runs after the complete native OnLoadGame dispatch; hook containers have no ordering contract.
extern "C" void Combo_SOH_AfterNativeLoadGame(void) {
    ComboCrossOotRuntime::nativeLoadDispatch = false;
    if (ComboCrossOotRuntime::state.Enabled()) {
        Entrance_ApplyCrossDungeonSafety();
        // Native-off contexts skipped the entrance savewarp branch while the slot was binding.
        if (!OTRGlobals::Instance->gRandoContext->GetOption(RSK_SHUFFLE_ENTRANCES))
            Entrance_SetSavewarpEntrance();
    }
    Combo_SOH_RestorePendingChecks();
    const int32_t checkpoint = Combo_SOH_ConsumeCrossDungeonRecovery();
    if (checkpoint >= 0) {
        gSaveContext.entranceIndex = checkpoint;
        gSaveContext.cutsceneIndex = 0;
    }
}
