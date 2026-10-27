// Unmodified shelf control from a82852a3a02c8cabc541cce7c124da71c5755efb; function name only changed.
static void Baseline_DrawSelectedSword(PlayState* play, const char* path, bool shop = false, bool fit = true) {
    OPEN_DISPS(play->state.gfxCtx);
    if (fit)
        ComboSwordGi_ApplyFit("oot", path, .04f, 1.8f, shop);
    Matrix_RotateY(Spin(play), MTXMODE_APPLY);
    // Standalone donor blades point along +X. Z alone turns that axis into
    // upright +Y; an extra X quarter turn would lay it flat in XZ.
    Matrix_RotateZ(1.8f, MTXMODE_APPLY);
    Matrix_Scale(.04f, .04f, .04f, MTXMODE_APPLY);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    gSPGrayscale(POLY_OPA_DISP++, false);
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, 255);
    gDPSetEnvColor(POLY_OPA_DISP++, 255, 255, 255, 255);
    gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    gDma1p(POLY_OPA_DISP++, G_DL_OTR_FILEPATH, path, 0, G_DL_PUSH);
    CLOSE_DISPS(play->state.gfxCtx);
    ComboDinSwordGi_DrawLayers(play, "oot", path);
}
