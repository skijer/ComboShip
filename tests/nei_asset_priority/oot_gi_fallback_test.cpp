#include <array>
#include <cassert>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "z64.h"

std::set<std::string> localMods, mmMods, altMods;
bool altEnabled = false, present = true;
int loads = 0, draws = 0;
Gfx native[2] = { gsDPSetPrimColor(0, 0, 10, 20, 30, 255), gsSPEndDisplayList() };
Gfx* drawn = nullptr;
const char* gGiHoverBootsDL = "__OTR__objects/boots";
int ResourceMgr_IsModAsset(const char* p) { return localMods.contains(p) || (altEnabled && altMods.contains(p)); }
int ResourceMgr_IsModAssetForGame(const char* game, const char* p) { assert(std::string(game)=="mm"); return mmMods.contains(p); }
bool ResourceMgr_FileExists(const char*) { return present; }
Gfx* ResourceMgr_LoadGfxByName(const char*) { ++loads; return native; }
bool MmAssets_IsAvailable() { return true; }
void* MmAssets_LoadResource(const char*) { ++loads; return native; }
void DrawCustomItemDiamond(PlayState*, Gfx* dl, f32) { ++draws; drawn = dl; }
uint32_t Pegasus_CrimsonRamp(uint32_t color) { return color ^ 0xFF000000u; }
#define SPDLOG_ERROR(...) ((void)0)
void NeiGi_ReportBlackMarketModel(const char*,const char*,const char* = nullptr) {}
#include "oot_gi_fallback.inc"
std::string pathOf(Gfx* dl) {
    assert(dl && (dl[0].words.w0 >> 24) == G_DL_OTR_FILEPATH);
    return reinterpret_cast<const char*>(dl[0].words.w1);
}
int main() {
    const char* mm = "objects/mm_sword";
    Gfx* cached = nullptr; u8 tried = 0;
    assert(LoadMmDLOnce(mm, &cached, &tried) == native);
    assert(loads == 1);
    mmMods.insert(mm);
    assert(pathOf(LoadMmDLOnce(mm, &cached, &tried)) == "__OTR__@mm:objects/mm_sword");
    localMods.insert(mm);
    assert(pathOf(LoadMmDLOnce(mm, &cached, &tried)) == "__OTR__objects/mm_sword");
    assert(loads == 1); // Deferred roots do not touch/retain loaded pointers.
    localMods.clear(); mmMods.clear();
    assert(LoadMmDLOnce(mm, &cached, &tried) == native);

    const char* slate = "__OTR__objects/slate";
    cached = nullptr; tried = 0; present = false;
    DrawCustomItemDiamondByPath(nullptr, slate, &cached, &tried, 1);
    assert(draws == 0);
    altMods.insert(slate); altEnabled = true;
    DrawCustomItemDiamondByPath(nullptr, slate, &cached, &tried, 1);
    assert(draws == 1 && pathOf(drawn) == slate); // Recover after previous miss.
    altEnabled = false;
    assert(NeiGi_ModOverrideDL(slate, false) == nullptr);

    present = true;
    Gfx* recolored = Pegasus_GetRecoloredBootsDL();
    assert(recolored != native);
    localMods.insert(gGiHoverBootsDL);
    assert(pathOf(Pegasus_GetRecoloredBootsDL()) == gGiHoverBootsDL);
    std::vector<Gfx> copy;
    auto remap = [](uint32_t c) -> uint32_t { return c ^ 0xFF000000u; };
    localMods.clear();
    assert(BuildRecoloredGiDL(gGiHoverBootsDL, remap, copy) == copy.data());
    localMods.insert(gGiHoverBootsDL);
    assert(pathOf(BuildRecoloredGiDL(gGiHoverBootsDL, remap, copy)) == gGiHoverBootsDL);
    localMods.clear();
    assert(Pegasus_GetRecoloredBootsDL() == recolored);
}
