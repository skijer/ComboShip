#ifndef OBJECT_SPRING_PETALS_H
#define OBJECT_SPRING_PETALS_H
#include "global.h"
#include "../../../../combo/NeiSpringBlossoms.h"

static void SpringPetals_Draw(PlayState* play, unsigned index, u8 alpha) {
    const u8* color = NeiSpringBlossoms_Color(index);
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    OPEN_DISPS(gfxCtx);
    gDPPipeSync(POLY_XLU_DISP++);
    gSPClearGeometryMode(POLY_XLU_DISP++, G_LIGHTING | G_CULL_BACK | G_CULL_FRONT);
    gSPSetGeometryMode(POLY_XLU_DISP++, G_SHADE | G_SHADING_SMOOTH);
    gSPTexture(POLY_XLU_DISP++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
    gDPSetCombineLERP(POLY_XLU_DISP++, SHADE, 0, PRIMITIVE, 0, 0, 0, 0, PRIMITIVE, 0, 0, 0, COMBINED, 0, 0, 0,
                      COMBINED);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, color[0], color[1], color[2], alpha);
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, NeiSpringBlossoms_Geometry(index));
    CLOSE_DISPS(gfxCtx);
}
#endif
