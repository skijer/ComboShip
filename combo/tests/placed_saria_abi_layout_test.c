/* Freestanding syntax checks run as both -m32 and -m64; no target libc needed. */
#include "actors/PlacedSariaABI.h"
#include <stddef.h>

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
} FrozenV1;

_Static_assert(sizeof(void*) == 4 || sizeof(void*) == 8, "supported pointer ABIs");
_Static_assert(sizeof(int) == 4 && sizeof(float) == 4, "existing scalar ABI");
_Static_assert(sizeof(FrozenV1) == (sizeof(void*) == 8 ? 224 : 116), "frozen legacy size including padding");
_Static_assert(sizeof(ComboPlacedSariaResources) == sizeof(FrozenV1), "legacy ABI must not grow");
#define CHECK_FIELD(field) \
    _Static_assert(offsetof(ComboPlacedSariaResources, field) == offsetof(FrozenV1, field), "legacy offset: " #field)
CHECK_FIELD(owner);
CHECK_FIELD(skeleton);
CHECK_FIELD(animation);
CHECK_FIELD(eyes);
CHECK_FIELD(mouth);
CHECK_FIELD(heads);
CHECK_FIELD(poseLimbs);
CHECK_FIELD(altEnabled);
CHECK_FIELD(custom);
#undef CHECK_FIELD

#ifdef COMBO_PLACED_SARIA_ABI_V2
_Static_assert(offsetof(ComboPlacedSariaResourcesV2, resources) == 0, "full legacy struct at start");
_Static_assert(offsetof(ComboPlacedSariaResourcesV2, drawScale) == sizeof(FrozenV1), "scale follows full legacy size");
_Static_assert(sizeof(ComboPlacedSariaResourcesV2) == (sizeof(void*) == 8 ? 232 : 120), "V2 size including padding");
#else
_Static_assert(0, "size-checked V2 entrypoint is missing");
#endif
