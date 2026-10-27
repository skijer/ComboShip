#include "overlays/actors/ovl_Dm_Stk/z_dm_stk.h"
#include "2s2h/Enhancements/Gameplay/ComboOpeningForest.h"
#include "entrances/SoloForest.h"
#include "entrances/ForestActorPlacements.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#define CHECK(x)                                                      \
    do {                                                              \
        if (!(x)) {                                                   \
            std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); \
            std::exit(1);                                             \
        }                                                             \
    } while (0)
static unsigned initialized, killed, freed, overlaps, destroyed, draws, spawns;
static bool floorAvailable = true, playerInCutscene;
static Actor spawned;
static const char* drawnPaths[4];
static unsigned drawnPathCount;
void FrameInterpolation_RecordOpenChild(const void*, int) {
}
void FrameInterpolation_RecordCloseChild() {
}
extern "C" {
SaveContext gSaveContext{};
PlayState* gPlayState;
GameState* gGameState;
int gComboGameMode;
void Actor_Kill(Actor*) {
    ++killed;
}
void Actor_SetScale(Actor* a, f32 s) {
    a->scale = { s, s, s };
}
void ActorShape_Init(ActorShape* shape, f32 y, ActorShadowFunc, f32) {
    shape->yOffset = y;
}
void ActorShadow_DrawCircle(Actor*, Lights*, PlayState*) {
}
void Actor_SetFocus(Actor* a, f32 y) {
    a->focus.pos = a->world.pos;
    a->focus.pos.y += y;
}
bool Player_InCsMode(PlayState*) {
    return playerInCutscene;
}
f32 Math_SinS(s16 angle) {
    return std::sin(angle * (6.28318530718f / 65536));
}
f32 BgCheck_EntityRaycastFloor5(CollisionContext*, CollisionPoly** poly, s32*, Actor*, Vec3f*) {
    static CollisionPoly floor;
    *poly = floorAvailable ? &floor : nullptr;
    return floorAvailable ? 123.0f : BGCHECK_Y_MIN;
}
s32 Collider_InitCylinder(PlayState*, ColliderCylinder*) {
    return 1;
}
s32 Collider_SetCylinder(PlayState*, ColliderCylinder* c, Actor*, ColliderCylinderInit* init) {
    c->dim = init->dim;
    c->base.atFlags = init->base.atFlags;
    c->base.acFlags = init->base.acFlags;
    c->base.ocFlags1 = init->base.ocFlags1;
    return 1;
}
s32 Collider_DestroyCylinder(PlayState*, ColliderCylinder*) {
    ++destroyed;
    return 1;
}
void Collider_UpdateCylinder(Actor* a, ColliderCylinder* c) {
    c->dim.pos = { (s16)a->world.pos.x, (s16)a->world.pos.y, (s16)a->world.pos.z };
}
s32 CollisionCheck_SetOC(PlayState*, CollisionCheckContext*, Collider* c) {
    CHECK(c->atFlags == AT_NONE && c->acFlags == AC_NONE && (c->ocFlags1 & OC1_ON));
    ++overlaps;
    return 1;
}
void SkelAnime_InitFlex(PlayState*, SkelAnime* a, FlexSkeletonHeader* skeleton, AnimationHeader* animation,
                        Vec3s* joints, Vec3s* morph, s32 count) {
    CHECK(std::strstr((const char*)skeleton, "gSkullKidSkel"));
    CHECK(std::strstr((const char*)animation, "gSkullKidRecliningFloatAnim"));
    CHECK(joints && morph && count == SKULL_KID_LIMB_MAX);
    a->jointTable = joints;
    a->morphTable = morph;
    a->limbCount = count;
    a->dListCount = 20;
    ++initialized;
}
s32 SkelAnime_Update(SkelAnime* a) {
    a->curFrame = std::fmod(a->curFrame + 1, 40.0f);
    return 0;
}
void ResourceMgr_UnregisterSkeleton(SkelAnime*) {
    ++freed;
}
void Graph_OpenDisps(Gfx**, Gfx*, GraphicsContext*, const char*, s32) {
}
void Graph_CloseDisps(Gfx**, Gfx*, GraphicsContext*, const char*, s32) {
}
void Gfx_SetupDL25_Opa(GraphicsContext*) {
}
void Scene_SetRenderModeXlu(PlayState*, s32, u32) {
}
Mtx* Matrix_Finalize(GraphicsContext*) {
    static Mtx m;
    return &m;
}
void gSPDisplayList(Gfx* pkt, Gfx* display) {
    CHECK(drawnPathCount < 4);
    drawnPaths[drawnPathCount++] = (const char*)display;
    gDma1p(pkt, G_DL_OTR_FILEPATH, display, 0, G_DL_PUSH);
}
void SkelAnime_DrawFlexOpa(PlayState* play, void**, Vec3s*, s32 count, OverrideLimbDrawOpa override,
                           PostLimbDrawOpa post, Actor* actor) {
    CHECK(count == 20 && override && post);
    ++draws;
    for (int limb : { SKULL_KID_LIMB_HEAD, SKULL_KID_LIMB_LEFT_HAND }) {
        Gfx* display = (Gfx*)1;
        Vec3f pos{};
        Vec3s rot{};
        CHECK(!override(play, limb, &display, &pos, &rot, actor) && !display);
        post(play, limb, &display, &rot, actor);
    }
}
Actor* Actor_Spawn(ActorContext*, PlayState*, s16 id, f32 x, f32 y, f32 z, s16, s16 yaw, s16, s32 params) {
    ++spawns;
    spawned = {};
    spawned.id = id;
    spawned.params = params;
    spawned.world.pos = { x, y, z };
    spawned.shape.rot.y = yaw;
    return &spawned;
}
}
#include "overlays/actors/ovl_Dm_Stk/z_dm_stk_combo.h"

