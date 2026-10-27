#include "2s2h/Enhancements/Gameplay/ComboOpeningForest.h"
#include "entrances/BlackMarketEntrance.h"
#include "entrances/SoloForest.h"
#include "gameplay/MagicMirrorAnchor.h"
#include "gameplay/MagicMirrorPlayer.h"
#include "global.h"
#include "mods/nei_save.h"
#if __has_include("2s2h/Enhancements/Camera/ComboForestOcarina.h")
#include "2s2h/Enhancements/Camera/ComboForestOcarina.h"
#endif
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

SaveContext gSaveContext{};
uintptr_t gSegments[NUM_SEGMENTS]{};
PlayState *gPlayState;
GameState *gGameState;
extern "C" {
int gComboGameMode = COMBO_GAME_BOTH;
}
static PlayState play;
static Player player;
static int letterbox;
static PlayerAnimationHeader *D_8085D17C[PLAYER_FORM_MAX];
static PlayerAnimationHeader *D_8085D190[PLAYER_FORM_MAX];
static PlayerAnimationHeader gPlayerAnim_pg_gakkiwait;
static s16 sOcarinaButtonIndexBufPos, sOcarinaButtonIndexBufLen;
static s16 sOcarinaButtonAlphaValues[9];
static u8 sLastPlayedSong;
u8 gNeiCustomSongsAvailable;
extern "C" NeiSaveData *Nei_Save() {
  static NeiSaveData state{};
  return &state;
}
extern "C" s32 CVarGetInteger(const char *, s32 fallback) { return fallback; }
#define lusprintf(...) ((void)0)
extern "C" OcarinaStaff *AudioOcarina_GetRecordingStaff() {
  static OcarinaStaff staff{};
  return &staff;
}
extern "C" OcarinaStaff *AudioOcarina_GetPlaybackStaff() {
  return AudioOcarina_GetRecordingStaff();
}
extern "C" void AudioOcarina_SetInstrument(u8) {}
extern "C" void AudioOcarina_SetPlaybackSong(s8, u8) { assert(false); }
extern "C" Actor *Actor_Spawn(ActorContext *, PlayState *, s16, f32, f32, f32,
                              s16, s16, s16, s32) {
  assert(false);
  return nullptr;
}
void Message_OpenText(PlayState *, u16) {}
void func_80150A84(PlayState *) {}
void Message_ResetOcarinaButtonState(PlayState *) {}
void Message_GrowTextbox(PlayState *) {}
extern "C" void Font_LoadChar(PlayState *, u16, s32) {}
extern "C" void Interface_SetBButtonInterfaceDoAction(PlayState *state,
                                                      s16 action) {
  state->interfaceCtx.bButtonInterfaceDoAction = action;
  state->interfaceCtx.bButtonInterfaceDoActionActive = true;
}
extern "C" s32 PlayerAnimation_Update(PlayState *, SkelAnime *) {
  return false;
}
void Player_Anim_PlayOnceFreeze(PlayState *, Player *,
                                PlayerAnimationHeader *) {}
void Player_Anim_PlayLoopAdjusted(PlayState *, Player *,
                                  PlayerAnimationHeader *) {}
void Player_Anim_PlayOnceAdjustedReverse(PlayState *, Player *,
                                         PlayerAnimationHeader *) {}
void func_80851EAC(Player *) {}
void func_8085255C(PlayState *, Player *) {}
void Player_StartTalking(PlayState *, Actor *) { assert(false); }
s32 Player_ActionHandler_13(Player *, PlayState *) { return false; }
s32 func_8082DA90(PlayState *) {
  assert(false);
  return false;
}
void Player_SetAction_PreserveItemAction(PlayState *, Player *,
                                         PlayerActionFunc, s32) {
  assert(false);
}
void Player_Action_88(Player *, PlayState *) { assert(false); }
s32 Player_TryCsAction(PlayState *, Player *, PlayerCsAction) {
  assert(false);
  return false;
}
void NativeIdle(Player *, PlayState *) {}
void func_80836A5C(Player *current, PlayState *) {
  current->actionFunc = NativeIdle;
}
// The engine compiles C in a permissive C++ target; make its OTR string casts
// explicit here.
#undef BEN_ANIM_EQUAL
#define BEN_ANIM_EQUAL(a, b)                                                   \
  (((a) == nullptr) ? ((b) == nullptr)                                         \
                    : (((b) != nullptr) &&                                     \
                       std::strcmp(static_cast<const char *>(a),               \
                                   reinterpret_cast<const char *>(b)) == 0))
