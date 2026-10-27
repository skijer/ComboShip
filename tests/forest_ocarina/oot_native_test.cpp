#include "ComboGameMode.h"
#include "entrances/SoloForest.h"
#include "global.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <libultraship/bridge/consolevariablebridge.h>

SaveContext gSaveContext{};
static GameInfo registers{};
GameInfo *gGameInfo = &registers;
PlayState *gPlayState;
extern "C" {
int gComboGameMode = COMBO_GAME_BOTH;
}
static PlayState play;
static Player player;
static Actor target;
static Camera camera;
static MeshHeader mesh;
static CollisionHeader collision;
static const std::array<int, 3> forestRoomTypes{{/* FOREST ROOM TYPES */}};
static CamData forestCameraRecords[] = {/* FOREST CAMERA RECORDS */};
static s32 sCameraInterfaceFlags, sUpdateCameraDirection;
s32 gUseAtanContFrac;
f32 qNaN0x10000 = NAN;
static bool animationFinished;
static LinkAnimationHeader gPlayerAnim_link_normal_okarina_start{},
    gPlayerAnim_link_normal_okarina_swing{},
    gPlayerAnim_link_normal_okarina_end{};
extern "C" int32_t CVarGetInteger(const char *, int32_t fallback) {
  return fallback;
}
extern "C" f32 TransformMasks_GetFormHeight() { return 0; }
extern "C" void Sfx_PlaySfxCentered(u16) {}
extern "C" void Interface_ChangeHudVisibilityMode(u16) {}
extern "C" s32 Camera_CopyPREGToModeValues(Camera *) {
  assert(false);
  return 0;
}
extern "C" s32 MinishTiny_IsActive() { return 0; }
extern "C" s32 SetCameraManual(Camera *) {
  assert(false);
  return 0;
}
extern "C" s32 Camera_ScriptedFreeCamActive(Camera *) {
  assert(false);
  return 0;
}
extern "C" s32 Camera_FreeMove(Camera *, f32) {
  assert(false);
  return 0;
}
extern "C" s32 Camera_BGCheckInfo(Camera *, Vec3f *, CamColChk *) { return 0; }
extern "C" f32 BgCheck_CameraRaycastFloor2(CollisionContext *, CollisionPoly **,
                                           s32 *, Vec3f *) {
  return BGCHECK_Y_MIN;
}
extern "C" s32 CollisionCheck_LineOCCheck(PlayState *, CollisionCheckContext *,
                                          Vec3f *, Vec3f *, Actor **, s32) {
  return false;
}
static void Player_SetupActionPreserveItemAction(PlayState *, Player *self,
                                                 PlayerActionFunc action, s32) {
  self->actionFunc = action;
}
static void Player_AnimPlayOnceAdjusted(PlayState *, Player *,
                                        LinkAnimationHeader *) {}
static void Player_AnimPlayLoopAdjusted(PlayState *, Player *,
                                        LinkAnimationHeader *) {}
static void func_8083A098(Player *self, LinkAnimationHeader *, PlayState *) {
  self->actionFunc = nullptr;
}
static s32 Player_ActionHandler_13(Player *, PlayState *) { return false; }
extern "C" s32 LinkAnimation_Update(PlayState *, SkelAnime *) {
  return animationFinished;
}
extern "C" void Player_StartTalking(PlayState *, Actor *) { assert(false); }
extern "C" void func_8010BD58(PlayState *, u16) {}
extern "C" s32 Player_TryCsAction(PlayState *, Actor *, s32) {
  assert(false);
  return false;
}
extern "C" Actor *Actor_Spawn(ActorContext *, PlayState *, s16, f32, f32, f32,
                              s16, s16, s16, s16) {
  assert(false);
  return nullptr;
}
extern "C" void Environment_WarpSongLeave(PlayState *) { assert(false); }
#define osSyncPrintf(...) ((void)0)
#define PCT(x) ((x)*.01f)
#define NEXTSETTING ((values++)->val)
#define NEXTPCT PCT(NEXTSETTING)
#define RELOAD_PARAMS                                                          \
  (camera->animState == 0 || camera->animState == 10 ||                        \
   camera->animState == 20 || R_RELOAD_CAM_PARAMS)
/* NATIVE CODE */

