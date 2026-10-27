#pragma once
#include "z64animation.h"

#ifdef __cplusplus
extern "C" {
#endif
/* Returns the original animation when Alt is off, the rig is unrelated or a
 * private fitted clip is absent/invalid. The actor retains its native identity. */
AnimationHeader* NativeNpcPose_ResolveAnimation(const SkelAnime* skelAnime, AnimationHeader* animation);
/* Optional pose-specific meshes use the same full native-animation key. */
Gfx* NativeNpcPose_SelectLimb(const SkelAnime* skelAnime, s32 limbIndex, Gfx* original);
Gfx* NativeNpcPose_SelectProp(const SkelAnime* skelAnime, const char* prop, Gfx* original);
/* Optional model-owned visual multiplier; vanilla/Alt-off/missing/invalid = 1. */
float NativeNpcPose_GetDrawScale(void** skeleton);
/* Optional archive-owned skin palette. Native limb count/order stay unchanged. */
const void* NativeNpcPose_GetSkinPlan(void** skeleton, s32 matrixCount);
s32 NativeNpcPose_SkinMatrixCount(const void* plan);
void NativeNpcPose_BlendSkinMatrix(const void* plan, s32 index, const MtxF* nativeMatrices, MtxF* result);
#ifdef __cplusplus
}
#endif
