#include "entrances/BlackMarketEntrance.h"
#include "global.h"
#ifdef COMBO_BUILD
#include "2s2h/Enhancements/Camera/ComboForestOcarina.h"
#endif
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>

SaveContext gSaveContext{};
#ifdef COMBO_BUILD
extern "C" {
int gComboGameMode = COMBO_GAME_BOTH;
}
#endif
namespace SOH {
struct Scene {
  ActorEntry *spawn;
  ActorCutscene *cutscenes;
  s16 count;
  bool hasCutsceneCommand = true;
};
} // namespace SOH

static Player player;
extern "C" void Object_InitContext(GameState *, ObjectContext *) {}
extern "C" void LightContext_Init(PlayState *, LightContext *) {}
extern "C" void Scene_ResetTransitionActorList(GameState *,
                                               TransitionActorList *) {}
extern "C" void Room_Init(PlayState *, RoomContext *) {}
extern "C" void Play_InitEnvironment(PlayState *, s16) {}
s32 OTRScene_ExecuteCommands(PlayState *play, SOH::Scene *scene) {
  play->linkActorEntry = scene->spawn;
  if (scene->hasCutsceneCommand)
    CutsceneManager_Init(play, scene->cutscenes, scene->count);
  return 0;
}
extern "C" Actor *Actor_SpawnAsChildAndCutscene(ActorContext *, PlayState *,
                                                s16 id, f32 x, f32 y, f32 z,
                                                s16 rx, s16 ry, s16 rz,
                                                s32 params, u32 csId, u32,
                                                Actor *) {
  Actor *actor = &player.actor;
  actor->id = id;
  actor->home.pos = {x, y, z};
  actor->home.rot = {rx, ry, rz};
  actor->params = params;
  /* CS_ID_NORMALIZATION */
  return actor;
}

/* PRODUCTION */

