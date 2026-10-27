#include "global.h"
#include "combo/BlackMarketShelves.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

static PlayState play;
static GraphicsContext graphics;
static std::array<Gfx, 4096> commands;
static std::array<Gfx, 16> overlay;
alignas(16) static std::array<unsigned char, 32768> arena;
static size_t used;
static int allocations, failAllocation = -1;
static std::vector<MtxF> matrices;
static int flameLists, vertexLoads;
static std::vector<std::array<Vtx, 4>> quads;
static int scrolls;
static int restores;
uintptr_t gSegments[16]{};
/* SHOP ANCHORS */
/* NATIVE SETUP */

void* Allocate(size_t bytes) {
    if (++allocations == failAllocation) return nullptr;
    const size_t aligned = (bytes + 15) & ~size_t(15);
    used += aligned;
    assert(used <= arena.size());
    graphics.polyOpa.d = reinterpret_cast<Gfx*>(reinterpret_cast<uintptr_t>(graphics.polyOpa.d) - aligned);
    return graphics.polyOpa.d;
}
Mtx* CaptureMatrix(MtxF* source, Mtx* destination) { matrices.push_back(*source); return destination; }
Gfx* RestoreSetup(Gfx* head, u32 index) {
    assert(index == 25); ++restores;
    for (const auto c : nativeSetup) {
        if ((c.words.w0 >> 24) == G_ENDDL) break;
        *head++ = c;
    }
    return head;
}
void CaptureList(Gfx* packet, const void* list) {
    assert(std::strcmp(static_cast<const char*>(list), "__OTR__objects/gameplay_keep/gEffFire1DL") == 0);
    uint32_t high = 0, prim = 0, env = 0;
    for (auto* c = commands.data(); c < packet; ++c) {
        const auto op = uint32_t(c->words.w0 >> 24);
        if (op == G_SETPRIMCOLOR) prim = c->words.w1;
        if (op == G_SETENVCOLOR) env = c->words.w1;
        if (op == G_SETOTHERMODE_H) {
            const unsigned length = (c->words.w0 & 255) + 1;
            const unsigned shift = 32 - ((c->words.w0 >> 8) & 255) - length;
            const uint32_t mask = uint32_t((uint64_t(1) << length) - 1) << shift;
            high = (high & ~mask) | (c->words.w1 & mask);
        }
    }
    assert((high & (3u << G_MDSFT_CYCLETYPE)) == G_CYC_2CYCLE &&
           "native decorative flames require a two-cycle setup before their display list");
    assert(prim == 0xe1cdffff && env == 0x5a2da000 && "decorative flames must use pale violet prim/env, never orange");
    ++flameLists; __gSPDisplayList(packet, list);
}
extern "C" void gSPVertex(Gfx* packet, uintptr_t vertices, int count, int start) {
    assert(count == 4 && start == 0);
    std::array<Vtx, 4> quad; std::memcpy(quad.data(), reinterpret_cast<void*>(vertices), sizeof(quad));
    quads.push_back(quad); ++vertexLoads; __gSPVertex(packet, vertices, count, start);
}
extern "C" char* ResourceMgr_LoadIfDListByName(char*) {
    assert(false && "the effects only bind native frame lists, never resource-name strings");
    return nullptr;
}
/* NATIVE SEGMENT WRAPPER */
Gfx* MakeScroll(GraphicsContext*, s32 tile0, u32 x0, u32 y0, s32 w0, s32 h0, s32 tile1,
                u32 x1, u32 y1, s32 w1, s32 h1, s32 dx0, s32 dy0, s32 dx1, s32 dy1) {
    assert(tile0 == 0 && tile1 == 1 && !x0 && !y0 && !x1 && y1 < 512);
    assert(w0 == 32 && h0 == 64 && w1 == 32 && h1 == 128 && !dx0 && !dy0 && !dx1 && dy1 == -20);
    ++scrolls; return static_cast<Gfx*>(Allocate(12 * sizeof(Gfx)));
}
#undef GRAPH_ALLOC
#ifdef OOT_EFFECT_TEST
#define Graph_Alloc(ctx, bytes) Allocate(bytes)
#else
#define GRAPH_ALLOC(ctx, bytes) Allocate(bytes)
#endif
#define Matrix_MtxFToMtx CaptureMatrix
#define Gfx_TwoTexScrollEx MakeScroll
#define gSPDisplayList CaptureList
#define Gfx_SetupDL RestoreSetup

#if __has_include("combo/BlackMarketInteriorEffects.h")
#include "combo/BlackMarketInteriorEffects.h"
#else
namespace BlackMarketInteriorEffects { void Draw(PlayState*, bool, bool) {} }
#endif

