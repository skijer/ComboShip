#ifdef COMBO_BUILD
#include "ComboForestActors.h"
#include "entrances/SoloForest.h"
#include "entrances/ForestActorPlacements.h"
#include "global.h"
#include "actors/ForestGiParticles.h"
#include "soh/Enhancements/randomizer/NeiGiRender.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/custom-message/CustomMessageManager.h"
#include "soh/ShipInit.hpp"
#include <cmath>

extern "C" void ComboForestActors_DrawSariaParticles(Actor* actor, PlayState* play, int seated) {
    if (!actor || !play || !play->state.gfxCtx || (gComboGameMode != COMBO_GAME_OOT && !Combo_IsCombinedGame()) ||
        play->pauseCtx.state != 0 || actor->xzDistToPlayer > 700.0f || std::fabs(actor->yDistToPlayer) > 300.0f)
        return;
    Matrix_Push();
    Matrix_Translate(actor->world.pos.x, actor->world.pos.y, actor->world.pos.z, MTXMODE_NEW);
    const auto mesh = ForestGiParticles::Saria(play->gameplayFrames, seated != 0, NeiGi_CameraBasis(play));
    NeiGi_DrawWorldParticles(play, mesh);
    Matrix_Pop();
}

namespace {
ComboForestActors::Visit sVisit;
bool ForestVisit(PlayState* play) {
    return play &&
           ComboSoloForest_IsOotVisit(play->sceneNum, play->roomCtx.curRoom.num, gSaveContext.gameMode,
                                      gSaveContext.fileNum, gSaveContext.sceneLayer, gSaveContext.cutsceneIndex);
}
bool Safe(PlayState* play) {
    return ForestVisit(play) && play->state.running && play->transitionTrigger == TRANS_TRIGGER_OFF &&
           play->transitionMode == TRANS_MODE_OFF && play->pauseCtx.state == 0 &&
           play->gameOverCtx.state == GAMEOVER_INACTIVE && play->csCtx.state == CS_STATE_IDLE &&
           !gSaveContext.cutsceneTrigger && !Player_InCsMode(play);
}
Actor* Find(PlayState* play, int id, int params) {
    for (const auto& list : play->actorCtx.actorLists)
        for (Actor* actor = list.head; actor; actor = actor->next)
            if (actor->id == id && actor->params == params && actor->update &&
                (actor->room == play->roomCtx.curRoom.num || actor->room == -1))
                return actor;
    return nullptr;
}
bool Spawn(PlayState* play, int id, int params, float x, float z, float lift, s16 yaw) {
    auto* player = GET_PLAYER(play);
    if (!player)
        return false;
    // Stay below the upper forest rooms when sampling this room's floor.
    Vec3f probe = { x, play->roomCtx.curRoom.num == 0 ? 120.0f : 500.0f, z };
    CollisionPoly* floor = nullptr;
    s32 bg;
    float y = BgCheck_EntityRaycastFloor5(play, &play->colCtx, &floor, &bg, &player->actor, &probe);
    if (!floor || !std::isfinite(y) || y <= BGCHECK_Y_MIN)
        return false;
    return Actor_Spawn(&play->actorCtx, play, id, x, y + lift, z, 0, yaw, 0, params) != nullptr;
}
void Update() {
    auto* play = gPlayState;
    if (!ForestVisit(play)) {
        sVisit.Reset();
        return;
    }
    if (!Safe(play) || !sVisit.Ready(play->roomCtx.curRoom.num, play->roomCtx.curRoom.segment, play->roomCtx.status))
        return;
    using namespace ComboForestActors;
    auto ensure = [&](unsigned bit, int id, int params, float x, float z, float lift, s16 yaw) {
        sVisit.Ensure(
            bit, [&] { return Find(play, id, params) != nullptr; },
            [&] { return Spawn(play, id, params, x, z, lift, yaw); });
    };
    if (play->roomCtx.curRoom.num == 0) {
        const auto side = ForestActorPlacements::SariaOffset(COMBO_SOLO_FOREST_STUMP_YAW);
        ensure(1, kViewer, kSaria, COMBO_SOLO_FOREST_STUMP_X + side.x, COMBO_SOLO_FOREST_STUMP_Z + side.z, 0,
               COMBO_SOLO_FOREST_STUMP_YAW);
        const float angle = COMBO_SOLO_FOREST_STUMP_YAW * (6.28318530718f / 65536.0f);
        const float sine = std::sin(angle), cosine = std::cos(angle);
        auto* beacon = Find(play, kViewer, kSaria);
        const float dx = beacon ? beacon->world.pos.x - COMBO_SOLO_FOREST_STUMP_X : side.x;
        const float dz = beacon ? beacon->world.pos.z - COMBO_SOLO_FOREST_STUMP_Z : side.z;
        const float lateral =
            dx * cosine - dz * sine <= 0 ? ForestActorPlacements::kHorseLateral : -ForestActorPlacements::kHorseLateral;
        ensure(2, ACTOR_EN_HORSE, kHorse,
               COMBO_SOLO_FOREST_STUMP_X + lateral * cosine + ForestActorPlacements::kFlankDepth * sine,
               COMBO_SOLO_FOREST_STUMP_Z - lateral * sine + ForestActorPlacements::kFlankDepth * cosine, 0,
               COMBO_SOLO_FOREST_STUMP_YAW);
    } else {
        ensure(4, kViewer, kSkullKid, ForestActorPlacements::kSkullKidX, ForestActorPlacements::kSkullKidZ,
               ForestActorPlacements::kSkullKidLift, ForestActorPlacements::kSkullKidYaw);
    }
}
void OpenText(uint16_t*, bool* loadFromTable) {
    auto* play = gPlayState;
    // The viewer sets textId on each update. Intercept only the actual forest speaker.
    if (!ForestVisit(play) || play->roomCtx.curRoom.num != 0 || !loadFromTable)
        return;
    auto* player = GET_PLAYER(play);
    auto* speaker = player ? player->talkActor : nullptr;
    if (!speaker || speaker != Find(play, ComboForestActors::kViewer, ComboForestActors::kSaria))
        return;
    CustomMessage message("These woods are a bridge\nbetween worlds...\nThe forest will guide you home.");
    message.AutoFormat();
    message.LoadIntoFont();
    *loadFromTable = false;
}
void Register() {
    if (gComboGameMode != COMBO_GAME_OOT)
        return;
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerUpdate>(Update);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](s16) { sVisit.Reset(); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>([] { sVisit.Reset(); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnOpenText>(OpenText);
}
RegisterShipInitFunc init(Register);
} // namespace
#endif
