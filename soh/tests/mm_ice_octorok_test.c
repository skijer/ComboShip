// Execute production Octorok collision/update states with native structures and collider data.
#include "global.h"
#include "overlays/actors/ovl_En_Okuta/z_en_okuta.h"
#include "overlays/actors/ovl_En_Arrow/z_en_arrow.h"
#include "overlays/actors/ovl_Bg_MmIceblock/z_bg_mm_iceblock.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

void EnOkuta_ColliderCheck(EnOkuta*, PlayState*);
void EnOkuta_Update(Actor*, PlayState*);
void EnOkuta_Destroy(Actor*, PlayState*);
void EnOkuta_Freeze(EnOkuta*, PlayState*);
void EnOkuta_FrozenInIceBlock(EnOkuta*, PlayState*);
static PlayState play;
static Player player;
static EnOkuta octorok;
static EnArrow arrow;
static ColliderInfo attack;
static WaterBox water;
static int enabled, finishes, deaths, acRegistrations, colorFilters, colliderDeletions;
static BgMmIceblock block;
static CollisionHeader collision;
static Gfx model[1];
static int spawns, spawnFail, resourceFail, dynaFail, linkVeto, registrations, deletions, pins, solidCollision,
    cutscene;

#define REQUIRE(c)                                               \
    do {                                                         \
        if (!(c)) {                                              \
            fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); \
            exit(1);                                             \
        }                                                        \
    } while (0)
