#ifndef COMBO_ITEM_EFFECT_COLORS_H
#define COMBO_ITEM_EFFECT_COLORS_H
#include <stdint.h>
// Stable presentation profiles, independent of the two games' item/GI enums.
// Defense and power follow the requested blue/red themes; movement follows
// the authored green stat models. Magic is refreshed by its owner's editor.
static inline int ComboRpgShimmerColor(int profile, uint8_t color[4]) {
    static const uint8_t colors[7][4] = { { 64, 144, 255, 255 }, { 0, 255, 31, 255 }, { 255, 64, 64, 255 },
                                          { 0, 200, 0, 255 },    { 30, 206, 0, 255 }, { 22, 147, 0, 255 },
                                          { 30, 206, 0, 255 } };
    if (profile < 0 || profile >= 7)
        return 0;
    for (int i = 0; i < 4; ++i)
        color[i] = colors[profile][i];
    return 1;
}
#endif
