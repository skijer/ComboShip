// Exercise the complete production actor with real engine structs and GBI
// commands. Resource submission/actor allocation fixtures are not visual proof.
#include "global.h"
#include "2s2h/BenPort.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REQUIRE(condition)                                               \
    do {                                                                 \
        if (!(condition)) {                                              \
            fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); \
            exit(1);                                                     \
        }                                                                \
    } while (0)

#undef OPEN_DISPS_PORT_HELPERS
#undef CLOSE_DISPS_PORT_HELPERS
#define OPEN_DISPS_PORT_HELPERS(gfxCtx)
#define CLOSE_DISPS_PORT_HELPERS(gfxCtx)
#include "overlays/actors/ovl_En_Wood02/z_en_wood02.c"
#define sCylinderInit sBranchingTreeCylinderInit
#undef FLAGS
#include "overlays/actors/ovl_Obj_Tree/z_obj_tree.c"
#undef sCylinderInit

SaveContext gSaveContext;
static PlayState play;
static Player player;
static GraphicsContext gfx;
static Gfx opaque[64], translucent[64];
static Mtx matrix;
static EnWood02 spawned[512];
static int spawnCount, spawnFailure, drops, swings, reflections, collisions;
static int gameplayRandomCalls;
static int liveSeason, eligible, alt;
static const char* altList;

