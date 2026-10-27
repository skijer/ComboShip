/* NATIVE MM SUPPORT */
#include "2s2h/GameInteractor/GameInteractor.h"
#include "combo/BlackMarketEntry.h"
#include "combo/BlackMarketGraphicsArena.h"
#include "combo/BlackMarketKeeper.h"
#include "combo/BlackMarketKeeperAssets.h"
#include "combo/BlackMarketMmShopScene.h"
#include "combo/BlackMarketMmShopLayout.h"
#include "mm/2s2h/Enhancements/Gameplay/BlackMarketCounterBarrier.h"
#include "combo/BlackMarketReceipt.h"
#include "combo/rando/BlackMarket.h"
#include "objects/object_bg/object_bg.h"
#include <sanitizer/asan_interface.h>

extern "C" s32 EnOssan_TakeItemOffShelf(EnOssan *);
extern "C" s32 EnOssan_ReturnItemToShelf(EnOssan *);

GameState *gGameState;
using namespace ComboRando::BlackMarket;
static bool active = true, safe = true, purchaseAccepted, receiptContext = true,
            sceneReady = true;
static bool placementIdle = true, customerFloorPresent = true, customerFloorDynamic;
static CollisionPoly customerFloor;
static bool sTalking, sCustomKeeper;
static int sSelected, purchases, closes;
static std::string sStatus;
static BlackMarketReceipt::State sReceipt;
static Session sSession;
static std::array<PurchaseResult, 8> status;
static std::array<Gfx, 2048> opaque, translucent;
static auto &keeper = shop; // A real EnOssan; no EnTakaraya allocation is cast.
bool Interior() { return true; }
bool Active() { return active; }
bool PackAvailable() { return true; }
bool Safe(PlayState *) { return safe; }
bool CustomKeeperAvailable() {
  return true;
} // Resource selection has its own keeper fixture.
const Item *Offer(int index) {
  return index >= 0 && index < 8 ? &Catalog()[index] : nullptr;
}
PurchaseResult Status(int index) {
  assert(index >= 0 && index < 8);
  return status[index];
}
bool ReceiptContext() { return receiptContext; }
bool ReceiptPlacementIdle() { return placementIdle; }
extern "C" f32 BgCheck_EntityRaycastFloor5_3(PlayState*, CollisionContext*, CollisionPoly** floor,
                                             s32* bg, Actor*, Vec3f* probe) {
  assert(probe->z >= 132 && probe->y >= 20 && "receipt must probe the saved customer floor");
  *floor = customerFloorPresent ? &customerFloor : nullptr;
  *bg = customerFloorDynamic ? 0 : BGCHECK_SCENE;
  return customerFloorPresent ? 0 : BGCHECK_Y_MIN;
}
void CancelReceipt(bool = false) { sReceipt.Cancel(); }
bool Purchase() {
  ++purchases;
  return purchaseAccepted && sReceipt.Begin(sSelected, 0, 0, 0);
}
extern "C" bool Player_InBlockingCsMode(PlayState *, Player *) { return false; }
// The resource loader/RTTI boundary is checked separately. Supply decoded
// authored scene commands before the actual native actor Init/GetCutscenes.
bool NativeShopSceneReady(PlayState *) { return sceneReady; }
struct FixtureSceneLifetime {
  void reset() {}
} sNativeShopSceneResource;
// Actual barrier collision/registration and talk offers run in the dedicated
// native barrier executable; this fixture controls that service boundary.
struct FixtureCounterBarrier { bool Ensure(PlayState*) { return true; } } sCounterBarrier;
/* PRODUCTION CONSTANTS */
/* NATIVE STOCK UPDATE */
/* PRODUCTION ADAPTER */
static GameInteractor drawHooks;
GameInteractor* GameInteractor::Instance = &drawHooks;
extern "C" u8 Hourglass_ShouldDrawGray(Actor*) { return false; }
extern "C" void Hourglass_PushGray(PlayState*) { assert(false); }
extern "C" void Hourglass_PopGray(PlayState*) { assert(false); }
extern "C" void GameInteractor_ExecuteOnActorDraw(Actor*) {}
/* PRODUCTION PLAYER DRAW HOOK */
/* INSPECTION LETTERBOX BOUNDARY */
static float lastFrameVisibleTop;
static float NativeVisibleTop() {
  // Run the real native scissor and rectangle writers in private command
  // buffers, preserving the shop's cursor GBI and the actual view/projection.
  GraphicsContext scratch{};
  std::array<Gfx, 64> opa{}, xlu{}, overlay{};
  scratch.polyOpa.p = opa.data();
  scratch.polyOpa.d = opa.data() + opa.size();
  scratch.polyXlu.p = xlu.data();
  scratch.polyXlu.d = xlu.data() + xlu.size();
  scratch.overlay.p = overlay.data();
  scratch.overlay.d = overlay.data() + overlay.size();
  View view = play.view;
  view.gfxCtx = &scratch;
  view.viewport = {0, 240, 0, 320};
  const auto snapshot = view;
  const auto current = ShrinkWindow_Letterbox_GetSize();
  const auto target = ShrinkWindow_Letterbox_GetSizeTarget();
  const auto visibleSize = std::clamp(current, 0, 120);
  View_ApplyLetterbox(&view);
  ShrinkWindow_Draw(&scratch);
  int scissors = 0, rectangles = 0;
  for (auto *p = opa.data(); p < scratch.polyOpa.p; ++p)
    if ((p->words.w0 >> 24) == G_SETSCISSOR) {
      ++scissors;
      assert((p->words.w0 & 0xFFF) == u32(visibleSize * 4));
    }
  for (auto *p = overlay.data(); p < scratch.overlay.p; ++p)
    if ((p->words.w0 >> 24) == G_FILLWIDERECT)
      ++rectangles;
  assert(!std::memcmp(&snapshot, &view, sizeof(View)));
  assert(current == ShrinkWindow_Letterbox_GetSize() &&
         target == ShrinkWindow_Letterbox_GetSizeTarget());
  if (!scissors) {
    assert(scratch.polyXlu.p == xlu.data() && !rectangles);
    return 2;
  }
  assert(scissors == 1 && rectangles == (current > 0 ? 4 : 0));
  return visibleSize + 2;
}

