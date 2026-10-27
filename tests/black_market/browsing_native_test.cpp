#include "global.h"
#include "overlays/actors/ovl_En_Ossan/z_en_ossan.h"
#include <libultraship/bridge/consolevariablebridge.h>
#ifndef OOT_BROWSING_TEST
#include "overlays/actors/ovl_En_Takaraya/z_en_takaraya.h"
#endif
#include <cassert>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <string>
#include <array>
#include <vector>
#include "combo/BlackMarketEntry.h"
#include "combo/BlackMarketShelves.h"
#include "combo/BlackMarketGraphicsArena.h"
#include "combo/BlackMarketReceipt.h"
BlackMarketReceipt::State sReceipt;
#if __has_include("combo/BlackMarketBrowsing.h")
#include "combo/BlackMarketBrowsing.h"
BlackMarketBrowsing::State sBrowser;
#endif
PlayState* gPlayState;
static PlayState play;
static Player player;
static Camera mainCamera, shopCamera;
static GraphicsContext graphics;
static std::array<Gfx, 128> commands;
alignas(16) static std::array<Gfx, 64> opaqueCommands;
static std::array<Gfx, 64> translucentCommands;
static CollisionHeader originalCollision{};
static std::array<Vec3s, 3> originalCameraData{};
static CamData originalCameraRecord{ CAM_SET_FREE0, 3, originalCameraData.data() };
static bool mirroredWorld;
static int cursorSetups;
extern "C" int32_t CVarGetInteger(const char* name, int32_t fallback) {
#ifdef OOT_BROWSING_TEST
    if (std::string(name) == CVAR_SETTING("FreeLook.Enabled") ||
        std::string(name) == CVAR_ENHANCEMENT("FixCameraDrift")) return 0;
    assert(std::string(name) == CVAR_ENHANCEMENT("MirroredWorld"));
#else
    assert(std::string(name) == "gModes.MirroredWorld.State");
#endif
    assert(fallback == 0);
    return mirroredWorld;
}
static bool active = true, accepted = true, advance;
static int state = TEXT_STATE_EVENT, purchases, allocations, cameraWrites, lastText, allocationId = 1, closes;
static int cursorSounds;
static bool purchaseAccepted;
#ifdef OOT_BROWSING_TEST
void Sfx_PlaySfxCentered(u16 id) { assert(id == NA_SE_SY_CURSOR); ++cursorSounds; }
#else
void Audio_PlaySfx(u16 id) { assert(id == NA_SE_SY_CURSOR); ++cursorSounds; }
#endif
static int sSelected;
static bool sTalking;
static s16 sBrowseCamera = -1, sBrowseParent = -1;
static s16 sBrowseCameraUid;
static Camera* sBrowseCameraPtr;
static bool sBrowsePlayerWasHidden;
static bool sBrowseTextboxSaved;
static std::array<s16, 6> sBrowseTextboxLayout{};
static int currentCs = -1, currentSub = -1, returnCamera;
struct KeeperState { SkelAnime skel; int timer = 90, eye; } sKeeper;
bool Active() { return active; }
bool Interior() { return active; }
// Observe the queued-GI boundary; native transaction fixtures own item/rupee delivery.
bool Purchase() {
    ++purchases;
    return purchaseAccepted && sReceipt.Begin(sSelected, 0, 0, 0);
}
int GetState(MessageContext*) { return state; }
bool ShouldAdvance(PlayState*) { return advance; }
bool Accept(Actor*, PlayState*) { bool result = accepted; accepted = false; return result; }
void Continue(PlayState*, u16 text) {
    lastText = text;
#ifdef OOT_BROWSING_TEST
    play.msgCtx.textId = text;
#else
    play.msgCtx.currentTextId = text;
#endif
    state = (text == 0x8F13 || text == 0x8F17) ? TEXT_STATE_CHOICE : TEXT_STATE_EVENT;
}
void Close(PlayState*) { state = TEXT_STATE_CLOSING; ++closes; }
void NativeCameraInit(Camera *, View *, CollisionContext *, PlayState *);
void InitNativeCameraTable();
s16 NativeStatus(PlayState*, s16, s16);
void NativeClearCamera(PlayState*, s16);
int CreateCamera(PlayState*) {
    ++allocations;
    if (allocationId < 0) return allocationId;
    InitNativeCameraTable();
    NativeCameraInit(&shopCamera, &play.view, &play.colCtx, &play);
    shopCamera.thisIdx = 1;
    play.cameraPtrs[1] = &shopCamera; return 1;
}
int ActiveCamera(PlayState*) {
#ifdef OOT_BROWSING_TEST
    return play.activeCamera;
#else
    return play.activeCamId;
#endif
}
void CameraStatus(PlayState* state, int id, int status) { NativeStatus(state, id, status); }
#define GameInteractor_Should(...) true
#define GameInteractor_InvertControl(...) 1
#define Play_GetCamera(play, id) ((play)->cameraPtrs[id])
#include "soh/soh/Enhancements/Gameplay/ComboBlackMarketNative.h"
/* NATIVE CAMERA */
static int nativeCameraFrames, nativeSettingRequests;
static int manualCameraSetters;
int SetCameraSetting(PlayState*, s16, s16) {
    ++manualCameraSetters; assert(false && "shop must enter native Data4 through scene camera data"); return 0;
}
void SetView(PlayState*, int, Vec3f*, Vec3f*) {
    ++manualCameraSetters; assert(false && "shop must not supply manual at/eye");
}
void SetFov(PlayState*, int, float) {
    ++manualCameraSetters; assert(false && "shop must not supply manual FOV");
}
s32 BrowseCameraDataIdx(Camera* camera, s32 dataIdx) {
    ++nativeSettingRequests;
    assert(dataIdx == 1 && camera->player == &player);
    const auto result = Camera_ChangeDataIdx(camera, dataIdx);
    assert(camera->camDataIdx == 1 && camera->setting == CAM_SET_PIVOT_SHOP_BROWSING);
    return result;
}
void BrowseCameraDirection(Camera* camera, s16 flags, void* data0, void* data1, s16 yaw, s16 pitch, UNK_TYPE extra) {
    ++cameraWrites;
    assert(camera == &shopCamera && flags == 0xC && !data0 && !data1 && pitch == 0 && extra == 0);
    Camera_SetCameraData(camera, flags, data0, data1, yaw, pitch, extra);
}
#define Camera_ChangeDataIdx BrowseCameraDataIdx
#define Camera_SetCameraData BrowseCameraDirection
#define Message_GetState GetState
#define Message_ShouldAdvance ShouldAdvance
#define Message_ContinueTextbox Continue
#define Message_CloseTextbox Close
#define Actor_ProcessTalkRequest Accept
#define Actor_TalkOfferAccepted(actor, state) Accept(actor, gPlayState)
#define Actor_OfferTalk(...) ((void)0)
#define Actor_SetFocus(...) ((void)0)
#define SkelAnime_Update(...) ((void)0)
#define Play_CreateSubCamera CreateCamera
#define Play_GetActiveCamId ActiveCamera
#define Play_ChangeCameraStatus CameraStatus
#define Play_GetCamera(play,id) ((play)->cameraPtrs[id])
#define Play_ClearCamera NativeClearCamera
#define Play_ChangeCameraSetting SetCameraSetting
#define Play_CameraChangeSetting SetCameraSetting
#define Play_SetCameraAtEye SetView
#define Play_CameraSetAtEye SetView
#define Play_SetCameraFov SetFov
#define Play_CameraSetFov SetFov
#define CutsceneManager_GetCurrentCsId() currentCs
#define CutsceneManager_GetCurrentSubCamId(id) currentSub
#define CutsceneManager_SetReturnCamera(id) (returnCamera = id)
/* NATIVE DEBUG */
#undef OPEN_DISPS
#undef CLOSE_DISPS
#ifdef OOT_BROWSING_TEST
#define OPEN_DISPS(ctx) { GraphicsContext* __gfxCtx = (ctx); Gfx* dispRefs[4]; \
    Graph_OpenDisps(dispRefs, ctx, __FILE__, __LINE__);
