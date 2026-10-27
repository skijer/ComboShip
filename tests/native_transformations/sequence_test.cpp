#include "z64.h"
#include "functions.h"
#include "macros.h"
#include "mods/transformation_masks/transformation_masks.h"
#include "mods/transformation_masks/assets/mm_asset_loader.h"
#include "mods/anim_translator/mm_anim_loader.h"
#include "mods/sound_translator/mm_sfx_ids.h"
#include "mods/extended_player.h"
#include "mods/items/helpers/camera_helper.h"
extern "C" {
#include "soh/framebuffer_effects.h"
uint32_t OTRGetGameRenderWidth();
uint32_t OTRGetGameRenderHeight();
}
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

/* PRODUCTION_STATE_ENUM */
struct {
    MmFormStateId state;
    MmPlayerTransformation currentForm, targetForm;
    s16 cutsceneTimer, cutscenePhase, flashAlpha;
    SkelAnime formSkelAnime;
    LinkAnimationHeader* idleAnim;
    LinkAnimationHeader* maskOffStart;
    u8 skeletonLoaded, zoraSwimEnabled;
    u8 bubbleCharging;
    f32 bubbleCharge, dekuCheekScale;
    s16 bubbleChargeTimer;
    ItemCameraState bubbleCameraState;
} gFormState;
static u8 sForceInstantTransform, sCutsceneMaskAnim;
#define MMFORM_IS_ZORA_SWIM() (gFormState.currentForm == MM_PLAYER_FORM_ZORA || gFormState.zoraSwimEnabled)
#define ZORA_SWIM_THRESHOLD 20.0f
#define MMFORM_LOG(...) ((void)0)
struct MmFormProperties { f32 cameraHeight; };
static MmFormProperties sFormProps[MM_PLAYER_FORM_MAX] = {{124}, {80}, {68}, {36}, {44}};
static Player player;
static PlayState play;
static Camera camera;
static LinkAnimationHeader animations[MM_ANIM_MAX];
static std::vector<std::pair<int, int>> sounds;
static std::vector<int> stopped;
static int frame, instant, missing = -1, loadCount, restoreCount, restrictCount, equipCount;
static bool skeletonFailure;
static bool maskFailure, cameraEnabled;
static int cameraCreates, cameraClears, lightCreates, lightClears;
static int blurCopies, blurDraws;
static int putAwayCount;
static bool sDekuBubbleActive;
static u32 renderWidth = 320, renderHeight = 240;
static LightNode nativeLight;
static LinkAnimationHeader* lastStarted;
static f32 cameraRandom = 0.25f;
extern "C" SaveContext gSaveContext;
SaveContext gSaveContext;
PlayState* gPlayState = &play;
static u8 MmForm_LoadFormSkeleton(PlayState*, MmPlayerTransformation);
static void MmForm_ApplyFormProperties(Player*, MmPlayerTransformation) {}
static void MmForm_RestoreOotState(Player* p) {
    ++restoreCount;
    p->stateFlags1 &= ~PLAYER_STATE1_INPUT_DISABLED;
    p->stateFlags3 &= ~PLAYER_STATE3_PAUSE_ACTION_FUNC;
}
static void MmForm_RestoreEquips(PlayState*) { ++equipCount; }
static void MmForm_SaveAndRestrictEquips(PlayState*) { ++restrictCount; }
static void MmForm_EnterSwimIdle(Player*, PlayState*) {}
static void ExtEquip_UnequipForTransform() {}
static void ExtEquip_RestoreFromTransform() {}
static u8 WolfLinkForm_IsSelected() { return 0; }
static void WolfLinkForm_PlayTransformSfx(u8) {}

