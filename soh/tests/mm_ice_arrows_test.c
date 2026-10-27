// Production arrow dispatch and platform lifecycles, using real game headers.
// Rendering, archive access, allocation and collision registration are engine boundaries.
#include "global.h"
#include "overlays/actors/ovl_En_Arrow/z_en_arrow.h"
#include "overlays/actors/ovl_Bg_MmIcefloe/z_bg_mm_icefloe.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void EnArrow_Update(Actor*, PlayState*);
void EnArrow_Fly(EnArrow*, PlayState*);

static PlayState play;
static Player player;
static EnArrow arrow;
static Actor glow;
static BgMmIcefloe floes[12];
static int enabled, spawns, moves, waterPresent, waterQueries, cutscene;
static int spawnFail, resourcesOk, dynaFail, registrations, deletions, resourcePins, releases, drawn;
static int births;
static WaterBox waterBox;
static CollisionHeader collision;
static Gfx model[1];
static Vec3f spawnPosition;
static GameInfo gameInfo;
GameInfo* gGameInfo = &gameInfo;
s16 gSw97ActorId_ArrowFire = 600, gSw97ActorId_ArrowIce = 601, gSw97ActorId_ArrowLight = 602;
s16 gSw97ActorId_ArrowDark = 603, gSw97ActorId_ArrowSoul = 604, gSw97ActorId_ArrowWind = 605;

#define REQUIRE(c)                                               \
    do {                                                         \
        if (!(c)) {                                              \
            fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); \
            exit(1);                                             \
        }                                                        \
    } while (0)