#define CLOSE_DISPS(ctx) Graph_CloseDisps(dispRefs, ctx, __FILE__, __LINE__); }
#else
#define OPEN_DISPS(ctx) { GraphicsContext* __gfxCtx = (ctx); Gfx* dispRefs[3]; Gfx dispVals[3]; \
    Graph_OpenDisps(dispRefs, dispVals, ctx, __FILE__, __LINE__);
#define CLOSE_DISPS(ctx) Graph_CloseDisps(dispRefs, dispVals, ctx, __FILE__, __LINE__); }
#endif
void SetupCursorOverlay(GraphicsContext* ctx) {
    static Gfx setup[] = { gsSPEndDisplayList() };
    OPEN_DISPS(ctx);
    ++cursorSetups;
    __gSPDisplayList(OVERLAY_DISP++, setup);
    CLOSE_DISPS(ctx);
}
#define Gfx_SetupDL39_Overlay SetupCursorOverlay
#define Gfx_SetupDL_39Overlay SetupCursorOverlay
void UpdateBrowsingCamera(PlayState*);
/* PRODUCTION */

void NativeCameraFrame() {
    if (!sTalking) return;
    assert(ActiveCamera(&play) == 1 && play.cameraPtrs[1] == &shopCamera);
    // Match native z_play's viewpoint dispatch before the active camera frame.
    Play_RequestViewpointBgCam(&play);
    assert(sTalking && ActiveCamera(&play) == 1 && play.cameraPtrs[1] == &shopCamera);
    assert(shopCamera.status == CAM_STAT_ACTIVE && shopCamera.setting == CAM_SET_PIVOT_SHOP_BROWSING &&
           shopCamera.mode == CAM_MODE_NORMAL && shopCamera.camDataIdx == 1 && shopCamera.player == &player);
    assert(sCameraSettings[shopCamera.setting].cameraModes[shopCamera.mode].funcIdx == CAM_FUNC_DATA4 &&
           "each executed native shop frame must dispatch Data4, never FREE0/Unique6");
    assert(play.colCtx.colHeader == &sShopCollision && play.unk_1242B == 2 && YREG(15) == 0x10);
    assert(sShopCollision.cameraDataListLen == 2 && sShopCollision.cameraDataList[0].cameraSType == originalCameraRecord.cameraSType &&
           sShopCollision.cameraDataList[0].camPosData == originalCameraRecord.camPosData &&
           originalCollision.cameraDataList == &originalCameraRecord && originalCollision.cameraDataListLen == 1 &&
           "installing the private bank camera must preserve accepted scene collision and original camera records");
    assert(Camera_GetBgCamSetting(&shopCamera, 1) == CAM_SET_PIVOT_SHOP_BROWSING &&
           Camera_GetCamBGData(&shopCamera) == sShopBgData.data());
    assert(Camera_Data4(&shopCamera));
    ++nativeCameraFrames;
    assert(shopCamera.eye.x == 0 && shopCamera.eye.y == 88 && shopCamera.eye.z == 253 &&
           "Data4 must retain the scene-authored customer eye while turning");
    assert(std::isfinite(shopCamera.at.x) && std::isfinite(shopCamera.at.y) && std::isfinite(shopCamera.at.z));
    assert(std::abs(shopCamera.fov - sBrowser.view.fov) <= .011f && shopCamera.roll == 0);
    assert(manualCameraSetters == 0);
}

void SetNativeSlot(int slot) {
    sSelected = sBrowser.slot = slot;
    for (int frame = 0; frame < 30; ++frame) {
        UpdateBrowsingCamera(&play);
        NativeCameraFrame();
    }
    assert(NativeBrowsingCameraSettled(&play));
}