static void Alive(Actor* actor, PlayState* p) {
}
void EnOkuta_Appear(EnOkuta* a, PlayState* p) {
}
void EnOkuta_Hide(EnOkuta* a, PlayState* p) {
}
void EnOkuta_Shoot(EnOkuta* a, PlayState* p) {
}
void EnOkuta_WaitToAppear(EnOkuta* a, PlayState* p) {
}
void EnOkuta_WaitToShoot(EnOkuta* a, PlayState* p) {
}
void EnOkuta_WaitToDie(EnOkuta* a, PlayState* p) {
}
void EnOkuta_Die(EnOkuta* a, PlayState* p) {
}
void EnOkuta_SetupDie(EnOkuta* a) {
    ++deaths;
    a->actionFunc = EnOkuta_Die;
}
void EnOkuta_SetupWaitToDie(EnOkuta* a) {
    a->actionFunc = EnOkuta_WaitToDie;
}
void EnOkuta_SpawnRipple(EnOkuta* a, PlayState* p) {
}
int32_t CVarGetInteger(const char* key, int32_t fallback) {
    return !strcmp(key, CVAR_ENHANCEMENT("MmIceArrows")) ? enabled : fallback;
}
void Actor_SetDropFlag(Actor* a, ColliderInfo* info, s32 value) {
}
void Enemy_StartFinishingBlow(PlayState* p, Actor* a) {
    ++finishes;
}
void Actor_SetColorFilter(Actor* a, s16 color, s16 intensity, s16 buffer, s16 duration) {
    ++colorFilters;
    a->colorFilterTimer = duration;
}
void Actor_Kill(Actor* a) {
    a->update = NULL;
    a->draw = NULL;
}
void Actor_SetScale(Actor* a, f32 scale) {
    a->scale = (Vec3f){ scale, scale, scale };
}
void Actor_SetFocus(Actor* a, f32 height) {
}
void Animation_PlayLoop(SkelAnime* s, AnimationHeader* a) {
}
s32 Math_StepToF(f32* value, f32 target, f32 step) {
    if (fabsf(*value - target) <= step) {
        *value = target;
        return true;
    }
    *value += *value < target ? step : -step;
    return false;
}
void Math_ApproachF(f32* value, f32 target, f32 scale, f32 step) {
    Math_StepToF(value, target, step);
}
f32 Rand_ZeroOne(void) {
    return .5f;
}
void EffectSsEnIce_SpawnFlyingVec3f(PlayState* p, Actor* a, Vec3f* pos, s16 r, s16 g, s16 b, s16 er, s16 eg, s16 eb,
                                    s16 alpha, f32 scale) {
}
s32 Collider_DestroyCylinder(PlayState* p, ColliderCylinder* c) {
    ++colliderDeletions;
    return 0;
}
void Collider_UpdateCylinder(Actor* a, ColliderCylinder* c) {
    c->dim.pos = (Vec3s){ a->world.pos.x, a->world.pos.y, a->world.pos.z };
}
s32 CollisionCheck_SetAC(PlayState* p, CollisionCheckContext* c, Collider* col) {
    ++acRegistrations;
    return 0;
}
s32 CollisionCheck_SetAT(PlayState* p, CollisionCheckContext* c, Collider* col) {
    return 0;
}
s32 CollisionCheck_SetOC(PlayState* p, CollisionCheckContext* c, Collider* col) {
    return 0;
}
s32 WaterBox_GetSurfaceImpl(PlayState* p, CollisionContext* c, f32 x, f32 z, f32* y, WaterBox** out) {
    *y = water.ySurface;
    *out = &water;
    return true;
}
void Actor_MoveXZGravity(Actor* a) {
}
void Actor_UpdateBgCheckInfo(PlayState* p, Actor* a, f32 b, f32 c, f32 d, s32 e) {
}
s32 SurfaceType_IsIgnoredByProjectiles(CollisionContext* c, CollisionPoly* p, s32 id) {
    return false;
}
void Math_Vec3f_Copy(Vec3f* a, Vec3f* b) {
    *a = *b;
}
s32 MmIceArrows_GetOctorokBlockActorId(void) {
    return 661;
}
void* MmIceArrows_AcquireBlockResources(CollisionHeader** col, Gfx** dl) {
    *col = resourceFail ? NULL : &collision;
    *dl = resourceFail ? NULL : model;
    if (resourceFail)
        return NULL;
    ++pins;
    return &collision;
}
void MmIceArrows_ReleaseResources(void* handle) {
    if (handle)
        --pins;
}
Actor* Actor_SpawnAsChild(ActorContext* actors, Actor* parent, PlayState* p, s16 id, f32 x, f32 y, f32 z, s16 rx,
                          s16 ry, s16 rz, s16 params) {
    ++spawns;
    REQUIRE(id == 661);
    if (spawnFail)
        return NULL;
    Actor* actor = &block.dyna.actor;
    actor->id = id;
    actor->update = BgMmIceblock_Update;
    actor->home.pos = actor->world.pos = (Vec3f){ x, y, z };
    BgMmIceblock_Init(actor, p);
    // Native Actor_SpawnAsChild links only after Actor_Init and honors a hook veto.
    if (!linkVeto) {
        parent->child = actor;
        actor->parent = parent;
    }
    return actor;
}
void DynaPolyActor_Init(DynaPolyActor* a, s32 flags) {
    REQUIRE(flags == DPM_UNK);
}
s32 DynaPoly_SetBgActor(PlayState* p, DynaCollisionContext* d, Actor* a, CollisionHeader* c) {
    REQUIRE(c == &collision);
    if (dynaFail)
        return BG_ACTOR_MAX;
    ++registrations;
    return 0;
}
void DynaPoly_DeleteBgActor(PlayState* p, DynaCollisionContext* d, s32 id) {
    REQUIRE(id == 0);
    ++deletions;
}
void func_8003EBF8(PlayState* p, DynaCollisionContext* d, s32 id) {
    solidCollision = 0;
}
void func_8003EC50(PlayState* p, DynaCollisionContext* d, s32 id) {
    solidCollision = 1;
}
s32 Collider_InitCylinder(PlayState* p, ColliderCylinder* c) {
    memset(c, 0, sizeof(*c));
    return 0;
}
s32 Collider_SetCylinder(PlayState* p, ColliderCylinder* c, Actor* a, ColliderCylinderInit* init) {
    c->base.actor = a;
    c->base.acFlags = init->base.acFlags;
    c->info.bumper.dmgFlags = init->info.bumper.dmgFlags;
    c->dim = init->dim;
    return 0;
}
void Audio_PlayActorSound2(Actor* a, u16 sound) {
}
s32 Play_InCsMode(PlayState* p) {
    return cutscene;
}
void Gfx_DrawDListOpa(PlayState* p, Gfx* dl) {
    REQUIRE(dl == model);
}