extern "C" {
s32 CVarGetInteger(const char*, s32) { return instant; }
LinkAnimationHeader* MmAnim_Load(MmAnimId id) { return id == missing ? nullptr : &animations[id]; }
void LinkAnimation_Change(PlayState*, SkelAnime* skel, LinkAnimationHeader* anim, f32 speed,
                          f32 start, f32 end, u8 mode, f32) {
    assert(anim != nullptr);
    lastStarted = anim;
    skel->animation = anim;
    skel->curFrame = start;
    skel->playSpeed = speed;
    skel->startFrame = start;
    skel->endFrame = end;
    skel->mode = mode;
}
s16 Animation_GetLastFrame(void* animation) {
    return ((LinkAnimationHeader*)animation)->common.frameCount - 1;
}
void LinkAnimation_PlayOnce(PlayState* p, SkelAnime* skel, LinkAnimationHeader* anim) {
    LinkAnimation_Change(p, skel, anim, 1.0f, 0, Animation_GetLastFrame(anim), ANIMMODE_ONCE, 0);
}
void LinkAnimation_PlayLoop(PlayState* p, SkelAnime* skel, LinkAnimationHeader* anim) {
    LinkAnimation_Change(p, skel, anim, 1.0f, 0, Animation_GetLastFrame(anim), ANIMMODE_LOOP, 0);
}
s32 LinkAnimation_Update(PlayState*, SkelAnime* skel) {
    if (!skel->animation) return 0;
    const f32 next = skel->curFrame + skel->playSpeed * 1.5f;
    if (skel->playSpeed < 0) {
        skel->curFrame = std::max(skel->endFrame, next);
        return skel->curFrame == skel->endFrame;
    }
    if (skel->mode == ANIMMODE_LOOP) {
        skel->curFrame = next > skel->endFrame ? skel->startFrame : next;
        return 0;
    }
    skel->curFrame = std::min(skel->endFrame, next);
    return next >= skel->endFrame;
}
s32 LinkAnimation_OnFrame(SkelAnime* skel, f32 target) {
    return skel->curFrame >= target && skel->curFrame - skel->playSpeed * 1.5f < target;
}
void Player_SetBootData(PlayState*, Player*) {}
// A sword-to-empty change is asynchronous in Player_UseItem. Pausing the
// upper-body update must not strand that request during a mask cutscene.
void Player_UseItem(PlayState*, Player* p, s32 item) {
    assert(item == ITEM_NONE);
    p->heldItemId = item;
    if (p->heldItemAction != PLAYER_IA_NONE) p->stateFlags1 |= PLAYER_STATE1_START_CHANGING_HELD_ITEM;
}
void CustomItems_PutAwayHeldItems(Player*, PlayState*) { ++putAwayCount; }
void ItemEquip_ResetUnequipSound(PlayState*, Player*, s32) {}
void Player_InitDefaultIA(PlayState*, Player*) {}
s32 func_8083485C(Player*, PlayState*) { return 0; }
ItemActionInitFunc ExtPlayer_GetItemActionInitFunc(int32_t item) { assert(item == PLAYER_IA_NONE); return Player_InitDefaultIA; }
ItemActionUpdateFunc ExtPlayer_GetItemActionUpdateFunc(int32_t item) { assert(item == PLAYER_IA_NONE); return func_8083485C; }
s32 Player_ActionToModelGroup(Player*, s32 item) { assert(item == PLAYER_IA_NONE); return PLAYER_MODELGROUP_DEFAULT; }
void Player_SetModelGroup(Player* p, s32 group) {
    p->modelGroup = group;
    p->leftHandType = PLAYER_MODELTYPE_LH_OPEN;
    p->rightHandType = PLAYER_MODELTYPE_RH_OPEN;
}
void Player_SetUpperActionFunc(Player* p, UpperActionFunc action) { p->upperActionFunc = action; p->upperAnimInterpWeight = 0; }
void Player_DestroyHookshot(Player*) {}
void Player_DetachHeldActor(PlayState*, Player* p) {
    if (p->heldActor) p->heldActor->parent = nullptr;
    p->heldActor = p->interactRangeActor = p->actor.child = nullptr;
    p->stateFlags1 &= ~PLAYER_STATE1_CARRYING_ACTOR;
}
/* PRODUCTION_ITEM_INIT */
/* PRODUCTION_BUBBLE_CLEANUP */
/* PRODUCTION_CAMERA_EXIT */
s16 Camera_GetCamDirYaw(Camera *cam) { return cam->camDir.y; }
s16 Play_CreateSubCamera(PlayState *p) {
  ++cameraCreates;
  if (!cameraEnabled)
    return SUBCAM_NONE;
  p->cameraPtrs[1] = &p->subCameras[0];
  p->cameraPtrs[1]->setting = CAM_SET_FREE0;
  return 1;
}
s16 Play_ChangeCameraStatus(PlayState* p, s16 id, s16 status) {
    if (status == CAM_STAT_ACTIVE) p->activeCamera = id;
    return 1;
}
void Play_ClearCamera(PlayState *p, s16 id) {
  ++cameraClears;
  p->cameraPtrs[id] = nullptr;
}
s32 Play_CameraSetAtEye(PlayState *p, s16 id, Vec3f *at, Vec3f *eye) {
  assert(id >= 0 && id < NUM_CAMS && p->cameraPtrs[id]);
  p->cameraPtrs[id]->at = *at;
  p->cameraPtrs[id]->eye = p->cameraPtrs[id]->eyeNext = *eye;
  return 1;
}
s32 Play_CameraSetFov(PlayState *p, s16 id, f32 fov) {
  assert(id >= 0 && id < NUM_CAMS && p->cameraPtrs[id]);
  p->cameraPtrs[id]->fov = fov;
  return 1;
}
s32 Play_SetCameraRoll(PlayState *p, s16 id, s16 roll) {
  p->cameraPtrs[id]->roll = roll;
  return 1;
}
s32 MmSfx_IsAvailable() { return 1; }
s32 MmSfx_PlayAtPos(u16 id, Vec3f*) { sounds.emplace_back(id, frame); return 1; }
void MmSfx_PlayTransformFlash() { MmSfx_PlayAtPos(MM_NA_SE_SY_TRANSFORM_MASK_FLASH, nullptr); }
void MmSfx_Stop(u16 id) { stopped.push_back(id); }
Gfx* MmAssets_LoadDisplayListGraphStrict(const char*) { static Gfx dummy[1]; return maskFailure ? nullptr : dummy; }
LightNode* LightContext_InsertLight(PlayState*, LightContext*, LightInfo*) { ++lightCreates; return &nativeLight; }
void LightContext_RemoveLight(PlayState*, LightContext*, LightNode*) { ++lightClears; }
void Lights_PointNoGlowSetInfo(LightInfo*, s16, s16, s16, u8, u8, u8, s16) {}
f32 Math_SinS(s16 value) { return std::sin(value * (3.14159265358979323846f / 32768.0f)); }
f32 Math_CosS(s16 value) { return std::cos(value * (3.14159265358979323846f / 32768.0f)); }
f32 Rand_ZeroOne() { return cameraRandom; }
VecSph *OLib_Vec3fDiffToVecSphGeo(VecSph *result, Vec3f *at, Vec3f *eye) {
  const f32 x = eye->x - at->x, y = eye->y - at->y, z = eye->z - at->z;
  const f32 xz = std::sqrt(x * x + z * z);
  result->r = std::sqrt(xz * xz + y * y);
  result->pitch =
      (s16)(std::atan2(y, xz) * (32768.0f / 3.14159265358979323846f));
  result->yaw = (s16)(std::atan2(x, z) * (32768.0f / 3.14159265358979323846f));
  return result;
}
Vec3f *OLib_VecSphGeoToVec3f(Vec3f *result, VecSph *offset) {
  result->x = offset->r * Math_CosS(offset->pitch) * Math_SinS(offset->yaw);
  result->y = offset->r * Math_SinS(offset->pitch);
  result->z = offset->r * Math_CosS(offset->pitch) * Math_CosS(offset->yaw);
  return result;
}
/* PRODUCTION_CAMERA_MATH */
s32 Math_StepToF(f32* value, f32 target, f32 step) {
    *value += std::clamp(target - *value, -step, step);
    return *value == target;
}
s32 gBlurFrameBuffer = 1;
u32 OTRGetGameRenderWidth() { return renderWidth; }
u32 OTRGetGameRenderHeight() { return renderHeight; }
void FB_CreateFramebuffers() {}
void FB_CopyToFramebuffer(Gfx** gfx, s32, s32, u8 once, u8* copied) {
    assert(once); ++blurCopies; *copied = 1; ++*gfx;
}
void FB_DrawFromFramebuffer(Gfx** gfx, s32, u8 alpha) { assert(alpha == 100); ++blurDraws; ++*gfx; }
void Graph_OpenDisps(Gfx**, GraphicsContext*, const char*, s32) {}
void Graph_CloseDisps(Gfx**, GraphicsContext*, const char*, s32) {}
Gfx* Graph_GfxPlusOne(Gfx* gfx) { return gfx + 1; }
Gfx* Graph_BranchDlist(Gfx* gfx, Gfx*) { return gfx; }
void FrameInterpolation_RecordOpenChild(const void*, int) {}
void FrameInterpolation_RecordCloseChild() {}
void gSPDisplayList(Gfx* packet, Gfx* target) { packet->words.w0 = 0xDE000000; packet->words.w1 = (uintptr_t)target; }
}