extern "C" Actor *Actor_Spawn(ActorContext *, PlayState *state, s16 id, f32 x,
                              f32 y, f32 z, s16, s16, s16, s32 params) {
  assert(id == ACTOR_EN_GIRLA && spawns < 8 &&
         params == BlackMarketMmShopScene::kItemParamBase + spawns);
  auto &item = stock[spawns++];
  item = {};
  item.actor.id = id;
  item.actor.params = params;
  item.actor.flags = ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_FRIENDLY;
  item.actor.world.pos = {x, y, z};
  InitNativeStock(&item.actor, state);
  assert(!(item.actor.flags & ACTOR_FLAG_ATTENTION_ENABLED));
  assert(item.actor.flags & ACTOR_FLAG_FRIENDLY);
  return &item.actor;
}
static void ResetArena() {
  graphics.overlay.p = overlay.data();
  graphics.overlay.d = overlay.data() + overlay.size();
  graphics.polyOpa.p = opaque.data();
  graphics.polyOpa.d = opaque.data() + opaque.size();
  graphics.polyXlu.p = translucent.data();
  graphics.polyXlu.d = translucent.data() + translucent.size();
}
static void Reset() {
  EndBrowsing(nullptr, true);
  play = {};
  player = {};
  shop = {};
  stock = {};
  graphics = {};
  gSaveContext = {};
  ShrinkWindow_Init();
  userDisableBlackBars = false;
  RegisterDisableBlackBars();
  play.state.framerateDivisor = 3;
  gPlayState = &play;
  gGameState = &play.state;
  active = safe = receiptContext = sceneReady = true;
  placementIdle = customerFloorPresent = true;
  customerFloorDynamic = false;
  customerFloor = {};
  customerFloor.normal.y = 32767;
  talkAccepted = advance = purchaseAccepted = mirror = false;
  invert = 1;
  sReceipt = {};
  status.fill(PurchaseResult::Ready);
  sStatus.clear();
  sSession.market = {{"stock", nlohmann::json::array()}};
  for (int i = 0; i < 8; ++i)
    sSession.market["stock"].push_back({{"clears", 1}});
  sSelected = purchases = closes = spawns = kills = objectLookups =
      objectLoads = 0;
  playerWaits = playerEnds = collisionQueries = nativeSkeletonInits =
      animationChanges = 0;
  callbacks.clear();
  nativeCalls.clear();
  messageState = TEXT_STATE_NONE;
  sCameraNextUID = 0;
  play.state.running = true;
  play.state.gfxCtx = &graphics;
  ResetArena();
  gSaveContext.save.day = 1;
  player.transformation = PLAYER_FORM_HUMAN;
  player.actor.id = ACTOR_PLAYER;
  player.actor.world.pos = {0, 0, 160};
  player.actor.floorPoly = &customerFloor;
  player.actor.floorBgId = BGCHECK_SCENE;
  player.actor.bgCheckFlags = BGCHECKFLAG_GROUND;
  play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
  InitNativeCameraTable();
  Camera_Init(&mainCamera, &play.view, &play.colCtx, &play);
  mainCamera.camId = CAM_ID_MAIN;
  mainCamera.focalActor = &player.actor;
  mainCamera.focalActorPosRot.pos = player.actor.world.pos;
  mainCamera.eye = mainCamera.eyeNext = {50, 100, 260};
  mainCamera.at = {0, 56, 0};
  play.cameraPtrs[0] = &mainCamera;
  Play_ChangeCameraStatus(&play, 0, CAM_STATUS_ACTIVE);
  for (int i = 0; i < 4; ++i) {
    std::memcpy(cameraData[i].data(), BlackMarketMmShopScene::kCameraRecords[i],
                sizeof(cameraData[i]));
    cameraInfo[i] = {CAM_SET_BLACK_MARKET_OOT, 3, cameraData[i].data()};
    cutscenes[i] = {s16(BlackMarketMmShopScene::kFirstPriority + i),
                    -1,
                    BlackMarketMmShopScene::kCutsceneCameras[i],
                    CS_SCRIPT_ID_NONE,
                    s16(i == 3 ? -1 : i + 1),
                    CS_END_SFX_NONE,
                    255,
                    BlackMarketMmShopScene::kHudVisibility,
                    CS_END_CAM_0,
                    BlackMarketMmShopScene::kLetterboxSize};
  }
  play.actorCsCamList = cameraInfo.data();
  CutsceneManager_Init(&play, cutscenes.data(), cutscenes.size());
  sCutsceneMgr.retCamId = CAM_ID_MAIN;
  shop.actor.id = ACTOR_EN_OSSAN;
  shop.actor.params = BlackMarketMmShopScene::kKeeperParams;
  shop.actor.objectSlot = 0;
  shop.actor.xzDistToPlayer = 40;
  InitNativeKeeper(&shop.actor, &play);
  assert(sNativeShop == &shop && shop.actionFunc == EnOssan_InitShop &&
         !objectLookups);
  KeeperUpdate(&shop.actor, &play);
  assert(shop.actionFunc == EnOssan_Idle && shop.actor.update == KeeperUpdate &&
         !sTalking);
  assert(spawns == 8 && nativeSkeletonInits == 1 && !kills);
  assert(shop.skelAnime.jointTable == sKeeperJoints.data() &&
         shop.skelAnime.morphTable == sKeeperMorphs.data());
  assert(shop.skelAnime.jointTable != shop.jointTable &&
         shop.skelAnime.morphTable != shop.morphTable);
  for (int i = 0; i < 8; ++i)
    assert(shop.items[i] == &stock[i] &&
           stock[i].actor.update == EnGirlA_Update);
}
static void NativeFrame() {
  nativeCalls.clear();
  ResetArena();
  KeeperUpdate(&shop.actor, &play);
  for (auto &item : stock)
    if (item.actor.update)
      item.actor.update(&item.actor, &play);
  ShrinkWindow_Update(play.state.framerateDivisor);
  if (NativeShopOwnCamera(&play) &&
      Play_GetActiveCamId(&play) == sBrowseCamera) {
    assert(sBrowseCameraPtr->setting == CAM_SET_BLACK_MARKET_OOT &&
           sBrowseCameraPtr->mode == CAM_MODE_NORMAL);
    assert(sCameraSettings[sBrowseCameraPtr->setting].cameraModes[0].funcId ==
           CAM_FUNC_DATA4);
    assert(Camera_Data4(sBrowseCameraPtr));
    Camera_UpdateInterface(sCameraInterfaceFlags);
  }
  KeeperDraw(&shop.actor, &play);
  lastFrameVisibleTop = NativeVisibleTop();
  CutsceneManager_Update();
  CutsceneManager_ClearWaiting();
  play.state.input[0] = {};
  advance = false;
  ++play.gameplayFrames;
}
static void Press(u16 button, bool shouldAdvance = false) {
  play.state.input[0].press.button = button;
  advance = shouldAdvance;
  NativeFrame();
}
static void Stick(int x, int y) {
  play.state.input[0].rel.stick_x = x;
  play.state.input[0].rel.stick_y = y;
  NativeFrame();
}
static void Talk() {
  talkAccepted = true;
  NativeFrame();
  assert(sTalking && shop.actionFunc == EnOssan_BeginInteraction &&
         shop.cutsceneState == 1);
  assert(CutsceneManager_GetCurrentCsId() == CS_ID_NONE &&
         sBrowseCamera == CAM_ID_NONE);
  assert(player.stateFlags2 & PLAYER_STATE2_20000000);
}
static void ReachSlot(int slot) {
  // Native state/cutscene transitions only; input never waits for pose
  // settling.
  const auto wanted = slot < 0   ? EnOssan_FaceShopkeeper
                      : slot < 4 ? EnOssan_BrowseRightShelf
                                 : EnOssan_BrowseLeftShelf;
  for (int frames = 0; shop.actionFunc != wanted && frames < 16; ++frames) {
    if (shop.actionFunc == EnOssan_Hello)
      Press(BTN_A, true);
    else
      NativeFrame();
  }
  assert(shop.actionFunc == wanted && sTalking && NativeShopOwnCamera(&play));
  assert(sBrowseCameraPtr->setting == CAM_SET_BLACK_MARKET_OOT &&
         sBrowseCameraPtr->roll == 0);
  if (slot >= 0)
    assert(shop.cursorIndex == slot);
}
static void ReturnKeeper() {
  const bool right = shop.cursorIndex < 4;
  NativeFrame();
  Stick(right ? -80 : 80, 0);
  if (shop.actionFunc == EnOssan_BrowseRightShelf ||
      shop.actionFunc == EnOssan_BrowseLeftShelf) {
    NativeFrame();
    Stick(right ? -80 : 80, 0);
  }
  ReachSlot(-1);
}
static void CheckOotPivotTiming() {
  for (int side : {-1, 1}) {
    Reset(); Talk(); ReachSlot(-1);
    const auto *camera = sBrowseCameraPtr;
    const auto uid = sBrowseCameraUid;
    const auto eye = camera->eye;
    const auto cs = CutsceneManager_GetCurrentCsId();
    const auto look = side < 0 ? EnOssan_LookToLeftShelf : EnOssan_LookToRightShelf;
    const auto browse = side < 0 ? EnOssan_BrowseLeftShelf : EnOssan_BrowseRightShelf;
    Stick(side * 80, 0);
    // OoT's .5 approach, maximum 10 degrees and .5 snap reaches a shelf
    // in seven updates: 10, 20, 25, 27.5, 28.75, 29.375, 30 degrees.
    constexpr float angles[] = {10, 20, 25, 27.5f, 28.75f, 29.375f, 30};
    for (int frame = 0; frame < 7; ++frame) {
      NativeFrame();
      assert(camera->paramData.ootShop.faceAngle == -side * angles[frame]);
      assert(sBrowseCameraPtr == camera && sBrowseCameraUid == uid &&
             CutsceneManager_GetCurrentCsId() == cs &&
             "OoT shelf pans retain one camera and cutscene owner");
      assert(!std::memcmp(&eye, &camera->eye, sizeof(eye)) &&
             "OoT shop camera pivots from a fixed eye");
      assert(shop.actionFunc == (frame < 6 ? look : browse));
      if (frame < 6) assert(!shop.drawCursor && shop.stickAccumX == 0);
    }
    NativeFrame();
    Stick(-side * 80, 0);
    assert(shop.actionFunc == EnOssan_LookToShopkeeperFromShelf);
    constexpr float returning[] = {20, 10, 5, 2.5f, 1.25f, .625f, 0};
    for (int frame = 0; frame < 7; ++frame) {
      NativeFrame();
      assert(sBrowseCameraPtr == camera && sBrowseCameraUid == uid &&
             CutsceneManager_GetCurrentCsId() == cs);
      assert(!std::memcmp(&eye, &camera->eye, sizeof(eye)));
      assert(camera->paramData.ootShop.faceAngle == -side * returning[frame]);
      assert(shop.actionFunc == (frame < 6 ? EnOssan_LookToShopkeeperFromShelf : EnOssan_FaceShopkeeper));
    }
  }
  std::puts("MM OoT port: seven-frame native shelf turn, fixed eye and continuous camera ownership");
}
static void CheckLifecycle() {
  Reset();
  Talk();
  ReachSlot(-1);
  Stick(80, 0);
  ReachSlot(0);
  const int starts = playerWaits;
  Stick(0, 80);
  assert(shop.cursorIndex == 1);
  NativeFrame();
  Stick(80, 0);
  assert(shop.cursorIndex == 3);
  NativeFrame();
  Stick(0, -80);
  assert(shop.cursorIndex == 2);
  NativeFrame();
  Stick(-80, 0);
  assert(shop.cursorIndex == 0 && playerWaits == starts);
  ReturnKeeper();
  Stick(-80, 0);
  ReachSlot(4);
  Stick(0, 80);
  assert(shop.cursorIndex == 5);
  NativeFrame();
  Stick(-80, 0);
  assert(shop.cursorIndex == 7);
  NativeFrame();
  Stick(0, -80);
  assert(shop.cursorIndex == 6);
  NativeFrame();
  Stick(80, 0);
  assert(shop.cursorIndex == 4);
  const auto owned = sBrowseCamera;
  Press(BTN_B);
  assert(!sTalking && shop.actionFunc == EnOssan_Idle &&
         !(player.stateFlags2 & PLAYER_STATE2_20000000));
  assert(CutsceneManager_GetCurrentCsId() == CS_ID_NONE &&
         !play.cameraPtrs[owned]);
  assert(Play_GetActiveCamId(&play) == CAM_ID_MAIN &&
         playerWaits == playerEnds);
  Talk();
  ReachSlot(-1);
  std::puts("MM production native shop: actual initialization, all8 stock, "
            "native bank/return chain, one-B exit and re-talk passed");
}
static void CheckAbortBoundaries() {
  for (int reason = 0; reason < 4; ++reason) {
    Reset();
    Talk();
    ReachSlot(-1);
    const auto owned = sBrowseCamera;
    if (reason == 0)
      play.transitionTrigger = TRANS_TRIGGER_START;
    if (reason == 1)
      play.transitionMode = TRANS_MODE_FILL_OUT;
    if (reason == 2)
      play.gameOverCtx.state = GAMEOVER_DEATH_START;
    if (reason == 3)
      active = false;
    NativeFrame();
    assert(!sTalking && !(player.stateFlags2 & PLAYER_STATE2_20000000));
    assert(CutsceneManager_GetCurrentCsId() == CS_ID_NONE &&
           !play.cameraPtrs[owned]);
  }
  Reset();
  Talk();
  ReachSlot(-1);
  gPlayState = nullptr;
  MonitorBrowsing();
  assert(!sTalking && !sNativeShop && !sBrowseCameraPtr);
  std::puts("MM production native shop: transition, death, inactive scene and "
            "teardown release passed");
}
static void CheckForeignCutscene() {
  Reset();
  const auto foreign = Play_CreateSubCamera(&play);
  Play_ChangeCameraStatus(&play, foreign, CAM_STATUS_ACTIVE);
  talkAccepted = true;
  NativeFrame();
  assert(!sTalking && Play_GetActiveCamId(&play) == foreign);
  Reset();
  CutsceneManager_Start(CS_ID_GLOBAL_DOOR, &player.actor);
  talkAccepted = true;
  NativeFrame();
  assert(!sTalking && CutsceneManager_GetCurrentCsId() == CS_ID_GLOBAL_DOOR);
  Reset();
  CutsceneManager_Start(CS_ID_GLOBAL_TALK, &player.actor);
  Talk();
  ReachSlot(-1);
  std::puts("MM production native shop: foreign camera/cutscene rejection and "
            "global-talk handoff passed");
}
static void CheckSameCutsceneDifferentActor() {
  Reset();
  Talk();
  ReachSlot(-1);
  const auto id = shop.csId;
  CutsceneManager_Stop(id);
  CutsceneManager_Update();
  CutsceneManager_ClearWaiting();
  Actor foreign{};
  foreign.id = ACTOR_EN_OSSAN;
  assert(CutsceneManager_StartWithPlayerCsAndSetFlag(id, &foreign) == id);
  const auto camera = Play_GetActiveCamId(&play);
  const auto snapshot = *play.cameraPtrs[camera];
  assert(CutsceneManager_IsCurrentActor(&foreign) &&
         !CutsceneManager_IsCurrentActor(&shop.actor));
  MonitorBrowsing();
  if (sTalking || CutsceneManager_GetCurrentCsId() != id ||
      sCutsceneMgr.endCsId != CS_ID_NONE)
    std::fprintf(stderr,
                 "same-cs foreign actor: talking%d current%d end%d owned%d "
                 "uid%d/%d targetShop%d targetForeign%d\n",
                 sTalking, CutsceneManager_GetCurrentCsId(),
                 sCutsceneMgr.endCsId, NativeShopOwnCamera(&play),
                 sBrowseCameraUid, play.cameraPtrs[camera]->uid,
                 CutsceneManager_IsCurrentActor(&shop.actor),
                 CutsceneManager_IsCurrentActor(&foreign));
  assert(!sTalking && CutsceneManager_GetCurrentCsId() == id &&
         sCutsceneMgr.endCsId == CS_ID_NONE);
  CutsceneManager_Update();
  CutsceneManager_ClearWaiting();
  assert(CutsceneManager_IsCurrentActor(&foreign) &&
         Play_GetActiveCamId(&play) == camera);
  assert(!std::memcmp(&snapshot, play.cameraPtrs[camera], sizeof(Camera)));
  std::puts("MM production native shop: same cutscene ID owned by another "
            "actor retains full native camera/manager state");
}
static void CheckLiveKeeperDestruction() {
  for (bool alreadyStopped : {true, false}) {
    Reset();
    Talk();
    ReachSlot(-1);
    const auto owned = sBrowseCamera;
    if (alreadyStopped)
      CutsceneManager_Stop(shop.csId);
    assert(shop.actor.destroy == NativeKeeperDestroyed);
    shop.actor.destroy(&shop.actor, &play);
    assert(!sNativeShop && !sTalking);
    assert(lastLetterbox == 0 && lastHud == HUD_VISIBILITY_ALL);
    // Model Actor_Delete releasing its real allocation after Destroy. The
    // manager's deferred End must not touch the retired native controller.
    __asan_poison_memory_region(&shop, sizeof(shop));
    CutsceneManager_Update();
    CutsceneManager_ClearWaiting();
    __asan_unpoison_memory_region(&shop, sizeof(shop));
    assert(CutsceneManager_GetCurrentCsId() == CS_ID_NONE &&
           sCutsceneMgr.targetActor == nullptr);
    assert(!play.cameraPtrs[owned] &&
           Play_GetActiveCamId(&play) == CAM_ID_MAIN &&
           playerWaits == playerEnds);
  }
  std::puts(
      "MM production native shop: real Destroy before/after Stop releases "
      "deferred actor/camera/player ownership without retired-actor access");
}
static void SelectItem() {
  Press(BTN_A);
  assert(shop.actionFunc == EnOssan_SelectItem);
  for (int i = 0; i < 6; ++i)
    NativeFrame();
  assert(shop.shopItemSelectedTween == 1);
}
static void CheckCustomerAccessAndReturn() {
  for (int form = 0; form < 5; ++form) {
    Reset(); player.transformation = form;
    assert(NativeKeeperCanTalk(&shop, &play));
    Talk(); ReachSlot(-1); Press(BTN_B);
    assert(!sTalking && shop.actionFunc == EnOssan_Idle);
  }
  for (int invalid = 0; invalid < 6; ++invalid) {
    Reset();
    if (invalid == 0) player.actor.world.pos.z = 120;
    if (invalid == 1) player.actor.world.pos.y = player.actor.floorHeight = 41;
    if (invalid == 2) player.actor.floorBgId = 0;
    if (invalid == 3) player.actor.bgCheckFlags = 0;
    if (invalid == 4) player.stateFlags1 |= PLAYER_STATE1_200000;
    if (invalid == 5) player.actor.floorPoly = nullptr;
    talkAccepted = true; NativeFrame();
    assert(!sTalking && shop.actionFunc == EnOssan_Idle && !sReceipt.pending);
  }
  for (int interruption = 0; interruption < 7; ++interruption) {
    Reset(); Talk(); ReachSlot(-1);
    Stick(80, 0); ReachSlot(0); NativeFrame(); SelectItem();
    purchaseAccepted = true; Press(BTN_A, true);
    sReceipt.delivered = true; NativeFrame();
    assert(shop.actionFunc == EnOssan_SetupItemPurchased);
    play.msgCtx.msgMode = 0;
    player.stateFlags1 &= ~PLAYER_STATE1_400;
    player.actor.world.pos = {75, 105, 76};
    player.actor.prevPos = player.actor.home.pos = player.actor.world.pos;
    player.actor.floorBgId = 0;
    player.actor.velocity = {1, 2, 3};
    player.actor.speed = player.speedXZ = player.pushedSpeed = 4;
    if (interruption == 1) placementIdle = false;
    if (interruption == 2) ++mainCamera.uid;
    if (interruption == 3) customerFloorPresent = false;
    if (interruption == 4) customerFloorDynamic = true;
    if (interruption == 5) player.stateFlags1 |= PLAYER_STATE1_200000;
    if (interruption == 6) sCustomerAnchor.receipt++;
    NativeFrame();
    if (interruption == 0) {
      assert(player.actor.world.pos.x == 0 && player.actor.world.pos.y == 0 && player.actor.world.pos.z == 160);
      assert(!std::memcmp(&player.actor.world.pos, &player.actor.prevPos, sizeof(Vec3f)) &&
             !std::memcmp(&player.actor.world.pos, &player.actor.home.pos, sizeof(Vec3f)));
      assert(player.actor.floorBgId == BGCHECK_SCENE && player.actor.floorPoly == &customerFloor);
      assert(player.actor.velocity.x == 0 && player.actor.velocity.y == 0 && player.actor.velocity.z == 0 &&
             player.actor.speed == 0 && player.speedXZ == 0 && player.pushedSpeed == 0);
    } else {
      assert(player.actor.world.pos.x == 75 && player.actor.world.pos.y == 105 && player.actor.world.pos.z == 76);
      if (interruption == 1) {
        assert(sReceipt.pending);
        placementIdle = true;
        NativeFrame();
        assert(!sReceipt.pending && player.actor.world.pos.x == 75 &&
               player.actor.world.pos.y == 105 && player.actor.world.pos.z == 76 &&
               "foreign release must not revive the retired customer anchor");
      }
    }
  }
  // Native global TALK/DOOR keep the main camera and bypass the event queue.
  // Their release must not revive a position captured before the foreign owner.
  for (int owner = 0; owner < 6; ++owner) {
    Reset(); Talk(); ReachSlot(-1);
    Stick(80, 0); ReachSlot(0); NativeFrame(); SelectItem();
    purchaseAccepted = true; Press(BTN_A, true);
    sReceipt.delivered = true; NativeFrame();
    assert(shop.actionFunc == EnOssan_SetupItemPurchased);
    Actor foreignActor{};
    const s16 cs = owner == 0 ? CS_ID_GLOBAL_TALK : owner == 1 ? CS_ID_GLOBAL_DOOR : 2;
    play.playerCsIds[PLAYER_CS_ID_ITEM_GET] = 2;
    player.talkActor = &foreignActor;
    player.actor.world.pos = {75, 105, 76};
    player.actor.floorBgId = 0;
    play.msgCtx.msgMode = 1;
    // A get-item flag and matching ID alone do not establish player ownership.
    if (owner >= 2) player.stateFlags1 |= PLAYER_STATE1_400;
    CutsceneManager_Start(cs, owner >= 3 ? &player.actor : &foreignActor);
    s16 foreignCamera = CAM_ID_NONE;
    if (owner == 4) {
      foreignCamera = Play_CreateSubCamera(&play);
      Play_ChangeCameraStatus(&play, foreignCamera, CAM_STATUS_ACTIVE);
    }
    if (owner == 5)
      ++Play_GetCamera(&play, Play_GetActiveCamId(&play))->uid;
    NativeFrame();
    assert(sReceipt.pending && player.actor.world.pos.x == 75);
    assert((sCustomerAnchor.play == &play) == (owner == 3));
    CutsceneManager_Stop(cs);
    // Player_Action_65 clears GET_ITEM before the keeper's update; manager End
    // is deferred until after actor updates in this same frame.
    play.msgCtx.msgMode = 0;
    player.stateFlags1 &= ~PLAYER_STATE1_400;
    NativeFrame();
    assert(sReceipt.pending && (sCustomerAnchor.play == &play) == (owner == 3));
    if (owner >= 4) {
      if (foreignCamera != CAM_ID_NONE) Play_ClearCamera(&play, foreignCamera);
      Play_ChangeCameraStatus(&play, CAM_ID_MAIN, CAM_STATUS_ACTIVE);
    }
    NativeFrame();
    assert(!sReceipt.pending);
    if (owner == 3)
      assert(player.actor.world.pos.x == 0 && player.actor.world.pos.y == 0 && player.actor.world.pos.z == 160);
    else
      assert(player.actor.world.pos.x == 75 && player.actor.world.pos.y == 105 && player.actor.world.pos.z == 76 &&
             "foreign native cutscene release must not revive the retired customer anchor");
  }
  std::puts("MM customer safety: all5 forms can talk from customer floor; counter/shelf/climb starts rejected; receipt returns to validated floor only under unchanged ownership, including native talk/door/item release");
}
static void CheckProductionReceipt() {
  Reset();
  Talk();
  ReachSlot(-1);
  Stick(80, 0);
  ReachSlot(0);
  NativeFrame();
  SelectItem();
  const auto bankCamera = sBrowseCamera;
  Press(BTN_A, true);
  assert(shop.actionFunc == EnOssan_CannotBuy && purchases == 1 &&
         !sReceipt.pending);
  assert(sBrowseCamera == bankCamera &&
         CutsceneManager_GetCurrentCsId() == shop.csId);
  Press(BTN_A, true);
  for (int i = 0; i < 9; ++i)
    NativeFrame();
  SelectItem();
  purchaseAccepted = true;
  Press(BTN_A, true);
  assert(shop.actionFunc == EnOssan_BuyItemWithFanfare && sReceipt.pending &&
         purchases == 2);
  assert(!sTalking && !(player.stateFlags2 & PLAYER_STATE2_20000000));
  assert(GameInteractor_ShouldActorDraw(&player.actor) && "native receipt releases replacement rendering");
  assert(CutsceneManager_GetCurrentCsId() == CS_ID_NONE &&
         Play_GetActiveCamId(&play) == CAM_ID_MAIN);
  for (int i = 0; i < 3; ++i)
    NativeFrame();
  assert(purchases == 2 && shop.actionFunc == EnOssan_BuyItemWithFanfare &&
         callbacks.empty());
  // Neither native stock callbacks nor native Actor_OfferGetItem may perform
  // a second grant/payment alongside the market's acknowledged receipt.
  sReceipt.delivered = true;
  NativeFrame();
  assert(shop.actionFunc == EnOssan_SetupItemPurchased);
  play.msgCtx.msgMode = 0;
  messageState = TEXT_STATE_DONE;
  player.stateFlags1 |= PLAYER_STATE1_400;
  Press(BTN_A, true);
  assert(sReceipt.pending && shop.actionFunc == EnOssan_SetupItemPurchased);
  player.stateFlags1 &= ~PLAYER_STATE1_400;
  safe = false;
  NativeFrame();
  assert(sReceipt.pending && shop.actionFunc == EnOssan_SetupItemPurchased);
  safe = true;
  NativeFrame();
  assert(!sReceipt.pending && shop.actionFunc == EnOssan_ItemPurchased);
  talkAccepted = true;
  NativeFrame();
  assert(shop.actionFunc == EnOssan_ContinueShopping &&
         shop.csId == shop.lookToShopkeeperCsId);
  assert(play.msgCtx.currentTextId == kContinueText &&
         messageState == TEXT_STATE_CHOICE);
  status[0] = PurchaseResult::SoldOut;
  play.msgCtx.choiceIndex = 0;
  Press(BTN_A, true);
  assert(shop.actionFunc == EnOssan_FaceShopkeeper &&
         play.msgCtx.currentTextId == kKeeperMenuText);
  assert(stock[0].isOutOfStock && stock[0].actor.world.pos.x == kShelves[0].x &&
         stock[0].actor.world.pos.z == kShelves[0].z && callbacks.empty());
  Press(BTN_B);
  assert(!sTalking && shop.actionFunc == EnOssan_Idle);
  Reset();
  Talk();
  ReachSlot(-1);
  Stick(80, 0);
  ReachSlot(0);
  NativeFrame();
  SelectItem();
  purchaseAccepted = true;
  Press(BTN_A, true);
  receiptContext = false;
  NativeFrame();
  assert(shop.actionFunc == EnOssan_Idle && !sReceipt.pending);
  std::puts("MM production native shop: rejected reservation, single receipt "
            "authority, pending/tutorial gating, native keeper "
            "continue/restock and context cancellation passed");
}
static void CheckDestructionWithForeignOwnerAndReceipt() {
  for (bool sameCutscene : {false, true}) {
    Reset();
    Talk();
    ReachSlot(-1);
    Actor foreignActor{};
    s16 foreign;
    if (sameCutscene) {
      const auto cs = shop.csId;
      CutsceneManager_Stop(cs);
      CutsceneManager_Update();
      CutsceneManager_ClearWaiting();
      assert(CutsceneManager_StartWithPlayerCsAndSetFlag(cs, &foreignActor) ==
             cs);
      foreign = Play_GetActiveCamId(&play);
    } else {
      foreign = Play_CreateSubCamera(&play);
      Play_ChangeCameraStatus(&play, foreign, CAM_STATUS_ACTIVE);
    }
    auto *camera = play.cameraPtrs[foreign];
    camera->eye = camera->eyeNext = {901, 902, 903};
    camera->at = {801, 802, 803};
    camera->fov = 39;
    camera->target = &foreignActor;
    const auto snapshot = *camera;
    lastHud = 73;
    lastLetterbox = 17;
    hudCalls = letterboxCalls = 0;
    shop.actor.destroy(&shop.actor, &play);
    __asan_poison_memory_region(&shop, sizeof(shop));
    CutsceneManager_Update();
    CutsceneManager_ClearWaiting();
    __asan_unpoison_memory_region(&shop, sizeof(shop));
    assert(Play_GetActiveCamId(&play) == foreign &&
           !std::memcmp(&snapshot, camera, sizeof(Camera)));
    assert(lastHud == 73 && lastLetterbox == 17 && !hudCalls &&
           !letterboxCalls);
    if (sameCutscene)
      assert(CutsceneManager_IsCurrentActor(&foreignActor));
  }
  for (int receipt = 0; receipt < 3; ++receipt) {
    Reset();
    Talk();
    ReachSlot(-1);
    Stick(80, 0);
    ReachSlot(0);
    NativeFrame();
    SelectItem();
    purchaseAccepted = true;
    Press(BTN_A, true);
    assert(sReceipt.pending);
    if (receipt == 1)
      sReceipt.delivered = true;
    if (receipt == 2)
      sReceipt.Cancel();
    // A room can destroy stock before its NPC. The keeper's destructor must
    // retire references without reading those already retired allocations.
    __asan_poison_memory_region(stock.data(), sizeof(stock));
    shop.actor.destroy(&shop.actor, &play);
    assert(!sReceipt.pending && !sNativeShop);
    __asan_poison_memory_region(&shop, sizeof(shop));
    CutsceneManager_Update();
    CutsceneManager_ClearWaiting();
    __asan_unpoison_memory_region(&shop, sizeof(shop));
    __asan_unpoison_memory_region(stock.data(), sizeof(stock));
  }
  std::puts("MM production native shop: destruction preserves "
            "same-CS/other-camera foreign owners and safely retires "
            "pending/delivered/cancelled receipts and stock");
}
static void ProjectNativeCamera(const Camera &camera) {
  auto dot = [](Vec3f a, Vec3f b) { return a.x * b.x + a.y * b.y + a.z * b.z; };
  Vec3f forward{camera.at.x - camera.eye.x, camera.at.y - camera.eye.y,
                camera.at.z - camera.eye.z};
  const auto length = std::sqrt(dot(forward, forward));
  forward = {forward.x / length, forward.y / length, forward.z / length};
  const auto horizontal = std::hypot(forward.x, forward.z);
  const Vec3f right{-forward.z / horizontal, 0, forward.x / horizontal};
  const Vec3f up{-right.z * forward.y,
                 right.z * forward.x - right.x * forward.z,
                 right.x * forward.y};
  const auto half = std::tan(camera.fov * 3.14159265359f / 360);
  auto &matrix = play.viewProjectionMtxF;
  matrix = {};
  matrix.xx = right.x / half * .75f;
  matrix.xy = right.y / half * .75f;
  matrix.xz = right.z / half * .75f;
  matrix.xw = -dot(camera.eye, right) / half * .75f;
  matrix.yx = up.x / half;
  matrix.yy = up.y / half;
  matrix.yz = up.z / half;
  matrix.yw = -dot(camera.eye, up) / half;
  matrix.wx = forward.x;
  matrix.wy = forward.y;
  matrix.wz = forward.z;
  matrix.ww = -dot(camera.eye, forward);
}
static void CheckAuthoredFraming() {
  for (int bank = 0; bank < 2; ++bank) {
    Reset();
    Talk();
    ReachSlot(-1);
    Stick(bank == 0 ? 80 : -80, 0);
    ReachSlot(bank * 4);
    // These idle frames establish a static framing measurement only; the
    // actual interaction is already in its native Browse state.
    for (int i = 0; i < 20; ++i)
      NativeFrame();
    const auto id = sBrowseCamera, uid = sBrowseCameraUid;
    auto snapshot = *sBrowseCameraPtr;
    assert(snapshot.fov == 60);
    const auto cameraRecord =
        BlackMarketMmShopScene::kCutsceneCameras[bank == 0 ? 2 : 1];
    const auto &record = BlackMarketMmShopScene::kCameraRecords[cameraRecord];
    assert(snapshot.eye.x == record[0] && snapshot.eye.y == record[1] &&
           snapshot.eye.z == record[2]);
    for (int slot = bank * 4; slot < bank * 4 + 4; ++slot) {
      shop.cursorIndex = slot;
      ProjectNativeCamera(*sBrowseCameraPtr);
      NativeFrame();
      NativeFrame();
      assert(shop.drawCursor && sBrowseCamera == id && sBrowseCameraUid == uid);
      assert(!std::memcmp(&snapshot.eye, &sBrowseCameraPtr->eye, sizeof(Vec3f)) &&
             snapshot.fov == sBrowseCameraPtr->fov);
      // Native Data4 can change the at-point distance. The viewing direction
      // must stay fixed; comparing the at point itself rejects donor behavior.
      const auto before = OLib_Vec3fDiffToVecGeo(&snapshot.eye, &snapshot.at);
      const auto after = OLib_Vec3fDiffToVecGeo(&sBrowseCameraPtr->eye, &sBrowseCameraPtr->at);
      assert(std::abs(before.yaw - after.yaw) <= 1 && std::abs(before.pitch - after.pitch) <= 1);
      assert(shop.cursorPos.x - 24 > 8 && shop.cursorPos.x + 24 < 312 &&
             shop.cursorPos.y - 24 > 32 && shop.cursorPos.y + 24 < 142 &&
             "all4 native48px cursors must clear the actual32px letterbox and "
             "bottom textbox in one bank view");
      bool rectangle = false;
      for (auto *packet = overlay.data(); packet < graphics.overlay.p;
           ++packet) {
        if ((packet->words.w0 >> 24) != G_TEXRECT)
          continue;
        assert(((packet->words.w0 >> 12) & 0xFFF) ==
               int((shop.cursorPos.x + 24) * 4));
        assert((packet->words.w0 & 0xFFF) == int((shop.cursorPos.y + 24) * 4));
        rectangle = true;
        break;
      }
      assert(rectangle);
      const float x = shop.cursorPos.x;
      mirror = true;
      NativeFrame();
      assert(shop.cursorPos.x == 320 - x);
      mirror = false;
    }
  }
  std::puts("MM production native shop: ported OoT Data4 native60FOV, "
            "immutable bank view, actualall8 48px cursor projection/GBI above "
            "textbox and32px letterbox passed");
}
static float ReferenceItemPixels(const Actor &item) {
  // A 40-unit reference mesh is measured through the real actor projection,
  // after native actions have moved the actual stock actor. This catches the
  // previous tiny confirmation view, which the clipping-only test accepted.
  Actor marker{};
  marker.focus.pos = item.world.pos;
  s16 x, bottom, top;
  Actor_GetScreenPos(&play, &marker, &x, &bottom);
  marker.focus.pos.y += 40 * item.scale.y;
  Actor_GetScreenPos(&play, &marker, &x, &top);
  return std::abs(bottom - top);
}
static void CheckInspectionLetterboxBoundary() {
  auto begin = [](int form, int slot) {
    Reset();
    player.transformation = form;
    Talk();
    ReachSlot(-1);
    Stick(slot < 4 ? 80 : -80, 0);
    ReachSlot(slot < 4 ? 0 : 4);
    for (int frame = 0; frame < 20; ++frame)
      NativeFrame();
    shop.cursorIndex = slot;
    assert(NativeVisibleTop() == 34);
    Press(BTN_A);
    assert(shop.actionFunc == EnOssan_SelectItem &&
           shop.shopItemSelectedTween == 0);
    assert(NativeVisibleTop() == 2 && "native scissor and bars must release "
                                      "before the first selected motion");
  };
  for (int form = 0; form < 5; ++form)
    for (int slot = 0; slot < 8; ++slot) {
      begin(form, slot);
      const auto eye = sBrowseCameraPtr->eye;
      for (int frame = 0; frame < 6; ++frame) {
        NativeFrame();
        assert(lastFrameVisibleTop == 2);
      }
      assert(shop.shopItemSelectedTween == 1 &&
             ShrinkWindow_Letterbox_GetSizeTarget() == 32 &&
             ShrinkWindow_Letterbox_GetSize() == 32);
      Press(BTN_B);
      assert(lastFrameVisibleTop == 2);
      for (int frame = 0; frame < 6; ++frame) {
        NativeFrame();
        assert(lastFrameVisibleTop == 2);
      }
      assert(shop.shopItemSelectedTween == 0 && NativeVisibleTop() == 2);
      NativeFrame();
      assert(NativeVisibleTop() == 34 &&
             !std::memcmp(&eye, &sBrowseCameraPtr->eye, sizeof(eye)));
      // Accepted receipt resets tween0 while the selected actor still draws
      // at its endpoint through the final frame of the owned shop camera.
      begin(form, slot);
      for (int frame = 0; frame < 6; ++frame)
        NativeFrame();
      purchaseAccepted = true;
      Press(BTN_A, true);
      assert(sReceipt.pending && shop.shopItemSelectedTween == 0 &&
             lastFrameVisibleTop == 2);
      assert(!NativeShopOwnCamera(&play) && NativeVisibleTop() == 34);
    }
  for (int edge = 0; edge < 18; ++edge) {
    begin(PLAYER_FORM_HUMAN, 0);
    for (int frame = 0; frame < 6; ++frame)
      NativeFrame();
    Actor other{};
    PlayState otherPlay{};
    GameState otherGame{};
    if (edge == 0)
      play.pauseCtx.state = PAUSE_STATE_MAIN;
    if (edge == 1)
      ++gSaveContext.save.shipSaveInfo.rando.finalSeed;
    if (edge == 2)
      ++gSaveContext.fileNum;
    if (edge == 3)
      shop.cursorIndex = 1;
    if (edge == 4)
      stock[0].actor.update = nullptr;
    if (edge == 5) {
      const auto cs = shop.csId;
      CutsceneManager_Stop(cs);
      CutsceneManager_Update();
      CutsceneManager_ClearWaiting();
      assert(CutsceneManager_StartWithPlayerCsAndSetFlag(cs, &other) == cs);
    }
    if (edge == 6) {
      const auto foreign = Play_CreateSubCamera(&play);
      Play_ChangeCameraStatus(&play, foreign, CAM_STATUS_ACTIVE);
    }
    if (edge == 7)
      ++sBrowseCameraPtr->uid;
    if (edge == 8)
      sBrowseCameraPtr->setting = CAM_SET_FIXED1;
    if (edge == 9)
      gPlayState = &otherPlay;
    if (edge == 10)
      gGameState = &otherGame;
    if (edge == 11)
      play.transitionTrigger = TRANS_TRIGGER_START;
    if (edge == 12)
      active = false;
    if (edge == 13)
      shop.actor.destroy(&shop.actor, &play);
    if (edge == 14)
      shop.items[0] = &stock[1];
    if (edge == 15)
      play.state.running = false;
    if (edge == 16)
      safe = false;
    if (edge == 17)
      play.gameOverCtx.state = static_cast<decltype(play.gameOverCtx.state)>(1);
    assert(
        NativeVisibleTop() == 34 &&
        "foreign/paused/stale inspection must retain native letterbox policy");
    // A mismatch observed during native update cannot re-arm the same Select
    // action under a different seed/file/slot; only a new selection may arm.
    if (edge >= 1 && edge <= 3) {
      NativeFrame();
      assert(NativeVisibleTop() == 34);
    }
  }
  // Run the actual existing DisableBlackBars registration/CVar boundary.
  // The shop hook must never override another hook's true result.
  begin(PLAYER_FORM_HUMAN, 0);
  userDisableBlackBars = true;
  RegisterDisableBlackBars();
  EndBrowsing(&play);
  assert(NativeVisibleTop() == 2 && userDisableBlackBars);
  userDisableBlackBars = false;
  RegisterDisableBlackBars();
  assert(NativeVisibleTop() == 34 && !userDisableBlackBars);
  std::puts(
      "MM inspection letterbox: real VB registry, native view scissor/black "
      "rectangle writers, all5 forms/all8 take/return/receipt first-frame, "
      "current/target32 and external CVar preserved; "
      "pause/seed/file/slot/item/owner/UID/scene/destroy controls passed");
}
static void CheckSelectedItemLegibility() {
  for (int form = 0; form < 5; ++form) {
    for (int slot = 0; slot < 8; ++slot) {
      Reset();
      player.transformation = form;
      Talk();
      ReachSlot(-1);
      Stick(slot < 4 ? 80 : -80, 0);
      ReachSlot(slot < 4 ? 0 : 4);
      for (int i = 0; i < 20; ++i)
        NativeFrame();
      shop.cursorIndex = slot;
      NativeFrame();
      ProjectNativeCamera(*sBrowseCameraPtr);
      const auto shelfPixels = ReferenceItemPixels(stock[slot].actor);
      const auto shelfPosition = stock[slot].actor.world.pos;
      const auto shelfScale = stock[slot].actor.scale;
      assert(shelfScale.x == .27f && shelfScale.y == .27f && shelfScale.z == .27f);
      assert(shelfPixels >= 16 && "shelf stock must remain visibly larger "
                                  "through actual native updates");
      SelectItem();
      ProjectNativeCamera(*sBrowseCameraPtr);
      const auto selectedPixels = ReferenceItemPixels(stock[slot].actor);
      assert(!std::memcmp(&shelfScale, &stock[slot].actor.scale, sizeof(Vec3f)));
      const auto distance = [&](Vec3f point) {
        return OLib_Vec3fDist(&sBrowseCameraPtr->eye, &point);
      };
      std::fprintf(stderr,
                   "form%d slot%d shelf %.0fpx/%.3f units selected %.0fpx/%.3f "
                   "units scale %.2f\n",
                   form, slot, shelfPixels, distance(shelfPosition),
                   selectedPixels, distance(stock[slot].actor.world.pos),
                   stock[slot].actor.scale.y);
      assert(selectedPixels >= 35 && selectedPixels >= 1.9f * shelfPixels &&
             distance(stock[slot].actor.world.pos) <= 64 &&
             "inspection must bring selected stock visibly closer with its "
             "actual native projection");
      // The shelf display itself is now closer. Preserve the established
      // absolute confirmation size and substantial additional enlargement.
      assert(
          selectedPixels >= 20 && selectedPixels >= 1.5f * shelfPixels &&
          "confirmation must visibly enlarge merchandise, not leave it tiny");
      Press(BTN_B);
      for (int i = 0; i < 10; ++i)
        NativeFrame();
      const auto &position = stock[slot].actor.world.pos;
      assert(position.x == kShelves[slot].x && position.y == kShelves[slot].y &&
             position.z == kShelves[slot].z && shop.shopItemSelectedTween == 0);
      assert(
          !std::memcmp(&shelfScale, &stock[slot].actor.scale, sizeof(Vec3f)));
    }
  }
  std::puts("MM selected-item legibility: all5 forms/all8 native selections "
            "project >=35px and >=1.9x shelf size within64 units; B restores "
            "exact shelf "
            "anchors and scale");
}
static void CheckImmediateCannotBuyFraming() {
  for (int form = 0; form < 5; ++form) {
    for (int slot = 0; slot < 8; ++slot) {
      Reset();
      player.transformation = form;
      Talk();
      ReachSlot(-1);
      Stick(slot < 4 ? 80 : -80, 0);
      ReachSlot(slot < 4 ? 0 : 4);
      for (int i = 0; i < 20; ++i)
        NativeFrame();
      shop.cursorIndex = slot;
      Press(BTN_A);
      for (int i = 0; i < 5; ++i)
        NativeFrame();
      assert(shop.shopItemSelectedTween < 1.f);
      // Confirm on the very frame the native tween first reaches 1; there is
      // no idle SelectItem frame to refresh a camera target after that step.
      Press(BTN_A, true);
      assert(shop.actionFunc == EnOssan_CannotBuy);
      for (int frame = 0; frame < 12; ++frame) {
        ProjectNativeCamera(*sBrowseCameraPtr);
        for (int corner = 0; corner < 8; ++corner) {
          Actor marker{};
          marker.focus.pos = stock[slot].actor.world.pos;
          marker.focus.pos.x += corner & 1 ? 12.967f : -12.967f;
          marker.focus.pos.y += corner & 2 ? 19.810f : -9.010f;
          marker.focus.pos.z += corner & 4 ? 12.767f : -12.767f;
          s16 x, y;
          Actor_GetScreenPos(&play, &marker, &x, &y);
          assert(x > 8 && x < 312 && y > NativeVisibleTop() && y < 138 &&
                 "immediate CannotBuy must retain the entire selected "
                 "item/seal frame");
        }
        NativeFrame();
      }
      Press(BTN_A, true);
      for (int frame = 0; frame < 12; ++frame)
        NativeFrame();
      const auto &record =
          BlackMarketMmShopScene::kCameraRecords[slot < 4 ? 3 : 2];
      assert(shop.shopItemSelectedTween == 0 &&
             sBrowseCameraPtr->eye.y == record[1]);
    }
  }
  std::puts("MM immediate first-complete-frame confirmation: all5 forms/all8 "
            "CannotBuy holds/return retain item/seal framing and restore "
            "native bank target");
}
static void DumpSelectionPaths() {
  std::printf("[");
  for (int form = 0; form < 5; ++form) {
    for (int slot = 0; slot < 8; ++slot) {
      Reset();
      player.transformation = form;
      Talk();
      ReachSlot(-1);
      Stick(slot < 4 ? 80 : -80, 0);
      ReachSlot(slot < 4 ? 0 : 4);
      for (int i = 0; i < 20; ++i)
        NativeFrame();
      if (form || slot)
        std::printf(",");
      std::printf("[");
      shop.cursorIndex = slot;
      const auto eye = sBrowseCameraPtr->eye;
      const auto direction =
          OLib_Vec3fDiffToVecGeo(&sBrowseCameraPtr->eye, &sBrowseCameraPtr->at);
      auto printPosition = [&]() {
        // The native tween stays linear on every axis, with no item-driven
        // camera motion during take/return (including interpolation endpoints).
        const auto &start = kShelves[slot];
        const auto &end = BlackMarketMmShopScene::kSelectedTargets[slot >> 2];
        const auto t = shop.shopItemSelectedTween;
        const auto &itemPosition = stock[slot].actor.world.pos;
        assert(std::abs(itemPosition.x - (start.x + (end[0] - start.x) * t)) <
               .0001f);
        assert(std::abs(itemPosition.y - (start.y + (end[1] - start.y) * t)) <
               .0001f);
        assert(std::abs(itemPosition.z - (start.z + (end[2] - start.z) * t)) <
               .0001f);
        assert(!std::memcmp(&eye, &sBrowseCameraPtr->eye, sizeof(eye)));
        const auto now = OLib_Vec3fDiffToVecGeo(&sBrowseCameraPtr->eye,
                                                &sBrowseCameraPtr->at);
        assert(std::abs(direction.pitch - now.pitch) <= 1 &&
               std::abs(direction.yaw - now.yaw) <= 1);
        const auto &position = stock[slot].actor.world.pos;
        const auto &camera = *sBrowseCameraPtr;
        std::printf("{\"position\":[%.8f,%.8f,%.8f],\"eye\":[%.8f,%.8f,%.8f],"
                    "\"at\":[%.8f,%.8f,%.8f],\"fov\":%.8f,\"visibleTop\":%.0f}",
                    position.x, position.y, position.z, camera.eye.x,
                    camera.eye.y, camera.eye.z, camera.at.x, camera.at.y,
                    camera.at.z, camera.fov, NativeVisibleTop());
      };
      printPosition();
      Press(BTN_A);
      std::printf(","); printPosition();
      for (int step = 0; step < 6; ++step) {
        NativeFrame();
        std::printf(",");
        printPosition();
      }
      assert(shop.shopItemSelectedTween == 1);
      Press(BTN_B);
      std::printf(","); printPosition();
      for (int step = 0; step < 6; ++step) {
        NativeFrame();
        std::printf(",");
        printPosition();
      }
      assert(shop.shopItemSelectedTween == 0);
      NativeFrame(); std::printf(","); printPosition();
      assert(NativeVisibleTop() == 34);
      std::printf("]");
    }
  }
  std::puts("]");
}
static void CheckForeignCameraTakeover() {
  for (s16 requestedId : {s16(2), s16(3)}) {
    Reset();
    Talk();
    ReachSlot(-1);
    // Protection belongs to the private scene, including subsequent native
    // Start/End pairs after an ordinary keeper -> bank -> keeper trip.
    Stick(80, 0);
    ReachSlot(0);
    ReturnKeeper();
    const auto ownId = sBrowseCamera;
    s16 fillerId = CAM_ID_NONE;
    Camera filler{};
    if (requestedId == 3) {
      fillerId = Play_CreateSubCamera(&play);
      assert(fillerId == 2);
      filler = *Play_GetCamera(&play, fillerId);
    }
    const auto foreignId = Play_CreateSubCamera(&play);
    assert(foreignId == requestedId && foreignId != ownId);
    auto *foreign = Play_GetCamera(&play, foreignId);
    foreign->eye = foreign->eyeNext = {901, 902, 903};
    foreign->at = {801, 802, 803};
    foreign->fov = 37;
    Play_ChangeCameraStatus(&play, foreignId, CAM_STATUS_ACTIVE);
    const auto snapshot = *foreign;
    MonitorBrowsing();
    assert(!sTalking && Play_GetActiveCamId(&play) == foreignId);
    assert(foreign->status == CAM_STATUS_ACTIVE &&
           foreign->uid == snapshot.uid);
    CutsceneManager_Update();
    CutsceneManager_ClearWaiting();
    assert(Play_GetActiveCamId(&play) == foreignId &&
           play.cameraPtrs[ownId] == nullptr);
    if (std::memcmp(&snapshot.eye, &foreign->eye, sizeof(Vec3f)) ||
        foreign->uid != snapshot.uid) {
      std::fprintf(stderr,
                   "FOREIGN CAMERA OVERWRITE: id=%d uid=%d->%d "
                   "eye=(%.0f,%.0f,%.0f)->(%.0f,%.0f,%.0f) stored=%d\n",
                   foreignId, snapshot.uid, foreign->uid, snapshot.eye.x,
                   snapshot.eye.y, snapshot.eye.z, foreign->eye.x,
                   foreign->eye.y, foreign->eye.z, sCutsceneMgr.isCameraStored);
    }
    assert(!std::memcmp(&snapshot, foreign, sizeof(Camera)) &&
           "shop release must preserve a foreign camera's identity and pose");
    CutsceneManager_Update();
    CutsceneManager_ClearWaiting();
    assert(Play_GetActiveCamId(&play) == foreignId &&
           sCutsceneMgr.retCamId == foreignId);
    assert(!std::memcmp(&snapshot, foreign, sizeof(Camera)));
    if (fillerId != CAM_ID_NONE)
      assert(!std::memcmp(&filler, Play_GetCamera(&play, fillerId),
                          sizeof(Camera)));
  }
  std::puts("MM native: foreign camera IDs 2/3 retain active status, UID and "
            "full camera state");
}

