// Private implementation included by each host's native translation unit.
#include "elemental_sword_finish_core.h"
#include <math.h>

#ifdef SWORD_FINISH_OOT
#define FINISH_INFO ColliderInfo
#define FINISH_FLAGS(info) ((info)->toucher.dmgFlags)
#define FINISH_FIRE DMG_ARROW_FIRE
#define FINISH_ICE DMG_ARROW_ICE
#define FINISH_LIGHT DMG_ARROW_LIGHT
#define FINISH_TABLE(data) ((data)->table)
#define FINISH_SCENE(play) ((play)->sceneNum)
#define FINISH_DEBUG_PAUSE(play) ((play)->pauseCtx.debugState)
#define FINISH_SETUP Gfx_SetupDL_25Xlu
#define FINISH_MATRIX_TO(buffer) Matrix_ToMtx(buffer, __FILE__, __LINE__)
#define FINISH_ROTATE_Z Matrix_RotateZ
#define FINISH_ROTATE_X Matrix_RotateX
#define FINISH_ROTATE_Y Matrix_RotateY
#else
#define FINISH_INFO ColliderElement
#define FINISH_FLAGS(info) ((info)->atDmgInfo.dmgFlags)
#define FINISH_FIRE DMG_FIRE_ARROW
#define FINISH_ICE DMG_ICE_ARROW
#define FINISH_LIGHT DMG_LIGHT_ARROW
#define FINISH_TABLE(data) ((data)->attack)
#define FINISH_SCENE(play) ((play)->sceneId)
#define FINISH_DEBUG_PAUSE(play) ((play)->pauseCtx.debugEditor)
#define FINISH_SETUP Gfx_SetupDL25_Xlu
#define FINISH_MATRIX_TO(buffer) Matrix_ToMtx(buffer)
#define FINISH_ROTATE_Z Matrix_RotateZF
#define FINISH_ROTATE_Y Matrix_RotateYF
#endif

static SwordFinishCore sFinish;
// Both audio banks retain source-position addresses. These live for the
// entire module lifetime; expiration/scene resets release each bank reference.
static Vec3f sFinishSoundPos[SWORD_FINISH_EFFECT_CAPACITY];

#define SWORD_FINISH_SOUND_REQUEST_LIMIT 32
static uint32_t sFinishSoundFrame;
static unsigned sFinishSoundRequests;
static unsigned sFinishSoundCursor;

static int DinFireSword_FinishReserveSound(PlayState* play) {
    if (sFinishSoundFrame != play->gameplayFrames) {
        sFinishSoundFrame = play->gameplayFrames;
        sFinishSoundRequests = 0;
    }
    // Leave space in the shared 256-entry ring for ordinary native sounds.
    if (sFinishSoundRequests >= SWORD_FINISH_SOUND_REQUEST_LIMIT)
        return 0;
    ++sFinishSoundRequests;
    return 1;
}

static int DinFireSword_FinishLoopSound(uint8_t element) {
    return element == SWORD_ELEMENT_LIGHT || element == SWORD_ELEMENT_ICE;
}

static void DinFireSword_FinishStopSound(unsigned slot, uint8_t element) {
#ifdef SWORD_FINISH_OOT
    if (element == SWORD_ELEMENT_LIGHT)
        MmSfx_StopAtPos(0x31F7, &sFinishSoundPos[slot]);
    else
        Audio_StopSfxByPosAndId(&sFinishSoundPos[slot], NA_SE_EV_ICE_FREEZE - SFX_FLAG);
#else
    AudioSfx_StopByPosAndId(&sFinishSoundPos[slot],
                            (element == SWORD_ELEMENT_LIGHT ? NA_SE_EN_COMMON_DEADLIGHT : NA_SE_EV_ICE_FREEZE) -
                                SFX_FLAG);
#endif
}