static void Alive(Actor* actor, PlayState* context) {
}
void lusprintf(const char* file, int32_t line, int32_t level, const char* fmt, ...) {
}
int32_t CVarGetInteger(const char* key, int32_t fallback) {
    return !strcmp(key, CVAR_ENHANCEMENT("MmIceArrows")) ? enabled : fallback;
}
void Actor_Kill(Actor* actor) {
    actor->update = NULL;
    actor->draw = NULL;
}
void Actor_MoveXZGravity(Actor* actor) {
    ++moves;
}
s32 Player_InBlockingCsMode(PlayState* context, Player* target) {
    return false;
}
Actor* Actor_Spawn(ActorContext* actors, PlayState* context, s16 id, f32 x, f32 y, f32 z, s16 rx, s16 ry, s16 rz,
                   s16 params) {
    ++spawns;
    spawnPosition = (Vec3f){ x, y, z };
    if (spawnFail)
        return NULL;
    REQUIRE(id == 660 && births < ARRAY_COUNT(floes));
    BgMmIcefloe* floe = &floes[births++];
    Actor* actor = &floe->dyna.actor;
    actor->id = id;
    actor->params = params;
    actor->update = BgMmIcefloe_Update;
    actor->home.pos = actor->world.pos = spawnPosition;
    actor->next = actors->actorLists[ACTORCAT_BG].head;
    actors->actorLists[ACTORCAT_BG].head = actor;
    BgMmIcefloe_Init(actor, context);
    return actor;
}
Actor* Actor_SpawnAsChild(ActorContext* actors, Actor* parent, PlayState* context, s16 id, f32 x, f32 y, f32 z, s16 rx,
                          s16 ry, s16 rz, s16 params) {
    parent->child = &glow;
    return &glow;
}
void Harpoon_NotifyVfxSpawn(Actor* actor, s32 kind, s32 attached) {
}
void func_8002836C(PlayState* p, Vec3f* a, Vec3f* b, Vec3f* c, Color_RGBA8* d, Color_RGBA8* e, s16 f, s16 g, s16 h) {
}
void Math_Vec3f_Copy(Vec3f* dest, Vec3f* src) {
    *dest = *src;
}
void Math_Vec3f_Diff(Vec3f* a, Vec3f* b, Vec3f* dest) {
    *dest = (Vec3f){ a->x - b->x, a->y - b->y, a->z - b->z };
}
void Math_Vec3f_Sum(Vec3f* a, Vec3f* b, Vec3f* dest) {
    *dest = (Vec3f){ a->x + b->x, a->y + b->y, a->z + b->z };
}
void Math_Vec3f_Scale(Vec3f* value, f32 scale) {
    value->x *= scale;
    value->y *= scale;
    value->z *= scale;
}
f32 Math3D_Vec3fMagnitudeSq(Vec3f* value) {
    return SQ(value->x) + SQ(value->y) + SQ(value->z);
}
f32 Rand_ZeroOne(void) {
    return .5f;
}
s16 Math_Atan2S(f32 x, f32 y) {
    return 0;
}
void Animation_PlayOnce(SkelAnime* skel, AnimationHeader* anim) {
}
s32 SkelAnime_Update(SkelAnime* skel) {
    return 0;
}
void Audio_PlayActorSound2(Actor* actor, u16 sfx) {
}
s32 BgCheck_ProjectileLineTest(CollisionContext* c, Vec3f* a, Vec3f* b, Vec3f* hit, CollisionPoly** poly, s32 wall,
                               s32 floor, s32 ceil, s32 one, s32* bg) {
    return false;
}
s32 BgCheck_EntityLineTest1(CollisionContext* c, Vec3f* a, Vec3f* b, Vec3f* hit, CollisionPoly** poly, s32 wall,
                            s32 floor, s32 ceil, s32 one, s32* bg) {
    return false;
}
s32 func_8002F9EC(PlayState* context, Actor* actor, CollisionPoly* poly, s32 bg, Vec3f* hit) {
    return 0;
}
void EffectSsStone1_Spawn(PlayState* context, Vec3f* pos, s32 reg) {
}
void EffectSsHitMark_SpawnCustomScale(PlayState* context, s32 type, s16 scale, Vec3f* pos) {
}
void SoundSource_PlaySfxAtFixedWorldPos(PlayState* context, Vec3f* pos, s32 duration, u16 sfx) {
}
s32 MmIceArrows_GetActorId(void) {
    return 660;
}
void* MmIceArrows_AcquireResources(CollisionHeader** col, Gfx** dl) {
    *col = resourcesOk ? &collision : NULL;
    *dl = resourcesOk ? model : NULL;
    if (resourcesOk) {
        ++resourcePins;
        return &collision;
    }
    return NULL;
}
void MmIceArrows_ReleaseResources(void* owner) {
    if (owner) {
        --resourcePins;
        ++releases;
    }
}
void Actor_SetScale(Actor* actor, f32 scale) {
    actor->scale = (Vec3f){ scale, scale, scale };
}
void DynaPolyActor_Init(DynaPolyActor* actor, s32 flags) {
    REQUIRE(flags == DPM_UNK);
}
s32 DynaPoly_SetBgActor(PlayState* context, DynaCollisionContext* dyna, Actor* actor, CollisionHeader* col) {
    REQUIRE(col == &collision);
    if (dynaFail)
        return BG_ACTOR_MAX;
    return registrations++;
}
void DynaPoly_DeleteBgActor(PlayState* context, DynaCollisionContext* dyna, s32 id) {
    REQUIRE(id >= 0 && id < registrations);
    ++deletions;
}
s32 WaterBox_GetSurface1(PlayState* context, CollisionContext* col, f32 x, f32 z, f32* y, WaterBox** out) {
    ++waterQueries;
    if (!waterPresent)
        return false;
    *out = &waterBox;
    *y = waterBox.ySurface;
    return true;
}
s32 Play_InCsMode(PlayState* context) {
    return cutscene;
}
void EffectSsGSplash_Spawn(PlayState* context, Vec3f* pos, Color_RGBA8* prim, Color_RGBA8* env, s16 type, s16 scale) {
}
void EffectSsGRipple_Spawn(PlayState* context, Vec3f* pos, s16 radius, s16 maxRadius, s16 life) {
}
void EffectSsIceSmoke_Spawn(PlayState* context, Vec3f* pos, Vec3f* vel, Vec3f* accel, s16 scale) {
}
void Gfx_DrawDListOpa(PlayState* context, Gfx* dl) {
    REQUIRE(dl == model);
    ++drawn;
}