static void CheckOwnedSlotReuse(bool stopFirst = false) {
  Reset();
  Talk();
  ReachSlot(-1);
  const auto ownedId = sBrowseCamera;
  const auto ownedUid = sBrowseCameraUid;
  if (stopFirst) {
    EndBrowsing(&play);
    assert(sCutsceneMgr.endCsId != CS_ID_NONE &&
           "native Stop must await End before camera cleanup");
  }
  Play_ClearCamera(&play, ownedId);
  const auto foreignId = Play_CreateSubCamera(&play);
  assert(foreignId == ownedId);
  auto *foreign = Play_GetCamera(&play, foreignId);
  assert(foreign->uid != ownedUid &&
         "native reused slot must have a fresh UID");
  foreign->eye = foreign->eyeNext = {711, 712, 713};
  foreign->at = {611, 612, 613};
  foreign->fov = 39;
  Play_ChangeCameraStatus(&play, foreignId, CAM_STATUS_ACTIVE);
  const auto snapshot = *foreign;
  if (!stopFirst)
    MonitorBrowsing();
  assert(!sTalking && !(player.stateFlags2 & (1u << 29)));
  CutsceneManager_Update();
  CutsceneManager_ClearWaiting();
  assert(play.cameraPtrs[foreignId] == foreign &&
         Play_GetActiveCamId(&play) == foreignId &&
         "deferred shop End must not clear a reused slot owned by a new native "
         "camera UID");
  assert(!std::memcmp(&snapshot, foreign, sizeof(Camera)) &&
         "deferred shop End must preserve all camera state after same-ID UID "
         "replacement");
  assert(CutsceneManager_GetCurrentCsId() == CS_ID_NONE &&
         playerWaits == playerEnds);
  assert(!(keeper.actor.flags & ACTOR_FLAG_FREEZE_EXCEPTION));
  CutsceneManager_Update();
  CutsceneManager_ClearWaiting();
  assert(Play_GetActiveCamId(&play) == foreignId &&
         sCutsceneMgr.retCamId == foreignId);
  assert(!std::memcmp(&snapshot, foreign, sizeof(Camera)));
  Play_ClearCamera(&play, foreignId);
  Play_ChangeCameraStatus(&play, CAM_ID_MAIN, CAM_STATUS_ACTIVE);
  Talk();
  ReachSlot(-1);
  Press(BTN_B);
  NativeFrame();
  assert(CutsceneManager_GetCurrentCsId() == CS_ID_NONE &&
         Play_GetActiveCamId(&play) == CAM_ID_MAIN);
  std::printf("MM native: same-slot UID replacement %s Stop preserves foreign "
              "state through deferred End and re-talk\n",
              stopFirst ? "after" : "before");
}

