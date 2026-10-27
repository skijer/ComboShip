#include <cstdio>

#if !__has_include("../../soh/soh/Enhancements/Gameplay/ComboTempleSages.cpp")
int main() {
  std::fputs("FAIL: Temple Sages native adapter is not implemented\n", stderr);
  return 1;
}
#else
#define CVAR_PREFIX_COSMETIC "gCosmetics"
#define CVAR_PREFIX_ENHANCEMENT "gEnhancements"
#include "ComboGameMode.h"
#include "gameplay/CrossGameTitleCard.h"
#include "global.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/ShipInit.hpp"
#include <array>
#include <cassert>
#include <cstring>
#include <optional>
#include <ship/resource/ResourceManager.h>
#include <vector>

PlayState *gPlayState;
GameState *gGameState;
SaveContext gSaveContext{};
u8 gWeatherMode;
extern "C" {
int gComboGameMode = COMBO_GAME_BOTH;
}
static GameInteractor interactor;
GameInteractor *GameInteractor::Instance = &interactor;
static bool arenaAvailable = true;
static int missingRoute = -1;
static const char *missingSuffix = nullptr;
static unsigned preflightChecks;
static bool resourcesAreMod = false;
static bool ownerRegistered = true, ownerArchivesAvailable = true,
            ownerAltAssets = false;
static bool resourcesOnlyAlt = false, resourcesOnlyMeta = false;
static std::optional<int32_t> travelModeCVar;
static unsigned travelModeReads;
static std::optional<int32_t> hideTitles;
static bool randomized, titleVeto;
static unsigned titlePresentations;
extern "C" int32_t CVarGetInteger(const char *key, int32_t defaultValue) {
  if (std::strcmp(key, "gEnhancements.TimeSavers.DisableTitleCard") == 0)
    return hideTitles.value_or(defaultValue);
  if (std::strncmp(key, "gCosmetics.HUD.TitleCard.", 25) == 0)
    return 0;
  assert(std::strcmp(key, "gEnhancements.TempleSagesTravel") == 0 &&
         defaultValue == 1);
  ++travelModeReads;
  return travelModeCVar.value_or(defaultValue);
}
extern "C" Color_RGB8 CVarGetColor24(const char *, Color_RGB8 value) {
  return value;
}
extern "C" void GameInteractor_ExecuteOnPresentTitleCard() {
  ++titlePresentations;
}
/* ACTUAL TITLE FUNCTIONS */
extern "C" bool GameInteractor_Should(GIVanillaBehavior behavior,
                                      uint32_t value, ...) {
  bool result = value;
  bool *should = &result;
  switch (behavior) {
  /* ACTUAL TITLE POLICY */
  default:
    break;
  }
  return result && !titleVeto;
}
static size_t budgetMatrices, budgetCommands;
static unsigned matrixPushes, matrixPops, interpolationPushes,
    interpolationPops;
static Vec3f matrixPosition;
static std::vector<Vec3f> matrixPositions;
static std::vector<float> matrixRotations;
static std::array<Mtx, 5> matrices;
static std::array<Gfx, 256> graphics;
static std::array<std::array<Gfx, 1>, 5> portalDisplayLists;
static std::vector<int> loadedRoutes;
constexpr const char *routes[] = {"forest", "fire", "water", "spirit",
                                  "shadow"};
int RouteFor(const char *path) {
  for (int i = 0; i < 5; ++i) {
    const std::string prefix =
        std::string("custom/temple_sages/portals/") + routes[i] + "/";
    if (std::strncmp(path, prefix.c_str(), prefix.size()) == 0)
      return i;
  }
  assert(false && "resource belongs to no portal route");
  return -1;
}
static unsigned ownerScopes, portalLoads;
static Player player;
static CollisionPoly floorPoly;
static int sceneToken, roomToken;

