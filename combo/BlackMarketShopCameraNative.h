// Included in each keeper controller's namespace, after the browsing globals.
// Camera ownership and pose calculation belong to the native host engine.
Actor* sBrowseKeeper = nullptr;

#ifdef BLACK_MARKET_BROWSING_OOT
CollisionHeader sShopCollision{};
CollisionHeader* sShopOriginalCollision = nullptr;
std::vector<CamData> sShopBgCameras;
std::array<Vec3s, 3> sShopBgData{};
s16 sShopBgIndex = -1, sShopOldViewpoint = 1, sShopOldRoomCamera = 0;

void InitBrowsingCameraScene(PlayState*) {
    sBrowseKeeper = nullptr;
    sShopOriginalCollision = nullptr;
    sShopBgCameras.clear();
    sShopBgIndex = -1;
}
bool BeginNativeBrowsingCamera(PlayState* play) {
    if (play->cameraPtrs[sBrowseParent]->childCamIdx >= CAM_ID_SUB_FIRST || !play->colCtx.colHeader)
        return false;
    auto* original = play->colCtx.colHeader;
    if (original->cameraDataListLen > 255 || (original->cameraDataListLen && !original->cameraDataList))
        return false;
    sBrowseCamera = Play_CreateSubCamera(play);
    if (sBrowseCamera < CAM_ID_SUB_FIRST)
        return false;
    sBrowseCameraPtr = Play_GetCamera(play, sBrowseCamera);
    sBrowseCameraUid = sBrowseCameraPtr->uid;
    sBrowseCameraPtr->parentCamIdx = sBrowseParent;
    sBrowseCameraPtr->childCamIdx = SUBCAM_FREE;
    // Bind the native Data4 player-height/at calculation. FREE0 had no such
    // binding and cannot implement the shop's scene camera contract.
    Camera_InitPlayerSettings(sBrowseCameraPtr, GET_PLAYER(play));
    sShopOriginalCollision = original;
    sShopCollision = *original;
    sShopBgCameras.clear();
    if (original->cameraDataListLen)
        sShopBgCameras.assign(original->cameraDataList, original->cameraDataList + original->cameraDataListLen);
    sShopBgIndex = 1;
    if (sShopBgCameras.size() < 2)
        sShopBgCameras.resize(2);
    sShopBgData = { Vec3s{ s16(BlackMarketBrowsing::kEye.x), s16(BlackMarketBrowsing::kEye.y),
                           s16(BlackMarketBrowsing::kEye.z) },
                    Vec3s{ -2367, -32768, 0 }, Vec3s{ 4600, 3, -1 } };
    sShopBgCameras[sShopBgIndex] = { CAM_SET_PIVOT_SHOP_BROWSING, 3, sShopBgData.data() };
    sShopCollision.cameraDataList = sShopBgCameras.data();
    sShopCollision.cameraDataListLen = sShopBgCameras.size();
    play->colCtx.colHeader = &sShopCollision;
    sShopOldViewpoint = play->unk_1242B;
    sShopOldRoomCamera = YREG(15);
    YREG(15) = 0x10;
    Play_SetShopBrowsingViewpoint(play);
    sBrowseCameraPtr->unk_14A &= ~0x40;
    Camera_ChangeDataIdx(sBrowseCameraPtr, sShopBgIndex);
    if (sBrowseCameraPtr->setting != CAM_SET_PIVOT_SHOP_BROWSING) {
        Play_ClearCamera(play, sBrowseCamera);
        play->colCtx.colHeader = original;
        play->unk_1242B = sShopOldViewpoint;
        YREG(15) = sShopOldRoomCamera;
        sShopOriginalCollision = nullptr;
        sBrowseCamera = sBrowseCameraUid = -1;
        sBrowseCameraPtr = nullptr;
        return false;
    }
    Play_ChangeCameraStatus(play, sBrowseParent, CAM_STAT_WAIT);
    Play_ChangeCameraStatus(play, sBrowseCamera, CAM_STAT_ACTIVE);
    return true;
}
bool OwnNativeBrowsingCamera(PlayState* play) {
    return sBrowseCamera >= CAM_ID_SUB_FIRST && Play_GetActiveCamId(play) == sBrowseCamera &&
           play->cameraPtrs[sBrowseCamera] == sBrowseCameraPtr && sBrowseCameraPtr->uid == sBrowseCameraUid;
}
void TickNativeBrowsingCamera(PlayState*) {
    // This is EnOssan_UpdateCameraDirection's native Data4 input. The scene
    // record fixes eye/pitch/roll; only the keeper-to-bank yaw changes.
    Camera_SetCameraData(sBrowseCameraPtr, 0xC, nullptr, nullptr, s16(-sBrowser.faceAngle), 0, 0);
    const s16 fov = std::lround(sBrowser.view.fov * 100);
    if (sShopBgData[2].x != fov) {
        sShopBgData[2].x = fov;
        if (sBrowseCameraPtr->animState != 0)
            sBrowseCameraPtr->animState = 20;
    }
}
void EndNativeBrowsingCamera(PlayState* play, bool destroying) {
    if (!destroying && play && sBrowseCamera >= CAM_ID_SUB_FIRST &&
        play->cameraPtrs[sBrowseCamera] == sBrowseCameraPtr && sBrowseCameraPtr->uid == sBrowseCameraUid) {
        const auto child = sBrowseCameraPtr->childCamIdx;
        const auto survivingChild =
            child >= CAM_ID_SUB_FIRST && child < NUM_CAMS && child != sBrowseCamera && play->cameraPtrs[child]
                ? child
                : SUBCAM_FREE;
        for (auto* camera : play->cameraPtrs) {
            if (!camera || camera == sBrowseCameraPtr)
                continue;
            if (camera->parentCamIdx == sBrowseCamera)
                camera->parentCamIdx = sBrowseParent;
            if (camera->childCamIdx == sBrowseCamera)
                camera->childCamIdx = survivingChild;
        }
        if (survivingChild >= CAM_ID_SUB_FIRST && play->cameraPtrs[sBrowseParent] &&
            play->cameraPtrs[sBrowseParent]->childCamIdx == SUBCAM_FREE)
            play->cameraPtrs[sBrowseParent]->childCamIdx = survivingChild;
        if (Play_GetActiveCamId(play) == sBrowseCamera && play->cameraPtrs[sBrowseParent])
            Play_ChangeCameraStatus(play, sBrowseParent, CAM_STAT_ACTIVE);
        Play_ClearCamera(play, sBrowseCamera);
    }
    if (!destroying && play && sShopOriginalCollision) {
        if (play->colCtx.colHeader == &sShopCollision)
            play->colCtx.colHeader = sShopOriginalCollision;
        play->unk_1242B = sShopOldViewpoint;
        YREG(15) = sShopOldRoomCamera;
    }
    sShopOriginalCollision = nullptr;
}
#else
std::array<std::array<Vec3s, 3>, 4> sShopCsData{};
std::array<ActorCsCamInfo, 4> sShopCsCameras{};
std::array<ActorCutscene, 4> sShopCutscenes{};
PlayState* sShopCameraScene = nullptr;
s16 sShopRequestedCs = CS_ID_NONE, sShopPlayingCs = CS_ID_NONE;