static void CheckOwnedSlotCleared(bool stopFirst = false) {
  Reset();
  Talk();
  ReachSlot(-1);
  const auto ownedId = sBrowseCamera;
  if (stopFirst) {
    EndBrowsing(&play);
    assert(sCutsceneMgr.endCsId != CS_ID_NONE);
  }
  Play_ClearCamera(&play, ownedId);
  Play_ChangeCameraStatus(&play, CAM_ID_MAIN, CAM_STATUS_ACTIVE);
  const auto snapshot = mainCamera;
  if (!stopFirst)
    MonitorBrowsing();
  assert(!sTalking && !(player.stateFlags2 & (1u << 29)));
  // This executes the real deferred End, including its former NULL CUR_CAM
  // dereference. The manager must still release player/keeper cutscene state.
  CutsceneManager_Update();
  CutsceneManager_ClearWaiting();
  assert(play.cameraPtrs[ownedId] == nullptr &&
         play.cameraPtrs[CAM_ID_MAIN] == &mainCamera);
  assert(Play_GetActiveCamId(&play) == CAM_ID_MAIN);
  assert(!std::memcmp(&snapshot, &mainCamera, sizeof(Camera)));
  assert(CutsceneManager_GetCurrentCsId() == CS_ID_NONE &&
         playerWaits == playerEnds);
  assert(!(keeper.actor.flags & ACTOR_FLAG_FREEZE_EXCEPTION));
  CutsceneManager_Update();
  CutsceneManager_ClearWaiting();
  assert(Play_GetActiveCamId(&play) == CAM_ID_MAIN &&
         sCutsceneMgr.retCamId == CAM_ID_MAIN);
  assert(!std::memcmp(&snapshot, &mainCamera, sizeof(Camera)));
  Talk();
  ReachSlot(-1);
  Press(BTN_B);
  NativeFrame();
  assert(CutsceneManager_GetCurrentCsId() == CS_ID_NONE &&
         Play_GetActiveCamId(&play) == CAM_ID_MAIN);
  std::printf("MM native: owned slot cleared %s Stop preserves main camera "
              "through deferred End and re-talk\n",
              stopFirst ? "after" : "before");
}

