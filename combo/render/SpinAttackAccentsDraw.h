// TU glue: include after native engine/GBI declarations.
#ifndef COMBO_SPIN_ATTACK_ACCENTS_DRAW_H
#define COMBO_SPIN_ATTACK_ACCENTS_DRAW_H
#include "SpinAttackAccents.h"
#include <libultraship/bridge/resourcebridge.h>
#include "SpinAttackAccentNative.h"

static inline void* CwSpinDraw_Alloc(GraphicsContext* gfx, size_t bytes) {
#ifdef CW_SPIN_HOST_MM
    return GRAPH_ALLOC(gfx, bytes);
#else
    return Graph_Alloc(gfx, bytes);
#endif
}

static inline int CwSpinAccents_DrawPass(PlayState* play, uint8_t element, int great, float phase, float alpha,
                                         size_t limit, uint32_t tint, int charge) {
    CwSpinAccentParticle particles[24];
    if (limit > 24)
        limit = 24;
    size_t count = charge ? CwSpinAccents_BuildCharge(element, great, phase, alpha, particles, limit)
                          : CwSpinAccents_Build(element, great, phase, alpha, particles, limit);
    if (count == 0)
        return 0;
    // Dark Energy's sparse red sparks use the native sparkle, while retaining
    // their existing Fire-derived deterministic motion and shared budget.
    uint8_t materialElement = tint != 0 && element == 1 ? 3 : element;
    const char* path = materialElement == 1 ? gEffFire2DL : materialElement == 2 ? gEffIceFragment1DL : gEffSparklesDL;
    Gfx* source = (Gfx*)ResourceGetDataByName(path);
    size_t bytes = ResourceGetSizeByName(path);
    Gfx scrollCheck[12];
    if (!source || bytes == 0 || bytes % sizeof(Gfx) ||
        !CwSpinAccents_NativeMaterial(NULL, source, bytes / sizeof(Gfx), scrollCheck))
        return 0;
    typedef struct {
        Mtx model;
        LookAt lookAt;
    } AccentFrame;
    size_t frameBytes = count * sizeof(AccentFrame);
    size_t total = frameBytes + sizeof(Mtx) + bytes + 12 * sizeof(Gfx);
    // Both native arenas allocate from the tail and do not check collision.
    uintptr_t head = (uintptr_t)play->state.gfxCtx->polyOpa.p;
    uintptr_t tail = (uintptr_t)play->state.gfxCtx->polyOpa.d;
    if (tail < head || tail - head < ((total + 15) & ~(size_t)15))
        return 0;
    AccentFrame* frames = (AccentFrame*)CwSpinDraw_Alloc(play->state.gfxCtx, total);
    if (!frames)
        return 0;
    Mtx* restore = (Mtx*)((uint8_t*)frames + frameBytes);
    Gfx* material = (Gfx*)(restore + 1);
    Gfx* scroll = (Gfx*)((uint8_t*)material + bytes);
    CwSpinAccents_NativeScroll(scroll, materialElement, play->gameplayFrames);
    CwSpinAccents_NativeMaterial(material, source, bytes / sizeof(Gfx), scroll);
    MtxF parent;
    Matrix_Get(&parent);
    Matrix_MtxFToMtx(&parent, restore);
    for (size_t i = 0; i < count; ++i) {
        CwSpinAccentParticle* p = &particles[i];
        if (tint != 0) {
            p->r = (uint8_t)(tint >> 16);
            p->g = (uint8_t)(tint >> 8);
            p->b = (uint8_t)tint;
        }
        MtxF model;
        CwSpinAccents_NativeModel(&model, &parent, &play->billboardMtxF, p, materialElement, phase, i);
        Matrix_MtxFToMtx(&model, &frames[i].model);
        if (materialElement == 2) {
            Vec3f position = { model.xw, model.yw, model.zw };
            float eyeX = play->view.eye.x;
            if (eyeX == position.x && play->view.eye.z == position.z)
                eyeX += 0.001f;
            Mtx unused;
            Hilite hilite;
            Vec3f light = { (float)play->envCtx.dirLight1.params.dir.x, (float)play->envCtx.dirLight1.params.dir.y,
                            (float)play->envCtx.dirLight1.params.dir.z };
            guLookAtHilite(&unused, &frames[i].lookAt, &hilite, eyeX, play->view.eye.y, play->view.eye.z, position.x,
                           position.y, position.z, 0, 1, 0, light.x, light.y, light.z, light.x, light.y, light.z, 16,
                           16);
        }
    }
    OPEN_DISPS(play->state.gfxCtx);
    gDPPipeSync(POLY_XLU_DISP++);
#ifdef CW_SPIN_HOST_MM
    Gfx_SetupDL25_Xlu(play->state.gfxCtx);
#else
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
#endif
    gSPClearGeometryMode(POLY_XLU_DISP++, G_FOG | G_LIGHTING | G_CULL_BOTH | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR);
    for (size_t i = 0; i < count; ++i) {
        CwSpinAccentParticle* p = &particles[i];
        gDPPipeSync(POLY_XLU_DISP++);
        gSPMatrix(POLY_XLU_DISP++, &frames[i].model, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0x80, p->r, p->g, p->b, p->a);
        if (materialElement == 2) {
            gDPSetEnvColor(POLY_XLU_DISP++, 0, 50, 100, p->a);
            gSPLookAt(POLY_XLU_DISP++, &frames[i].lookAt);
        } else {
            gDPSetEnvColor(POLY_XLU_DISP++, p->r, p->g / 5, p->b / 4, 0);
        }
        gSPDisplayList(POLY_XLU_DISP++, material);
    }
    gSPMatrix(POLY_XLU_DISP++, restore, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    // Subsequent charge glows/actors start from the same native XLU setup.
#ifdef CW_SPIN_HOST_MM
    Gfx_SetupDL25_Xlu(play->state.gfxCtx);
#else
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
#endif
    CLOSE_DISPS(play->state.gfxCtx);
    return 1;
}

static inline int CwSpinAccents_DrawLimited(PlayState* play, uint8_t element, int great, float phase, float alpha,
                                            size_t limit, uint32_t tint) {
    return CwSpinAccents_DrawPass(play, element, great, phase, alpha, limit, tint, 0);
}

static inline int CwSpinAccents_DrawCharge(PlayState* play, uint8_t element, int great, float phase, float alpha) {
    return CwSpinAccents_DrawPass(play, element, great, phase, alpha, great ? 8 : 4, 0, 1);
}

static inline int CwSpinAccents_Draw(PlayState* play, uint8_t element, int great, float phase, float alpha) {
    return CwSpinAccents_DrawLimited(play, element, great, phase, alpha, great ? 24 : 12, 0);
}
#endif