static void Setup(void) {
    memset(&play, 0, sizeof(play));
    memset(&player, 0, sizeof(player));
    memset(&arrow, 0, sizeof(arrow));
    memset(&glow, 0, sizeof(glow));
    memset(floes, 0, sizeof(floes));
    memset(&collision, 0, sizeof(collision));
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
    arrow.actor.id = ACTOR_EN_ARROW;
    arrow.actor.params = ARROW_ICE;
    arrow.actor.update = Alive;
    arrow.actor.child = &glow;
    glow.update = Alive;
    arrow.actor.world.pos = (Vec3f){ 30, -10, 60 };
    arrow.unk_210 = (Vec3f){ 10, 10, 20 };
    arrow.actionFunc = EnArrow_Fly;
    arrow.timer = 10;
    enabled = waterPresent = resourcesOk = 1;
    spawns = moves = waterQueries = births = cutscene = spawnFail = dynaFail = 0;
    registrations = deletions = resourcePins = releases = drawn = 0;
    waterBox = (WaterBox){ -1000, 0, -1000, 2000, 2000, 0 };
    // Independently read from native oot.o2r; native model and collision have identical extents.
    collision.minBounds = (Vec3s){ -1473, -1985, -1696 };
    collision.maxBounds = (Vec3s){ 1473, 150, 1696 };
    collision.numVertices = 13;
    collision.numPolygons = 22;
}

static void Tick(BgMmIcefloe* floe, int count) {
    while (count-- > 0) {
        ++play.gameplayFrames;
        BgMmIcefloe_Update(&floe->dyna.actor, &play);
    }
}
static void Reshoot(void) {
    arrow.actor.update = Alive;
    arrow.actor.bgCheckFlags = 0;
    arrow.actionFunc = EnArrow_Fly;
    arrow.timer = 10;
    arrow.hitFlags = arrow.touchedPoly = 0;
    arrow.collider.base.atFlags = 0;
    arrow.actor.child = &glow;
    glow.update = Alive;
    arrow.actor.world.pos = (Vec3f){ 30, -10, 60 };
    arrow.unk_210 = (Vec3f){ 10, 10, 20 };
    EnArrow_Update(&arrow.actor, &play);
}