// The cached API deliberately remains autumn: foliage must consult the current
// play/selection API even before the environment's next update.
int MMWeather_Season(void) {
    return SEASON_AUTUMN;
}
int MMWeather_SeasonForPlay(const PlayState* current) {
    REQUIRE(current == &play);
    return eligible ? liveSeason : -1;
}
float MMWeather_RandomFloat(void) {
    return 0.75f;
}
int32_t CVarGetInteger(const char* key, int32_t fallback) {
    (void)key;
    return fallback;
}
bool ResourceMgr_IsAltAssetsEnabled(void) {
    return alt;
}
uint8_t ResourceMgr_FileAltExists(const char* path) {
    return alt && altList != NULL && !strcmp(path, altList);
}
void Gfx_SetupDL25_Xlu(GraphicsContext* context) {
    REQUIRE(context == &gfx);
}
void Gfx_SetupDL25_Opa(GraphicsContext* context) {
    REQUIRE(context == &gfx);
}
void gSPDisplayList(Gfx* command, Gfx* displayList) {
    __gSPDisplayList(command, displayList);
}
void Gfx_DrawDListOpa(PlayState* current, Gfx* displayList) {
    gSPDisplayList(current->state.gfxCtx->polyOpa.p++, displayList);
}
Mtx* Matrix_Finalize(GraphicsContext* context) {
    REQUIRE(context == &gfx);
    return &matrix;
}
void Matrix_RotateZYX(s16 x, s16 y, s16 z, MatrixMode mode) {
    (void)x;
    (void)y;
    (void)z;
    (void)mode;
}
f32 Math_SinS(s16 angle) {
    return sinf(angle * (3.14159265358979323846f / 32768.0f));
}
f32 Math_CosS(s16 angle) {
    return cosf(angle * (3.14159265358979323846f / 32768.0f));
}
f32 Rand_CenteredFloat(f32 scale) {
    (void)scale;
    ++gameplayRandomCalls;
    return 0.0f;
}
f32 Rand_ZeroOne(void) {
    ++gameplayRandomCalls;
    return 0.5f;
}
void Math_ApproachF(f32* value, f32 target, f32 scale, f32 maxStep) {
    f32 change = (*value - target) * scale;
    change = fminf(maxStep, fmaxf(-maxStep, change));
    *value -= change;
}
void Actor_SetScale(Actor* actor, f32 scale) {
    actor->scale = (Vec3f){ scale, scale, scale };
}
void Actor_UpdatePos(Actor* actor) {
    actor->world.pos.x += actor->velocity.x;
    actor->world.pos.y += actor->velocity.y;
    actor->world.pos.z += actor->velocity.z;
}
void Actor_Kill(Actor* actor) {
    actor->update = NULL;
    actor->draw = NULL;
}
void ActorShape_Init(ActorShape* shape, f32 yOffset, ActorShadowFunc shadow, f32 shadowScale) {
    shape->yOffset = yOffset;
    shape->shadowDraw = shadow;
    shape->shadowScale = shadowScale;
}
void Actor_ProcessInitChain(Actor* actor, InitChainEntry* init) {
    (void)actor;
    (void)init;
}
// Sanitizer registration can retain the complete branching-tree profile even
// when section GC removes these unrelated engine paths in an ordinary build.
// This fixture exercises tree/leaf rendering and spawning; fail if a retained
// initializer, destructor, or sway helper is accidentally executed.
void DynaPolyActor_Init(DynaPolyActor* actor, s32 flags) {
    REQUIRE(!"branching-tree DynaPoly initialization is outside this fixture");
}
void CollisionHeader_GetVirtual(CollisionHeader* source, CollisionHeader** destination) {
    REQUIRE(!"branching-tree collision initialization is outside this fixture");
}
s32 DynaPoly_SetBgActor(PlayState* current, DynaCollisionContext* context, Actor* actor, CollisionHeader* header) {
    REQUIRE(!"branching-tree background registration is outside this fixture");
    return 0;
}
void DynaPoly_DeleteBgActor(PlayState* current, DynaCollisionContext* context, s32 id) {
    REQUIRE(!"branching-tree background destruction is outside this fixture");
}
void CollisionCheck_SetInfo2(CollisionCheckInfo* info, DamageTable* damage, CollisionCheckInfoInit2* init) {
    REQUIRE(!"branching-tree collision setup is outside this fixture");
}
f32 Math_SmoothStepToF(f32* value, f32 target, f32 fraction, f32 step, f32 minimum) {
    REQUIRE(!"branching-tree sway is outside this fixture");
    return 0.0f;
}
s32 Collider_InitCylinder(PlayState* current, ColliderCylinder* collider) {
    (void)current;
    memset(collider, 0, sizeof(*collider));
    return 0;
}
s32 Collider_SetCylinder(PlayState* current, ColliderCylinder* collider, Actor* actor, ColliderCylinderInit* init) {
    (void)current;
    (void)init;
    collider->base.actor = actor;
    return 0;
}
s32 Collider_DestroyCylinder(PlayState* current, ColliderCylinder* collider) {
    (void)current;
    (void)collider;
    return 0;
}
void Collider_UpdateCylinder(Actor* actor, ColliderCylinder* collider) {
    REQUIRE(collider->base.actor == actor);
}
s32 CollisionCheck_SetAC(PlayState* current, CollisionCheckContext* context, Collider* collider) {
    (void)current;
    (void)context;
    (void)collider;
    ++collisions;
    return 0;
}
s32 CollisionCheck_SetOC(PlayState* current, CollisionCheckContext* context, Collider* collider) {
    (void)current;
    (void)context;
    (void)collider;
    ++collisions;
    return 0;
}
s32 Flags_GetCollectible(PlayState* current, s32 flag) {
    (void)current;
    (void)flag;
    return false;
}
void Flags_SetCollectible(PlayState* current, s32 flag) {
    (void)current;
    (void)flag;
}
s32 func_800A8150(s32 index) {
    return index;
}
Actor* Item_DropCollectible(PlayState* current, Vec3f* position, u32 params) {
    (void)current;
    (void)position;
    (void)params;
    ++drops;
    return NULL;
}
void Item_DropCollectibleRandom(PlayState* current, Actor* source, Vec3f* position, s16 params) {
    (void)current;
    (void)source;
    (void)position;
    (void)params;
    ++drops;
}
void Actor_PlaySfx(Actor* actor, u16 sound) {
    (void)actor;
    if (sound == NA_SE_EV_TREE_SWING) {
        ++swings;
    } else if (sound == NA_SE_IT_REFLECTION_WOOD) {
        ++reflections;
    } else {
        REQUIRE(!"unexpected native tree sound");
    }
}
void SkinMatrix_Vec3fMtxFMultXYZW(MtxF* transform, Vec3f* source, Vec3f* target, f32* w) {
    (void)transform;
    *target = *source;
    *w = 1.0f;
}
s32 Ship_CalcShouldDrawAndUpdate(PlayState* current, Actor* actor, Vec3f* position, f32 w, bool* draw, bool* update) {
    (void)current;
    (void)actor;
    (void)position;
    (void)w;
    *draw = *update = true;
    return true;
}
f32 BgCheck_EntityRaycastFloor5(CollisionContext* context, CollisionPoly** poly, s32* bgId, Actor* actor,
                                Vec3f* position) {
    (void)context;
    (void)poly;
    (void)bgId;
    (void)actor;
    return position->y - 200.0f;
}
Actor* Actor_Spawn(ActorContext* context, PlayState* current, s16 id, f32 x, f32 y, f32 z, s16 rotX, s16 rotY, s16 rotZ,
                   s32 params) {
    REQUIRE(id == ACTOR_EN_WOOD02);
    if (spawnFailure) {
        return NULL;
    }
    REQUIRE(spawnCount < ARRAY_COUNT(spawned));
    EnWood02* leaf = &spawned[spawnCount++];
    memset(leaf, 0, sizeof(*leaf));
    leaf->actor.id = id;
    leaf->actor.params = params;
    leaf->actor.world.pos = leaf->actor.home.pos = (Vec3f){ x, y, z };
    leaf->actor.world.rot = leaf->actor.home.rot = (Vec3s){ rotX, rotY, rotZ };
    leaf->actor.update = EnWood02_Update;
    leaf->actor.draw = EnWood02_Draw;
    EnWood02_Init(&leaf->actor, current);
    leaf->actor.next = context->actorLists[ACTORCAT_PROP].first;
    context->actorLists[ACTORCAT_PROP].first = &leaf->actor;
    return &leaf->actor;
}
Actor* Actor_SpawnAsChild(ActorContext* context, Actor* parent, PlayState* current, s16 id, f32 x, f32 y, f32 z,
                          s16 rotX, s16 rotY, s16 rotZ, s32 params) {
    Actor* child = Actor_Spawn(context, current, id, x, y, z, rotX, rotY, rotZ, params);
    if (child != NULL) {
        child->parent = parent;
    }
    return child;
}