void ResetCursorArena(size_t opa = 64, size_t xlu = 64, size_t overlay = 128) {
    graphics = {}; commands = {}; opaqueCommands = {}; translucentCommands = {}; cursorSetups = 0;
    graphics.polyOpa.p = opaqueCommands.data(); graphics.polyOpa.d = opaqueCommands.data() + opa;
    graphics.polyXlu.p = translucentCommands.data(); graphics.polyXlu.d = translucentCommands.data() + xlu;
    graphics.overlay.p = commands.data(); graphics.overlay.d = commands.data() + overlay;
    play.state.gfxCtx = &graphics;
}
void SetCursorProjection() {
    // A deterministic projection: native helpers must project the selected
    // actor's focus, then emit the cursor rectangle about that screen point.
    play.viewProjectionMtxF = {};
    play.viewProjectionMtxF.xx = .005f;
    play.viewProjectionMtxF.yy = .005f;
    play.viewProjectionMtxF.ww = 1;
}
void CheckCursorArenas() {
    SetCursorProjection();
    SetNativeSlot(0);
    for (int shortage = 0; shortage < 6; ++shortage) {
        ResetCursorArena();
        if (shortage == 0) { graphics.overlay.p = commands.data() + commands.size(); graphics.overlay.d = graphics.overlay.p; }
        if (shortage == 1) graphics.overlay.d = graphics.overlay.p + 31;
        if (shortage == 2) { graphics.polyOpa.p = opaqueCommands.data() + opaqueCommands.size(); graphics.polyOpa.d = graphics.polyOpa.p; }
        if (shortage == 3) graphics.polyOpa.d = graphics.polyOpa.p + 7;
        if (shortage == 4) { graphics.polyXlu.p = translucentCommands.data() + translucentCommands.size(); graphics.polyXlu.d = graphics.polyXlu.p; }
        if (shortage == 5) graphics.polyXlu.d = graphics.polyXlu.p + 7;
        const auto before = graphics;
        const auto opaBefore = opaqueCommands;
        const auto xluBefore = translucentCommands;
        const auto overlayBefore = commands;
        DrawBrowseCursor();
        assert(cursorSetups == 0 && "short cursor arenas must skip before native debug/setup commands");
        assert(graphics.polyOpa.p == before.polyOpa.p && graphics.polyOpa.d == before.polyOpa.d);
        assert(graphics.polyXlu.p == before.polyXlu.p && graphics.polyXlu.d == before.polyXlu.d);
        assert(graphics.overlay.p == before.overlay.p && graphics.overlay.d == before.overlay.d);
        assert(!std::memcmp(opaqueCommands.data(), opaBefore.data(), sizeof(opaqueCommands)));
        assert(!std::memcmp(translucentCommands.data(), xluBefore.data(), sizeof(translucentCommands)));
        assert(!std::memcmp(commands.data(), overlayBefore.data(), sizeof(commands)));
    }
    ResetCursorArena(8, 8, 32);
    DrawBrowseCursor();
    assert(cursorSetups == 1 && graphics.polyOpa.p <= graphics.polyOpa.d &&
           graphics.polyXlu.p <= graphics.polyXlu.d && graphics.overlay.p <= graphics.overlay.d);
    bool rectangle = false;
    for (auto* packet = commands.data(); packet < graphics.overlay.p; ++packet)
        rectangle |= (packet->words.w0 >> 24) == G_TEXRECT;
    assert(rectangle && "a tight complete cursor budget must still draw the selection rectangle");
}
void CheckCursorAnchors() {
    SetCursorProjection();
    std::array<std::array<std::array<int, 2>, 8>, 2> centers{};
    for (int mirrored = 0; mirrored < 2; ++mirrored) for (int slot = 0; slot < 8; ++slot) {
        mirroredWorld = mirrored;
        SetNativeSlot(slot);
        Actor item{};
        item.focus.pos = kShelves[slot];
        EnOssan nativeShop{};
        EnGirlA nativeItem{}; nativeItem.actor = item; nativeShop.cursorIndex = slot;
#ifdef OOT_BROWSING_TEST
        nativeShop.shelfSlots[slot] = &nativeItem;
#else
        nativeShop.items[slot] = &nativeItem;
#endif
        NativeCursorPos(&play, &nativeShop);
#ifdef OOT_BROWSING_TEST
        const int x = nativeShop.cursorX, y = nativeShop.cursorY;
#else
        const int x = nativeShop.cursorPos.x, y = nativeShop.cursorPos.y;
#endif
        ResetCursorArena();
        DrawBrowseCursor();
        bool found = false;
        for (auto* packet = commands.data(); packet < graphics.overlay.p; ++packet) {
            if ((packet->words.w0 >> 24) != G_TEXRECT)
                continue;
            const int right = (packet->words.w0 >> 12) & 0xfff, bottom = packet->words.w0 & 0xfff;
            const int left = (packet->words.w1 >> 12) & 0xfff, top = packet->words.w1 & 0xfff;
            centers[mirrored][slot] = { (left + right) / 8, (top + bottom) / 8 };
            assert(centers[mirrored][slot][0] == x && centers[mirrored][slot][1] == y &&
                   "all cursor rectangles must match native EnOssan projection in normal and mirrored worlds");
            found = true;
        }
        assert(found && graphics.overlay.p < commands.data() + commands.size());
    }
    for (int mirrored = 0; mirrored < 2; ++mirrored) for (int a = 0; a < 8; ++a)
        for (int b = a + 1; b < 8; ++b)
            assert(centers[mirrored][a] != centers[mirrored][b] && "all eight item anchors must have distinct projected cursor centers");
    for (int slot = 0; slot < 8; ++slot)
        assert(centers[1][slot][0] == SCREEN_WIDTH - centers[0][slot][0] && centers[1][slot][1] == centers[0][slot][1]);
    mirroredWorld = false;
    SetNativeSlot(-1);
}

void CheckMirroredInput() {
    for (bool mirrored : { false, true }) for (int source = 0; source < 4; ++source) {
        mirroredWorld = mirrored;
        SetNativeSlot(-1); sBrowser.ResetInput();
        Continue(&play, kWelcomeText);
        play.state.input[0] = {};
        if (source < 2) play.state.input[0].rel.stick_x = source == 0 ? 60 : -60;
        else play.state.input[0].press.button = source == 2 ? BTN_DRIGHT : BTN_DLEFT;
        EnOssan nativeShop{};
        NativeJoystick(&play, &nativeShop);
        const int intent = source < 2 ? (nativeShop.stickAccumX > 0 ? 1 : -1)
                                      : NativeDpadIntent(play.state.input[0].press.button);
        const int soundBefore = cursorSounds;
        BrowseInput(&play);
        assert(sSelected == (intent > 0 ? 0 : 4) && cursorSounds == soundBefore + 1 &&
               "analog and D-pad bank direction must match host native mirrored-world controls");
    }
    mirroredWorld = false; play.state.input[0] = {};
    SetNativeSlot(-1); sBrowser.ResetInput(); cursorSounds = 0;
    Continue(&play, kWelcomeText);
}

