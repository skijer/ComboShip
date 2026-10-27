#ifndef COMBO_ITEM_RECEIPT_RENDER_H
#define COMBO_ITEM_RECEIPT_RENDER_H
#include "ComboItemReceiptPresentation.h"

/* Included after each engine's GBI declarations. The caller owns position and
 * lifetime; this routine only emits the sprite described by the saved seed. */
static inline Gfx* ComboReceipt_DrawIcon(Gfx* gfx, const CwItemReceiptPresentation* p,
                                         const CwItemReceiptLayout* layout, int x, int y, int alpha) {
    if (!ComboReceipt_HasIcon(p) || layout->iconWidth < 1 || layout->iconHeight < 1)
        return gfx;
    gDPPipeSync(gfx++);
    gDPSetCombineMode(gfx++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
    gDPSetPrimColor(gfx++, 0, 0, p->iconHasColor ? p->iconColor[0] : 255, p->iconHasColor ? p->iconColor[1] : 255,
                    p->iconHasColor ? p->iconColor[2] : 255, alpha);
    if (p->iconIA8) {
        gDPLoadTextureBlock(gfx++, p->iconPath, G_IM_FMT_IA, G_IM_SIZ_8b, p->iconWidth, p->iconHeight, 0,
                            G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
                            G_TX_NOLOD);
    } else {
        gDPLoadTextureBlock(gfx++, p->iconPath, G_IM_FMT_RGBA, G_IM_SIZ_32b, p->iconWidth, p->iconHeight, 0,
                            G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
                            G_TX_NOLOD);
    }
    gSPTextureRectangle(gfx++, x << 2, y << 2, (x + layout->iconWidth) << 2, (y + layout->iconHeight) << 2,
                        G_TX_RENDERTILE, 0, 0, (p->iconWidth << 10) / layout->iconWidth,
                        (p->iconHeight << 10) / layout->iconHeight);
    gDPPipeSync(gfx++);
    gDPSetCombineLERP(gfx++, 0, 0, 0, PRIMITIVE, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, PRIMITIVE, TEXEL0, 0, PRIMITIVE, 0);
    return gfx;
}
#endif
