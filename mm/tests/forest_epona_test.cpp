#include "overlays/actors/ovl_En_Horse/z_en_horse.h"
#include "2s2h/Enhancements/Gameplay/ComboOpeningForest.h"
#include "entrances/SoloForest.h"
#include "2s2h/Enhancements/Graphics/EponaCosmetics.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#define CHECK(x)                                                      \
    do {                                                              \
        if (!(x)) {                                                   \
            std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); \
            std::exit(1);                                             \
        }                                                             \
    } while (0)
static unsigned initialized, freed, killed, destroyedColliders, overlaps, draws, cosmetics, cosmeticsEnded;
void FrameInterpolation_RecordOpenChild(const void*, int) {
}
void FrameInterpolation_RecordCloseChild() {
}
extern "C" {
SaveContext gSaveContext{};
int gComboGameMode;
bool gHorsePlayedEponasSong = true;
void gSPSegment(void* command, int segment, uintptr_t target) {
    __gSPSegment(static_cast<Gfx*>(command), segment, target);
}
void Actor_Kill(Actor*) {
    ++killed;
}
void Actor_SetScale(Actor* a, f32 s) {
    a->scale = { s, s, s };
}
void ActorShape_Init(ActorShape*, f32, ActorShadowFunc, f32) {
}
void ActorShadow_DrawCircle(Actor*, Lights*, PlayState*) {
}
void Actor_SetFocus(Actor* a, f32 y) {
    a->focus.pos = a->world.pos;
    a->focus.pos.y += y;
}
f32 BgCheck_EntityRaycastFloor5(CollisionContext*, CollisionPoly** poly, s32*, Actor*, Vec3f*) {
    static CollisionPoly floor;
    *poly = &floor;
    return 12.0f;
}
f32 Math_SinS(s16 angle) {
    return std::sin(angle * (6.28318530718f / 65536));
}
f32 Math_CosS(s16 angle) {
    return std::cos(angle * (6.28318530718f / 65536));
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
    ++destroyedColliders;
    return 1;
}
void Collider_UpdateCylinder(Actor* a, ColliderCylinder* c) {
    c->dim.pos = { (s16)a->world.pos.x, (s16)a->world.pos.y, (s16)a->world.pos.z };
}
s32 CollisionCheck_SetOC(PlayState*, CollisionCheckContext*, Collider* collider) {
    CHECK(collider->atFlags == AT_NONE && collider->acFlags == AC_NONE && (collider->ocFlags1 & OC1_ON));
    ++overlaps;
    return 1;
}
void Skin_Setup(Skin* skin) {
    *skin = {};
}
void Skin_Init(GameState*, Skin* skin, SkeletonHeader* skeleton, AnimationHeader* animation) {
    CHECK(std::strstr(reinterpret_cast<const char*>(skeleton), "gEponaSkel"));
    CHECK(std::strstr(reinterpret_cast<const char*>(animation), "gEponaIdleAnim"));
    ++initialized;
    skin->skelAnime.endFrame = 69;
}
void Skin_Free(GameState*, Skin*) {
    ++freed;
}
s32 SkelAnime_Update(SkelAnime* a) {
    a->curFrame = std::fmod(a->curFrame + a->playSpeed, 70);
    return 0;
}
void Graph_OpenDisps(Gfx**, Gfx*, GraphicsContext*, const char*, s32) {
}
void Graph_CloseDisps(Gfx**, Gfx*, GraphicsContext*, const char*, s32) {
}
void Gfx_SetupDL25_Opa(GraphicsContext*) {
}
int MMEponaCosmetics_BeginDraw(PlayState*, Skin*) {
    ++cosmetics;
    return 1;
}
void MMEponaCosmetics_EndDraw(PlayState*, Skin*) {
    ++cosmeticsEnded;
}
void func_80138258(Actor*, PlayState*, Skin*, SkinPostDraw post, SkinOverrideLimbDraw override, s32 translate) {
    CHECK(!post && override && translate);
    ++draws;
}
}
#include "overlays/actors/ovl_En_Horse/z_en_horse_combo.h"