static void CheckReturnSlotOwnership() {
  for (s16 returnId : {s16(CAM_ID_MAIN), s16(2)})
    for (bool reuse : {false, true}) {
      Reset();
      Talk();
      ReachSlot(-1);
      const auto ownedId = sBrowseCamera;
      s16 foreignId = CAM_ID_NONE;
      if (returnId == 2) {
        assert(Play_CreateSubCamera(&play) == returnId);
        CutsceneManager_SetReturnCamera(returnId);
      } else if (!reuse) {
        foreignId = Play_CreateSubCamera(&play);
        assert(foreignId == 2);
      }
      const auto returnUid = Play_GetCamera(&play, returnId)->uid;
      EndBrowsing(&play);
      assert(sCutsceneMgr.endCsId != CS_ID_NONE);
      Play_ClearCamera(&play, returnId);
      Camera replacement{};
      Camera expectedMain = mainCamera;
      expectedMain.status = CAM_STATUS_ACTIVE;
      if (reuse) {
        if (returnId == CAM_ID_MAIN) {
          Camera_Init(&mainCamera, &play.view, &play.colCtx, &play);
          mainCamera.camId = CAM_ID_MAIN;
          play.cameraPtrs[CAM_ID_MAIN] = &mainCamera;
        } else {
          assert(Play_CreateSubCamera(&play) == returnId);
        }
        auto *camera = Play_GetCamera(&play, returnId);
        assert(camera->uid != returnUid);
        camera->eye = camera->eyeNext = {501, 502, 503};
        camera->at = {401, 402, 403};
        camera->fov = 41;
        replacement = *camera;
        if (returnId == CAM_ID_MAIN)
          replacement.status = CAM_STATUS_ACTIVE;
      } else if (foreignId != CAM_ID_NONE) {
        auto *foreign = Play_GetCamera(&play, foreignId);
        foreign->eye = foreign->eyeNext = {301, 302, 303};
        foreign->at = {201, 202, 203};
        foreign->fov = 43;
        Play_ChangeCameraStatus(&play, foreignId, CAM_STATUS_ACTIVE);
        replacement = *foreign;
      }
      CutsceneManager_Update();
      CutsceneManager_ClearWaiting();
      assert(play.cameraPtrs[ownedId] == nullptr &&
             CutsceneManager_GetCurrentCsId() == CS_ID_NONE);
      assert(playerWaits == playerEnds &&
             !(keeper.actor.flags & ACTOR_FLAG_FREEZE_EXCEPTION));
      assert(!sCutsceneMgr.isCameraStored &&
             "skipped native restoration must retire its stored snapshot");
      if (reuse) {
        assert(play.cameraPtrs[returnId] != nullptr);
        assert(!std::memcmp(&replacement, play.cameraPtrs[returnId],
                            sizeof(Camera)) &&
               "deferred End must not copy or restore over a reused "
               "return-camera UID");
      } else {
        assert(play.cameraPtrs[returnId] == nullptr);
      }
      if (foreignId != CAM_ID_NONE) {
        assert(Play_GetActiveCamId(&play) == foreignId);
        assert(!std::memcmp(&replacement, play.cameraPtrs[foreignId],
                            sizeof(Camera)));
      } else {
        assert(Play_GetActiveCamId(&play) == CAM_ID_MAIN);
        if (returnId != CAM_ID_MAIN)
          assert(!std::memcmp(&expectedMain, &mainCamera, sizeof(Camera)));
      }
      // Idle Update must borrow the surviving active camera, never a stale
      // return-slot ID which was just rejected by the deferred End.
      const auto activeId = Play_GetActiveCamId(&play);
      const auto activeSnapshot = *play.cameraPtrs[activeId];
      CutsceneManager_Update();
      CutsceneManager_ClearWaiting();
      assert(!std::memcmp(&activeSnapshot, play.cameraPtrs[activeId],
                          sizeof(Camera)));
      assert(sCutsceneMgr.retCamId == activeId);
      if (reuse && returnId != activeId)
        assert(!std::memcmp(&replacement, play.cameraPtrs[returnId],
                            sizeof(Camera)));
    }
  std::puts("MM native: default/explicit return slots cleared or reused after "
            "Stop preserve new owners through End and idle");
}

