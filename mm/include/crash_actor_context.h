#ifndef MM_CRASH_ACTOR_CONTEXT_H
#define MM_CRASH_ACTOR_CONTEXT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Copied before dispatch, so the crash reporter never has to dereference the actor
// whose update callback failed. This is diagnostic context, not callback validation.
typedef struct {
    int active;
    int sceneId;
    int room;
    int actorId;
    int category;
    int params;
    uintptr_t actor;
    uintptr_t callback;
} MMCrashActorContext;

const MMCrashActorContext* MM_GetCrashActorContext(void);

#ifdef __cplusplus
}
#endif

#endif