/* DONOR_CAMERA_REFERENCE */

static u8 MmForm_LoadFormSkeleton(PlayState* p, MmPlayerTransformation form) {
    ++loadCount;
    if (skeletonFailure) return 0;
    gFormState.skeletonLoaded = 1;
    const auto off = form == MM_PLAYER_FORM_GORON ? MM_ANIM_PG_MASKOFFSTART :
                     form == MM_PLAYER_FORM_DEKU ? MM_ANIM_PN_MASKOFFSTART : MM_ANIM_PZ_MASKOFFSTART;
    gFormState.maskOffStart = MmAnim_Load(off);
    gFormState.idleAnim = &animations[MM_ANIM_LINK_NORMAL_WAIT_FREE];
    LinkAnimation_PlayLoop(p, &gFormState.formSkelAnime, gFormState.idleAnim);
    return 1;
}

/* PRODUCTION_NATIVE_SEQUENCE */
/* PRODUCTION_CONTROLLERS */
static void NewSceneEntry(PlayState* play, Player* player) {
    /* PRODUCTION_INIT_ENTRY */
}
static void DeathEntry() {
    /* PRODUCTION_DEATH_ENTRY */
}

static void Begin(MmPlayerTransformation form, bool removing = false) {
    player = {};
    play = {};
    gFormState = {};
    gSaveContext = {};
    gSaveContext.linkAge = std::getenv("NATIVE_AGE") ? std::atoi(std::getenv("NATIVE_AGE")) : 1;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
    play.cameraPtrs[0] = &camera;
    camera = {};
    camera.at = {0, 35, 0};
    camera.eye = {0, 55, 140};
    camera.fov = 65;
    camera.camDir.y = (s16)0x8000;
    player.actor.focus.pos.y = LINK_IS_ADULT ? 68.0f : 44.0f;
    gFormState.currentForm = removing ? form : MM_PLAYER_FORM_HUMAN;
    gFormState.targetForm = removing ? MM_PLAYER_FORM_HUMAN : form;
    gFormState.state = removing ? MMFORM_STATE_DETRANSFORMING : MMFORM_STATE_TRANSFORMING;
    gFormState.skeletonLoaded = removing;
    if (removing) MmForm_LoadFormSkeleton(&play, form);
    loadCount = restoreCount = restrictCount = equipCount = 0;
    sounds.clear(); stopped.clear(); lastStarted = nullptr;
    for (auto& anim : animations) anim.common.frameCount = 15;
    animations[MM_ANIM_CL_SETMASK].common.frameCount = 66;
    animations[MM_ANIM_CL_SETMASKEND].common.frameCount = 3;
    animations[MM_ANIM_CL_MASKOFF].common.frameCount = 10;
    animations[MM_ANIM_LINK_NORMAL_WAIT_FREE].common.frameCount = 72;
    frame = 0;
}
static void Tick(bool removing = false) {
    ++frame;
    ++play.gameplayFrames;
    // OoT clears this after its action, then MmForm must claim the next tick.
    player.stateFlags3 &= ~PLAYER_STATE3_PAUSE_ACTION_FUNC;
    if (removing) MmForm_UpdateDetransforming(&player, &play);
    else MmForm_UpdateTransforming(&player, &play);
}
static int Count(u16 id) {
    return std::count_if(sounds.begin(), sounds.end(), [id](auto s) { return s.first == id; });
}

