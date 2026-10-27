#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
typedef float f32;
typedef int s32;
typedef struct {
    int id;
} CollisionPoly;
typedef struct {
    float x, y, z;
} Vec3f;
typedef struct {
    short x, y, z;
} Vec3s;
typedef struct {
    Vec3f pos;
    Vec3s rot;
} PosRot;
typedef struct {
    Vec3s rot;
} Shape;
typedef struct {
    int mass;
} ColInfo;
typedef struct {
    PosRot world, home;
    Shape shape;
    ColInfo colChkInfo;
    unsigned flags;
    float speedXZ;
    Vec3f velocity;
    CollisionPoly* floorPoly;
    float floorHeight;
    int floorBgId;
} Actor;
typedef struct {
    struct {
        int radius, height;
    } dim;
    int base;
} Collider;
typedef struct {
    int skelAnime;
} Skin;
typedef struct {
    Actor actor;
    int type, stateFlags, playerControlled;
    Collider cyl1;
    Skin skin;
    int blinkTimer;
} EnHorse;
typedef struct {
    int colChkCtx, colCtx;
} PlayState;
enum {
    HORSE_EPONA,
    ENHORSE_UNRIDEABLE = 8,
    ACTOR_FLAG_UPDATE_CULLING_DISABLED = 16,
    MASS_IMMOVABLE = 255,
    BGCHECK_SCENE = 50
};
static int sCylinderInit2, gEponaSkel, gEponaIdleAnim;
static int initColliders, initSkins, updates, oc, randomBlink;
static void ActorShadow_DrawCircle(void) {
}
static void Actor_SetScale(Actor* a, float scale) {
    (void)a;
    assert(scale == 0.01f);
}
static void ActorShape_Init(Shape* s, float y, void (*draw)(void), float size) {
    (void)s;
    assert(y == 0 && draw == ActorShadow_DrawCircle && size == 40.0f);
}
static void Collider_InitCylinder(PlayState* p, Collider* c) {
    (void)p;
    (void)c;
    ++initColliders;
}
static void Collider_SetCylinder(PlayState* p, Collider* c, Actor* a, int* data) {
    (void)p;
    (void)c;
    (void)a;
    (void)data;
}
static void Skin_Init(PlayState* p, Skin* s, int* skel, int* anim) {
    (void)p;
    (void)s;
    assert(skel == &gEponaSkel && anim == &gEponaIdleAnim);
    ++initSkins;
}
static void Animation_PlayLoop(int* s, int* a) {
    (void)s;
    assert(a == &gEponaIdleAnim);
}
static void SkelAnime_Update(int* s) {
    (void)s;
    ++updates;
}
static float Rand_ZeroOne(void) {
    return randomBlink ? 0.0f : 1.0f;
}
static void Collider_UpdateCylinder(Actor* a, Collider* c) {
    (void)a;
    (void)c;
}
static void CollisionCheck_SetOC(PlayState* p, int* ctx, int* base) {
    (void)p;
    (void)ctx;
    (void)base;
    ++oc;
}
static CollisionPoly floorPoly;
static float BgCheck_EntityRaycastFloor5(PlayState* play, int* colCtx, CollisionPoly** poly, s32* bgId, Actor* actor,
                                         Vec3f* probe) {
    assert(colCtx == &play->colCtx);
    assert(probe->x == actor->world.pos.x && probe->z == actor->world.pos.z && probe->y == actor->world.pos.y + 10);
    *poly = &floorPoly;
    *bgId = 7;
    return 12.5f;
}
/* PRODUCTION_HOST */
int main(void) {
    PlayState play = { 0 };
    EnHorse horse = { 0 };
    horse.actor.home.pos = (Vec3f){ -825, 25, -273 };
    horse.actor.home.rot.y = 0x3000;
    horse.actor.world = horse.actor.home;
    EnHorse_ForestInit(&horse, &play);
    assert(horse.actor.floorPoly == &floorPoly && horse.actor.floorBgId == 7 && horse.actor.floorHeight == 12.5f);
    assert(horse.actor.world.pos.y == 25 && horse.actor.home.pos.y == 25);
    assert(initColliders == 1 && initSkins == 1 && !horse.playerControlled);
    assert(horse.stateFlags == ENHORSE_UNRIDEABLE && horse.actor.colChkInfo.mass == MASS_IMMOVABLE);
    assert(horse.cyl1.dim.radius == 35 && horse.cyl1.dim.height == 80);
    horse.actor.world.pos.x = 500;
    horse.actor.speedXZ = 20;
    horse.actor.velocity.y = 90;
    randomBlink = 1;
    EnHorse_ForestUpdate(&horse, &play);
    assert(horse.actor.world.pos.x == -825 && horse.actor.world.pos.y == 25);
    assert(horse.actor.world.rot.y == 0x3000 && horse.actor.shape.rot.y == 0x3000);
    assert(horse.actor.speedXZ == 0 && horse.actor.velocity.y == 0 && horse.blinkTimer == 1);
    randomBlink = 0;
    for (int i = 0; i < 3; ++i)
        EnHorse_ForestUpdate(&horse, &play);
    assert(horse.blinkTimer == 0 && updates == 4 && oc == 4);
}