/* PRODUCTION_SKULL_KID_SPAWN */

int main() {
    static PlayState play;
    CHECK(ComboForestSkullKid_ObjectId(ACTOR_DM_STK, COMBO_FOREST_SKULL_KID_PARAMS, OBJECT_STK) == GAMEPLAY_KEEP);
    CHECK(ComboForestSkullKid_ObjectId(ACTOR_DM_STK, 0, OBJECT_STK) == OBJECT_STK);
    CHECK(ComboForestSkullKid_ObjectId(ACTOR_DM_STK, 1, OBJECT_STK) == OBJECT_STK);
    CHECK(ComboForestSkullKid_ObjectId(ACTOR_EN_VIEWER, COMBO_FOREST_SKULL_KID_PARAMS, OBJECT_STK) == OBJECT_STK);
    DmStk ordinary{}, kid{};
    DmStk before = ordinary;
    CHECK(!DmStk_ComboTryInit(&ordinary.actor, &play));
    CHECK(!std::memcmp(&before, &ordinary, sizeof(before)));
    kid.actor.params = COMBO_FOREST_SKULL_KID_PARAMS;
    CHECK(DmStk_ComboTryInit(&kid.actor, &play) && killed == 1 && !kid.actor.destroy && !initialized);
    gComboGameMode = COMBO_GAME_BOTH;
    gSaveContext.fileNum = 0;
    gSaveContext.gameMode = GAMEMODE_NORMAL;
    gSaveContext.save.entrance = ENTRANCE(LOST_WOODS, 0);
    play.sceneId = SCENE_LOST_WOODS;
    play.roomCtx.curRoom.num = 1;
    kid.actor.world.pos = { 2250, 140, 220 };
    kid.actor.world.rot = { 100, -0x5000, 200 };
    SaveContext saveBefore = gSaveContext;
    CHECK(DmStk_ComboTryInit(&kid.actor, &play) && initialized == 1);
    CHECK(kid.actor.scale.x == .01f && kid.actor.world.rot.x == 0 && kid.actor.world.rot.z == 0);
    CHECK(kid.actor.shape.rot.y == -0x5000 && kid.actor.update && kid.actor.draw && kid.actor.destroy);
    CHECK(kid.actor.gravity == 0 && kid.actor.speed == 0);
    for (int tick = 0; tick < 80; ++tick) {
        kid.actor.update(&kid.actor, &play);
        CHECK(std::fabs(kid.actor.shape.yOffset * kid.actor.scale.y) <= 10.001f);
    }
    CHECK(overlaps == 80 && !std::memcmp(&saveBefore, &gSaveContext, sizeof(saveBefore)));
    CHECK(kid.actor.world.pos.x == 2250 && kid.actor.world.pos.y == 140 && kid.actor.world.pos.z == 220);
    static GraphicsContext gfx;
    static Gfx commands[32];
    gfx.polyOpa.p = commands;
    play.state.gfxCtx = &gfx;
    kid.actor.draw(&kid.actor, &play);
    CHECK(draws == 1 && gfx.polyOpa.p > commands);
    CHECK(drawnPathCount == 4 && std::strstr(drawnPaths[0], "gSkullKidNormalHeadDL") &&
          std::strstr(drawnPaths[1], "gSkullKidNormalEyesDL") &&
          std::strstr(drawnPaths[2], "gSkullKidMajorasMask1DL") &&
          std::strstr(drawnPaths[3], "gSkullKidOpenLeftHandDL"));
    kid.actor.destroy(&kid.actor, &play);
    CHECK(freed == 1 && destroyed == 1);
    puts("PASS forest Skull Kid: exact reclining clip, authored transform, hover, native mask/hands, OC-only "
         "collision, cleanup, no save writes");

    gPlayState = &play;
    gGameState = &play.state;
    play.state.running = true;
    play.roomCtx.curRoom.segment = (void*)1;
    auto attempt = [&] {
        sSpawnAttempted = false;
        SpawnForestSkullKid();
    };
    play.roomCtx.curRoom.num = 0;
    attempt();
    CHECK(!spawns);
    play.roomCtx.curRoom.num = 1;
    gComboGameMode = COMBO_GAME_MM;
    attempt();
    CHECK(!spawns);
    gComboGameMode = COMBO_GAME_BOTH;
    gSaveContext.sceneLayer = 1;
    attempt();
    CHECK(!spawns);
    gSaveContext.sceneLayer = 0;
    gSaveContext.fileNum = 0xFF;
    attempt();
    CHECK(!spawns);
    gSaveContext.fileNum = 0;
    play.pauseCtx.state = PAUSE_STATE_MAIN;
    attempt();
    CHECK(!spawns);
    play.pauseCtx.state = PAUSE_STATE_OFF;
    play.transitionTrigger = TRANS_TRIGGER_START;
    attempt();
    CHECK(!spawns);
    play.transitionTrigger = TRANS_TRIGGER_OFF;
    playerInCutscene = true;
    attempt();
    CHECK(!spawns);
    playerInCutscene = false;
    floorAvailable = false;
    attempt();
    CHECK(!spawns);
    floorAvailable = true;
    play.roomCtx.status = 1;
    play.roomCtx.curRoom.segment = nullptr;
    attempt();
    CHECK(!spawns && !sSpawnAttempted);
    play.roomCtx.status = 0;
    attempt();
    CHECK(!spawns && !sSpawnAttempted);
    play.roomCtx.curRoom.segment = (void*)1;
    Actor authored{};
    authored.id = ACTOR_DM_STK;
    authored.params = COMBO_FOREST_SKULL_KID_PARAMS;
    play.actorCtx.actorLists[ACTORCAT_ITEMACTION].first = &authored;
    attempt();
    CHECK(!spawns);
    play.actorCtx.actorLists[ACTORCAT_ITEMACTION].first = nullptr;
    attempt();
    CHECK(spawns == 1);
    CHECK(spawned.id == ACTOR_DM_STK && spawned.params == COMBO_FOREST_SKULL_KID_PARAMS);
    CHECK(spawned.world.pos.x == 2302 && spawned.world.pos.z == -185 && spawned.world.pos.y == 158);
    SpawnForestSkullKid();
    CHECK(spawns == 1);
    CHECK(!std::memcmp(&saveBefore, &gSaveContext, sizeof(saveBefore)));
    // Solo MM uses its exploration copy, never the native story scene.
    gComboGameMode = COMBO_GAME_MM;
    play.sceneId = COMBO_SOLO_FOREST_MM_SCENE;
    gSaveContext.save.entrance = COMBO_SOLO_FOREST_MM_ENTRANCE;
    const unsigned priorSpawns = spawns;
    attempt();
    CHECK(spawns == priorSpawns + 1 && spawned.params == COMBO_FOREST_SKULL_KID_PARAMS);
    DmStk solo{};
    solo.actor.params = COMBO_FOREST_SKULL_KID_PARAMS;
    CHECK(DmStk_ComboTryInit(&solo.actor, &play) && initialized == 2 && solo.actor.update);
    solo.actor.destroy(&solo.actor, &play);
    gComboGameMode = COMBO_GAME_OOT;
    attempt();
    CHECK(spawns == priorSpawns + 1);
    puts("PASS forest Skull Kid placement: combo and solo-MM exploration copies, authored precedence, "
         "ready-room/floor/pause/transition/prologue guards");
}