static void AssertCamera(const MmCameraReference::Camera &expected) {
  const Camera *actual = play.cameraPtrs[play.activeCamera];
  auto close = [](f32 actual, f32 expected) {
    if (std::fabs(actual - expected) >= 0.025f)
      std::fprintf(stderr,
                   "camera mismatch at frame %d: actual=%f expected=%f\n",
                   frame, actual, expected);
    assert(std::fabs(actual - expected) < 0.025f);
  };
  close(actual->fov, expected.fov);
  close(actual->at.x, expected.at.x);
  close(actual->at.y, expected.at.y);
  close(actual->at.z, expected.at.z);
  close(actual->eye.x, expected.eye.x);
  close(actual->eye.y, expected.eye.y);
  close(actual->eye.z, expected.eye.z);
  assert(actual->roll == expected.roll);
}

static void CameraDonorCase(MmPlayerTransformation form, bool removing,
                            bool skip) {
  for (s16 yaw : {0, 0x4000, -0x7000})
    for (f32 sign : {0.25f, 0.75f}) {
      cameraEnabled = true;
      cameraRandom = sign;
      Begin(form, removing);
      player.actor.world.pos = {129, 17, -83};
      player.actor.focus.pos = player.actor.world.pos;
      player.actor.focus.pos.y += removing ? sFormProps[form].cameraHeight
                                  : LINK_IS_ADULT ? 68.0f
                                                  : 44.0f;
      camera.at = player.actor.world.pos;
      camera.at.y += 30;
      camera.eye = camera.at;
      camera.eye.x += Math_SinS(yaw) * 140;
      camera.eye.z += Math_CosS(yaw) * 140;
      camera.eye.y += 20;
      camera.camDir.y = yaw + (s16)0x8000;
      const Camera original = camera;
      MmCameraReference::Camera expected{};
      expected.at = camera.at;
      expected.eye = camera.eye;
      expected.fov = camera.fov;
      expected.focalActor = &player.actor;
      // MM follows the face from the previous rendered pose. Feed both real
      // controllers the same crouch/struggle motion, then the revealed form.
      for (int tick = 0; sNativeMaskCs.active || tick == 0; ++tick) {
        assert(tick < 160);
        const bool revealing = gFormState.cutscenePhase == 2;
        if (revealing)
          player.actor.focus.pos.y = player.actor.world.pos.y +
                                     (removing ? LINK_IS_ADULT ? 68.0f : 44.0f
                                               : sFormProps[form].cameraHeight);
        else
          player.actor.focus.pos.y -= std::sin(tick * 0.13f) * 0.3f;
        expected.focalActorPosRot = player.actor.world;
        expected.focalActorPosRot.rot.y = yaw;
        if (skip && tick == 10)
          expected.animState = 999;
        if (revealing)
          expected.animState = removing ? 3 : 4;
        play.state.input[0].press.button = skip && tick == 9 ? BTN_A : 0;
        if (removing)
          MmCameraReference::Camera_Demo5(&expected);
        else
          MmCameraReference::Camera_Demo4(&expected);
        Tick(removing);
        if (sNativeMaskCs.active)
          AssertCamera(expected);
      }
      assert(play.activeCamera == 0 && play.cameraPtrs[1] == nullptr);
      assert(std::memcmp(&original, &camera, sizeof(camera)) == 0 &&
             "cutscene must preserve the prior gameplay camera");
    }
}

