/* Run the complete production cape draw with native MM structures/GBI.
 * Archive, CVar and matrix allocation boundaries are supplied by this fixture.
 * Cloth simulation is left idle; the test covers its material submission.
 */
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include "z64.h"
#include "z64player.h"
#include "functions.h"
#include "2s2h/BenPort.h"

static u8 sAvailable;
static u8 sAlpha;
static void* sRawPixels;

s32 CVarGetInteger(const char* name, s32 fallback) {
    if (strcmp(name, "gItemEditor.Cape.Custom") == 0)
        return 1;
    if (strcmp(name, "gItemEditor.Cape.ColorA") == 0)
        return sAlpha;
    return fallback;
}

f32 CVarGetFloat(const char* name, f32 fallback) {
    (void)name;
    return fallback;
}

u8 ResourceMgr_FileExists(const char* path) {
    assert(strcmp(path, "__OTR__overlays/ovl_En_Ganon_Mant/gMantTex") == 0);
    return sAvailable;
}

void* OotAssets_LoadTexOrDList(const char* path) {
    assert(strcmp(path, "__OTR__overlays/ovl_En_Ganon_Mant/gMantTex") == 0);
    return sRawPixels;
}

u8 ExtEquip_CapeVisible(void) { return 1; }
int ResourceMgr_OTRSigCheck(char* data) { return strncmp(data, "__OTR__", 7) == 0; }
Vtx* ResourceMgr_LoadVtxByName(char* path) { (void)path; assert(0); return NULL; }
void FrameInterpolation_RecordOpenChild(const void* key, int line) { (void)key; (void)line; }
void FrameInterpolation_RecordCloseChild(void) {}
void Gfx_SetupDL25_Opa(GraphicsContext* gfxCtx) { (void)gfxCtx; }
void Matrix_Translate(f32 x, f32 y, f32 z, MatrixMode mode) {
    (void)x; (void)y; (void)z; (void)mode;
}
static Mtx sMatrix;
Mtx* Matrix_Finalize(GraphicsContext* gfxCtx) { (void)gfxCtx; return &sMatrix; }
/* Physics is not ticked by this material fixture. Keep native signatures. */
void Matrix_Push(void) {}
void Matrix_Pop(void) {}
void Matrix_RotateXF(f32 angle, MatrixMode mode) { (void)angle; (void)mode; }
void Matrix_RotateYF(f32 angle, MatrixMode mode) { (void)angle; (void)mode; }
void Matrix_RotateZF(f32 angle, MatrixMode mode) { (void)angle; (void)mode; }
void Matrix_MultVec3f(Vec3f* src, Vec3f* dst) { *dst = *src; }
f32 Math_Atan2F_XY(f32 x, f32 y) { return atan2f(y, x); }
f32 Math_SinS(s16 angle) { return sinf(angle * (3.14159265358979323846f / 32768.0f)); }
f32 Math_CosS(s16 angle) { return cosf(angle * (3.14159265358979323846f / 32768.0f)); }
void Math_ApproachZeroF(f32* value, f32 scale, f32 step) { (void)value; (void)scale; (void)step; }

#include "mm_nei_graph.inc"
#include "mods/equipment/behaviors/equip_magiccape.c"

/* Return the native SETTIMG command without translating/rebuilding it. */
int CapeProbe_Draw(u8 alpha, u8 available, void* pixels, u32 stateFlags,
                   uintptr_t words[2], u32* triangles) {
    static Gfx opa[1024], xlu[1024], overlay[32];
    GraphicsContext gfx = { 0 };
    PlayState play = { 0 };
    Player player = { 0 };
    gfx.polyOpa.p = opa; gfx.polyOpa.d = opa + 1024;
    gfx.polyXlu.p = xlu; gfx.polyXlu.d = xlu + 1024;
    gfx.overlay.p = overlay; gfx.overlay.d = overlay + 32;
    play.state.gfxCtx = &gfx;
    player.stateFlags1 = stateFlags;
    sAvailable = available; sAlpha = alpha; sRawPixels = pixels;
    sCapeInitialized = 1;
    sCapeUpdateHasRun = 0;
    *triangles = 0;
    MagicCape_Draw(&player, &play);
    Gfx* start = alpha == 255 ? opa : xlu;
    Gfx* end = alpha == 255 ? gfx.polyOpa.p : gfx.polyXlu.p;
    int found = 0;
    for (Gfx* command = start; command < end; ++command) {
        u32 op = command->words.w0 >> 24;
        if (op == G_SETTIMG) {
            words[0] = command->words.w0;
            words[1] = command->words.w1;
            found++;
        }
        if (op == G_TRI2)
            *triangles += 2;
    }
    return found;
}
