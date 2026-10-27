// TU glue shared by native and foreign mask GIs. Include after the host engine headers.
#ifndef COMBO_MASK_SHIMMER_H
#define COMBO_MASK_SHIMMER_H

#include <stdint.h>
#include "ComboSongDraw.h"

// 0 = ordinary mask; 1..4 = transformations; 5..8 = Odolwa, Goht, Gyorg, Twinmold.
static inline void ComboMaskShimmerColor(int profile, uint8_t color[4]) {
    static const uint8_t colors[9][4] = {
        { 220, 225, 240, 255 }, { 50, 220, 90, 255 }, { 240, 64, 64, 255 }, { 64, 144, 255, 255 }, { 0, 0, 0, 255 },
        { 145, 20, 133, 255 },  { 220, 55, 55, 255 }, { 19, 99, 165, 255 }, { 168, 180, 20, 255 },
    };
    int i;
    for (i = 0; i < 4; ++i) {
        color[i] = colors[profile >= 0 && profile < 9 ? profile : 0][i];
    }
}

// MM inventory ordering, including OoT's imported aliases. These are item slots, not GIDs.
static inline int ComboMmMaskShimmerColor(int index, uint8_t color[4]) {
    // Inventory order. Ordinary masks follow their dominant authored palette;
    // transformations retain their established effect identities.
    static const uint8_t colors[24][4] = {
        { 220, 65, 65, 255 },   { 95, 70, 140, 255 },   { 64, 144, 255, 255 }, { 135, 95, 205, 255 },
        { 245, 145, 205, 255 }, { 50, 220, 90, 255 },   { 255, 200, 55, 255 }, { 220, 225, 240, 255 },
        { 255, 220, 80, 255 },  { 90, 205, 95, 255 },   { 205, 125, 80, 255 }, { 240, 64, 64, 255 },
        { 220, 185, 145, 255 }, { 175, 180, 220, 255 }, { 135, 95, 205, 255 }, { 245, 165, 205, 255 },
        { 235, 80, 85, 255 },   { 64, 144, 255, 255 },  { 230, 145, 85, 255 }, { 225, 210, 155, 255 },
        { 235, 190, 75, 255 },  { 215, 220, 200, 255 }, { 205, 105, 65, 255 }, { 0, 0, 0, 255 },
    };
    if (index < 0 || index >= 24) {
        return 0;
    }
    for (int i = 0; i < 4; ++i)
        color[i] = colors[index][i];
    return 1;
}

// OoT mask order: Keaton, Skull, Spooky, Bunny, Goron, Zora, Gerudo, Truth, Mario.
static inline int ComboOotMaskShimmerColor(int index, uint8_t color[4]) {
    static const uint8_t colors[9][4] = {
        { 255, 200, 55, 255 }, { 220, 215, 180, 255 }, { 155, 100, 70, 255 },
        { 255, 220, 80, 255 }, { 215, 150, 85, 255 },  { 100, 180, 230, 255 },
        { 230, 105, 70, 255 }, { 235, 80, 85, 255 },   { 230, 60, 60, 255 },
    };
    if (index < 0 || index >= 9)
        return 0;
    for (int i = 0; i < 4; ++i)
        color[i] = colors[index][i];
    return 1;
}

static inline int ComboMmRemainsShimmerColor(int index, uint8_t color[4]) {
    if (index < 0 || index >= 4) {
        return 0;
    }
    ComboMaskShimmerColor(5 + index, color);
    return 1;
}

// The shared NEI mesh renderer supplies this C boundary in both hosts. The
// caller owns the incoming model pose; no texture archive is needed for stars.
#ifdef __cplusplus
extern "C" {
#endif
void NeiGi_DrawShimmerOverlay(PlayState* play, const uint8_t color[4], const char* owner);
void NeiGi_DrawChateauShimmer(PlayState* play);
void NeiGi_DrawSongOverlay(PlayState* play, int song, const char* owner);
#ifdef __cplusplus
}
#endif

static inline void ComboDrawMaskShimmer(PlayState* play, const char* sparkle, const uint8_t color[4],
                                        const char* owner) {
    (void)sparkle; // Retain the existing call boundary; NEI stars are procedural.
    NeiGi_DrawShimmerOverlay(play, color, owner);
}
#endif
