#ifndef COMBO_DIN_SWORD_GI_H
#define COMBO_DIN_SWORD_GI_H
#include "../DinSwordGiResources.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <cmath>

extern "C" int ResourceMgr_GetDinSwordGiProfileForGame(const char* owner, const char* path);
extern "C" bool NeiGi_CanDrawLayers(PlayState* play, size_t matrices, size_t opa, size_t xlu);
#ifdef COMBO_DIN_SWORD_GI_HOST_MM
extern "C" Color_RGBA8 CosmeticEditor_GetChangedColor(u8 r, u8 g, u8 b, u8 a, const char* cosmeticId);
#endif

static inline void ComboDinSwordGi_Material(Gfx** display, bool flame, Color_RGB8 core, Color_RGB8 outer,
                                            uint16_t phase) {
    const int scroll = (phase * (flame ? 5 : 3)) & 127;
    const uint8_t alpha = flame ? uint8_t((.96f + .04f * std::sin(phase * .71f)) * 255.f) : 255;
    gSPClearGeometryMode((*display)++, G_LIGHTING | G_FOG | G_CULL_BOTH | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR);
    gDPSetCycleType((*display)++, G_CYC_2CYCLE);
    gDPSetRenderMode((*display)++, G_RM_PASS, flame ? G_RM_AA_ZB_XLU_SURF2 : G_RM_AA_ZB_OPA_SURF2);
    gDPSetTextureLUT((*display)++, G_TT_NONE);
    gDPSetTextureFilter((*display)++, G_TF_BILERP);
    gDPSetAlphaCompare((*display)++, G_AC_NONE);
    gSPTexture((*display)++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    if (flame) {
        gDPSetCombineLERP((*display)++, PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, SHADE, 0, 0, 0, 0,
                          COMBINED, COMBINED, 0, PRIMITIVE, 0);
    } else {
        gDPSetCombineLERP((*display)++, PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, 0, 0, 0, SHADE, 0, 0, 0, COMBINED,
                          COMBINED, 0, PRIMITIVE, 0);
    }
    gDPLoadTextureBlock((*display)++, flame ? DinSwordGi::flameTexture : DinSwordGi::coreTexture, G_IM_FMT_I,
                        G_IM_SIZ_8b, 64, 32, 0, G_TX_WRAP, G_TX_WRAP, 6, 5, G_TX_NOLOD, G_TX_NOLOD);
    gDPSetTileSize((*display)++, 0, 0, scroll, 63 << 2, scroll + (31 << 2));
    gDPSetPrimColor((*display)++, 0, 0, core.r, core.g, core.b, alpha);
    gDPSetEnvColor((*display)++, outer.r, outer.g, outer.b, 255);
}

// Caller supplies the selected blade's fully fitted/upright matrix. All layer
// geometry, hashes and textures resolve inside that blade's resource owner.
static inline void ComboDinSwordGi_DrawLayers(PlayState* play, const char* owner, const char* path) {
    const int profile = ResourceMgr_GetDinSwordGiProfileForGame(owner, path);
    if (profile < 1 || profile > 3 || !NeiGi_CanDrawLayers(play, 1, 80, 80))
        return;
    const auto& layers = DinSwordGi::profiles[profile - 1];
#ifdef COMBO_DIN_SWORD_GI_HOST_MM
    const Color_RGBA8 coreColor = CosmeticEditor_GetChangedColor(255, 225, 122, 255, "Custom.DinFireSwordCore");
    const Color_RGBA8 outerColor = CosmeticEditor_GetChangedColor(255, 43, 3, 255, "Custom.DinFireSwordOuter");
    const Color_RGB8 core = { coreColor.r, coreColor.g, coreColor.b };
    const Color_RGB8 outer = { outerColor.r, outerColor.g, outerColor.b };
#else
    const Color_RGB8 core = CVarGetColor24("gCosmetics.Custom.DinFireSwordCore.Value", { 255, 225, 122 });
    const Color_RGB8 outer = CVarGetColor24("gCosmetics.Custom.DinFireSwordOuter.Value", { 255, 43, 3 });
#endif
    const uint16_t phase = play->gameplayFrames & 1023u;
#ifdef COMBO_DIN_SWORD_GI_HOST_MM
    Mtx* matrix = Matrix_Finalize(play->state.gfxCtx);
#else
    Mtx* matrix = Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__);
#endif
    OPEN_DISPS(play->state.gfxCtx);
    gSPComboRMPush(POLY_OPA_DISP++, owner);
    gSPComboRMPush(POLY_XLU_DISP++, owner);
#ifdef COMBO_DIN_SWORD_GI_HOST_MM
    Gfx_SetupDL25_Opa(play->state.gfxCtx);
    Gfx_SetupDL25_Xlu(play->state.gfxCtx);
#else
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
#endif
    gSPGrayscale(POLY_OPA_DISP++, false);
    gSPGrayscale(POLY_XLU_DISP++, false);
    gSPMatrix(POLY_OPA_DISP++, matrix, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPMatrix(POLY_XLU_DISP++, matrix, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    ComboDinSwordGi_Material(&POLY_OPA_DISP, false, core, outer, phase);
    ComboDinSwordGi_Material(&POLY_XLU_DISP, true, core, outer, phase);
    gDma1p(POLY_OPA_DISP++, G_DL_OTR_FILEPATH, layers.core, 0, G_DL_PUSH);
    gDma1p(POLY_XLU_DISP++, G_DL_OTR_FILEPATH, layers.flame, 0, G_DL_PUSH);
    gDPPipeSync(POLY_OPA_DISP++);
    gDPPipeSync(POLY_XLU_DISP++);
#ifdef COMBO_DIN_SWORD_GI_HOST_MM
    Gfx_SetupDL25_Opa(play->state.gfxCtx);
    Gfx_SetupDL25_Xlu(play->state.gfxCtx);
#else
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
#endif
    gSPComboRMPop(POLY_OPA_DISP++);
    gSPComboRMPop(POLY_XLU_DISP++);
    CLOSE_DISPS(play->state.gfxCtx);
}
#endif
