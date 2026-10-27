#include "combo/ComboGameMode.h"
#include "combo/entrances/LostWoodsEntrance.h"
#include "combo/entrances/SoloForest.h"
#include "combo/gameplay/MagicMirrorAnchor.h"
#include "combo/gameplay/MagicMirrorArrival.h"
#include "combo/gameplay/MagicMirrorItem.h"
#include "combo/gameplay/MagicMirrorPlayer.h"
#include "global.h"
#ifdef MIRROR_OOT
#include "soh/soh/FleetShipCombo/FleetComboIds.h"
#else
#include "mm/2s2h/FleetShipCombo/FleetComboIds.h"
#endif
#ifndef MIRROR_OOT
#include "2s2h/Enhancements/Gameplay/ComboForestActors.h"
#endif
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

extern "C" {
int gComboGameMode;
}
SaveContext gSaveContext{};
PlayState *gPlayState;
GameState *gGameState;
static PlayState play;
static Player player;
#ifndef MIRROR_OOT
static ActorCutscene *testSceneCutscenes;
extern "C" {
s16 sSceneCutsceneCount;
ActorCutscene *CutsceneManager_GetCutsceneEntry(s16 id) {
  assert(testSceneCutscenes && id >= 0 && id < sSceneCutsceneCount);
  return &testSceneCutscenes[id];
}
}
#endif
static bool owned, blocked, pack = true;
static bool shuffled, stumpCollected;
static bool stumpReceiptPending;
static int stumpReceiptCancels;
static bool pending, stow, handoffAccepted;
static bool alternateModel;
static int animationStarts, idleRestores;
static std::vector<u16> mirrorSounds;
static int castCameras, releasedCameras;
static bool ownsCastCamera;
#ifdef MIRROR_OOT
extern "C" void Sfx_PlaySfxCentered(u16 sound) {
#else
extern "C" void Audio_PlaySfx(u16 sound) {
#endif
  assert(sound == NA_SE_EV_TRE_BOX_FLASH || sound == NA_SE_PL_MAGIC_WIND_WARP);
  mirrorSounds.push_back(sound);
}
static void ExpectSounds(std::initializer_list<u16> expected) {
  assert(mirrorSounds == std::vector<u16>(expected));
}
static int mirrorDraws, worldShimmerDraws, matrixDepth;
static Vec3f mirrorOrigin;
static int auraDraws;
static float auraScale, auraAlpha;
static Vec3f auraOrigin;
static int handoffs, prompts, pickups, errors, saves;
static u32 seed = 77;
static u16 buttonItem;
static u8 textState;
static bool advance;
static int closed;
static bool nativeMessages;
static void TestMessageSfx() {}
#include "native_message.inc.h"
static bool noFloor, obstructed, capUnavailable;
static float floorY;
static CollisionPoly landingFloor;
extern "C" f32 BgCheck_EntityRaycastFloor3(CollisionContext *context,
                                           CollisionPoly **poly, s32 *bg,
                                           Vec3f *point) {
  // Resolve the fixture's real cap triangles instead of treating the entire
  // world as one flat floor. This exercises footprint support at a moved cap.
  const auto *header = context->colHeader;
  if (!noFloor && !capUnavailable && header) {
    for (int i = 0; i < header->numPolygons; ++i) {
      auto &candidate = header->polyList[i];
      const auto a = header->vtxList[candidate.vtxData[0] & 0x1FFF];
      const auto b = header->vtxList[candidate.vtxData[1] & 0x1FFF];
      const auto c = header->vtxList[candidate.vtxData[2] & 0x1FFF];
      const float d =
          float(b.x - a.x) * (c.z - a.z) - float(b.z - a.z) * (c.x - a.x);
      if (std::fabs(d) < .001f || a.y > point->y)
        continue;
      const float u =
          ((point->x - a.x) * (c.z - a.z) - (point->z - a.z) * (c.x - a.x)) / d;
      const float v =
          ((b.x - a.x) * (point->z - a.z) - (b.z - a.z) * (point->x - a.x)) / d;
      if (u >= -.0001f && v >= -.0001f && u + v <= 1.0001f) {
        *poly = &candidate;
        *bg = BGCHECK_SCENE;
        return a.y;
      }
    }
  }
  *poly = noFloor ? nullptr : &landingFloor;
  *bg = BGCHECK_SCENE;
  return noFloor ? BGCHECK_Y_MIN : floorY;
}
extern "C" s32 BgCheck_EntityLineTest1(CollisionContext *, Vec3f *, Vec3f *,
                                       Vec3f *, CollisionPoly **, s32, s32, s32,
                                       s32, s32 *) {
  return obstructed;
}
void SetText(u16 text) {
#ifdef MIRROR_OOT
  play.msgCtx.textId = text;
#else
  play.msgCtx.currentTextId = text;
#endif
}
namespace MirrorHost {
bool Shuffled() { return shuffled; }
bool PickupAvailable() { return shuffled ? !stumpCollected : !owned; }
void CancelPickup() {
  stumpReceiptPending = false;
  ++stumpReceiptCancels;
}
void UpdateCastCamera(PlayState *, Player *) {
  if (!ownsCastCamera) {
    ownsCastCamera = true;
    ++castCameras;
  }
}
void ReleaseCastCamera(PlayState *) {
  if (ownsCastCamera)
    ++releasedCameras;
  ownsCastCamera = false;
}
void ForgetCastCamera() { ownsCastCamera = false; }
#include "native_mirror_sound.inc.h"
u32 Seed() { return seed; }
bool PackAvailable() { return pack; }
bool Busy(PlayState *) { return blocked; }
bool TransitionPending() { return pending; }
bool AlternateModel() { return alternateModel; }
bool TimedActivity() { return false; }
bool QueuePickup(int) {
  ++pickups;
  stumpReceiptPending = shuffled;
  return true;
}
void DrawWorldShimmer(PlayState *) { ++worldShimmerDraws; }
void DrawCastAura(PlayState *, Player *current, float scale, float alpha) {
  ++auraDraws;
  auraOrigin = current->actor.world.pos;
  auraScale = scale;
  auraAlpha = alpha;
}
void OpenPrompt() {
  ++prompts;
  SetText(0x3FEC);
  play.msgCtx.msgMode = MSGMODE_TEXT_DISPLAYING;
  textState = TEXT_STATE_CHOICE;
  if (nativeMessages) {
    play.msgCtx.msgLength = 64;
    play.msgCtx.msgMode = MSGMODE_TEXT_DONE;
#ifdef MIRROR_OOT
    play.msgCtx.textboxEndType = TEXTBOX_ENDTYPE_2_CHOICE;
#else
    play.msgCtx.textboxEndType = TEXTBOX_ENDTYPE_TWO_CHOICE;
    play.msgCtx.nextTextId = 0xFFFF;
#endif
  }
}
bool RequestHandoff() {
  ++handoffs;
  return handoffAccepted;
}
void Save() { ++saves; }
void Error() { ++errors; }
void Trace(const char *, PlayState *) {}
bool ButtonClaimed(u16) { return false; }
u16 ButtonItem(s32) { return buttonItem; }
} // namespace MirrorHost
namespace ComboMagicMirrorVisual {
void Draw(PlayState *) { ++mirrorDraws; }
} // namespace ComboMagicMirrorVisual
namespace MirrorHost {
void DrawWorldPickup(PlayState *state) {
  ComboMagicMirrorVisual::Draw(state);
  DrawWorldShimmer(state);
}
} // namespace MirrorHost
extern "C" void Matrix_Push() { ++matrixDepth; }
extern "C" void Matrix_Pop() { --matrixDepth; }
#ifdef MIRROR_OOT
using TestMatrixMode = u8;
extern "C" void Matrix_RotateY(f32, u8) {}
#else
using TestMatrixMode = MatrixMode;
extern "C" void Matrix_RotateYF(f32, MatrixMode) {}
#endif
extern "C" void Matrix_Translate(f32 x, f32 y, f32 z, TestMatrixMode) {
  mirrorOrigin = {x, y, z};
}
extern "C" void Matrix_Scale(f32, f32, f32, TestMatrixMode) {}
extern "C" void Matrix_ReplaceRotation(MtxF *) {}
extern "C" uint8_t MagicMirror_IsOwned(void) { return owned; }
extern "C" void TravelTools_Grant(uint16_t) { owned = true; }
extern "C" auto Message_GetState(MessageContext *context)
    -> decltype(Message_GetState((MessageContext *)nullptr)) {
  if (nativeMessages)
    return NativeMessage_GetState(context);
  return textState;
}
extern "C" auto Message_ShouldAdvance(PlayState *current)
    -> decltype(Message_ShouldAdvance((PlayState *)nullptr)) {
  if (nativeMessages)
    return NativeMessage_ShouldAdvance(current);
  return advance;
}
extern "C" void Message_CloseTextbox(PlayState *current) {
  ++closed;
  if (nativeMessages) {
    NativeMessage_CloseTextbox(current);
    return;
  }
  current->msgCtx.msgMode = MSGMODE_TEXT_CLOSING;
  textState = TEXT_STATE_NONE;
}
#ifdef MIRROR_OOT
extern "C" s32 Player_InCsMode(PlayState *) { return blocked; }
#else
extern "C" bool Player_InCsMode(PlayState *) { return blocked; }
#define COMBO_MAGIC_MIRROR_MM
#endif

// Asset loading and native idle/put-away plumbing are the boundary doubles;
// the cast action, ownership checks, cancellation and travel below are real.
#ifdef MIRROR_OOT
static LinkAnimationHeader gPlayerAnim_link_magic_kaze1;
#else
static PlayerAnimationHeader gPlayerAnim_link_magic_kaze1;
#endif
void NativeIdle(Player *, PlayState *) {}
void NativeSetAction(Player *current, PlayerActionFunc action) {
  current->actionFunc = action;
  current->av2.actionVar2 = 0;
#ifdef MIRROR_OOT
  current->stateFlags1 &= ~PLAYER_STATE1_IN_CUTSCENE;
#else
  current->stateFlags1 &= ~PLAYER_STATE1_20000000;
#endif
}
void Player_Action_WaitForPutAway(Player *current, PlayState *state) {
  current->afterPutAwayFunc(state, current);
}
s32 Player_SetupWaitForPutAway(PlayState *, Player *current,
                               AfterPutAwayFunc after) {
  NativeSetAction(current, Player_Action_WaitForPutAway);
  current->afterPutAwayFunc = after;
  return stow;
}
extern "C" s16 Animation_GetLastFrame(void *) { return 55; }
#ifdef MIRROR_OOT
#undef R_UPDATE_RATE
#define R_UPDATE_RATE 3
extern "C" void LinkAnimation_AnimateFrame(PlayState *, SkelAnime *) {}
#else
extern "C" void PlayerAnimation_AnimateFrame(PlayState *, SkelAnime *) {}
#endif
#include "native_animation.inc.h"
#ifdef MIRROR_OOT
s32 Player_SetupAction(PlayState *, Player *current, PlayerActionFunc action,
                       s32) {
  NativeSetAction(current, action);
  return 1;
}
LinkAnimationHeader *Player_GetIdleAnim(Player *) {
  return &gPlayerAnim_link_magic_kaze1;
}
void func_80839FFC(Player *current, PlayState *) {
  NativeSetAction(current, NativeIdle);
  ++idleRestores;
}
void Player_AnimChangeOnceMorph(PlayState *, Player *, LinkAnimationHeader *) {}
extern "C" void LinkAnimation_Change(PlayState *, SkelAnime *skel,
                                     LinkAnimationHeader *animation, f32 speed,
                                     f32 start, f32 end, u8 mode, f32 morph) {
#else
s32 Player_SetAction(PlayState *, Player *current, PlayerActionFunc action,
                     s32) {
  NativeSetAction(current, action);
  return 1;
}
void func_80839E74(Player *current, PlayState *) {
  NativeSetAction(current, NativeIdle);
  ++idleRestores;
}
extern "C" void PlayerAnimation_Change(PlayState *, SkelAnime *skel,
                                       PlayerAnimationHeader *animation,
                                       f32 speed, f32 start, f32 end, u8 mode,
                                       f32 morph) {
#endif
  assert(animation == &gPlayerAnim_link_magic_kaze1 && mode == ANIMMODE_ONCE &&
         morph < 0);
  skel->animation = animation;
  skel->curFrame = start;
  skel->endFrame = end;
  skel->playSpeed = speed;
  skel->animLength = 56;
#ifdef MIRROR_OOT
  skel->update.link = LinkAnimation_Once;
#else
  skel->update.player = PlayerAnimation_Once;
#endif
  ++animationStarts;
}
#include "combo/gameplay/MagicMirrorPlayer.inc.c"
#include "combo/gameplay/MagicMirrorRuntime.inc.h"

// Compile the actual native local-pad branch and each Wolf input policy. The
// controller source remains untouched so the message can read Yes/No and B.
static bool TestCustomItems_BlocksMovement(Player *) { return false; }
static bool Testfunc_8082DA90(PlayState *) { return false; }
static bool TestTransformMasks_IsTransformed() { return false; }
static bool TestMmMaskWear_IsKamaroDancing() { return false; }
static void TestWolfLinkHost_RestorePlayerInput(Player *, Input *) {}
static void TestGameInteractor_ExecuteOnPassPlayerInputs(Input *) {}
static void TestTransformMasks_FilterB(Input *) {}
static void TestGetStickData(f32 *magnitude, s16 *yaw, Input *input) {
  *magnitude = std::hypot(input->cur.stick_x, input->cur.stick_y);
  *yaw = 0;
}
static s16 TestCameraYaw(PlayState *) { return 0; }
#include "native_prompt_input.inc.h"

void Scene(int scene) {
#ifdef MIRROR_OOT
  play.sceneNum = scene;
#else
  play.sceneId = scene;
#endif
}
void Reset() {
  play = {};
  player = {};
  gSaveContext = {};
  gPlayState = &play;
  gGameState = &play.state;
  play.state.running = true;
#ifdef MIRROR_OOT
  play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
#else
  play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
  play.state.framerateDivisor = 3;
#endif
  player.actor.bgCheckFlags = BGCHECKFLAG_GROUND;
  player.actionFunc = NativeIdle;
  gSaveContext.gameMode = GAMEMODE_NORMAL;
  gSaveContext.fileNum = 1;
  owned = true;
  blocked = false;
  pack = true;
  shuffled = stumpCollected = false;
  pending = stow = false;
  handoffAccepted = true;
  alternateModel = false;
  animationStarts = idleRestores = 0;
  mirrorSounds.clear();
  mirrorDraws = worldShimmerDraws = matrixDepth = 0;
  auraDraws = 0;
  auraScale = auraAlpha = 0;
  handoffs = prompts = pickups = errors = saves = 0;
  closed = 0;
  advance = false;
  textState = TEXT_STATE_NONE;
  nativeMessages = false;
  buttonItem = EXT_ITEM_MAGIC_MIRROR;
  noFloor = obstructed = capUnavailable = false;
  floorY = 0;
  landingFloor = {};
  landingFloor.normal.y = 32767;
  MirrorRuntime::sArrivalPending = false;
  MirrorRuntime::sArrivalPlay = nullptr;
#ifdef MIRROR_OOT
  gComboGameMode = COMBO_GAME_OOT;
  Scene(SCENE_HYRULE_FIELD);
  gSaveContext.health = 0x30;
  gSaveContext.magic = 0x30;
#else
  gComboGameMode = COMBO_GAME_MM;
  sSceneCutsceneCount = 0;
  testSceneCutscenes = nullptr;
  Scene(SCENE_00KEIKOKU);
  gSaveContext.save.saveInfo.playerData.health = 0x30;
  gSaveContext.save.saveInfo.playerData.magic = 0x30;
#endif
  gSaveContext.respawn[RESPAWN_MODE_TOP].data = 1;
  gSaveContext.respawn[RESPAWN_MODE_TOP].pos = {123, 456, 789};
  gSaveContext.respawn[RESPAWN_MODE_RETURN].data = 2;
  gSaveContext.respawn[RESPAWN_MODE_RETURN].pos = {-4, 5, -6};
  MirrorRuntime::Reset();
  castCameras = releasedCameras = 0;
  ownsCastCamera = false;
  stumpReceiptPending = false;
  stumpReceiptCancels = 0;
}
void CompletePromptClose() {
  // The synthetic-message cases don't execute native Message_Update.
  assert(MirrorRuntime::sPromptAccepted);
  play.msgCtx.msgMode = MSGMODE_NONE;
  play.msgCtx.msgLength = 0;
  SetText(0);
  textState = TEXT_STATE_NONE;
  advance = false;
  MirrorRuntime::Update();
}
void ConfirmCast() {
  play.state.input[0].press.button = BTN_CLEFT;
  MirrorRuntime::Update();
  play.state.input[0].press.button = BTN_A;
  play.msgCtx.choiceIndex = 0;
  advance = true;
  MirrorRuntime::Update();
  CompletePromptClose();
  advance = false;
  play.state.input[0].press.button = 0;
}

void TestPromptMovement() {
  Reset();
  play.state.input[0].press.button = BTN_CLEFT;
  MirrorRuntime::Update();
  auto &raw = play.state.input[0];
  raw = {};
  raw.cur.stick_x = 60;
  raw.cur.stick_y = -40;
  raw.cur.button = BTN_R;
  raw.press.button = BTN_A;
  const auto menuInput = raw;
  player.stateFlags1 &=
      ~MirrorRuntime::kPromptFreeze; // Native cleanup may clear the flag.
  MirrorRuntime::Update();
  assert(MirrorRuntime::sPrompt &&
         (player.stateFlags1 & MirrorRuntime::kPromptFreeze));
  // Clear again after the early prompt update: local-pad ownership must not
  // depend on that native action flag surviving the rest of the frame.
  player.stateFlags1 &= ~MirrorRuntime::kPromptFreeze;
#ifdef MIRROR_OOT
  player.linearVelocity = player.actor.speedXZ = 4;
#else
  player.speedXZ = player.actor.speed = 4;
#endif
  player.actor.velocity.x = player.actor.velocity.z = 3;
  const auto local = ReadNativePromptInput(&player, &play);
  const Input empty{};
  assert(std::memcmp(&local, &empty, sizeof(local)) == 0);
  assert(player.actor.velocity.x == 0 && player.actor.velocity.z == 0);
  // Wolf may read after another action change, including its raw-pad OoT
  // route. The live prompt independently blocks every movement procedure.
  player.stateFlags1 &= ~MirrorRuntime::kPromptFreeze;
#ifdef MIRROR_OOT
  const auto wolf = TestWolfReadInput(&player, &play);
#else
  const auto wolf = TestWolfReadInput(&player, &play, &raw);
#endif
  assert(wolf.blocked && !wolf.aPress && !wolf.bPress);
  assert(std::memcmp(&raw, &menuInput, sizeof(raw)) == 0);
  raw.press.button = BTN_B;
  MirrorRuntime::Update();
  assert(!MirrorRuntime::sPrompt &&
         !(player.stateFlags1 & MirrorRuntime::kPromptFreeze));
  const auto closing = ReadNativePromptInput(&player, &play);
  assert(std::memcmp(&closing, &empty, sizeof(closing)) == 0);
#ifdef MIRROR_OOT
  const auto closingWolf = TestWolfReadInput(&player, &play);
#else
  const auto closingWolf = TestWolfReadInput(&player, &play, &raw);
#endif
  assert(closingWolf.blocked && !closingWolf.aPress && !closingWolf.bPress);
  assert(raw.cur.stick_x == menuInput.cur.stick_x && raw.press.button == BTN_B);
  MirrorRuntime::Update(); // The next input frame restores gameplay control.
  const auto released = ReadNativePromptInput(&player, &play);
  assert(released.cur.stick_x == menuInput.cur.stick_x &&
         released.press.button == BTN_B);
#ifdef MIRROR_OOT
  const auto releasedWolf = TestWolfReadInput(&player, &play);
#else
  const auto releasedWolf = TestWolfReadInput(&player, &play, &raw);
#endif
  assert(!releasedWolf.blocked && releasedWolf.bPress);
  puts("PASS prompt owns human/Wolf movement after native action changes; raw "
       "choice pad and cancel restore preserved");
}

void TestPromptOwnsInputDuringNativeStateChanges() {
  Reset();
  play.state.input[0].press.button = BTN_CLEFT;
  MirrorRuntime::Update();
  auto &raw = play.state.input[0];
  raw = {};
  raw.cur.stick_x = raw.rel.stick_x = 60;
  raw.cur.stick_y = raw.rel.stick_y = 40;
  raw.cur.button = raw.press.button = BTN_A | BTN_B;
  const auto choiceInput = raw;
  NativeSetAction(&player, NativeIdle);
  // Collision/interaction state is mutable inside Player_UpdateCommon,
  // before late form readers. Eligibility changes must cancel at Update,
  // not return the still-open prompt's input to a movement reader.
  player.actor.bgCheckFlags &= ~BGCHECKFLAG_GROUND;
  assert(MirrorRuntime::sPrompt);
  assert(MagicMirror_PromptOwnsPlayer(&play, &player));
  const auto local = ReadNativePromptInput(&player, &play);
  const Input empty{};
  assert(std::memcmp(&local, &empty, sizeof(local)) == 0);
#ifdef MIRROR_OOT
  const auto wolf = TestWolfReadInput(&player, &play);
#else
  const auto wolf = TestWolfReadInput(&player, &play, &raw);
#endif
  assert(wolf.blocked && !wolf.aPress && !wolf.bPress);
  assert(std::memcmp(&raw, &choiceInput, sizeof(raw)) == 0);
  MirrorRuntime::Update();
  assert(!MirrorRuntime::sPrompt && closed == 1);
  puts("PASS prompt keeps gameplay input through native eligibility changes "
       "until its owner cancels");
}

void TestMarioPromptInputGate() {
  Reset();
  assert(NativeMarioReadsPad(&play, &player));
  play.state.input[0].press.button = BTN_CLEFT;
  MirrorRuntime::Update();
  player.stateFlags1 &= ~MirrorRuntime::kPromptFreeze;
  // MM Mario intentionally ignores its generic cutscene bit, and reads the
  // untouched raw pad after native filtering. Ownership must park it before
  // its input read/libsm64 position writeback, including accepted closing.
  assert(MagicMirrorPlayer_State(&player) == MAGIC_MIRROR_CAST_PLAYING);
  assert(!NativeMarioReadsPad(&play, &player));
  play.state.input[0].press.button = BTN_A;
  play.msgCtx.choiceIndex = 0;
  advance = true;
  MirrorRuntime::Update();
  assert(MirrorRuntime::sPromptAccepted);
  assert(!NativeMarioReadsPad(&play, &player));
  CompletePromptClose();
  assert(!NativeMarioReadsPad(&play, &player));
  MirrorRuntime::Reset();
  assert(NativeMarioReadsPad(&play, &player));
  puts("PASS native Mario park gate covers prompt, accepted closing and cast, "
       "then releases");
}

void TestChoiceReleaseInput() {
  for (bool yes : {false, true}) {
    Reset();
    play.state.input[0].press.button = BTN_CLEFT;
    MirrorRuntime::Update();
    auto &raw = play.state.input[0];
    raw = {};
    raw.press.button = BTN_A;
    raw.cur.stick_x = 60;
    raw.cur.stick_y = 20;
    play.msgCtx.choiceIndex = yes ? 0 : 1;
    advance = true;
    const auto choiceInput = raw;
    MirrorRuntime::Update();
    assert(MirrorRuntime::sPrompt == yes);
    const auto local = ReadNativePromptInput(&player, &play);
    const Input empty{};
    assert(std::memcmp(&local, &empty, sizeof(local)) == 0);
#ifdef MIRROR_OOT
    const auto wolf = TestWolfReadInput(&player, &play);
#else
    const auto wolf = TestWolfReadInput(&player, &play, &raw);
#endif
    assert(wolf.blocked && !wolf.aPress && !wolf.bPress);
    assert(std::memcmp(&raw, &choiceInput, sizeof(raw)) == 0);
    advance = false;
    if (yes)
      CompletePromptClose();
    else
      MirrorRuntime::Update();
    if (yes) {
      assert(MagicMirrorPlayer_State(&player) == MAGIC_MIRROR_CAST_PLAYING);
    } else {
      const auto released = ReadNativePromptInput(&player, &play);
      assert(released.press.button == BTN_A && released.cur.stick_x == 60);
    }
  }
  puts("PASS No/Yes consume one gameplay input frame without changing raw "
       "choice input; No restores and Yes casts");
}

void TestPromptOwnershipLifetime() {
  for (int reason = 0; reason < 9; ++reason) {
    Reset();
    play.state.input[0].press.button = BTN_CLEFT;
    MirrorRuntime::Update();
    assert(MagicMirror_PromptOwnsPlayer(&play, &player));
    switch (reason) {
    case 0:
      SetText(1234);
      break;
    case 1:
      play.msgCtx.msgMode = MSGMODE_TEXT_CLOSING;
      break;
    case 2:
      play.msgCtx.msgMode = MSGMODE_NONE;
      break;
    case 3:
      ++seed;
      break;
    case 4:
      play.pauseCtx.state = 1;
      break;
    case 5:
      owned = false;
      break;
    case 6:
      gGameState = nullptr;
      break;
    case 7:
      gComboGameMode =
          gComboGameMode == COMBO_GAME_MM ? COMBO_GAME_OOT : COMBO_GAME_MM;
      break;
    case 8:
      gSaveContext.fileNum = 2;
      break;
    }
    if (reason == 4)
      MirrorRuntime::Update(); // Safety cancellation belongs to the prompt
                               // owner.
    assert(!MagicMirror_PromptOwnsPlayer(&play, &player));
  }
  Reset();
  // Preserve a flag already owned before the prompt, but clear one that the
  // prompt reacquires after the original native owner releases it.
  player.stateFlags1 |= MirrorRuntime::kPromptFreeze;
  play.state.input[0].press.button = BTN_CLEFT;
  MirrorRuntime::Update();
  MirrorRuntime::ReleasePrompt(true);
  assert(player.stateFlags1 & MirrorRuntime::kPromptFreeze);
  Reset();
  player.stateFlags1 |= MirrorRuntime::kPromptFreeze;
  play.state.input[0].press.button = BTN_CLEFT;
  MirrorRuntime::Update();
  NativeSetAction(&player, NativeIdle);
  MirrorRuntime::Update();
  MirrorRuntime::ReleasePrompt(true);
  assert(!(player.stateFlags1 & MirrorRuntime::kPromptFreeze));
  Reset();
  auto *retired = new Player(player);
#ifdef MIRROR_OOT
  play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &retired->actor;
#else
  play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &retired->actor;
#endif
  play.state.input[0].press.button = BTN_CLEFT;
  MirrorRuntime::Update();
  assert(MagicMirror_PromptOwnsPlayer(&play, retired));
  delete retired;
#ifdef MIRROR_OOT
  play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
#else
  play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
#endif
  assert(!MagicMirror_PromptOwnsPlayer(&play, &player));
  MirrorRuntime::Update(); // Reject the old identity without touching freed
                           // memory.
  assert(!MirrorRuntime::sPrompt);
  Reset();
  retired = new Player(player);
#ifdef MIRROR_OOT
  play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &retired->actor;
#else
  play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &retired->actor;
#endif
  play.state.input[0].press.button = BTN_CLEFT;
  MirrorRuntime::Update();
  delete retired;
  MirrorRuntime::Destroy();
  assert(!MirrorRuntime::sPrompt);
  puts("PASS prompt identity/flag cleanup across message, file, mode, safety, "
       "player replacement and post-arena teardown");
}

void LoadForest(float x, float y, float z) {
  static Vec3s vertices[13];
  static CollisionPoly polys[12];
  static CollisionHeader collision;
  vertices[0] = {static_cast<s16>(x), static_cast<s16>(y), static_cast<s16>(z)};
  for (int i = 0; i < 12; ++i) {
    const float angle = i * 3.14159265359f / 6;
    vertices[i + 1] = {static_cast<s16>(std::round(x + 55 * std::cos(angle))),
                       static_cast<s16>(y),
                       static_cast<s16>(std::round(z + 56 * std::sin(angle)))};
    polys[i] = {};
    polys[i].vtxData[0] = 0;
    polys[i].vtxData[1] = i + 1;
    polys[i].vtxData[2] = (i + 1) % 12 + 1;
    polys[i].normal.y = 32767;
  }
  collision = {};
  collision.numVertices = 13;
  collision.vtxList = vertices;
  collision.numPolygons = 12;
  collision.polyList = polys;
  play.colCtx.colHeader = &collision;
  play.roomCtx.curRoom.num = 0;
  play.transitionTrigger = TRANS_TRIGGER_END;
  gSaveContext.sceneLayer = 0;
  gSaveContext.respawnFlag = 0;
  floorY = y - 80;
#ifdef MIRROR_OOT
  Scene(COMBO_SOLO_FOREST_OOT_SCENE);
  gSaveContext.entranceIndex = COMBO_SOLO_FOREST_OOT_ENTRANCE;
  gSaveContext.cutsceneIndex = 0;
#else
  Scene(Combo_IsCombinedGame() ? SCENE_LOST_WOODS : COMBO_SOLO_FOREST_MM_SCENE);
  gSaveContext.save.entrance = Combo_IsCombinedGame()
                                   ? ENTRANCE(LOST_WOODS, 0)
                                   : COMBO_SOLO_FOREST_MM_ENTRANCE;
  gSaveContext.save.cutsceneIndex = 0;
#endif
}

void TestArrival() {
  // Queue in an outdoor departure, then resolve from the destination resource.
  for (bool combined : {false, true}) {
#ifdef MIRROR_OOT
    if (combined)
      continue; // Combined OoT queues on the destination MM side.
#endif
    Reset();
    if (combined)
      gComboGameMode = COMBO_GAME_BOTH;
    assert(MirrorRuntime::Travel(&play));
    ExpectSounds({NA_SE_PL_MAGIC_WIND_WARP});
    MirrorRuntime::Destroy(); // The request survives scene arena destruction.
    LoadForest(1500, 320, -1300);
    ActorEntry original{};
    original.pos = {11, 22, 33};
    original.rot.y = 123;
    const auto copy = original;
    const auto respawns = gSaveContext;
    play.linkActorEntry = &original;
    MagicMirror_PrepareArrival(&play);
    assert(play.linkActorEntry != &original);
    assert(std::memcmp(&original, &copy, sizeof(original)) == 0);
    assert(play.linkActorEntry->pos.x == 1500 &&
           play.linkActorEntry->pos.y == 320 &&
           play.linkActorEntry->pos.z == -1300);
#ifdef MIRROR_OOT
    const float arrivalAngle =
        play.linkActorEntry->rot.y * (6.28318530718f / 65536.0f);
#else
    const float arrivalAngle =
        ((play.linkActorEntry->rot.y >> 7) & 0x1FF) * (3.14159265359f / 180.0f);
    assert((play.linkActorEntry->rot.y & 127) == 127);
#endif
    const float targetX =
        COMBO_SOLO_FOREST_STUMP_X - play.linkActorEntry->pos.x;
    const float targetZ =
        COMBO_SOLO_FOREST_STUMP_Z - play.linkActorEntry->pos.z;
    const float targetLength = std::sqrt(targetX * targetX + targetZ * targetZ);
    assert(
        (std::sin(arrivalAngle) * targetX + std::cos(arrivalAngle) * targetZ) /
                targetLength >
            .9999f &&
        "Mirror arrival must face the visible market/portal, including at a "
        "moved cap");
    assert(std::memcmp(&gSaveContext.respawn, &respawns.respawn,
                       sizeof(respawns.respawn)) == 0);
    MagicMirror_FinishArrival(&play);
    assert(player.actionFunc == NativeIdle && idleRestores == 1);
    ExpectSounds({NA_SE_PL_MAGIC_WIND_WARP, NA_SE_PL_MAGIC_WIND_WARP});
    assert(play.linkActorEntry ==
           &original); // Preserve legacy scene-derived doorway anchors.
    MagicMirror_FinishArrival(&play);
    assert(idleRestores == 1);
    ExpectSounds({NA_SE_PL_MAGIC_WIND_WARP, NA_SE_PL_MAGIC_WIND_WARP});
    play.linkActorEntry = &original;
    MagicMirror_PrepareArrival(&play);
    assert(play.linkActorEntry == &original); // Consumed exactly once.
  }
  // The native cap's first view matches the final market/portal-facing shot.
  Reset();
  MagicMirror_QueueArrival();
  LoadForest(-377, 80, 416);
  ActorEntry nativeEntry{};
  play.linkActorEntry = &nativeEntry;
  MagicMirror_PrepareArrival(&play);
  assert(play.linkActorEntry != &nativeEntry);
#ifdef MIRROR_OOT
  assert(play.linkActorEntry->rot.y == -28478);
#else
  assert(((play.linkActorEntry->rot.y >> 7) & 0x1FF) == 204);
#endif
  MagicMirror_FinishArrival(&play);
  // Moved geometry replaces the previous scene's cached anchor at the same
  // address.
  Reset();
  LoadForest(-377, 80, 416);
  assert(MirrorRuntime::Locate(&play));
  MagicMirror_QueueArrival();
  LoadForest(-1500, 400, 1500);
  ActorEntry original{};
  play.linkActorEntry = &original;
  MagicMirror_PrepareArrival(&play);
  assert(
      play.linkActorEntry != &original && play.linkActorEntry->pos.x == -1500 &&
      play.linkActorEntry->pos.y == 400 && play.linkActorEntry->pos.z == 1500);
  // A blocked/missing cap retains the existing checked ground fallback.
  Reset();
  MagicMirror_QueueArrival();
  LoadForest(-377, 80, 416);
  capUnavailable = true;
  play.linkActorEntry = &original;
  MagicMirror_PrepareArrival(&play);
  assert(play.linkActorEntry != &original &&
         play.linkActorEntry->pos.x == -377 &&
         play.linkActorEntry->pos.y == 0 && play.linkActorEntry->pos.z == 296);
  // Slot/seed/mode/scene mismatches, cutscenes, absent floor and walls fail
  // closed.
  for (int reason = 0; reason < 8; ++reason) {
    Reset();
    MagicMirror_QueueArrival();
    LoadForest(-377, 80, 416);
    play.linkActorEntry = &original;
    switch (reason) {
    case 0:
      ++gSaveContext.fileNum;
      break;
    case 1:
      ++seed;
      break;
    case 2:
      gComboGameMode =
          COMBO_GAME_OOT == gComboGameMode ? COMBO_GAME_MM : COMBO_GAME_OOT;
      break;
    case 3:
      Scene(0xFFFF);
      break;
    case 4:
      gSaveContext.cutsceneTrigger = 1;
      break;
    case 5:
      noFloor = true;
      break;
    case 6:
      obstructed = true;
      break;
    case 7:
      play.colCtx.colHeader = nullptr;
      break;
    }
    MagicMirror_PrepareArrival(&play);
    assert(play.linkActorEntry == &original && !MirrorRuntime::sArrivalPending);
    MagicMirror_FinishArrival(&play);
    assert(idleRestores == 0);
  }
  puts("PASS native stump-relative arrival, scene moves, safe fallback, "
       "cached-resource/respawn preservation and one-shot context");
}

void TestStumpTopPickup() {
  for (bool combined : {false, true}) {
#ifdef MIRROR_OOT
    if (combined)
      continue; // Combined mode's forest and pickup belong to MM.
#endif
    for (int reason = 0; reason < 8; ++reason) {
      Reset();
      if (combined)
        gComboGameMode = COMBO_GAME_BOTH;
      LoadForest(-377, 80, 416);
      play.transitionTrigger = TRANS_TRIGGER_OFF;
      owned = false;
      player.actor.world.pos = {-377, 80, 416};
      player.actor.floorHeight = 80;
      player.actor.floorBgId = BGCHECK_SCENE;
      player.actor.floorPoly = play.colCtx.colHeader->polyList;
      switch (reason) {
      case 0:
        player.actor.world.pos.y = player.actor.floorHeight = 0;
        break;
      case 1:
        player.actor.world.pos.x += 45.1f;
        break;
      case 2:
        player.actor.world.pos.y += 5.1f;
        break;
      case 3:
        player.actor.floorBgId = 0;
        break; // A platform above the cap is not the stump.
      case 4:
        player.actor.floorPoly = &landingFloor;
        break;
      case 5:
        player.actor.floorHeight = 76;
        break; // Near the ramp's upper end.
      case 6:
        player.actor.bgCheckFlags = 0;
        break;
      case 7:
        player.actor.floorPoly = nullptr;
        break;
      }
      MirrorRuntime::Update();
      assert(pickups == 0 && !owned && saves == 0);
    }
    Reset();
    if (combined)
      gComboGameMode = COMBO_GAME_BOTH;
    LoadForest(-377, 80, 416);
    play.transitionTrigger = TRANS_TRIGGER_OFF;
    owned = false;
    player.actor.world.pos = {-377, 80, 416};
    player.actor.floorHeight = 80;
    player.actor.floorBgId = BGCHECK_SCENE;
    player.actor.floorPoly = play.colCtx.colHeader->polyList;
    MirrorRuntime::Update();
    assert(pickups == 1 && !owned && saves == 0);
    const int token = MirrorRuntime::sPickupToken;
    MirrorRuntime::Update();
    assert(pickups == 1);
    MirrorRuntime::FinishPickup(token);
    MirrorRuntime::FinishPickup(token);
    assert(owned && saves == 1);
    MirrorRuntime::Update();
    assert(pickups == 1);
  }
  puts("PASS Mirror pickup requires grounded static stump-top contact, rejects "
       "base/ramp/platform, and grants once");
}

void TestWorldRewardShimmer() {
  Reset();
  LoadForest(-377, 80, 416);
  owned = false;
  MirrorRuntime::DrawWorld();
  assert(mirrorDraws == 1 && worldShimmerDraws == 1 && matrixDepth == 0);
  assert(mirrorOrigin.x == -377 && mirrorOrigin.y == 122 &&
         mirrorOrigin.z == 416);
  owned = true;
  MirrorRuntime::DrawWorld();
  owned = false;
  MirrorRuntime::sPickupToken = 1;
  MirrorRuntime::DrawWorld();
  assert(mirrorDraws == 1 && worldShimmerDraws == 1);
  for (int reason = 0; reason < 3; ++reason) {
    Reset();
    LoadForest(-377, 80, 416);
    owned = false;
    if (reason == 0)
      play.roomCtx.curRoom.num = 1;
    else if (reason == 1)
      play.colCtx.colHeader = nullptr;
    else
      gComboGameMode =
          gComboGameMode == COMBO_GAME_MM ? COMBO_GAME_OOT : COMBO_GAME_MM;
    MirrorRuntime::DrawWorld();
    assert(mirrorDraws == 0 && worldShimmerDraws == 0 && matrixDepth == 0);
  }
  puts("PASS uncollected world Mirror alone requests native shimmer; "
       "owned/queued/invalid contexts suppress it");
}

void TestNativeChoiceLifecycle(bool combined) {
  Reset();
  if (combined)
    gComboGameMode = COMBO_GAME_BOTH;
  nativeMessages = true;
  play.state.input[0].press.button = BTN_CLEFT;
  MirrorRuntime::Update();
  assert(Message_GetState(&play.msgCtx) == TEXT_STATE_CHOICE);
  ExpectSounds({NA_SE_EV_TRE_BOX_FLASH});
  play.state.input[0].press.button = BTN_A;
  MirrorRuntime::Update();
  // A native close retains live text for two ticks. Forcing NONE here sends
  // native drawing back to DISPLAYING and the next update cancels the cast.
  assert(play.msgCtx.msgMode == MSGMODE_TEXT_CLOSING &&
         play.msgCtx.msgLength != 0);
  assert(MagicMirrorPlayer_State(&player) == MAGIC_MIRROR_CAST_PLAYING);
  ExpectSounds({NA_SE_EV_TRE_BOX_FLASH});
  // Confirmation is committed: extra A/B/C presses during the native close
  // cannot reopen the prompt, restart its timer or cancel the queued cast.
  play.state.input[0].press.button = BTN_A | BTN_B | BTN_CLEFT;
  MirrorRuntime::Update();
  assert(closed == 1 && animationStarts == 1 && play.msgCtx.stateTimer == 2);
  play.state.input[0].press.button = 0;
  for (int frame = 0; frame < 2; ++frame) {
    assert(MagicMirror_PromptOwnsPlayer(&play, &player));
    // Native closing-tail boundary; HUD/audio/ocarina cleanup is outside
    // this player fixture. The real GetState/ShouldAdvance/Close run above.
    if (--play.msgCtx.stateTimer == 0) {
      play.msgCtx.msgLength = 0;
      play.msgCtx.msgMode = MSGMODE_NONE;
      SetText(0);
    }
    MirrorRuntime::Update();
  }
  assert(closed == 1 && animationStarts == 1);
  assert(MagicMirrorPlayer_State(&player) == MAGIC_MIRROR_CAST_PLAYING);
  assert(castCameras == 1 && ownsCastCamera);
  ExpectSounds({NA_SE_EV_TRE_BOX_FLASH});
  for (int frame = 0; frame < 80 && !MirrorRuntime::sDeparted; ++frame) {
    player.actionFunc(&player, &play);
    MirrorRuntime::Update();
  }
  assert(MirrorRuntime::sDeparted);
  ExpectSounds({NA_SE_EV_TRE_BOX_FLASH, NA_SE_PL_MAGIC_WIND_WARP});
  assert(castCameras == 1);
  for (int frame = 0; frame < 5; ++frame)
    MirrorRuntime::Update();
  ExpectSounds({NA_SE_EV_TRE_BOX_FLASH, NA_SE_PL_MAGIC_WIND_WARP});
#ifdef MIRROR_OOT
  if (combined) {
    assert(handoffs == 1 && play.transitionTrigger == TRANS_TRIGGER_OFF);
  } else {
    assert(play.nextEntranceIndex == COMBO_SOLO_FOREST_OOT_ENTRANCE);
    assert(play.transitionTrigger == TRANS_TRIGGER_START);
  }
#else
  assert(play.nextEntrance ==
         (combined ? ENTRANCE(LOST_WOODS, 0) : COMBO_SOLO_FOREST_MM_ENTRANCE));
  assert(play.transitionTrigger == TRANS_TRIGGER_START);
#endif
  puts("PASS single native Yes waits for textbox teardown, casts once and "
       "reaches Lost Woods travel");
}

void TestCastStartsAndPersistsDuringPrompt() {
  for (u16 button : {BTN_CLEFT, BTN_CDOWN, BTN_CRIGHT, BTN_DUP, BTN_DDOWN,
                     BTN_DLEFT, BTN_DRIGHT}) {
    Reset();
    play.state.input[0].press.button = button;
    MirrorRuntime::Update();
    assert(MirrorRuntime::sPrompt &&
           MagicMirrorPlayer_State(&player) == MAGIC_MIRROR_CAST_PLAYING);
    assert(animationStarts == 1 && prompts == 1 && ownsCastCamera);
    MirrorRuntime::DrawWorld();
    assert(auraDraws == 1 && auraAlpha > 0 && auraAlpha < 255 &&
           mirrorDraws == 0);
    play.state.input[0].press.button = 0;
    for (int tick = 0; tick < 1200; ++tick) {
      ++play.gameplayFrames;
      player.actionFunc(&player, &play);
      MirrorRuntime::Update();
      MirrorRuntime::DrawWorld();
      assert(MirrorRuntime::sPrompt &&
             play.transitionTrigger == TRANS_TRIGGER_OFF && handoffs == 0);
    }
    assert(MagicMirrorPlayer_State(&player) == MAGIC_MIRROR_CAST_READY &&
           animationStarts == 1);
    assert(auraDraws == 1201 && auraScale > .99f && auraAlpha == 255 &&
           ownsCastCamera);
    ExpectSounds({NA_SE_EV_TRE_BOX_FLASH});
    player.stateFlags1 &= ~MirrorRuntime::kPromptFreeze;
    MirrorRuntime::Update(); // Reacquire a flag cleared by native cleanup.
    play.state.input[0].press.button = BTN_A;
    play.msgCtx.choiceIndex = 0;
    advance = true;
    MirrorRuntime::Update();
    assert(MirrorRuntime::sPromptAccepted &&
           play.transitionTrigger == TRANS_TRIGGER_OFF);
    CompletePromptClose();
    assert(animationStarts == 1 &&
           MagicMirrorPlayer_State(&player) == MAGIC_MIRROR_CAST_READY);
    assert(
        player.stateFlags1 &
        MirrorRuntime::kPromptFreeze); // Cast inherits prompt input ownership.
    MirrorRuntime::Update();
    assert(play.transitionTrigger == TRANS_TRIGGER_START &&
           animationStarts == 1);
    ExpectSounds({NA_SE_EV_TRE_BOX_FLASH, NA_SE_PL_MAGIC_WIND_WARP});
  }
  for (bool useB : {false, true}) {
    Reset();
    play.state.input[0].press.button = BTN_CLEFT;
    MirrorRuntime::Update();
    play.state.input[0].press.button = useB ? BTN_B : BTN_A;
    play.msgCtx.choiceIndex = 1;
    advance = true;
    MirrorRuntime::Update();
    assert(!MirrorRuntime::sPrompt && !MirrorRuntime::sCastPlay &&
           !ownsCastCamera);
    assert(player.actionFunc == NativeIdle &&
           !(player.stateFlags1 & MirrorRuntime::kPromptFreeze));
    MirrorRuntime::DrawWorld();
    assert(auraDraws == 0 && play.transitionTrigger == TRANS_TRIGGER_OFF &&
           handoffs == 0);
    ExpectSounds({NA_SE_EV_TRE_BOX_FLASH});
  }
  Reset();
  stow = true;
  play.state.input[0].press.button = BTN_CLEFT;
  MirrorRuntime::Update();
  assert(MirrorRuntime::sPrompt &&
         MagicMirrorPlayer_State(&player) == MAGIC_MIRROR_CAST_PUT_AWAY);
  MirrorRuntime::DrawWorld();
  assert(auraDraws == 1 && auraAlpha > 0 && animationStarts == 0);
  player.actionFunc(&player, &play);
  assert(animationStarts == 1 &&
         MagicMirrorPlayer_State(&player) == MAGIC_MIRROR_CAST_PLAYING);
  play.state.input[0].press.button = BTN_B;
  MirrorRuntime::Update();
  assert(!MirrorRuntime::sCastPlay && player.actionFunc == NativeIdle);
  puts("PASS equipped C/D starts ritual immediately; long prompt sustains aura "
       "without travel; late Yes never replays; No/B and holster clean up");
}

void TestShuffledStumpIndependentOfMirrorOwnership() {
  for (bool mirrorOwned : {false, true}) {
    Reset();
    shuffled = true;
    owned = mirrorOwned;
    LoadForest(-377, 80, 416);
    play.transitionTrigger = TRANS_TRIGGER_OFF;
    player.actor.world.pos = {-377, 80, 416};
    player.actor.floorHeight = 80;
    player.actor.floorBgId = BGCHECK_SCENE;
    player.actor.floorPoly = play.colCtx.colHeader->polyList;
    MirrorRuntime::Update();
    assert(pickups == 1 && MirrorRuntime::sPickupToken != 0);
    const int token = MirrorRuntime::sPickupToken;
    MirrorRuntime::FinishPickup(token + 1);
    assert(MirrorRuntime::sPickupToken == token);
    MirrorRuntime::FinishPickup(token);
    assert(MirrorRuntime::sPickupToken == 0);
    assert(owned == mirrorOwned &&
           saves == 0); // Native rando backend owns reward and collection.
    assert(stumpReceiptPending);
    const int cancelledBefore = stumpReceiptCancels;
    MirrorRuntime::Reset(); // Same PlayState/player/scene addresses, after
                            // native acceptance.
    assert(!stumpReceiptPending && stumpReceiptCancels == cancelledBefore + 1);
    assert(!MirrorRuntime::FinishPickup(token));
    stumpCollected = true;
    MirrorRuntime::Update();
    MirrorRuntime::DrawWorld();
    assert(pickups == 1 && mirrorDraws == 0 && worldShimmerDraws == 0);
  }
  puts("PASS shuffled stump remains collectible independently of Mirror "
       "ownership; token guarded, no fixed grant and collected display "
       "suppressed");
}

void TestArrivalRetainsPreparedItemCamera() {
#ifndef MIRROR_OOT
  for (int variant = 0; variant < 3; ++variant) {
    Reset();
    MagicMirror_QueueArrival();
    LoadForest(-377, 80, 416);
    ActorEntry original{};
    original.rot.y = 204 << 7; // Private prepared chain starts at zero.
    play.linkActorEntry = &original;
    ActorCutscene cameras[2]{};
    cameras[0].csCamId = CS_CAM_ID_GLOBAL_ITEM_OCARINA;
    cameras[0].scriptIndex = CS_SCRIPT_ID_NONE;
    cameras[0].additionalCsId = variant == 0 ? CS_ID_NONE : 1;
    cameras[1].csCamId = variant == 2 ? CS_CAM_ID_GLOBAL_ITEM_OCARINA
                                      : CS_CAM_ID_GLOBAL_ITEM_GET;
    cameras[1].scriptIndex = CS_SCRIPT_ID_NONE;
    cameras[1].additionalCsId = CS_ID_NONE;
    testSceneCutscenes = cameras;
    sSceneCutsceneCount = variant == 0 ? 1 : 2;
    MagicMirror_PrepareArrival(&play);
    assert(play.linkActorEntry != &original);
    assert((play.linkActorEntry->rot.y & 127) == (variant == 2 ? 127 : 0));
    MagicMirror_FinishArrival(&play);
  }
  puts("PASS Mirror recall retains legacy/native-item camera chains and "
       "rejects an unrelated successor");
#endif
}

int main() {
  TestArrivalRetainsPreparedItemCamera();
  TestShuffledStumpIndependentOfMirrorOwnership();
  TestCastStartsAndPersistsDuringPrompt();
  Reset();
  stow = true;
  ConfirmCast();
  assert(MagicMirrorPlayer_State(&player) == MAGIC_MIRROR_CAST_PUT_AWAY);
  assert(castCameras == 0 && !ownsCastCamera);
  player.afterPutAwayFunc(&play, &player);
  MirrorRuntime::Update();
  assert(castCameras == 1 && ownsCastCamera);
  player.actionFunc = NativeIdle; // Damage/another action interrupts our cast.
  MirrorRuntime::Update();
  assert(releasedCameras == 1 && !ownsCastCamera);
  ExpectSounds({NA_SE_EV_TRE_BOX_FLASH}); // Interrupted cast never departs.
  Reset();
  ConfirmCast();
  Player replacement{};
#ifdef MIRROR_OOT
  play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &replacement.actor;
#else
  play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &replacement.actor;
#endif
  MirrorRuntime::Update();
  assert(releasedCameras == 1 && !ownsCastCamera && idleRestores == 0);
  Reset();
  assert(!MagicMirrorPlayer_Begin(nullptr, &player));
  assert(!MagicMirrorPlayer_Begin(&play, nullptr));
  ExpectSounds({});
  assert(MagicMirrorPlayer_Begin(&play, &player));
  ExpectSounds({});
  assert(!MagicMirrorPlayer_Begin(&play, &player));
  player.actionFunc(&player, &play);
  ExpectSounds({});
  MagicMirrorPlayer_End(&play, &player);
  ExpectSounds({});
  Reset();
  stow = true;
  assert(MagicMirrorPlayer_Begin(&play, &player));
  ExpectSounds({});
  MagicMirrorPlayer_End(&play, &player);
  ExpectSounds({});
  TestNativeChoiceLifecycle(false);
  TestNativeChoiceLifecycle(true);
  TestPromptOwnsInputDuringNativeStateChanges();
  TestMarioPromptInputGate();
  TestPromptMovement();
  TestChoiceReleaseInput();
  TestPromptOwnershipLifetime();
  TestArrival();
  TestStumpTopPickup();
  TestWorldRewardShimmer();
  assert(FcEquip_OotToMm16(EXT_ITEM_MAGIC_MIRROR) == EXT_ITEM_MAGIC_MIRROR);
  assert(FcEquip_MmToOot16(EXT_ITEM_MAGIC_MIRROR) == EXT_ITEM_MAGIC_MIRROR);
  assert(FcEquip_OotToMm16(EXT_ITEM_MAGIC_MIRROR + 1) == 0xFF);
  assert(MirrorRuntime::kPromptText < 0x4E20);
  Reset();
  play.state.input[0].press.button = BTN_CLEFT;
  MirrorRuntime::Update();
  assert(prompts == 1 && !(play.state.input[0].press.button & BTN_CLEFT));
  assert(player.stateFlags1 & MirrorRuntime::kPromptFreeze);
  // An unsafe change must release the private choice box and its owned freeze.
  player.actor.bgCheckFlags = 0;
  MirrorRuntime::Update();
  assert(closed == 1 && play.msgCtx.msgMode == MSGMODE_TEXT_CLOSING);
  assert(!(player.stateFlags1 & MirrorRuntime::kPromptFreeze));
  Reset();
  play.state.input[0].press.button = BTN_CLEFT;
  MirrorRuntime::Update();
  SetText(1234);
  MirrorRuntime::Update();
  assert(closed == 0); // never close another actor's replacement message
  assert(!(player.stateFlags1 & MirrorRuntime::kPromptFreeze));
  Reset();
  play.state.input[0].press.button = BTN_CLEFT;
  MirrorRuntime::Update();
  play.state.input[0].press.button = BTN_B;
  advance = true;
  MirrorRuntime::Update();
  assert(closed == 1 && play.transitionTrigger == TRANS_TRIGGER_OFF);
  ExpectSounds({NA_SE_EV_TRE_BOX_FLASH});
  Reset();
  play.state.input[0].press.button = BTN_CLEFT;
  MirrorRuntime::Update();
  play.state.input[0].press.button = BTN_A;
  play.msgCtx.choiceIndex = 1;
  advance = true;
  MirrorRuntime::Update();
  assert(closed == 1 && play.transitionTrigger == TRANS_TRIGGER_OFF);
  ExpectSounds({NA_SE_EV_TRE_BOX_FLASH});
  Reset();
  play.state.input[0].press.button = BTN_CLEFT;
  MirrorRuntime::Update();
  play.state.input[0].press.button = BTN_A;
  play.msgCtx.choiceIndex = 0;
  advance = true;
  MirrorRuntime::Update();
  // Confirming commits to a visible cast before changing worlds.
  assert(closed == 1 && play.transitionTrigger == TRANS_TRIGGER_OFF);
  CompletePromptClose();
  assert(MagicMirrorPlayer_State(&player) == MAGIC_MIRROR_CAST_PLAYING);
  assert(animationStarts == 1 &&
         (player.stateFlags1 & MirrorRuntime::kPromptFreeze));
  player.bodyPartsPos[PLAYER_BODYPART_HEAD] = {10, 60, 30};
  MirrorRuntime::DrawWorld();
  assert(mirrorDraws == 0 && auraDraws == 1 && auraAlpha > 0 &&
         auraAlpha < 255); // Immediate growing ritual, no overhead item.
  const auto castSave = gSaveContext;
  // New presses cannot reopen the prompt or restart/skip the committed
  // motion.
  play.state.input[0].press.button = BTN_B | BTN_CLEFT;
  for (int tick = 0; tick < 80 && play.transitionTrigger == TRANS_TRIGGER_OFF;
       ++tick) {
    player.actionFunc(&player, &play);
    MirrorRuntime::Update();
    MirrorRuntime::DrawWorld();
    assert(mirrorDraws ==
           0); // Even the raised-hands finish is ritual energy only.
    if (MagicMirrorPlayer_State(&player) != MAGIC_MIRROR_CAST_READY)
      assert(play.transitionTrigger == TRANS_TRIGGER_OFF);
  }
  assert(play.transitionTrigger == TRANS_TRIGGER_START && prompts == 1 &&
         animationStarts == 1);
  assert(MagicMirrorPlayer_State(&player) == MAGIC_MIRROR_CAST_READY);
  MirrorRuntime::DrawWorld();
  assert(mirrorDraws == 0 && auraDraws > 0 && matrixDepth == 0);
  assert(auraScale > 0.9f && auraAlpha > 200 &&
         auraOrigin.x == player.actor.world.pos.x);
  alternateModel = true;
  player.actor.world.pos = {100, 20, -50};
  MirrorRuntime::DrawWorld();
  assert(mirrorDraws == 0 && worldShimmerDraws == 0 && matrixDepth == 0);
  assert(auraOrigin.x == 100 && auraOrigin.y == 20 && auraOrigin.z == -50);
  alternateModel = false;
  const float fullAlpha = auraAlpha;
  MirrorRuntime::Update();
  MirrorRuntime::DrawWorld();
  assert(auraAlpha > 0 && auraAlpha < fullAlpha);
  for (int tick = 0; tick < 12; ++tick)
    MirrorRuntime::Update();
  const int fadedDraws = auraDraws;
  MirrorRuntime::DrawWorld();
  assert(auraDraws == fadedDraws && mirrorDraws == 0);
  assert(std::memcmp(&castSave.respawn, &gSaveContext.respawn,
                     sizeof(castSave.respawn)) == 0);
#ifdef MIRROR_OOT
  assert(castSave.magic == gSaveContext.magic);
#else
  assert(castSave.save.saveInfo.playerData.magic ==
         gSaveContext.save.saveInfo.playerData.magic);
#endif
  MirrorRuntime::Update(); // Fade holds the raised-hands pose until scene
                           // teardown.
  assert(MagicMirrorPlayer_State(&player) == MAGIC_MIRROR_CAST_READY);
  MirrorRuntime::Reset();
  assert(idleRestores == 1 &&
         !(player.stateFlags1 & MirrorRuntime::kPromptFreeze));
  MirrorRuntime::DrawWorld();
  assert(
      auraDraws ==
      fadedDraws); // No sampled cast energy survives cancellation or teardown.
  Reset();
  gComboGameMode = COMBO_GAME_BOTH;
  ConfirmCast();
  for (int tick = 0; tick < 80 && !MirrorRuntime::sDeparted; ++tick) {
    player.actionFunc(&player, &play);
    MirrorRuntime::Update();
  }
  assert(MirrorRuntime::sDeparted);
  MirrorRuntime::Update();
#ifdef MIRROR_OOT
  assert(handoffs == 1 && play.transitionTrigger == TRANS_TRIGGER_OFF);
  Reset();
  gComboGameMode = COMBO_GAME_BOTH;
  handoffAccepted = false;
  assert(!MirrorRuntime::Travel(&play));
  ExpectSounds({}); // Rejected host transfer must not announce a departure.
#else
  assert(play.nextEntrance == ENTRANCE(LOST_WOODS, 0));
#endif
  // Native held-item put-away completes before the flourish begins.
  Reset();
  stow = true;
  assert(MagicMirrorPlayer_Begin(&play, &player));
  assert(MagicMirrorPlayer_State(&player) == MAGIC_MIRROR_CAST_PUT_AWAY &&
         animationStarts == 0);
  player.actionFunc(&player, &play);
  assert(MagicMirrorPlayer_State(&player) == MAGIC_MIRROR_CAST_PLAYING &&
         animationStarts == 1);
  MagicMirrorPlayer_End(&play, &player);
  assert(player.actionFunc == NativeIdle);
  // An external native action keeps ownership if it interrupts the cast.
  assert(MagicMirrorPlayer_Begin(&play, &player));
  NativeSetAction(&player, NativeIdle);
  MagicMirrorPlayer_End(&play, &player);
  assert(idleRestores == 1);
  // A cast is revoked if its ownership, scene safety or private context
  // changes.
  for (int reason = 0; reason < 6; ++reason) {
    Reset();
    ConfirmCast();
    assert(MagicMirrorPlayer_State(&player) == MAGIC_MIRROR_CAST_PLAYING);
    switch (reason) {
    case 0:
      owned = false;
      break;
    case 1:
      player.actor.bgCheckFlags = 0;
      break;
    case 2:
      play.pauseCtx.state = 1;
      break;
    case 3:
      play.msgCtx.msgMode = MSGMODE_TEXT_DISPLAYING;
      break;
    case 4:
      pending = true;
      break;
    case 5:
      ++seed;
      break;
    }
    MirrorRuntime::Update();
    assert(!MirrorRuntime::sCastPlay && player.actionFunc == NativeIdle);
    assert(play.transitionTrigger == TRANS_TRIGGER_OFF && handoffs == 0);
  }
  Reset();
  ConfirmCast();
  NativeSetAction(&player, NativeIdle);
  MirrorRuntime::Update();
  assert(!MirrorRuntime::sCastPlay && idleRestores == 0);
  Reset();
  stow = true;
  ConfirmCast();
  for (int tick = 0; tick < 200; ++tick)
    MirrorRuntime::Update();
  assert(!MirrorRuntime::sCastPlay && player.actionFunc == NativeIdle);
  assert(play.transitionTrigger == TRANS_TRIGGER_OFF);
  // MM's destroy hook runs after arena cleanup: never access a freed player.
  Reset();
  auto *retiredPlayer = new Player(player);
#ifdef MIRROR_OOT
  play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &retiredPlayer->actor;
#else
  play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &retiredPlayer->actor;
#endif
  ConfirmCast();
  delete retiredPlayer;
  MirrorRuntime::Destroy();
  assert(!MirrorRuntime::sCastPlay && idleRestores == 0);
  Reset();
  assert(MirrorRuntime::Safe(&play, false));
  assert(MirrorRuntime::AllowedScene(&play));
  const auto previousReturn = gSaveContext.respawn[RESPAWN_MODE_RETURN];
  const auto previousTop = gSaveContext.respawn[RESPAWN_MODE_TOP];
#ifdef MIRROR_OOT
  gSaveContext.cutsceneIndex = 0xFFF3;
#else
  gSaveContext.save.cutsceneIndex = 0xFFF3;
#endif
  assert(MirrorRuntime::Travel(&play));
  assert(play.transitionTrigger == TRANS_TRIGGER_START);
#ifdef MIRROR_OOT
  assert(play.nextEntranceIndex == COMBO_SOLO_FOREST_OOT_ENTRANCE);
  assert(gSaveContext.cutsceneIndex == 0);
#else
  assert(play.nextEntrance == COMBO_SOLO_FOREST_MM_ENTRANCE);
  assert(gSaveContext.save.cutsceneIndex == 0);
#endif
  assert(std::memcmp(&previousReturn,
                     &gSaveContext.respawn[RESPAWN_MODE_RETURN],
                     sizeof(previousReturn)) == 0);
  assert(std::memcmp(&previousTop, &gSaveContext.respawn[RESPAWN_MODE_TOP],
                     sizeof(previousTop)) == 0);
  Reset();
  gComboGameMode = COMBO_GAME_BOTH;
  assert(MirrorRuntime::Travel(&play));
#ifdef MIRROR_OOT
  assert(handoffs == 1 && play.transitionTrigger == TRANS_TRIGGER_OFF);
#else
  assert(play.nextEntrance == ENTRANCE(LOST_WOODS, 0));
#endif
  Reset();
  owned = false;
  assert(!MirrorRuntime::Travel(&play));
  ExpectSounds({});
  Reset();
  pack = false;
  assert(!MirrorRuntime::Travel(&play));
  Reset();
  blocked = true;
  assert(!MirrorRuntime::Travel(&play));
  Reset();
  player.actor.bgCheckFlags = 0;
  assert(!MirrorRuntime::Travel(&play));
  Reset();
  play.pauseCtx.state = 1;
  assert(!MirrorRuntime::Travel(&play));
  Reset();
  play.transitionTrigger = TRANS_TRIGGER_START;
  assert(!MirrorRuntime::Travel(&play));
  Reset();
  play.csCtx.state = 1;
  assert(!MirrorRuntime::Travel(&play));
  Reset();
  gSaveContext.fileNum = 0xFF;
  assert(!MirrorRuntime::Travel(&play));
  Reset();
  gSaveContext.gameMode = GAMEMODE_TITLE_SCREEN;
  assert(!MirrorRuntime::Travel(&play));
  Reset();
#ifdef MIRROR_OOT
  Scene(SCENE_DEKU_TREE);
  gComboGameMode = COMBO_GAME_MM;
#else
  Scene(SCENE_MITURIN);
  gComboGameMode = COMBO_GAME_OOT;
#endif
  assert(!MirrorRuntime::Travel(&play));
  // A stale deferred pickup never grants into another file, seed, scene, or
  // mode.
  Reset();
  owned = false;
  MirrorRuntime::Bind(&play);
  auto token = MirrorRuntime::BeginPickup();
  assert(token > 0);
  gSaveContext.fileNum = 2;
  MirrorRuntime::FinishPickup(token);
  assert(!owned);
  Reset();
  owned = false;
  MirrorRuntime::Bind(&play);
  token = MirrorRuntime::BeginPickup();
  ++seed;
  MirrorRuntime::FinishPickup(token);
  assert(!owned);
  Reset();
  owned = false;
  MirrorRuntime::Bind(&play);
  token = MirrorRuntime::BeginPickup();
  MirrorRuntime::Reset();
  MirrorRuntime::FinishPickup(token);
  assert(!owned);
  Reset();
  owned = false;
  MirrorRuntime::Bind(&play);
  token = MirrorRuntime::BeginPickup();
  MirrorRuntime::FinishPickup(token);
  assert(owned && saves == 1);
  MirrorRuntime::FinishPickup(token);
  assert(saves == 1);
  puts("PASS native mirror: unchanged Farore cast, sampled ritual aura without "
       "overhead item, fade hold, "
       "safe cancellation/teardown, preserved magic/respawns, travel and "
       "pickup");
}
