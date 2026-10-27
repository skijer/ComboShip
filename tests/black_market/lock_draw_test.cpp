#include "global.h"
#include "combo/BlackMarketLock.h"
#include "combo/BlackMarketGraphicsArena.h"
#include "rando/BlackMarket.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>
#ifndef OOT_LOCK_TEST
#include "combo/BlackMarketMmShopLayout.h"
#include "overlays/actors/ovl_En_Ossan/z_en_ossan.h"
#endif

using namespace ComboRando::BlackMarket;
PlayState* gPlayState;
static PlayState play;
static GraphicsContext graphics;
static std::array<Gfx, 1024> commands;
alignas(16) static std::array<Gfx, 4096> opaqueCommands;
static std::array<Gfx, 64> overlayCommands;
static std::vector<Vtx*> vertexAllocations;
static std::vector<std::string> drawnKeys;
static std::array<Actor, 8> sDisplays;
#ifndef OOT_LOCK_TEST
static EnOssan nativeShop;
static std::array<EnGirlA, 8> nativeStock;
static EnOssan* sNativeShop = &nativeShop;
static PlayState* sNativeShopPlay;
#endif
static constexpr Vec3f kShelves[8] = {
    {72,64,48},{72,104,48},{120,64,48},{120,104,48},
    {-72,64,48},{-72,104,48},{-120,64,48},{-120,104,48}
};
static int sSelected = 3, allocations, matrixAllocations, setups, draws, pushDepth;
static int shelfDraws, interiorEffects;
void DrawShelves(PlayState*) { ++shelfDraws; }
static bool sTalking = true, sTitlePending, sGrannyActive, active = true, interior = true, allocate = true, report = true;
static bool allOffers = false, refillOffer = false;
static int soldOutSlot = -1;
namespace BlackMarketInteriorEffects { void Draw(PlayState*, bool enabled, bool) { if (enabled) ++interiorEffects; } }
static std::vector<Vec3f> translations;
static std::vector<float> scales;
// These are fresh arena vertices, so the bridge's native raw-pointer packet
// applies directly; no resource-path resolution is involved.
extern "C" void gSPVertex(Gfx* packet, uintptr_t vertices, int count, int start) {
    __gSPVertex(packet, vertices, count, start);
}
bool Active() { return active; }
bool Interior() { return interior; }
static bool seals = true;
int CVarGetInteger(const char* name,int fallback) {
    return std::strcmp(name,BlackMarketVisual::kSealsCvar)==0 ? seals : fallback;
}
bool Safe(PlayState*) { return false; }
void PresentTitle() { assert(false); }
const Item* Offer(int slot) {
    return slot == 7 && !allOffers ? nullptr : slot == 6 && refillOffer ? Find("wildcard.chateau") : &Catalog()[slot];
}
PurchaseResult Status(int slot) {
    return slot == soldOutSlot ? PurchaseResult::SoldOut : slot == 0 || slot == 2 ? PurchaseResult::Locked : PurchaseResult::Ready;
}
void SubmitOffer(const Item& item) {
    const int slot = int(&item - Catalog().data());
    const bool locked = Status(slot) == PurchaseResult::Locked;
    assert(BlackMarketVisual::Active() == locked && "only locked offers should query geometry");
    if (report) BlackMarketVisual::Report({10.f + slot,20,30}, {16.f + slot,40,34},
                                        [](BlackMarketVisual::Point point) { return point; });
    drawnKeys.emplace_back(item.key);
    ++draws;
}
void DrawOffer(PlayState*, const Item& item) { SubmitOffer(item); }
namespace ComboBlackMarketItems { int Id(const Item& item) { return int(&item - Catalog().data()); } }
namespace Rando { void DrawResolvedItem(int id, int, Actor* actor) {
#ifndef OOT_LOCK_TEST
    bool owned = false;
    for (int slot = 0; slot < 8; ++slot)
        owned |= actor == &nativeStock[slot].actor && Offer(slot) && ComboBlackMarketItems::Id(*Offer(slot)) == id;
    assert(owned && "render the real native actor for this seeded stock slot");
#endif
    SubmitOffer(Catalog()[id]);
} }
/* NATIVE_ALLOCATION */
/* NATIVE_DEBUG */
void* Allocate(size_t bytes) {
    assert(bytes == 64 * sizeof(Vtx));
    if (!allocate) return nullptr;
    auto* vertices = static_cast<Vtx*>(NativeAllocate(&graphics, bytes));
    ++allocations;
    vertexAllocations.push_back(vertices);
    return vertices;
}
Mtx* NewMatrix() {
    ++matrixAllocations;
    auto* matrix = static_cast<Mtx*>(NativeAllocate(&graphics, sizeof(Mtx)));
    *matrix = {};
    return matrix;
}
#undef OPEN_DISPS
#undef CLOSE_DISPS
#ifdef OOT_LOCK_TEST
#define OPEN_DISPS(ctx) { GraphicsContext* __gfxCtx = (ctx); Gfx* dispRefs[4]; \
    Graph_OpenDisps(dispRefs, ctx, __FILE__, __LINE__);
