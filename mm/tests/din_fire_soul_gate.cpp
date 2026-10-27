// Resource-free execution of the unchanged Souls.cpp map, ownership predicate,
// registration condition and collision callback. Only hook dispatch/save flags
// are boundaries; the native collision caller is in din_fire_damage_test.c.
#include "global.h"
#include "Rando/Types.h"
#include <cstdarg>
#include <unordered_map>

static void (*sCollisionHook)(bool*, va_list);
static bool sOwned;
#define COND_VB_SHOULD(behavior, condition, ...) \
    sCollisionHook = (condition) ? +[](bool* should, va_list args) __VA_ARGS__ : nullptr

extern "C" s32 Flags_GetRandoInf(s32 flag) {
    return sOwned &&
           (flag == RANDO_INF_OBTAINED_SOUL_OF_ENEMY_CHUCHUS || flag == RANDO_INF_OBTAINED_SOUL_OF_ENEMY_DEKU_BABAS);
}
#include "din_soul_native.inc"

extern "C" void Test_SetSoulState(int enabled, int owned) {
    gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
    RANDO_SAVE_OPTIONS[RO_SHUFFLE_ENEMY_SOULS] = enabled;
    sOwned = owned;
    Test_RegisterSoulGate();
}
static int Dispatch(Collider* at, ...) {
    bool should = true;
    va_list args;
    va_start(args, at);
    if (sCollisionHook != nullptr)
        sCollisionHook(&should, args);
    va_end(args);
    return should;
}
extern "C" int Test_SoulGate(Collider* at, Collider* ac) {
    return Dispatch(at, at, ac);
}
