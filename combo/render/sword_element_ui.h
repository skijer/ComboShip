#ifndef SWORD_ELEMENT_UI_H
#define SWORD_ELEMENT_UI_H

#include "sword_element_ui_art.h"
#include "sword_element_ui_type_data.h"

static inline void SwordElementUi_MaskColor(Gfx** display, uint32_t rgb, uint8_t alpha) {
    Gfx* dl = *display;
    gDPPipeSync(dl++);
    gDPSetCombineMode(dl++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
    gDPSetPrimColor(dl++, 0, 0, rgb >> 16, (rgb >> 8) & 255, rgb & 255, alpha);
    *display = dl;
}

static inline void SwordElementUi_LoadMask(Gfx** display, const SwordElementUiMask* mask) {
    Gfx* dl = *display;
    gDPLoadTextureBlock(dl++, mask->pixels, G_IM_FMT_IA, G_IM_SIZ_8b, mask->width, mask->height, 0,
                        G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
                        G_TX_NOLOD);
    *display = dl;
}

static inline void SwordElementUi_Mask(Gfx** display, const SwordElementUiMask* mask, int x, int y, int width,
                                       int height, uint32_t rgb, uint8_t alpha) {
    SwordElementUi_MaskColor(display, rgb, alpha);
    SwordElementUi_LoadMask(display, mask);
    SwordElementUi_Rect(display, x * 4, y * 4, width * 4, height * 4, 0, 0, mask->width * 1024 / width,
                        mask->height * 1024 / height, 0);
}

static inline void SwordElementUi_Text(Gfx** display, const SwordElementUiMask* mask, int x, int y, uint32_t rgb,
                                       uint8_t alpha) {
    SwordElementUi_MaskColor(display, rgb, alpha);
    SwordElementUi_LoadMask(display, mask);
    /* Two physical texels per logical pixel preserve the font's smooth curves. */
    SwordElementUi_Rect(display, x * 4, y * 4, mask->width * 2, mask->height * 2, 0, 0, 2048, 2048, 0);
}

static inline void SwordElementUi_Box(Gfx** display, int x, int y, int width, int height, uint32_t rgb, uint8_t alpha) {
    const int positionsX[] = { x, x + 6, x + width - 6 };
    const int positionsY[] = { y, y + 6, y + height - 6 };
    const int widths[] = { 6, width - 12, 6 };
    const int heights[] = { 6, height - 12, 6 };
    const int source[] = { 0, 16, 16 };
    const int sourceSize[] = { 16, 1, 16 };
    SwordElementUi_MaskColor(display, rgb, alpha);
    SwordElementUi_LoadMask(display, &sSwordElementUiRound);
    /* Nine-slicing keeps the corner radius constant when the panel stretches. */
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) {
            SwordElementUi_Rect(display, positionsX[column] * 4, positionsY[row] * 4, widths[column] * 4,
                                heights[row] * 4, source[column] * 32, source[row] * 32,
                                sourceSize[column] * 1024 / widths[column], sourceSize[row] * 1024 / heights[row], 0);
        }
    }
}