bool NativeBrowsingCameraCapacity(PlayState* play) {
    // Native CutsceneManager uses subCameras[2] to store its return snapshot.
    // Its Start path assumes allocation succeeds; never enter it without one
    // of the ordinary subcamera slots, including after a deferred queue wait.
    for (s16 id = CAM_ID_SUB_FIRST; id < NUM_CAMS - 1; ++id)
        if (!play->cameraPtrs[id])
            return true;
    return false;
}

void InitBrowsingCameraScene(PlayState* play) {
    sBrowseKeeper = nullptr;
    sShopCameraScene = play;
    sShopRequestedCs = sShopPlayingCs = CS_ID_NONE;
    if (!play)
        return;
    for (int i = 0; i < 4; ++i) {
        const s16 yaw = i == 1 ? s16(-27307) : i == 2 ? s16(27307) : s16(-32768);
        sShopCsData[i] = { Vec3s{ s16(BlackMarketBrowsing::kEye.x), s16(BlackMarketBrowsing::kEye.y),
                                  s16(BlackMarketBrowsing::kEye.z) },
                           Vec3s{ 2367, yaw, 0 }, Vec3s{ s16(i == 1 || i == 2 ? 7200 : 4600), 40, -1 } };
        sShopCsCameras[i] = { CAM_SET_FIXED1, 3, sShopCsData[i].data() };
        sShopCutscenes[i] = { 1,
                              -1,
                              s16(i),
                              CS_SCRIPT_ID_NONE,
                              s16(i == 3 ? CS_ID_NONE : i + 1),
                              CS_END_SFX_NONE,
                              255,
                              CS_HUD_VISIBILITY_ALL,
                              CS_END_CAM_0,
                              0 };
    }
    // This private interior has no donor actor cutscenes. Publish a real
    // keeper -> left -> right -> return chain before the first talk request.
    play->actorCsCamList = sShopCsCameras.data();
    CutsceneManager_Init(play, sShopCutscenes.data(), sShopCutscenes.size());
    CutsceneManager_SetCameraOwnershipProtection(true);
}
bool BeginNativeBrowsingCamera(PlayState* play) {
    if (sShopCameraScene != play || !sBrowseKeeper || !NativeBrowsingCameraCapacity(play))
        return false;
    const auto current = CutsceneManager_GetCurrentCsId();
    if (current != CS_ID_NONE && current != CS_ID_GLOBAL_TALK)
        return false;
    sShopRequestedCs = sShopPlayingCs = CS_ID_NONE;
    sBrowseCamera = sBrowseCameraUid = -1;
    sBrowseCameraPtr = nullptr;
    return true;
}
bool OwnNativeBrowsingCamera(PlayState* play) {
    const auto current = CutsceneManager_GetCurrentCsId();
    if (current == CS_ID_NONE || current == CS_ID_GLOBAL_TALK)
        return Play_GetActiveCamId(play) == sBrowseParent;
    return current == sShopPlayingCs && sBrowseCamera >= CAM_ID_SUB_FIRST &&
           Play_GetActiveCamId(play) == sBrowseCamera && play->cameraPtrs[sBrowseCamera] == sBrowseCameraPtr &&
           sBrowseCameraPtr->uid == sBrowseCameraUid;
}
void TickNativeBrowsingCamera(PlayState* play) {
    const auto left = CutsceneManager_GetAdditionalCsId(0);
    const auto right = CutsceneManager_GetAdditionalCsId(left);
    const auto back = CutsceneManager_GetAdditionalCsId(right);
    const s16 requested = sBrowser.slot < 0   ? (sShopRequestedCs == CS_ID_NONE ? 0 : back)
                          : sBrowser.slot < 4 ? right
                                              : left;
    // 0 and its linked return are the same keeper shot. Rows never restart a
    // cutscene; only native look-to-bank/look-to-keeper transitions can do so.
    if (sShopRequestedCs != requested && !(sBrowser.slot < 0 && sShopRequestedCs == 0)) {
        sShopRequestedCs = requested;
        if (sShopPlayingCs != CS_ID_NONE && CutsceneManager_GetCurrentCsId() == sShopPlayingCs)
            CutsceneManager_Stop(sShopPlayingCs);
    }
    if (sShopPlayingCs == sShopRequestedCs && CutsceneManager_GetCurrentCsId() == sShopPlayingCs)
        return;
    const auto current = CutsceneManager_GetCurrentCsId();
    if (current == CS_ID_GLOBAL_TALK)
        CutsceneManager_Stop(CS_ID_GLOBAL_TALK);
    if (current != CS_ID_NONE) {
        CutsceneManager_Queue(sShopRequestedCs);
        return;
    }
    if (!CutsceneManager_IsNext(sShopRequestedCs)) {
        CutsceneManager_Queue(sShopRequestedCs);
        return;
    }
    if (!NativeBrowsingCameraCapacity(play)) {
        CutsceneManager_Queue(sShopRequestedCs);
        return;
    }
    sShopCutscenes[sShopRequestedCs].endCam = CS_END_CAM_0;
    CutsceneManager_StartWithPlayerCsAndSetFlag(sShopRequestedCs, sBrowseKeeper);
    if (CutsceneManager_GetCurrentCsId() != sShopRequestedCs)
        return;
    sShopPlayingCs = sShopRequestedCs;
    sBrowseCamera = CutsceneManager_GetCurrentSubCamId(sShopPlayingCs);
    sBrowseCameraPtr = Play_GetCamera(play, sBrowseCamera);
    sBrowseCameraUid = sBrowseCameraPtr->uid;
}
void EndNativeBrowsingCamera(PlayState* play, bool destroying) {
    if (!destroying && sShopPlayingCs != CS_ID_NONE && CutsceneManager_GetCurrentCsId() == sShopPlayingCs) {
        CutsceneManager_Stop(sShopPlayingCs);
    }
    sShopRequestedCs = sShopPlayingCs = CS_ID_NONE;
}
#endif