static void CheckValidStoredReturn() {
  Reset();
  Talk();
  ReachSlot(-1);
  const auto ownedId = sBrowseCamera;
  const auto returnId = Play_CreateSubCamera(&play);
  assert(returnId == 2 && sCutsceneMgr.isCameraStored);
  const auto oldReturnUid = play.cameraPtrs[returnId]->uid;
  const auto snapshotUid = mainCamera.uid;
  assert(oldReturnUid != snapshotUid);
  CutsceneManager_SetReturnCamera(returnId);
  CutsceneManager_GetCutsceneEntry(shop.csId)->endCam = CS_END_CAM_1;
  EndBrowsing(&play);
  CutsceneManager_Update();
  CutsceneManager_ClearWaiting();
  auto *restored = play.cameraPtrs[returnId];
  assert(Play_GetActiveCamId(&play) == returnId &&
         restored->status == CAM_STATUS_ACTIVE &&
         "a validated native EndCam1 restore must activate its explicit return "
         "camera after replacing its UID");
  assert(restored->uid == snapshotUid && restored->camId == returnId);
  assert(restored->eye.x == 50 && restored->eye.y == 100 &&
         restored->eye.z == 260 && restored->fov == 60);
  assert(play.cameraPtrs[ownedId] == nullptr && !sCutsceneMgr.isCameraStored);
  assert(CutsceneManager_GetCurrentCsId() == CS_ID_NONE &&
         playerWaits == playerEnds);
  assert(!(keeper.actor.flags & ACTOR_FLAG_FREEZE_EXCEPTION));
  const auto snapshot = *restored;
  CutsceneManager_Update();
  CutsceneManager_ClearWaiting();
  assert(Play_GetActiveCamId(&play) == returnId &&
         !std::memcmp(&snapshot, restored, sizeof(Camera)));
  std::puts("MM native: valid explicit EndCam1 restoration retains native UID "
            "replacement and return activation");
}

