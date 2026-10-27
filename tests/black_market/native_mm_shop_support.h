// All shop actions, joystick state, item tween, cursor GBI, camera functions
// and cutscene manager below are actual source. Only service/resource
// boundaries are controlled. No modeled replacement of a native shop action is
// linked.
#include "2s2h/GameInteractor/GameInteractor.h"
#include "black_market_shop_adapter.h"
#include "global.h"
extern "C" {
#include "z64shrink_window.h"
}
#include "native_mm_shop_boundary.h"
#include "objects/gameplay_keep/gameplay_keep.h"
#include "overlays/actors/ovl_En_Ossan/z_en_ossan.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <libultraship/bridge/consolevariablebridge.h>
#include <string>
#include <vector>

SaveContext gSaveContext{};
PlayState *gPlayState;
static PlayState play;
static Player player;
static EnOssan shop;
static std::array<EnGirlA, 8> stock;
static std::array<Vec3s, 24> galJoints, galMorphs;
static Camera mainCamera;
static GraphicsContext graphics;
static std::array<Gfx, 2048> overlay;
static std::array<std::array<Vec3s, 3>, 4> cameraData;
static std::array<ActorCsCamInfo, 4> cameraInfo;
static std::array<ActorCutscene, 4> cutscenes;
static bool market, talkAccepted, advance, canOffer, canBegin, canStart,
    reserveWorks, receiptDone, mirror, initWorks = true;
static int messageState, objectLookups, objectLoads, spawns, kills,
    adapterInits, beginCalls, startCalls;
static int nativeSkeletonInits, animationChanges, invert = 1, receiptState,
                                                  reservations, offers;
static int playerWaits, playerEnds, collisionQueries, buyResult;
static std::vector<std::string> callbacks;
static std::vector<std::string> trace;
static std::vector<std::string> nativeCalls;
static u16 lastAAction, lastHud;
static int lastLetterbox, hudCalls, letterboxCalls;
static bool userDisableBlackBars;

extern "C" u8 FixtureMessageState(MessageContext *) { return messageState; }
extern "C" s32 FixtureAdvance(PlayState *) { return advance; }
extern "C" void FixtureContinue(PlayState *, u16 text) {
  play.msgCtx.currentTextId = text;
  messageState =
      (text == 0x640 || text == 0x642 || text == 0x8F13 || text == 0x8F14 ||
       text == 0x8F16 || text == 0x8F17 || (text >= 0x700 && text < 0x708))
          ? TEXT_STATE_CHOICE
          : TEXT_STATE_EVENT;
  play.msgCtx.textboxEndType = messageState == TEXT_STATE_CHOICE
                                   ? TEXTBOX_ENDTYPE_TWO_CHOICE
                                   : TEXTBOX_ENDTYPE_DEFAULT;
}
extern "C" void FixtureStart(PlayState *state, u16 text, Actor *) {
  FixtureContinue(state, text);
}
extern "C" void FixtureClose(PlayState *) { messageState = TEXT_STATE_CLOSING; }
extern "C" s32 FixtureTalk(Actor *, GameState *) {
  bool result = talkAccepted;
  talkAccepted = false;
  return result;
}
extern "C" s32 FixtureOfferTalk(Actor *, PlayState *, f32) {
  ++offers;
  return true;
}
extern "C" void FixtureSkeleton(PlayState *, SkelAnime *skel,
                                FlexSkeletonHeader *, AnimationHeader *,
                                Vec3s *joints, Vec3s *morphs, s32 count) {
  ++nativeSkeletonInits;
  skel->jointTable = joints;
  skel->morphTable = morphs;
  skel->limbCount = count;
}
extern "C" s32 Object_GetSlot(ObjectContext *, s16) {
  ++objectLookups;
  return 1;
}
extern "C" s32 Object_IsLoaded(ObjectContext *, s32) {
  ++objectLoads;
  return true;
}
extern "C" void Actor_Kill(Actor *actor) {
  ++kills;
  actor->update = nullptr;
}
extern "C" void Actor_ProcessInitChain(Actor *, InitChainEntry *) {}
extern "C" void Actor_SetObjectDependency(PlayState *, Actor *) {}
extern "C" void ActorShape_Init(ActorShape *, f32, ActorShadowFunc, f32) {}
extern "C" void ActorShadow_DrawCircle(Actor *, Lights *, PlayState *) {}
extern "C" void Actor_SetScale(Actor *actor, f32 value) {
  actor->scale = {value, value, value};
}
extern "C" s32 Collider_DestroyCylinder(PlayState *, ColliderCylinder *) {
  return true;
}
extern "C" s32 SubS_ChangeAnimationByInfoS(SkelAnime *, AnimationInfoS *, s32) {
  ++animationChanges;
  return true;
}
extern "C" s32 SubS_UpdateFidgetTables(PlayState *, s16 *, s16 *, s32) {
  return true;
}
extern "C" s32 Actor_TrackPlayer(PlayState *, Actor *, Vec3s *, Vec3s *,
                                 Vec3f) {
  return true;
}
extern "C" s32 SkelAnime_Update(SkelAnime *) { return true; }
extern "C" s16 Animation_GetLastFrame(void *) { return 30; }
extern "C" s32 Animation_OnFrame(SkelAnime *, f32) { return false; }
extern "C" void Actor_PlaySfx(Actor *, u16) {}
extern "C" f32 Rand_ZeroOne() { return .5f; }
extern "C" u16 Text_GetFaceReaction(PlayState *, FaceReactionSet) { return 0; }
extern "C" void Flags_SetWeekEventReg(s32 flag) {
  WEEKEVENTREG(flag >> 8) |= flag & 0xFF;
}
extern "C" void Graph_OpenDisps(Gfx **, Gfx *, GraphicsContext *, const char *,
                                s32) {}