void Reset() {
    play = {}; graphics = {}; commands = {}; arena = {}; used = 0;
    allocations = flameLists = vertexLoads = scrolls = restores = 0; failAllocation = -1;
    matrices.clear(); quads.clear(); play.state.running = true; play.state.gfxCtx = &graphics;
    graphics.polyXlu.p = commands.data();
    graphics.polyXlu.d = commands.data() + commands.size();
    graphics.polyOpa.p = reinterpret_cast<Gfx*>(arena.data());
    graphics.polyOpa.d = reinterpret_cast<Gfx*>(arena.data() + arena.size());
    graphics.overlay.p = overlay.data(); graphics.overlay.d = overlay.data() + overlay.size();
    play.billboardMtxF.xx = play.billboardMtxF.yy = play.billboardMtxF.zz = play.billboardMtxF.ww = 1;
    play.view.eye = {0, 80, 300};
    static std::array<char, 16> rawSegment{};
    gSegments[8] = reinterpret_cast<uintptr_t>(rawSegment.data());
}
void CheckPackets(bool oot) {
    uint32_t geometry = G_LIGHTING | G_FOG | G_CULL_BOTH, high = 0, low = 0, prim = 0, env = 0;
    uintptr_t image = 0, segment8 = 0;
    bool texture = false, intensity8 = false;
    int batches = 0, triangles = 0;
    Gfx combine{}, expected{};
    gDPSetCombineMode(&expected, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA2);
    for (auto* c = commands.data(); c < graphics.polyXlu.p; ++c) {
        const auto op = uint32_t(c->words.w0 >> 24);
        if (op == G_GEOMETRYMODE) geometry = (geometry & (c->words.w0 & 0xffffff)) | c->words.w1;
        if (op == G_SETTIMG) image = c->words.w1;
        if (op == G_TEXTURE) texture = (c->words.w0 & 255) != 0;
        if (op == G_SETPRIMCOLOR) prim = c->words.w1;
        if (op == G_SETENVCOLOR) env = c->words.w1;
        if (op == G_SETCOMBINE) combine = *c;
        if (op == G_SETTILE && ((c->words.w1 >> 24) & 7) == 0)
            intensity8 = ((c->words.w0 >> 21) & 7) == G_IM_FMT_I && ((c->words.w0 >> 19) & 3) == G_IM_SIZ_8b;
        if (op == G_SETOTHERMODE_H || op == G_SETOTHERMODE_L) {
            const unsigned length = (c->words.w0 & 255) + 1, shift = 32 - ((c->words.w0 >> 8) & 255) - length;
            const uint32_t mask = uint32_t((uint64_t(1) << length) - 1) << shift;
            auto& mode = op == G_SETOTHERMODE_H ? high : low;
            mode = (mode & ~mask) | (c->words.w1 & mask);
        }
        if (op == G_MOVEWORD && ((c->words.w0 >> 16) & 255) == G_MW_SEGMENT && (c->words.w0 & 0xffff) == 32)
            segment8 = c->words.w1;
        if (op == G_VTX) {
            ++batches;
            assert((geometry & (G_LIGHTING | G_FOG | G_CULL_BOTH)) == 0 && (geometry & G_ZBUFFER));
            assert((low & Z_UPD) == 0 && (low & FORCE_BL) && (low & 3) == G_AC_NONE);
            assert((high & (3u << G_MDSFT_CYCLETYPE)) == G_CYC_2CYCLE);
            assert((high & (3u << G_MDSFT_TEXTLUT)) == G_TT_NONE && texture && intensity8);
            assert((prim & 255) > 0 && (prim & 255) <= 72 && (prim >> 8) == 0xdae2da);
            assert(combine.words.w0 == expected.words.w0 && combine.words.w1 == expected.words.w1 &&
                   "cloud texture intensity and per-puff alpha must both participate in the native combiner");
            assert(std::strcmp(reinterpret_cast<const char*>(image), oot ? BlackMarketInteriorEffects::kOotSmoke : BlackMarketInteriorEffects::kMmSmoke) == 0);
        }
        if (op == G_TRI2) triangles += 2;
    }
    assert(batches == vertexLoads && triangles == vertexLoads * 2);
    assert(prim == 0xffffffff && env == 0x80808080 && segment8 == gSegments[8] && restores == 1);
    assert((geometry & (G_FOG | G_LIGHTING | G_CULL_BACK)) == (G_FOG | G_LIGHTING | G_CULL_BACK) &&
           "effect-specific geometry, palette and borrowed segment must not leak into later actors");
}
int main() {
    Reset(); BlackMarketInteriorEffects::Draw(&play, true, false);
    assert(flameLists == 2 && vertexLoads >= 20 && vertexLoads <= 24 &&
           "MM keeps decorative flames and only the two entrance-side vapor emitters");
    for (const auto& quad : quads) {
        int z = 0;
        for (const auto& vertex : quad) z += vertex.v.ob[2];
        assert(z > 800 && "removed rear pots must not leave vapor crossing the shelf banks");
    }
    assert(matrices.size() == 3 && scrolls == 2 && used < 8192);
    assert(used == BlackMarketInteriorEffects::kFrameTailBytes &&
           graphics.polyXlu.p - commands.data() <= BlackMarketInteriorEffects::kXluCommands);
    assert(restores == 1 && "restore the host's native setup after the optical effects");
    CheckPackets(false);
    assert(BlackMarketInteriorEffects::kTorches[0].x == 75 && BlackMarketInteriorEffects::kTorches[1].x == -110 &&
           BlackMarketInteriorEffects::kTorches[0].z == 205 && BlackMarketInteriorEffects::kTorches[1].z == 205 &&
           "default decorative flames must follow their relocated complete stands outside the shelf banks");
    for (int torch = 0; torch < 2; ++torch) {
        const auto p = BlackMarketInteriorEffects::kTorches[torch];
        const auto& m = matrices[torch];
        assert(m.xw == p.x && m.yw == p.y && m.zw == p.z && std::abs(m.yy - .004f) < 1e-6f);
        // Actual native fire bounds are +/-4000, Y[0,16000]. Reserve every
        // cylindrical billboard orientation, including its entire upper tip.
        const Vec3f low{p.x-16,p.y,p.z-16}, high{p.x+16,p.y+64,p.z+16};
        const auto overlap = [&](Vec3f a, Vec3f b) {
            return high.x >= a.x && low.x <= b.x && high.y >= a.y && low.y <= b.y && high.z >= a.z && low.z <= b.z;
        };
        for (const auto& shelf : BlackMarketShelves::kCollision)
            assert(!overlap({float(shelf.low.x),float(shelf.low.y),float(shelf.low.z)},
                            {float(shelf.high.x),float(shelf.high.y),float(shelf.high.z)}));
        for (const auto item : kShelves)
            assert(!overlap({item.x-18,item.y-16,item.z-18},{item.x+18,item.y+29,item.z+18}) &&
                   "the complete native flame must clear every selected item/seal envelope");
        // Retained neighbors audited from complete connected prop components,
        // not just the potion liquid mouths or the torch origins.
        const Vec3f neighbors[][2] = {
            {{138,84,140},{155,117,174}}, // capped right canisters and lids
            {{118,74,171},{140,120,189}}, // bone curio
            {{107,8,176},{160,75,236}},  // cabinet and fittings
            {{94,8,218},{143,47,267}},   // middle-right cauldron body
            {{34,129,144},{77,181,186}}, // rear-right hanging basket
            {{61,125,228},{105,181,272}},// front-right hanging basket
            {{-85,107,156},{-31,175,210}},// rear-left hanging basket
            {{-128,124,239},{-81,180,285}},// front-left hanging basket
            {{112,90,161},{153,148,199}} // hanging herbs over bone curio
        };
        for (const auto& bounds : neighbors) {
            assert(!overlap(bounds[0],bounds[1]) && "the full flame must clear retained pots, canisters, curios and hanging props");
            const Vec3f standLow{p.x-(torch ? 17:19),0,p.z-11}, standHigh{p.x+(torch ? 17:15),83,p.z+21};
            assert(!(standHigh.x >= bounds[0].x && standLow.x <= bounds[1].x &&
                     standHigh.y >= bounds[0].y && standLow.y <= bounds[1].y &&
                     standHigh.z >= bounds[0].z && standLow.z <= bounds[1].z) &&
                   "the complete relocated torch stand must clear retained props");
        }
    }
    assert(matrices[2].xx == 1 && matrices[2].yy == 1 && matrices[2].zz == 1 && !matrices[2].xw);
    const auto firstFrame = quads;
    Reset(); BlackMarketInteriorEffects::Draw(&play, true, false);
    assert(firstFrame.size() == quads.size() && std::memcmp(firstFrame.data(), quads.data(), quads.size()*sizeof(quads[0])) == 0 &&
           "pause/repeated draws at the same gameplay frame must not advance or mutate effects");
    for (int pot = 2; pot < 4; ++pot) {
        float minY = 1000, maxY = -1000;
        int count = 0;
        for (const auto& quad : quads) {
            const float x = (quad[0].v.ob[0] + quad[2].v.ob[0]) * .5f;
            const float y = (quad[0].v.ob[1] + quad[2].v.ob[1]) * .5f;
            const float z = (quad[0].v.ob[2] + quad[2].v.ob[2]) * .5f;
            const auto origin = BlackMarketInteriorEffects::kPotMouths[pot];
            if (std::abs(x-origin.x) < 2 && std::abs(z-origin.z) < 2) {
                ++count; minY = std::min(minY,y); maxY = std::max(maxY,y);
            }
        }
        assert(count >= 11 && maxY-minY > 90 && "each actual pot mouth must have a sustained tall vapor column");
        const auto a = BlackMarketInteriorEffects::Vapor(pot,0,0), b = BlackMarketInteriorEffects::Vapor(pot,0,1);
        assert(std::abs(b.center.y-a.center.y-.95f) < .001f);
        assert(a.halfHeight >= 18 && a.halfHeight >= a.halfWidth * 2 &&
               "vertically stretched native cloud cores must overlap into steam rather than discrete round beads");
    }
    for (int disabled = 0; disabled < 5; ++disabled) {
        Reset();
        if (disabled == 1) play.state.running = false;
        if (disabled == 2) play.roomCtx.curRoom.num = 1;
        if (disabled == 3) play.state.gfxCtx = nullptr;
        BlackMarketInteriorEffects::Draw(disabled == 4 ? nullptr : &play, disabled != 0, false);
        assert(!allocations && !flameLists && !vertexLoads && graphics.polyXlu.p == commands.data());
    }
    for (int failure = 1; failure <= 4; ++failure) {
        Reset(); failAllocation = failure; BlackMarketInteriorEffects::Draw(&play,true,false);
        assert(!flameLists && !vertexLoads && graphics.polyXlu.p == commands.data() && matrices.empty());
    }
    for (int exhausted = 0; exhausted < 3; ++exhausted) {
        Reset();
        auto& stream = exhausted == 0 ? graphics.polyOpa : exhausted == 1 ? graphics.polyXlu : graphics.overlay;
        if (exhausted == 0)
            stream.d = reinterpret_cast<Gfx*>(arena.data() + BlackMarketInteriorEffects::kFrameTailBytes + 8*sizeof(Gfx)-1);
        else
            stream.d = stream.p + (exhausted == 1 ? BlackMarketInteriorEffects::kXluCommands-1 : 1);
        const auto opaTail = graphics.polyOpa.d;
        BlackMarketInteriorEffects::Draw(&play,true,false);
        assert(!allocations && !scrolls && matrices.empty() && !flameLists && !vertexLoads &&
               graphics.polyXlu.p == commands.data() && graphics.polyOpa.d == opaTail &&
               "real frame arena p/d exhaustion must skip before any native allocation or scroll construction");
    }
    Reset();
    graphics.polyOpa.d = reinterpret_cast<Gfx*>(arena.data() + BlackMarketInteriorEffects::kFrameTailBytes + 8*sizeof(Gfx));
    graphics.polyXlu.d = commands.data() + BlackMarketInteriorEffects::kXluCommands;
    graphics.overlay.d = overlay.data() + 2;
    BlackMarketInteriorEffects::Draw(&play,true,false);
    assert(flameLists == 2 && vertexLoads >= 20 && graphics.polyOpa.d >= graphics.polyOpa.p + 8 &&
           graphics.polyXlu.p <= graphics.polyXlu.d && "the full declared frame budget must suffice without an overrun");
    Reset(); play.gameplayFrames = UINT32_MAX; BlackMarketInteriorEffects::Draw(&play,true,true);
    assert(flameLists == 2 && vertexLoads >= 40);
    CheckPackets(true);
    bool nativeOotSmoke = false;
    for (auto* c = commands.data(); c < graphics.polyXlu.p; ++c)
        if ((c->words.w0 >> 24) == G_SETTIMG)
            nativeOotSmoke |= std::strcmp(reinterpret_cast<const char*>(c->words.w1),"__OTR__objects/gameplay_keep/gDust1Tex") == 0;
    assert(nativeOotSmoke && "OoT must resolve its own native cloud texture without a cross-game archive dependency");
    // Pause stores an icon buffer in CPU segment 8. Reset/scene teardown can
    // free that buffer while leaving its address in the segment table. Copying
    // the raw segment value into a GBI packet must not probe its old memory as
    // a resource name. Exercise the real host wrapper and signature check.
    for (bool oot : { false, true }) {
        Reset();
        auto* retired = static_cast<char*>(std::malloc(64));
        assert(retired);
        std::memset(retired, 0, 64);
        gSegments[8] = reinterpret_cast<uintptr_t>(retired);
        std::free(retired);
        BlackMarketInteriorEffects::Draw(&play, true, oot);
        CheckPackets(oot);
        assert(flameLists == 2 && vertexLoads >= (oot ? 40 : 20));
    }
    std::puts("PASS native interior effects: two pale-violet native fire lists/scrolls; full flame-shelf/item clearance; MM entry-only/OoT four-column vapor; native host texture ownership; frame/pause determinism; default-only guards; allocation failure and frame wrap");
}