#define CLOSE_DISPS(ctx) Graph_CloseDisps(dispRefs, ctx, __FILE__, __LINE__); }
#else
#define OPEN_DISPS(ctx) { GraphicsContext* __gfxCtx = (ctx); Gfx* dispRefs[3]; Gfx dispVals[3]; \
    Graph_OpenDisps(dispRefs, dispVals, ctx, __FILE__, __LINE__);
#define CLOSE_DISPS(ctx) Graph_CloseDisps(dispRefs, dispVals, ctx, __FILE__, __LINE__); }
#endif
void Setup(GraphicsContext* ctx) {
    OPEN_DISPS(ctx);
    ++setups;
    // SETUPDL_25 is opaque even when appended to the XLU stream. Poison LUT
    // too, so the actual effect must explicitly establish every required state.
    gDPSetRenderMode(ctx->polyXlu.p++, G_RM_FOG_SHADE_A, G_RM_AA_ZB_OPA_SURF2);
    gDPSetTextureLUT(ctx->polyXlu.p++, G_TT_RGBA16);
    gSPTexture(ctx->polyXlu.p++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
    CLOSE_DISPS(ctx);
}
#undef GRAPH_ALLOC
#define GRAPH_ALLOC(ctx, bytes) Allocate(bytes)
#define Graph_Alloc(ctx, bytes) Allocate(bytes)
#define Gfx_SetupDL25_Xlu(ctx) Setup(ctx)
#define Gfx_SetupDL_25Xlu(ctx) Setup(ctx)
#define Matrix_Push() (++pushDepth)
#define Matrix_Pop() assert(--pushDepth >= 0)
#define Matrix_Translate(x,y,z,mode) translations.push_back({x,y,z})
#define Matrix_Scale(x,y,z,mode) scales.push_back(x)
#define Matrix_RotateY(...) ((void)0)
#define Matrix_RotateYS(...) ((void)0)
#define Matrix_NewMtx(...) NewMatrix()
#undef MATRIX_FINALIZE_AND_LOAD
#define MATRIX_FINALIZE_AND_LOAD(packet, ...) \
    gSPMatrix(packet, NewMatrix(), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW)
/* PRODUCTION */

void Reset() {
    play = {}; graphics = {}; commands = {}; opaqueCommands = {}; overlayCommands = {};
    gPlayState = &play; play.state.running = true; play.state.gfxCtx = &graphics;
    graphics.polyOpa.p = opaqueCommands.data(); graphics.polyOpa.d = opaqueCommands.data() + opaqueCommands.size();
    graphics.polyXlu.p = commands.data(); graphics.polyXlu.d = commands.data() + commands.size();
    graphics.overlay.p = overlayCommands.data(); graphics.overlay.d = overlayCommands.data() + overlayCommands.size();
    play.gameplayFrames = 31;
    allocations = matrixAllocations = setups = draws = pushDepth = shelfDraws = interiorEffects = 0;
    vertexAllocations.clear();
    drawnKeys.clear(); allOffers = refillOffer = false; soldOutSlot = -1; sSelected = 3;
    translations.clear(); scales.clear(); active = interior = allocate = report = true;
    sTalking = true; sTitlePending = sGrannyActive = false;
#ifndef OOT_LOCK_TEST
    nativeShop = {}; nativeStock = {}; sNativeShopPlay = &play;
    for (int i = 0; i < 8; ++i) {
        nativeShop.items[i] = &nativeStock[i];
        nativeStock[i].actor.world.pos = kShelves[i];
        nativeStock[i].actor.scale = {.25f,.25f,.25f};
        nativeStock[i].actor.update = [](Actor*, PlayState*) {};
    }
#endif
}
void CheckPackets(int effects) {
    int vertexBatches = 0, triangles = 0, syncs = 0;
    uint32_t geometry = G_LIGHTING | G_FOG | G_CULL_BOTH, low = 0, high = 0;
    bool texture = false;
    Gfx combine{};
    gDPSetCombineMode(&combine, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
    uintptr_t combine0 = 0, combine1 = 0, image = 0;
    for (const Gfx* cmd = commands.data(); cmd != graphics.polyXlu.p; ++cmd) {
        const uint32_t op = uint32_t(cmd->words.w0) >> 24;
        if (op == G_GEOMETRYMODE) geometry = (geometry & (cmd->words.w0 & 0xffffff)) | cmd->words.w1;
        if (op == G_SETOTHERMODE_H || op == G_SETOTHERMODE_L) {
            const unsigned length = (cmd->words.w0 & 255) + 1;
            const unsigned shift = 32 - ((cmd->words.w0 >> 8) & 255) - length;
            const uint32_t mask = uint32_t((uint64_t(1) << length) - 1) << shift;
            auto& target = op == G_SETOTHERMODE_H ? high : low;
            target = (target & ~mask) | (cmd->words.w1 & mask);
        }
        if (op == G_TEXTURE) texture = (cmd->words.w0 & 255) != 0;
        if (op == G_SETTIMG) image = cmd->words.w1;
        if (op == G_SETCOMBINE) { combine0 = cmd->words.w0; combine1 = cmd->words.w1; }
        if (op == G_RDPPIPESYNC) ++syncs;
        if (op == G_VTX) {
            assert((geometry & (G_LIGHTING | G_FOG | G_CULL_BOTH)) == 0);
            const uint32_t mask = ~((1u << G_MDSFT_RENDERMODE) - 1);
            assert((low & mask) == uint32_t(G_RM_PASS | G_RM_AA_ZB_XLU_SURF2));
            assert((low & Z_UPD) == 0 && (low & FORCE_BL) != 0);
            assert((high & (3u << G_MDSFT_TEXTLUT)) == G_TT_NONE && texture);
            assert(combine0 == combine.words.w0 && combine1 == combine.words.w1);
            assert(image == uintptr_t(BlackMarketVisual::kLockTexture.data()));
            assert(((cmd->words.w0 >> 12) & 255) == 32);
            assert(cmd->words.w1 == uintptr_t(vertexAllocations[vertexBatches / 2] + (vertexBatches % 2) * 32));
            ++vertexBatches;
        }
        if (op == G_TRI2) ++triangles;
    }
    assert(vertexBatches == effects * 2 && triangles == effects * 16);
    assert(syncs >= effects && setups == effects * 2 && !texture && pushDepth == 0);
}
int main() {
    const BlackMarketVisual::Frame frame{{10,20,30},{16,40,34},true};
    for (int shortage = 0; shortage < 6; ++shortage) {
        Reset();
        if (shortage == 0) graphics.polyOpa.d = graphics.polyOpa.p + (64 * sizeof(Vtx) - 16) / sizeof(Gfx);
        if (shortage == 1) graphics.polyOpa.d = graphics.polyOpa.p + (64 * sizeof(Vtx) + sizeof(Mtx)) / sizeof(Gfx) + 7;
        // Data alone fits exactly, but native debug OPEN_DISPS still writes at the OPA head.
        if (shortage == 2) graphics.polyOpa.d = graphics.polyOpa.p + (64 * sizeof(Vtx) + sizeof(Mtx)) / sizeof(Gfx);
        if (shortage == 3) graphics.polyXlu.d = graphics.polyXlu.p + 63;
        if (shortage == 4) graphics.overlay.d = graphics.overlay.p + 1;
        if (shortage == 5) graphics.polyOpa.d = reinterpret_cast<Gfx*>(uintptr_t(graphics.polyOpa.p) - 16);
        const auto before = graphics;
        const auto opaBefore = opaqueCommands;
        const auto xluBefore = commands;
        const auto overlayBefore = overlayCommands;
        DrawSeal(&play, frame, true);
        assert(allocations == 0 && matrixAllocations == 0 && setups == 0 && translations.empty() &&
               "native short arenas must skip the entire seal before allocation or command writes");
        assert(graphics.polyOpa.p == before.polyOpa.p && graphics.polyOpa.d == before.polyOpa.d);
        assert(graphics.polyXlu.p == before.polyXlu.p && graphics.polyXlu.d == before.polyXlu.d);
        assert(graphics.overlay.p == before.overlay.p && graphics.overlay.d == before.overlay.d);
        assert(!std::memcmp(opaqueCommands.data(), opaBefore.data(), sizeof(opaqueCommands)));
        assert(!std::memcmp(commands.data(), xluBefore.data(), sizeof(commands)));
        assert(!std::memcmp(overlayCommands.data(), overlayBefore.data(), sizeof(overlayCommands)));
    }
    Reset();
    graphics.polyOpa.d = graphics.polyOpa.p + (64 * sizeof(Vtx) + sizeof(Mtx)) / sizeof(Gfx) + 8;
    graphics.polyXlu.d = graphics.polyXlu.p + 64;
    graphics.overlay.d = graphics.overlay.p + 2;
    DrawSeal(&play, frame, true);
    assert(allocations == 1 && matrixAllocations == 1 && graphics.polyOpa.p <= graphics.polyOpa.d &&
           graphics.polyXlu.p <= graphics.polyXlu.d && graphics.overlay.p <= graphics.overlay.d);
    Reset();
    DrawSeal(&play, frame, true);
    assert(allocations == 1 && translations.size() == 1);
    assert(translations[0].x == 13 && translations[0].y == 30 && translations[0].z == 32);
    assert(scales.size() == 1 && scales[0] == 1.f / BlackMarketVisual::kLockVertexScale);
    float sealHalfHeight = 0;
    for (int i = 0; i < 64; ++i)
        sealHalfHeight =
            std::max(sealHalfHeight, std::abs(vertexAllocations[0][i].v.ob[1] / BlackMarketVisual::kLockVertexScale));
#ifndef OOT_LOCK_TEST
    assert(sealHalfHeight > 10 && sealHalfHeight <= 12.7f &&
           "MM ribbons must wrap the item with compact padding for larger shelf stock");
#else
    assert(sealHalfHeight > 12.7f && "OoT must retain its original ribbon padding");
#endif
    const auto expected = BlackMarketVisual::BuildLock(frame, play.gameplayFrames, true,
#ifdef OOT_LOCK_TEST
                                                       1.f
#else
                                                       .5f
#endif
    );
    assert(!BlackMarketVisual::BuildLock(frame, 0, false, 0).valid);
    assert(!BlackMarketVisual::BuildLock(frame, 0, false, NAN).valid);
    for (int i = 0; i < 64; ++i) {
        const auto& actual = vertexAllocations[0][i].v; const auto& want = expected.vertices[i];
        assert(actual.ob[0] == want.x && actual.ob[1] == want.y && actual.ob[2] == want.z);
        assert(actual.tc[0] == want.s && actual.tc[1] == want.t && actual.cn[3] == 255);
    }
    CheckPackets(1);
    Reset(); DrawSeal(&play, {}, false);
    assert(allocations == 0 && graphics.polyXlu.p == commands.data() && translations.empty());
    allocate = false; DrawSeal(&play, frame, false);
    assert(allocations == 0 && graphics.polyXlu.p == commands.data() && translations.empty());
    Reset(); allOffers = true; sSelected = 6; DrawMarket();
    const auto fullStock = drawnKeys;
    assert(fullStock.size() == 8 && draws == 8 && allocations == 2);
    CheckPackets(2);
    Reset(); allOffers = true; sSelected = soldOutSlot = 6; DrawMarket();
    auto unsoldStock = fullStock; unsoldStock.erase(unsoldStock.begin() + 6);
    assert(drawnKeys == unsoldStock && draws == 7 && allocations == 2 &&
           "a sold one-time offer must disappear while the other seven merchandise models and seals stay intact");
    assert(sSelected == 6 && Offer(sSelected) && Status(sSelected) == PurchaseResult::SoldOut &&
           "a sold shelf remains selectable for its saved sold-out dialogue");
    CheckPackets(2);
    Reset(); allOffers = refillOffer = true; soldOutSlot = 6; DrawMarket();
    assert(draws == 8 && drawnKeys[6] == "wildcard.chateau" && allocations == 2 &&
           "a renewable refill remains visible even when purchase state exists for its slot");
    CheckPackets(2);
    Reset(); DrawMarket();
    assert(shelfDraws == 1 && "native shop must draw physical shelves in the shared room-appearance path");
    assert(interiorEffects == 1 && "the default room must draw its native flames/vapor independently of item stock");
    assert(draws == 7 && allocations == 2 && !BlackMarketVisual::Active());
    assert(translations.size() == 9 && translations[1].x == 13 && translations[4].x == 15);
    CheckPackets(2);
    Reset(); report = false; DrawMarket();
    assert(draws == 7 && allocations == 0 && graphics.polyXlu.p == commands.data());
    Reset(); seals = false; DrawMarket();
    assert(shelfDraws == 1 && "hiding seals must not hide shelves");
    assert(draws == 7 && allocations == 0 && !BlackMarketVisual::Active());
    seals = true;
    Reset(); active = false; DrawMarket();
    assert(draws == 0 && allocations == 0 && shelfDraws == 1 && interiorEffects == 1 &&
           "an interior without active stock still presents its physical shelves");
    Reset(); interior = false; DrawMarket(); assert(draws == 0 && allocations == 0 && shelfDraws == 0 && interiorEffects == 0);
    Reset(); sGrannyActive = true; DrawMarket(); assert(interiorEffects == 0 && "Granny owns its existing flame/smoke path");
    Reset(); play.state.running = false; DrawMarket(); assert(draws == 0 && allocations == 0);
#ifndef OOT_LOCK_TEST
    Reset();
    nativeStock[3].actor.world.pos = {123.25f, 77.5f, -99.0f};
    nativeStock[3].actor.scale = {.27f,.27f,.27f};
    DrawMarket();
    assert(nativeStock[3].actor.world.pos.x == 123.25f && nativeStock[3].actor.world.pos.z == -99.0f);
    bool sawNativePosition = false;
    for (const auto& translation : translations)
        sawNativePosition |= translation.x == 123.25f && translation.z == -99.0f;
    assert(sawNativePosition && "draw must retain the native selected-item tween position");
    assert(std::find(scales.begin(), scales.end(), .27f) != scales.end());
#endif
    std::puts("PASS native market X: native unbounded allocation with OPA/matrix/command/overlay shortage guards, real GBI blend/depth/LUT/texture/combiner, 64 fitted vertices, locked-only capture and restored state");
}
