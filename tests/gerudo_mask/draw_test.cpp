#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
extern "C" {
#include "global.h"
}
#include "Rando/NeiResourceRouting.h"
#include "2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h"
#include "ComboItemDrawABI.h"

static GraphicsContext gfx;
static PlayState play;
static Mtx matrix;
static Gfx opa[64];
static bool ownerReady, directReady;
static int ownerQueries, directDraws, legacyDraws, shimmerDraws;
static std::vector<std::string> submitted;
static constexpr const char* warrior = "__OTR__objects/object_gi_gerudo_warrior/gGiGerudoWarriorMaskDL";
extern "C" int32_t OOT_NeiResourceExists(const char* path) {
    assert(!std::strcmp(path, warrior));
    ++ownerQueries;
    return ownerReady;
}
void FrameInterpolation_RecordOpenChild(const void*, int) {}
void FrameInterpolation_RecordCloseChild() {}
extern "C" {
PlayState* gPlayState = &play;
void gSPDisplayList(Gfx* command, Gfx* dl) {
    submitted.emplace_back(reinterpret_cast<const char*>(dl));
    __gSPDisplayList(command, dl);
}
Mtx* Matrix_Finalize(GraphicsContext*) { return &matrix; }
void Gfx_SetupDL25_Opa(GraphicsContext*) {}
void Graph_OpenDisps(Gfx**, Gfx*, GraphicsContext*, const char*, s32) {}
void Graph_CloseDisps(Gfx**, Gfx*, GraphicsContext*, const char*, s32) {}
}
static bool DrawOotDirectOpa(const char* path, Gfx**) {
    assert(!std::strcmp(path, "__OTR__objects/object_gi_gerudomask/gGiGerudoMaskDL"));
    ++directDraws;
    return directReady;
}
static void DrawOotGetItemOpa(const char* path, Gfx**) {
    assert(!std::strcmp(path, "__OTR__objects/object_gi_gerudomask/gGiGerudoMaskDL"));
    ++legacyDraws;
}
static void DrawOotMaskShimmer(int mask) { assert(mask == 6); ++shimmerDraws; }
/* PRODUCTION_GERUDO_DRAW */

int main() {
    play.state.gfxCtx = &gfx;
    gfx.polyOpa.p = opa;
    // Both legacy fallback paths retain the same shimmer. A subsequent owner
    // load is re-queried; a cold first frame must not permanently hide the GI.
    DrawOotGerudoMask();
    assert(directDraws == 1 && legacyDraws == 1 && shimmerDraws == 1 && submitted.empty());
    directReady = true;
    DrawOotGerudoMask();
    assert(directDraws == 2 && legacyDraws == 1 && shimmerDraws == 2 && submitted.empty());
    ownerReady = true;
    DrawOotGerudoMask();
    assert(ownerQueries == 3 && directDraws == 2 && legacyDraws == 1 && shimmerDraws == 3);
    assert(submitted.size() == 1 && submitted.front() ==
           "__OTR__@oot:objects/object_gi_gerudo_warrior/gGiGerudoWarriorMaskDL");
    assert(gfx.polyOpa.p - opa == 2);
    ownerReady = false;
    DrawOotGerudoMask();
    assert(ownerQueries == 4 && directDraws == 3 && shimmerDraws == 4);
    puts("PASS native MM Gerudo GI: actual owner routing, cold/warm reload, both fallbacks and retained shimmer");
}
