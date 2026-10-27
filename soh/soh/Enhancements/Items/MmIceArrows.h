#pragma once

#include "global.h"

#ifdef __cplusplus
extern "C" {
#endif

struct EnArrow;
struct EnOkuta;
s32 MmIceArrows_TryFreezeWater(PlayState* play, struct EnArrow* arrow);
s32 MmIceArrows_TryFreezeOctorok(PlayState* play, struct EnOkuta* octorok);
s32 MmIceArrows_GetActorId(void);
s32 MmIceArrows_GetOctorokBlockActorId(void);
// The handle retains the collision/model resources until the actor unregisters its mesh.
void* MmIceArrows_AcquireResources(CollisionHeader** collision, Gfx** model);
void* MmIceArrows_AcquireBlockResources(CollisionHeader** collision, Gfx** model);
void MmIceArrows_ReleaseResources(void* handle);

#ifdef __cplusplus
}
#endif
