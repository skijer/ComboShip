#ifdef COMBO_BUILD
#include "ComboForestSkullKid.h"
#include "ComboOpeningForest.h"
#include "ComboForestActors.h"
#include "entrances/ForestActorPlacements.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"
#include <cmath>

namespace {
bool sSpawnAttempted = false;

void SpawnForestSkullKid() {
    auto* play = gPlayState;
    if (sSpawnAttempted || !play || gGameState != &play->state || !play->state.running ||
        !ComboForestActors_IsExploration(play) || play->roomCtx.curRoom.num != 1 || play->roomCtx.status != 0 ||
        !play->roomCtx.curRoom.segment || play->transitionTrigger != TRANS_TRIGGER_OFF ||
        play->transitionMode != TRANS_MODE_OFF || play->pauseCtx.state != PAUSE_STATE_OFF ||
        play->csCtx.state != CS_STATE_IDLE || gSaveContext.cutsceneTrigger || play->soaringCsOrSoTCsPlaying ||
        Player_InCsMode(play))
        return;
    for (const auto& list : play->actorCtx.actorLists)
        for (Actor* actor = list.first; actor; actor = actor->next)
            if (actor->id == ACTOR_DM_STK && actor->params == COMBO_FOREST_SKULL_KID_PARAMS) {
                sSpawnAttempted = true;
                return;
            }
    sSpawnAttempted = true;
    // Use the marked raised ledge, facing the clearing. Sample its real floor
    // so optional appearance packs do not leave him at a stale ground height.
    using namespace ForestActorPlacements;
    Vec3f pos = { kSkullKidX, kSkullKidProbeY, kSkullKidZ };
    CollisionPoly* poly = nullptr;
    s32 bgId = BGCHECK_SCENE;
    const float floor = BgCheck_EntityRaycastFloor5(&play->colCtx, &poly, &bgId, nullptr, &pos);
    if (!poly || !std::isfinite(floor) || floor <= BGCHECK_Y_MIN)
        return;
    Actor_Spawn(&play->actorCtx, play, ACTOR_DM_STK, pos.x, floor + kSkullKidLift, pos.z, 0, kSkullKidYaw, 0,
                COMBO_FOREST_SKULL_KID_PARAMS);
}

void RegisterForestSkullKid() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](s16, s8) { sSpawnAttempted = false; });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnRoomInit>([](s16, s8) { sSpawnAttempted = false; });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>([] { sSpawnAttempted = false; });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameStateUpdate>(SpawnForestSkullKid);
}
static RegisterShipInitFunc init(RegisterForestSkullKid, {});
} // namespace
#endif
