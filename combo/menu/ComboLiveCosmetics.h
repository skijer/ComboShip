#ifndef COMBO_LIVE_COSMETICS_H
#define COMBO_LIVE_COSMETICS_H

#include "ComboItemDrawABI.h"

// TU glue: Color_RGB8 comes from the owning game's engine headers.
static inline Color_RGB8 CwLiveCosmeticColor(const char* valueCvar, Color_RGB8 fallback) {
    uint8_t rgb[3] = { fallback.r, fallback.g, fallback.b };
    OOT_SampleGiCosmeticColor(valueCvar, fallback.r, fallback.g, fallback.b, rgb);
    return { rgb[0], rgb[1], rgb[2] };
}

#endif