static int DinFireSword_FinishContext(PlayState* play) {
    if (play && sFinish.context == (uintptr_t)play && sFinish.frame == play->gameplayFrames &&
        sFinish.scene == FINISH_SCENE(play) && sFinish.room == play->roomCtx.curRoom.num &&
        sFinish.paused == (play->pauseCtx.state != 0 || FINISH_DEBUG_PAUSE(play) != 0) &&
        sFinish.active == (play->transitionTrigger == TRANS_TRIGGER_OFF))
        return sFinish.active && !sFinish.paused;
    uint8_t wasSounded[SWORD_FINISH_EFFECT_CAPACITY];
    for (unsigned i = 0; i < SWORD_FINISH_EFFECT_CAPACITY; ++i)
        wasSounded[i] = sFinish.effects[i].life && sFinish.effects[i].sounded &&
                                DinFireSword_FinishLoopSound(sFinish.effects[i].element)
                            ? sFinish.effects[i].element
                            : 0;
    if (!play) {
        for (unsigned i = 0; i < SWORD_FINISH_EFFECT_CAPACITY; ++i)
            if (wasSounded[i])
                DinFireSword_FinishStopSound(i, wasSounded[i]);
        memset(&sFinish, 0, sizeof(sFinish));
        return 0;
    }
    SwordFinishCore_Context(&sFinish, (uintptr_t)play, FINISH_SCENE(play), play->roomCtx.curRoom.num,
                            play->gameplayFrames, play->pauseCtx.state != 0 || FINISH_DEBUG_PAUSE(play) != 0,
                            play->transitionTrigger == TRANS_TRIGGER_OFF);
    for (unsigned i = 0; i < SWORD_FINISH_EFFECT_CAPACITY; ++i)
        if (wasSounded[i] && (!sFinish.effects[i].life || sFinish.paused)) {
            DinFireSword_FinishStopSound(i, wasSounded[i]);
            // Resume may occur without advancing the simulation frame.
            sFinish.effects[i].sounded = 0;
        }
    return sFinish.active && !sFinish.paused;
}

static SwordFinishPoint DinFireSword_FinishOrigin(const Actor* actor) {
    return (SwordFinishPoint){ actor->world.pos.x, actor->world.pos.y, actor->world.pos.z, 0 };
}

int DinFireSword_UseIceMaterial(PlayState* play, const Actor* actor) {
    if (!play || !actor)
        return 0;
    DinFireSword_FinishContext(play);
    // Keys only: a late native spawn need not read a victim that was killed.
    return SwordFinishCore_UseIceMaterial(&sFinish, (uintptr_t)actor);
}

static int DinFireSword_FinishEnemy(const Actor* actor) {
    return actor && (actor->category == ACTORCAT_ENEMY || actor->category == ACTORCAT_BOSS ||
                     (actor->flags & ACTOR_FLAG_HOSTILE));
}

static uint8_t DinFireSword_FinishElement(PlayState* play, const FINISH_INFO* info) {
    if (!info)
        return 0;
    uint32_t original = DinFireSword_OriginalDamageFlags(play, info);
    uint32_t added = FINISH_FLAGS(info) ^ original;
    if (original == FINISH_FLAGS(info))
        return 0;
    return added == FINISH_FIRE    ? SWORD_ELEMENT_FIRE
           : added == FINISH_ICE   ? SWORD_ELEMENT_ICE
           : added == FINISH_LIGHT ? SWORD_ELEMENT_LIGHT
                                   : 0;
}

