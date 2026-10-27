#pragma once

#ifdef COMBO_BUILD
#include "z64actor.h"
#include "z64object.h"
// MM Dm_Stk (0x0191): the same reclining, crossed-leg loop as OoT's placed entry.
#define COMBO_FOREST_SKULL_KID_PARAMS 0x7E50

// Actor spawning checks the bank before Init. This host loads named resources
// and must not require a scene to contain Skull Kid's story object bank.
static inline s16 ComboForestSkullKid_ObjectId(s16 actorId, s32 params, s16 nativeObjectId) {
    return actorId == ACTOR_DM_STK && params == COMBO_FOREST_SKULL_KID_PARAMS ? GAMEPLAY_KEEP : nativeObjectId;
}
#endif