extern "C" s32 Player_ActionToMagicSpell(Player *, s32) { return -1; }
/* ACTUAL CONTROL */
namespace Ship {
OwnRMScope::OwnRMScope(const char *game) {
  assert(std::strcmp(game, "oot") == 0);
  ++ownerScopes;
}
OwnRMScope::~OwnRMScope() { --ownerScopes; }
ResourceManager::ResourceManager() {}
ResourceManager::~ResourceManager() {}
ArchiveManager::ArchiveManager() {}
ArchiveManager::~ArchiveManager() {}
std::shared_ptr<ResourceManager>
CrossRMRegistry::Get(const std::string &owner) {
  assert(owner == "oot");
  static auto manager = std::make_shared<ResourceManager>();
  return ownerRegistered ? manager : nullptr;
}
std::shared_ptr<ArchiveManager> ResourceManager::GetArchiveManager() {
  static auto archives = std::make_shared<ArchiveManager>();
  return ownerArchivesAvailable ? archives : nullptr;
}
bool ResourceManager::IsAltAssetsEnabled() { return ownerAltAssets; }
bool ArchiveManager::HasFile(const std::string &selected) {
  ++preflightChecks;
  const bool alt = selected.starts_with("alt/");
  const bool meta = selected.ends_with(".meta");
  std::string path = alt ? selected.substr(4) : selected;
  if (meta)
    path.resize(path.size() - 5);
  const int route = RouteFor(path.c_str());
  if (resourcesOnlyAlt != alt || resourcesOnlyMeta != meta)
    return false;
  return route != missingRoute || !missingSuffix ||
         !path.ends_with(missingSuffix);
}
} // namespace Ship
extern "C" int ResourceMgr_OTRSigCheck(char *path) {
  return reinterpret_cast<uintptr_t>(path) >= 0x10000 &&
         std::memcmp(path, "__OTR__", 7) == 0;
}
Gfx *PakLoader_GetDLOverride(const char *) { return nullptr; }
extern "C" Gfx *ResourceMgr_LoadGfxByName(const char *path) {
  assert(ownerScopes == 1 && std::strncmp(path, "__OTR__", 7) == 0);
  const int route = RouteFor(path + 7);
  assert(std::string(path).ends_with("/effect"));
  loadedRoutes.push_back(route);
  ++portalLoads;
  return portalDisplayLists[route].data();
}
/* ACTUAL GBI */
extern "C" int ResourceMgr_IsModAssetForGame(const char *owner,
                                             const char *path) {
  assert(std::strcmp(owner, "oot") == 0);
  const int route = RouteFor(path);
  ++preflightChecks;
  // Production deliberately excludes soh.o2r from mod provenance. Built-in
  // resource availability must therefore not depend on this query.
  return resourcesAreMod && (route != missingRoute || !missingSuffix ||
                             !std::string(path).ends_with(missingSuffix));
}
extern "C" bool NeiGi_CanDrawLayers(PlayState *, size_t count, size_t opa,
                                    size_t xlu) {
  assert(opa == 0);
  budgetMatrices = count;
  budgetCommands = xlu;
  return arenaAvailable;
}
extern "C" void Graph_OpenDisps(Gfx **, GraphicsContext *, const char *, s32) {}
extern "C" void Graph_CloseDisps(Gfx **, GraphicsContext *, const char *, s32) {
}
extern "C" void Gfx_SetupDL_25Xlu(GraphicsContext *gfx) {
  gSPDisplayList(gfx->polyXlu.p++, reinterpret_cast<Gfx *>(0x1000));
}
extern "C" void Matrix_Push() { ++matrixPushes; }
extern "C" void Matrix_Pop() { ++matrixPops; }
extern "C" void Matrix_Translate(float x, float y, float z, u8 mode) {
  assert(mode == MTXMODE_NEW);
  matrixPosition = {x, y, z};
}
extern "C" void Matrix_RotateY(float yaw, u8 mode) {
  assert(mode == MTXMODE_APPLY);
  matrixRotations.push_back(yaw);
}
extern "C" Mtx *Matrix_NewMtx(GraphicsContext *, char *, s32) {
  assert(matrixPositions.size() < matrices.size());
  matrixPositions.push_back(matrixPosition);
  return &matrices[matrixPositions.size() - 1];
}
extern "C" void FrameInterpolation_RecordOpenChild(const void *, int) {
  ++interpolationPushes;
}
extern "C" void FrameInterpolation_RecordCloseChild() { ++interpolationPops; }
#include "../../soh/soh/Enhancements/Gameplay/ComboTempleSages.cpp"
static bool advance = true, cameraUnavailable;
static std::string confirmationText;
CustomMessage::CustomMessage(std::string text, TextBoxType, TextBoxPosition) {
  confirmationText = text;
}
void CustomMessage::AutoFormat() {}
void CustomMessage::LoadIntoFont() {}
/* ACTUAL MESSAGE STATE */
extern "C" u8 Message_ShouldAdvance(PlayState *) { return advance; }
extern "C" void Message_StartTextbox(PlayState *play, u16 id, Actor *) {
  play->msgCtx.textId = id;
  play->msgCtx.msgLength = 1;
  play->msgCtx.msgMode = MSGMODE_TEXT_START;
  bool fromTable = true;
  TempleSages::ConfirmationNative::OpenText(&id, &fromTable);
  assert(!fromTable);
}
extern "C" void Message_CloseTextbox(PlayState *play) {
  play->msgCtx.msgLength = 0;
  play->msgCtx.msgMode = MSGMODE_NONE;
}
extern "C" s16 Play_CreateSubCamera(PlayState *p) {
  if (cameraUnavailable)
    return SUBCAM_NONE;
  p->cameraPtrs[1] = &p->subCameras[0];
  p->cameraPtrs[1]->uid = 42;
  return 1;
}
extern "C" s16 Play_GetActiveCamId(PlayState *p) { return p->activeCamera; }
extern "C" s16 Play_ChangeCameraStatus(PlayState *p, s16 id, s16 status) {
  if (status == CAM_STAT_ACTIVE)
    p->activeCamera = id;
  return 1;
}
extern "C" void Play_ClearCamera(PlayState *p, s16 id) {
  p->cameraPtrs[id] = nullptr;
}
extern "C" void Camera_InitPlayerSettings(Camera *, Player *) {}
extern "C" s32 Camera_RequestSetting(Camera *, s16) { return 1; }
extern "C" s32 Play_CameraSetAtEye(PlayState *, s16, Vec3f *, Vec3f *) {
  return 1;
}
extern "C" s32 Play_CameraSetFov(PlayState *, s16, f32 fov) {
  assert(fov == 55);
  return 1;
}
static std::shared_ptr<SOH::Scene> hubResource;
std::shared_ptr<Ship::IResource>
ResourceMgr_GetResourceByNameHandlingMQ(const char *path) {
  assert(ownerScopes == 1 &&
         std::strcmp(path, "scenes/shared/tokinoma_scene/tokinoma_room_0") ==
             0);
  return hubResource;
}
extern "C" void Math_Vec3f_Copy(Vec3f *target, Vec3f *source) {
  *target = *source;
}
void NativeRespawn(PlayState *play, Player *self) {
  Actor *thisx = &self->actor;
  const int respawnFlag = gSaveContext.respawnFlag;
  int respawnMode = 0;
  /* ACTUAL RESPAWN */
  /* ACTUAL ROOM */
  gSaveContext.respawnFlag = 0;
}

