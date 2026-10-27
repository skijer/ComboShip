// TU glue: include after the host engine/GBI declarations. Both native MM and
// foreign MM GIs in OoT submit this same effect; the owner selects Alt resources.
#ifndef COMBO_SPIN_ATTACK_GI_H
#define COMBO_SPIN_ATTACK_GI_H

#ifdef __cplusplus
extern "C" {
#endif
void NeiGi_ReportBlackMarketModel(const char* owner, const char* opaque, const char* translucent);
#ifdef __cplusplus
}
#endif

#ifdef COMBO_SPIN_GI_HOST_MM
#define CSG_SETUP_XLU(ctx) Gfx_SetupDL25_Xlu(ctx)
#define CSG_LOAD_MTX(pkt, ctx) MATRIX_FINALIZE_AND_LOAD(pkt, ctx)
#else
#define CSG_SETUP_XLU(ctx) Gfx_SetupDL_25Xlu(ctx)
#define CSG_LOAD_MTX(pkt, ctx) \
    gSPMatrix(pkt, Matrix_NewMtx(ctx, (char*)__FILE__, __LINE__), G_MTX_MODELVIEW | G_MTX_LOAD)
#endif

static inline void ComboDrawSpinAttackGi(PlayState* play, const char* disk, const char* cylinder, float scale,
                                         const uint8_t color[4], const char* owner) {
    Matrix_Push();
    Matrix_RotateZYX(0x1800, (s16)(play->gameplayFrames * 0x180), 0, MTXMODE_APPLY);
    Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);
#ifdef COMBO_SPIN_GI_HOST_MM
    NeiGi_ReportBlackMarketModel(owner ? owner : "mm", disk, cylinder);
#else
    NeiGi_ReportBlackMarketModel(owner ? owner : "oot", disk, cylinder);
#endif
    OPEN_DISPS(play->state.gfxCtx);
    CSG_SETUP_XLU(play->state.gfxCtx);
    gSPGrayscale(POLY_XLU_DISP++, false);
    gDPSetTextureLUT(POLY_XLU_DISP++, G_TT_NONE);
    CSG_LOAD_MTX(POLY_XLU_DISP++, play->state.gfxCtx);
    // Match EnMThunder's burst material and two-layer scroll. These are real
    // gameplay display lists, rendered at GI scale without spawning an actor.
    gSPSegment(POLY_XLU_DISP++, 8,
               (uintptr_t)Gfx_TwoTexScrollEx(play->state.gfxCtx, 0, 255 - (play->gameplayFrames * 30 & 255), 0, 64, 32,
                                             1, 255 - (play->gameplayFrames * 20 & 255), 0, 8, 8, -30, 0, -20, 0));
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0x80, color[0], color[1], color[2], 255);
    gDPSetEnvColor(POLY_XLU_DISP++, 255, 100, 0, 128);
    if (owner != nullptr) {
        gSPComboRMPush(POLY_XLU_DISP++, owner);
    }
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)disk);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)cylinder);
    if (owner != nullptr) {
        gSPComboRMPop(POLY_XLU_DISP++);
    }
    // Do not leave a later GI sampling this item's scroll segment.
    static Gfx empty[] = { gsSPEndDisplayList(), gsSPEndDisplayList(), gsSPEndDisplayList(), gsSPEndDisplayList(),
                           gsSPEndDisplayList(), gsSPEndDisplayList(), gsSPEndDisplayList(), gsSPEndDisplayList() };
    gSPSegment(POLY_XLU_DISP++, 8, (uintptr_t)empty);
    CLOSE_DISPS(play->state.gfxCtx);
    Matrix_Pop();
}

#undef CSG_SETUP_XLU
#undef CSG_LOAD_MTX
#endif
