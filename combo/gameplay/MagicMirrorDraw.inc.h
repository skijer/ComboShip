#pragma once

// Include after the native game's global.h. MM callers define
// COMBO_MAGIC_MIRROR_MM; OoT callers leave it undefined. The caller owns the
// object's translation, rotation and ordinary-unit scale in the current matrix.
#include "MagicMirrorGeometry.h"
extern "C" u8 ResourceMgr_FileExists(const char* path);

namespace ComboMagicMirrorVisual {
namespace {
alignas(2) const char kForestReflection[] = "__OTR__textures/magic_mirror/gMagicMirrorForestReflectionTex";
size_t AvailableBytes(const void* head, const void* tail, bool alignTail) {
    const uintptr_t begin = reinterpret_cast<uintptr_t>(head);
    uintptr_t end = reinterpret_cast<uintptr_t>(tail);
    if (alignTail)
        end &= ~uintptr_t(15); // OoT's tail allocator also discards initial misalignment.
    return begin && end >= begin ? static_cast<size_t>(end - begin) : size_t(0);
}
bool CanDraw(const GraphicsContext* gfx, const ComboMagicMirrorGeometry::Mesh& mesh, bool reflection) {
    // Native Graph_Alloc / GRAPH_ALLOC unconditionally subtract the OPA tail.
    // Check the complete draw before touching an arena, including both aligned
    // allocations and each batch's vertex/triangle commands.
    const size_t vertexBytes = (sizeof(Vtx) * mesh.vertexCount + 15) & ~size_t(15);
    const size_t matrixBytes = (sizeof(Mtx) * 2 + 15) & ~size_t(15);
    const size_t dataBytes = vertexBytes + matrixBytes;
    // Per stream: two native setup DL calls (3 commands each in debug builds),
    // outer OPEN/CLOSE markers, draw state, and final sync/matrix restoration.
    size_t opaCommands = 14, xluCommands = 15;
    if (reflection)
        xluCommands += 15; // Owned one-cycle texture state and native untextured-glint restoration.
    for (int i = 0; i < mesh.batchCount; ++i) {
        const auto& batch = mesh.batches[i];
        const size_t commands = 1 + (batch.triangleCount + 1) / 2;
        (i < mesh.opaqueBatchCount ? opaCommands : xluCommands) += commands;
    }
    const size_t opaAvailable = AvailableBytes(gfx->polyOpa.p, gfx->polyOpa.d, true);
    const size_t xluAvailable = AvailableBytes(gfx->polyXlu.p, gfx->polyXlu.d, false);
    const size_t overlayAvailable = AvailableBytes(gfx->overlay.p, gfx->overlay.d, false);
    // Nested setup OPEN/CLOSE brackets temporarily touch a second overlay cell
    // even though they restore its cursor; protect that scratch storage too.
    return dataBytes <= opaAvailable && opaCommands <= (opaAvailable - dataBytes) / sizeof(Gfx) &&
           xluCommands <= xluAvailable / sizeof(Gfx) && overlayAvailable / sizeof(Gfx) >= 2;
}
void* Allocate(PlayState* play, size_t bytes) {
#ifdef COMBO_MAGIC_MIRROR_MM
    return GRAPH_ALLOC(play->state.gfxCtx, bytes);
#else
    return Graph_Alloc(play->state.gfxCtx, bytes);
#endif
}
void ToMatrix(Mtx* matrix) {
#ifdef COMBO_MAGIC_MIRROR_MM
    Matrix_ToMtx(matrix);
#else
    Matrix_ToMtx(matrix, const_cast<char*>(__FILE__), __LINE__);
#endif
}
void SetupOpaque(PlayState* play) {
#ifdef COMBO_MAGIC_MIRROR_MM
    Gfx_SetupDL25_Opa(play->state.gfxCtx);
#else
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
#endif
}
void SetupTranslucent(PlayState* play) {
#ifdef COMBO_MAGIC_MIRROR_MM
    Gfx_SetupDL25_Xlu(play->state.gfxCtx);
#else
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
#endif
}
void Submit(Gfx*& display, const Vtx* vertices, const ComboMagicMirrorGeometry::Mesh& mesh, int begin, int end) {
    for (int i = begin; i < end; ++i) {
        const auto& batch = mesh.batches[i];
        gSPVertex(display++, reinterpret_cast<uintptr_t>(vertices + batch.firstVertex), batch.vertexCount, 0);
        int triangle = 0;
        for (; triangle + 1 < batch.triangleCount; triangle += 2) {
            const auto& a = mesh.triangles[batch.firstTriangle + triangle];
            const auto& b = mesh.triangles[batch.firstTriangle + triangle + 1];
            gSP2Triangles(display++, a.a, a.b, a.c, 0, b.a, b.b, b.c, 0);
        }
        if (triangle < batch.triangleCount) {
            const auto& a = mesh.triangles[batch.firstTriangle + triangle];
            gSP1Triangle(display++, a.a, a.b, a.c, 0);
        }
    }
}
} // namespace

// Translation-unit-local because the two native engines use distinct GBI/setup
// APIs even though their callable Draw interface and shared mesh are identical.
// OPEN_DISPS declares interpolation callbacks with its enclosing language linkage.
extern "C" {
static void Draw(PlayState* play) {
    using namespace ComboMagicMirrorGeometry;
    if (!play || !play->state.gfxCtx)
        return;
    Mesh mesh;
    Build(play->gameplayFrames, mesh);
    const bool reflection = ResourceMgr_FileExists(kForestReflection) != 0;
    if (!CanDraw(play->state.gfxCtx, mesh, reflection))
        return;
    auto* vertices = static_cast<Vtx*>(Allocate(play, sizeof(Vtx) * mesh.vertexCount));
    auto* matrices = static_cast<Mtx*>(Allocate(play, sizeof(Mtx) * 2));
    if (!vertices || !matrices)
        return;
    const int glassBegin = mesh.batches[mesh.opaqueBatchCount].firstVertex;
    const int glassEnd = mesh.batches[mesh.glassEndBatch].firstVertex;
    for (int i = 0; i < mesh.vertexCount; ++i) {
        const auto& source = mesh.vertices[i];
        auto& vertex = vertices[i];
        vertex = {};
        vertex.v.ob[0] = static_cast<s16>(std::lround(source.x * kVertexPrecision));
        vertex.v.ob[1] = static_cast<s16>(std::lround(source.y * kVertexPrecision));
        vertex.v.ob[2] = static_cast<s16>(std::lround(source.z * kVertexPrecision));
        const bool glass = i >= glassBegin && i < glassEnd;
        const Color color = reflection && glass ? ReflectionModulation({ source.r, source.g, source.b, source.a })
                                                : Color{ source.r, source.g, source.b, source.a };
        vertex.v.cn[0] = color.r;
        vertex.v.cn[1] = color.g;
        vertex.v.cn[2] = color.b;
        vertex.v.cn[3] = color.a;
        if (glass) {
            vertex.v.tc[0] = static_cast<s16>(std::lround(ReflectionS(source.x) * kReflectionWidth * 32));
            vertex.v.tc[1] = static_cast<s16>(std::lround(ReflectionT(source.y) * kReflectionHeight * 32));
        }
    }
    ToMatrix(&matrices[0]);
    Matrix_Push();
    Matrix_Scale(1.0f / kVertexPrecision, 1.0f / kVertexPrecision, 1.0f / kVertexPrecision, MTXMODE_APPLY);
    ToMatrix(&matrices[1]);
    Matrix_Pop();

    OPEN_DISPS(play->state.gfxCtx);
    SetupOpaque(play);
    gSPMatrix(POLY_OPA_DISP++, &matrices[1], G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gSPClearGeometryMode(POLY_OPA_DISP++, G_LIGHTING | G_FOG | G_CULL_BOTH);
    gSPTexture(POLY_OPA_DISP++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
    gDPSetCombineMode(POLY_OPA_DISP++, G_CC_SHADE, G_CC_SHADE);
    Submit(POLY_OPA_DISP, vertices, mesh, 0, mesh.opaqueBatchCount);

    SetupTranslucent(play);
    gSPMatrix(POLY_XLU_DISP++, &matrices[1], G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    // Both sides remain visible while spinning. Only the recessed glass uses
    // the fixed scene view; absent new assets retain the exact untextured pane.
    gSPClearGeometryMode(POLY_XLU_DISP++, G_LIGHTING | G_FOG | G_CULL_BOTH);
    gSPTexture(POLY_XLU_DISP++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
    gDPSetRenderMode(POLY_XLU_DISP++, G_RM_PASS, G_RM_AA_ZB_XLU_SURF2);
    gDPSetCombineMode(POLY_XLU_DISP++, G_CC_SHADE, G_CC_SHADE);
    if (reflection) {
        gDPSetCycleType(POLY_XLU_DISP++, G_CYC_1CYCLE);
        gSPTexture(POLY_XLU_DISP++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
        gDPSetTextureLUT(POLY_XLU_DISP++, G_TT_NONE);
        gDPLoadTextureBlock(POLY_XLU_DISP++, kForestReflection, G_IM_FMT_RGBA, G_IM_SIZ_16b, kReflectionWidth,
                            kReflectionHeight, 0, G_TX_CLAMP, G_TX_CLAMP, 6, 5, G_TX_NOLOD, G_TX_NOLOD);
        gDPSetCombineMode(POLY_XLU_DISP++, G_CC_MODULATEIA, G_CC_MODULATEIA);
    }
    Submit(POLY_XLU_DISP, vertices, mesh, mesh.opaqueBatchCount, mesh.glassEndBatch);
    if (reflection) {
        gDPPipeSync(POLY_XLU_DISP++);
        gDPSetCycleType(POLY_XLU_DISP++, G_CYC_2CYCLE);
        gSPTexture(POLY_XLU_DISP++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
        gDPSetCombineMode(POLY_XLU_DISP++, G_CC_SHADE, G_CC_SHADE);
    }
    Submit(POLY_XLU_DISP, vertices, mesh, mesh.glassEndBatch, mesh.batchCount);

    gDPPipeSync(POLY_OPA_DISP++);
    SetupOpaque(play);
    gSPMatrix(POLY_OPA_DISP++, &matrices[0], G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gDPPipeSync(POLY_XLU_DISP++);
    SetupTranslucent(play);
    gSPMatrix(POLY_XLU_DISP++, &matrices[0], G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    CLOSE_DISPS(play->state.gfxCtx);
}
} // extern "C"
} // namespace ComboMagicMirrorVisual
