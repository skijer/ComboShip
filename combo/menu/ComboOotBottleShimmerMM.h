#ifndef COMBO_OOT_BOTTLE_SHIMMER_MM_H
#define COMBO_OOT_BOTTLE_SHIMMER_MM_H
#include <stdint.h>
#include "ComboBottleShimmer.h"
// MM's imported OoT bottles use native MM models, but keep the OoT NEI
// shimmer palette. Include after MM's RandoItemId declaration.
static inline int MM_OotBottleShimmerColor(RandoItemId id, uint8_t color[4]) {
    uint8_t r = 220, g = 225, b = 240;
    switch (id) {
        case RI_OOT_BOTTLE_BIG_POE:
            r = 150;
            g = 200;
            b = 0;
            break;
        case RI_OOT_BOTTLE_BLUE_FIRE:
        case RI_OOT_BOTTLE_BLUE_POTION:
            r = 100;
            g = 160;
            b = 255;
            break;
        case RI_OOT_BOTTLE_FAIRY:
            return ComboBottleShimmer_Color(CW_SHIMMER_FAIRY, color);
        case RI_OOT_BOTTLE_MAGIC_MUSHROOM:
            return ComboBottleShimmer_Color(CW_SHIMMER_MUSHROOM, color);
        case RI_OOT_BOTTLE_GREEN_POTION:
            r = 0;
            g = 200;
            b = 0;
            break;
        case RI_OOT_BOTTLE_POE:
            r = 100;
            g = 0;
            b = 200;
            break;
        case RI_OOT_BOTTLE_BUGS:
        case RI_OOT_BOTTLE_FISH:
        case RI_OOT_RUTOS_LETTER:
            break;
        default:
            return 0;
    }
    color[0] = r;
    color[1] = g;
    color[2] = b;
    color[3] = 255;
    return 1;
}
#endif
