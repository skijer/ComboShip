#pragma once
#include <stdint.h>

// Append-only seed values. Zero preserves the four-pickup behavior of older seeds.
#define NEI_SEASONS_INDIVIDUAL 0
#define NEI_SEASONS_ROD 1
#define NEI_SEASONS_GATED 2
#define NEI_SEASONS_MASK 0x0Fu

static inline unsigned NeiSeasons_SeedMode(int mode) {
    return mode >= NEI_SEASONS_INDIVIDUAL && mode <= NEI_SEASONS_GATED ? (unsigned)mode : NEI_SEASONS_INDIVIDUAL;
}

static inline uint8_t NeiSeasons_OotGates(int stormsCheck, int jabuClear, int waterClear, int fireClear,
                                          int forestClear, int iceClear) {
    return ((stormsCheck || jabuClear || waterClear) ? 1u : 0u) | (fireClear ? 2u : 0u) | (forestClear ? 4u : 0u) |
           (iceClear ? 8u : 0u);
}

static inline uint8_t NeiSeasons_MmGates(int woodfallClear, int snowheadClear, int greatBayClear, int stoneTowerClear) {
    return (greatBayClear ? 1u : 0u) | (stoneTowerClear ? 2u : 0u) | (woodfallClear ? 4u : 0u) |
           (snowheadClear ? 8u : 0u);
}