static void Reset(void) {
    memset(&play, 0, sizeof(play));
    memset(&player, 0, sizeof(player));
    memset(&gfx, 0, sizeof(gfx));
    memset(spawned, 0, sizeof(spawned));
    play.state.gfxCtx = &gfx;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
    play.sceneId = SCENE_TOWN;
    play.gameplayFrames = 1;
    spawnCount = spawnFailure = drops = swings = reflections = collisions = 0;
    gameplayRandomCalls = 0;
    liveSeason = SEASON_AUTUMN;
    eligible = true;
    alt = false;
    altList = NULL;
}
static void InitTree(EnWood02* tree, s16 type) {
    memset(tree, 0, sizeof(*tree));
    tree->actor.id = ACTOR_EN_WOOD02;
    tree->actor.params = type;
    tree->actor.update = EnWood02_Update;
    tree->actor.draw = EnWood02_Draw;
    tree->actor.flags = ACTOR_FLAG_INSIDE_CULLING_VOLUME;
    tree->actor.xzDistToPlayer = 100.0f;
    EnWood02_Init(&tree->actor, &play);
}
static void BeginDraw(void) {
    memset(opaque, 0, sizeof(opaque));
    memset(translucent, 0, sizeof(translucent));
    gfx.polyOpa.p = opaque;
    gfx.polyXlu.p = translucent;
}
static uintptr_t Color(const Gfx* first, const Gfx* end, u8 opcode) {
    for (const Gfx* command = first; command < end; ++command) {
        if ((command->words.w0 >> 24) == opcode) {
            return command->words.w1;
        }
    }
    REQUIRE(!"expected material color command");
    return 0;
}
static int Warm(uintptr_t color) {
    int red = (color >> 24) & 255;
    int green = (color >> 16) & 255;
    int blue = (color >> 8) & 255;
    return red > green && green > blue && red > 150 && blue < 80;
}
static int LeafCount(void) {
    int count = 0;
    for (Actor* actor = play.actorCtx.actorLists[ACTORCAT_PROP].first; actor != NULL; actor = actor->next) {
        if (actor->update != NULL && actor->id == ACTOR_EN_WOOD02 &&
            (actor->params == WOOD_LEAF_GREEN || actor->params == WOOD_LEAF_YELLOW)) {
            ++count;
        }
    }
    return count;
}

static void RequireTintScoped(const Gfx* first, const Gfx* end, const char* resource) {
    const int legacyLeaf = !strcmp(resource, object_wood02_DL_000700);
    int enabled = false;
    int enables = 0;
    int disables = 0;
    int lists = 0;
    for (const Gfx* command = first; command < end; ++command) {
        switch (command->words.w0 >> 24) {
            case G_SETGRAYSCALE:
                enabled = command->words.w1 != 0;
                if (enabled) {
                    REQUIRE(command->words.w1 == (legacyLeaf ? 1 : 2));
                    ++enables;
                } else {
                    ++disables;
                }
                break;
            case G_SETINTENSITY:
                REQUIRE(legacyLeaf ? Warm(command->words.w1) : command->words.w1 == 0xEF484CFF);
                REQUIRE((command->words.w1 & 255) == 255);
                break;
            case G_DL:
                REQUIRE(enabled);
                REQUIRE(command->words.w1 == (uintptr_t)resource);
                ++lists;
                break;
        }
    }
    REQUIRE(!enabled && enables == 1 && disables == 1 && lists == 1);
}

