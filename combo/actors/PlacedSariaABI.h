#ifndef COMBO_PLACED_SARIA_ABI_H
#define COMBO_PLACED_SARIA_ABI_H
#include <stddef.h>

/* Only opaque owner handles and the games' matching normal-animation/flex
 * headers cross this boundary. No Actor, PlayState, C++ object or save data. */
typedef struct {
    void* owner;
    void* skeleton;
    void* animation;
    const char* eyes[3];
    const char* mouth;
    const char* heads[3];
    const char* poseLimbs[17];
    int altEnabled;
    int custom;
} ComboPlacedSariaResources;

/* The original declaration above is frozen: older modules allocate exactly
 * sizeof(ComboPlacedSariaResources). Extend only a versioned, sized entrypoint. */
#define COMBO_PLACED_SARIA_ABI_V2 2
typedef struct {
    ComboPlacedSariaResources resources;
    float drawScale;
} ComboPlacedSariaResourcesV2;

typedef int (*ComboPlacedSariaLoadFunc)(unsigned pose, ComboPlacedSariaResources* output);
typedef int (*ComboPlacedSariaLoadV2Func)(unsigned pose, ComboPlacedSariaResourcesV2* output, size_t callerSize);
typedef void (*ComboPlacedSariaReleaseFunc)(void* owner);
typedef int (*ComboPlacedSariaAltFunc)(void);

#endif