void CheckReleaseLifecycle(Actor* keeper) {
    const int purchaseBefore = purchases;
    const auto press = [&](u16 button, bool shouldAdvance = false) {
        play.state.input[0] = {}; play.state.input[0].press.button = button; advance = shouldAdvance;
        BrowseInput(&play);
        play.state.input[0] = {}; advance = false;
    };
    const auto freshTalk = [&] {
        if (sTalking) EndBrowsing(&play);
        gPlayState = &play; active = true; accepted = true; allocationId = 1;
        state = TEXT_STATE_EVENT; player.stateFlags2 = 0;
        play.transitionTrigger = TRANS_TRIGGER_OFF; play.transitionMode = TRANS_MODE_OFF;
        play.gameOverCtx.state = GAMEOVER_INACTIVE; play.pauseCtx.state = 0;
        play.csCtx.state = CS_STATE_IDLE;
        play.cameraPtrs[0] = &mainCamera; play.cameraPtrs[1] = play.cameraPtrs[2] = nullptr;
#ifdef OOT_BROWSING_TEST
        play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
        mainCamera.childCamIdx = SUBCAM_FREE;
        play.colCtx.colHeader = &originalCollision; play.unk_1242B = 1; YREG(15) = 7; InitBrowsingCameraScene(&play);
#else
        play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
#endif
        CameraStatus(&play, 0, 7);
        KeeperUpdate(keeper, &play);
        assert(sTalking && sSelected == -1 && sBrowser.slot == -1 &&
               sBrowser.phase == BlackMarketBrowsing::Phase::Browse && ActiveCamera(&play) == 1);
    };
    const auto released = [&](bool teardown = false) {
      assert(!sTalking && sBrowseCamera == -1 && sBrowseParent == -1 &&
             sBrowseCameraUid == -1 && sBrowseCameraPtr == nullptr &&
             !sBrowseTextboxSaved &&
             sBrowser.phase == BlackMarketBrowsing::Phase::Browse &&
             sBrowser.slot == -1 && purchases == purchaseBefore);
      if (!teardown)
        assert(play.colCtx.colHeader == &originalCollision && play.unk_1242B == 1 && YREG(15) == 7 &&
               "normal shop release must restore the original collision header, viewpoint and room camera register");
    };
    for (int page = 0; page < 3; ++page) {
      for (int interruption = 0; interruption < 6; ++interruption) {
        freshTalk();
        for (int frame = 0; frame < 24; ++frame) { press(0); NativeCameraFrame(); }
        press(BTN_A, true);
        if (page > 0) press(BTN_A, true);
        if (page > 1) { play.msgCtx.choiceIndex = 0; press(BTN_A, true); }
        assert(sBrowser.phase != BlackMarketBrowsing::Phase::Browse && sSelected == -1);
        if (interruption == 0) state = TEXT_STATE_NONE;
        if (interruption == 1) state = TEXT_STATE_CLOSING;
        if (interruption == 2) Continue(&play, 0x1234);
        if (interruption == 3) play.transitionTrigger = TRANS_TRIGGER_START;
        if (interruption == 4) play.gameOverCtx.state = static_cast<decltype(play.gameOverCtx.state)>(1);
        if (interruption == 5) { gPlayState = nullptr; MonitorBrowsing(); released(true); }
        else { press(0); released(); }
        if (interruption == 2) assert(play.msgCtx.textId == 0x1234 && state != TEXT_STATE_CLOSING);
      }
    }
    for (auto phase : { BlackMarketBrowsing::Phase::Browse, BlackMarketBrowsing::Phase::Confirm,
                        BlackMarketBrowsing::Phase::Result }) {
        freshTalk(); sSelected = sBrowser.slot = 0; sBrowser.phase = phase;
        Continue(&play, phase == BlackMarketBrowsing::Phase::Confirm ? kConfirmText :
                        phase == BlackMarketBrowsing::Phase::Result ? kResultText : kBrowseText);
        // B must remain usable while the keeper/shelf camera is still turning.
        press(BTN_B);
        if (phase != BlackMarketBrowsing::Phase::Browse) {
            assert(sTalking && sBrowser.phase == BlackMarketBrowsing::Phase::Browse && sSelected == 0);
            press(BTN_B);
        }
        assert(sTalking && sSelected == -1);
        press(BTN_B); released();
        assert(ActiveCamera(&play) == 0 && !play.cameraPtrs[1] && !(player.stateFlags2 & (1u << 29)));
    }
    freshTalk(); SetNativeSlot(0);
    sBrowser.phase = BlackMarketBrowsing::Phase::Result; Continue(&play, kResultText);
    press(BTN_A, true);
    assert(sTalking && sBrowser.phase == BlackMarketBrowsing::Phase::Browse && purchases == purchaseBefore);
    press(BTN_B); press(BTN_B); released();
    for (int interruption = 0; interruption < 5; ++interruption) {
        freshTalk();
        if (interruption == 0) state = TEXT_STATE_NONE;
        // Active() is the native controller's scene/file/session predicate.
        if (interruption == 1) active = false;
        if (interruption == 2) play.transitionTrigger = TRANS_TRIGGER_START;
        if (interruption == 3) play.transitionMode = static_cast<decltype(play.transitionMode)>(1);
        if (interruption == 4) play.gameOverCtx.state = static_cast<decltype(play.gameOverCtx.state)>(1);
        press(0); released();
        assert(ActiveCamera(&play) == 0 && !play.cameraPtrs[1] && !(player.stateFlags2 & (1u << 29)));
        freshTalk(); press(BTN_B); released(); // immediate re-talk must release again
    }
    freshTalk();
    gPlayState = nullptr; MonitorBrowsing(); released(true);
    freshTalk(); press(BTN_B); released();
    freshTalk();
    // Teardown callbacks run after native actor/camera cleanup. Poison those
    // addresses so any accidental lookup/dereference is caught by ASan/UBSan.
#ifdef OOT_BROWSING_TEST
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = reinterpret_cast<Actor*>(uintptr_t(1));
#else
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = reinterpret_cast<Actor*>(uintptr_t(1));
#endif
    play.cameraPtrs[sBrowseCamera] = reinterpret_cast<Camera*>(uintptr_t(1));
    sBrowseCameraPtr = reinterpret_cast<Camera*>(uintptr_t(1));
    EndBrowsing(&play, true); released(true);
    freshTalk(); press(BTN_B); released();
}