static void Materials(void) {
    Reset();
    EnWood02 tree;
    const s16 types[] = { WOOD_TREE_OVAL_GREEN, WOOD_TREE_CONICAL_MEDIUM, WOOD_TREE_KAKARIKO_ADULT };
    for (u32 i = 0; i < ARRAY_COUNT(types); ++i) {
        InitTree(&tree, types[i]);
        liveSeason = SEASON_OFF;
        BeginDraw();
        EnWood02_Draw(&tree.actor, &play);
        Gfx originalTrunk[64];
        size_t trunkSize = (gfx.polyOpa.p - opaque) * sizeof(Gfx);
        memcpy(originalTrunk, opaque, trunkSize);
        uintptr_t nativeColor = Color(translucent, gfx.polyXlu.p, G_SETENVCOLOR);
        liveSeason = SEASON_AUTUMN;
        BeginDraw();
        EnWood02_Draw(&tree.actor, &play);
        REQUIRE(Warm(Color(translucent, gfx.polyXlu.p, G_SETENVCOLOR)));
        RequireTintScoped(translucent, gfx.polyXlu.p, (const char*)D_808C4D70[tree.drawType]);
        REQUIRE(Color(translucent, gfx.polyXlu.p, G_SETINTENSITY) == 0xEF484CFF);
        REQUIRE(Color(translucent, gfx.polyXlu.p, G_SETGRAYSCALE) == 2);
        REQUIRE((gfx.polyOpa.p - opaque) * sizeof(Gfx) == trunkSize);
        REQUIRE(!memcmp(originalTrunk, opaque, trunkSize));
        liveSeason = SEASON_WINTER;
        BeginDraw();
        EnWood02_Draw(&tree.actor, &play);
        REQUIRE(Color(translucent, gfx.polyXlu.p, G_SETINTENSITY) == 0xE2EEE9FF);
        REQUIRE(Color(translucent, gfx.polyXlu.p, G_SETGRAYSCALE) == 3);
        for (int season = SEASON_SPRING; season <= SEASON_OFF; ++season) {
            if (season == SEASON_AUTUMN || season == SEASON_SPRING || season == SEASON_WINTER) {
                continue;
            }
            liveSeason = season;
            BeginDraw();
            EnWood02_Draw(&tree.actor, &play);
            REQUIRE(Color(translucent, gfx.polyXlu.p, G_SETENVCOLOR) == nativeColor);
        }
        liveSeason = SEASON_AUTUMN;
        eligible = false;
        BeginDraw();
        EnWood02_Draw(&tree.actor, &play);
        REQUIRE(Color(translucent, gfx.polyXlu.p, G_SETENVCOLOR) == nativeColor);
        eligible = true;
    }
    EnWood02 leaf;
    InitTree(&leaf, WOOD_LEAF_GREEN);
    BeginDraw();
    EnWood02_Draw(&leaf.actor, &play);
    REQUIRE(Warm(Color(opaque, gfx.polyOpa.p, G_SETPRIMCOLOR)));
    REQUIRE((Color(opaque, gfx.polyOpa.p, G_SETPRIMCOLOR) & 255) == 127);
    REQUIRE(leaf.actor.params == WOOD_LEAF_GREEN);
    RequireTintScoped(opaque, gfx.polyOpa.p, object_wood02_DL_000700);
    REQUIRE(Color(opaque, gfx.polyOpa.p, G_SETGRAYSCALE) == 1);
    liveSeason = SEASON_OFF;
    BeginDraw();
    EnWood02_Draw(&leaf.actor, &play);
    REQUIRE(Color(opaque, gfx.polyOpa.p, G_SETPRIMCOLOR) == 0x32AA467F);
    puts("PASS autumn tree/leaf palette, separate trunk, native alpha and immediate season/eligibility restore");
}

// Strip only the new tint commands and require every native draw/material byte
// to survive. Body lists must draw before the tint and only the canopy under it.
static void RequireSpringCommands(const Gfx* first, const Gfx* end, const Gfx* native, size_t count,
                                  const void* canopy) {
    size_t source = 0;
    unsigned mode = 0, enables = 0, disables = 0, colors = 0, canopies = 0;
    for (const Gfx* cmd = first; cmd < end; ++cmd) {
        unsigned op = cmd->words.w0 >> 24;
        if (op == G_SETGRAYSCALE) {
            REQUIRE(cmd->words.w1 == 0 || cmd->words.w1 == 3);
            mode = cmd->words.w1;
            if (mode)
                ++enables;
            else
                ++disables;
            continue;
        }
        if (op == G_SETINTENSITY) {
            REQUIRE(cmd->words.w1 == 0xF6C1D9FF);
            ++colors;
            continue;
        }
        if (op == G_DL) {
            REQUIRE(mode == (cmd->words.w1 == (uintptr_t)canopy ? 3 : 0));
            canopies += cmd->words.w1 == (uintptr_t)canopy;
        }
        REQUIRE(source < count && !memcmp(cmd, native + source++, sizeof(Gfx)));
    }
    REQUIRE(!mode && enables == 1 && disables == 1 && colors == 1 && canopies == 1 && source == count);
}