template <class Hook, class... Args> void Fire(Args... args) {
  interactor.ExecuteHooks<Hook>(args...);
}
static std::array<ActorEntry, 7> actorEntries;
void Enter(PlayState &play, GraphicsContext &gfx, int mode = COMBO_GAME_BOTH) {
  Fire<GameInteractor::OnExitGame>(0);
  Fire<GameInteractor::OnPlayDestroy>();
  play = {};
  gfx = {};
  player = {};
  floorPoly = {};
  gSaveContext = {};
  gPlayState = &play;
  gGameState = &play.state;
  gComboGameMode = mode;
  gSaveContext.gameMode = GAMEMODE_NORMAL;
  gSaveContext.fileNum = 0;
  gSaveContext.sceneLayer = SCENE_LAYER_ADULT_DAY;
  gSaveContext.nextCutsceneIndex = 0xFFEF;
  gSaveContext.inventory.questItems = 0x3F;
  gWeatherMode = 7;
  play.sceneNum = SCENE_TEMPLE_OF_TIME;
  play.sceneSegment = &sceneToken;
  play.roomCtx.curRoom.num = 0;
  play.roomCtx.curRoom.segment = &roomToken;
  play.state.running = true;
  play.state.gfxCtx = &gfx;
  play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
  play.cameraPtrs[0] = &play.mainCamera;
  play.mainCamera.uid = 9;
  play.mainCamera.childCamIdx = SUBCAM_FREE;
  cameraUnavailable = false;
  player.actor.world.pos = {2000, -120, 0};
  player.actor.floorHeight = -120;
  player.actor.floorPoly = &floorPoly;
  player.actor.bgCheckFlags = BGCHECKFLAG_GROUND;
  floorPoly.normal.y = 32767;
  for (int i = 0; i < 5; ++i)
    actorEntries[i] = {ACTOR_EN_OKARINA_TAG,
                       {s16(i * 300), -120, 0},
                       {0, s16(i * 1000), 0},
                       s16(0xFC00 | i)};
  actorEntries[5] = {ACTOR_BG_TOKI_SWD, {1, 2, 3}, {0, 0, 0}, 0};
  play.setupActorList = actorEntries.data();
  play.numSetupActors = 6;
  hubResource = std::make_shared<SOH::Scene>();
  auto hubActors = std::make_shared<SOH::SetActorList>();
  for (int i = 0; i < 6; ++i) {
    const auto &actor = actorEntries[i];
    hubActors->actorList.push_back(
        {actor.id, actor.pos, actor.rot, actor.params});
  }
  hubActors->numActors = hubActors->actorList.size();
  hubResource->commands.push_back(hubActors);
  Fire<GameInteractor::OnSceneInit>(play.sceneNum);
  Fire<GameInteractor::AfterSceneCommands>(play.sceneNum);
  arenaAvailable = true;
  missingRoute = -1;
  missingSuffix = nullptr;
  resourcesAreMod = false;
  ownerRegistered = ownerArchivesAvailable = true;
  ownerAltAssets = resourcesOnlyAlt = resourcesOnlyMeta = false;
  preflightChecks = 0;
  loadedRoutes.clear();
  travelModeCVar.reset();
  hideTitles.reset();
  randomized = titleVeto = false;
  titlePresentations = 0;
  Combo_SOH_ConsumeCrossGameTitleCard();
  Combo_SOH_SwapCustomTitleCardPresentation(false);
  travelModeReads = 0;
  budgetMatrices = budgetCommands = 0;
  matrixPushes = matrixPops = interpolationPushes = interpolationPops = 0;
  ownerScopes = portalLoads = 0;
  matrixPositions.clear();
  matrixRotations.clear();
  gfx.polyXlu.p = graphics.data();
}
void RawUpdate() { Fire<GameInteractor::OnPlayerUpdate>(); }
void Choice(int choice, u16 button = BTN_A) {
  auto *play = gPlayState;
  play->msgCtx.msgLength = 1;
  play->msgCtx.msgMode = MSGMODE_TEXT_DONE;
  play->msgCtx.textboxEndType = TEXTBOX_ENDTYPE_2_CHOICE;
  play->msgCtx.choiceIndex = choice;
  play->state.input[0].press.button = 0;
  RawUpdate();
  play->state.input[0].press.button = button;
  RawUpdate();
  play->state.input[0].press.button = 0;
}
void Update() {
  RawUpdate();
  if (TempleSages::ConfirmationNative::Active() &&
      !TempleSages::ConfirmationNative::Holding())
    Choice(0);
}
void ExpectNoTravel(const PlayState &play) {
  assert(play.transitionTrigger == TRANS_TRIGGER_OFF &&
         gSaveContext.respawnFlag == 0);
}

void TestNativeArrivals() {
  PlayState play;
  GraphicsContext gfx;
  constexpr int expected[] = {0x600, 0x4F6, 0x604, 0x1F1, 0x568};
  for (int mode : {COMBO_GAME_BOTH, COMBO_GAME_OOT}) {
    for (int i = 0; i < 5; ++i) {
      Enter(play, gfx, mode);
      Update();
      player.actor.world.pos.x = i * 300;
      gSaveContext.inventory.questItems = 0;
      Update();
      ExpectNoTravel(play);
      gSaveContext.inventory.questItems = 1u << i;
      gSaveContext.respawn[RESPAWN_MODE_RETURN].pos = {123, 456, 789};
      Update();
      assert(play.nextEntranceIndex == expected[i] &&
             play.transitionTrigger == TRANS_TRIGGER_START);
      assert(play.transitionType == TRANS_TYPE_FADE_WHITE &&
             gSaveContext.nextTransitionType == TRANS_TYPE_FADE_WHITE);
      const auto &arrival = gSaveContext.respawn[RESPAWN_MODE_RETURN];
      assert(arrival.entranceIndex == expected[i] &&
             arrival.playerParams == 0x5FF && arrival.data == i);
      assert(gSaveContext.respawnFlag == -3 && gWeatherMode == 0);
      assert(arrival.pos.x == 123 && arrival.pos.y == 456 &&
             arrival.pos.z == 789);
    }
  }
  Enter(play, gfx);
  player.actor.world.pos.x = 0;
  Update();
  ExpectNoTravel(play);
  player.actor.world.pos.x = 2000;
  Update();
  actorEntries[5] = actorEntries[0];
  Fire<GameInteractor::AfterSceneCommands>(play.sceneNum);
  player.actor.world.pos.x = 0;
  Update();
  ExpectNoTravel(play);
}

void TestPackagedPortalReadiness() {
  PlayState play;
  GraphicsContext gfx;
  for (int mode : {COMBO_GAME_BOTH, COMBO_GAME_OOT}) {
    for (bool alt : {false, true}) {
      Enter(play, gfx, mode);
      ownerAltAssets = alt;
      Fire<GameInteractor::OnPlayDrawEnd>();
      assert(portalLoads == 5 && matrixPositions.size() == 5);
      RawUpdate();
      player.actor.world.pos.x = 0;
      RawUpdate();
      assert(TempleSages::ConfirmationNative::Active());
    }
  }
  // An Alt-only external archive can satisfy readiness only when selected.
  for (bool alt : {false, true}) {
    Enter(play, gfx);
    resourcesAreMod = resourcesOnlyAlt = true;
    ownerAltAssets = alt;
    Fire<GameInteractor::OnPlayDrawEnd>();
    assert(portalLoads == (alt ? 5u : 0u));
  }
  // Raw/meta availability follows the owning archive, not a host cache.
  for (bool alt : {false, true}) {
    Enter(play, gfx);
    resourcesOnlyMeta = true;
    resourcesOnlyAlt = ownerAltAssets = alt;
    Fire<GameInteractor::OnPlayDrawEnd>();
    assert(portalLoads == 5);
  }
  for (bool archivesMissing : {false, true}) {
    Enter(play, gfx);
    if (archivesMissing)
      ownerArchivesAvailable = false;
    else
      ownerRegistered = false;
    Fire<GameInteractor::OnPlayDrawEnd>();
    assert(portalLoads == 0 && matrixPositions.empty());
    RawUpdate();
    player.actor.world.pos.x = 0;
    RawUpdate();
    assert(!TempleSages::ConfirmationNative::Active());
  }
}