static void Require(bool value, const char *message) {
  if (!value) {
    std::fprintf(stderr, "FAIL: %s\n", message);
    std::exit(1);
  }
}
static void Reset(int mode, int age, s16 scene, int roomType, int bgCamIndex) {
  play = {};
  player = {};
  camera = {};
  target = {};
  mesh = {};
  collision = {};
  gSaveContext = {};
  registers = {};
  gPlayState = &play;
  gComboGameMode = mode;
  gSaveContext.linkAge = age;
  gSaveContext.gameMode = GAMEMODE_NORMAL;
  gSaveContext.fileNum = 0;
  gSaveContext.entranceIndex = scene == COMBO_SOLO_FOREST_OOT_SCENE
                                   ? COMBO_SOLO_FOREST_OOT_ENTRANCE
                                   : ENTR_LOST_WOODS_SOUTH_EXIT;
  play.sceneNum = scene;
  play.state.running = true;
  play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
  mesh.base.type = roomType;
  play.roomCtx.curRoom.meshHeader = &mesh;
  collision.cameraDataList = forestCameraRecords;
  play.colCtx.colHeader = &collision;
  play.cameraPtrs[CAM_ID_MAIN] = &camera;
  camera.play = &play;
  camera.player = &player;
  camera.thisIdx = CAM_ID_MAIN;
  camera.status = CAM_STAT_ACTIVE;
  camera.mode = CAM_MODE_NORMAL;
  camera.setting = CAM_SET_FREE0;
  camera.prevSetting = CAM_SET_NORMAL0;
  camera.camDataIdx = camera.prevCamDataIdx = -1;
  camera.eye = camera.eyeNext = {50, 100, 260};
  camera.at = {0, 56, 0};
  camera.dist = 268;
  camera.fov = 60;
  camera.fovUpdateRate = .05f;
  camera.playerPosRot.rot.y = player.actor.shape.rot.y = 12288;
  target.shape.rot.y = -8192;
  for (int i = 0; i < ARRAY_COUNT(sOREGInit); ++i)
    OREG(i) = sOREGInit[i];
  OREG(27) = 20;
  if (bgCamIndex >= 0) {
    Require(Camera_ChangeDataIdx(&camera, bgCamIndex) >= 0 ||
                camera.setting == CAM_SET_NORMAL0,
            "prepared forest background camera resolves through native "
            "collision metadata");
  } else {
    Camera_RequestSetting(&camera, CAM_SET_NORMAL0);
  }
  camera.unk_14A = 0;
  animationFinished = false;
}
struct OotOcarinaView {
  Vec3f eye, at;
  f32 fov;
};
static OotOcarinaView PlayAndCancel(bool interaction) {
  player.unk_6A8 = interaction ? &target : nullptr;
  NativePullOutOcarina(&player, &play);
  Require(camera.setting == CAM_SET_TURN_AROUND &&
              camera.data2 == (interaction ? 91 : 90) &&
              player.actionFunc == Player_Action_8084E3C4 &&
              (player.stateFlags2 & PLAYER_STATE2_OCARINA_PLAYING),
          "native instrument branch selects TURN_AROUND and the correct "
          "ocarina variant");
  Require(camera.target == (interaction ? &target : nullptr),
          "native interaction camera receives its actor target");
  animationFinished = true;
  Player_Action_8084E3C4(&player, &play);
  animationFinished = false;
  Require(player.av2.actionVar2 == 1,
          "native playing action advances after pull-out animation");
  for (int frame = 0; frame < 90; ++frame) {
    NativeOcarinaCameraFrame(&camera);
    Require(camera.setting == CAM_SET_TURN_AROUND,
            "native playing view persists until cancellation");
  }
  Require(std::isfinite(camera.eye.x) && std::isfinite(camera.eye.y) &&
              std::isfinite(camera.eye.z) && camera.eye.z != 260 &&
              camera.fov == (interaction ? 55 : 45),
          "native KeepOn4 produces the ordinary playing/interaction view");
  OotOcarinaView result{camera.eye, camera.at, camera.fov};
  play.msgCtx.ocarinaMode = OCARINA_MODE_04;
  Player_Action_8084E3C4(&player, &play);
  Require(camera.unk_14C & 8,
          "native player cancel action requests return-camera cleanup");
  NativeOcarinaCameraFrame(&camera);
  Require(camera.setting == CAM_SET_NORMAL0 &&
              play.activeCamera == CAM_ID_MAIN &&
              camera.status == CAM_STAT_ACTIVE,
          "native cancellation restores the background/follow camera");
  return result;
}
static void SameView(const OotOcarinaView &view,
                     const OotOcarinaView &control) {
  Require(!std::memcmp(&view.eye, &control.eye, sizeof(Vec3f)) &&
              !std::memcmp(&view.at, &control.at, sizeof(Vec3f)) &&
              view.fov == control.fov,
          "OoT solo/combined forest camera matches the native outdoor control");
}
int main() {
  InitNativeCameraTable();
  for (int age : {LINK_AGE_CHILD, LINK_AGE_ADULT}) {
    for (bool interaction : {false, true}) {
      Reset(COMBO_GAME_BOTH, age, SCENE_LOST_WOODS, 0, -1);
      const OotOcarinaView control = PlayAndCancel(interaction);
      for (int mode : {COMBO_GAME_OOT, COMBO_GAME_BOTH}) {
        for (int visit = 0; visit < 3; ++visit) {
          for (int room = 0; room < 3; ++room) {
            Reset(mode, age,
                  mode == COMBO_GAME_OOT ? COMBO_SOLO_FOREST_OOT_SCENE
                                         : SCENE_LOST_WOODS,
                  mode == COMBO_GAME_OOT ? forestRoomTypes[room] : 0,
                  mode == COMBO_GAME_OOT ? room : -1);
            Require(
                Play_CamIsNotFixed(&play),
                "prepared playable forest rooms permit native item cameras");
            SameView(PlayAndCancel(interaction), control);
          }
        }
      }
    }
  }
  Reset(COMBO_GAME_OOT, LINK_AGE_CHILD, COMBO_SOLO_FOREST_OOT_SCENE, 1, -1);
  NativePullOutOcarina(&player, &play);
  Require(camera.setting == CAM_SET_NORMAL0,
          "native fixed-room camera rejection is preserved");
  Reset(COMBO_GAME_BOTH, LINK_AGE_CHILD, SCENE_CASTLE_COURTYARD_GUARDS_DAY, 2,
        -1);
  NativePullOutOcarina(&player, &play);
  Require(camera.setting == CAM_SET_NORMAL0,
          "native courtyard camera restriction is preserved");
#ifdef COMBO_BUILD
  std::puts(
      "PASS OoT-only and combined native ocarina pull-out/KeepOn4/cancel: "
      "child/adult, free-play/actor interaction, three prepared rooms/re-entry "
      "and native fixed-camera controls; no OoT production change needed");
#else
  std::puts(
      "PASS standalone OoT native ocarina path and cancellation unchanged");
#endif
}