void CheckReceiptHandoff(Actor* keeper) {
    const int before = purchases;
    sReceipt.Cancel(); purchaseAccepted = true;
    gPlayState = &play; active = true; accepted = true; allocationId = 1;
    play.transitionTrigger = TRANS_TRIGGER_OFF; play.transitionMode = TRANS_MODE_OFF;
    play.gameOverCtx.state = GAMEOVER_INACTIVE; play.pauseCtx.state = 0;
    play.csCtx.state = CS_STATE_IDLE; player.stateFlags2 = 0x100;
    play.cameraPtrs[0] = &mainCamera; play.cameraPtrs[1] = play.cameraPtrs[2] = nullptr;
#ifdef OOT_BROWSING_TEST
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
    mainCamera.childCamIdx = SUBCAM_FREE;
    play.colCtx.colHeader = &originalCollision; play.unk_1242B = 1; YREG(15) = 7; InitBrowsingCameraScene(&play);
#else
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
#endif
    CameraStatus(&play, 0, 7); Continue(&play, kWelcomeText);
    KeeperUpdate(keeper, &play);
    assert(sTalking && ActiveCamera(&play) == 1 && (player.stateFlags2 & (1u << 29)));
    SetNativeSlot(0);
    sBrowser.phase = BlackMarketBrowsing::Phase::Confirm;
    Continue(&play, kConfirmText); play.msgCtx.choiceIndex = 0;
    play.state.input[0] = {}; play.state.input[0].press.button = BTN_A; advance = true;
    BrowseInput(&play);
    play.state.input[0] = {}; advance = false;
    assert(purchases == before + 1 && sReceipt.pending && !sReceipt.delivered && sReceipt.slot == 0);
    assert(!sTalking && sBrowseCamera == -1 && sBrowseParent == -1 && !play.cameraPtrs[1] &&
           ActiveCamera(&play) == 0 && state == TEXT_STATE_CLOSING && player.stateFlags2 == 0x100 &&
           "accepted GI purchase must release the native shop camera and shop-only hide bit before native presentation");
    assert(play.colCtx.colHeader == &originalCollision && play.unk_1242B == 1 && YREG(15) == 7);
    const int camerasBefore = allocations, writesBefore = cameraWrites, textBefore = lastText;
    accepted = true; state = TEXT_STATE_EVENT;
    KeeperUpdate(keeper, &play);
    assert(accepted && !sTalking && allocations == camerasBefore && cameraWrites == writesBefore &&
           lastText == textBefore && purchases == before + 1 &&
           "pending GI receipt must not consume a keeper talk offer or restart browsing");
    Camera itemCamera{}; NativeCameraInit(&itemCamera, &play.view, &play.colCtx, &play);
    itemCamera.thisIdx = 2; play.cameraPtrs[2] = &itemCamera; CameraStatus(&play, 2, 7);
    MonitorBrowsing(); KeeperUpdate(keeper, &play);
    assert(ActiveCamera(&play) == 2 && play.cameraPtrs[2] == &itemCamera && !play.cameraPtrs[1] &&
           allocations == camerasBefore && cameraWrites == writesBefore &&
           "pending receipt must leave the native get-item camera in control");
    sReceipt.Cancel(); purchaseAccepted = false;
    play.cameraPtrs[2] = nullptr; CameraStatus(&play, 0, 7);
    KeeperUpdate(keeper, &play);
    assert(sTalking && allocations == camerasBefore + 1 && ActiveCamera(&play) == 1 &&
           "keeper browsing must be available again after the receipt is retired");
    EndBrowsing(&play);
    assert(player.stateFlags2 == 0x100);
}

void CheckFixedCustomerEye() {
  BlackMarketBrowsing::State camera;
  for (int slot : {-1, 0, 1, 3, 2, -1, 4, 5, 7, 6, -1}) {
    camera.slot = slot;
    for (int frame = 0; frame < 24; ++frame) {
      camera.TickCamera();
      assert(camera.view.eye.x == 0 && camera.view.eye.y == 88 &&
             camera.view.eye.z == 253 &&
             "keeper, either bank and both rows must turn from one close "
             "customer eye without pulling behind Link");
    }
    assert(camera.CameraSettled());
  }
}

void CheckSteadyShelfBank() {
  for (int direction : {-1, 1}) {
    BlackMarketBrowsing::State browser;
    assert(browser.Move(direction, 0));
    for (int frame = 0; frame < 30; ++frame)
      browser.TickCamera();
    const auto bank = browser.view;
    for (const auto move : {std::array<int, 2>{0, 1}, {direction, 0},
                            {0, -1}, {-direction, 0}}) {
      assert(browser.Move(move[0], move[1]));
      for (int frame = 0; frame < 30; ++frame) {
        browser.TickCamera();
        assert(browser.view.eye.x == bank.eye.x && browser.view.eye.y == bank.eye.y &&
               browser.view.eye.z == bank.eye.z && browser.view.at.x == bank.at.x &&
               browser.view.at.y == bank.at.y && browser.view.at.z == bank.at.z &&
               browser.view.fov == bank.fov &&
               "moving the cursor within one shelf bank must never move or pitch the camera");
      }
    }
  }
}

void CheckNativeSteadyShelfBank() {
    for (int direction : {-1, 1}) {
        SetNativeSlot(direction > 0 ? 0 : 4);
        const auto eye = shopCamera.eye;
        const auto angles = [] {
            const float x = shopCamera.at.x - shopCamera.eye.x, y = shopCamera.at.y - shopCamera.eye.y;
            const float z = shopCamera.at.z - shopCamera.eye.z;
            return std::array<float, 2>{std::atan2(x, -z), std::atan2(y, std::hypot(x, z))};
        };
        const auto bank = angles();
        const auto fov = shopCamera.fov;
        const auto sceneFov = sShopBgData[2].x;
        for (const auto move : {std::array<int, 2>{0, 1}, {direction, 0}, {0, -1}, {-direction, 0}}) {
            assert(sBrowser.Move(move[0], move[1])); sSelected = sBrowser.slot;
            for (int frame = 0; frame < 30; ++frame) {
                UpdateBrowsingCamera(&play); NativeCameraFrame();
                const auto actual = angles();
                assert(shopCamera.eye.x == eye.x && shopCamera.eye.y == eye.y && shopCamera.eye.z == eye.z &&
                       std::abs(actual[0] - bank[0]) < .00001f && std::abs(actual[1] - bank[1]) < .00001f &&
                       shopCamera.fov == fov && sShopBgData[2].x == sceneFov && NativeBrowsingCameraSettled(&play) &&
                       "moving across all four offers must retain one settled native bank eye/direction/FOV");
            }
        }
    }
    SetNativeSlot(-1);
}

