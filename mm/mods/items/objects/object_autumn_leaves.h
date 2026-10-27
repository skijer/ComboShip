#ifndef OBJECT_AUTUMN_LEAVES_H
#define OBJECT_AUTUMN_LEAVES_H
#include "global.h"

// Private generated RGBA art. Native scene, tree and impact-leaf textures are untouched.
static const ALIGN_ASSET(2) char sAutumnLeafCrimson[] = "__OTR__objects/nei_autumn/leaves/crimson_tex";
static const ALIGN_ASSET(2) char sAutumnLeafOrange[] = "__OTR__objects/nei_autumn/leaves/orange_tex";
static const ALIGN_ASSET(2) char sAutumnLeafGold[] = "__OTR__objects/nei_autumn/leaves/gold_tex";
static const ALIGN_ASSET(2) char sAutumnLeafCopper[] = "__OTR__objects/nei_autumn/leaves/copper_tex";
static const char* sAutumnLeafTextures[] = { sAutumnLeafCrimson, sAutumnLeafOrange, sAutumnLeafGold,
                                             sAutumnLeafCopper };
static Vtx sAutumnLeafVertices[] = {
    VTX(-160, -160, 0, 0, 1024, 255, 255, 255, 255),
    VTX(160, -160, 0, 1024, 1024, 255, 255, 255, 255),
    VTX(160, 160, 0, 1024, 0, 255, 255, 255, 255),
    VTX(-160, 160, 0, 0, 0, 255, 255, 255, 255),
};
static Gfx sAutumnLeafGeometry[] = {
    gsSPVertex(sAutumnLeafVertices, 4, 0),
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSPEndDisplayList(),
};

// Uses the caller's matrix and XLU scope. The next particle receives its own material.
static void AutumnLeaves_Draw(PlayState* play, unsigned index, u8 alpha) {
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    OPEN_DISPS(gfxCtx);
    gDPPipeSync(POLY_XLU_DISP++);
    gSPClearGeometryMode(POLY_XLU_DISP++, G_LIGHTING | G_CULL_BACK);
    // Native snow setup is two-cycle. Preserve cycle one: sampling TEXEL0
    // again in cycle two selects the unowned neighboring texture slot.
    gDPSetCombineMode(POLY_XLU_DISP++, G_CC_MODULATEIA_PRIM, G_CC_PASS2);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, alpha);
    gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, 255);
    gDPSetTextureLUT(POLY_XLU_DISP++, G_TT_NONE);
    gDPLoadTextureBlock(POLY_XLU_DISP++, sAutumnLeafTextures[index & 3], G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, 0,
                        G_TX_CLAMP, G_TX_CLAMP, 5, 5, G_TX_NOLOD, G_TX_NOLOD);
    gSPTexture(POLY_XLU_DISP++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, sAutumnLeafGeometry);
    CLOSE_DISPS(gfxCtx);
}
#endif
