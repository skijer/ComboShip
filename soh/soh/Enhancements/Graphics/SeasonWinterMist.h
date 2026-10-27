#pragma once

#include "functions.h"
#include "macros.h"
#include <cmath>

// The Winter mist visual is shared by seasonal weather and the local Snowhead
// approach. Callers own placement, lifetime, frame allocation and draw budgets.
namespace SoHSeasonWinterMist {
constexpr int kColumns = 5, kVertexCount = 15;

inline Gfx* Prepare(Gfx* gfx) {
    gDPPipeSync(gfx++);
    gDPSetCycleType(gfx++, G_CYC_1CYCLE);
    gDPSetRenderMode(gfx++, G_RM_AA_ZB_XLU_SURF, G_RM_AA_ZB_XLU_SURF2);
    gSPClearGeometryMode(gfx++, G_LIGHTING | G_CULL_BOTH | G_FOG);
    gSPSetGeometryMode(gfx++, G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH);
    gDPSetTextureLUT(gfx++, G_TT_NONE);
    gSPTexture(gfx++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
    gDPSetCombineMode(gfx++, G_CC_SHADE, G_CC_SHADE);
    return gfx;
}

inline void Vertices(Vtx* vertices, float rightX, float rightZ, float width, float height, float alpha) {
    for (int row = 0; row < 3; ++row)
        for (int column = 0; column < kColumns; ++column) {
            const float x = (column - 2) * 0.5f;
            const float y = (row - 1) * height * (1 - 0.3f * std::abs(x));
            const float a = row == 1 && column > 0 && column < 4 ? alpha * (column == 2 ? 1 : 0.68f) : 0;
            const Vtx vertex =
                VTX(static_cast<s16>(std::lround(rightX * (x * width))), static_cast<s16>(std::lround(y)),
                    static_cast<s16>(std::lround(rightZ * (x * width))), 0, 0, 192, 222, 245,
                    static_cast<u8>(std::lround(a * 255)));
            vertices[row * kColumns + column] = vertex;
        }
}

inline Gfx* Draw(Gfx* gfx, Vtx* vertices) {
    gSPVertex(gfx++, reinterpret_cast<uintptr_t>(vertices), kVertexCount, 0);
    for (int row = 0; row < 2; ++row)
        for (int column = 0; column < kColumns - 1; ++column) {
            const int index = row * kColumns + column;
            gSP2Triangles(gfx++, index, index + kColumns, index + kColumns + 1, 0, index, index + kColumns + 1,
                          index + 1, 0);
        }
    return gfx;
}
} // namespace SoHSeasonWinterMist