int main(void) {
    // A missing first-water-entry dispatch must fail before the projectile moves again.
    Setup();
    EnArrow_Update(&arrow.actor, &play);
    REQUIRE(spawns == 1);
    REQUIRE(arrow.actor.update == NULL);
    REQUIRE(glow.update == NULL);
    REQUIRE(moves == 0);
    REQUIRE(fabsf(spawnPosition.x - 20) < .001f && fabsf(spawnPosition.y) < .001f &&
            fabsf(spawnPosition.z - 40) < .001f);
    BgMmIcefloe* first = &floes[0];
    REQUIRE(first->timer == 20 && first->phase == MM_ICEFLOE_GROWING);
    Tick(first, 19);
    REQUIRE(first->phase == MM_ICEFLOE_GROWING);
    Tick(first, 1);
    REQUIRE(first->phase == MM_ICEFLOE_FLOATING && first->timer == 300);
    REQUIRE(fabsf(first->dyna.actor.scale.z * 3392 - 83.2f) < .001f);
    REQUIRE(fabsf(first->dyna.actor.scale.x * 2946 - 72.26038f) < .01f);
    REQUIRE(fabsf(first->dyna.actor.world.pos.y + first->dyna.actor.scale.y * 150 - 10) < .001f);
    BgMmIcefloe_Draw(&first->dyna.actor, &play);
    REQUIRE(drawn == 1);
    cutscene = 1;
    Tick(first, 90);
    REQUIRE(first->timer == 300);
    cutscene = 0;
    Tick(first, 15);
    REQUIRE(fabsf(first->dyna.actor.world.pos.y + first->dyna.actor.scale.y * 150 - 7) < .001f);
    Tick(first, 284);
    REQUIRE(first->timer == 1 && first->phase == MM_ICEFLOE_FLOATING);
    Tick(first, 1);
    REQUIRE(first->timer == 50 && first->phase == MM_ICEFLOE_MELTING);
    Tick(first, 49);
    REQUIRE(first->dyna.actor.update != NULL);
    Tick(first, 1);
    REQUIRE(first->dyna.actor.update == NULL);
    REQUIRE(resourcePins == 1 && deletions == 0);
    BgMmIcefloe_Destroy(&first->dyna.actor, &play);
    REQUIRE(resourcePins == 0 && deletions == 1 && releases == 1);
    BgMmIcefloe_Destroy(&first->dyna.actor, &play);
    REQUIRE(deletions == 1 && releases == 1);

    // Unchecked must not even query water or alter the vanilla projectile/glow lifecycle.
    Setup();
    enabled = 0;
    EnArrow_Update(&arrow.actor, &play);
    REQUIRE(spawns == 0 && moves == 1 && waterQueries == 0 && glow.update != NULL);
    const s16 otherTypes[] = { ARROW_NORMAL, ARROW_FIRE, ARROW_LIGHT, ARROW_SEED_ICE, ARROW_SW97_ICE };
    for (int i = 0; i < ARRAY_COUNT(otherTypes); ++i) {
        Setup();
        arrow.actor.params = otherTypes[i];
        EnArrow_Update(&arrow.actor, &play);
        REQUIRE(spawns == 0 && moves == 1 && waterQueries == 0);
    }
    Setup();
    arrow.actor.world.pos.y = 5;
    EnArrow_Update(&arrow.actor, &play);
    REQUIRE(spawns == 0);
    Setup();
    arrow.unk_210.y = -10;
    EnArrow_Update(&arrow.actor, &play);
    REQUIRE(spawns == 0);
    REQUIRE(arrow.actor.bgCheckFlags & BGCHECKFLAG_WATER);
    Setup();
    arrow.actor.bgCheckFlags = BGCHECKFLAG_WATER;
    EnArrow_Update(&arrow.actor, &play);
    REQUIRE(spawns == 0 && waterQueries == 0);
    Setup();
    arrow.unk_210.x = 5000;
    EnArrow_Update(&arrow.actor, &play);
    REQUIRE(spawns == 0);
    Setup();
    arrow.touchedPoly = 1;
    EnArrow_Update(&arrow.actor, &play);
    REQUIRE(spawns == 0 && waterQueries == 0);
    Setup();
    arrow.timer = 0;
    arrow.collider.base.atFlags = AT_HIT;
    // Contacted actors and geometry keep their native impact processing.
    // Timer1 exits Fly before inspecting the fixture's absent collider hit-info.
    arrow.timer = 1;
    EnArrow_Update(&arrow.actor, &play);
    REQUIRE(spawns == 0 && waterQueries == 0);

    Setup();
    spawnFail = 1;
    EnArrow_Update(&arrow.actor, &play);
    REQUIRE(spawns == 1 && arrow.actor.update != NULL && glow.update != NULL);
    Setup();
    resourcesOk = 0;
    EnArrow_Update(&arrow.actor, &play);
    REQUIRE(arrow.actor.update != NULL && registrations == 0);
    BgMmIcefloe_Destroy(&floes[0].dyna.actor, &play);
    REQUIRE(releases == 0 && deletions == 0);
    Setup();
    dynaFail = 1;
    EnArrow_Update(&arrow.actor, &play);
    REQUIRE(arrow.actor.update != NULL && resourcePins == 1);
    BgMmIcefloe_Destroy(&floes[0].dyna.actor, &play);
    REQUIRE(resourcePins == 0 && deletions == 0);

    Setup();
    Reshoot();
    Reshoot();
    Reshoot();
    REQUIRE(births == 3 && floes[0].phase == MM_ICEFLOE_GROWING);
    Reshoot();
    REQUIRE(births == 4 && floes[0].phase == MM_ICEFLOE_MELTING && floes[0].timer == 50);
    REQUIRE(floes[1].phase == MM_ICEFLOE_GROWING && floes[3].phase == MM_ICEFLOE_GROWING);
    Reshoot();
    REQUIRE(floes[1].phase == MM_ICEFLOE_MELTING && floes[2].phase == MM_ICEFLOE_GROWING);
    for (int i = 0; i < births; ++i) {
        BgMmIcefloe_Destroy(&floes[i].dyna.actor, &play);
    }
    REQUIRE(resourcePins == 0 && deletions == registrations);
    // Scene teardown leaves no static actor pointers which a new PlayState can revisit.
    Setup();
    Reshoot();
    REQUIRE(floes[0].phase == MM_ICEFLOE_GROWING);
    Tick(&floes[0], 20);
    waterPresent = 0;
    Tick(&floes[0], 1);
    REQUIRE(floes[0].phase == MM_ICEFLOE_MELTING && floes[0].timer == 50);
    enabled = 0;
    Tick(&floes[0], 1);
    REQUIRE(floes[0].dyna.actor.update == NULL);
    BgMmIcefloe_Destroy(&floes[0].dyna.actor, &play);
    REQUIRE(resourcePins == 0);
    puts("MM ice arrows production behavior: PASS");
    return 0;
}
