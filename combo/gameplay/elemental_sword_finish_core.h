#ifndef COMBO_ELEMENTAL_SWORD_FINISH_CORE_H
#define COMBO_ELEMENTAL_SWORD_FINISH_CORE_H

#include <stdint.h>
#include <string.h>

// Presentation only. Actor memory is never owned or dereferenced here. Hit
// keys expire at consumption; material provenance retains only bounded keys.
#define SWORD_FINISH_HIT_CAPACITY 2048
#define SWORD_FINISH_EFFECT_CAPACITY 2048
#define SWORD_FINISH_POINT_CAPACITY 12
#define SWORD_FINISH_DURATION 24
#define SWORD_FINISH_HAS_PARTS 1
#define SWORD_FINISH_HAS_ICE_MATERIAL 1
// Native ice spawns for 36/40 frames; its stun color filter can last 80.
#define SWORD_FINISH_ICE_MATERIAL_DURATION 96

typedef struct {
    float x, y, z, radius;
} SwordFinishPoint;
typedef struct {
    SwordFinishPoint origin;
    SwordFinishPoint points[SWORD_FINISH_POINT_CAPACITY];
    uint8_t count;
} SwordFinishShape;
typedef struct {
    uintptr_t key;
    uint32_t parts;
    uint8_t element, native, ambiguous, updating, health, iceImpacted;
    SwordFinishShape shape;
} SwordFinishHit;
typedef struct {
    // Finished presentations contain copied values only, including after deletion.
    SwordFinishShape shape;
    uint8_t element, life, sounded;
    uint32_t soundFrame;
} SwordFinishEffect;
typedef struct {
    uintptr_t key;
    uint8_t life;
} SwordFinishIceMaterial;
typedef struct {
    uintptr_t context;
    uint32_t frame;
    int16_t scene, room;
    uint8_t paused, active;
    uint16_t hitCount, iceMaterialCount;
    SwordFinishHit hits[SWORD_FINISH_HIT_CAPACITY];
    SwordFinishEffect effects[SWORD_FINISH_EFFECT_CAPACITY];
    SwordFinishIceMaterial iceMaterials[SWORD_FINISH_HIT_CAPACITY];
} SwordFinishCore;

static inline void SwordFinishCore_Context(SwordFinishCore* core, uintptr_t context, int16_t scene, int16_t room,
                                           uint32_t frame, int paused, int active) {
    int changed = core->context != context || core->scene != scene || core->room != room || frame < core->frame;
    if (changed || !active)
        memset(core, 0, sizeof(*core));
    if (frame != core->frame) {
        memset(core->hits, 0, sizeof(core->hits));
        core->hitCount = 0;
        if (!paused && active) {
            for (unsigned i = 0; i < SWORD_FINISH_EFFECT_CAPACITY; ++i)
                if (core->effects[i].life)
                    --core->effects[i].life;
            if (core->iceMaterialCount)
                for (unsigned i = 0; i < SWORD_FINISH_HIT_CAPACITY; ++i)
                    if (core->iceMaterials[i].life && !--core->iceMaterials[i].life) {
                        core->iceMaterials[i].key = 0;
                        --core->iceMaterialCount;
                    }
        }
    }
    if (paused) {
        memset(core->hits, 0, sizeof(core->hits));
        core->hitCount = 0;
    }
    core->context = context;
    core->scene = scene;
    core->room = room;
    core->frame = frame;
    core->paused = paused;
    core->active = active;
}

static inline SwordFinishHit* SwordFinishCore_Find(SwordFinishCore* core, uintptr_t key) {
    if (!key || !core->active || core->paused || !core->hitCount)
        return NULL;
    for (unsigned i = 0; i < SWORD_FINISH_HIT_CAPACITY; ++i)
        if (core->hits[i].key == key)
            return &core->hits[i];
    return NULL;
}

static inline void SwordFinishCore_SetIceMaterial(SwordFinishCore* core, uintptr_t key, int enabled) {
    if (!key || (enabled && (!core->active || core->paused)) || (!enabled && !core->iceMaterialCount))
        return;
    SwordFinishIceMaterial* empty = NULL;
    for (unsigned i = 0; i < SWORD_FINISH_HIT_CAPACITY; ++i) {
        SwordFinishIceMaterial* mark = &core->iceMaterials[i];
        if (mark->key == key) {
            mark->life = enabled ? SWORD_FINISH_ICE_MATERIAL_DURATION : 0;
            if (!enabled) {
                mark->key = 0;
                --core->iceMaterialCount;
            }
            return;
        }
        if (!mark->key && !empty)
            empty = mark;
    }
    if (enabled && empty) {
        empty->key = key;
        empty->life = SWORD_FINISH_ICE_MATERIAL_DURATION;
        ++core->iceMaterialCount;
    }
}

static inline int SwordFinishCore_UseIceMaterial(const SwordFinishCore* core, uintptr_t key) {
    if (!key || !core->active || !core->iceMaterialCount)
        return 0;
    for (unsigned i = 0; i < SWORD_FINISH_HIT_CAPACITY; ++i)
        if (core->iceMaterials[i].key == key)
            return core->iceMaterials[i].life != 0;
    return 0;
}

