#include "global.h"
#include "2s2h/Enhancements/Gameplay/ComboWoodfallPassage.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

PlayState* gPlayState;
SaveContext gSaveContext{};
static bool ready = true;
static Mtx matrix{};

// Endpoint and engine services stop at the native command submission boundary.
extern "C" int ComboWoodfallPassage_Get(PlayState*, Vec3f* anchor, s16* yaw) {
    if (!ready)
        return 0;
    *anchor = { 1, 2, 3 };
    *yaw = 4;
    return 1;
}
extern "C" void gSPDisplayList(Gfx* packet, Gfx* list) { __gSPDisplayList(packet, list); }
extern "C" void gSPVertex(Gfx* packet, uintptr_t vertices, int count, int offset) {
    __gSPVertex(packet, vertices, count, offset);
}
extern "C" void Matrix_Push() {}
extern "C" void Matrix_Pop() {}
extern "C" void Matrix_Translate(float, float, float, MatrixMode) {}
extern "C" void Matrix_RotateYS(s16, MatrixMode) {}
extern "C" Mtx* Matrix_Finalize(GraphicsContext*) { return &matrix; }

// Debug graph bookkeeping is outside this material/geometry fixture.
#undef OPEN_DISPS
#undef CLOSE_DISPS
#define OPEN_DISPS(ctx) { GraphicsContext* __gfxCtx = (ctx)
#define CLOSE_DISPS(ctx) }
Gfx gSetupDLs[SETUPDL_MAX][6]{};
/* SETUP */
/* BUILD */
/* DRAW */

int main() {
    Gfx setup25[] = { /* LIST */ };
    // Setup explicitly writes opaque depth-tested/depth-writing state. Previous
    // translucent draws cannot turn the black mouth into an animated overlay.
    const Gfx expectedMode = gsDPSetOtherMode(
        G_AD_PATTERN | G_CD_MAGICSQ | G_CK_NONE | G_TC_FILT | G_TF_BILERP | G_TT_NONE | G_TL_TILE |
            G_TD_CLAMP | G_TP_PERSP | G_CYC_2CYCLE | G_PM_NPRIMITIVE,
        G_AC_NONE | G_ZS_PIXEL | G_RM_FOG_SHADE_A | G_RM_AA_ZB_OPA_SURF2);
    assert(setup25[3].words.w0 == expectedMode.words.w0 && setup25[3].words.w1 == expectedMode.words.w1);
    assert((setup25[3].words.w1 & Z_UPD) != 0 && (setup25[3].words.w1 & FORCE_BL) == 0);
    std::memcpy(gSetupDLs[SETUPDL_25], setup25, sizeof(setup25));
    GraphicsContext gfx{};
    PlayState play{};
    gPlayState = &play;
    play.state.gfxCtx = &gfx;
    std::vector<Gfx> first;
    for (u32 frame : { 0u, 1u, 31u, 64u, 65535u, 0xffffffffu }) {
        std::array<Gfx, 256> opa{}, xlu{};
        gfx.polyOpa.p = opa.data();
        gfx.polyXlu.p = xlu.data();
        play.gameplayFrames = frame;
        DrawWoodfallPassage();
        const size_t count = gfx.polyOpa.p - opa.data();
        assert(count > 0 && gfx.polyXlu.p == xlu.data());
        unsigned vertexLoads = 0, images = 0, tiles = 0, combines = 0, lists = 0;
        for (size_t i = 0; i < count; ++i) {
            const auto& command = opa[i];
            const auto op = command.words.w0 >> 24;
            assert(op != G_MOVEWORD && "no scroll segment binding belongs to this owner");
            if (op == G_DL) {
                assert(command.words.w1 == reinterpret_cast<uintptr_t>(gSetupDLs[SETUPDL_25]));
                ++lists;
            } else if (op == G_VTX) {
                const auto* vertices = reinterpret_cast<const Vtx*>(command.words.w1);
                assert(((command.words.w0 >> 12) & 255) == 4);
                for (int j = 0; j < 4; ++j) {
                    assert(vertices[j].v.cn[3] == 255);
                    if (vertexLoads == kQuadCount - 1) {
                        assert(vertices[j].v.cn[0] == 0 && vertices[j].v.cn[1] == 0 && vertices[j].v.cn[2] == 0);
                        assert(vertices[j].v.ob[2] == -1);
                    }
                }
                ++vertexLoads;
            } else if (op == G_SETTIMG) {
                ++images;
                assert(std::strcmp(reinterpret_cast<const char*>(command.words.w1),
                    "__OTR__scenes/nonmq/Z2_26SARUNOMORI/Z2_26SARUNOMORITex_00BAC0") == 0);
            } else if (op == G_SETTILESIZE) {
                ++tiles;
                const Gfx expected = gsDPSetTileSize(G_TX_RENDERTILE, 0, 0,
                    (32 - 1) << G_TEXTURE_IMAGE_FRAC, (32 - 1) << G_TEXTURE_IMAGE_FRAC);
                // w0 includes the upper-left origin; w1 holds tile/lower-right.
                assert(command.words.w0 == expected.words.w0 && command.words.w1 == expected.words.w1);
            } else if (op == G_SETCOMBINE) {
                Gfx expected{};
                gDPSetCombineMode(&expected, G_CC_MODULATEI, G_CC_MODULATEI);
                assert(command.words.w0 == expected.words.w0 && command.words.w1 == expected.words.w1);
                ++combines;
            }
        }
        assert(vertexLoads == kQuadCount && images == 1 && tiles == 1 && combines == 1 && lists == 2);
        if (first.empty()) {
            first.assign(opa.data(), gfx.polyOpa.p);
        } else {
            assert(first.size() == count);
            for (size_t i = 0; i < count; ++i)
                assert(first[i].words.w0 == opa[i].words.w0 && first[i].words.w1 == opa[i].words.w1);
        }
    }
    ready = false;
    std::array<Gfx, 256> opa{}, xlu{};
    gfx.polyOpa.p = opa.data();
    gfx.polyXlu.p = xlu.data();
    DrawWoodfallPassage();
    assert(gfx.polyOpa.p == opa.data() && gfx.polyXlu.p == xlu.data());
    std::puts("PASS actual Woodfall commands: static OPA bark arch, separate black backing, no XLU/scroll, endpoint guard");
}