extern "C" void Graph_CloseDisps(Gfx **, Gfx *, GraphicsContext *, const char *,
                                 s32) {}
extern "C" void Audio_PlaySfx(u16) {}
extern "C" void Audio_PlaySfx_MessageDecide() {}
extern "C" void Audio_PlaySfx_MessageCancel() {}
extern "C" void Interface_SetHudVisibility(u16 value) {
  lastHud = value;
  ++hudCalls;
}
extern "C" void Interface_SetAButtonDoAction(PlayState *, u16 value) {
  lastAAction = value;
}
extern "C" void ShrinkWindow_Letterbox_SetSizeTarget(s32 value) {
  lastLetterbox = value;
  ++letterboxCalls;
}
extern "C" s32 Actor_OfferGetItem(Actor *, PlayState *, GetItemId, f32, f32) {
  callbacks.push_back("offer");
  return true;
}
extern "C" s32 Actor_OfferTalkExchangeEquiCylinder(Actor *, PlayState *, f32,
                                                   PlayerItemAction) {
  return true;
}
extern "C" s32 Player_SetCsActionWithHaltedActors(PlayState *, Actor *,
                                                  u8 action) {
  if (action == PLAYER_CSACTION_WAIT)
    ++playerWaits;
  else if (action == PLAYER_CSACTION_END)
    ++playerEnds;
  else
    assert(false);
  return true;
}
extern "C" int GameInteractor_InvertControl(GIInvertType type) {
  assert(type == GI_INVERT_SHOP_X);
  return invert;
}
extern "C" int CVarGetInteger(const char *name, int fallback) {
  if (std::strcmp(name, "gEnhancements.Graphics.DisableBlackBars") == 0)
    return userDisableBlackBars;
  return std::strcmp(name, "gModes.MirroredWorld.State") == 0 ? mirror
                                                              : fallback;
}
extern "C" float CVarGetFloat(const char *, float fallback) { return fallback; }
extern "C" f32 Player_GetHeight(Player *) { return 56; }
extern "C" f32 Math_FAtan2F(f32 y, f32 x) { return std::atan2(y, x); }
extern "C" s32 BgCheck_CameraLineTest1(CollisionContext *, Vec3f *, Vec3f *,
                                       Vec3f *, CollisionPoly **, s32, s32, s32,
                                       s32, s32 *) {
  ++collisionQueries;
  return false;
}
extern "C" u16 BgCheck_GetBgCamSettingImpl(CollisionContext *, u32, s32) {
  assert(false);
  return 0;
}
extern "C" Vec3s *BgCheck_GetBgCamFuncDataImpl(CollisionContext *, s32, s32) {
  assert(false);
  return nullptr;
}
extern "C" void func_800E0348(Camera *) { assert(false); }
extern "C" void Cutscene_StartScripted(PlayState *, u8) { assert(false); }
extern "C" void Gfx_SetupDL39_Overlay(GraphicsContext *) {}
extern "C" void Gfx_SetupDL25_Opa(GraphicsContext *) {}
extern "C" void FrameInterpolation_RecordOpenChild(const void *, int) {}
extern "C" void FrameInterpolation_RecordCloseChild() {}
extern "C" void gSPSegment(void *packet, int segment, uintptr_t target) {
  __gSPSegment(static_cast<Gfx *>(packet), segment, target);
}
extern "C" void gDPSetPrimColorOverride(Gfx *packet, u8 m, u8 l, u8 r, u8 g,
                                        u8 b, u8 a, const char *) {
  gDPSetPrimColor(packet, m, l, r, g, b, a);
}
static bool ComboOpeningForest_IsExploration(PlayState *) { return false; }
#define GameInteractor_Should(...) true
#define GameInteractor_ExecuteOnCameraChangeSettingsFlags(...) ((void)0)
#define GameInteractor_ExecuteOnCameraChangeModeFlags(...) ((void)0)
#define __osMemset std::memset
#define RELOAD_PARAMS(camera)                                                  \
  ((camera)->animState == 0 || (camera)->animState == 10 ||                    \
   (camera)->animState == 20)