static PlayState play;
int main() {
  ActorEntry resourceSpawn = /* RESOURCE_SPAWN */;
  const ActorEntry original = resourceSpawn;
  SOH::Scene scene{&resourceSpawn, nullptr, 0};
  play.sceneSegment = &scene;
#ifdef COMBO_BUILD
  // Replay the shipped private forest header, including its missing 0x1B.
  // A cold OoT handoff starts with no MM scene; a later recall can leave a
  // different scene's camera manager behind. Both must bind this scene.
  ActorEntry forestSpawn = /* FOREST_SPAWN */;
  SOH::Scene forest{&forestSpawn, nullptr, 0, /* FOREST_HAS_CUTSCENES */};
  gSaveContext.gameMode = GAMEMODE_NORMAL;
  gSaveContext.save.entrance = ENTRANCE(LOST_WOODS, 0);
  PlayState previous{};
  ActorCutscene previousList[1]{};
  for (int entry = 0; entry < 2; ++entry) {
    if (entry == 0)
      sCutsceneMgr.play = nullptr;
    else
      CutsceneManager_Init(&previous, previousList, 1);
    sWaitingCutsceneList[0] = sNextCutsceneList[0] = 255;
    play.sceneSegment = &forest;
    play.sceneId = SCENE_LOST_WOODS;
    OTRPlay_InitScene(&play, 0);
    assert(sCutsceneMgr.play == &play &&
           "private forest must initialize its camera manager before native "
           "camera setup");
    assert(sSceneCutsceneCount == 2 &&
           sSceneCutsceneList[0].csCamId == CS_CAM_ID_GLOBAL_ITEM_OCARINA &&
           sSceneCutsceneList[0].additionalCsId == 1 &&
           sSceneCutsceneList[1].csCamId == CS_CAM_ID_GLOBAL_ITEM_GET &&
           sSceneCutsceneList[1].additionalCsId == CS_ID_NONE &&
           sWaitingCutsceneList[0] == 0 && sNextCutsceneList[0] == 0);
    Play_AssignPlayerCsIdsFromScene(&play, play.linkActorEntry->rot.y & 0x7F);
    assert(play.playerCsIds[PLAYER_CS_ID_ITEM_OCARINA] == 0 &&
           play.playerCsIds[PLAYER_CS_ID_ITEM_GET] == 1);
    play.mainCamera.camId = CAM_ID_MAIN;
    play.mainCamera.eye = {10, 20, 30};
    CutsceneManager_StoreCamera(&play.mainCamera);
    assert(sCutsceneMgr.isCameraStored && play.subCameras[2].eye.y == 20 &&
           play.subCameras[2].status == CAM_STATUS_INACTIVE);
  }
  // A normal scene's own cutscene command must replace the empty default.
  SOH::Scene nativeScene{&resourceSpawn, previousList, 1};
  play.sceneSegment = &nativeScene;
  OTRPlay_InitScene(&play, 0);
  assert(sSceneCutsceneList == previousList && sSceneCutsceneCount == 1 &&
         !sCutsceneMgr.isCameraStored);
  play.sceneSegment = &scene;
  puts("PASS private forest cold OoT arrival and recalled scene: real camera "
       "storage, fresh cutscene queues and "
       "native-header override");
  // Entry and repeated entry use the exact bundled/legacy archive row for
  // every native MM form. This used to read address 0x4 during assignment.
  for (int cycle = 0; cycle < 3; ++cycle) {
    for (int form = 0; form < PLAYER_FORM_MAX; ++form) {
      gSaveContext.save.playerForm = form;
      play.sceneId = COMBO_BLACK_MARKET_SCENE;
      OTRPlay_InitScene(&play, 0);
      Actor_SpawnEntry(&play.actorCtx, play.linkActorEntry, &play);
      Play_AssignPlayerCsIdsFromScene(&play, player.actor.csId);
      assert(player.actor.csId == CS_ID_NONE);
      for (s16 id : play.playerCsIds)
        assert(id == CS_ID_NONE);
      assert(player.actor.home.rot.y == COMBO_BLACK_MARKET_SPAWN_YAW);
      assert(player.actor.home.pos.x == original.pos.x &&
             player.actor.home.pos.y == original.pos.y &&
             player.actor.home.pos.z == original.pos.z &&
             player.actor.params == original.params);
      assert(gSaveContext.save.playerForm == form);
      assert(play.linkActorEntry != &resourceSpawn);
      assert(std::memcmp(&resourceSpawn, &original, sizeof(original)) == 0);
    }
  }
  scene.spawn = nullptr;
  OTRPlay_InitScene(&play, 0);
  assert(play.linkActorEntry == nullptr);
  scene.spawn = &resourceSpawn;
  // An already authored MM spawn with a real chain is left intact, including
  // scene-local item, transformation, death, revive and travel cameras.
#endif
  ActorCutscene chain[PLAYER_CS_ID_MAX]{};
  for (int i = 0; i < PLAYER_CS_ID_MAX; ++i) {
    chain[i].csCamId = sPlayerCsIdToCsCamId[i];
    chain[i].additionalCsId = i + 1 == PLAYER_CS_ID_MAX ? CS_ID_NONE : i + 1;
    chain[i].priority = 700;
  }
  scene.cutscenes = chain;
  scene.count = PLAYER_CS_ID_MAX;
  resourceSpawn.rot.y = 180 << 7;
  for (s16 sceneId : {s16(COMBO_BLACK_MARKET_SCENE), s16(SCENE_TOWN),
                      s16(SCENE_LOST_WOODS)}) {
    play.sceneId = sceneId;
    OTRPlay_InitScene(&play, 0);
    assert(play.linkActorEntry == &resourceSpawn);
    Actor_SpawnEntry(&play.actorCtx, play.linkActorEntry, &play);
    Play_AssignPlayerCsIdsFromScene(&play, player.actor.csId);
    for (int i = 0; i < PLAYER_CS_ID_MAX; ++i)
      assert(play.playerCsIds[i] == i);
    assert(chain[0].priority == 550);
  }
  // Ordinary scenes keep their native packed rotation and CS_ID_NONE.
  scene.cutscenes = nullptr;
  scene.count = 0;
  resourceSpawn.rot.y = (90 << 7) | 127;
  play.sceneId = SCENE_TOWN;
  OTRPlay_InitScene(&play, 0);
  assert(play.linkActorEntry == &resourceSpawn);
  Actor_SpawnEntry(&play.actorCtx, play.linkActorEntry, &play);
  Play_AssignPlayerCsIdsFromScene(&play, player.actor.csId);
  assert(player.actor.csId == CS_ID_NONE &&
         player.actor.home.rot.y == DEG_TO_BINANG(90));
#ifdef COMBO_BUILD
  puts("PASS Combo market spawn/re-entry across five forms; source "
       "cache/form/params preserved; native scene chains unchanged");
#else
  puts("PASS standalone MM scene startup and native player cutscene chains "
       "unchanged");
#endif
}