static int DinFireSword_FinishNative(const Actor* actor, uint8_t element, uint32_t receiverFlags) {
    const DamageTable* table = actor->colChkInfo.damageTable;
    uint32_t flag = element == 1 ? FINISH_FIRE : element == 2 ? FINISH_ICE : FINISH_LIGHT;
    if (!table || !element || !(receiverFlags & flag) || !actor->colChkInfo.damageEffect ||
        actor->colChkInfo.damageEffect != (FINISH_TABLE(table)[0x0A + element] >> 4))
        return 0;
        // This list only prevents duplication of already-audited native responses.
        // Fallback eligibility above/below is independent of actor ID or table code.
#ifdef SWORD_FINISH_OOT
    if (actor->id == ACTOR_EN_PEEHAT && actor->params > 0)
        return 0;
    if (element == SWORD_ELEMENT_LIGHT)
        return 0; // OoT's light stun is not MM's light burn.
    if (element == SWORD_ELEMENT_ICE)
        return actor->id == ACTOR_EN_ZF || actor->id == ACTOR_EN_CROW || actor->id == ACTOR_EN_FIREFLY;
    if (actor->id == ACTOR_EN_ANUBICE)
        return 1;
#else
    if (element != SWORD_ELEMENT_FIRE && actor->id == ACTOR_EN_DINOFOS)
        return 1;
    if (element == SWORD_ELEMENT_LIGHT && (actor->id == ACTOR_EN_RD || actor->id == ACTOR_EN_FZ))
        return 1;
#endif
    switch (actor->id) {
        case ACTOR_EN_WF:
        case ACTOR_EN_WALLMAS:
        case ACTOR_EN_FLOORMAS:
        case ACTOR_EN_CROW:
        case ACTOR_EN_DEKUNUTS:
        case ACTOR_EN_PEEHAT:
        case ACTOR_EN_FIREFLY:
            return 1;
        default:
            return 0;
    }
}

static float DinFireSword_FinishClamp(float value, float low, float high) {
    return value < low ? low : value > high ? high : value;
}

static void DinFireSword_FinishBox(SwordFinishShape* shape, float x, float bottom, float z, float radiusX, float height,
                                   float radiusZ) {
    radiusX = DinFireSword_FinishClamp(fabsf(radiusX), 4, 250);
    radiusZ = DinFireSword_FinishClamp(fabsf(radiusZ), 4, 250);
    height = DinFireSword_FinishClamp(fabsf(height), 8, 500);
    float radius = fmaxf(fmaxf(radiusX, radiusZ) * 0.75f, height * 0.18f);
    shape->count = 0;
    for (int i = 0; i < 3; ++i)
        shape->points[shape->count++] = (SwordFinishPoint){ x, bottom + height * (0.18f + i * 0.32f), z, radius };
    for (int i = 0; i < 4; ++i)
        shape->points[shape->count++] = (SwordFinishPoint){ x + (i == 0   ? radiusX * 0.6f
                                                                 : i == 1 ? -radiusX * 0.6f
                                                                          : 0),
                                                            bottom + height * 0.5f,
                                                            z + (i == 2   ? radiusZ * 0.6f
                                                                 : i == 3 ? -radiusZ * 0.6f
                                                                          : 0),
                                                            radius };
}

static void DinFireSword_FinishVertices(SwordFinishShape* shape, const Vec3f* vertices, unsigned count) {
    Vec3f min = vertices[0], max = min;
    for (unsigned i = 1; i < count; ++i) {
        min.x = fminf(min.x, vertices[i].x);
        min.y = fminf(min.y, vertices[i].y);
        min.z = fminf(min.z, vertices[i].z);
        max.x = fmaxf(max.x, vertices[i].x);
        max.y = fmaxf(max.y, vertices[i].y);
        max.z = fmaxf(max.z, vertices[i].z);
    }
    DinFireSword_FinishBox(shape, (min.x + max.x) * 0.5f, min.y, (min.z + max.z) * 0.5f, (max.x - min.x) * 0.5f,
                           max.y - min.y, (max.z - min.z) * 0.5f);
}

