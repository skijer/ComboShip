// Real native camera structures and production ownership logic; pool/service
// calls are controlled so allocation/reuse/interruption can be exercised.
#include "global.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#ifdef COMBO_MAGIC_MIRROR_MM
#include "native_mirror_camera_table.inc.h"
#endif
static PlayState play;
static Player player;
static Camera mainCamera;
static int allocations, clears, changes;
static s16 nextUid;
static bool full, reject;
extern "C" s16 Play_GetActiveCamId(PlayState* p) {
#ifdef COMBO_MAGIC_MIRROR_MM
    return p->activeCamId;
#else
    return p->activeCamera;
#endif
}
extern "C" s16 Play_CreateSubCamera(PlayState* p) {
    ++allocations;
    if (full) return -1;
    for (s16 id = CAM_ID_SUB_FIRST; id < NUM_CAMS; ++id) {
        if (p->cameraPtrs[id])
            continue;
        p->cameraPtrs[id] = &p->subCameras[id - 1];
        *p->cameraPtrs[id] = {};
        p->cameraPtrs[id]->uid = ++nextUid;
#ifdef COMBO_MAGIC_MIRROR_MM
        p->cameraPtrs[id]->camId = id;
#endif
        return id;
    }
    return -1;
}
extern "C" void Play_ClearCamera(PlayState* p, s16 id) {
    assert(id > 0 && id < NUM_CAMS);
    ++clears;
    p->cameraPtrs[id] = nullptr;
}
#ifdef COMBO_MAGIC_MIRROR_MM
extern "C" s32 Play_ChangeCameraStatus(PlayState* p, s16 id, s16 status) {
    if (status == CAM_STATUS_ACTIVE) p->activeCamId = id;
#else
extern "C" s16 Play_ChangeCameraStatus(PlayState* p, s16 id, s16 status) {
    if (status == CAM_STAT_ACTIVE) p->activeCamera = id;
#endif
    ++changes;
    p->cameraPtrs[id]->status = status;
    return status;
}
extern "C" void Play_CopyCamera(PlayState* p, s16 dest, s16 src) {
    assert(p->cameraPtrs[dest] && p->cameraPtrs[src] && "Native return copy cannot target a released Mirror");
    p->cameraPtrs[dest]->eye = p->cameraPtrs[src]->eye;
    p->cameraPtrs[dest]->at = p->cameraPtrs[src]->at;
    p->cameraPtrs[dest]->fov = p->cameraPtrs[src]->fov;
}
#ifdef COMBO_MAGIC_MIRROR_MM
extern "C" void Camera_InitFocalActorSettings(Camera* c, Actor* a) { c->focalActor = a; }
extern "C" s32 Camera_ChangeSetting(Camera* c, s16 setting) {
    assert((setting == CAM_SET_MAGIC_MIRROR || setting == CAM_SET_FREE0) && setting != CAM_SET_NONE);
    if (!reject) c->setting = setting;
    return !reject;
}
#else
extern "C" void Camera_InitPlayerSettings(Camera* c, Player* p) { c->player = p; }
extern "C" s32 Camera_RequestSetting(Camera* c, s16 setting) {
    assert(setting == CAM_SET_TURN_AROUND);
    if (!reject) c->setting = setting;
    return !reject;
}
extern "C" void Camera_SetCameraData(Camera* c, s16 flags, void*, void*, s16 kind, s16, UNK_TYPE) {
    assert(flags == 4 && kind == 10);
    c->data2 = kind;
}
#endif
#ifdef COMBO_MAGIC_MIRROR_MM
PlayState* gPlayState = &play;
extern "C" Camera* Play_GetCamera(PlayState* p, s16 id) {
    return p->cameraPtrs[id == CAM_ID_NONE ? Play_GetActiveCamId(p) : id];
}
extern "C" s32 Camera_ChangeStatus(Camera* c, s16 status) { c->status = status; return status; }
extern "C" s16 Camera_UnsetStateFlag(Camera* c, s16 flags) { c->stateFlags &= ~flags; return c->stateFlags; }
extern "C" s32 Camera_ChangeActorCsCamIndex(Camera*, s32) { return 1; }
extern "C" void func_800E0348(Camera*) {}
extern "C" void Cutscene_StartScripted(PlayState*, u8) {}
extern "C" s32 Player_SetCsActionWithHaltedActors(PlayState*, Actor*, u8) { return 1; }
extern "C" void Audio_PlaySfx(u16) {}
extern "C" void Interface_SetHudVisibility(u16) {}
extern "C" void ShrinkWindow_Letterbox_SetSizeTarget(s32) {}
#define GameInteractor_Should(...) true
#define ComboOpeningForest_IsExploration(...) false
#include "native_mirror_manager.inc.h"
#undef GameInteractor_Should
#undef ComboOpeningForest_IsExploration
static ActorCutscene sceneCutscene{0, -1, CS_CAM_ID_NONE, CS_SCRIPT_ID_NONE, CS_ID_NONE, CS_END_SFX_NONE, 255,
                                  CS_HUD_VISIBILITY_ALL_ALT, CS_END_CAM_0, 0};
#endif
#include "gameplay/MagicMirrorCamera.inc.h"
static void Reset() {
    ComboMagicMirrorCamera::Forget();
    play = {}; player = {}; mainCamera = {};
    mainCamera.eye = {1, 2, 3}; mainCamera.at = {4, 5, 6};
    mainCamera.fov = 55; mainCamera.setting = CAM_SET_NORMAL0;
    mainCamera.status = 7;
    play.cameraPtrs[0] = &mainCamera;
    allocations = clears = changes = 0; full = reject = false;
#ifdef COMBO_MAGIC_MIRROR_MM
    CutsceneManager_Init(&play, &sceneCutscene, 1);
    CutsceneManager_SetReturnCamera(CAM_ID_MAIN);
#endif
}
int main() {
#ifdef COMBO_MAGIC_MIRROR_MM
    assert(mirrorSetting.validModes == 0x2F);
    for (int mode : {0,1,2,3,5}) {
        assert(mirrorSetting.cameraModes[mode].funcId == CAM_FUNC_KEEPON4);
        assert(mirrorSetting.cameraModes[mode].numValues == 9);
        auto* values = mirrorSetting.cameraModes[mode].values;
        assert(values[0].val == -50 && values[1].val == 150 && values[2].val == -15);
        assert(values[3].val == 175 && values[5].val == 70 && values[8].val == 60);
    }
#endif
    using namespace ComboMagicMirrorCamera;
#ifdef COMBO_MAGIC_MIRROR_MM
    // Native idle StoreCamera uses final slot storage as its return snapshot.
    Reset();
    Camera first{}, second{};
    play.cameraPtrs[1] = &first;
    play.cameraPtrs[2] = &second;
    Begin(&play, &player);
    assert(sId == NUM_CAMS - 1);
    const s16 mirrorUid = sCamera->uid;
    CutsceneManager_Update();
    assert(sCamera->uid == mirrorUid && sCamera->status == CAM_STATUS_ACTIVE &&
           "Native idle snapshot must not overwrite the active Mirror camera");
    Release(&play);
    assert(!play.cameraPtrs[NUM_CAMS - 1]);
    // A native cutscene borrows the active Mirror as its return camera.
    Reset();
    Begin(&play, &player);
    assert(CutsceneManager_Start(0, &player.actor) == 0 && sCutsceneMgr.retCamId == sId);
    Release(&play);
    CutsceneManager_End();
    assert(Play_GetActiveCamId(&play) == CAM_ID_MAIN && !play.cameraPtrs[1] && !play.cameraPtrs[2]);
    assert(sCutsceneMgr.retCamId == CAM_ID_MAIN);
    CutsceneManager_Update(); // Subsequent idle manager update remains safe.
#endif
    Reset();
    const auto mainBefore = mainCamera;
    Begin(&play, &player);
    assert(allocations == 1 && Play_GetActiveCamId(&play) == 1);
    assert(mainCamera.setting == mainBefore.setting && mainCamera.fov == mainBefore.fov);
    assert(mainCamera.eye.x == 1 && mainCamera.at.y == 5);
    Begin(&play, &player); assert(allocations == 1);
    Release(&play);
    assert(clears == 1 && Play_GetActiveCamId(&play) == 0 && mainCamera.status == 7);
    assert(mainCamera.setting == mainBefore.setting && mainCamera.eye.z == 3);
    Release(&play); assert(clears == 1);
    Reset(); full = true; Begin(&play, &player); full = false; Begin(&play, &player);
    assert(allocations == 1 && changes == 0); Release(&play);
    Reset(); reject = true; Begin(&play, &player);
    assert(clears == 1 && changes == 0 && Play_GetActiveCamId(&play) == 0);
    Begin(&play, &player); assert(allocations == 1);
    Reset(); Begin(&play, &player); ++play.cameraPtrs[1]->uid;
    Release(&play); assert(clears == 0 && Play_GetActiveCamId(&play) == 1);
    Reset(); Begin(&play, &player);
    Camera other{}; other.uid = 999; play.cameraPtrs[2] = &other;
    Play_ChangeCameraStatus(&play, 2, 7); const int prior = changes;
    Release(&play); assert(clears == 1 && Play_GetActiveCamId(&play) == 2 && changes == prior);
    Reset(); Begin(&play, &player); const auto copy = play;
    Forget(); Release(nullptr);
    assert(std::memcmp(&play, &copy, sizeof(play)) == 0 && clears == 0);
    Reset(); Play_ChangeCameraStatus(&play, 0, 7);
    Camera busy{}; play.cameraPtrs[2] = &busy; Play_ChangeCameraStatus(&play, 2, 7);
    Begin(&play, &player); assert(allocations == 0);
    std::puts("PASS: native Mirror subcamera acquisition, main preservation, failure, reuse, competing owner and post-arena forget");
}