static inline void SwordFinishCore_Hit(SwordFinishCore* core, uintptr_t key, uint8_t element, int native,
                                       unsigned damage, const SwordFinishShape* shape) {
    if (!key || !core->active || core->paused || !damage)
        return;
    SwordFinishHit* hit = SwordFinishCore_Find(core, key);
    if (hit != NULL) {
        // A merged hit from another attack/element cannot be assigned a sole killer.
        if (hit->element != element || !element)
            hit->ambiguous = 1;
        return;
    }
    for (unsigned i = 0; i < SWORD_FINISH_HIT_CAPACITY; ++i) {
        if (!core->hits[i].key) {
            hit = &core->hits[i];
            memset(hit, 0, sizeof(*hit));
            hit->key = key;
            ++core->hitCount;
            hit->element = element;
            hit->native = native;
            hit->ambiguous = !element;
            if (shape)
                hit->shape = *shape;
            return;
        }
    }
}

static inline void SwordFinishCore_Begin(SwordFinishCore* core, uintptr_t key, uint8_t health) {
    SwordFinishHit* hit = SwordFinishCore_Find(core, key);
    if (hit) {
        hit->updating = 1;
        hit->health = health;
    }
}

static inline void SwordFinishCore_Forget(SwordFinishCore* core, uintptr_t key) {
    if (!key || !core->hitCount)
        return;
    for (unsigned i = 0; i < SWORD_FINISH_HIT_CAPACITY; ++i)
        if (core->hits[i].key == key) {
            memset(&core->hits[i], 0, sizeof(core->hits[i]));
            --core->hitCount;
        }
}

static inline void SwordFinishCore_Fatal(SwordFinishCore* core, uintptr_t key, const SwordFinishPoint* origin) {
    SwordFinishHit* hit = SwordFinishCore_Find(core, key);
    if (!hit || !hit->updating)
        return;
    if (hit->element >= 1 && hit->element <= 3 && !hit->ambiguous && (!hit->native || hit->element == 2) &&
        !hit->parts && hit->shape.count) {
        for (unsigned i = 0; i < SWORD_FINISH_EFFECT_CAPACITY; ++i) {
            SwordFinishEffect* effect = &core->effects[i];
            if (!effect->life) {
                memset(effect, 0, sizeof(*effect));
                effect->element = hit->element;
                effect->life = SWORD_FINISH_DURATION;
                effect->shape = hit->shape;
                if (origin) {
                    for (unsigned j = 0; j < effect->shape.count; ++j) {
                        effect->shape.points[j].x += origin->x - hit->shape.origin.x;
                        effect->shape.points[j].y += origin->y - hit->shape.origin.y;
                        effect->shape.points[j].z += origin->z - hit->shape.origin.z;
                    }
                    effect->shape.origin = *origin;
                }
                // Native ice responses retain their own mesh. The copied
                // origin still owns death audio after the actor is removed.
                if (hit->native)
                    effect->shape.count = 0;
                break;
            }
        }
    }
    SwordFinishCore_Forget(core, key);
}

// A native actor can confirm destruction of a flags-only receiver or one bat
// inside a group. The caller supplies that destroyed part's exact accepted hit;
// other parts' ordinary/foreign collisions must not confer or erase its credit.
static inline void SwordFinishCore_ConfirmPart(SwordFinishCore* core, uintptr_t key, unsigned part, uint8_t element,
                                               int native, const SwordFinishShape* shape) {
    if (part >= 32 || element < 1 || element > 3 || !shape || !shape->count)
        return;
    SwordFinishHit* hit = SwordFinishCore_Find(core, key);
    if (!hit) {
        SwordFinishCore_Hit(core, key, element, native, 1, shape);
        hit = SwordFinishCore_Find(core, key);
    }
    if (!hit || (hit->parts & (1u << part)))
        return;
    SwordFinishHit saved = *hit;
    hit->parts = 0;
    hit->element = element;
    hit->native = native;
    hit->ambiguous = 0;
    hit->updating = 1;
    hit->shape = *shape;
    SwordFinishCore_Fatal(core, key, NULL);
    saved.parts |= 1u << part;
    *hit = saved;
    ++core->hitCount;
}

static inline void SwordFinishCore_Part(SwordFinishCore* core, uintptr_t key, unsigned part,
                                        const SwordFinishShape* shape) {
    SwordFinishHit* hit = SwordFinishCore_Find(core, key);
    if (hit && hit->updating && !hit->ambiguous)
        SwordFinishCore_ConfirmPart(core, key, part, hit->element, hit->native, shape);
}

static inline void SwordFinishCore_End(SwordFinishCore* core, uintptr_t key, uint8_t health,
                                       const SwordFinishPoint* origin) {
    SwordFinishHit* hit = SwordFinishCore_Find(core, key);
    if (hit && hit->updating && hit->health && !health)
        SwordFinishCore_Fatal(core, key, origin);
    // Never keep a previous strike as credit for damage or a scripted death later.
    SwordFinishCore_Forget(core, key);
}
#endif