static void Setup(void) {
    memset(&play, 0, sizeof(play));
    memset(&octorok, 0, sizeof(octorok));
    memset(&arrow, 0, sizeof(arrow));
    memset(&attack, 0, sizeof(attack));
    memset(&player, 0, sizeof(player));
    memset(&block, 0, sizeof(block));
    memset(&collision, 0, sizeof(collision));
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
    enabled = 1;
    finishes = deaths = acRegistrations = colorFilters = colliderDeletions = 0;
    spawns = spawnFail = resourceFail = dynaFail = linkVeto = registrations = deletions = pins = solidCollision =
        cutscene = 0;
    water.ySurface = 0;
    collision.minBounds = (Vec3s){ -600, 0, -600 };
    collision.maxBounds = (Vec3s){ 600, 1000, 600 };
    collision.numVertices = 8;
    collision.numPolygons = 12;
    octorok.actor.id = ACTOR_EN_OKUTA;
    octorok.actor.update = Alive;
    octorok.actor.colChkInfo.health = 1;
    octorok.actor.colChkInfo.damage = 4;
    octorok.actor.colChkInfo.damageEffect = 3;
    octorok.actor.flags = ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_HOSTILE;
    octorok.actor.floorHeight = -200;
    octorok.actor.scale = (Vec3f){ .01f, .01f, .01f };
    octorok.headScale = (Vec3f){ 1.1f, 1.2f, 1.3f };
    octorok.skelAnime.jointTable = octorok.jointTable;
    octorok.collider.base.acFlags = AC_ON | AC_HIT;
    octorok.collider.base.ac = &arrow.actor;
    octorok.collider.info.acHitInfo = &attack;
    octorok.actionFunc = EnOkuta_WaitToShoot;
    arrow.actor.id = ACTOR_EN_ARROW;
    arrow.actor.params = ARROW_ICE;
    arrow.actor.update = Alive;
    attack.toucher.dmgFlags = DMG_ARROW_ICE;
}
static void TickBlock(int frames) {
    while (frames-- > 0 && block.dyna.actor.update) {
        BgMmIceblock_Update(&block.dyna.actor, &play);
    }
}
static void ExpectVanillaHit(void) {
    EnOkuta_ColliderCheck(&octorok, &play);
    REQUIRE(octorok.actor.colChkInfo.health == 0 && finishes == 1 && octorok.actionFunc == EnOkuta_Freeze &&
            spawns == 0);
    REQUIRE(octorok.timer == 80);
    for (int i = 0; i < 80; ++i) {
        EnOkuta_Freeze(&octorok, &play);
    }
    REQUIRE(deaths == 1);
}
int main(void) {
    // Native OoT would kill this enemy before dispatching its 80-frame ice reaction.
    // A validated ice-arrow block must leave health/progression untouched until thaw.
    Setup();
    EnOkuta_ColliderCheck(&octorok, &play);
    REQUIRE(octorok.actor.colChkInfo.health == 1 && finishes == 0);
    REQUIRE(spawns == 1 && octorok.actionFunc == EnOkuta_FrozenInIceBlock && octorok.actor.child == &block.dyna.actor);
    REQUIRE(block.dyna.actor.parent == &octorok.actor && !(octorok.collider.base.acFlags & AC_ON));
    REQUIRE(!(octorok.actor.flags & ACTOR_FLAG_ATTENTION_ENABLED) &&
            (octorok.actor.flags & ACTOR_FLAG_UPDATE_CULLING_DISABLED));
    EnOkuta_Update(&octorok.actor, &play);
    REQUIRE(acRegistrations == 0 && fabsf(octorok.headScale.y - 1.2f) < .001f);
    REQUIRE(block.growTimer == 20 && !solidCollision && block.lifetime == 450);
    cutscene = 1;
    TickBlock(60);
    REQUIRE(block.growTimer == 20);
    cutscene = 0;
    TickBlock(20);
    REQUIRE(block.phase == MM_ICEBLOCK_SOLID && solidCollision);
    REQUIRE(fabsf(block.dyna.actor.scale.x * 1200 - 60) < .001f && fabsf(block.dyna.actor.scale.y * 1000 - 60) < .001f);
    // Fire-arrow AC must cover the 60x60 cube's corners, not only its inscribed circle.
    REQUIRE(SQ(block.collider.dim.radius) >= 30 * 30 + 30 * 30 && block.collider.dim.height >= 60);
    BgMmIceblock_Draw(&block.dyna.actor, &play);
    TickBlock(449);
    REQUIRE(block.lifetime == 1 && block.phase == MM_ICEBLOCK_SOLID);
    TickBlock(1);
    REQUIRE(block.phase == MM_ICEBLOCK_MELTING && octorok.actor.colChkInfo.health == 1);
    TickBlock(70);
    REQUIRE(block.dyna.actor.update == NULL && pins == 1 && deletions == 0);
    EnOkuta_Update(&octorok.actor, &play);
    REQUIRE(octorok.actionFunc == EnOkuta_WaitToShoot && octorok.actor.colChkInfo.health == 1 && finishes == 0 &&
            deaths == 0);
    REQUIRE(octorok.collider.base.acFlags & AC_ON);
    REQUIRE((octorok.actor.flags & ACTOR_FLAG_ATTENTION_ENABLED) &&
            !(octorok.actor.flags & ACTOR_FLAG_UPDATE_CULLING_DISABLED));
    BgMmIceblock_Destroy(&block.dyna.actor, &play);
    REQUIRE(pins == 0 && deletions == 1);
    BgMmIceblock_Destroy(&block.dyna.actor, &play);
    REQUIRE(deletions == 1 && colliderDeletions == 1);

    Setup();
    enabled = 0;
    ExpectVanillaHit();
    Setup();
    arrow.actor.id = ACTOR_PLAYER;
    ExpectVanillaHit();
    const s16 nonVanillaArrows[] = { ARROW_NORMAL, ARROW_SW97_ICE, ARROW_SEED_ICE };
    for (int i = 0; i < ARRAY_COUNT(nonVanillaArrows); ++i) {
        Setup();
        arrow.actor.params = nonVanillaArrows[i];
        ExpectVanillaHit();
    }
    Setup();
    attack.toucher.dmgFlags = DMG_SLASH_KOKIRI;
    ExpectVanillaHit();
    Setup();
    octorok.collider.info.acHitInfo = NULL;
    ExpectVanillaHit();
    Setup();
    octorok.collider.base.ac = NULL;
    ExpectVanillaHit();
    Setup();
    octorok.collider.base.acFlags = AC_HIT;
    ExpectVanillaHit();
    Setup();
    octorok.actor.colChkInfo.damage = octorok.actor.colChkInfo.damageEffect = 0;
    EnOkuta_ColliderCheck(&octorok, &play);
    REQUIRE(spawns == 0 && finishes == 0 && octorok.actor.colChkInfo.health == 1);
    Setup();
    octorok.actor.colChkInfo.health = 0;
    EnOkuta_ColliderCheck(&octorok, &play);
    REQUIRE(spawns == 0 && octorok.actor.colChkInfo.health == 0);
    Setup();
    Actor other = { 0 };
    octorok.actor.child = &other;
    ExpectVanillaHit();
    REQUIRE(octorok.actor.child == &other);

    Setup();
    spawnFail = 1;
    EnOkuta_ColliderCheck(&octorok, &play);
    REQUIRE(octorok.actor.colChkInfo.health == 0 && finishes == 1 && octorok.actionFunc == EnOkuta_Freeze &&
            !octorok.actor.child);
    Setup();
    resourceFail = 1;
    EnOkuta_ColliderCheck(&octorok, &play);
    REQUIRE(octorok.actor.colChkInfo.health == 0 && finishes == 1 && !octorok.actor.child && !block.dyna.actor.parent);
    BgMmIceblock_Destroy(&block.dyna.actor, &play);
    REQUIRE(pins == 0 && deletions == 0 && colliderDeletions == 0);
    Setup();
    dynaFail = 1;
    EnOkuta_ColliderCheck(&octorok, &play);
    REQUIRE(octorok.actor.colChkInfo.health == 0 && finishes == 1 && !octorok.actor.child);
    BgMmIceblock_Destroy(&block.dyna.actor, &play);
    REQUIRE(pins == 0 && deletions == 0 && colliderDeletions == 0);
    Setup();
    linkVeto = 1;
    EnOkuta_ColliderCheck(&octorok, &play);
    REQUIRE(octorok.actor.colChkInfo.health == 0 && finishes == 1 && !octorok.actor.child &&
            block.dyna.actor.update == NULL);
    BgMmIceblock_Destroy(&block.dyna.actor, &play);
    REQUIRE(pins == 0 && deletions == 1);

    Setup();
    EnOkuta_ColliderCheck(&octorok, &play);
    TickBlock(20);
    ColliderInfo fire = { 0 };
    fire.toucher.dmgFlags = DMG_ARROW_FIRE;
    block.collider.base.acFlags |= AC_HIT;
    block.collider.info.acHitInfo = &fire;
    TickBlock(1);
    REQUIRE(block.phase == MM_ICEBLOCK_MELTING && !(block.collider.base.acFlags & AC_ON));
    TickBlock(70);
    EnOkuta_Update(&octorok.actor, &play);
    REQUIRE(octorok.actor.colChkInfo.health == 1 && !finishes && !deaths && octorok.actionFunc == EnOkuta_WaitToShoot);
    BgMmIceblock_Destroy(&block.dyna.actor, &play);
    Setup();
    EnOkuta_ColliderCheck(&octorok, &play);
    TickBlock(20);
    block.collider.base.acFlags |= AC_HIT;
    block.collider.info.acHitInfo = NULL;
    TickBlock(1);
    REQUIRE(block.phase == MM_ICEBLOCK_SOLID);

    Setup();
    EnOkuta_ColliderCheck(&octorok, &play);
    EnOkuta_Destroy(&octorok.actor, &play);
    REQUIRE(!octorok.actor.child && !block.dyna.actor.parent && !block.dyna.actor.update);
    BgMmIceblock_Destroy(&block.dyna.actor, &play);
    REQUIRE(pins == 0 && deletions == 1 && colliderDeletions == 2);
    Setup();
    EnOkuta_ColliderCheck(&octorok, &play);
    BgMmIceblock_Destroy(&block.dyna.actor, &play);
    REQUIRE(!octorok.actor.child);
    EnOkuta_Update(&octorok.actor, &play);
    REQUIRE(octorok.actionFunc == EnOkuta_WaitToShoot && octorok.actor.colChkInfo.health == 1);
    Setup();
    EnOkuta_ColliderCheck(&octorok, &play);
    octorok.actor.update = NULL;
    TickBlock(1);
    REQUIRE(block.dyna.actor.update == NULL);
    BgMmIceblock_Destroy(&block.dyna.actor, &play);
    Setup();
    EnOkuta_ColliderCheck(&octorok, &play);
    enabled = 0;
    TickBlock(1);
    EnOkuta_Update(&octorok.actor, &play);
    REQUIRE(octorok.actionFunc == EnOkuta_WaitToShoot && octorok.actor.colChkInfo.health == 1);
    BgMmIceblock_Destroy(&block.dyna.actor, &play);
    Setup();
    octorok.actor.flags |= ACTOR_FLAG_UPDATE_CULLING_DISABLED;
    EnOkuta_ColliderCheck(&octorok, &play);
    water.ySurface = 20;
    EnOkuta_Update(&octorok.actor, &play);
    TickBlock(1);
    REQUIRE(octorok.actor.world.pos.y == 20 && block.waterHeight == 20);
    enabled = 0;
    EnOkuta_Update(&octorok.actor, &play);
    REQUIRE(octorok.actor.flags & ACTOR_FLAG_UPDATE_CULLING_DISABLED);
    BgMmIceblock_Destroy(&block.dyna.actor, &play);
    puts("MM Octorok frozen-block production states: PASS");
}
