/* Production flex traversal; matrix primitives model affine scale/translation.
 * Verifies incoming-origin scaling, callbacks/prop transforms and stack restore. */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "global.h"
#include "soh/Enhancements/cosmetics/NativeNpcPose.h"
void SkelAnime_DrawFlexOpaWithXlu(PlayState*, void**, Vec3s*, s32, OverrideLimbDrawOpa, PostLimbDrawOpa, void*);

static MtxF current, stack[16], observed[2];
static int depth, observedCount, scaleCalls;
static float requestedScale = 1.10f;
static Mtx storage[8];
static Gfx body[1];
static void** expectedSkeleton;

float NativeNpcPose_GetDrawScale(void** skeleton) {
    assert(skeleton == expectedSkeleton);
    return requestedScale;
}
const void* NativeNpcPose_GetSkinPlan(void** skeleton, s32 count) {
    return NULL;
}
s32 NativeNpcPose_SkinMatrixCount(const void* plan) {
    return 0;
}
void NativeNpcPose_BlendSkinMatrix(const void* plan, s32 index, const MtxF* native, MtxF* result) {
    assert(0);
}
void* Graph_Alloc(GraphicsContext* context, size_t size) {
    assert(size <= sizeof(storage));
    return storage;
}
void Graph_OpenDisps(Gfx** a, GraphicsContext* c, const char* d, s32 e) {
}
void Graph_CloseDisps(Gfx** a, GraphicsContext* c, const char* d, s32 e) {
}
void gSPSegment(void* command, int segment, uintptr_t target) {
}
void gSPDisplayList(Gfx* command, Gfx* display) {
}
void FrameInterpolation_RecordOpenChild(const void* pointer, int identifier) {
}
void FrameInterpolation_RecordCloseChild(void) {
}
void Matrix_Push(void) {
    stack[depth++] = current;
}
void Matrix_Pop(void) {
    current = stack[--depth];
}
void Matrix_Scale(f32 x, f32 y, f32 z, u8 mode) {
    assert(mode == MTXMODE_APPLY && x == y && y == z);
    ++scaleCalls;
    current.xx *= x;
    current.yy *= y;
    current.zz *= z;
}
void Matrix_TranslateRotateZYX(Vec3f* translation, Vec3s* rotation) {
    assert(rotation->x == 0 && rotation->y == 0 && rotation->z == 0);
    current.xw += current.xx * translation->x;
    current.yw += current.yy * translation->y;
    current.zw += current.zz * translation->z;
}
Mtx* Matrix_ToMtx(Mtx* dest, char* file, s32 line) {
    return dest;
}
void Matrix_Put(MtxF* src) {
    current = *src;
}
void Matrix_MtxToMtxF(Mtx* source, MtxF* dest) {
    assert(0);
}
int32_t CVarGetInteger(const char* name, int32_t fallback) {
    return fallback;
}
void osSyncPrintfUnused(const char* format, ...) {
}
void lusprintf(const char* file, int32_t line, int32_t level, const char* format, ...) {
}

static void Post(PlayState* play, s32 index, Gfx** display, Vec3s* rotation, void* actor) {
    assert(index == ++observedCount);
    observed[index - 1] = current;
}
static void PostGfx(PlayState* play, s32 index, Gfx** display, Vec3s* rotation, void* actor, Gfx** commands) {
    Post(play, index, display, rotation, actor);
}
static void Check(float factor) {
    assert(depth == 0 && observedCount == 2);
    assert(fabsf(current.xx - .01f) < 1e-7f && current.xw == 100.f && current.yw == 200.f && current.zw == 300.f);
    assert(fabsf(observed[0].xx - .01f * factor) < 1e-7f);
    assert(fabsf(observed[0].xw - (100.f + 2.f * factor)) < 1e-4f);
    assert(fabsf(observed[0].yw - (200.f + 7.f * factor)) < 1e-4f);
    assert(fabsf(observed[1].yw - (200.f + 10.f * factor)) < 1e-4f);
    /* A prop emitted from post-limb uses the same uniformly enlarged matrix. */
    assert(fabsf(observed[1].xw + 50.f * observed[1].xx - (100.f + 2.5f * factor)) < 1e-4f);
    assert(scaleCalls == (factor == 1.f ? 0 : 1));
}
int main(void) {
    PlayState play = { 0 };
    GraphicsContext context = { 0 };
    Gfx commands[128] = { 0 };
    Vec3s joints[3] = { { 200, 700, 100 }, { 0 }, { 0 } }, saved[3];
    StandardLimb root = { { 0 }, 1, LIMB_DONE, body }, child = { { 0, 300, 0 }, LIMB_DONE, LIMB_DONE, body };
    LodLimb lodRoot = { { 0 }, 1, LIMB_DONE, { body, body } },
            lodChild = { { 0, 300, 0 }, LIMB_DONE, LIMB_DONE, { body, body } };
    void* skeleton[] = { &root, &child };
    void* lodSkeleton[] = { &lodRoot, &lodChild };
    memcpy(saved, joints, sizeof(saved));
    play.state.gfxCtx = &context;
    const float factors[] = { 1.10f, 1.f, 1.25f };
    for (int factor = 0; factor < 3; ++factor) {
        requestedScale = factors[factor];
        for (int route = 0; route < 4; ++route) {
            memset(&current, 0, sizeof(current));
            current.xx = current.yy = current.zz = .01f;
            current.ww = 1.f;
            current.xw = 100.f;
            current.yw = 200.f;
            current.zw = 300.f;
            observedCount = scaleCalls = depth = 0;
            context.polyOpa.p = commands;
            context.polyXlu.p = commands + 64;
            expectedSkeleton = route == 0 ? lodSkeleton : skeleton;
            if (route == 0)
                SkelAnime_DrawFlexLod(&play, lodSkeleton, joints, 2, NULL, Post, NULL, 0);
            else if (route == 1)
                SkelAnime_DrawFlexOpa(&play, skeleton, joints, 2, NULL, Post, NULL);
            else if (route == 2)
                SkelAnime_DrawFlex(&play, skeleton, joints, 2, NULL, PostGfx, NULL, commands);
            else {
#ifdef COMBO_BUILD
                SkelAnime_DrawFlexOpaWithXlu(&play, skeleton, joints, 2, NULL, Post, NULL);
#else
                SkelAnime_DrawFlexOpa(&play, skeleton, joints, 2, NULL, Post, NULL);
#endif
            }
            Check(requestedScale);
            assert(memcmp(saved, joints, sizeof(saved)) == 0);
        }
    }
    puts("PASS real flex draw routes: incoming-origin scaling includes root/limbs/props, preserves joints and restores "
         "matrices.");
}