void TestNativeSafety() {
  PlayState play;
  GraphicsContext gfx;
  for (int bad = 0; bad < 27; ++bad) {
    Enter(play, gfx);
    Update();
    player.actor.world.pos.x = 0;
    switch (bad) {
    case 0:
      gComboGameMode = COMBO_GAME_MM;
      break;
    case 1:
      gSaveContext.fileNum = 0xFF;
      break;
    case 2:
      gSaveContext.gameMode = GAMEMODE_TITLE_SCREEN;
      break;
    case 3:
      play.state.running = false;
      break;
    case 4:
      play.sceneNum = SCENE_CHAMBER_OF_THE_SAGES;
      break;
    case 5:
      play.roomCtx.curRoom.num = 1;
      break;
    case 6:
      play.roomCtx.status = 1;
      break;
    case 7:
      play.roomCtx.curRoom.segment = nullptr;
      break;
    case 8:
      gSaveContext.sceneLayer = 4;
      break;
    case 9:
      gSaveContext.cutsceneIndex = 0xFFF0;
      break;
    case 10:
      gSaveContext.cutsceneTrigger = 1;
      break;
    case 11:
      play.csCtx.state = CS_STATE_UNSKIPPABLE_EXEC;
      break;
    case 12:
      play.pauseCtx.state = 1;
      break;
    case 13:
      play.gameOverCtx.state = GAMEOVER_DEATH_START;
      break;
    case 14:
      play.transitionMode = TRANS_MODE_SETUP;
      break;
    case 15:
      play.msgCtx.msgMode = MSGMODE_TEXT_DISPLAYING;
      break;
    case 16:
      player.actor.bgCheckFlags = 0;
      break;
    case 17:
      player.actor.floorPoly = nullptr;
      break;
    case 18:
      player.stateFlags1 = PLAYER_STATE1_INPUT_DISABLED;
      break;
    case 19:
      gSaveContext.nextCutsceneIndex = 0xFFF1;
      break;
    case 20:
      gGameState = nullptr;
      break;
    case 21:
      player.stateFlags1 = PLAYER_STATE1_ON_HORSE;
      break;
    case 22:
      player.stateFlags1 = PLAYER_STATE1_IN_WATER;
      break;
    case 23:
      player.stateFlags1 = PLAYER_STATE1_CLIMBING_LADDER;
      break;
    case 24:
      player.stateFlags1 = PLAYER_STATE1_TALKING;
      break;
    case 25:
      player.stateFlags1 = PLAYER_STATE1_GETTING_ITEM;
      break;
    case 26:
      gSaveContext.respawnFlag = 2;
      break;
    }
    Update();
    assert(play.transitionTrigger == TRANS_TRIGGER_OFF);
    if (bad != 26)
      assert(gSaveContext.respawnFlag == 0);
  }
  Enter(play, gfx);
  Update();
  Fire<GameInteractor::OnPlayDestroy>();
  player.actor.world.pos.x = 0;
  Update();
  ExpectNoTravel(play);
}

