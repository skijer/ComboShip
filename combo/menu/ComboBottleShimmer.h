#ifndef COMBO_BOTTLE_SHIMMER_H
#define COMBO_BOTTLE_SHIMMER_H

#include <stdint.h>

/* These profile IDs are independent of GIDs and bottle-content draw IDs. */
enum { CW_SHIMMER_MUSHROOM = 1, CW_SHIMMER_PRINCESS, CW_SHIMMER_GOLD_DUST, CW_SHIMMER_FAIRY, CW_SHIMMER_SEAHORSE };

static inline uint32_t ComboBottleShimmer_ColorHex(int profile) {
    switch (profile) {
        case CW_SHIMMER_MUSHROOM:
            return 0xDE64F5u;
        case CW_SHIMMER_PRINCESS:
            return 0x80D846u;
        case CW_SHIMMER_GOLD_DUST:
            return 0xFFD45Au;
        case CW_SHIMMER_FAIRY:
            return 0xFFA0EBu;
        case CW_SHIMMER_SEAHORSE:
            return 0xFFE66Du;
        default:
            return 0;
    }
}

static inline int ComboBottleShimmer_Color(int profile, uint8_t color[4]) {
    const uint32_t rgb = ComboBottleShimmer_ColorHex(profile);
    if (!rgb || !color)
        return 0;
    color[0] = (uint8_t)(rgb >> 16);
    color[1] = (uint8_t)(rgb >> 8);
    color[2] = (uint8_t)rgb;
    color[3] = 255;
    return 1;
}

#ifdef __cplusplus
extern "C" {
#endif
struct PlayState;
void ComboBottleShimmer_DrawMotes(struct PlayState* play, int profile);
#ifdef __cplusplus
}
#endif

#endif