static void CheckLegacyNativeCameraBehavior() {
  for (u8 endCam :
       {u8(CS_END_CAM_0), u8(CS_END_CAM_1), u8(CS_END_CAM_SMOOTH)}) {
    Reset();
    // A new native scene must default to the legacy behavior after leaving
    // the private shop, even though Reset just initialized the shop scene.
    CutsceneManager_Init(&play, cutscenes.data(), cutscenes.size());
    CutsceneManager_StoreCamera(&mainCamera);
    cutscenes[0].endCam = endCam;
    assert(CutsceneManager_StartWithPlayerCsAndSetFlag(0, &keeper.actor) == 0);
    const auto ownedId = CutsceneManager_GetCurrentSubCamId(0);
    auto *owned = play.cameraPtrs[ownedId];
    owned->eye = owned->eyeNext = {11, 22, 33};
    owned->at = {1, 2, 3};
    owned->fov = 49;
    owned->roll = 17;
    const auto returnId = Play_CreateSubCamera(&play);
    assert(returnId == 2);
    const auto oldReturnUid = play.cameraPtrs[returnId]->uid;
    CutsceneManager_SetReturnCamera(returnId);
    // Baseline End always activates its return camera, even if main has
    // independently become active. Private-scene protection must be off.
    Play_ChangeCameraStatus(&play, CAM_ID_MAIN, CAM_STATUS_ACTIVE);
    CutsceneManager_Stop(0);
    CutsceneManager_Update();
    CutsceneManager_ClearWaiting();
    auto *returned = play.cameraPtrs[returnId];
    assert(
        Play_GetActiveCamId(&play) == returnId &&
        returned->status == CAM_STATUS_ACTIVE &&
        "normal native Init must disable private-scene ownership protection");
    assert(play.cameraPtrs[ownedId] == nullptr &&
           CutsceneManager_GetCurrentCsId() == CS_ID_NONE);
    assert(playerWaits == playerEnds &&
           !(keeper.actor.flags & ACTOR_FLAG_FREEZE_EXCEPTION));
    if (endCam == CS_END_CAM_1) {
      assert(returned->uid == mainCamera.uid && returned->uid != oldReturnUid);
      assert(returned->eye.x == 50 && returned->eye.y == 100 &&
             returned->eye.z == 260 && returned->fov == 60);
      assert(!sCutsceneMgr.isCameraStored);
    } else {
      assert(returned->uid == oldReturnUid);
      assert(returned->eye.x == 11 && returned->eye.y == 22 &&
             returned->eye.z == 33 && returned->fov == 49);
      assert(returned->roll == 17);
    }
    assert(bool(CutsceneManager_IsNext(CS_ID_GLOBAL_RETURN_TO_CAM)) ==
           (endCam == CS_END_CAM_SMOOTH));
  }
  Reset();
  CutsceneManager_Init(&play, cutscenes.data(), cutscenes.size());
  for (s16 id = CAM_ID_SUB_FIRST; id < NUM_CAMS; ++id)
    assert(Play_CreateSubCamera(&play) == id);
  auto *reserved = play.cameraPtrs[NUM_CAMS - 1];
  reserved->eye = {901, 902, 903};
  CutsceneManager_StoreCamera(&mainCamera);
  assert(sCutsceneMgr.isCameraStored && reserved->uid == mainCamera.uid);
  assert(reserved->eye.x == 50 && reserved->eye.y == 100 &&
         reserved->eye.z == 260);
  assert(reserved->status == CAM_STATUS_INACTIVE);
  std::puts("MM native: scene Init defaults to legacy EndCam0/1/smooth return "
            "and snapshot-storage behavior");
}

static void CheckForeignMotionAfterStop() {
  for (s16 requestedId : {s16(CAM_ID_MAIN), s16(2), s16(3)}) {
    Reset();
    Talk();
    ReachSlot(-1);
    const auto ownedId = sBrowseCamera;
    s16 foreignId = CAM_ID_MAIN;
    while (foreignId < requestedId)
      foreignId = Play_CreateSubCamera(&play);
    assert(foreignId == requestedId);
    auto *foreign = Play_GetCamera(&play, foreignId);
    const auto uid = foreign->uid;
    foreign->eye = foreign->eyeNext = {101, 102, 103};
    foreign->at = {51, 52, 53};
    foreign->fov = 45;
    Play_ChangeCameraStatus(&play, foreignId, CAM_STATUS_ACTIVE);
    MonitorBrowsing();
    assert(!sTalking && sCutsceneMgr.endCsId != CS_ID_NONE &&
           foreign->uid == uid);
    // A subsequent native owner update occurs before the deferred End.
    foreign->eye = foreign->eyeNext = {1001, 1002, 1003};
    foreign->at = {951, 952, 953};
    foreign->fov = 47;
    const auto latest = *foreign;
    CutsceneManager_Update();
    CutsceneManager_ClearWaiting();
    assert(play.cameraPtrs[ownedId] == nullptr &&
           Play_GetActiveCamId(&play) == foreignId);
    assert(!std::memcmp(&latest, foreign, sizeof(Camera)) &&
           "deferred End must preserve the foreign owner's latest pose, "
           "including the original return camera");
    assert(CutsceneManager_GetCurrentCsId() == CS_ID_NONE &&
           playerWaits == playerEnds);
    assert(!(keeper.actor.flags & ACTOR_FLAG_FREEZE_EXCEPTION));
    CutsceneManager_Update();
    CutsceneManager_ClearWaiting();
    assert(Play_GetActiveCamId(&play) == foreignId &&
           sCutsceneMgr.retCamId == foreignId);
    assert(!std::memcmp(&latest, foreign, sizeof(Camera)) &&
           "idle snapshot storage must not overwrite or deactivate a surviving "
           "active camera, including ID3");
  }
  std::puts("MM native: active return/foreign cameras retain post-Stop pose "
            "and status through End and idle");
}

static void CheckNoFreeSubcamera() {
  for (bool snapshotSlotTaken : {false, true}) {
    Reset();
    const int limit = NUM_CAMS - (snapshotSlotTaken ? 0 : 1);
    for (int id = CAM_ID_SUB_FIRST; id < limit; ++id)
      assert(Play_CreateSubCamera(&play) == id);
    std::array<Camera, NUM_CAMS> snapshots;
    for (int id = 0; id < limit; ++id)
      snapshots[id] = *play.cameraPtrs[id];
    // Isolate the adapter from the manager's reserved ID3 snapshot write.
    talkAccepted = true;
    KeeperUpdate(&keeper.actor, &play);
    assert(!sTalking && "shop must reject native start when ordinary "
                        "subcameras are already owned");
    assert(CutsceneManager_GetCurrentCsId() == CS_ID_NONE &&
           Play_GetActiveCamId(&play) == CAM_ID_MAIN);
    for (int id = 0; id < limit; ++id)
      assert(!std::memcmp(&snapshots[id], play.cameraPtrs[id], sizeof(Camera)));
    if (!snapshotSlotTaken)
      assert(play.cameraPtrs[NUM_CAMS - 1] == nullptr);
  }
  std::puts("MM native: exhausted ordinary camera pool preserves owners and "
            "reserved snapshot slot");
}