void TestNativePortal() {
  PlayState play;
  GraphicsContext gfx;
  Enter(play, gfx);
  Fire<GameInteractor::OnPlayDrawEnd>();
  assert(matrixPositions.size() == 5 && budgetMatrices == 5);
  assert(matrixPushes == matrixPops &&
         interpolationPushes == interpolationPops);
  assert(size_t(gfx.polyXlu.p - graphics.data()) <= budgetCommands);
  for (int i = 0; i < 5; ++i) {
    assert(matrixPositions[i].x == i * 300 && matrixPositions[i].z == 0);
    assert(matrixPositions[i].y > -120 && matrixPositions[i].y <= -118);
  }
  constexpr float angles[] = {0, .0958737992f, .191747598f, .287621398f,
                              .383495197f};
  assert(matrixRotations.size() == 5);
  for (int i = 0; i < 5; ++i)
    assert(std::fabs(matrixRotations[i] - angles[i]) < .00001f);
  constexpr unsigned colors[] = {0x69FF91, 0xFF6928, 0x5AB4FF, 0xFFC355,
                                 0xBE69FF};
  size_t colorCount = 0, portalCalls = 0, envRestores = 0;
  for (auto *cmd = graphics.data(); cmd != gfx.polyXlu.p; ++cmd) {
    if ((cmd->words.w0 >> 24) == G_SETPRIMCOLOR) {
      if (colorCount < 5) {
        assert((cmd->words.w1 >> 8) == colors[colorCount]);
        assert((cmd->words.w1 & 255) > 0);
      } else {
        assert(cmd->words.w1 ==
               0xFFFFFFFFu); // Restore caller tint after the portal.
      }
      ++colorCount;
    }
    if ((cmd->words.w0 >> 24) == G_SETENVCOLOR) {
      assert(cmd->words.w1 == 0xFFFFFFFFu);
      ++envRestores;
    }
    if ((cmd->words.w0 >> 24) == G_DL && cmd->words.w1 != 0x1000) {
      assert(cmd->words.w1 == reinterpret_cast<uintptr_t>(
                                  portalDisplayLists[portalCalls].data()));
      ++portalCalls;
    }
  }
  assert(colorCount == 6 && portalCalls == 5 && portalLoads == 5 &&
         ownerScopes == 0);
  assert((loadedRoutes == std::vector<int>{0, 1, 2, 3, 4}));
  assert(preflightChecks == 30 && envRestores == 1);
  auto alphaAt = [&](size_t route) {
    size_t index = 0;
    for (auto *cmd = graphics.data(); cmd != gfx.polyXlu.p; ++cmd)
      if ((cmd->words.w0 >> 24) == G_SETPRIMCOLOR && index++ == route)
        return unsigned(cmd->words.w1 & 255);
    return 0u;
  };
  // Every valid landing stays visible; only earned portals use the bright
  // pulse.
  Enter(play, gfx);
  gSaveContext.inventory.questItems =
      (1u << QUEST_MEDALLION_FIRE) | (1u << QUEST_MEDALLION_SHADOW);
  Fire<GameInteractor::OnPlayDrawEnd>();
  assert(matrixPositions.size() == 5 && budgetMatrices == 5 &&
         portalLoads == 5);
  for (size_t route = 0; route < 5; ++route)
    assert(alphaAt(route) == ((route == 1 || route == 4) ? 155u : 52u));
  assert(size_t(gfx.polyXlu.p - graphics.data()) <= budgetCommands);
  for (u32 rewards : {0u, 1u << QUEST_MEDALLION_LIGHT}) {
    Enter(play, gfx);
    gSaveContext.inventory.questItems = rewards;
    Fire<GameInteractor::OnPlayDrawEnd>();
    assert(matrixPositions.size() == 5 && portalLoads == 5);
    for (size_t route = 0; route < 5; ++route)
      assert(alphaAt(route) == 52u);
    Update();
    player.actor.world.pos.x = 0;
    Update();
    ExpectNoTravel(play);
  }
  Enter(play, gfx);
  gSaveContext.inventory.questItems = 1u << QUEST_MEDALLION_FOREST;
  play.gameplayFrames = 60;
  Fire<GameInteractor::OnPlayDrawEnd>();
  assert(alphaAt(0) == 190u && alphaAt(1) == 59u);
  Enter(play, gfx);
  arenaAvailable = false;
  Fire<GameInteractor::OnPlayDrawEnd>();
  assert(matrixPositions.empty() && gfx.polyXlu.p == graphics.data());
  // A missing route or sampler dependency fails closed before reserving arenas.
  for (int route = 0; route < 5; ++route) {
    for (const char *suffix :
         {"/effect", "/material", "/texture_flow", "/texture_detail",
          "/geometry", "/geometry_vtx"}) {
      Enter(play, gfx);
      missingRoute = route;
      missingSuffix = suffix;
      Fire<GameInteractor::OnPlayDrawEnd>();
      assert(matrixPositions.empty() && gfx.polyXlu.p == graphics.data());
      assert(budgetMatrices == 0 && budgetCommands == 0 && portalLoads == 0 &&
             ownerScopes == 0);
    }
  }
}
void TestNativeTravelModes() {
  PlayState play;
  GraphicsContext gfx;
  // Missing setting defaults to dungeon rewards. All five portals stay visible.
  Enter(play, gfx);
  gSaveContext.inventory.questItems = 0;
  Fire<GameInteractor::OnPlayDrawEnd>();
  assert(travelModeReads > 0 && portalLoads == 5);
  for (int mode : {0, 1, 2, -1, 3, 4, 999}) {
    for (u32 rewards : {0u, 0x1Fu}) {
      Enter(play, gfx);
      travelModeCVar = mode;
      gSaveContext.inventory.questItems = rewards;
      Fire<GameInteractor::OnPlayDrawEnd>();
      assert(portalLoads == 5 && matrixPositions.size() == 5);
      unsigned route = 0;
      for (auto *cmd = graphics.data(); cmd != gfx.polyXlu.p; ++cmd) {
        if ((cmd->words.w0 >> 24) == G_SETPRIMCOLOR && route++ < 5) {
          const bool bright =
              mode == 2 || (mode != 0 && mode != 3 && rewards != 0);
          assert((cmd->words.w1 & 255) == (bright ? 155u : 52u));
        }
      }
      Update(); // Deliberately arm from outside in the selected mode.
      player.actor.world.pos.x = 0;
      Update();
      const bool travel = mode == 2 || (mode != 0 && mode != 3 && rewards != 0);
      assert((play.transitionTrigger == TRANS_TRIGGER_START) == travel);
      assert(gSaveContext.inventory.questItems ==
             rewards); // Free never forges reward bits.
    }
  }
  // Every Free route retains its original native destination without rewards.
  constexpr int expected[] = {0x600, 0x4F6, 0x604, 0x1F1, 0x568};
  for (int i = 0; i < 5; ++i) {
    Enter(play, gfx);
    travelModeCVar = 2;
    gSaveContext.inventory.questItems = 0;
    Update();
    player.actor.world.pos.x = i * 300;
    Update();
    assert(play.nextEntranceIndex == expected[i] &&
           play.transitionTrigger == TRANS_TRIGGER_START);
    assert(gSaveContext.inventory.questItems == 0);
  }
  // Toggling while standing inside resets the entry latch, then requires
  // reentry.
  for (int mode : {0, 2}) {
    Enter(play, gfx);
    gSaveContext.inventory.questItems = 0;
    Update();
    player.actor.world.pos.x = 0;
    travelModeCVar = mode;
    Update();
    ExpectNoTravel(play);
    travelModeCVar = 2;
    Update();
    ExpectNoTravel(play);
    player.actor.world.pos.x = 2000;
    Update();
    player.actor.world.pos.x = 0;
    Update();
    assert(play.transitionTrigger == TRANS_TRIGGER_START &&
           gSaveContext.inventory.questItems == 0);
  }
}
void TestStaleLayouts() {
  PlayState play;
  GraphicsContext gfx;
  Enter(play, gfx);
  Update();
  actorEntries[6] = {
      ACTOR_EN_OKARINA_TAG, {1500, -120, 0}, {0, 0, 0}, s16(0xFC05)};
  play.numSetupActors = actorEntries.size();
  Fire<GameInteractor::AfterSceneCommands>(play.sceneNum);
  player.actor.world.pos.x = 0;
  Update();
  ExpectNoTravel(play);
  Fire<GameInteractor::OnPlayDrawEnd>();
  assert(matrixPositions.empty() && gfx.polyXlu.p == graphics.data());
  // A scene initialization cannot draw the preceding room's captured poses.
  Enter(play, gfx);
  Fire<GameInteractor::OnSceneInit>(play.sceneNum);
  Fire<GameInteractor::OnPlayDrawEnd>();
  assert(matrixPositions.empty() && gfx.polyXlu.p == graphics.data());
}
void Destination(PlayState &play, int route) {
  Fire<GameInteractor::OnPlayDestroy>();
  const auto &location = TempleSages::kReturnPlacements[route];
  play.sceneNum = location.scene;
  play.roomCtx.curRoom.num = location.room;
  play.transitionTrigger = TRANS_TRIGGER_OFF;
  play.transitionMode = TRANS_MODE_OFF;
  gSaveContext.respawnFlag = 0; // Native Play_Init consumed the outbound -3.
  player.actor.world.pos = {location.effect.position.x,
                            location.effect.position.y,
                            location.effect.position.z};
  player.actor.floorHeight = location.effect.position.y;
  Fire<GameInteractor::OnSceneInit>(play.sceneNum);
  Fire<GameInteractor::AfterSceneCommands>(play.sceneNum);
}
void TestReturns() {
  PlayState play;
  GraphicsContext gfx;
  for (int route = 0; route < 5; ++route) {
    for (int mode : {0, 1, 2, 3}) {
      for (u32 rewards :
           {0u, 1u << route, 1u << ((route + 1) % 5), 1u << (6 + route),
            1u << (6 + (route + 1) % 5), 0x800u}) {
        Enter(play, gfx);
        travelModeCVar = mode;
        gSaveContext.inventory.questItems = rewards;
        Destination(
            play,
            route); // Also works after fresh file load without visiting hub.
        Fire<GameInteractor::OnPlayDrawEnd>();
        assert(portalLoads == (mode == 0 ? 0u : 1u));
        if (mode != 0) {
          assert(loadedRoutes == std::vector<int>{route} &&
                 budgetMatrices == 1);
          assert(matrixPositions.size() == 1 &&
                 size_t(gfx.polyXlu.p - graphics.data()) <= budgetCommands);
        }
        Update(); // Spawn on the effect: never bounce.
        ExpectNoTravel(play);
        const auto on = player.actor.world.pos;
        player.actor.world.pos.x += 200;
        Update();
        player.actor.world.pos = on;
        const auto before = gSaveContext.respawn[RESPAWN_MODE_RETURN];
        Update();
        const bool allowed = mode == 2 ||
                             (mode == 1 && rewards == (1u << route)) ||
                             (mode == 3 && rewards == (1u << (6 + route)));
        assert((play.transitionTrigger == TRANS_TRIGGER_START) == allowed);
        assert(gSaveContext.inventory.questItems == rewards);
        if (!allowed)
          continue;
        assert(gSaveContext.respawnFlag == 2 &&
               play.nextEntranceIndex == ENTR_TEMPLE_OF_TIME_ENTRANCE);
        const auto arrival = gSaveContext.respawn[RESPAWN_MODE_RETURN];
        const auto expected = HubArrival(sHubPads[route]);
        assert(arrival.roomIndex == 0 && arrival.pos.x == expected.position.x &&
               arrival.pos.z == expected.position.z);
        assert(arrival.yaw == sHubPads[route].yaw &&
               arrival.playerParams == 0xDFF);
        const float dx = arrival.pos.x - sHubPads[route].position.x;
        const float dz = arrival.pos.z - sHubPads[route].position.z;
        assert(dx * dx + dz * dz > 84.0f * 84.0f);
        Fire<GameInteractor::OnPlayDestroy>();
        play.sceneNum = SCENE_TEMPLE_OF_TIME;
        play.roomCtx.curRoom.num = 0;
        play.transitionTrigger = TRANS_TRIGGER_OFF;
        Fire<GameInteractor::OnSceneInit>(play.sceneNum);
        Fire<GameInteractor::AfterSceneCommands>(play.sceneNum);
        NativeRespawn(&play, &player); // Exact Player_Init/Room_Init source,
                                       // not an ad-hoc teleport.
        assert(player.actor.world.pos.x == arrival.pos.x &&
               player.actor.world.pos.z == arrival.pos.z);
        assert(player.actor.home.pos.x == arrival.pos.x &&
               player.actor.prevPos.z == arrival.pos.z);
        assert(player.actor.shape.rot.y == arrival.yaw &&
               player.yaw == arrival.yaw);
        player.actor.floorHeight = arrival.pos.y;
        Update();
        ExpectNoTravel(play);
        assert(!sPending.active &&
               gSaveContext.respawn[RESPAWN_MODE_RETURN].playerParams ==
                   before.playerParams);
        assert(play.actorCtx.titleCtx.texture &&
               "successful scene portal return must queue the native chamber "
               "card");
        assert(
            std::strcmp(
                static_cast<const char *>(play.actorCtx.titleCtx.texture),
                "__OTR__textures/g_pn_14/gChamberOfTheSagesTitleCardENGTex") ==
            0);
        assert(play.actorCtx.titleCtx.width == 144 &&
               play.actorCtx.titleCtx.height == 24);
        player.actor.world.pos = {float(route * 300), -120, 0};
        Update(); // Deliberate outbound reentry still works after return.
        assert(play.nextEntranceIndex == TempleSages::Entrance(route) &&
               gSaveContext.respawnFlag == -3);
      }
    }
  }
  // Every destination refuses unsafe game/control/load/scene state.
  for (int route = 0; route < 5; ++route) {
    for (int bad = 0; bad < 9; ++bad) {
      Enter(play, gfx);
      Destination(play, route);
      player.actor.world.pos.x += 200;
      Update();
      player.actor.world.pos.x -= 200;
      switch (bad) {
      case 0:
        player.stateFlags1 = PLAYER_STATE1_INPUT_DISABLED;
        break;
      case 1:
        play.pauseCtx.state = 1;
        break;
      case 2:
        play.csCtx.state = CS_STATE_UNSKIPPABLE_EXEC;
        break;
      case 3:
        play.roomCtx.status = 1;
        break;
      case 4:
        play.roomCtx.curRoom.segment = nullptr;
        break;
      case 5:
        ++play.roomCtx.curRoom.num;
        break;
      case 6:
        gSaveContext.sceneLayer = 4;
        break;
      case 7:
        gComboGameMode = COMBO_GAME_MM;
        break;
      case 8:
        player.actor.floorPoly = nullptr;
        break;
      }
      Update();
      ExpectNoTravel(play);
    }
  }
  // Active room resources must contain the exact five unique marker routes.
  for (int bad = 0; bad < 4; ++bad) {
    Enter(play, gfx);
    if (bad == 0)
      hubResource.reset();
    if (bad == 1)
      std::dynamic_pointer_cast<SOH::SetActorList>(hubResource->commands[0])
          ->actorList[0]
          .id = 0;
    if (bad == 2)
      std::dynamic_pointer_cast<SOH::SetActorList>(hubResource->commands[0])
          ->actorList[5] =
          std::dynamic_pointer_cast<SOH::SetActorList>(hubResource->commands[0])
              ->actorList[0];
    if (bad == 3)
      std::dynamic_pointer_cast<SOH::SetActorList>(hubResource->commands[0])
          ->numActors = 255;
    Destination(play, 0);
    Fire<GameInteractor::OnPlayDrawEnd>();
    assert(portalLoads == 0);
    player.actor.world.pos.x += 200;
    Update();
    player.actor.world.pos.x -= 200;
    Update();
    ExpectNoTravel(play);
  }
  // Mode changes while native control is blocked still clear the entry latch.
  Enter(play, gfx);
  Destination(play, 0);
  player.actor.world.pos.x += 200;
  Update();
  player.actor.world.pos.x -= 200;
  player.stateFlags1 = PLAYER_STATE1_INPUT_DISABLED;
  travelModeCVar = 2;
  Update();
  player.stateFlags1 = 0;
  Update();
  ExpectNoTravel(play);
  for (bool replace : {false, true}) {
    Enter(play, gfx);
    Destination(play, 0);
    player.actor.world.pos.x += 200;
    Update();
    player.actor.world.pos.x -= 200;
    Update();
    assert(sPending.active);
    if (replace) {
      gSaveContext.respawn[RESPAWN_MODE_RETURN].entranceIndex = 1234;
      gSaveContext.respawnFlag = -3;
    }
    Fire<GameInteractor::OnSceneInit>(SCENE_HYRULE_FIELD);
    assert(!sPending.active && gSaveContext.respawnFlag == (replace ? -3 : 0));
    if (replace)
      assert(gSaveContext.respawn[RESPAWN_MODE_RETURN].entranceIndex == 1234);
  }
}
void TestGatedPortalBrightness() {
  PlayState play;
  GraphicsContext gfx;
  constexpr u32 songMasks[] = {0x40, 0x80, 0x100, 0x200, 0x400};
  constexpr u32 rewardMasks[] = {1, 2, 4, 8, 16};
  for (int mode : {1, 3}) {
    for (bool returning : {false, true}) {
      for (int route = 0; route < 5; ++route) {
        for (u32 inventory :
             {0u, songMasks[route], rewardMasks[route], 0x3Fu, 0x800u,
              songMasks[(route + 1) % 5], rewardMasks[(route + 1) % 5]}) {
          for (unsigned frame : {0u, 60u, 180u, 360u}) {
            Enter(play, gfx);
            travelModeCVar = mode;
            gSaveContext.inventory.questItems = inventory;
            if (returning)
              Destination(play, route);
            play.gameplayFrames = frame;
            Fire<GameInteractor::OnPlayDrawEnd>();
            std::vector<unsigned> alpha;
            for (auto *cmd = graphics.data(); cmd != gfx.polyXlu.p; ++cmd)
              if ((cmd->words.w0 >> 24) == G_SETPRIMCOLOR)
                alpha.push_back(cmd->words.w1 & 255);
            // The final prim-color command restores the caller's white tint.
            assert(alpha.size() == (returning ? 2u : 6u) &&
                   alpha.back() == 255u);
            alpha.pop_back();
            for (size_t index = 0; index < alpha.size(); ++index) {
              const int drawn = returning ? route : int(index);
              const bool unlocked =
                  (inventory &
                   (mode == 1 ? rewardMasks[drawn] : songMasks[drawn])) != 0;
              // Active pulse stays 120..190; dormant pulse stays 42..62.
              assert(unlocked ? alpha[index] >= 120 && alpha[index] <= 190
                              : alpha[index] >= 42 && alpha[index] <= 62);
            }
            assert(gSaveContext.inventory.questItems == inventory);
          }
        }
      }
    }
  }
}

