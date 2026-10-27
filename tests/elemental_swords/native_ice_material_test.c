// Real EnIce init/draw/update with native layouts and GBI. Resource selection,
// matrices, light setup and accepted-hit attribution are engine boundaries.
#include "global.h"
#include "overlays/effects/ovl_Effect_Ss_En_Ice/z_eff_ss_en_ice.h"
#include "objects/gameplay_keep/gameplay_keep.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

u32 EffectSsEnIce_Init(PlayState*, u32, EffectSs*, void*);
void EffectSsEnIce_Draw(PlayState*, u32, EffectSs*);
void EffectSsEnIce_Update(PlayState*, u32, EffectSs*);
static int swordHit, queries, materialRequests, available = 1, matrices;
static PlayState play;
static GraphicsContext gfx;
static Gfx commands[256], scroll[12], custom[1];
static Mtx matrix;
Vec3f gZeroVec3f;
void lusprintf(const char* file, int32_t line, int32_t level, const char* format, ...) {
    (void)file;(void)line;(void)level;(void)format;
}
void FrameInterpolation_RecordOpenChild(const void* a, int b) { (void)a;(void)b; }
void FrameInterpolation_RecordCloseChild(void) {}

int DinFireSword_UseIceMaterial(PlayState* state, const Actor* actor) {
    assert(state == &play);
    ++queries;
    return actor != NULL && swordHit;
}
Gfx* SwordFinishIce_GetDisplayList(uint32_t frame) {
    assert(frame == play.gameplayFrames);
    ++materialRequests;
    return available ? custom : NULL;
}
f32 Rand_CenteredFloat(f32 scale) { (void)scale; return 0; }
f32 Rand_ZeroOne(void) { return .5f; }
f32 Math_SinS(s16 angle) { (void)angle; return 0; }
f32 Math_CosS(s16 angle) { (void)angle; return 1; }
s16 Math_Vec3f_Yaw(Vec3f* a, Vec3f* b) { (void)a; (void)b; return 0; }
#ifdef ICE_TEST_MM
void Math_Vec3f_Copy(Vec3f* dest, Vec3f* src) { *dest = *src; }
void Math_Vec3f_Diff(Vec3f* a, Vec3f* b, Vec3f* out) { *out = (Vec3f){a->x-b->x,a->y-b->y,a->z-b->z}; }
void Math_Vec3f_Sum(Vec3f* a, Vec3f* b, Vec3f* out) { *out = (Vec3f){a->x+b->x,a->y+b->y,a->z+b->z}; }
s16 Math_Atan2S_XY(f32 a, f32 b) { (void)a; (void)b; return 0; }
void Matrix_Translate(f32 x, f32 y, f32 z, MatrixMode mode) { (void)x;(void)y;(void)z;(void)mode; }
void Matrix_Scale(f32 x, f32 y, f32 z, MatrixMode mode) { (void)x;(void)y;(void)z;(void)mode; }
void Matrix_RotateXS(s16 x, MatrixMode mode) { (void)x; (void)mode; }
void Matrix_RotateYS(s16 x, MatrixMode mode) { (void)x; (void)mode; }
Mtx* Matrix_Finalize(GraphicsContext* context) { assert(context == &gfx); ++matrices; return &matrix; }
void Gfx_SetupDL25_Xlu(GraphicsContext* context) { assert(context == &gfx); }
Hilite* func_800BCC68(Vec3f* pos, PlayState* state) { (void)pos; assert(state == &play); return NULL; }
void Graph_OpenDisps(Gfx** refs, Gfx* values, GraphicsContext* context, const char* file, s32 line) {
    (void)refs;(void)values;(void)file;(void)line; assert(context == &gfx);
}
void Graph_CloseDisps(Gfx** refs, Gfx* values, GraphicsContext* context, const char* file, s32 line) {
    (void)refs;(void)values;(void)file;(void)line; assert(context == &gfx);
}
#else
s16 Math_Atan2S(f32 a, f32 b) { (void)a; (void)b; return 0; }
void Matrix_Translate(f32 x, f32 y, f32 z, u8 mode) { (void)x;(void)y;(void)z;(void)mode; }
void Matrix_Scale(f32 x, f32 y, f32 z, u8 mode) { (void)x;(void)y;(void)z;(void)mode; }
void Matrix_RotateX(f32 x, u8 mode) { (void)x; (void)mode; }
void Matrix_RotateY(f32 x, u8 mode) { (void)x; (void)mode; }
Mtx* Matrix_NewMtx(GraphicsContext* context, char* file, s32 line) {
    (void)file;(void)line; assert(context == &gfx); ++matrices; return &matrix;
}
void Gfx_SetupDL_25Xlu(GraphicsContext* context) { assert(context == &gfx); }
Hilite* func_8002EB44(Vec3f* p, Vec3f* eye, Vec3f* light, GraphicsContext* context) {
    (void)p;(void)eye;(void)light; assert(context == &gfx); return NULL;
}
void osSyncPrintfUnused(const char* format, ...) { (void)format; }
void Graph_OpenDisps(Gfx** refs, GraphicsContext* context, const char* file, s32 line) {
    (void)refs;(void)file;(void)line; assert(context == &gfx);
}
void Graph_CloseDisps(Gfx** refs, GraphicsContext* context, const char* file, s32 line) {
    (void)refs;(void)file;(void)line; assert(context == &gfx);
}
#endif
void gSPDisplayList(Gfx* packet, Gfx* list) { __gSPDisplayList(packet, list); }
void gSPSegment(void* packet, int segment, uintptr_t value) { __gSPSegment(packet, segment, value); }
Gfx* Gfx_TwoTexScrollEx(GraphicsContext* context, s32 t0, u32 x0, u32 y0, s32 w0, s32 h0,
                      s32 t1, u32 x1, u32 y1, s32 w1, s32 h1, s32 sx0, s32 sy0, s32 sx1, s32 sy1) {
    (void)t0;(void)x0;(void)y0;(void)t1;(void)x1;(void)y1;(void)sx0;(void)sy0;(void)sx1;(void)sy1;
    assert(context == &gfx);
#ifndef ICE_DAMAGE_DRAW
    assert(w0 == 32 && h0 == 16 && w1 == 64 && h1 == 32);
#endif
    return scroll;
}

