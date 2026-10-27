#ifdef COMBO_BUILD
#include "ComboForestArrivalCamera.h"
#include "global.h"
#include "2s2h/Enhancements/Gameplay/ComboOpeningForest.h"
#include "2s2h/Enhancements/Gameplay/ComboRegionalPassages.h"
#include "entrances/MmRegionalBridgePlacements.h"
#include "ComboGameMode.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"
#include <algorithm>
#include <cmath>

extern "C" s32 Camera_BgCheckInfo(Camera* camera, Vec3f* from, CameraCollision* to);
extern "C" s32 sCameraInterfaceFlags;
extern "C" void Player_Action_35(Player* player, PlayState* play);

namespace {
struct ArrivalPresentation {
    int route = -1;
    int slot = -1;
    u32 seed = 0;
    PlayState* play = nullptr;
    s16 cameraUid = -1;
    s16 setting = CAM_SET_NONE;
    int frames = 0;
    Vec3f playerPos{}, eye{}, at{};
    bool entryWalking = false;
    Vec3f lastEye{};
} sArrival;

void CancelPresentation() {
    sArrival = {};
}

bool MatchesDestination(PlayState* play, int route) {
    if (!play || gGameState != &play->state || !play->state.running || gSaveContext.gameMode != GAMEMODE_NORMAL ||
        !ComboLostWoods_IsPlayableSlot(gSaveContext.fileNum) || gSaveContext.sceneLayer != 0 ||
        gSaveContext.save.cutsceneIndex != 0 || gSaveContext.cutsceneTrigger || play->csCtx.state != CS_STATE_IDLE ||
        play->soaringCsOrSoTCsPlaying)
        return false;
    if (route == COMBO_PORTAL_OPENING_FOREST)
        return ComboOpeningForest_IsExploration(play) && play->roomCtx.curRoom.num == 0 && play->linkActorEntry;
    if (route == COMBO_PORTAL_SNOWHEAD) {
        const auto* placement = ComboMmRegional::ForScene(play->sceneId);
        const int entrance = play->sceneId == SCENE_10YUKIYAMANOMURA2 ? ENTRANCE(MOUNTAIN_VILLAGE_SPRING, 8)
                                                                      : ENTRANCE(MOUNTAIN_VILLAGE_WINTER, 8);
        return Combo_IsCombinedGame() && placement && placement->route == route && play->curSpawn == 8 &&
               gSaveContext.save.entrance == entrance && gSaveContext.respawnFlag == 0 &&
               gSaveContext.save.shipSaveInfo.pauseSaveEntrance == -1 && !gSaveContext.save.isOwlSave &&
               play->linkActorEntry && PLAYER_GET_START_MODE(play->linkActorEntry) == PLAYER_START_MODE_F &&
               ComboRegionalPassagesMm_Get(play, nullptr, nullptr, nullptr);
    }
    return route == COMBO_PORTAL_LOST_WOODS && play->sceneId == SCENE_26SARUNOMORI && play->roomCtx.curRoom.num == 1 &&
           gSaveContext.save.entrance == ENTRANCE(WOODS_OF_MYSTERY, 0);
}

float Distance(const Vec3f& a, const Vec3f& b) {
    const float x = a.x - b.x, y = a.y - b.y, z = a.z - b.z;
    return std::sqrt(x * x + y * y + z * z);
}

bool CollideEye(Camera* camera, const Vec3f& at, Vec3f& eye) {
    CameraCollision collision{};
    collision.pos = eye;
    Vec3f from = at;
    if (Camera_BgCheckInfo(camera, &from, &collision)) {
        const float distance = Distance(at, collision.pos);
        if (distance < 100.0f)
            return false; // Leave native placement intact if the reveal cannot fit.
        const float scale = (distance - 3.0f) / distance;
        eye = { at.x + (collision.pos.x - at.x) * scale, at.y + (collision.pos.y - at.y) * scale,
                at.z + (collision.pos.z - at.z) * scale };
    }
    return true;
}

void PreparePresentation(s8, s8 spawnNum) {
    const int route = sArrival.route;
    const int slot = sArrival.slot;
    const u32 seed = sArrival.seed;
    CancelPresentation(); // Every scene consumes the request, including mismatches.
    PlayState* play = gPlayState;
    if (spawnNum != (route == COMBO_PORTAL_SNOWHEAD ? 8 : 0) || slot != gSaveContext.fileNum ||
        seed != gSaveContext.save.shipSaveInfo.rando.finalSeed || !MatchesDestination(play, route))
        return;
    Player* player = GET_PLAYER(play);
    Camera* camera = &play->mainCamera;
    if (!player || camera->focalActor != &player->actor || camera->status != CAM_STATUS_ACTIVE ||
        play->activeCamId != CAM_ID_MAIN || camera->mode != CAM_MODE_NORMAL)
        return;

    Vec3f position = player->actor.world.pos;
    s16 yaw = -8192;
    CollisionPoly* floor = nullptr;
    s32 floorBg = BGCHECK_SCENE;
    const bool relocate = route == COMBO_PORTAL_OPENING_FOREST || route == COMBO_PORTAL_SNOWHEAD;
    if (relocate) {
        Vec3f anchor;
        float depth;
        if (route == COMBO_PORTAL_SNOWHEAD) {
            const auto& mouth = ComboMmRegional::kMountainWinter.mouth;
            yaw = mouth.yaw;
            anchor = { mouth.pos.x, mouth.pos.y, mouth.pos.z };
            depth = ComboMmRegional::kMountainPortalArrivalDepth;
        } else {
            yaw = ComboOpeningForest_ReturnYaw(play);
            anchor = ComboOpeningForest_ReturnAnchor(play);
            depth = 226.0f;
        }
        const float angle = yaw * (6.28318530718f / 65536.0f);
        // Forest keeps its proven Mystery reveal spacing. Snowhead uses its
        // separately sampled dry apron beyond the return trigger.
        position = { anchor.x + std::sin(angle) * depth, anchor.y, anchor.z + std::cos(angle) * depth };
        Vec3f probe = position;
        probe.y += 100.0f;
        const float floorY =
            BgCheck_EntityRaycastFloor5_3(play, &play->colCtx, &floor, &floorBg, &player->actor, &probe);
        if (floorY == BGCHECK_Y_MIN || !floor || floor->normal.y < 23170 || std::fabs(floorY - anchor.y) > 60.0f)
            return;
        position.y = floorY;
    }
    const float height = Player_GetHeight(player);
    const float angle = yaw * (6.28318530718f / 65536.0f);
    // The mountain apron faces a native cliff. A short native-FOV view
    // fits every form without driving the eye into that cliff.
    const float arm = route == COMBO_PORTAL_SNOWHEAD ? 160.0f : std::max(190.0f, height * 4.3f);
    const float atHeight = std::clamp(height * 1.05f, 46.0f, 80.0f);
    const Vec3f at{ position.x, position.y + atHeight, position.z };
    Vec3f eye{ position.x + std::sin(angle) * arm, at.y + std::max(20.0f, height * 0.25f),
               position.z + std::cos(angle) * arm };
    if (!CollideEye(camera, at, eye))
        return;

    // Preparation is transactional: floor and camera clearance both succeeded.
    // Only queued portal visits move Link; the scene spawn/cache/save stay intact.
    const bool entryWalking = player->actionFunc == Player_Action_35;
    if (relocate) {
        if (entryWalking) {
            const float turn = (s16)(yaw - player->actor.shape.rot.y) * (6.28318530718f / 65536.0f);
            const Vec3f delta{ player->unk_3A0.x - player->actor.world.pos.x,
                               player->unk_3A0.y - player->actor.world.pos.y,
                               player->unk_3A0.z - player->actor.world.pos.z };
            // Native E/D/F entry walking stores an absolute target. Carry its
            // distance and heading with the new arrival instead of walking back.
            player->unk_3A0 = { position.x + delta.x * std::cos(turn) + delta.z * std::sin(turn), position.y + delta.y,
                                position.z - delta.x * std::sin(turn) + delta.z * std::cos(turn) };
        }
        player->actor.world.pos = player->actor.prevPos = position;
        player->yaw = player->actor.world.rot.y = player->actor.shape.rot.y = yaw;
        player->actor.floorHeight = position.y;
        player->actor.floorPoly = floor;
        player->actor.floorBgId = floorBg;
        camera->focalActorPosRot = player->actor.world;
    }
    sArrival = { route, slot, seed, play, camera->uid, camera->setting, 0, position, eye, at };
    sArrival.entryWalking = entryWalking;
    sArrival.lastEye = eye;
    camera->at = at;
    camera->eye = camera->eyeNext = eye;
    camera->dist = Distance(at, eye);
    camera->focalActorAtOffset = { at.x - position.x, at.y - position.y, at.z - position.z };
    camera->fov = route == COMBO_PORTAL_SNOWHEAD ? 60.0f : 35.0f;
    camera->roll = 0;
}

bool PresentArrival(Camera* camera) {
    PlayState* play = sArrival.play;
    if (!play)
        return false;
    if (!MatchesDestination(play, sArrival.route) || gPlayState != play || gSaveContext.fileNum != sArrival.slot ||
        gSaveContext.save.shipSaveInfo.rando.finalSeed != sArrival.seed || camera != &play->mainCamera ||
        camera->uid != sArrival.cameraUid || play->activeCamId != CAM_ID_MAIN || camera->setting != sArrival.setting ||
        camera->status != CAM_STATUS_ACTIVE || camera->mode != CAM_MODE_NORMAL ||
        play->pauseCtx.state != PAUSE_STATE_OFF || play->gameOverCtx.state != GAMEOVER_INACTIVE ||
        play->transitionTrigger == TRANS_TRIGGER_START) {
        CancelPresentation();
        return false;
    }
    const Input& input = play->state.input[0];
    const Player* player = GET_PLAYER(play);
    sArrival.entryWalking = sArrival.entryWalking && player && player->actionFunc == Player_Action_35;
    if (!player || std::abs(input.cur.stick_x) > 10 || std::abs(input.cur.stick_y) > 10 ||
        std::abs(input.cur.right_stick_x) > 1 || std::abs(input.cur.right_stick_y) > 1 || input.cur.button ||
        Distance(camera->eye, sArrival.lastEye) > 0.1f || Distance(camera->at, sArrival.at) > 0.1f ||
        std::fabs(camera->fov - (sArrival.route == COMBO_PORTAL_SNOWHEAD ? 60.0f : 35.0f)) > 0.01f ||
        (!sArrival.entryWalking && Distance(player->actor.world.pos, sArrival.playerPos) > 4.0f) ||
        (Player_InCsMode(play) && !sArrival.entryWalking && play->transitionTrigger != TRANS_TRIGGER_END) ||
        (!sArrival.entryWalking && play->transitionTrigger == TRANS_TRIGGER_OFF && ++sArrival.frames > 60)) {
        // Leave eye/at, rates and native handler state exactly where they are.
        // Native follow/free-look resumes from this view instead of resetting behind Link.
        CancelPresentation();
        return false;
    }
    if (sArrival.entryWalking) {
        const Vec3f delta{ player->actor.world.pos.x - sArrival.playerPos.x,
                           player->actor.world.pos.y - sArrival.playerPos.y,
                           player->actor.world.pos.z - sArrival.playerPos.z };
        sArrival.at.x += delta.x;
        sArrival.at.y += delta.y;
        sArrival.at.z += delta.z;
        sArrival.eye.x += delta.x;
        sArrival.eye.y += delta.y;
        sArrival.eye.z += delta.z;
        sArrival.playerPos = player->actor.world.pos;
    }
    Vec3f eye = sArrival.eye;
    if (!CollideEye(camera, sArrival.at, eye)) {
        CancelPresentation();
        return false;
    }
    camera->at = sArrival.at;
    camera->eye = camera->eyeNext = eye;
    sArrival.lastEye = eye;
    camera->dist = Distance(camera->at, eye);
    sCameraInterfaceFlags = CAM_INTERFACE_FLAGS(CAM_LETTERBOX_NONE, CAM_HUD_VISIBILITY_ALL, 0);
    return true;
}

void RegisterForestArrivalCamera() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>(PreparePresentation);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>(CancelPresentation);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSaveLoad>([](s16) { CancelPresentation(); });
    GameInteractor::Instance->RegisterGameHookForID<GameInteractor::ShouldVanillaBehavior>(
        VB_USE_CUSTOM_CAMERA, [](GIVanillaBehavior, bool* should, va_list args) {
            if (!*should) {
                CancelPresentation();
                return;
            }
            va_list copy;
            va_copy(copy, args);
            Camera* camera = va_arg(copy, Camera*);
            va_end(copy);
            if (PresentArrival(camera))
                *should = false;
        });
}
static RegisterShipInitFunc initFunc(RegisterForestArrivalCamera, {});
} // namespace

extern "C" void ComboForestArrivalCamera_Queue(int route) {
    CancelPresentation();
    if ((!ComboLostWoods_IsForestRoute(route) && route != COMBO_PORTAL_SNOWHEAD) ||
        !ComboLostWoods_IsPlayableSlot(gSaveContext.fileNum))
        return;
    sArrival.route = route;
    sArrival.slot = gSaveContext.fileNum;
    sArrival.seed = gSaveContext.save.shipSaveInfo.rando.finalSeed;
}
#endif