static SwordFinishShape DinFireSword_FinishBounds(const Collider* collider) {
    SwordFinishShape shape = { 0 };
    const Actor* actor = collider->actor;
    shape.origin = DinFireSword_FinishOrigin(actor);
    if (collider->shape == COLSHAPE_CYLINDER) {
        const Cylinder16* dim = &((const ColliderCylinder*)collider)->dim;
        DinFireSword_FinishBox(&shape, dim->pos.x, dim->pos.y + dim->yShift, dim->pos.z, dim->radius, dim->height,
                               dim->radius);
    } else if (collider->shape == COLSHAPE_JNTSPH) {
        const ColliderJntSph* joint = (const ColliderJntSph*)collider;
        for (int i = 0; joint->elements && i < joint->count && shape.count < SWORD_FINISH_POINT_CAPACITY; ++i) {
            const Sphere16* sphere = &joint->elements[i].dim.worldSphere;
            if (sphere->radius > 0)
                shape.points[shape.count++] = (SwordFinishPoint){ sphere->center.x, sphere->center.y, sphere->center.z,
                                                                  DinFireSword_FinishClamp(sphere->radius, 4, 150) };
        }
    } else if (collider->shape == COLSHAPE_QUAD) {
        DinFireSword_FinishVertices(&shape, ((const ColliderQuad*)collider)->dim.quad, 4);
    } else if (collider->shape == COLSHAPE_TRIS) {
        const ColliderTris* tris = (const ColliderTris*)collider;
        if (tris->elements && tris->count > 0)
            DinFireSword_FinishVertices(&shape, tris->elements[0].dim.vtx, 3);
#ifndef SWORD_FINISH_OOT
    } else if (collider->shape == COLSHAPE_SPHERE) {
        const Sphere16* sphere = &((const ColliderSphere*)collider)->dim.worldSphere;
        shape.points[shape.count++] = (SwordFinishPoint){ sphere->center.x, sphere->center.y, sphere->center.z,
                                                          DinFireSword_FinishClamp(sphere->radius, 4, 150) };
#endif
    }
    if (!shape.count) {
        // Unusual/custom receiver: a conservative envelope from its live focus.
        float height = fabsf(actor->focus.pos.y - actor->world.pos.y);
        DinFireSword_FinishBox(&shape, actor->world.pos.x, actor->world.pos.y, actor->world.pos.z,
                               fmaxf(height * 0.3f, 12), fmaxf(height, 24), fmaxf(height * 0.3f, 12));
    }
    return shape;
}

static void DinFireSword_FinishIceImpact(PlayState* play, uintptr_t key, uint8_t element,
                                         const SwordFinishShape* shape) {
    if (element != SWORD_ELEMENT_ICE)
        return;
    SwordFinishHit* hit = SwordFinishCore_Find(&sFinish, key);
    if (!hit || hit->iceImpacted || !DinFireSword_FinishReserveSound(play))
        return;
    Vec3f pos = { shape->origin.x, shape->origin.y, shape->origin.z };
    // The native fixed-world emitter copies this value; it never retains the
    // collider, the dying actor or this stack address.
    SoundSource_PlaySfxAtFixedWorldPos(play, &pos, 24, NA_SE_IT_EXPLOSION_ICE);
    hit->iceImpacted = 1;
}

void DinFireSword_FinishAcceptedHit(PlayState* play, const void* receiverCollider, const FINISH_INFO* info,
                                    uint32_t receiverFlags, uint32_t damage) {
    const Collider* receiver = receiverCollider;
    if (!DinFireSword_FinishContext(play) || !receiver || !DinFireSword_FinishEnemy(receiver->actor))
        return;
    uint8_t element = DinFireSword_FinishElement(play, info);
    // Material provenance follows accepted collisions, independently of the
    // audio request quota and the shorter-lived fatal-attribution record.
    SwordFinishCore_SetIceMaterial(&sFinish, (uintptr_t)receiver->actor, element == SWORD_ELEMENT_ICE);
    if (!damage)
        return;
    SwordFinishShape shape = DinFireSword_FinishBounds(receiver);
    SwordFinishCore_Hit(&sFinish, (uintptr_t)receiver->actor, element,
                        DinFireSword_FinishNative(receiver->actor, element, receiverFlags), damage, &shape);
    DinFireSword_FinishIceImpact(play, (uintptr_t)receiver->actor, element, &shape);
}