void CheckShelfFraming() {
    const float radius = .32f * 38, halfHeight = .32f * 42;
    const auto extension = [](float half, float floor, float extra) {
        return std::max((floor + 1.4f) / .875f, (std::max(half, floor) + 1.4f) / .875f - half) + extra + 1.f / 64;
    };
    // The authored item-fit contract includes selected scale, bob, tall feather
    // and wrapping X seal. Arbitrary replacement models are outside this proof.
    const Vec3f low{ -radius - extension(radius, 2.4f, .85f),
                     -2 - .32f * 22 - extension(halfHeight, 3.2f, .85f),
                     -radius - extension(radius, 2.4f, .45f) };
    const Vec3f high{ -low.x, 2 + .32f * 62 + extension(halfHeight, 3.2f, .85f), -low.z };
    const auto dot = [](Vec3f a, Vec3f b) { return a.x*b.x + a.y*b.y + a.z*b.z; };
    for (int selected = 0; selected < 8; ++selected) {
        SetNativeSlot(selected);
        // Use the output of the real Data4 frame, including its native binary
        // angle/trig quantization, rather than the adapter's intended view.
        const Vec3f eye = shopCamera.eye;
        Vec3f forward{ shopCamera.at.x - eye.x, shopCamera.at.y - eye.y, shopCamera.at.z - eye.z };
        const float length = std::sqrt(dot(forward, forward));
        forward = { forward.x / length, forward.y / length, forward.z / length };
        const float rightLength = std::hypot(forward.x, forward.z);
        const Vec3f right{ -forward.z / rightLength, 0, forward.x / rightLength };
        const Vec3f up{ -right.z * forward.y, right.z * forward.x - right.x * forward.z, right.x * forward.y };
        const float half = std::tan(shopCamera.fov * 3.14159265f / 360);
        const auto project = [&](Vec3f p) {
            const Vec3f d{ p.x - eye.x, p.y - eye.y, p.z - eye.z };
            const float depth = dot(d, forward);
            assert(depth > 0);
            return std::array<float, 2>{ 160 + dot(d, right) / depth / half * 120,
                                         120 - dot(d, up) / depth / half * 120 };
        };
        // Graphics boundary: form a deterministic 4:3 perspective matrix from
        // the native camera, then execute native Actor_GetScreenPos and GBI.
        auto& matrix = play.viewProjectionMtxF; matrix = {};
        matrix.xx = right.x / half * .75f; matrix.xy = right.y / half * .75f; matrix.xz = right.z / half * .75f;
        matrix.xw = -dot(eye, right) / half * .75f;
        matrix.yx = up.x / half; matrix.yy = up.y / half; matrix.yz = up.z / half;
        matrix.yw = -dot(eye, up) / half;
        matrix.wx = forward.x; matrix.wy = forward.y; matrix.wz = forward.z; matrix.ww = -dot(eye, forward);
        const int first = selected < 4 ? 0 : 4;
        for (int offer = first; offer < first + 4; ++offer) {
            const auto anchor = kShelves[offer];
            const int bank = offer < 4 ? 0 : 6;
            const auto& bottom = BlackMarketShelves::kBoxes[bank + (offer & 1 ? 4 : 3)];
            const auto& top = BlackMarketShelves::kBoxes[bank + (offer & 1 ? 5 : 4)];
            const int localColumnX = ((offer % 4) < 2 ? 35 : 70) * (offer < 4 ? 1 : -1);
            const auto shelfPosition = BlackMarketShelves::RoomPoint({s16(localColumnX), s16(anchor.y), 110});
            assert(std::abs(anchor.x - shelfPosition.x) <= .5f && std::abs(anchor.z - shelfPosition.z) <= .5f &&
                   "literal offer anchors must agree with the actual native bank geometry transform");
            const auto plank = BlackMarketShelves::Corners(bottom);
            const float dx = anchor.x - plank[0].x, dz = anchor.z - plank[0].z;
            const float ux = plank[1].x - plank[0].x, uz = plank[1].z - plank[0].z;
            const float vx = plank[4].x - plank[0].x, vz = plank[4].z - plank[0].z;
            const float determinant = ux * vz - uz * vx;
            const float u = (dx * vz - dz * vx) / determinant;
            const float v = (ux * dz - uz * dx) / determinant;
            assert(u > 0 && u < 1 && v > 0 && v < 1 &&
                   anchor.y + low.y > bottom.high.y && anchor.y + high.y < top.low.y &&
                   "actual transformed shelf openings must contain their authored offer rows");
            const auto cursor = project(anchor);
            assert(cursor[0] - 20 > 8 && cursor[0] + 20 < 312 && cursor[1] - 20 > 4 && cursor[1] + 20 < 142 &&
                   "all four offers and cursors must stay above the textbox in one bank shot");
            for (int corner = 0; corner < 8; ++corner) {
                const auto point = project({ anchor.x + (corner & 1 ? high.x : low.x),
                                             anchor.y + (corner & 2 ? high.y : low.y),
                                             anchor.z + (corner & 4 ? high.z : low.z) });
                assert(point[0] > 8 && point[0] < 312 && point[1] > 8 && point[1] < 138 &&
                       "all four bank items and seals must clear the native bottom textbox at 4:3");
            }
            sSelected = sBrowser.slot = offer;
            for (bool mirrored : {false, true}) {
                mirroredWorld = mirrored;
                ResetCursorArena(); DrawBrowseCursor();
                bool rectangle = false;
                for (auto* packet = commands.data(); packet < graphics.overlay.p; ++packet) {
                    if ((packet->words.w0 >> 24) != G_TEXRECT) continue;
                    const int right = (packet->words.w0 >> 12) & 0xfff, bottom = packet->words.w0 & 0xfff;
                    const int left = (packet->words.w1 >> 12) & 0xfff, top = packet->words.w1 & 0xfff;
                    assert(left > 8 * 4 && right < 312 * 4 && top > 4 * 4 && bottom < 142 * 4);
                    assert((left + right) / 8 == (mirrored ? 320 - int(cursor[0]) : int(cursor[0])) &&
                           (top + bottom) / 8 == int(cursor[1]));
                    rectangle = true;
                }
                assert(rectangle && "all bank offers must draw an unclipped cursor from the native camera projection");
            }
            mirroredWorld = false;
        }
    }
    SetNativeSlot(-1);
}