static void SpringMaterials(void) {
    Reset();
    EnWood02 tree;
    ObjTree branching = { 0 };
    for (int type = WOOD_TREE_CONICAL_LARGE; type <= WOOD_TREE_KAKARIKO_ADULT + 1; ++type) {
        // Disable the special tree's collectible child; this fixture covers its
        // actual canopy draw, while its En_Ani child is a separate actor.
        InitTree(&tree, type == WOOD_TREE_KAKARIKO_ADULT + 1 ? 0x8000 | WOOD_TREE_SPECIAL : type);
        for (int ownership = 0; ownership < 3; ++ownership) {
            alt = ownership != 0;
            altList = ownership == 2 ? (const char*)D_808C4D70[tree.drawType] : NULL;
            liveSeason = SEASON_OFF;
            BeginDraw();
            EnWood02_Draw(&tree.actor, &play);
            Gfx trunk[64], leaves[64];
            size_t trunkCount = gfx.polyOpa.p - opaque, leafCount = gfx.polyXlu.p - translucent;
            memcpy(trunk, opaque, trunkCount * sizeof(Gfx));
            memcpy(leaves, translucent, leafCount * sizeof(Gfx));
            liveSeason = SEASON_SPRING;
            BeginDraw();
            EnWood02_Draw(&tree.actor, &play);
            REQUIRE((size_t)(gfx.polyOpa.p - opaque) == trunkCount && !memcmp(trunk, opaque, trunkCount * sizeof(Gfx)));
            RequireSpringCommands(translucent, gfx.polyXlu.p, leaves, leafCount, D_808C4D70[tree.drawType]);
            for (int restore = 0; restore < 3; ++restore) {
                liveSeason = restore == 0 ? SEASON_SUMMER : restore == 1 ? SEASON_OFF : SEASON_SPRING;
                eligible = restore != 2;
                BeginDraw();
                EnWood02_Draw(&tree.actor, &play);
                REQUIRE((size_t)(gfx.polyXlu.p - translucent) == leafCount &&
                        !memcmp(leaves, translucent, leafCount * sizeof(Gfx)));
                REQUIRE((size_t)(gfx.polyOpa.p - opaque) == trunkCount &&
                        !memcmp(trunk, opaque, trunkCount * sizeof(Gfx)));
            }
            eligible = true;
        }
    }
    for (int large = 0; large <= 1; ++large) {
        branching.dyna.actor.params = large;
        for (int ownership = 0; ownership < 3; ++ownership) {
            alt = ownership != 0;
            altList = ownership == 2 ? gTreeLeavesDL : NULL;
            liveSeason = SEASON_OFF;
            BeginDraw();
            ObjTree_Draw(&branching.dyna.actor, &play);
            Gfx native[64];
            size_t count = gfx.polyOpa.p - opaque;
            memcpy(native, opaque, count * sizeof(Gfx));
            liveSeason = SEASON_SPRING;
            BeginDraw();
            ObjTree_Draw(&branching.dyna.actor, &play);
            RequireSpringCommands(opaque, gfx.polyOpa.p, native, count, gTreeLeavesDL);
            for (int restore = 0; restore < 3; ++restore) {
                liveSeason = restore == 0 ? SEASON_SUMMER : restore == 1 ? SEASON_OFF : SEASON_SPRING;
                eligible = restore != 2;
                BeginDraw();
                ObjTree_Draw(&branching.dyna.actor, &play);
                REQUIRE((size_t)(gfx.polyOpa.p - opaque) == count && !memcmp(native, opaque, count * sizeof(Gfx)));
            }
            eligible = true;
        }
    }
    const s16 unchanged[] = { WOOD_BUSH_GREEN_SMALL, WOOD_BUSH_BLACK_SMALL, WOOD_LEAF_GREEN, WOOD_LEAF_YELLOW };
    for (unsigned i = 0; i < ARRAY_COUNT(unchanged); ++i) {
        InitTree(&tree, unchanged[i]);
        liveSeason = SEASON_OFF;
        BeginDraw();
        EnWood02_Draw(&tree.actor, &play);
        Gfx opa[64], xlu[64];
        size_t opaCount = gfx.polyOpa.p - opaque, xluCount = gfx.polyXlu.p - translucent;
        memcpy(opa, opaque, opaCount * sizeof(Gfx));
        memcpy(xlu, translucent, xluCount * sizeof(Gfx));
        liveSeason = SEASON_SPRING;
        BeginDraw();
        EnWood02_Draw(&tree.actor, &play);
        REQUIRE((size_t)(gfx.polyOpa.p - opaque) == opaCount && !memcmp(opa, opaque, opaCount * sizeof(Gfx)));
        REQUIRE((size_t)(gfx.polyXlu.p - translucent) == xluCount && !memcmp(xlu, translucent, xluCount * sizeof(Gfx)));
    }
    puts("PASS spring all tree canopies/Alt ownership, lifted full blossom tint, native trunks/leaves/bushes and "
         "exact summer/Off/eligibility cleanup");
}