int main() {
    static PlayState play;
    EnHorse ordinary{}, horse{};
    ordinary.actor.params = 15;
    EnHorse untouched = ordinary;
    CHECK(!EnHorse_ComboTryInit(&ordinary.actor, &play));
    CHECK(std::memcmp(&ordinary, &untouched, sizeof(ordinary)) == 0);
    horse.actor.params = COMBO_FOREST_EPONA_PARAMS;
    CHECK(EnHorse_ComboTryInit(&horse.actor, &play));
    CHECK(killed == 1 && initialized == 0 && !horse.actor.destroy);
    play.sceneId = SCENE_LOST_WOODS;
    gComboGameMode = COMBO_GAME_BOTH;
    gSaveContext.fileNum = 0;
    gSaveContext.gameMode = GAMEMODE_NORMAL;
    gSaveContext.save.entrance = ENTRANCE(LOST_WOODS, 0);
    horse.actor.world.pos = { 10, 20, 30 };
    horse.actor.world.rot = { 100, 16384, 200 };
    SaveContext before = gSaveContext;
    CHECK(EnHorse_ComboTryInit(&horse.actor, &play));
    CHECK(initialized == 1 && horse.type == HORSE_TYPE_2 && horse.actor.scale.x == 0.00648f);
    CHECK(horse.actor.update && horse.actor.draw && horse.actor.destroy);
    CHECK(horse.stateFlags & ENHORSE_UNRIDEABLE);
    CHECK(horse.actor.world.rot.x == 0 && horse.actor.world.rot.z == 0 && horse.actor.shape.rot.y == 16384);
    for (int tick = 0; tick < 50; ++tick)
        horse.actor.update(&horse.actor, &play);
    CHECK(overlaps == 100 && horse.blinkTimer == 1);
    CHECK(horse.actor.world.pos.x == 10 && horse.actor.world.pos.y == 20 && horse.actor.world.pos.z == 30);
    CHECK(horse.actor.speed == 0 && horse.actor.gravity == 0 && !horse.playerControlled);
    CHECK(horse.skin.skelAnime.curFrame > 0 && horse.skin.skelAnime.playSpeed == 2.0f / 3.0f);
    CHECK(gHorsePlayedEponasSong && std::memcmp(&before, &gSaveContext, sizeof(before)) == 0);
    static GraphicsContext gfx;
    Gfx commands[16]{};
    gfx.polyOpa.p = commands;
    play.state.gfxCtx = &gfx;
    for (int tick = 0; tick < 3; ++tick) {
        CHECK(horse.blinkTimer == tick + 1);
        EnHorse_ComboOverride(&horse.actor, &play, 13, &horse.skin);
        horse.actor.update(&horse.actor, &play);
    }
    CHECK(horse.blinkTimer == 0 && gfx.polyOpa.p == commands + 3);
    horse.actor.draw(&horse.actor, &play);
    CHECK(draws == 1 && cosmetics == 1 && cosmeticsEnded == 1);
    horse.actor.destroy(&horse.actor, &play);
    CHECK(freed == 1 && destroyedColliders == 2);
    gComboGameMode = COMBO_GAME_MM;
    play.sceneId = COMBO_SOLO_FOREST_MM_SCENE;
    gSaveContext.save.entrance = COMBO_SOLO_FOREST_MM_ENTRANCE;
    EnHorse solo{};
    solo.actor.params = COMBO_FOREST_EPONA_PARAMS;
    CHECK(EnHorse_ComboTryInit(&solo.actor, &play) && initialized == 2 && solo.actor.update);
    solo.actor.destroy(&solo.actor, &play);
    puts("PASS forest Epona: reserved init bypass, ordinary params untouched, native idle/blink/cosmetics, stationary "
         "colliders, cleanup, no mount/song/save side effects");
}