int main() {
  CheckSteadyShelfBank();
  CheckFixedCustomerEye();
  InitNativeCameraTable();
  NativeCameraInit(&mainCamera, &play.view, &play.colCtx, &play);
  originalCollision.cameraDataList = &originalCameraRecord;
  originalCollision.cameraDataListLen = 1;
  play.colCtx.colHeader = &originalCollision;
  play.unk_1242B = 1; YREG(15) = 7;
  InitBrowsingCameraScene(&play);
  gPlayState = &play;
#ifdef OOT_BROWSING_TEST
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
#else
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
#endif
    play.cameraPtrs[0] = &mainCamera;
#ifdef OOT_BROWSING_TEST
    Actor keeper{};
#else
    EnTakaraya gal{}; Actor& keeper = gal.actor;
#endif
    KeeperUpdate(&keeper, &play);
    NativeCameraFrame();
    assert(allocations == 1 && ActiveCamera(&play) == 1 && cameraWrites > 0 &&
           "accepting shop talk must start a native controlled browsing camera");
    assert(sTalking && sSelected == -1 && (player.stateFlags2 & (1u << 29)));
    assert(shopCamera.eye.x == 0 && shopCamera.eye.y == 88 && shopCamera.eye.z == 253 &&
           shopCamera.at.z < shopCamera.eye.z && shopCamera.fov == 46 &&
           "the welcome view must face the keeper from the customer side at conversational distance");
    CheckNativeSteadyShelfBank();
    CheckShelfFraming();
    CheckCursorArenas();
    CheckCursorAnchors();
    CheckMirroredInput();
#ifdef OOT_BROWSING_TEST
    assert(mainCamera.childCamIdx==SUBCAM_FREE &&
           "owned shop subcamera must not hold native parent=MAIN cutscenes in the one-point queue");
#endif
    auto input = [&](u16 button, bool shouldAdvance = false) {
        play.state.input[0].press.button = button; advance = shouldAdvance;
        KeeperUpdate(&keeper,&play);
        NativeCameraFrame();
        play.state.input[0].press.button = 0; advance = false;
    };
    auto finishPan = [&] { for (int i = 0; i < 24; ++i) input(0); };
    // Keeper A talks without selecting stock or changing the native Data4 view.
    input(BTN_A, true);
    assert(sSelected == -1 && lastText == 0x8F15 && purchases == 0 && sTalking);
    input(BTN_A, true);
    assert(lastText == 0x8F17 && sSelected == -1 && purchases == 0);
    play.msgCtx.choiceIndex = 0;
    input(BTN_A, true);
    assert(lastText == 0x8F18 && purchases == 0 && ActiveCamera(&play) == 1);
    state = TEXT_STATE_AWAITING_NEXT;
    input(BTN_A, true);
    assert(lastText == 0x8F18 && sSelected == -1);
    state = TEXT_STATE_EVENT;
    input(BTN_A, true);
    assert(lastText == kWelcomeText && sSelected == -1 && purchases == 0);
    input(BTN_A, true); input(BTN_A, true);
    play.msgCtx.choiceIndex = 1;
    input(BTN_A, true);
    assert(lastText == kWelcomeText && purchases == 0);
    for (int page = 0; page < 3; ++page) {
        input(BTN_A, true);
        if (page > 0) input(BTN_A, true);
        if (page > 1) { play.msgCtx.choiceIndex = 0; input(BTN_A, true); }
        input(BTN_B, true);
        assert(lastText == kWelcomeText && sTalking && sSelected == -1 && purchases == 0);
    }
    play.state.input[0].rel.stick_x = 20; input(0); play.state.input[0].rel.stick_x = 0;
    assert(cursorSounds == 0 && "stick deadzone and idle keeper frames must not play cursor audio");
    input(BTN_DRIGHT); assert(sSelected == 0 && cursorSounds == 1);
    input(BTN_DUP); assert(sSelected == 0 && "do not move between items while turning toward the shelf");
    input(BTN_A,true); assert(lastText != kConfirmText && purchases == 0 &&
                             "pan input must not select an item before the shelf is visible");
    assert(cursorSounds == 1 && "blocked pan inputs must not repeat the cursor sound");
    graphics.overlay.p = commands.data(); DrawBrowseCursor();
    assert(graphics.overlay.p == commands.data() && "hide the item cursor until the shelf pan finishes");
    finishPan();
    input(BTN_DUP); assert(sSelected == 1);
    finishPan();
    input(BTN_DRIGHT); assert(sSelected == 3);
    input(BTN_DDOWN); assert(sSelected == 2);
    finishPan();
    input(BTN_DLEFT); assert(sSelected == 0);
    input(BTN_DDOWN); assert(sSelected == 0 && cursorSounds == 5 &&
                            "only actual row/column changes play one native cursor cue");
    for (int i=0;i<24;++i) input(0);
    assert(shopCamera.at.x > shopCamera.eye.x && shopCamera.eye.z > shopCamera.at.z && shopCamera.fov == 72);
    const float rightYaw = std::atan2(shopCamera.at.x - shopCamera.eye.x,
                                     shopCamera.eye.z - shopCamera.at.z) * 180.f / 3.14159265f;
    assert(std::abs(rightYaw - 30) < .25f && "use native OoT's 30-degree right shelf direction");
    input(BTN_DLEFT); assert(sSelected == -1 && cursorSounds == 6);
    input(BTN_DLEFT); assert(sSelected == -1 && "return through the keeper view before changing shelf banks");
    assert(cursorSounds == 6);
    finishPan();
    input(BTN_DLEFT); assert(sSelected == 4);
    finishPan();
    input(BTN_DUP); assert(sSelected == 5);
    finishPan();
    input(BTN_DLEFT); assert(sSelected == 7);
    input(BTN_DDOWN); assert(sSelected == 6 && cursorSounds == 10);
    for (int i=0;i<24;++i) input(0);
    assert(shopCamera.at.x < shopCamera.eye.x && shopCamera.fov == 72);
    const float leftYaw = std::atan2(shopCamera.at.x - shopCamera.eye.x,
                                    shopCamera.eye.z - shopCamera.at.z) * 180.f / 3.14159265f;
    assert(std::abs(leftYaw + 30) < .25f && "use native OoT's 30-degree left shelf direction");
    input(BTN_A,true); assert(lastText == kConfirmText && purchases == 0);
    input(BTN_DRIGHT); assert(sSelected == 6 && purchases == 0);
    play.msgCtx.choiceIndex = 1;
    input(BTN_A,true); assert(lastText == kBrowseText && purchases == 0);
    input(BTN_A,true); assert(lastText == kConfirmText);
    play.msgCtx.choiceIndex = 0;
    input(BTN_A,true); assert(lastText == kResultText && purchases == 1 && !sReceipt.pending &&
                             sTalking && ActiveCamera(&play) == 1 &&
                             "rejected purchase must retain browsing and show its result");
    input(0); assert(purchases == 1);
    input(BTN_A,true); assert(lastText == kBrowseText && purchases == 1);
    assert(cursorSounds == 10 && "confirmation, cancellation and results do not replay selection audio");
    const int writesBeforePause = cameraWrites;
    play.pauseCtx.state = 1;
    input(BTN_DRIGHT); assert(sSelected == 6 && cameraWrites == writesBeforePause);
    assert(cursorSounds == 10 && "paused navigation must remain silent");
    play.pauseCtx.state = 0;
    input(BTN_B); assert(sSelected == -1 && sTalking && cursorSounds == 11);
    input(BTN_B); assert(!sTalking && ActiveCamera(&play)==0 && !play.cameraPtrs[1]);
    assert(cursorSounds == 11 && "leaving the keeper view does not move a merchandise cursor");
    assert(!(player.stateFlags2 & (1u << 29)));
    player.stateFlags2 = (1u<<29) | 0x100;
    accepted = true; state = TEXT_STATE_EVENT; input(0);
    state = TEXT_STATE_CLOSING; input(0);
    assert(!sTalking && player.stateFlags2 == ((1u<<29)|0x100));
    player.stateFlags2 = 0x100;
    accepted = true; state = TEXT_STATE_EVENT; allocationId = -1; input(0);
    assert(!sTalking && ActiveCamera(&play)==0 && player.stateFlags2==0x100);
    allocationId = 1; accepted = true; state = TEXT_STATE_EVENT; input(0);
    Camera otherCamera{}; NativeCameraInit(&otherCamera, &play.view, &play.colCtx, &play);
    otherCamera.thisIdx = 2; play.cameraPtrs[2] = &otherCamera;
    otherCamera.eye = otherCamera.eyeNext = {901, 902, 903}; otherCamera.at = {805, 806, 807};
    otherCamera.fov = 53; otherCamera.roll = 123;
    currentCs = 8; currentSub = 2; returnCamera = 1;
#ifdef OOT_BROWSING_TEST
    shopCamera.childCamIdx = 2; otherCamera.parentCamIdx = 1; mainCamera.childCamIdx = SUBCAM_FREE;
#endif
    CameraStatus(&play,2,7);
    Camera expectedOtherCamera = otherCamera; expectedOtherCamera.parentCamIdx = 0;
    // Native z_play dispatches the viewpoint after actor updates and before
    // the later shop monitor hook. A foreign actor/hook may take the camera
    // in this window, while the scene still advertises the shop's BG index1.
    assert(sTalking && play.unk_1242B == 2 && play.colCtx.colHeader == &sShopCollision);
    Play_RequestViewpointBgCam(&play);
    if (std::memcmp(&otherCamera, &expectedOtherCamera, sizeof(Camera)))
        std::fprintf(stderr, "OOT VIEWPOINT CAMERA MUTATION: setting=%d->%d bgIndex=%d->%d uid=%d->%d\n",
                     expectedOtherCamera.setting, otherCamera.setting, expectedOtherCamera.camDataIdx,
                     otherCamera.camDataIdx, expectedOtherCamera.uid, otherCamera.uid);
    assert(!sTalking && sBrowseCamera == -1 && sBrowseCameraUid == -1 && sBrowseCameraPtr == nullptr &&
           play.colCtx.colHeader == &originalCollision && play.unk_1242B == 1 && YREG(15) == 7 &&
           state == TEXT_STATE_CLOSING && player.stateFlags2 == 0x100 &&
           "native viewpoint dispatch must release lost shop ownership before routing scene camera data");
    MonitorBrowsing();
    assert(!sTalking && ActiveCamera(&play)==2 && !play.cameraPtrs[1] &&
           "a cutscene taking the camera must cancel shop ownership without stealing it back");
#ifdef OOT_BROWSING_TEST
    assert(otherCamera.parentCamIdx==0 && mainCamera.childCamIdx==2);
    assert(!std::memcmp(&otherCamera, &expectedOtherCamera, sizeof(Camera)) &&
           "native shop release may repair the foreign camera parent link but must preserve its identity and full pose");
#else
    assert(returnCamera==0 && "native CutsceneManager must have a live return camera after shop interruption");
#endif
    currentCs = currentSub = -1;
    play.cameraPtrs[2] = nullptr;
#ifdef OOT_BROWSING_TEST
    mainCamera.childCamIdx = SUBCAM_FREE;
#endif
    CameraStatus(&play,0,7); accepted = true; state = TEXT_STATE_EVENT; input(0);
    const int cameraWritesBeforeReuse = cameraWrites;
    // The engines reuse subCameras[id-1] storage; pointer equality is insufficient.
    ++shopCamera.uid; const Camera reusedCamera = shopCamera;
    Play_RequestViewpointBgCam(&play);
    assert(!sTalking && play.colCtx.colHeader == &originalCollision && play.unk_1242B == 1 && YREG(15) == 7 &&
           "native viewpoint dispatch must release UID reuse before routing scene camera data");
    MonitorBrowsing();
    assert(!std::memcmp(&shopCamera, &reusedCamera, sizeof(Camera)) &&
           "UID reuse must preserve every field of the newly owned native camera");
    assert(!sTalking && play.cameraPtrs[1]==&shopCamera && cameraWrites==cameraWritesBeforeReuse);
    play.cameraPtrs[1] = nullptr;
#ifdef OOT_BROWSING_TEST
    mainCamera.childCamIdx = SUBCAM_FREE;
#endif
    CameraStatus(&play,0,7); accepted = true; state = TEXT_STATE_EVENT; input(0);
    input(BTN_DRIGHT); assert(sSelected == 0 && cursorSounds == 12 && "Right enters the first bank with one cursor cue");
    finishPan(); input(BTN_A,true); assert(lastText==kConfirmText);
    input(BTN_B); assert(lastText==kBrowseText && purchases==1);
    // FastText/SkipText makes native Message_ShouldAdvance true for a held B.
    play.state.input[0].cur.button = BTN_B;
    input(0,true);
    assert(lastText==kBrowseText && purchases==1 &&
           "held B must not reopen confirmation or accept the default Buy choice");
    assert(cursorSounds == 12);
    play.state.input[0].cur.button = 0;
    EndBrowsing(&play);
    accepted = true; state = TEXT_STATE_EVENT; input(0);
    Continue(&play,0x1234);
    const int closesBeforeForeignMessage = closes;
    MonitorBrowsing();
    assert(!sTalking && closes==closesBeforeForeignMessage && state==TEXT_STATE_EVENT &&
           "shop cleanup must preserve a replacement actor's dialogue");
    active = false; accepted = true;
    const int allocationsBeforeEmptyStock = allocations;
    KeeperUpdate(&keeper, &play);
    assert(lastText==BlackMarketEntry::kUnavailableText && !sTalking && purchases==1 &&
           allocations==allocationsBeforeEmptyStock &&
           "empty stock offers a normal explanation without browsing or purchasing");
    CheckReleaseLifecycle(&keeper);
    CheckReceiptHandoff(&keeper);
    assert(nativeSettingRequests > 0 && nativeCameraFrames > 100 && manualCameraSetters == 0);
    BlackMarketBrowsing::Axis axis;
    assert(axis.Step(30)==0 && axis.Step(60)==1);
    for (int i=0;i<9;++i) assert(axis.Step(60)==0);
    assert(axis.Step(60)==1 && axis.Step(-60)==-1 && axis.Step(0)==0 && axis.Step(-60)==-1);
    std::puts(
        "PASS OoT shop adapter: actual Camera_Init/InitPlayerSettings, background-camera "
        "lookup, ChangeDataIdx/setting/mode and Data4/CalcAtDefault/native trig frames; "
        "keeper/both banks/rows/return, all four bank offers/cursors above textbox at 4:3, "
        "normal/mirrored cursor GBI, navigation, purchase/receipt and ownership cleanup; "
        "native viewpoint-dispatch takeover guard; zero manual at/eye/FOV or FREE0 frames");
}
