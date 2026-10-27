#include "ComboGameMode.h"
#include "actors/RegionalActorPlacement.h"
#include "entrances/LakeHyliaBridge.h"
#include "global.h"
extern "C" {
#include "src/overlays/actors/ovl_En_Viewer/static_story_actor.h"
}
#include <cassert>
#include <cstring>
static Actor spawned[16];
static unsigned spawnCount;
static bool ready, floorAvailable = true, blocked;
static int endpointRoute = 7;
extern "C" {
PlayState *gPlayState;
SaveContext gSaveContext;
int gComboGameMode = COMBO_GAME_BOTH;
s32 Player_InCsMode(PlayState *) { return 0; }
int ComboRegionalPassagesOot_Get(PlayState *, int *route, Vec3f *p, s16 *y) {
  if (!ready)
    return 0;
  *route = endpointRoute;
  *p = {1840, 280, 0};
  *y = -16384;
  if (endpointRoute == 6) {
    *p = {ComboLakeHylia::kMouth.x, ComboLakeHylia::kMouth.y,
          ComboLakeHylia::kMouth.z};
    *y = ComboLakeHylia::kYaw;
  }
  return 1;
}
f32 BgCheck_EntityRaycastFloor5(PlayState *, CollisionContext *,
                                CollisionPoly **p, s32 *bg, Actor *,
                                Vec3f *pos) {
  static CollisionPoly floor;
  *p = floorAvailable ? &floor : nullptr;
  *bg = BGCHECK_SCENE;
  return pos->y - 200;
}
u32 SurfaceType_GetSceneExitIndex(CollisionContext *, CollisionPoly *, s32) {
  return 0;
}
s32 BgCheck_EntityLineTest1(CollisionContext *, Vec3f *, Vec3f *, Vec3f *,
                            CollisionPoly **, s32, s32, s32, s32, s32 *) {
  return blocked;
}
void Actor_Kill(Actor *a) { a->update = nullptr; }
void Alive(Actor *, PlayState *) {}
Actor *Actor_Spawn(ActorContext *ctx, PlayState *, s16 id, f32 x, f32 y, f32 z,
                   s16 rx, s16 ry, s16 rz, s16 params) {
  assert(spawnCount < 16);
  auto *a = &spawned[spawnCount++];
  a->id = id;
  a->params = params;
  a->world.pos = {x, y, z};
  a->update = Alive;
  a->room = 0;
  a->next = ctx->actorLists[ACTORCAT_NPC].head;
  ctx->actorLists[ACTORCAT_NPC].head = a;
  return a;
}
}
/* PRODUCTION_RUNTIME */
int main() {
  PlayState p{};
  gPlayState = &p;
  p.state.running = true;
  p.roomCtx.curRoom.segment = (void *)1;
  Actor authored{};
  authored.id = ACTOR_EN_VIEWER;
  authored.params = 0x7E1F;
  authored.update = Alive;
  p.actorCtx.actorLists[ACTORCAT_NPC].head = &authored;
  for (int mode : {COMBO_GAME_OOT, COMBO_GAME_MM, COMBO_GAME_BOTH})
    for (int route : {5, 6, 7})
      for (int tick = 0; tick < 120; ++tick) {
        gComboGameMode = mode;
        endpointRoute = route;
        ready = true;
        ComboRegionalActorsOot_Update();
        assert(spawnCount == 0 && authored.update == Alive);
        assert(p.actorCtx.actorLists[ACTORCAT_NPC].head == &authored);
      }
  puts("PASS regional scenes: no automatic catalogue actors; Prelude-authored actor preserved");
}