bool NativeBrowsingCameraSettled(PlayState* play) {
    if (!OwnNativeBrowsingCamera(play) || sBrowseCamera < CAM_ID_SUB_FIRST || !sBrowser.CameraSettled())
        return false;
#ifndef BLACK_MARKET_BROWSING_OOT
    if (sShopRequestedCs != sShopPlayingCs || CutsceneManager_GetCurrentCsId() != sShopPlayingCs)
        return false;
#endif
    const auto expected = BlackMarketBrowsing::Target(sBrowser.slot);
    const auto* camera = sBrowseCameraPtr;
    const float x = camera->at.x - camera->eye.x, y = camera->at.y - camera->eye.y;
    const float z = camera->at.z - camera->eye.z;
    constexpr float degrees = 180.f / 3.14159265359f;
    return std::abs(camera->eye.x - expected.eye.x) < .25f && std::abs(camera->eye.y - expected.eye.y) < .25f &&
           std::abs(camera->eye.z - expected.eye.z) < .25f &&
           std::abs(std::atan2(x, -z) * degrees - BlackMarketBrowsing::BankAngle(sBrowser.slot)) < .25f &&
           std::abs(std::atan2(y, std::hypot(x, z)) * degrees - BlackMarketBrowsing::kPitchDegrees) < .25f &&
           std::abs(camera->fov - expected.fov) < .25f && camera->roll == 0;
}