static void WinterMaterials(void) {
    Reset();
    EnWood02 tree;
    const s16 types[] = { WOOD_TREE_OVAL_GREEN, WOOD_TREE_CONICAL_MEDIUM, WOOD_TREE_KAKARIKO_ADULT };
    for (unsigned i = 0; i < ARRAY_COUNT(types); ++i) {
        InitTree(&tree, types[i]);
        liveSeason = SEASON_OFF;
        BeginDraw();
        EnWood02_Draw(&tree.actor, &play);
        Gfx originalTrunk[64], originalLeaves[64];
        size_t trunkSize = (gfx.polyOpa.p - opaque) * sizeof(Gfx);
        size_t leafSize = (gfx.polyXlu.p - translucent) * sizeof(Gfx);
        memcpy(originalTrunk, opaque, trunkSize);
        memcpy(originalLeaves, translucent, leafSize);
        const s16 scenes[] = { SCENE_00KEIKOKU,  SCENE_BACKTOWN,    SCENE_24KEMONOMITI,
                               SCENE_20SICHITAI, SCENE_20SICHITAI2, SCENE_21MITURINMAE };
        for (unsigned scene = 0; scene < ARRAY_COUNT(scenes); ++scene) {
            play.sceneId = scenes[scene];
            liveSeason = SEASON_WINTER;
            BeginDraw();
            EnWood02_Draw(&tree.actor, &play);
            REQUIRE(Color(translucent, gfx.polyXlu.p, G_SETINTENSITY) == 0xE2EEE9FF);
            int frost = 0, draws = 0;
            for (const Gfx* cmd = translucent; cmd < gfx.polyXlu.p; ++cmd) {
                if ((cmd->words.w0 >> 24) == G_SETGRAYSCALE) {
                    REQUIRE(cmd->words.w1 == 0 || cmd->words.w1 == 3);
                    frost = cmd->words.w1;
                }
                if ((cmd->words.w0 >> 24) == G_DL) {
                    REQUIRE(frost == 3 && cmd->words.w1 == (uintptr_t)D_808C4D70[tree.drawType]);
                    ++draws;
                }
            }
            REQUIRE(frost == 0 && draws == 1);
            REQUIRE((gfx.polyOpa.p - opaque) * sizeof(Gfx) == trunkSize && !memcmp(originalTrunk, opaque, trunkSize));
            eligible = false;
            BeginDraw();
            EnWood02_Draw(&tree.actor, &play);
            REQUIRE((gfx.polyXlu.p - translucent) * sizeof(Gfx) == leafSize);
            REQUIRE(!memcmp(originalLeaves, translucent, leafSize));
            eligible = true;
        }
    }
    ObjTree branching = { 0 };
    for (int ownership = 0; ownership < 3; ++ownership) {
        alt = ownership != 0;
        altList = ownership == 2 ? gTreeLeavesDL : NULL;
        liveSeason = SEASON_OFF;
        BeginDraw();
        ObjTree_Draw(&branching.dyna.actor, &play);
        Gfx native[64];
        const size_t count = gfx.polyOpa.p - opaque;
        memcpy(native, opaque, count * sizeof(Gfx));
        liveSeason = SEASON_WINTER;
        BeginDraw();
        ObjTree_Draw(&branching.dyna.actor, &play);
        unsigned mode = 0, tinted = 0, untinted = 0;
        for (const Gfx* cmd = opaque; cmd < gfx.polyOpa.p; ++cmd) {
            if ((cmd->words.w0 >> 24) == G_SETGRAYSCALE)
                mode = cmd->words.w1;
            if ((cmd->words.w0 >> 24) == G_SETINTENSITY)
                REQUIRE(cmd->words.w1 == 0xE2EEE9FF);
            if ((cmd->words.w0 >> 24) == G_DL) {
                REQUIRE(mode == (cmd->words.w1 == (uintptr_t)gTreeLeavesDL ? 3 : 0));
                if (mode)
                    ++tinted;
                else
                    ++untinted;
            }
        }
        REQUIRE(mode == 0 && tinted == 1 && untinted == 1);
        eligible = false;
        BeginDraw();
        ObjTree_Draw(&branching.dyna.actor, &play);
        REQUIRE((size_t)(gfx.polyOpa.p - opaque) == count && !memcmp(native, opaque, count * sizeof(Gfx)));
        eligible = true;
    }
    puts("PASS winter tree canopies across adjacent outdoors and branching trees; native/Alt, untouched trunks and "
         "ineligible play");
}

static void AssetOwnership(void) {
    Reset();
    EnWood02 tree;
    InitTree(&tree, WOOD_TREE_OVAL_GREEN);
    liveSeason = SEASON_OFF;
    BeginDraw();
    EnWood02_Draw(&tree.actor, &play);
    Gfx native[64];
    size_t nativeSize = (gfx.polyXlu.p - translucent) * sizeof(Gfx);
    memcpy(native, translucent, nativeSize);
    alt = true;
    altList = (const char*)D_808C4D70[tree.drawType];
    liveSeason = SEASON_AUTUMN;
    BeginDraw();
    EnWood02_Draw(&tree.actor, &play);
    RequireTintScoped(translucent, gfx.polyXlu.p, D_808C4D70[tree.drawType]);
    altList = NULL;
    BeginDraw();
    EnWood02_Draw(&tree.actor, &play);
    RequireTintScoped(translucent, gfx.polyXlu.p, (const char*)D_808C4D70[tree.drawType]);

    EnWood02 leaf;
    InitTree(&leaf, WOOD_LEAF_GREEN);
    altList = object_wood02_DL_000700;
    BeginDraw();
    EnWood02_Draw(&leaf.actor, &play);
    REQUIRE(Color(opaque, gfx.polyOpa.p, G_SETPRIMCOLOR) == 0x32AA467F);
    REQUIRE(gfx.polyOpa.p - opaque == 2);
    REQUIRE(opaque[1].words.w1 == (uintptr_t)object_wood02_DL_000700);

    const s16 bushTypes[] = { WOOD_BUSH_GREEN_SMALL, WOOD_BUSH_BLACK_SMALL };
    for (u32 i = 0; i < ARRAY_COUNT(bushTypes); ++i) {
        InitTree(&tree, bushTypes[i]);
        liveSeason = SEASON_OFF;
        BeginDraw();
        EnWood02_Draw(&tree.actor, &play);
        nativeSize = (gfx.polyXlu.p - translucent) * sizeof(Gfx);
        memcpy(native, translucent, nativeSize);
        liveSeason = SEASON_AUTUMN;
        BeginDraw();
        EnWood02_Draw(&tree.actor, &play);
        REQUIRE((gfx.polyXlu.p - translucent) * sizeof(Gfx) == nativeSize);
        REQUIRE(!memcmp(native, translucent, nativeSize));
    }
    puts("PASS selected Alt material ownership, native resource submission, no tint leaks and unchanged bushes");
}