#ifdef COMBO_BUILD
namespace MirrorHost {
u32 Seed() { return gSaveContext.save.shipSaveInfo.rando.finalSeed; }
} // namespace MirrorHost
extern "C" void MagicMirrorPlayer_Arrive(PlayState *, Player *) {}
extern "C" f32 BgCheck_EntityRaycastFloor3(CollisionContext *context,
                                           CollisionPoly **poly, s32 *bg,
                                           Vec3f *point) {
  const auto *header = context->colHeader;
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
  *poly = nullptr;
  return BGCHECK_Y_MIN;
}
extern "C" s32 BgCheck_EntityLineTest1(CollisionContext *, Vec3f *, Vec3f *,
                                       Vec3f *, CollisionPoly **, s32, s32, s32,
                                       s32, s32 *) {
  return false;
}
#endif
namespace SOH {
struct Scene {
  ActorEntry *spawn;
  ActorCutscene *cutscenes = nullptr;
  s16 count = 0;
  bool room = false;
};
} // namespace SOH
extern "C" void Object_InitContext(GameState *, ObjectContext *) {}
extern "C" void LightContext_Init(PlayState *, LightContext *) {}
extern "C" void Scene_ResetTransitionActorList(GameState *,
                                               TransitionActorList *) {}
extern "C" void Room_Init(PlayState *, RoomContext *) {}
extern "C" void Play_InitEnvironment(PlayState *, s16) {}
s32 OTRScene_ExecuteCommands(PlayState *state, SOH::Scene *scene) {
  // The runner decodes all six prepared MM room resources and verifies that
  // none contains actor-cutscene command 0x1B. Their decode is a boundary here.
  if (scene->room)
    return 0;
  state->linkActorEntry = scene->spawn;
#ifdef COMBO_BUILD
  ComboOpeningForest_PrepareSpawn(state);
#endif
  if (scene->cutscenes)
    CutsceneManager_Init(state, scene->cutscenes, scene->count);
  return 0;
}
extern "C" void func_80123140(PlayState *, Player *) {}
extern "C" void Actor_SpawnTransitionActors(PlayState *, ActorContext *) {}
extern "C" void func_800FEAB0() {}
extern "C" u32 Environment_GetStormState(PlayState *) {
  return STORM_STATE_OFF;
}
extern "C" void Environment_StopStormNatureAmbience(PlayState *) {}
#define GameInteractor_ExecuteAfterRoomSceneCommands(...) ((void)0)
extern "C" Actor *Actor_SpawnAsChildAndCutscene(ActorContext *, PlayState *,
                                                s16 id, f32 x, f32 y, f32 z,
                                                s16 rx, s16 ry, s16 rz,
                                                s32 params, u32 csId, u32,
                                                Actor *) {
  Actor *actor = &player.actor;
  actor->id = id;
  actor->world.pos = actor->home.pos = {x, y, z};
  actor->shape.rot = actor->home.rot = {rx, ry, rz};
  actor->params = params;
  /* CS_ID_NORMALIZATION */
  return actor;
}
extern "C" void ShrinkWindow_Letterbox_SetSize(s32 value) { letterbox = value; }
extern "C" void ShrinkWindow_Letterbox_SetSizeTarget(s32 value) {
  letterbox = value;
}
extern "C" void Audio_PlaySfx(u16) {}
extern "C" s32 Player_SetCsActionWithHaltedActors(PlayState *, Actor *, u8) {
  return true;
}
extern "C" void Cutscene_StartScripted(PlayState *, u8) { assert(false); }
extern "C" void func_800E0348(Camera *) { assert(false); }
extern "C" u16 BgCheck_GetBgCamSettingImpl(CollisionContext *, u32, s32) {
  assert(false);
  return 0;
}
extern "C" f32 Math_FAtan2F(f32 y, f32 x) { return std::atan2(y, x); }
extern "C" f32 BgCheck_CameraRaycastFloor2(CollisionContext *, CollisionPoly **,
                                           s32 *, Vec3f *) {
  return BGCHECK_Y_MIN;
}
// The fixture supplies a clear collision volume to the native ocarina handler.
extern "C" s16 func_800CC260(Camera *, Vec3f *eye, Vec3f *at, VecGeo *offset,
                             Actor **, s16) {
  *eye = OLib_AddVecGeoToVec3f(at, offset);
  return 0;
}
extern "C" s32 func_800CBFA4(Camera *, Vec3f *, Vec3f *, s16) { return 0; }
#define GameInteractor_Should(...) true
#define GameInteractor_ExecuteOnCameraChangeSettingsFlags(...) ((void)0)
#define GameInteractor_ExecuteOnCameraChangeModeFlags(...) ((void)0)
#define __osMemset std::memset
#define RELOAD_PARAMS(camera)                                                  \
  ((camera)->animState == 0 || (camera)->animState == 10 ||                    \
   (camera)->animState == 20)
