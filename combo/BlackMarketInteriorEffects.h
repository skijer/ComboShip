#pragma once
// Include after the host's global.h. All resources stay in its native archive.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include "BlackMarketGraphicsArena.h"

namespace BlackMarketInteriorEffects {
// Audited retained liquid circles: tri0, tri18 (two mouths), and tri16.
inline constexpr std::array<Vec3f, 4> kPotMouths{
    { { 73, 37, 48 }, { -91, 51, 24 }, { 118, 45, 243 }, { 118, 40, 333 } }
};
// Prepared stand-only vertex translations preserve each bowl/shaft/foot shape:
// right +(-55,0,213), left +(29,0,213). The original bundle stays unchanged.
// Asymmetric gaps clear the retained cabinets, pots and hanging baskets, plus
// the complete shelf/item envelopes. Native EnLight's small decorative scale
// gives each upright flame radius16 and height64, including its upper tip.
inline constexpr std::array<Vec3f, 2> kTorches{ { { 75, 76, 205 }, { -110, 76, 205 } } };
inline constexpr float kFlameScale = .004f;
inline constexpr std::array<uint8_t, 4> kFlamePrim{ 225, 205, 255, 255 };
inline constexpr std::array<uint8_t, 4> kFlameEnv{ 90, 45, 160, 0 };
alignas(2) inline constexpr char kFire[] = "__OTR__objects/gameplay_keep/gEffFire1DL";
alignas(2) inline constexpr char kMmSmoke[] = "__OTR__objects/gameplay_keep/gEffDust1Tex";
alignas(2) inline constexpr char kOotSmoke[] = "__OTR__objects/gameplay_keep/gDust1Tex";
inline constexpr int kPuffsPerPot = 12, kPuffCount = kPuffsPerPot * kPotMouths.size();
inline constexpr size_t kFrameTailBytes = ((sizeof(Vtx) * kPuffCount * 4 + 15) & ~size_t(15)) +
                                          ((sizeof(Mtx) * 3 + 15) & ~size_t(15)) +
                                          2 * ((sizeof(Gfx) * 12 + 15) & ~size_t(15));
inline constexpr size_t kXluCommands = 256;
struct Puff {
    Vec3f center;
    float halfWidth, halfHeight;
    uint8_t alpha;
};
inline Puff Vapor(int pot, int puff, uint32_t frame) {
    // Staggered, overlapping puffs form a continuous rising column, not a
    // periodically spawned burst. Stretch the soft I8 core vertically so the
    // 9.5-unit center spacing reads as steam rather than round particle beads.
    // No actor, effect slot, RNG or saved state.
    const float age = (frame % 120 + puff * 10 + pot * 17) % 120;
    const auto mouth = kPotMouths[pot];
    const float fade = std::max(0.f, std::min({ 1.f, age / 12, (120 - age) / 30 }));
    return { { mouth.x + std::sin(age * .07f + pot) * 1.3f, mouth.y + 1 + age * .95f,
               mouth.z + std::sin(age * .055f + pot * 2) * .9f },
             6 + age * .035f,
             18 + age * .08f,
             uint8_t(72 * fade) };
}

inline void Draw(PlayState* play, bool defaultInterior, bool oot) {
    if (!defaultInterior || !play || !play->state.running || !play->state.gfxCtx || play->roomCtx.curRoom.num != 0)
        return;
    auto* gfx = play->state.gfxCtx;
    // Both native allocators move the OPA tail without returning null on
    // exhaustion. Reserve the complete optional draw and debug-node slack
    // before constructing any matrices, scroll lists or display commands.
    if (!BlackMarketGraphicsArena::HasRoom(gfx, kFrameTailBytes, 8, kXluCommands, 2))
        return;
    const auto allocate = [&](size_t bytes) -> void* {
#ifdef GRAPH_ALLOC
        return GRAPH_ALLOC(gfx, bytes);
#else
        return Graph_Alloc(gfx, bytes);
#endif
    };
    auto* vertices = static_cast<Vtx*>(allocate(sizeof(Vtx) * kPuffCount * 4));
    auto* transforms = static_cast<Mtx*>(allocate(sizeof(Mtx) * 3));
    if (!vertices || !transforms)
        return;
    std::array<Gfx*, 2> scrolling;
    for (int torch = 0; torch < 2; ++torch) {
        scrolling[torch] =
            Gfx_TwoTexScrollEx(gfx, 0, 0, 0, 32, 64, 1, 0,
                               (uint32_t(0) - (play->gameplayFrames + torch * 17) * 20) & 511, 32, 128, 0, 0, 0, -20);
        if (!scrolling[torch])
            return;
    }
    // Cylindrical billboards keep a standing flame upright, matching EnLight.
    const float bx = play->billboardMtxF.xx, bz = play->billboardMtxF.zx;
    const float length = std::hypot(bx, bz);
    const float rightX = length > .001f ? bx / length : 1, rightZ = length > .001f ? bz / length : 0;
    for (int torch = 0; torch < 2; ++torch) {
        MtxF model{};
        model.xx = rightX * kFlameScale;
        model.zx = rightZ * kFlameScale;
        model.yy = kFlameScale;
        model.xz = -rightZ * kFlameScale;
        model.zz = rightX * kFlameScale;
        model.ww = 1;
        model.xw = kTorches[torch].x;
        model.yw = kTorches[torch].y;
        model.zw = kTorches[torch].z;
        Matrix_MtxFToMtx(&model, transforms + torch);
    }
    MtxF identity{};
    identity.xx = identity.yy = identity.zz = identity.ww = 1;
    Matrix_MtxFToMtx(&identity, transforms + 2);

    auto*& head = gfx->polyXlu.p;
    gDPPipeSync(head++);
    gDPSetCycleType(head++, G_CYC_2CYCLE);
    gDPSetTextureLUT(head++, G_TT_NONE);
    gDPSetAlphaCompare(head++, G_AC_NONE);
    gDPSetTexturePersp(head++, G_TP_PERSP);
    gDPSetTextureFilter(head++, G_TF_BILERP);
    gSPClearGeometryMode(head++, G_LIGHTING | G_FOG | G_CULL_BOTH | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR);
    gSPSetGeometryMode(head++, G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH);
    // The host-native display list supplies its own two-texture flame combiner
    // and texture paths. It does not overwrite the caller's prim/env palette.
    for (int torch = 0; torch < 2; ++torch) {
        gSPSegment(head++, 0x08, uintptr_t(scrolling[torch]));
        gDPSetPrimColor(head++, 0x80, 0x80, kFlamePrim[0], kFlamePrim[1], kFlamePrim[2], kFlamePrim[3]);
        gDPSetEnvColor(head++, kFlameEnv[0], kFlameEnv[1], kFlameEnv[2], kFlameEnv[3]);
        gSPMatrix(head++, transforms + torch, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gSPDisplayList(head++, reinterpret_cast<Gfx*>(const_cast<char*>(kFire)));
    }

    std::array<Puff, kPuffCount> puffs{};
    int puffCount = 0;
    // MM's private presentation removes both rear cauldrons. Preserve the
    // original OoT room and all its emitters while retaining MM's entry pots.
    for (int pot = oot ? 0 : 2; pot < int(kPotMouths.size()); ++pot)
        for (int puff = 0; puff < kPuffsPerPot; ++puff)
            puffs[puffCount++] = Vapor(pot, puff, play->gameplayFrames);
    const auto distance = [&](const Puff& puff) {
        const auto p = puff.center, e = play->view.eye;
        return (p.x - e.x) * (p.x - e.x) + (p.y - e.y) * (p.y - e.y) + (p.z - e.z) * (p.z - e.z);
    };
    std::sort(puffs.begin(), puffs.begin() + puffCount,
              [&](const Puff& a, const Puff& b) { return distance(a) > distance(b); });
    gDPPipeSync(head++);
    gDPSetCycleType(head++, G_CYC_2CYCLE);
    gDPSetRenderMode(head++, G_RM_PASS, G_RM_ZB_CLD_SURF2);
    gDPSetCombineMode(head++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA2);
    // The native flame DL can enable culling; establish smoke state explicitly.
    gSPClearGeometryMode(head++, G_LIGHTING | G_FOG | G_CULL_BOTH | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR);
    gSPSetGeometryMode(head++, G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH);
    gSPTexture(head++, 0xffff, 0xffff, 0, G_TX_RENDERTILE, G_ON);
    gDPLoadTextureBlock(head++, oot ? kOotSmoke : kMmSmoke, G_IM_FMT_I, G_IM_SIZ_8b, 32, 32, 0, G_TX_CLAMP, G_TX_CLAMP,
                        5, 5, G_TX_NOLOD, G_TX_NOLOD);
    gSPMatrix(head++, transforms + 2, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    for (int puff = 0; puff < puffCount; ++puff) {
        const auto& p = puffs[puff];
        if (!p.alpha)
            continue;
        for (int corner = 0; corner < 4; ++corner) {
            const float x = (corner == 1 || corner == 2 ? 1 : -1) * p.halfWidth;
            const float y = (corner >= 2 ? 1 : -1) * p.halfHeight;
            auto& v = vertices[puff * 4 + corner];
            v = {};
            v.v.ob[0] = std::lround(p.center.x + play->billboardMtxF.xx * x + play->billboardMtxF.xy * y);
            v.v.ob[1] = std::lround(p.center.y + play->billboardMtxF.yx * x + play->billboardMtxF.yy * y);
            v.v.ob[2] = std::lround(p.center.z + play->billboardMtxF.zx * x + play->billboardMtxF.zy * y);
            v.v.tc[0] = corner == 1 || corner == 2 ? 1024 : 0;
            v.v.tc[1] = corner >= 2 ? 0 : 1024;
            v.v.cn[0] = v.v.cn[1] = v.v.cn[2] = v.v.cn[3] = 255;
        }
        gDPSetPrimColor(head++, 0, 0, 218, 226, 218, p.alpha);
        gSPVertex(head++, uintptr_t(vertices + puff * 4), 4, 0);
        gSP2Triangles(head++, 0, 1, 2, 0, 0, 2, 3, 0);
    }
    gSPTexture(head++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
    gDPPipeSync(head++);
    // These commands are confined to XLU. Restore the native setup, neutral
    // palette and borrowed scroll segment before subsequent draw hooks/items.
    head = Gfx_SetupDL(head, 25);
    gDPSetPrimColor(head++, 0, 0, 255, 255, 255, 255);
    gDPSetEnvColor(head++, 128, 128, 128, 128);
    // This is a raw CPU segment value, not a resource-name argument. Pause can
    // leave its retired icon buffer here after reset/scene teardown; the public
    // wrapper probes that address for an OTR signature and can read freed memory.
    // Emit the existing segment value directly, without dereferencing it.
    __gSPSegment(head++, 0x08, gSegments[8]);
}
} // namespace BlackMarketInteriorEffects