void DinFireSword_FinishConfirmedHit(PlayState* play, const void* receiverCollider, const FINISH_INFO* info,
                                     uint32_t receiverFlags) {
    const Collider* receiver = receiverCollider;
    if (!DinFireSword_FinishContext(play) || !receiver || !receiver->actor || !info ||
        !(FINISH_FLAGS(info) & receiverFlags))
        return;
    uint8_t element = DinFireSword_FinishElement(play, info);
    SwordFinishCore_SetIceMaterial(&sFinish, (uintptr_t)receiver->actor, element == SWORD_ELEMENT_ICE);
    if (!element)
        return;
    SwordFinishShape shape = DinFireSword_FinishBounds(receiver);
    SwordFinishCore_ConfirmPart(&sFinish, (uintptr_t)receiver->actor, 0, element,
                                DinFireSword_FinishNative(receiver->actor, element, receiverFlags), &shape);
    DinFireSword_FinishIceImpact(play, (uintptr_t)receiver->actor, element, &shape);
}

void DinFireSword_FinishConfirmedPart(PlayState* play, Actor* actor, const FINISH_INFO* info, unsigned part, float x,
                                      float y, float z, float radius) {
    if (!DinFireSword_FinishContext(play) || !actor)
        return;
    uint8_t element = DinFireSword_FinishElement(play, info);
    SwordFinishCore_SetIceMaterial(&sFinish, (uintptr_t)actor, element == SWORD_ELEMENT_ICE);
    if (!element)
        return;
    SwordFinishShape shape = { 0 };
    shape.count = 1;
    shape.origin = shape.points[0] = (SwordFinishPoint){ x, y, z, DinFireSword_FinishClamp(radius, 4, 150) };
    SwordFinishCore_ConfirmPart(&sFinish, (uintptr_t)actor, part, element, 0, &shape);
    DinFireSword_FinishIceImpact(play, (uintptr_t)actor, element, &shape);
}

static uint8_t DinFireSword_FinishHealth(const Actor* actor) {
#ifdef SWORD_FINISH_OOT
    // Both bosses deliberately test signed health after u8 subtraction. Observe
    // their overkill as fatal, while their native progression clamps stay intact.
    if (actor->id == ACTOR_BOSS_GANON || actor->id == ACTOR_BOSS_GANON2)
        return (int8_t)actor->colChkInfo.health <= 0 ? 0 : actor->colChkInfo.health;
#endif
    return actor->colChkInfo.health;
}

void DinFireSword_FinishBeginActor(PlayState* play, Actor* actor) {
    if (DinFireSword_FinishContext(play) && SwordFinishCore_Find(&sFinish, (uintptr_t)actor))
        SwordFinishCore_Begin(&sFinish, (uintptr_t)actor, DinFireSword_FinishHealth(actor));
}
void DinFireSword_FinishEndActor(PlayState* play, Actor* actor) {
    if (DinFireSword_FinishContext(play) && SwordFinishCore_Find(&sFinish, (uintptr_t)actor)) {
        SwordFinishPoint origin = DinFireSword_FinishOrigin(actor);
        SwordFinishCore_End(&sFinish, (uintptr_t)actor, DinFireSword_FinishHealth(actor), &origin);
    }
}
void DinFireSword_FinishFatal(PlayState* play, Actor* actor) {
    if (DinFireSword_FinishContext(play) && SwordFinishCore_Find(&sFinish, (uintptr_t)actor)) {
        SwordFinishPoint origin = DinFireSword_FinishOrigin(actor);
        SwordFinishCore_Fatal(&sFinish, (uintptr_t)actor, &origin);
    }
}
void DinFireSword_FinishForget(PlayState* play, Actor* actor) {
    // Do not read actor memory: Actor_Destroy may also run during scene teardown.
    DinFireSword_FinishContext(play);
    if (play && sFinish.context == (uintptr_t)play) {
        SwordFinishCore_Forget(&sFinish, (uintptr_t)actor);
        SwordFinishCore_SetIceMaterial(&sFinish, (uintptr_t)actor, 0);
    }
}
void DinFireSword_FinishTeardown(PlayState* play) {
    // DestroyContext runs before PlayState storage is released. An unrelated
    // or older context must not clear a newer play session's presentations.
    if (sFinish.context == (uintptr_t)play)
        DinFireSword_FinishContext(NULL);
}