void TestSongOwnershipBeforeYes() {
  PlayState play;
  GraphicsContext gfx;
  for (bool returning : {false, true}) {
    for (int route = 0; route < 5; ++route) {
      Enter(play, gfx);
      travelModeCVar = 3;
      gSaveContext.inventory.questItems = 1u << (QUEST_SONG_MINUET + route);
      if (returning) {
        Destination(play, route);
        player.actor.world.pos.x += 200;
      }
      RawUpdate();
      const auto on =
          returning ? TempleSages::kReturnPlacements[route].effect.position
                    : TempleSages::Point{float(route * 300), -120, 0};
      player.actor.world.pos = {on.x, on.y, on.z};
      RawUpdate();
      assert(TempleSages::ConfirmationNative::Active());
      gSaveContext.inventory.questItems =
          0x3F; // Losing the song cannot be bypassed by Yes or rewards.
      Choice(0);
      ExpectNoTravel(play);
      assert(!TempleSages::ConfirmationNative::Active());
    }
  }
}

void TestExplicitConfirmation() {
  PlayState play;
  GraphicsContext gfx;
  namespace Confirmation = TempleSages::ConfirmationNative;
  for (int route = 0; route < 5; ++route) {
    for (bool returning : {false, true}) {
      Enter(play, gfx);
      if (returning)
        Destination(play, route);
      const auto pad = sPads.Pads()[route];
      player.actor.world.pos = {pad.position.x + 200, pad.position.y,
                                pad.position.z};
      player.actor.floorHeight = pad.position.y;
      RawUpdate();
      player.actor.world.pos = {pad.position.x, pad.position.y, pad.position.z};
      RawUpdate();
      assert(Confirmation::Active() && !Confirmation::Holding());
      ExpectNoTravel(play);
      assert(player.stateFlags1 & PLAYER_STATE1_INPUT_DISABLED);
      assert(confirmationText == Confirmation::Text(route, returning));
      Fire<GameInteractor::OnPlayDrawEnd>();
      assert(portalLoads ==
             (returning ? 1u
                        : 5u)); // Owned message cannot hide its framed portal.
      Choice(1);
      ExpectNoTravel(play);
      assert(!Confirmation::Active() &&
             !(player.stateFlags1 & PLAYER_STATE1_INPUT_DISABLED));
      RawUpdate();
      RawUpdate();
      assert(!Confirmation::Active()); // No requires leaving the same tight
                                       // face volume.
      player.actor.world.pos.x += 200;
      RawUpdate();
      player.actor.world.pos.x -= 200;
      RawUpdate();
      assert(Confirmation::Active());
      Choice(0);
      assert(Confirmation::Holding() &&
             play.transitionTrigger == TRANS_TRIGGER_START);
      assert(!(player.stateFlags1 & PLAYER_STATE1_INPUT_DISABLED));
      const auto loads = portalLoads;
      matrixPositions.clear();
      matrixRotations.clear();
      gfx.polyXlu.p = graphics.data();
      Fire<GameInteractor::OnPlayDrawEnd>();
      assert(portalLoads ==
             loads + (returning ? 1u : 5u)); // Camera holds through white fade.
      Fire<GameInteractor::OnPlayDestroy>();
      assert(!Confirmation::Active() && play.activeCamera == CAM_ID_MAIN);
    }
  }
  // Missing art cannot create an invisible interaction in either direction.
  for (bool returning : {false, true}) {
    Enter(play, gfx);
    if (returning)
      Destination(play, 0);
    const auto pad = sPads.Pads()[0];
    player.actor.world.pos = {pad.position.x + 200, pad.position.y,
                              pad.position.z};
    player.actor.floorHeight = pad.position.y;
    RawUpdate();
    missingRoute = 0;
    missingSuffix = "/effect";
    player.actor.world.pos = {pad.position.x, pad.position.y, pad.position.z};
    RawUpdate();
    assert(!Confirmation::Active() &&
           !(player.stateFlags1 & PLAYER_STATE1_INPUT_DISABLED));
    ExpectNoTravel(play);
  }
  // Input/control restoration survives mode abort and same-player room changes.
  for (int interruption : {0, 1}) {
    Enter(play, gfx);
    RawUpdate();
    player.actor.world.pos.x = 0;
    RawUpdate();
    assert(Confirmation::Active());
    if (interruption == 0)
      travelModeCVar = 0;
    else
      ++play.roomCtx.curRoom.num;
    RawUpdate();
    assert(!Confirmation::Active() &&
           !(player.stateFlags1 & PLAYER_STATE1_INPUT_DISABLED));
    ExpectNoTravel(play);
  }
}
void TestChamberTitles() {
  PlayState play;
  GraphicsContext gfx;
  auto position = [&](float z) {
    player.actor.world.pos = {0, -120, z};
    RawUpdate();
  };
  auto cross = [&] {
    position(-1100);
    position(-1260);
  };
  auto expect = [&](const char *language = "ENG") {
    const auto &card = play.actorCtx.titleCtx;
    const std::string path =
        std::string("__OTR__textures/g_pn_14/gChamberOfTheSagesTitleCard") +
        language + "Tex";
    assert(card.texture && "tunnel entry must queue a native chamber card "
                           "without a scene transition");
    assert(std::strcmp(static_cast<const char *>(card.texture), path.c_str()) ==
           0);
    assert(card.x == 160 && card.y == 120 && card.width == 144 &&
           card.height == 24);
    assert(!card.isBossCard && !card.hasTranslation &&
           card.durationTimer == 80 && card.delayTimer == 20);
    assert(!Combo_SOH_HasCrossGameTitleCard() &&
           !Combo_SOH_IsPresentingCustomTitleCard());
    ExpectNoTravel(play);
  };
  for (int mode : {COMBO_GAME_BOTH, COMBO_GAME_OOT}) {
    for (int setup = 0; setup < 4; ++setup) {
      Enter(play, gfx, mode);
      gSaveContext.sceneLayer = setup;
      Fire<GameInteractor::AfterSceneCommands>(play.sceneNum);
      travelModeCVar =
          0; // The physical chamber still has a title with portals Off.
      const auto saved = gSaveContext;
      cross();
      expect();
      assert(!std::memcmp(&saved, &gSaveContext, sizeof(saved)));
      for (int frame = 0; frame < 120; ++frame) {
        TitleCard_Update(&play, &play.actorCtx.titleCtx);
        RawUpdate();
        if (frame == 50) {
          const auto &card = play.actorCtx.titleCtx;
          assert(card.alpha == 255 && card.intensityR == 255 &&
                 card.intensityG == 255 && card.intensityB == 255);
        }
      }
      assert(titlePresentations == 1 && play.actorCtx.titleCtx.alpha == 0 &&
             play.actorCtx.titleCtx.durationTimer == 0);
      cross();
      expect(); // Walking fully out through the tunnel rearms it.
    }
  }
  for (const auto &[language, suffix] :
       std::array<std::pair<int, const char *>, 4>{{{LANGUAGE_ENG, "ENG"},
                                                    {LANGUAGE_GER, "GER"},
                                                    {LANGUAGE_FRA, "FRA"},
                                                    {LANGUAGE_JPN, "JPN"}}}) {
    Enter(play, gfx);
    gSaveContext.language = language;
    cross();
    expect(suffix);
  }
  for (bool random : {false, true})
    for (int hide : {-1, 0, 1})
      for (bool veto : {false, true}) {
        Enter(play, gfx);
        randomized = random;
        titleVeto = veto;
        if (hide >= 0)
          hideTitles = hide;
        cross();
        assert(bool(play.actorCtx.titleCtx.texture) == (hide != 1 && !veto));
        play.actorCtx.titleCtx = {};
        RawUpdate();
        assert(!play.actorCtx.titleCtx
                    .texture); // No repeated veto/hidden request.
      }
  Enter(play, gfx);
  play.actorCtx.titleCtx.durationTimer = 2;
  cross();
  assert(!play.actorCtx.titleCtx.texture); // Queue waits for an existing card.
  play.actorCtx.titleCtx = {};
  play.pauseCtx.state = 1;
  RawUpdate();
  assert(!play.actorCtx.titleCtx.texture);
  play.pauseCtx.state = 0;
  RawUpdate();
  expect();
  for (int invalid = 0; invalid < 8; ++invalid) {
    Enter(play, gfx);
    play.actorCtx.titleCtx.durationTimer = 2;
    cross();
    if (invalid == 0)
      Fire<GameInteractor::OnSceneInit>(play.sceneNum);
    if (invalid == 1)
      Fire<GameInteractor::OnLoadGame>(0);
    if (invalid == 2)
      Fire<GameInteractor::OnExitGame>(0);
    if (invalid == 3)
      ++gSaveContext.fileNum;
    if (invalid == 4)
      ++play.roomCtx.curRoom.num;
    if (invalid == 5)
      gComboGameMode = COMBO_GAME_MM;
    if (invalid == 6)
      gSaveContext.sceneLayer = 4;
    if (invalid == 7)
      position(-1100); // Walk out before the queued title can appear.
    play.actorCtx.titleCtx = {};
    RawUpdate();
    assert(!play.actorCtx.titleCtx.texture);
  }
  // Ordinary temple arrival, chamber-side spawn, and moving outside the opening
  // never fire.
  Enter(play, gfx);
  position(-2400);
  assert(!play.actorCtx.titleCtx.texture);
  Enter(play, gfx);
  player.actor.world.pos = {500, -120, -1100};
  RawUpdate();
  player.actor.world.pos.z = -1260;
  RawUpdate();
  assert(!play.actorCtx.titleCtx.texture);
  Enter(play, gfx);
  play.numSetupActors = 0;
  Fire<GameInteractor::AfterSceneCommands>(play.sceneNum);
  cross();
  assert(!play.actorCtx.titleCtx.texture);
  std::puts(
      "PASS native chamber cards: return/tunnel queue, native "
      "resource/locales/white fade, hide policy, re-entry and stale state");
}
int main() {
  for (const auto &init : ShipInit::GetAll()["*"])
    init();
  TestChamberTitles();
  TestPackagedPortalReadiness();
  TestNativeArrivals();
  TestNativeSafety();
  TestNativePortal();
  TestNativeTravelModes();
  TestStaleLayouts();
  TestReturns();
  TestGatedPortalBrightness();
  TestSongOwnershipBeforeYes();
  TestExplicitConfirmation();
  std::puts("PASS Temple Sages actual adapter: registered room capture, native "
            "arrivals, safety gates and bounded portal commands");
}
#endif
