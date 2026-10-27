#pragma once
#include "z64actor.h"
#include "z64object.h"
#include "actors/RegionalActorParams.h"
static inline int ComboRegionalActors_IsPlaced(s16 id, s32 params) {
    unsigned p = (u16)params, pose = (p >> 4) & 15, family = p & 0xFF0F;
    return (id == ACTOR_EN_RAIL_SKB && family == COMBO_REGIONAL_STALCHILD_PARAMS && pose < 5) ||
           (id == ACTOR_EN_ZOG && family == COMBO_REGIONAL_MIKAU_PARAMS && pose < 2) ||
           (id == ACTOR_EN_GG && family == COMBO_REGIONAL_DARMANI_PARAMS && pose < 2);
}
static inline s16 ComboRegionalActors_ObjectId(s16 id, s32 params, s16 nativeObjectId) {
    return ComboRegionalActors_IsPlaced(id, params) ? GAMEPLAY_KEEP : nativeObjectId;
}