#define CAM_RODATA_UNSCALE(x) ((x)*.01f)
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
static bool sUpdateCameraDirection;
static Vec3f D_801EDDD0, D_801EDDE0, D_801EDDF0;
/* NATIVE CODE */

static void Require(bool condition, const char *message) {
  if (!condition) {
    std::fprintf(stderr, "FAIL: %s\n", message);
    std::exit(1);
  }
}
static ActorEntry combinedSpawn = /* COMBINED SPAWN */;
static ActorEntry soloSpawn = /* SOLO SPAWN */;
// Native Lost Woods command 0x1B, common unscripted player chain, entries 0..8.
// The immutable mm.o2r has this chain before its two prologue script entries.
static const std::array<ActorCutscene, 9> nativeChain{{
    {700, -1, -3, -1, 1, 0, 255, 0, 0, 27},
    {600, -1, -2, -1, 2, 0, 255, 0, 0, 27},
    {700, -1, -4, -1, 3, 0, 255, 0, 0, 27},
    {700, -1, -5, -1, 4, 0, 255, 0, 0, 27},
    {500, -1, -7, -1, 5, 0, 255, 0, 0, 32},
    {400, -1, -11, -1, 6, 0, 255, 0, 1, 32},
    {100, -1, -8, -1, 7, 0, 255, 0, 0, 32},
    {200, -1, -9, -1, 8, 0, 255, 0, 0, 32},
    {800, -1, -6, -1, -1, 0, 255, 0, 0, 32},
}};
static void Reset(int mode = COMBO_GAME_BOTH, s16 scene = SCENE_LOST_WOODS) {
  play = {};
  player = {};
  gSaveContext = {};
  gPlayState = &play;
  gGameState = &play.state;
  gComboGameMode = mode;
  gSaveContext.gameMode = GAMEMODE_NORMAL;
  gSaveContext.fileNum = 0;
  gSaveContext.save.entrance = scene == COMBO_SOLO_FOREST_MM_SCENE
                                   ? COMBO_SOLO_FOREST_MM_ENTRANCE
                                   : ENTRANCE(LOST_WOODS, 0);
  play.sceneId = scene;
  play.state.running = true;
  play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
  player.transformation = PLAYER_FORM_HUMAN;
  player.actor.scale = {.01f, .01f, .01f};
  sCameraNextUID = 0;
}
static void Load(SOH::Scene &scene, s32 spawn = 0, bool mirror = false) {
  static_assert(sizeof(ActorEntry) == 16);
  static_assert(sizeof(ActorCutscene) == 16 &&
                offsetof(ActorCutscene, csCamId) == 4 &&
                offsetof(ActorCutscene, additionalCsId) == 8 &&
                offsetof(ActorCutscene, letterboxSize) == 15);
  play.sceneSegment = &scene;
  const ActorEntry original = *scene.spawn;
  const SaveContext saved = gSaveContext;
  play.mainCamera.eye = {31, 41, 51};
  play.mainCamera.fov = 53;
  const Camera untouched = play.mainCamera;
  OTRPlay_InitScene(&play, spawn);
  Require(!std::memcmp(scene.spawn, &original, sizeof(original)),
          "scene resource spawn remains byte-identical");
  Require(!std::memcmp(&play.mainCamera, &untouched, sizeof(untouched)),
          "metadata preparation does not reset a camera");
  Require(play.linkActorEntry->id == original.id &&
              !std::memcmp(&play.linkActorEntry->pos, &original.pos,
                           sizeof(Vec3s)) &&
              play.linkActorEntry->rot.x == original.rot.x &&
              play.linkActorEntry->rot.z == original.rot.z &&
              (play.linkActorEntry->rot.y & ~0x7F) ==
                  (original.rot.y & ~0x7F) &&
              play.linkActorEntry->params == original.params,
          "metadata preparation preserves spawn pose/params/day bits");
#ifdef COMBO_BUILD
  ActorEntry *prepared = play.linkActorEntry;
#endif
#ifdef COMBO_BUILD
  if (mirror) {
    static Vec3s vertices[13];
    static CollisionPoly polygons[12];
    static CollisionHeader collision;
    vertices[0] = {-377, 80, 416};
    for (int i = 0; i < 12; ++i) {
      const float angle = i * 3.14159265359f / 6;
      vertices[i + 1] = {
          static_cast<s16>(std::round(-377 + 55 * std::cos(angle))), 80,
          static_cast<s16>(std::round(416 + 56 * std::sin(angle)))};
      polygons[i] = {};
      polygons[i].vtxData[0] = 0;
      polygons[i].vtxData[1] = i + 1;
      polygons[i].vtxData[2] = (i + 1) % 12 + 1;
      polygons[i].normal.y = 32767;
    }
    collision = {};
    collision.numVertices = 13;
    collision.vtxList = vertices;
    collision.numPolygons = 12;
    collision.polyList = polygons;
    play.colCtx.colHeader = &collision;
    MirrorRuntime::QueueArrival();
    MirrorRuntime::PrepareArrival(&play);
    Require(play.linkActorEntry->pos.x == -377 &&
                play.linkActorEntry->pos.y == 80 &&
                play.linkActorEntry->pos.z == 416,
            "actual Mirror arrival selects the stump cap before native "
            "Player_Init");
    Require(((play.linkActorEntry->rot.y >> 7) & 0x1FF) == 204 &&
                play.linkActorEntry->rot.x == original.rot.x &&
                play.linkActorEntry->rot.z == original.rot.z &&
                play.linkActorEntry->params ==
                    PLAYER_PARAMS(0xFF, PLAYER_START_MODE_D),
            "Mirror retains its cap-facing heading, native day bits and "
            "stationary start");
    Require(!std::memcmp(scene.spawn, &original, sizeof(original)),
            "Mirror arrival preserves cached scene spawn");
  }
#endif
  Actor_SpawnEntry(&play.actorCtx, play.linkActorEntry, &play);
  Play_AssignPlayerCsIdsFromScene(&play, player.actor.csId);
#ifdef COMBO_BUILD
  if (mirror) {
    MirrorRuntime::FinishArrival(&play);
    Require(play.linkActorEntry == prepared,
            "Mirror releases its temporary spawn to the prepared scene source");
  }
#endif
  Require(
      !std::memcmp(scene.spawn, &original, sizeof(original)) &&
          !std::memcmp(&gSaveContext, &saved, sizeof(saved)),
      "native spawn and Mirror arrival preserve resource bytes and save state");
  Camera_Init(&play.mainCamera, &play.view, &play.colCtx, &play);
  play.mainCamera.camId = CAM_ID_MAIN;
  play.mainCamera.setting = CAM_SET_NORMAL0;
  play.mainCamera.focalActor = &player.actor;
  play.mainCamera.focalActorPosRot = player.actor.world;
  play.mainCamera.focalActorPosRot.rot = player.actor.shape.rot;
  play.mainCamera.eye = play.mainCamera.eyeNext = {50, 100, 260};
  play.mainCamera.at = {0, 56, 0};
  play.mainCamera.dist = 268;
  play.cameraPtrs[CAM_ID_MAIN] = &play.mainCamera;
  Play_ChangeCameraStatus(&play, CAM_ID_MAIN, CAM_STATUS_ACTIVE);
}
struct OcarinaView {
  Vec3f eye, at;
  f32 fov;
  s16 setting, mode;
};
static void FollowCameraHudFrame() {
  // Clear-volume follow motion is outside this fixture. Its actual native
  // NORMAL1 row still drives the camera/interface ownership boundary.
  Require(play.mainCamera.setting == CAM_SET_NORMAL0,
          "follow camera retains its native setting");
  Camera_UpdateInterface(sSetNormal0ModeNormalData[9].val);
  NativeHudFrame(&play);
}
static bool FullHudVisible() {
  const auto &ui = play.interfaceCtx;
  return ui.healthAlpha == 255 && ui.magicAlpha == 255 && ui.aAlpha == 255 &&
         ui.cLeftAlpha == 255 && ui.cDownAlpha == 255 &&
         ui.cRightAlpha == 255 && ui.shipInterface.dpad.dUpAlpha == 255 &&
         ui.shipInterface.dpad.dDownAlpha == 255 &&
         ui.shipInterface.dpad.dLeftAlpha == 255 &&
         ui.shipInterface.dpad.dRightAlpha == 255 && ui.minimapAlpha == 255 &&
         ui.startAlpha == 255 && ui.bAlpha == 255;
}
static void LoadNextRoom(int room) {
  static SOH::Scene preparedRoom{nullptr, nullptr, 0, true};
  const auto id = play.playerCsIds[PLAYER_CS_ID_ITEM_OCARINA];
  const auto entries = sSceneCutsceneList;
  play.roomCtx.roomRequestAddr = &preparedRoom;
  play.roomCtx.status = 1;
  play.roomCtx.curRoom.num = room;
  Require(OTRfunc_800973FC(&play, &play.roomCtx) && play.roomCtx.status == 0 &&
              play.playerCsIds[PLAYER_CS_ID_ITEM_OCARINA] == id &&
              sSceneCutsceneList == entries,
          "native prepared-room completion retains the scene's player camera "
          "chain");
}
static OcarinaView PlayAndCancel(int form, bool allowMissing = false) {
  player.transformation = form;
  for (int frame = 0; frame < 10; ++frame)
    FollowCameraHudFrame();
  Require(FullHudVisible() && sCameraHudVisibility == HUD_VISIBILITY_ALL,
          "normal exploration starts with visible HUD and native ALL cache");
  const s16 csId = play.playerCsIds[PLAYER_CS_ID_ITEM_OCARINA];
  const bool missing = csId <= CS_ID_NONE;
  if (!missing) {
    // A competing global player cutscene has native priority 2. The prepared
    // forest entry must wait at priority 550 rather than bypassing arbitration.
    CutsceneManager_Queue(CS_ID_GLOBAL_ELEGY);
    Require(!func_808323C0(&player, csId),
            "ocarina queues behind a competing cutscene");
    CutsceneManager_Update();
    CutsceneManager_ClearWaiting();
    Require(CutsceneManager_IsNext(CS_ID_GLOBAL_ELEGY) == 1 &&
                CutsceneManager_IsNext(csId) == 0,
            "higher-priority native cutscene retains ownership");
    CutsceneManager_Update();
    Require(!func_808323C0(&player, csId),
            "first ocarina request queues through native priority arbitration");
    CutsceneManager_Update();
    CutsceneManager_ClearWaiting();
    Require(CutsceneManager_IsNext(csId) == 1,
            "native manager promotes the ocarina request");
    Require(func_808323C0(&player, csId),
            "second request starts the native ocarina cutscene");
    Require(letterbox == 27 &&
                gSaveContext.hudVisibility == HUD_VISIBILITY_NONE_ALT,
            "native ocarina HUD and letterbox metadata apply");
  } else {
    Require(func_808323C0(&player, csId),
            "native player accepts a missing camera ID and still plays");
  }
  Camera *camera = Play_GetCamera(&play, Play_GetActiveCamId(&play));
  if (!missing)
    Require(camera != &play.mainCamera &&
                play.mainCamera.status == CAM_STATUS_WAIT &&
                camera->setting == CAM_SET_ITEM1 &&
                camera->bgCamIndex == CS_CAM_ID_GLOBAL_ITEM_OCARINA,
            "native cutscene starts an ITEM1 ocarina subcamera");
  // Actual Player_Action_63 opens the actual message staff and owns
  // cancellation. Animation completion, audio/text resources and movement
  // return are explicit boundaries; they do not touch the cutscene ID, camera
  // or HUD state.
  player.actionFunc = Player_Action_63;
  player.skelAnime.mode = ANIMMODE_LOOP;
  player.av2.actionVar2 = 0;
  Player_Action_63(&player, &play);
  Require(play.msgCtx.msgMode == MSGMODE_OCARINA_STARTING &&
              play.interfaceCtx.bButtonInterfaceDoActionActive &&
              gSaveContext.nextHudVisibility == HUD_VISIBILITY_B_ALT,
          "native staff requests Stop/B-only HUD visibility");
  for (int frame = 0; frame < 20; ++frame) {
    if (!missing) {
      Camera_KeepOn4(camera);
      Camera_UpdateInterface(sCameraInterfaceFlags);
      // Native Play_Draw services the handler's deferred-view update before
      // clearing unk164, allowing KeepOn4 initialization to finish.
      if (play.view.unk164) {
        Camera_KeepOn4(camera);
        Camera_UpdateInterface(sCameraInterfaceFlags);
        play.view.unk164 = 0;
      }
      NativeHudFrame(&play);
    } else
      FollowCameraHudFrame();
    CutsceneManager_Update();
    Require(CutsceneManager_GetCurrentCsId() == csId,
            "ocarina cutscene remains active until native cancellation");
  }
  if (!missing)
    Require(std::isfinite(camera->eye.x) && std::isfinite(camera->eye.y) &&
                std::isfinite(camera->eye.z) && camera->eye.z != 260 &&
                camera->fov > 60,
            "native KeepOn4 creates the playing view");
  OcarinaView result{camera->eye, camera->at, camera->fov, camera->setting,
                     camera->mode};
  play.msgCtx.ocarinaMode = OCARINA_MODE_END;
  Player_Action_63(&player, &play);
  Require(!play.interfaceCtx.bButtonInterfaceDoActionActive &&
              player.actionFunc == NativeIdle,
          "actual native cancel releases Stop and returns player movement");
  CutsceneManager_Update();
  CutsceneManager_ClearWaiting();
  Require(CutsceneManager_GetCurrentCsId() == CS_ID_NONE &&
              play.activeCamId == CAM_ID_MAIN &&
              play.mainCamera.status == CAM_STATUS_ACTIVE &&
              play.cameraPtrs[1] == nullptr &&
              play.mainCamera.setting == CAM_SET_NORMAL0,
          "cancel returns to the native main camera");
  for (int frame = 0; frame < 10; ++frame)
    FollowCameraHudFrame();
  if (missing) {
    Require(!FullHudVisible() && play.interfaceCtx.bAlpha == 255 &&
                play.interfaceCtx.healthAlpha == 0 &&
                play.interfaceCtx.aAlpha == 0 &&
                play.interfaceCtx.cLeftAlpha == 0 &&
                sCameraHudVisibility == HUD_VISIBILITY_ALL,
            "missing ocarina chain reproduces wide camera and persistent "
            "sword-only HUD");
    std::printf("native lost-chain reproduction: camera=%d HUD cache=%d "
                "health=%d A=%d C=%d B=%d\n",
                play.activeCamId, sCameraHudVisibility,
                play.interfaceCtx.healthAlpha, play.interfaceCtx.aAlpha,
                play.interfaceCtx.cLeftAlpha, play.interfaceCtx.bAlpha);
    Require(allowMissing, "Mirror arrival must preserve the prepared ocarina "
                          "camera and restore the HUD");
  } else
    Require(FullHudVisible(),
            "native camera return restores hearts, magic, A/C/D-pad and B");
  return result;
}
static void SameView(const OcarinaView &a, const OcarinaView &b) {
  Require(!std::memcmp(&a.eye, &b.eye, sizeof(Vec3f)) &&
              !std::memcmp(&a.at, &b.at, sizeof(Vec3f)) && a.fov == b.fov &&
              a.setting == b.setting && a.mode == b.mode,
          "forest ocarina view matches the native scene across player forms");
}
#ifdef COMBO_BUILD
static void RejectedArrivalChains() {
  for (bool solo : {false, true}) {
    for (int reason = 0; reason < 7; ++reason) {
      Reset(solo ? COMBO_GAME_MM : COMBO_GAME_BOTH,
            solo ? COMBO_SOLO_FOREST_MM_SCENE : SCENE_LOST_WOODS);
      ActorEntry spawn = solo ? soloSpawn : combinedSpawn;
      spawn.rot.y &= ~0x7F;
      ActorCutscene entries[2] = {nativeChain[0], nativeChain[1]};
      entries[0].additionalCsId = CS_ID_NONE;
      entries[1].additionalCsId = CS_ID_NONE;
      s16 count = 1;
      switch (reason) {
      case 0:
        count = 0;
        break; // No scene metadata, even with malformed source ID zero.
      case 1:
        entries[0].csCamId = CS_CAM_ID_GLOBAL_ITEM_GET;
        break;
      case 2:
        entries[0].scriptIndex = 0;
        break;
      case 3:
        entries[0].additionalCsId = 1;
        break; // Truncated/changed chain.
      case 4:
        count = 2;
        break; // Another authored list must retain baseline handling.
      case 5:
        spawn.rot.y |= 1;
        break; // Foreign source ID cannot become chain zero.
      case 6:
        break; // Other spawns do not inherit the fallback.
      }
      SOH::Scene scene{&spawn, entries, count};
      Load(scene, reason == 6 ? 1 : 0, true);
      Require(player.actor.csId == CS_ID_NONE &&
                  play.playerCsIds[PLAYER_CS_ID_ITEM_OCARINA] == CS_ID_NONE &&
                  sSceneCutsceneCount == count,
              "no-chain, foreign, scripted, changed-list and other-spawn "
              "arrivals retain sentinel127");
      // Keep a negative control through the same real message/camera/HUD path.
      if (reason == 0)
        PlayAndCancel(PLAYER_FORM_HUMAN, true);
    }
  }
}
#endif
int main() {
  InitNativeCameraTable();
#ifdef COMBO_BUILD
  for (bool solo : {false, true}) {
    Reset(solo ? COMBO_GAME_MM : COMBO_GAME_BOTH,
          solo ? COMBO_SOLO_FOREST_MM_SCENE : SCENE_LOST_WOODS);
    SOH::Scene forest{solo ? &soloSpawn : &combinedSpawn};
    Load(forest, 0, true);
    std::printf("Mirror/native spawn: mode=%d actor.csId=%d ocarina.csId=%d "
                "scene.csCount=%d\n",
                gComboGameMode, player.actor.csId,
                play.playerCsIds[PLAYER_CS_ID_ITEM_OCARINA],
                sSceneCutsceneCount);
    PlayAndCancel(PLAYER_FORM_HUMAN);
  }
#endif
  for (int form = 0; form < PLAYER_FORM_MAX; ++form) {
    Reset();
    ActorEntry nativeSpawn = combinedSpawn;
    nativeSpawn.rot.y &= ~0x7F;
    auto chain = nativeChain;
    SOH::Scene nativeScene{&nativeSpawn, chain.data(), (s16)chain.size()};
    Load(nativeScene);
    Require(sSceneCutsceneList == chain.data() && sSceneCutsceneCount == 9 &&
                chain[0].priority == 550,
            "authored native cutscene chains are retained");
    const OcarinaView control = PlayAndCancel(form);
#ifdef COMBO_BUILD
    for (int visit = 0; visit < 3; ++visit) {
      for (bool solo : {false, true})
        for (bool mirror : {false, true}) {
          Reset(solo ? COMBO_GAME_MM : COMBO_GAME_BOTH,
                solo ? COMBO_SOLO_FOREST_MM_SCENE : SCENE_LOST_WOODS);
          SOH::Scene forest{solo ? &soloSpawn : &combinedSpawn};
          Load(forest, 0, mirror);
          Require(play.playerCsIds[PLAYER_CS_ID_ITEM_OCARINA] > CS_ID_NONE,
                  "playable forest supplies a native ocarina cutscene ID");
          Require(sSceneCutsceneCount == 2 &&
                      sSceneCutsceneList[0].scriptIndex == CS_SCRIPT_ID_NONE &&
                      sSceneCutsceneList[0].additionalCsId == 1 &&
                      sSceneCutsceneList[0].priority == 550,
                  "missing native ocarina and pickup metadata form one "
                  "unscripted chain");
          ActorCutscene expected = nativeChain[0];
          expected.priority =
              550; // Native Play_AssignPlayerCsIdsFromScene normalization.
          Require(
              !std::memcmp(&sSceneCutsceneList[0], &expected, sizeof(expected)),
              "native ocarina camera/script/priority/timing/HUD/end fields are "
              "retained");
          ActorCutscene pickup = nativeChain[1];
          pickup.priority = 500;
          pickup.additionalCsId = CS_ID_NONE;
          Require(
              play.playerCsIds[PLAYER_CS_ID_ITEM_GET] == 1 &&
                  !std::memcmp(&sSceneCutsceneList[1], &pickup, sizeof(pickup)),
              "pickup receives the native get-item camera with an "
              "unscripted terminal entry");
          for (int id = 1; id < PLAYER_CS_ID_MAX; ++id)
            if (id != PLAYER_CS_ID_ITEM_GET)
              Require(play.playerCsIds[id] == CS_ID_NONE,
                      "unrelated missing player cutscene IDs retain their "
                      "behavior");
          const auto view = PlayAndCancel(form);
          if (!mirror)
            SameView(view, control);
          PlayAndCancel(
              form); // A second use must restore the HUD without reloading.
          for (int room : {1, 2, 0}) {
            LoadNextRoom(room);
            PlayAndCancel(form);
          }
        }
    }
#endif
  }
  for (int guard = 0; guard < 12; ++guard) {
    Reset();
    ActorEntry spawn = combinedSpawn;
    if (guard == 0)
      gComboGameMode = COMBO_GAME_OOT;
    if (guard == 1)
      play.sceneId = SCENE_TOWN;
    if (guard == 2)
      gSaveContext.sceneLayer = 1;
    if (guard == 3)
      gSaveContext.save.entrance = ENTRANCE(LOST_WOODS, 1);
    if (guard == 4)
      gSaveContext.save.cutsceneIndex = 0xFFF0;
    if (guard == 5)
      gSaveContext.gameMode = GAMEMODE_TITLE_SCREEN;
    if (guard == 6)
      gSaveContext.fileNum = 0xFF;
    if (guard == 7)
      spawn.rot.y &= ~0x7F;
    if (guard == 8) {
      gComboGameMode = COMBO_GAME_MM;
      play.sceneId = SCENE_26SARUNOMORI;
    }
    if (guard == 10) {
      gComboGameMode = COMBO_GAME_MM;
      play.sceneId = COMBO_SOLO_FOREST_MM_SCENE;
    }
    if (guard == 11) {
      play.sceneId = COMBO_SOLO_FOREST_MM_SCENE;
      gSaveContext.save.entrance = COMBO_SOLO_FOREST_MM_ENTRANCE;
    }
    SOH::Scene scene{&spawn};
    play.sceneSegment = &scene;
    OTRPlay_InitScene(&play, guard == 9 ? 1 : 0);
#ifdef COMBO_BUILD
    Require(
        sSceneCutsceneCount == 0,
        "nonexploration, other scenes and authored spawn chains are unchanged");
#endif
  }
#ifdef COMBO_BUILD
  RejectedArrivalChains();
  Reset();
  ActorCutscene authored{600,
                         -1,
                         CS_CAM_ID_GLOBAL_ITEM_GET,
                         CS_SCRIPT_ID_NONE,
                         CS_ID_NONE,
                         0,
                         255,
                         0,
                         0,
                         27};
  SOH::Scene authoredScene{&combinedSpawn, &authored, 1};
  Load(authoredScene);
  Require(sSceneCutsceneCount == 1 && sSceneCutsceneList == &authored &&
              play.playerCsIds[PLAYER_CS_ID_ITEM_OCARINA] == CS_ID_NONE,
          "an authored nonempty scene list is never replaced to add an ocarina "
          "fallback");
#endif
#ifdef COMBO_BUILD
  std::puts(
      "PASS combined/MM-solo forest native ocarina queue, ITEM1/KeepOn4 "
      "camera, actual staff/Player_Action_63 cancel and HUD restoration: "
      "ordinary/Mirror arrivals, five forms, repeated use/re-entry, priority "
      "and rejected-chain guards, native room completion 0/1/2/0; save/spawn "
      "bytes preserved");
#else
  std::puts("PASS standalone MM native ocarina flow unchanged across five "
            "forms; forest adapter excluded");
#endif
}
