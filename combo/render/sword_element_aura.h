#ifndef SWORD_ELEMENT_AURA_H
#define SWORD_ELEMENT_AURA_H
#include "sword_element_aura_data.h"

// Fire retains Din's exact two-cycle material and 3/5 scroll. Frost and light
// keep the native metal visible through two translucent, blade-bound layers.
// Their opacity also includes I8 intensity and vertex alpha; keep enough body
// after filtering for the casing to read at ordinary gameplay camera distance.
static void SwordAura_Material(Gfx** display, u8 element, s32 outerLayer, Color_RGB8 core, Color_RGB8 outer,
                               u16 phase) {
    s32 translucent = element != SWORD_ELEMENT_FIRE || outerLayer;
    s32 speed = element == SWORD_ELEMENT_FIRE  ? (outerLayer ? 5 : 3)
                : element == SWORD_ELEMENT_ICE ? (outerLayer ? 2 : 1)
                                               : (outerLayer ? 4 : 2);
    s32 scroll = (phase * speed) & 127;
    u8 alpha = element == SWORD_ELEMENT_FIRE  ? (outerLayer ? (u8)((.96f + .04f * sinf(phase * .71f)) * 255.f) : 255)
               : element == SWORD_ELEMENT_ICE ? (outerLayer ? 225 : 200)
                                              : (u8)((outerLayer ? 220.f : 185.f) + 20.f * sinf(phase * .075f));
    const u8* texture = element == SWORD_ELEMENT_FIRE  ? (outerLayer ? sSwordAuraFireFlameTex : sSwordAuraFireCoreTex)
                        : element == SWORD_ELEMENT_ICE ? sSwordAuraIceTex
                                                       : sSwordAuraLightTex;
    gSPClearGeometryMode((*display)++, G_LIGHTING | G_FOG | G_CULL_BOTH | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR);
    gDPSetCycleType((*display)++, G_CYC_2CYCLE);
    gDPSetRenderMode((*display)++, G_RM_PASS, translucent ? G_RM_AA_ZB_XLU_SURF2 : G_RM_AA_ZB_OPA_SURF2);
    gDPSetTextureLUT((*display)++, G_TT_NONE);
    gDPSetTextureFilter((*display)++, G_TF_BILERP);
    gDPSetAlphaCompare((*display)++, G_AC_NONE);
    gSPTexture((*display)++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    if (translucent) {
        gDPSetCombineLERP((*display)++, PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, SHADE, 0, 0, 0, 0,
                          COMBINED, COMBINED, 0, PRIMITIVE, 0);
    } else {
        gDPSetCombineLERP((*display)++, PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, 0, 0, 0, SHADE, 0, 0, 0, COMBINED,
                          COMBINED, 0, PRIMITIVE, 0);
    }
    gDPLoadTextureBlock((*display)++, texture, G_IM_FMT_I, G_IM_SIZ_8b, 64, 32, 0, G_TX_WRAP, G_TX_WRAP, 6, 5,
                        G_TX_NOLOD, G_TX_NOLOD);
    gDPSetTileSize((*display)++, 0, 0, scroll, 63 << 2, scroll + (31 << 2));
    gDPSetPrimColor((*display)++, 0, 0, core.r, core.g, core.b, alpha);
    gDPSetEnvColor((*display)++, outer.r, outer.g, outer.b, 255);
}
#endif
