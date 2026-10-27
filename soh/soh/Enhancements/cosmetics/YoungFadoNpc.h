#ifndef SOH_YOUNG_FADO_NPC_H
#define SOH_YOUNG_FADO_NPC_H

#include "soh/ResourceManagerHelpers.h"
#include "align_asset_macro.h"

/* The base rig is a native Kokiri copy; its Alt rig has the same joint layout.
 * Registering this private path lets SkeletonPatcher switch both variants when
 * Alt Assets changes without altering another Kokiri or resetting animation. */
static const ALIGN_ASSET(2) char sYoungFadoNpcSkel[] = "__OTR__objects/object_fa/YoungFadoNpcPOC1/Skel";
static const ALIGN_ASSET(2) char sYoungFadoNpcHead[] = "__OTR__objects/object_fa/YoungFadoNpcPOC1/Body/Limb14";
static const ALIGN_ASSET(2) char sYoungFadoNpcSeated5Anim[] =
    "__OTR__objects/object_fa/YoungFadoNpcPOC1/Animations/Seated5";

/* Pelvis and both thigh wrappers replace only the seated cloth batches. */
static const ALIGN_ASSET(2) char sYoungFadoNpcSeated4Bodies[3][80] = {
    "__OTR__objects/object_fa/YoungFadoNpcPOC1/Seated4/Limb00",
    "__OTR__objects/object_fa/YoungFadoNpcPOC1/Seated4/Limb01",
    "__OTR__objects/object_fa/YoungFadoNpcPOC1/Seated4/Limb04",
};
static const ALIGN_ASSET(2) char sYoungFadoNpcSeated5Bodies[3][80] = {
    "__OTR__objects/object_fa/YoungFadoNpcPOC1/Seated5/Limb00",
    "__OTR__objects/object_fa/YoungFadoNpcPOC1/Seated5/Limb01",
    "__OTR__objects/object_fa/YoungFadoNpcPOC1/Seated5/Limb04",
};

static inline bool YoungFadoNpc_IsActive(void) {
    return ResourceMgr_IsAltAssetsEnabled() && ResourceMgr_FileExists(sYoungFadoNpcSkel) &&
           ResourceMgr_FileAltExists(sYoungFadoNpcSkel) && ResourceMgr_FileAltExists(sYoungFadoNpcHead);
}

static inline const char* YoungFadoNpc_SelectSkeleton(const char* nativeSkeleton) {
    return ResourceMgr_FileExists(sYoungFadoNpcSkel) ? sYoungFadoNpcSkel : nativeSkeleton;
}

static inline const char* YoungFadoNpc_SelectStaticAnimation(const char* nativeAnimation, u8 pose) {
    /* Keep this paired path even when Alt is off. The animation sampler resolves
     * the native base or fitted Alt on each frame, including after Alt toggles. */
    if (pose == 5 && ResourceMgr_FileExists(sYoungFadoNpcSkel) && ResourceMgr_FileAltExists(sYoungFadoNpcSkel) &&
        ResourceMgr_FileAltExists(sYoungFadoNpcHead) && ResourceMgr_FileExists(sYoungFadoNpcSeated5Anim) &&
        ResourceMgr_FileAltExists(sYoungFadoNpcSeated5Anim)) {
        return sYoungFadoNpcSeated5Anim;
    }
    return nativeAnimation;
}

static inline Gfx* YoungFadoNpc_SelectHead(Gfx* nativeHead) {
    if (YoungFadoNpc_IsActive()) {
        return (Gfx*)sYoungFadoNpcHead;
    }
    return nativeHead;
}

static inline Gfx* YoungFadoNpc_SelectSeatedBody(Gfx* nativeBody, s32 limbIndex, u8 pose) {
    const char* path;
    s32 slot;

    if (pose != 4 && pose != 5) {
        return nativeBody;
    }
    switch (limbIndex) {
        case 1:
            slot = 0;
            break;
        case 2:
            slot = 1;
            break;
        case 5:
            slot = 2;
            break;
        default:
            return nativeBody;
    }
    if (!YoungFadoNpc_IsActive()) {
        return nativeBody;
    }
    path = pose == 4 ? sYoungFadoNpcSeated4Bodies[slot] : sYoungFadoNpcSeated5Bodies[slot];
    return ResourceMgr_FileAltExists(path) ? (Gfx*)path : nativeBody;
}

#endif