static void CheckQueuedPoolExhaustion() {
  Reset();
  Talk();
  const auto original = mainCamera;
  for (int id = CAM_ID_SUB_FIRST; id < NUM_CAMS - 1; ++id)
    assert(Play_CreateSubCamera(&play) == id);
  for (int i = 0; i < 4; ++i)
    NativeFrame();
  assert(shop.actionFunc == EnOssan_BeginInteraction &&
         shop.cutsceneState == 1);
  assert(CutsceneManager_GetCurrentCsId() == CS_ID_NONE &&
         mainCamera.setting == original.setting);
  assert(!play.cameraPtrs[NUM_CAMS - 1] ||
         play.cameraPtrs[NUM_CAMS - 1]->status == CAM_STATUS_INACTIVE);
  Play_ClearCamera(&play, CAM_ID_SUB_FIRST);
  ReachSlot(-1);
  Press(BTN_B);
  assert(!sTalking && CutsceneManager_GetCurrentCsId() == CS_ID_NONE);
  std::puts("MM production native shop: queued pool exhaustion retries native "
            "Start without consuming reserved snapshot slot");
}
static void CheckKeeperDialogue() {
  Reset(); Talk(); ReachSlot(-1);
  assert(play.msgCtx.currentTextId == kKeeperMenuText);
  const auto camera = sBrowseCameraPtr;
  const auto cs = CutsceneManager_GetCurrentCsId();
  auto begin = [] {
    play.msgCtx.choiceIndex = 0; Press(BTN_A, true);
    assert(shop.actionFunc == EnOssan_TalkToShopkeeper &&
           play.msgCtx.currentTextId == kKeeperTalkText);
  };
  begin(); Press(BTN_A, true);
  assert(shop.actionFunc == EnOssan_TalkToShopkeeper &&
         play.msgCtx.currentTextId == 0x8F17 && messageState == TEXT_STATE_CHOICE);
  play.msgCtx.choiceIndex = 0; Press(BTN_A, true);
  assert(play.msgCtx.currentTextId == 0x8F18);
  messageState = TEXT_STATE_AWAITING_NEXT; Press(BTN_A, true);
  assert(shop.actionFunc == EnOssan_TalkToShopkeeper && play.msgCtx.currentTextId == 0x8F18);
  messageState = TEXT_STATE_EVENT; Press(BTN_A, true);
  assert(shop.actionFunc == EnOssan_FaceShopkeeper && play.msgCtx.currentTextId == kKeeperMenuText);
  begin(); Press(BTN_A, true); play.msgCtx.choiceIndex = 1; Press(BTN_A, true);
  assert(shop.actionFunc == EnOssan_FaceShopkeeper && play.msgCtx.currentTextId == kKeeperMenuText);
  for (int page = 0; page < 3; ++page) {
    begin();
    if (page > 0) Press(BTN_A, true);
    if (page > 1) { play.msgCtx.choiceIndex = 0; Press(BTN_A, true); }
    Press(BTN_B, true);
    assert(shop.actionFunc == EnOssan_FaceShopkeeper && play.msgCtx.currentTextId == kKeeperMenuText);
  }
  assert(sTalking && sBrowseCameraPtr == camera && CutsceneManager_GetCurrentCsId() == cs &&
         purchases == 0 && !sReceipt.pending && callbacks.empty());
  Press(BTN_B);
  assert(!sTalking && shop.actionFunc == EnOssan_Idle);
  for (int page = 0; page < 3; ++page) {
    for (int interruption = 0; interruption < 5; ++interruption) {
      Reset(); Talk(); ReachSlot(-1); begin();
      if (page > 0) Press(BTN_A, true);
      if (page > 1) { play.msgCtx.choiceIndex = 0; Press(BTN_A, true); }
      if (interruption == 0) messageState = TEXT_STATE_NONE;
      if (interruption == 1) messageState = TEXT_STATE_CLOSING;
      if (interruption == 2) play.msgCtx.currentTextId = 0x1234;
      if (interruption == 3) play.transitionTrigger = TRANS_TRIGGER_START;
      if (interruption == 4) play.gameOverCtx.state = static_cast<decltype(play.gameOverCtx.state)>(1);
      NativeFrame();
      assert(!sTalking && shop.actionFunc == EnOssan_Idle && purchases == 0 && !sReceipt.pending);
      if (interruption == 2) assert(play.msgCtx.currentTextId == 0x1234 && messageState != TEXT_STATE_CLOSING);
    }
  }
  std::puts("MM optional native keeper intro/help/back/B retains camera; zero purchases/receipts");
}
static void CheckPlayerDrawBoundary() {
  static int submitted = 0;
  const auto visible = [&] {
    player.actor.draw = [](Actor*, PlayState*) { ++submitted; };
    const int before = submitted;
    DrawPlayerBoundary(&player.actor);
    return submitted != before;
  };
  // The native actor dispatch is before every native/replacement player body.
  // Replacing the callback represents the native, adult/custom and Wolf/Mario
  // submissions without mocking the actual draw gate or hook registry.
  for (int side : {-1, 1}) {
    Reset();
    assert(visible() && "ordinary gameplay player remains drawable");
    Talk();
    assert(!visible() && "accepted shop talk must suppress the actual player draw dispatch");
    ReachSlot(-1);
    assert(!visible());
    Stick(side * 80, 0);
    for (int frame = 0; frame < 10; ++frame) {
      NativeFrame();
      assert(!visible() && "turning and extreme shelf browse must hide replacement bodies");
    }
    NativeFrame(); SelectItem();
    assert(!visible() && "native selection keeps replacement player models hidden");
    Reset(); Talk(); ReachSlot(-1);
    play.msgCtx.choiceIndex = 0;
    Press(BTN_A, true);
    assert(shop.actionFunc == EnOssan_TalkToShopkeeper && !visible());
    Press(BTN_B);
    assert(!visible());
    Press(BTN_B);
    assert(visible() && "B exit restores the player without changing its draw pointer");
    Reset(); Talk(); ReachSlot(-1);
    player.stateFlags2 &= ~PLAYER_STATE2_20000000;
    assert(visible() && "native receipt hide-bit release restores player presentation");
    player.stateFlags2 |= PLAYER_STATE2_20000000;
    sReceipt.pending = true;
    assert(visible() && "receipt ownership must never suppress the get-item presentation");
    sReceipt.pending = false;
    assert(!visible());
    active = false;
    assert(visible() && "another scene/session cannot inherit the shop draw filter");
    active = true;
    Actor other = player.actor;
    const int before = submitted;
    DrawPlayerBoundary(&other);
    assert(submitted == before + 1 && "only the actual player is hidden");
    EndBrowsing(&play);
    assert(visible());
    Reset(); Talk(); ReachSlot(-1);
    NativeKeeperDestroyed(&shop.actor, &play);
    assert(visible() && "destroyed keeper releases player presentation");
  }
  std::puts("MM shop player draw: real hook registry/Actor_Draw boundary suppresses browse/turn/keeper models and releases receipt/B/interrupt/destroy; foreign actor unaffected");
}
int main(int argc, char **argv) {
  RegisterPlayerDrawHook();
  RegisterInspectionLetterboxHook();
  if (argc == 2 && std::string(argv[1]) == "inspection") {
    CheckInspectionLetterboxBoundary();
    CheckSelectedItemLegibility();
    CheckImmediateCannotBuyFraming();
    return 0;
  }
  if (argc == 2 && std::string(argv[1]) == "player-draw") {
    CheckPlayerDrawBoundary();
    return 0;
  }

  if (argc == 2 && std::string(argv[1]) == "customer") {
    CheckCustomerAccessAndReturn();
    return 0;
  }
  if (argc == 2 && std::string(argv[1]) == "oot-pivot") {
    CheckOotPivotTiming();
    return 0;
  }
  if (argc == 2 && std::string(argv[1]) == "selection-paths") {
    DumpSelectionPaths();
    return 0;
  }
  CheckPlayerDrawBoundary();
  std::setbuf(stdout, nullptr);
  if (argc == 2 && std::string(argv[1]) == "receipt") {
    CheckProductionReceipt();
    return 0;
  }
  if (argc == 2 && std::string(argv[1]) == "framing") {
    CheckAuthoredFraming();
    return 0;
  }
  if (argc == 2 && std::string(argv[1]) == "same-cs") {
    CheckSameCutsceneDifferentActor();
    return 0;
  }
  if (argc == 2 && std::string(argv[1]) == "destroy") {
    CheckLiveKeeperDestruction();
    CheckDestructionWithForeignOwnerAndReceipt();
    return 0;
  }
  if (argc == 2 && std::string(argv[1]) == "foreign-camera") {
    CheckForeignCameraTakeover();
    return 0;
  }
  if (argc == 2 && std::string(argv[1]) == "camera-pool") {
    CheckNoFreeSubcamera();
    return 0;
  }
  if (argc == 2 && std::string(argv[1]) == "queued-pool") {
    CheckQueuedPoolExhaustion();
    return 0;
  }
  if (argc == 2 && std::string(argv[1]) == "reused-camera") {
    CheckOwnedSlotReuse();
    CheckOwnedSlotReuse(true);
    return 0;
  }
  if (argc == 2 && std::string(argv[1]) == "cleared-camera") {
    CheckOwnedSlotCleared();
    CheckOwnedSlotCleared(true);
    return 0;
  }
  if (argc == 2 && std::string(argv[1]) == "return-camera") {
    CheckReturnSlotOwnership();
    return 0;
  }
  if (argc == 2 && std::string(argv[1]) == "foreign-motion") {
    CheckForeignMotionAfterStop();
    return 0;
  }
  if (argc == 2 && std::string(argv[1]) == "valid-return") {
    CheckValidStoredReturn();
    return 0;
  }
  if (argc == 2 && std::string(argv[1]) == "legacy-camera") {
    CheckLegacyNativeCameraBehavior();
    return 0;
  }
  CheckCustomerAccessAndReturn();
  CheckOotPivotTiming();
  CheckKeeperDialogue();
  CheckLifecycle();
  CheckAbortBoundaries();
  CheckForeignCutscene();
  CheckSameCutsceneDifferentActor();
  CheckLiveKeeperDestruction();
  CheckProductionReceipt();
  CheckDestructionWithForeignOwnerAndReceipt();
  CheckAuthoredFraming();
  CheckInspectionLetterboxBoundary();
  CheckSelectedItemLegibility();
  CheckImmediateCannotBuyFraming();
  CheckForeignCameraTakeover();
  CheckOwnedSlotReuse();
  CheckOwnedSlotReuse(true);
  CheckOwnedSlotCleared();
  CheckOwnedSlotCleared(true);
  CheckReturnSlotOwnership();
  CheckValidStoredReturn();
  CheckLegacyNativeCameraBehavior();
  CheckForeignMotionAfterStop();
  CheckNoFreeSubcamera();
  CheckQueuedPoolExhaustion();
  std::puts(
      "MM production native EnOssan/EnGirlA + actual CutsceneManager/ported OoT Data4 "
      "integration passed; configured runtime unproven");
}