int main(int argc, char** argv) {
    assert(argc == 2);
    const std::string test = argv[1];
    const char* formEnv = std::getenv("NATIVE_FORM");
    const auto form = (MmPlayerTransformation)std::stoi(formEnv ? formEnv : "3");
  if (test == "camera-donor-on" || test == "camera-donor-off") {
    CameraDonorCase(form, test == "camera-donor-off", false);
  } else if (test == "camera-skip") {
    CameraDonorCase(form, false, true);
    CameraDonorCase(form, true, true);
  } else if (test == "camera-facing") {
    cameraEnabled = true;
    Begin(form);
    Tick();
    const s16 facing = player.actor.shape.rot.y;
    player.actor.shape.rot.y += 0x4000;
    player.yaw += 0x4000;
    Tick();
    assert(player.actor.shape.rot.y == facing && player.yaw == facing &&
           "the native shot owns facing until release");
    MmForm_OnPlayDestroy(&play, &player);
  } else if (test == "mask-on") {
        Begin(form); Tick();
        assert(putAwayCount == 1 && player.heldItemAction == PLAYER_IA_NONE);
        assert(lastStarted == &animations[MM_ANIM_CL_SETMASK] && "native mask-on must animate Link");
        assert(player.stateFlags3 & PLAYER_STATE3_PAUSE_ACTION_FUNC);
        while (frame < 83) { Tick(); assert(!gFormState.skeletonLoaded); }
        assert(player.skelAnime.animation == &animations[MM_ANIM_CL_SETMASKEND]);
        while (gFormState.state != MMFORM_STATE_ACTIVE && frame < 150) Tick();
        assert(gFormState.state == MMFORM_STATE_ACTIVE && loadCount == 1 && restrictCount == 1);
        assert(!(player.stateFlags1 & PLAYER_STATE1_INPUT_DISABLED));
        assert(!(player.stateFlags3 & PLAYER_STATE3_PAUSE_ACTION_FUNC));
    } else if (test == "drawn-item") {
        Begin(form);
        player.heldItemId = ITEM_SWORD_KOKIRI;
        player.heldItemAction = player.itemAction = PLAYER_IA_SWORD_KOKIRI;
        player.leftHandType = PLAYER_MODELTYPE_LH_SWORD;
        player.rightHandType = PLAYER_MODELTYPE_RH_SHIELD;
        player.stateFlags1 |= PLAYER_STATE1_START_CHANGING_HELD_ITEM | PLAYER_STATE1_SHIELDING;
        player.meleeWeaponState = 1;
        player.currentMask = gSaveContext.ship.maskMemory = PLAYER_MASK_BUNNY;
        Tick();
        assert(player.heldItemId == ITEM_NONE && player.heldItemAction == PLAYER_IA_NONE && player.itemAction == PLAYER_IA_NONE);
        assert(player.leftHandType == PLAYER_MODELTYPE_LH_OPEN && player.rightHandType == PLAYER_MODELTYPE_RH_OPEN);
        assert(player.upperActionFunc == func_8083485C && putAwayCount == 1);
        assert(!(player.stateFlags1 & (PLAYER_STATE1_START_CHANGING_HELD_ITEM | PLAYER_STATE1_SHIELDING)));
        assert(player.meleeWeaponState == 0);
        assert(player.currentMask == PLAYER_MASK_NONE && gSaveContext.ship.maskMemory == PLAYER_MASK_NONE);
        MmForm_OnPlayDestroy(&play, &player);
    } else if (test == "bubble-aim") {
        Begin(MM_PLAYER_FORM_DEKU, true);
        sDekuBubbleActive = true;
        player.heldItemAction = player.itemAction = PLAYER_IA_SLINGSHOT;
        player.unk_6AD = 2;
        player.stateFlags1 |= PLAYER_STATE1_FIRST_PERSON | PLAYER_STATE1_ITEM_IN_HAND | PLAYER_STATE1_READY_TO_FIRE;
        gFormState.bubbleCameraState.firstPersonActive = 1;
        gFormState.bubbleCharging = 1;
        gFormState.bubbleCharge = 12;
        gFormState.bubbleChargeTimer = 50;
        gFormState.dekuCheekScale = 1.3f;
        Tick(true);
        assert(!sDekuBubbleActive && !gFormState.bubbleCharging && gFormState.bubbleCharge == 0);
        assert(gFormState.bubbleChargeTimer == 0 && gFormState.dekuCheekScale == 1);
        assert(!gFormState.bubbleCameraState.firstPersonActive && player.unk_6AD == 0);
        assert(!(player.stateFlags1 & (PLAYER_STATE1_FIRST_PERSON | PLAYER_STATE1_ITEM_IN_HAND | PLAYER_STATE1_READY_TO_FIRE)));
        assert(player.heldItemAction == PLAYER_IA_NONE);
        assert(std::find(stopped.begin(), stopped.end(), MM_NA_SE_PL_DEKUNUTS_BUBLE_BREATH) != stopped.end());
        while (gFormState.state != MMFORM_STATE_INACTIVE && frame < 140) Tick(true);
        assert(gFormState.state == MMFORM_STATE_INACTIVE);
    } else if (test == "mask-off") {
        Begin(form, true); Tick(true);
        assert(gFormState.formSkelAnime.animation == gFormState.maskOffStart && "native form removal clip must start");
        while (frame < 55) { Tick(true); assert(restoreCount == 0); }
        while (gFormState.state != MMFORM_STATE_INACTIVE && frame < 140) Tick(true);
        assert(gFormState.state == MMFORM_STATE_INACTIVE && restoreCount == 1 && equipCount == 1);
        assert(!(player.stateFlags3 & PLAYER_STATE3_PAUSE_ACTION_FUNC));
    } else if (test == "audio") {
        Begin(form);
        while (gFormState.state != MMFORM_STATE_ACTIVE && frame < 160) Tick();
        for (u16 id : {MM_NA_SE_PL_PUT_OUT_ITEM, MM_NA_SE_IT_SET_TRANSFORM_MASK, MM_NA_SE_PL_FREEZE_S,
                       MM_NA_SE_IT_TRANSFORM_MASK_BROKEN, MM_NA_SE_PL_TRANSFORM_VOICE,
                       MM_NA_SE_EV_LIGHTNING_HARD, MM_NA_SE_SY_TRANSFORM_MASK_FLASH}) assert(Count(id) == 1);
        for (auto s : sounds) {
            if (s.first == MM_NA_SE_SY_TRANSFORM_MASK_FLASH) assert(s.second >= 84);
            if (s.first == MM_NA_SE_EV_LIGHTNING_HARD) assert(s.second == 59);
        }
    } else if (test == "skip") {
        Begin(form); Tick();
        while (frame < 5) Tick();
        play.state.input[0].press.button = BTN_A; Tick();
        play.state.input[0].press.button = 0;
        while (gFormState.state != MMFORM_STATE_ACTIVE && frame < 50) Tick();
        assert(gFormState.state == MMFORM_STATE_ACTIVE && frame < 50);
        assert(Count(MM_NA_SE_PL_TRANSFORM_VOICE) == 0);
        assert(std::find(stopped.begin(), stopped.end(), MM_NA_SE_PL_TRANSFORM_VOICE) != stopped.end());
    } else if (test == "instant") {
        instant = 1; Begin(form);
        for (int i = 0; i < 5; ++i) Tick();
        assert(gFormState.state == MMFORM_STATE_ACTIVE && loadCount == 1);
        assert(Count(MM_NA_SE_SY_TRANSFORM_MASK_FLASH) == 1);
        assert(Count(MM_NA_SE_PL_TRANSFORM_VOICE) == 0);
    } else if (test == "failure") {
        skeletonFailure = true; Begin(form);
        for (int i = 0; i < 110 && gFormState.state != MMFORM_STATE_INACTIVE; ++i) Tick();
        assert(gFormState.state == MMFORM_STATE_INACTIVE);
        assert(!(player.stateFlags1 & PLAYER_STATE1_INPUT_DISABLED));
        assert(!(player.stateFlags3 & PLAYER_STATE3_PAUSE_ACTION_FUNC));
    } else if (test == "toggle") {
        Begin(form); Tick(); instant = 1;
        for (int i = 0; i < 10; ++i) Tick();
        assert(gFormState.state == MMFORM_STATE_TRANSFORMING && "mid-animation setting change cannot strand ownership");
        while (gFormState.state != MMFORM_STATE_ACTIVE && frame < 150) Tick();
        assert(gFormState.state == MMFORM_STATE_ACTIVE);
        assert(!(player.stateFlags3 & PLAYER_STATE3_PAUSE_ACTION_FUNC));
    } else if (test.rfind("missing-", 0) == 0) {
        const bool removing = test.find("-off") != std::string::npos;
        missing = test.find("animation") != std::string::npos ? MM_ANIM_CL_MASKOFF : -1;
        maskFailure = test.find("mask") != std::string::npos;
        Begin(form, removing);
        for (int i = 0; i < 5; ++i) Tick(removing);
        assert(gFormState.state == (removing ? MMFORM_STATE_INACTIVE : MMFORM_STATE_ACTIVE));
        assert(removing ? restoreCount == 1 : loadCount == 1);
        assert(Count(MM_NA_SE_PL_TRANSFORM_VOICE) == 0);
        assert(!sNativeMaskCs.active);
    } else if (test == "scene-lifecycle") {
        cameraEnabled = true; Begin(form); Tick();
        assert(sNativeMaskCs.active);
        PlayState nextScene{};
        Player nextPlayer{};
        nextScene.envCtx.adjFogNear = 37;
        nextScene.activeCamera = 0;
        NewSceneEntry(&nextScene, &nextPlayer);
        assert(!sNativeMaskCs.active && "scene entry must abandon stale light/camera handles");
        assert(nextScene.envCtx.adjFogNear == 37 && nextScene.activeCamera == 0);
        assert(cameraClears == 0 && lightClears == 0 && "destroyed scene handles cannot be freed through a new scene");
        assert(gFormState.state == MMFORM_STATE_INACTIVE && gFormState.currentForm == MM_PLAYER_FORM_HUMAN);
        assert(gFormState.cutsceneTimer == 0 && gFormState.flashAlpha == 0);
    } else if (test == "scene-address-reuse") {
        cameraEnabled = true; Begin(form); Tick();
        play.envCtx.adjFogNear = 37;
        play.activeCamera = 0;
        NewSceneEntry(&play, &player);
        assert(!sNativeMaskCs.active);
        assert(play.envCtx.adjFogNear == 37 && play.activeCamera == 0);
        assert(cameraClears == 0 && lightClears == 0);
    } else if (test == "destroy-after-reveal") {
        cameraEnabled = true; Begin(form);
        while (gFormState.cutscenePhase != 2 && frame < 130) Tick();
        assert(gFormState.currentForm == form);
        MmForm_OnPlayDestroy(&play, &player);
        assert(!sNativeMaskCs.active && gFormState.state == MMFORM_STATE_ACTIVE);
        assert(gFormState.cutsceneTimer == 0 && gFormState.flashAlpha == 0);
        assert(restrictCount == 1 && cameraClears == 1 && lightClears == 1);
    } else if (test == "camera-lighting") {
        cameraEnabled = true; Begin(form);
        play.envCtx.adjFogNear = 71;
        play.envCtx.adjAmbientColor[0] = 15;
        while (gFormState.state != MMFORM_STATE_ACTIVE && frame < 150) Tick();
        assert(gFormState.state == MMFORM_STATE_ACTIVE);
        assert(cameraCreates == 1 && cameraClears == 1 && lightCreates == 1 && lightClears == 1);
        assert(play.activeCamera == 0 && play.envCtx.adjFogNear == 71 && play.envCtx.adjAmbientColor[0] == 15);
    } else if (test == "effects") {
        Begin(form);
        GraphicsContext graphics{};
        Gfx opaque[1024]{}, overlay[512]{};
        graphics.polyOpa.p = opaque;
        graphics.overlay.p = overlay;
        play.state.gfxCtx = &graphics;
        while (frame < 8) Tick();
        assert(MmForm_NativeMaskAttachment(&player, PLAYER_LIMB_L_HAND).dl);
        assert(!MmForm_NativeMaskAttachment(&player, PLAYER_LIMB_HEAD).dl);
        while (frame < 12) Tick();
        assert(MmForm_NativeMaskAttachment(&player, PLAYER_LIMB_HEAD).dl);
        assert(!MmForm_NativeMaskAttachment(&player, PLAYER_LIMB_L_HAND).dl);
        while (frame < 19) Tick();
        MmForm_DrawNativeTransformationPostProcess(&play); assert(blurCopies == 0);
        Tick(); MmForm_DrawNativeTransformationPostProcess(&play);
        assert(blurCopies == 1 && blurDraws == 0);
        Tick(); MmForm_DrawNativeTransformationPostProcess(&play);
        assert(blurCopies == 2 && blurDraws == 1);
        renderWidth = 640;
        Tick(); MmForm_DrawNativeTransformationPostProcess(&play);
        assert(blurCopies == 3 && blurDraws == 1 && "resize must recapture without blending stale pixels");
        while (frame < 58) Tick();
        assert(sNativeMaskCs.faceAlpha == 50);
        while (frame < 63) Tick();
        assert(sNativeMaskCs.faceAlpha == 255);
        while (gFormState.cutscenePhase != 2 && frame < 140) Tick();
        assert(sNativeMaskCs.faceAlpha == 0);
        assert(MmForm_NativeMaskAttachment(&player, PLAYER_LIMB_L_HAND).dl);
        MmForm_DrawNativeTransformationPostProcess(&play); assert(blurCopies == 3);
    } else if (test == "audio-off") {
        Begin(form, true);
        while (gFormState.state != MMFORM_STATE_INACTIVE && frame < 140) Tick(true);
        assert(Count(MM_NA_SE_IT_SET_TRANSFORM_MASK) == 1 && Count(MM_NA_SE_PL_FACE_CHANGE) == 1);
        assert(Count(MM_NA_SE_SY_TRANSFORM_MASK_FLASH) == 1 && Count(MM_NA_SE_PL_TRANSFORM_VOICE) == 0);
        for (auto cue : sounds) {
            if (cue.first == MM_NA_SE_IT_SET_TRANSFORM_MASK) assert(cue.second == 8);
            if (cue.first == MM_NA_SE_PL_FACE_CHANGE) assert(cue.second == 15);
        }
    } else if (test == "death-after-reveal") {
        cameraEnabled = true; Begin(form);
        while (gFormState.cutscenePhase != 2 && frame < 130) Tick();
        assert(gFormState.state == MMFORM_STATE_TRANSFORMING);
        DeathEntry();
        assert(!sNativeMaskCs.active && gFormState.state == MMFORM_STATE_ACTIVE);
        assert(cameraClears == 1 && lightClears == 1);
        assert(!(player.stateFlags1 & PLAYER_STATE1_INPUT_DISABLED));
        assert(!(player.stateFlags3 & PLAYER_STATE3_PAUSE_ACTION_FUNC));
    } else return 2;
    std::printf("PASS %s form=%d age=%d\n", argv[1], (int)form, gSaveContext.linkAge);
}
