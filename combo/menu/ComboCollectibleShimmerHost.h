// Include after the owning game's engine headers. Foreign descriptors call the
// owner's GetItem_GetShimmerColor, so no host palette or game state crosses over.
#ifndef COMBO_COLLECTIBLE_SHIMMER_HOST_H
#define COMBO_COLLECTIBLE_SHIMMER_HOST_H
#include "ComboCollectibleShimmerDraw.h"

static inline int ComboCollectibleShimmer_ProfileForDrawId(int32_t drawId) {
    switch (drawId) {
        case GID_RUPEE_GREEN:
            return CW_COLLECTIBLE_GREEN;
        case GID_RUPEE_BLUE:
            return CW_COLLECTIBLE_BLUE;
        case GID_RUPEE_RED:
            return CW_COLLECTIBLE_RED;
        case GID_RUPEE_PURPLE:
            return CW_COLLECTIBLE_PURPLE;
#ifdef COMBO_COLLECTIBLE_SHIMMER_HOST_MM
        case GID_RUPEE_HUGE:
            return CW_COLLECTIBLE_LARGE;
        case GID_RUPEE_SILVER:
            return CW_COLLECTIBLE_SILVER;
        case GID_RECOVERY_HEART:
#else
        case GID_RUPEE_GOLD:
            return CW_COLLECTIBLE_LARGE;
        case GID_HEART:
#endif
        case GID_HEART_CONTAINER:
        case GID_HEART_PIECE:
            return CW_COLLECTIBLE_HEART;
        default:
            return -1;
    }
}

static inline int ComboCollectibleShimmer_Color(int profile, uint8_t color[4]) {
    if (profile < 0 || profile >= CW_COLLECTIBLE_COUNT || !CVarGetInteger("gEnhancements.CollectibleShimmer", 0))
        return CW_ITEM_SHIMMER_NONE;
#ifdef COMBO_COLLECTIBLE_SHIMMER_HOST_MM
    MM_SampleCollectibleShimmerColor(profile, color);
#else
    OOT_SampleCollectibleShimmerColor(profile, color);
#endif
    return CW_ITEM_SHIMMER_TINTED;
}

static inline int ComboCollectibleShimmer_ColorForDrawId(int32_t drawId, uint8_t color[4]) {
    return ComboCollectibleShimmer_Color(ComboCollectibleShimmer_ProfileForDrawId(drawId), color);
}

// Native drop meshes use hundreds of units, while the GI sampler uses tens.
// Scope the conversion to this overlay; never rescale the model or its actor.
static inline void ComboCollectibleShimmer_DrawDrop(PlayState* play, int profile, float giScale) {
    uint8_t color[4];
    if (!ComboCollectibleShimmer_Color(profile, color))
        return;
    Matrix_Push();
    Matrix_Scale(giScale, giScale, giScale, MTXMODE_APPLY);
    NeiGi_DrawTintedShimmerOverlay(play, color, 0);
    Matrix_Pop();
}
#endif