#define CAM_RODATA_UNSCALE(x) ((x)*0.01f)
#define GET_NEXT_RO_DATA(values) ((values++)->val)
#define GET_NEXT_SCALED_RO_DATA(values)                                        \
  CAM_RODATA_UNSCALE(GET_NEXT_RO_DATA(values))
#define CAM_DATA_IS_BG (1 << 12)
#define CAM_CHANGE_SETTING_0 (1 << 0)
#define CAM_CHANGE_SETTING_1 (1 << 1)
#define CAM_CHANGE_SETTING_2 (1 << 2)
#define CAM_CHANGE_SETTING_3 (1 << 3)
#define CAM_CHANGE_MODE_0 (1 << 0)
#define CAM_CHANGE_MODE_1 (1 << 1)
#define CAM_CHANGE_MODE_BATTLE (1 << 2)
#define CAM_CHANGE_MODE_FOLLOW_TARGET (1 << 3)
#define CAM_CHANGE_MODE_4 (1 << 4)
#define CAM_CHANGE_MODE_FIRST_PERSON (1 << 5)
PlayState *sCamPlayState;
s16 sCameraNextUID;
s32 sCameraInterfaceFlags, sCameraHudVisibility, sCameraLetterboxSize,
    sCameraInitSceneTimer;
/* NATIVE CAMERA AND MANAGER */

#define Message_GetState FixtureMessageState
#define Message_ShouldAdvance FixtureAdvance
#define Message_ContinueTextbox FixtureContinue
#define Message_StartTextbox FixtureStart
#define Message_CloseTextbox FixtureClose
#define Actor_TalkOfferAccepted FixtureTalk
#define Actor_OfferTalk FixtureOfferTalk
#define SkelAnime_InitFlex FixtureSkeleton
#define SkelAnime_DrawFlexOpa(...) ((void)0)