static int DinFireSword_FinishPlaySound(PlayState* play, SwordFinishEffect* effect) {
    if (sFinish.paused || !effect->life)
        return 0;
    if (effect->sounded &&
        (!DinFireSword_FinishLoopSound(effect->element) || effect->soundFrame == play->gameplayFrames))
        return 0;
    if (!DinFireSword_FinishReserveSound(play))
        return 0;
    Vec3f pos = { effect->shape.origin.x, effect->shape.origin.y, effect->shape.origin.z };
    if (DinFireSword_FinishLoopSound(effect->element)) {
        unsigned slot = (unsigned)(effect - sFinish.effects);
        if (slot >= SWORD_FINISH_EFFECT_CAPACITY)
            return 0;
        SkinMatrix_Vec3fMtxFMultXYZ(&play->viewProjectionMtxF, &pos, &sFinishSoundPos[slot]);
        effect->sounded = 1;
        effect->soundFrame = play->gameplayFrames;
        // Native frozen damage refreshes ICE_FREEZE - SFX_FLAG each frame.
        // Use copied positions: even dismembered actors may already be gone.
        // Only light needs the imported MM bank in OoT.
#ifdef SWORD_FINISH_OOT
        if (effect->element == SWORD_ELEMENT_LIGHT)
            MmSfx_PlayAtPos(0x39F7 - 0x800, &sFinishSoundPos[slot]);
        else
            Audio_PlaySfxGeneral(NA_SE_EV_ICE_FREEZE - SFX_FLAG, &sFinishSoundPos[slot], 4, &gSfxDefaultFreqAndVolScale,
                                 &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
#else
        Audio_PlaySfx_AtPos(&sFinishSoundPos[slot],
                            (effect->element == SWORD_ELEMENT_LIGHT ? NA_SE_EN_COMMON_DEADLIGHT : NA_SE_EV_ICE_FREEZE) -
                                SFX_FLAG);
#endif
    } else {
        effect->sounded = 1;
        SoundSource_PlaySfxAtFixedWorldPos(play, &pos, 24, NA_SE_EV_BURN_OUT);
    }
    return 1;
}

static void DinFireSword_FinishPlayAllSounds(PlayState* play) {
    unsigned start = sFinishSoundCursor;
    for (unsigned visited = 0; visited < SWORD_FINISH_EFFECT_CAPACITY; ++visited) {
        unsigned slot = (start + visited) % SWORD_FINISH_EFFECT_CAPACITY;
        if (DinFireSword_FinishPlaySound(play, &sFinish.effects[slot]))
            sFinishSoundCursor = (slot + 1) % SWORD_FINISH_EFFECT_CAPACITY;
    }
    for (unsigned slot = 0; slot < SWORD_FINISH_EFFECT_CAPACITY; ++slot) {
        SwordFinishEffect* effect = &sFinish.effects[slot];
        if (effect->life && DinFireSword_FinishLoopSound(effect->element) && effect->sounded &&
            effect->soundFrame != play->gameplayFrames) {
            DinFireSword_FinishStopSound(slot, effect->element);
            effect->sounded = 0;
        }
    }
}

// A finish is optional extra work in a shared native graphics arena. Preserve
// room for later passes and bound this pass independently of recorded deaths.
#define SWORD_FINISH_DRAW_POINT_LIMIT 32
static unsigned sFinishDrawCursor;

static int DinFireSword_FinishCanDrawPoint(GraphicsContext* gfx) {
    uintptr_t opaHead = (uintptr_t)gfx->polyOpa.p, opaTail = (uintptr_t)gfx->polyOpa.d;
    uintptr_t xluHead = (uintptr_t)gfx->polyXlu.p, xluTail = (uintptr_t)gfx->polyXlu.d;
    return opaHead && xluHead && opaTail >= opaHead && xluTail >= xluHead && opaTail - opaHead >= 2048 &&
           xluTail - xluHead >= (64 + 32) * sizeof(Gfx);
}

static Mtx* DinFireSword_FinishAllocateMatrix(GraphicsContext* gfx) {
#ifdef SWORD_FINISH_OOT
    return Graph_Alloc(gfx, sizeof(Mtx));
#else
    return GRAPH_ALLOC(gfx, sizeof(Mtx));
#endif
}

void DinFireSword_FinishDrawAll(PlayState* play) {
    if (!DinFireSword_FinishContext(play))
        return;
    DinFireSword_FinishPlayAllSounds(play);
    Matrix_Push();
    unsigned budget = SWORD_FINISH_DRAW_POINT_LIMIT;
    unsigned start = sFinishDrawCursor;
    OPEN_DISPS(play->state.gfxCtx);
    for (unsigned visited = 0; visited < SWORD_FINISH_EFFECT_CAPACITY; ++visited) {
        unsigned slot = (start + visited) % SWORD_FINISH_EFFECT_CAPACITY;
        SwordFinishEffect* effect = &sFinish.effects[slot];
        if (!effect->life)
            continue;
        uint8_t alpha = effect->life < 10 ? effect->life * 255 / 10 : 255;
        for (unsigned i = 0; i < effect->shape.count; ++i) {
            if (!budget || !DinFireSword_FinishCanDrawPoint(play->state.gfxCtx))
                break;
            Mtx* allocatedMatrix = DinFireSword_FinishAllocateMatrix(play->state.gfxCtx);
            if (allocatedMatrix == NULL)
                break;
            FINISH_SETUP(play->state.gfxCtx);
            SwordFinishPoint* point = &effect->shape.points[i];
            FrameInterpolation_RecordOpenChild(point, effect->element);
            Matrix_Translate(point->x, point->y, point->z, MTXMODE_NEW);
            float scale = DinFireSword_FinishClamp(point->radius / 12.0f, 0.3f, 10);
            if (effect->element == SWORD_ELEMENT_ICE) {
                extern Gfx* SwordFinishIce_GetDisplayList(uint32_t frame);
                Gfx* iceDisplayList = SwordFinishIce_GetDisplayList(play->gameplayFrames);
                Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);
                FINISH_ROTATE_Y((i & 1) ? 3.14159265f : 0, MTXMODE_APPLY);
#ifdef SWORD_FINISH_OOT
                // Native EnIce randomizes pitch. Keep clumps distinct without
                // consuming the gameplay RNG from a presentation-only draw.
                FINISH_ROTATE_X(((slot * 73u + i * 151u + 97u) & 255u) * (6.28318530f / 256.0f) - 3.14159265f,
                                MTXMODE_APPLY);
#endif
                gSPSegment(POLY_XLU_DISP++, 0x08,
                           Gfx_TwoTexScrollEx(play->state.gfxCtx, 0, 0, play->gameplayFrames & 255, 32, 16, 1, 0,
                                              (play->gameplayFrames * 2) & 255, 64, 32, 0, 1, 0, 2));
#ifdef SWORD_FINISH_OOT
                // Retain each host's original palette for unsupported native
                // or Alt roots; scoped frost uses a blue/white envelope.
                gDPSetPrimColor(POLY_XLU_DISP++, 0, 0x80, iceDisplayList ? 95 : 150, iceDisplayList ? 165 : 150,
                                iceDisplayList ? 215 : 150, 250);
                gDPSetEnvColor(POLY_XLU_DISP++, iceDisplayList ? 230 : 235, iceDisplayList ? 250 : 245, 255, alpha);
#else
                gDPSetPrimColor(POLY_XLU_DISP++, 0, 0x80, iceDisplayList ? 95 : 170, iceDisplayList ? 165 : 255,
                                iceDisplayList ? 215 : 255, 255);
                gDPSetEnvColor(POLY_XLU_DISP++, iceDisplayList ? 230 : 200, iceDisplayList ? 250 : 200, 255, alpha);
#endif
                gSPMatrix(POLY_XLU_DISP++, FINISH_MATRIX_TO(allocatedMatrix),
                          G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
#ifdef SWORD_FINISH_OOT
                Vec3f pos = { point->x, point->y, point->z }, lightDir = { 89.8f, 0, 89.8f };
                func_8002EB44(&pos, &play->view.eye, &lightDir, play->state.gfxCtx);
                if (iceDisplayList)
                    gSPDisplayList(POLY_XLU_DISP++, iceDisplayList);
                else
                    gSPDisplayList(POLY_XLU_DISP++, gEffIceFragment2DL);
#else
                Vec3f pos = { point->x, point->y, point->z };
                func_800BCC68(&pos, play);
                if (iceDisplayList)
                    gSPDisplayList(POLY_XLU_DISP++, iceDisplayList);
                else
                    gSPDisplayList(POLY_XLU_DISP++, gEffIceFragment2MaterialDL);
                gSPDisplayList(POLY_XLU_DISP++, gEffIceFragment2ModelDL);
#endif
            } else if (effect->element == SWORD_ELEMENT_FIRE) {
                Matrix_ReplaceRotation(&play->billboardMtxF);
                Matrix_Scale(scale * 0.004f, scale * 0.004f, scale * 0.004f, MTXMODE_APPLY);
                gDPSetPrimColor(POLY_XLU_DISP++, 0x80, 0x80, 255, 255, 0, alpha);
                gDPSetEnvColor(POLY_XLU_DISP++, 255, 10, 0, 0);
                gSPSegment(POLY_XLU_DISP++, 0x08,
                           Gfx_TwoTexScrollEx(play->state.gfxCtx, 0, 0, 0, 32, 64, 1, 0,
                                              ((i * 10 + play->gameplayFrames) * -20) & 511, 32, 128, 0, 0, 0, -20));
                gSPMatrix(POLY_XLU_DISP++, FINISH_MATRIX_TO(allocatedMatrix),
                          G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
                gSPDisplayList(POLY_XLU_DISP++, gEffFire1DL);
            } else {
                Matrix_ReplaceRotation(&play->billboardMtxF);
                Matrix_Scale(scale * 2.2f, scale * 2.2f, scale * 2.2f, MTXMODE_APPLY);
                // Deterministic spin avoids changing gameplay/drop RNG from draw.
                FINISH_ROTATE_Z((play->gameplayFrames * 0.11f) + i * 1.618f, MTXMODE_APPLY);
#ifdef SWORD_FINISH_OOT
                Gfx* material = MmAssets_LoadDisplayListGraphStrict("objects/gameplay_keep/gLightOrbMaterial1DL");
                Gfx* model = MmAssets_LoadDisplayListGraphStrict("objects/gameplay_keep/gLightOrbModelDL");
                if (material && model) {
                    MmAssets_EnsureStrictTextureBindings();
                    gSPDisplayList(POLY_XLU_DISP++, material);
                } else {
                    gSPDisplayList(POLY_XLU_DISP++, "__OTR__overlays/ovl_Boss_Ganon2/gGanonLightOrbMaterialDL");
                }
#else
                gSPDisplayList(POLY_XLU_DISP++, gLightOrbMaterial1DL);
#endif
                // Native MM LIGHT_ORBS white core and gold envelope.
                gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 200, alpha);
                gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 100, 128);
                gSPMatrix(POLY_XLU_DISP++, FINISH_MATRIX_TO(allocatedMatrix),
                          G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
#ifdef SWORD_FINISH_OOT
                if (material && model)
                    gSPDisplayList(POLY_XLU_DISP++, model);
                else
                    gSPDisplayList(POLY_XLU_DISP++, "__OTR__overlays/ovl_Boss_Ganon2/gGanonLightOrbModelDL");
#else
                gSPDisplayList(POLY_XLU_DISP++, gLightOrbModelDL);
#endif
            }
            FrameInterpolation_RecordCloseChild();
            --budget;
            sFinishDrawCursor = (slot + 1) % SWORD_FINISH_EFFECT_CAPACITY;
        }
    }
    if (budget != SWORD_FINISH_DRAW_POINT_LIMIT)
        FINISH_SETUP(play->state.gfxCtx);
    CLOSE_DISPS(play->state.gfxCtx);
    Matrix_Pop();
}