static void BranchingTreeMaterials(void) {
    Reset();
    ObjTree tree;
    memset(&tree, 0, sizeof(tree));
    for (s16 large = 0; large <= 1; ++large) {
        tree.dyna.actor.params = large;
        liveSeason = SEASON_OFF;
        BeginDraw();
        ObjTree_Draw(&tree.dyna.actor, &play);
        Gfx native[64];
        size_t nativeSize = (gfx.polyOpa.p - opaque) * sizeof(Gfx);
        memcpy(native, opaque, nativeSize);
        REQUIRE(opaque[1].words.w1 == (uintptr_t)gTreeBodyDL);
        liveSeason = SEASON_AUTUMN;
        BeginDraw();
        ObjTree_Draw(&tree.dyna.actor, &play);
        REQUIRE(!memcmp(native, opaque, 2 * sizeof(Gfx)));
        RequireTintScoped(opaque + 2, gfx.polyOpa.p, gTreeLeavesDL);
        for (int season = SEASON_SPRING; season <= SEASON_OFF; ++season) {
            if (season == SEASON_AUTUMN || season == SEASON_SPRING || season == SEASON_WINTER)
                continue;
            liveSeason = season;
            BeginDraw();
            ObjTree_Draw(&tree.dyna.actor, &play);
            REQUIRE((gfx.polyOpa.p - opaque) * sizeof(Gfx) == nativeSize);
            REQUIRE(!memcmp(native, opaque, nativeSize));
        }
        liveSeason = SEASON_AUTUMN;
        for (int condition = 0; condition < 2; ++condition) {
            eligible = condition != 0;
            alt = condition != 0;
            altList = gTreeLeavesDL;
            BeginDraw();
            ObjTree_Draw(&tree.dyna.actor, &play);
            if (eligible) {
                REQUIRE(!memcmp(native, opaque, 2 * sizeof(Gfx)));
                RequireTintScoped(opaque + 2, gfx.polyOpa.p, gTreeLeavesDL);
            } else {
                REQUIRE((gfx.polyOpa.p - opaque) * sizeof(Gfx) == nativeSize);
                REQUIRE(!memcmp(native, opaque, nativeSize));
            }
        }
        eligible = true;
        altList = NULL; // Texture-only replacements still use native material.
        BeginDraw();
        ObjTree_Draw(&tree.dyna.actor, &play);
        RequireTintScoped(opaque + 2, gfx.polyOpa.p, gTreeLeavesDL);
        alt = false;
    }
    puts("PASS Clock Town branching-tree leaves, unchanged trunk, live Off/story and Alt material ownership");
}

static void Particles(void) {
    Reset();
    EnWood02 trees[20];
    for (u32 i = 0; i < ARRAY_COUNT(trees); ++i) {
        InitTree(&trees[i], WOOD_TREE_OVAL_GREEN);
        trees[i].actor.world.pos.x = trees[i].actor.home.pos.x = i * 13.0f;
    }
    int lastSpawn = -1000;
    for (int frame = 1; frame <= 240; ++frame) {
        play.gameplayFrames = frame;
        int before = spawnCount;
        for (u32 i = 0; i < ARRAY_COUNT(trees); ++i) {
            EnWood02_Update(&trees[i].actor, &play);
            REQUIRE(trees[i].unk_148 == 0 && trees[i].actor.home.rot.y == 0);
        }
        REQUIRE(spawnCount - before <= 1);
        if (spawnCount != before) {
            REQUIRE(frame - lastSpawn >= 10);
            lastSpawn = frame;
        }
        REQUIRE(LeafCount() <= 8);
        for (int i = 0; i < spawnCount; ++i) {
            if (spawned[i].actor.update != NULL) {
                f32 beforeY = spawned[i].actor.world.pos.y;
                EnWood02_Update(&spawned[i].actor, &play);
                REQUIRE(spawned[i].actor.world.pos.y < beforeY);
            }
        }
    }
    REQUIRE(spawnCount > 0);
    REQUIRE(drops == 0 && swings == 0 && reflections == 0);
    REQUIRE(gameplayRandomCalls == 0);
    puts("PASS ambient native falling leaves, staggered cadence, global actor cap and native gameplay RNG isolation");
}