static inline void SwordElementUi_DrawChooser(Gfx** display, const void* swordIcon, uint8_t selection, uint8_t active,
                                              uint8_t available, uint8_t alpha) {
    static const uint32_t accents[] = { 0xb7c6d9, 0xf2a477, 0x8bd6f3, 0xf2dc9b };
    const SwordElementUiMask* labels[] = { &sSwordElementUiNone, &sSwordElementUiFire, &sSwordElementUiIce,
                                           &sSwordElementUiLight };
    Gfx* dl = *display;
    gDPPipeSync(dl++);
    gDPSetCycleType(dl++, G_CYC_1CYCLE);
    gDPSetRenderMode(dl++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gDPSetTextureFilter(dl++, G_TF_BILERP);
    gSPGrayscale(dl++, false);
    SwordElementUi_Box(&dl, 44, 116, 232, 110, 0xa3b5ce, alpha * 65 / 255);
    SwordElementUi_Box(&dl, 45, 117, 230, 108, 0x101722, alpha * 236 / 255);
    SwordElementUi_Text(&dl, &sSwordElementUiTitle, 56, 122, 0xf2f5fa, alpha);
    SwordElementUi_Text(&dl, &sSwordElementUiSubtitle, 56, 135, 0x9aaac0, alpha);
    for (uint8_t element = 0; element < 4; ++element) {
        const uint8_t unlocked = (available >> element) & 1;
        const uint8_t selected = selection == element;
        const int x = 56 + element * 53;
        const uint32_t label = !unlocked ? 0x768397 : selected ? 0xf4f7fc : 0xbdc9d9;
        SwordElementUi_Box(&dl, x, 146, 47, 56, selected ? accents[element] : 0x6e829f,
                           alpha * (selected ? 215 : 65) / 255);
        SwordElementUi_Box(&dl, x + 1, 147, 45, 54, selected ? 0x263245 : 0x182231,
                           alpha * (selected ? 240 : 210) / 255);
        if (element == 0 && swordIcon != NULL) {
            SwordElementUi_MaskColor(&dl, 0xd7e0ee, alpha);
            gDPLoadTextureBlock(dl++, swordIcon, G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, 0, G_TX_NOMIRROR | G_TX_CLAMP,
                                G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
            SwordElementUi_Rect(&dl, (x + 10) * 4, 153 * 4, 27 * 4, 27 * 4, 0, 0, 1213, 1213, 0);
        } else {
            SwordElementUi_DrawElement(&dl, element, x + 8, 151, 31, 31, alpha * (unlocked ? 255 : 100) / 255,
                                       !unlocked, 0);
        }
        SwordElementUi_Text(&dl, labels[element], x + (47 - labels[element]->width / 2) / 2, 183, label, alpha);
        if (active == element) {
            SwordElementUi_Text(&dl, &sSwordElementUiActive, x + (47 - sSwordElementUiActive.width / 2) / 2, 195,
                                0xc5e5d7, alpha);
        } else if (!unlocked) {
            SwordElementUi_Text(&dl, &sSwordElementUiLocked, x + (47 - sSwordElementUiLocked.width / 2) / 2, 195,
                                0x8492a6, alpha);
        }
        if (!unlocked) {
            SwordElementUi_Mask(&dl, &sSwordElementUiLock, x + 36, 150, 6, 6, 0xb0bccd, alpha);
        }
    }
    SwordElementUi_Mask(&dl, &sSwordElementUiSolid, 56, 207, 206, 1, 0x91a3bd, alpha * 32 / 255);
    SwordElementUi_Text(&dl, &sSwordElementUiArrows, 56, 211, 0xbdcadd, alpha);
    SwordElementUi_Text(&dl, &sSwordElementUiSelect, 72, 214, 0x9aaac0, alpha);
    SwordElementUi_Mask(&dl, &sSwordElementUiCircle, 166, 211, 10, 10, 0xb3e0cc, alpha);
    SwordElementUi_Text(&dl, &sSwordElementUiA, 166 + (10 - sSwordElementUiA.width / 2) / 2, 212, 0x152a23, alpha);
    SwordElementUi_Text(&dl, &sSwordElementUiApply, 180, 214, 0xdce5f1, alpha);
    SwordElementUi_Mask(&dl, &sSwordElementUiCircle, 222, 211, 10, 10, 0x71829a, alpha);
    SwordElementUi_Mask(&dl, &sSwordElementUiCircle, 223, 212, 8, 8, 0x182231, alpha);
    SwordElementUi_Text(&dl, &sSwordElementUiB, 222 + (10 - sSwordElementUiB.width / 2) / 2, 212, 0xd0ddec, alpha);
    SwordElementUi_Text(&dl, &sSwordElementUiBack, 236, 214, 0x9aaac0, alpha);
    *display = dl;
}

#endif
