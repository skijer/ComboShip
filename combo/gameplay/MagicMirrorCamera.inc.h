#pragma once

// Include after the native engine headers. The main camera's framing is preserved:
// only this cast's allocated subcamera and generation may be released.
namespace ComboMagicMirrorCamera {
static s16 sId = -1, sUid = 0;
static Camera* sCamera = nullptr;
static bool sAttempted = false;

static void Forget() {
    sId = -1;
    sUid = 0;
    sCamera = nullptr;
    sAttempted = false;
}

static void Release(PlayState* play) {
    if (play && sId >= CAM_ID_SUB_FIRST && sId < NUM_CAMS && play->cameraPtrs[sId] == sCamera && sCamera &&
        sCamera->uid == sUid) {
        // A different live subcamera belongs to its own cutscene. Leave it
        // active while discarding our now-inactive allocation.
        if (Play_GetActiveCamId(play) == sId && play->cameraPtrs[CAM_ID_MAIN]) {
#ifdef COMBO_MAGIC_MIRROR_MM
            Play_ChangeCameraStatus(play, CAM_ID_MAIN, CAM_STATUS_ACTIVE);
#else
            Play_ChangeCameraStatus(play, CAM_ID_MAIN, CAM_STAT_ACTIVE);
#endif
        }
        Play_ClearCamera(play, sId);
    }
    Forget();
}

static void Begin(PlayState* play, Player* player) {
    if (sAttempted)
        return;
    sAttempted = true; // Failure must not steal a camera on a later frame.
    if (!play || !player || Play_GetActiveCamId(play) != CAM_ID_MAIN || !play->cameraPtrs[CAM_ID_MAIN])
        return;
#ifdef COMBO_MAGIC_MIRROR_MM
    // The native manager otherwise uses the last subcamera as snapshot storage
    // and can retain this camera as a cutscene return target. Keep its ownership
    // guard enabled for this scene, including after an interrupted cast releases
    // our camera; the manager may still need to reject that stale return target.
    CutsceneManager_SetCameraOwnershipProtection(true);
#endif
    const s16 id = Play_CreateSubCamera(play);
    if (id < CAM_ID_SUB_FIRST || id >= NUM_CAMS)
        return;
    sId = id;
    sCamera = play->cameraPtrs[id];
    if (!sCamera) {
        Forget();
        return;
    }
    sUid = sCamera->uid;
#ifdef COMBO_MAGIC_MIRROR_MM
    Camera_InitFocalActorSettings(sCamera, &player->actor);
#else
    Camera_InitPlayerSettings(sCamera, player);
#endif
    Play_CopyCamera(play, id, CAM_ID_MAIN);
#ifdef COMBO_MAGIC_MIRROR_MM
    Camera_ChangeSetting(sCamera, CAM_SET_MAGIC_MIRROR);
    constexpr s16 setting = CAM_SET_MAGIC_MIRROR;
#else
    Camera_RequestSetting(sCamera, CAM_SET_TURN_AROUND);
    Camera_SetCameraData(sCamera, 4, nullptr, nullptr, 10, 0, 0);
    constexpr s16 setting = CAM_SET_TURN_AROUND;
#endif
    if (sCamera->setting != setting) {
        Release(play);
        sAttempted = true;
        return;
    }
#ifdef COMBO_MAGIC_MIRROR_MM
    Play_ChangeCameraStatus(play, CAM_ID_MAIN, CAM_STATUS_WAIT);
    Play_ChangeCameraStatus(play, id, CAM_STATUS_ACTIVE);
#else
    Play_ChangeCameraStatus(play, CAM_ID_MAIN, CAM_STAT_WAIT);
    Play_ChangeCameraStatus(play, id, CAM_STAT_ACTIVE);
#endif
}
} // namespace ComboMagicMirrorCamera