static void ParticleGates(void) {
    for (int condition = 0; condition < 9; ++condition) {
        Reset();
        play.sceneId = SCENE_TOWN + condition;
        EnWood02 tree;
        InitTree(&tree, condition == 6 ? WOOD_BUSH_GREEN_SMALL : WOOD_TREE_OVAL_GREEN);
        switch (condition) {
            case 0:
                tree.actor.xzDistToPlayer = 601.0f;
                break;
            case 1:
                tree.actor.playerHeightRel = -251.0f;
                break;
            case 2:
                tree.actor.flags &= ~ACTOR_FLAG_INSIDE_CULLING_VOLUME;
                break;
            case 3:
                play.pauseCtx.state = 1;
                break;
            case 4:
                liveSeason = SEASON_OFF;
                break;
            case 5:
                eligible = false;
                break;
            case 7:
                alt = true;
                altList = (const char*)D_808C4D70[tree.drawType];
                break;
            case 8:
                tree.unk_146 = -500;
                break;
        }
        for (int frame = 1; frame <= 120; ++frame) {
            play.gameplayFrames = frame;
            EnWood02_Update(&tree.actor, &play);
        }
        REQUIRE(spawnCount == 0);
        REQUIRE(drops == 0 && swings == 0 && reflections == 0);
    }

    Reset();
    EnWood02 tree;
    InitTree(&tree, WOOD_TREE_OVAL_GREEN);
    for (int i = 0; i < 8; ++i) {
        Actor_Spawn(&play.actorCtx, &play, ACTOR_EN_WOOD02, 0, 200, 0, 0, 0, 0, WOOD_LEAF_GREEN);
    }
    for (int frame = 1; frame <= 120; ++frame) {
        play.gameplayFrames = frame;
        EnWood02_Update(&tree.actor, &play);
    }
    REQUIRE(spawnCount == 8 && LeafCount() == 8);
    tree.actor.home.rot.y = 1;
    EnWood02_Update(&tree.actor, &play);
    REQUIRE(spawnCount == 12 && swings == 1);
    REQUIRE(LeafCount() == 12); // Ambient cap never suppresses a native roll burst.

    Reset();
    InitTree(&tree, WOOD_TREE_OVAL_GREEN);
    spawnFailure = true;
    for (int frame = 1; frame <= 120; ++frame) {
        play.gameplayFrames = frame;
        EnWood02_Update(&tree.actor, &play);
    }
    REQUIRE(spawnCount == 0 && drops == 0 && swings == 0);
    spawnFailure = false;
    for (int frame = 121; frame <= 160; ++frame) {
        play.gameplayFrames = frame;
        EnWood02_Update(&tree.actor, &play);
    }
    REQUIRE(spawnCount == 1);
    puts("PASS nearby/visible/pause/selection/story/Alt/impact gates, native leaf cap and allocation recovery");
}

static void ParticleLifecycle(void) {
    Reset();
    EnWood02 tree;
    InitTree(&tree, WOOD_TREE_OVAL_GREEN);
    play.gameplayFrames = 40;
    EnWood02_Update(&tree.actor, &play);
    REQUIRE(spawnCount == 1);
    EnWood02* leaf = &spawned[0];
    REQUIRE(leaf->actor.params == WOOD_LEAF_YELLOW && leaf->unk_14A[0] == 75);
    REQUIRE(leaf->actor.world.pos.y == tree.actor.world.pos.y + 200.0f);
    for (int frame = 1; frame <= 74; ++frame) {
        EnWood02_Update(&leaf->actor, &play);
        REQUIRE(leaf->actor.update != NULL);
    }
    EnWood02_Update(&leaf->actor, &play);
    REQUIRE(leaf->actor.update == NULL);

    play.roomCtx.curRoom.num = 1;
    EnWood02_Update(&tree.actor, &play);
    REQUIRE(spawnCount == 2);
    play.sceneId = SCENE_00KEIKOKU;
    EnWood02_Update(&tree.actor, &play);
    REQUIRE(spawnCount == 3);
    play.gameplayFrames = 0;
    EnWood02_Update(&tree.actor, &play);
    REQUIRE(spawnCount == 4);
    REQUIRE(gameplayRandomCalls == 0);
    liveSeason = SEASON_OFF;
    EnWood02_Update(&spawned[1].actor, &play);
    REQUIRE(spawned[1].actor.update == NULL);
    liveSeason = SEASON_SPRING;
    EnWood02_Update(&spawned[2].actor, &play);
    REQUIRE(spawned[2].actor.update == NULL);
    liveSeason = SEASON_AUTUMN;
    eligible = false;
    EnWood02_Update(&spawned[3].actor, &play);
    REQUIRE(spawned[3].actor.update == NULL);
    EnWood02 native;
    InitTree(&native, WOOD_LEAF_GREEN);
    EnWood02_Update(&native.actor, &play);
    REQUIRE(native.actor.update != NULL && native.unk_14A[0] == 74);
    puts("PASS native lifetime/falling motion, room/scene/frame budget reset and ambient-only removal on Off/story");
}

static void NativeImpact(void) {
    Reset();
    EnWood02 tree;
    InitTree(&tree, WOOD_TREE_OVAL_GREEN);
    tree.actor.home.rot.y = 1;
    tree.unk_148 = 1;
    tree.collider.base.acFlags |= AC_HIT;
    play.gameplayFrames = 3;
    EnWood02_Update(&tree.actor, &play);
    REQUIRE(drops == 1 && swings == 1 && reflections == 1 && collisions == 2);
    REQUIRE(spawnCount == 4);
    REQUIRE(gameplayRandomCalls == 16);
    REQUIRE(tree.unk_148 == 0 && tree.actor.home.rot.y == 0 && tree.unk_146 == -20);
    puts("PASS native impact collision, item drop and four leaf burst");
}

int main(int argc, char** argv) {
    if (argc == 1 || !strcmp(argv[1], "materials")) {
        SpringMaterials();
        BranchingTreeMaterials();
        WinterMaterials();
        Materials();
        AssetOwnership();
    }
    if (argc == 1 || !strcmp(argv[1], "particles")) {
        Particles();
        ParticleGates();
        ParticleLifecycle();
    }
    if (argc == 1 || !strcmp(argv[1], "native")) {
        NativeImpact();
    }
    return 0;
}