#ifndef ICE_DAMAGE_DRAW
static size_t Draw(EffectSs* effect, int expectCustom) {
    gfx.polyXlu.p = commands;
    gfx.polyXlu.d = commands + ARRAY_COUNT(commands);
    const int priorMatrices = matrices;
    const int priorQueries = queries;
    EffectSsEnIce_Draw(&play, 0, effect);
    assert(matrices == priorMatrices + 1 && queries == priorQueries);
    unsigned roots = 0;
    for (Gfx* command = commands; command < gfx.polyXlu.p; ++command) {
        if ((command->words.w0 >> 24) == G_DL) {
#ifdef ICE_TEST_MM
            const void* want = roots ? (const void*)gEffIceFragment2ModelDL
                                    : expectCustom ? (const void*)custom : (const void*)gEffIceFragment2MaterialDL;
#else
            const void* want = expectCustom ? (const void*)custom : (const void*)gEffIceFragment2DL;
#endif
            if (want == custom) {
                assert(command->words.w1 == (uintptr_t)custom);
            } else {
                assert(strcmp((const char*)command->words.w1, want) == 0);
            }
            ++roots;
        }
    }
    assert(roots ==
#ifdef ICE_TEST_MM
           2
#else
           1
#endif
    );
    return (size_t)(gfx.polyXlu.p - commands);
}

int main(void) {
    play.state.gfxCtx = &gfx;
    play.gameplayFrames = 7;
    EffectSs effect = {0};
    EffectSsEnIceInitParams params = {0};
    Actor* actor = calloc(1, sizeof(*actor));
    assert(actor);
    params.actor = actor;
    params.scale = 1.5f;
    params.primColor = (Color_RGBA8){150,150,150,250};
    params.envColor = (Color_RGBA8){235,245,255,255};
    swordHit = 1;
    assert(EffectSsEnIce_Init(&play, 0, &effect, &params));
    assert(effect.regs[3] == 1 && queries == 1);
    free(actor); // Material ownership was captured at init; draw cannot query this address.
    const size_t markedCommands = Draw(&effect, 1);
    assert(materialRequests == 1);
    available = 0;
    assert(Draw(&effect, 0) == markedCommands);
    available = 1;

    // Reusing a particle slot for ordinary arrow/magic ice clears the mark.
    actor = calloc(1, sizeof(*actor));
    params.actor = actor;
    swordHit = 0;
    assert(EffectSsEnIce_Init(&play, 0, &effect, &params));
    assert(effect.regs[3] == 0 && queries == 2);
    const int before = materialRequests;
    assert(Draw(&effect, 0) == markedCommands && materialRequests == before);
    free(actor);

    params.actor = NULL;
    params.type = 1;
    params.life = 20;
    swordHit = 1; // Normal debris cannot inherit a current sword selection.
    effect.regs[3] = 1;
    assert(EffectSsEnIce_Init(&play, 0, &effect, &params));
    assert(effect.regs[3] == 0 && queries == 2);
    assert(Draw(&effect, 0) == markedCommands && materialRequests == before);
    const s16 pitch = effect.regs[2];
    EffectSsEnIce_Update(&play, 0, &effect);
    assert(effect.regs[2] == pitch);
    puts("PASS native EnIce sword-origin init flag -> custom/fallback draw; ordinary ice and normal rotation preserved, "
         "actor-free draw and unchanged arena/matrix counts (ASan/UBSan)");
}
#endif
