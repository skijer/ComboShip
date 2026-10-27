#ifndef SWORD_ELEMENT_UI_ART_H
#define SWORD_ELEMENT_UI_ART_H

#include "sword_element_ui_art_data.h"

/* Texture data is typed RGBA32, never a resource-name string. Each tile is
 * exactly one native 4096-byte load, even though the complete icon is 64x64.
 * Quarter-pixel destinations keep odd HUD editor sizes and anchors exact. */
static inline const SwordElementUiRgbaTile* SwordElementUi_Art(uint8_t element) {
    switch (element) {
        case 1:
            return sSwordElementUiFireArt;
        case 2:
            return sSwordElementUiIceArt;
        case 3:
            return sSwordElementUiLightArt;
        default:
            return NULL;
    }
}

static inline void SwordElementUi_Rect(Gfx** display, int x4, int y4, int width4, int height4, int s, int t, int dsdx,
                                       int dtdy, uint8_t wide) {
    Gfx* dl = *display;
    if (wide) {
        gSPWideTextureRectangle(dl++, x4, y4, x4 + width4, y4 + height4, G_TX_RENDERTILE, s, t, dsdx, dtdy);
    } else {
        gSPTextureRectangle(dl++, x4, y4, x4 + width4, y4 + height4, G_TX_RENDERTILE, s, t, dsdx, dtdy);
    }
    *display = dl;
}

static inline void SwordElementUi_DrawElement(Gfx** display, uint8_t element, int left, int top, int width, int height,
                                              uint8_t alpha, uint8_t muted, uint8_t wide) {
    const SwordElementUiRgbaTile* tiles = SwordElementUi_Art(element);
    if (tiles == NULL || width <= 0 || height <= 0 || alpha == 0) {
        return;
    }
    Gfx* dl = *display;
    int dsdx = SWORD_ELEMENT_UI_ICON_SIZE * 1024 / width;
    int dtdy = SWORD_ELEMENT_UI_ICON_SIZE * 1024 / height;
    /* Fast3D decodes signed S5.10 steps. Tiny editor badges must not wrap
     * into negative sampling; retain the largest forward step representable. */
    if (dsdx > INT16_MAX)
        dsdx = INT16_MAX;
    if (dtdy > INT16_MAX)
        dtdy = INT16_MAX;
    gDPPipeSync(dl++);
    gDPSetCombineMode(dl++, G_CC_MODULATERGBA_PRIM, G_CC_MODULATERGBA_PRIM);
    gDPSetPrimColor(dl++, 0, 0, muted ? 128 : 255, muted ? 140 : 255, muted ? 156 : 255, alpha);
    gDPSetTextureFilter(dl++, G_TF_BILERP);
    gDPSetGrayscaleColor(dl++, 123, 137, 154, 255);
    gSPGrayscale(dl++, muted != 0);
    for (int tile = 0; tile < 4; ++tile) {
        gDPLoadTextureBlock(dl++, tiles[tile].pixels, G_IM_FMT_RGBA, G_IM_SIZ_32b, SWORD_ELEMENT_UI_TILE_SIZE,
                            SWORD_ELEMENT_UI_TILE_SIZE, 0, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP,
                            G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
        SwordElementUi_Rect(&dl, left * 4 + (tile & 1) * width * 2, top * 4 + (tile >> 1) * height * 2, width * 2,
                            height * 2, 0, 0, dsdx, dtdy, wide);
    }
    gSPGrayscale(dl++, false);
    *display = dl;
}

#endif
