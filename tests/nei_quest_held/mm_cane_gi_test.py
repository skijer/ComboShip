"""Exercise the real MM cane GI callback with separate local/donor resources.

The selected donor legacy root must survive the authored-GI decline. Recording
and replaying owner commands also catches a raw MM root whose children inherit
another game's draw context. Resources/GPU submission are the only doubles.
"""
from pathlib import Path
import os
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from, functions

draw_source = (ROOT / "mm/2s2h/Rando/DrawItem.cpp").read_text()
source = functions(draw_source)
native = block_from(draw_source, draw_source.index("void Rando::DrawResolvedItem("))
native_start = native.index("        case RI_OOT_NEI_CANE_OF_SOMARIA:")
native_end = native.index("        case RI_OOT_NEI_DEKU_LEAF:", native_start)
dispatch = "void NativeCaneDispatch(RandoItemId randoItemId) { switch (randoItemId) {\n"
dispatch += native[native_start:native_end] + "default: assert(false); }}\n"
foreign = functions((ROOT / "combo/menu/ComboForeignDrawMM.h").read_text())["MM_DrawComboForeign"]
foreign_start = foreign.index("    if (info->drawKind == CW_DRAW_KIND_NEI_CANE) {")
foreign_end = foreign.index("    if (info->drawKind == CW_DRAW_KIND_NEI_GI)", foreign_start)
dispatch += "void ForeignCaneDispatch(const CwItemDrawInfo* info) {\n"
dispatch += foreign[foreign_start:foreign_end] + "assert(false); }\n"
preamble = r'''
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
void NeiGi_ReportBlackMarketModel(const char*,const char*,const char* = nullptr) {}
#include <string>
#include <vector>
#include "combo/menu/ComboItemDrawABI.h"
#include "mm/2s2h/Rando/Types.h"
using u8 = uint8_t;
constexpr int MTXMODE_APPLY = 1;
struct Gfx { int identity; };
struct PlayState { struct { void* gfxCtx; } state; };
PlayState play{};
PlayState* gPlayState = &play;
const char* cane = "__OTR__objects/object_somaria/g_somaria_cane_give_dl";
Gfx mmBase{10}, mmMod{11};
bool mmPresent = true, mmAlt = false, mmOverride = false, mmBaseOverride = false;
bool donorPresent = true, donorAlt = false, donorOverride = false, donorBaseOverride = false;
int fallback = 0, localLoads = 0;
int dedicatedUltrahand = 0, matrixDepth = 0, shimmers = 0;
std::vector<std::array<u8, 3>> flames;
std::vector<bool> grayscale;
std::vector<std::string> ownerStack;
struct Command { const char* owner; Gfx* root; bool pop; };
std::vector<Command> commands;
#define OPEN_DISPS(ctx)
#define CLOSE_DISPS(ctx)
#define POLY_OPA_DISP ((Gfx*)nullptr)
#define MATRIX_FINALIZE_AND_LOAD(pkt,ctx)
#define gSPDisplayList(pkt,dl) commands.push_back({nullptr,dl,false})
#define gSPComboRMPush(pkt,owner) commands.push_back({owner,nullptr,false})
#define gSPComboRMPop(pkt) commands.push_back({nullptr,nullptr,true})
void Gfx_SetupDL25_Opa(void*) {}
void Matrix_Scale(float x,float y,float z,int) { assert(x == .25f && y == .25f && z == .25f); }
void SetGrayscaleColor(u8 r,u8 g,u8 b,u8 a) { assert(r == 255 && g == 215 && b == 70 && a == 255); }
#define gDPSetGrayscaleColor(pkt,r,g,b,a) SetGrayscaleColor(r,g,b,a)
#define gSPGrayscale(pkt,enabled) grayscale.push_back(enabled)
u8 OotAssets_PathAllowed(const char* path) { assert(!strcmp(path,cane)); return 1; }
u8 ResourceMgr_FileExists(const char* path) { assert(!strcmp(path,cane)); return mmPresent || mmBaseOverride; }
u8 ResourceMgr_FileAltExists(const char* path) { assert(!strcmp(path,cane)); return mmOverride; }
bool ResourceMgr_IsAltAssetsEnabled() { return mmAlt; }
int ResourceMgr_IsModAsset(const char* path) { assert(!strcmp(path,cane)); return mmBaseOverride || (mmAlt && mmOverride); }
Gfx* ResourceMgr_LoadGfxByName(const char* path) {
    assert(!strcmp(path,cane)); ++localLoads;
    return mmBaseOverride || (mmAlt && mmOverride) ? &mmMod : mmPresent ? &mmBase : nullptr;
}
int NeiResource_IsMod(const char* path) { assert(!strcmp(path,cane)); return donorBaseOverride || (donorAlt && donorOverride); }
int NeiResource_Available(const char* path) { assert(!strcmp(path,cane)); return donorPresent; }
const char* NeiResource_Route(const char* path) {
    assert(!strcmp(path,cane));
    return "__OTR__@oot:objects/object_somaria/g_somaria_cane_give_dl";
}
void DrawOotSlateRuneFlame(u8 r,u8 g,u8 b) { flames.push_back({r,g,b}); }
void DrawOotRodStandIn(Gfx**,u8,u8,u8) { ++fallback; }
void DrawOotNeiUltrahand() { ++dedicatedUltrahand; }
void Matrix_Push() { ++matrixDepth; }
void Matrix_Pop() { assert(matrixDepth > 0); --matrixDepth; }
constexpr int GID_RUPEE_BLUE = 1;
void GetItem_Draw(PlayState*,int) { assert(false && "valid cane must not fall through to the sentinel"); }
void ComboDrawMaskShimmer(PlayState*,const char*,const uint8_t*,const char* owner) {
    assert(!strcmp(owner,"oot")); ++shimmers;
}
struct Draw { int identity; std::string nestedOwner; };
std::vector<Draw> Replay(const char* inheritedOwner) {
    std::vector<Draw> result;
    ownerStack = {inheritedOwner};
    for (const auto& cmd : commands) {
        if (cmd.owner) ownerStack.push_back(cmd.owner);
        else if (cmd.pop) { assert(ownerStack.size() > 1); ownerStack.pop_back(); }
        else if (cmd.root == &mmBase || cmd.root == &mmMod)
            result.push_back({cmd.root->identity,ownerStack.back()});
        else {
            assert(!strcmp(reinterpret_cast<const char*>(cmd.root),
                "__OTR__@oot:objects/object_somaria/g_somaria_cane_give_dl"));
            assert(donorPresent && (donorBaseOverride || (donorAlt && donorOverride)));
            result.push_back({12,"oot"}); // Routed root and its nested graph share the owner.
        }
    }
    assert(ownerStack == std::vector<std::string>{inheritedOwner});
    return result;
}
void Reset() { commands.clear(); flames.clear(); grayscale.clear(); fallback = localLoads = 0; }
'''
checks = r'''
int main() {
    // The owner descriptor declines the authored GI for an OoT-only legacy mod.
    // The native MM callback must submit that selected root, including after a
    // previous frame already populated its MM stock cache.
    DrawOotNeiCaneOfSomaria(RI_OOT_NEI_CANE_OF_SOMARIA);
    donorAlt = donorOverride = true;
    Reset(); DrawOotNeiCaneOfSomaria(RI_OOT_NEI_CANE_OF_SOMARIA);
    auto shown = Replay("mm");
    assert(shown.size() == 1 && shown[0].identity == 12 && shown[0].nestedOwner == "oot" &&
           "selected OoT legacy cane was discarded in favor of MM stock");
    assert(localLoads == 0 && fallback == 0 && flames.empty() && grayscale.empty());

    // A selected native MM override wins over the donor, and its raw graph
    // must be scoped to MM even when an outer draw span belongs to OoT.
    mmAlt = mmOverride = true;
    Reset(); DrawOotNeiCaneOfSomaria(RI_OOT_NEI_CANE_OF_SOMARIA);
    shown = Replay("oot");
    assert(shown.size() == 1 && shown[0].identity == 11 && shown[0].nestedOwner == "mm");
    assert(localLoads == 1 && fallback == 0);
    mmAlt = false; mmBaseOverride = true;
    Reset(); DrawOotNeiCaneOfSomaria(RI_OOT_NEI_CANE_OF_SOMARIA);
    shown = Replay("oot");
    assert(shown.size() == 1 && shown[0].identity == 11 && shown[0].nestedOwner == "mm");
    mmBaseOverride = false;

    // Disabling the MM override reveals the selected donor, then disabling
    // the donor override restores MM stock without retaining either cache.
    mmAlt = false;
    Reset(); DrawOotNeiCaneOfSomaria(RI_OOT_NEI_CANE_OF_SOMARIA);
    shown = Replay("mm"); assert(shown.size() == 1 && shown[0].identity == 12);
    donorAlt = false;
    donorBaseOverride = true;
    Reset(); DrawOotNeiCaneOfSomaria(RI_OOT_NEI_CANE_OF_SOMARIA);
    shown = Replay("mm"); assert(shown.size() == 1 && shown[0].identity == 12);
    donorBaseOverride = false;
    Reset(); DrawOotNeiCaneOfSomaria(RI_OOT_NEI_CANE_OF_SOMARIA);
    shown = Replay("oot");
    assert(shown.size() == 1 && shown[0].identity == 10 && shown[0].nestedOwner == "mm");

    // Concrete skills never re-read ownership: red/gold tint and upgrade
    // flames remain tied to the granted skill while resource selection is live.
    const RandoItemId skills[] = {RI_OOT_NEI_CANE_OF_SOMARIA, RI_OOT_NEI_CANE_PACCI_FLIP,
        RI_OOT_NEI_CANE_SOMARIA_BLOCK, RI_OOT_NEI_CANE_PACCI_STONE,
        RI_OOT_NEI_CANE_SOMARIA_PLATFORM, RI_OOT_NEI_CANE_PACCI_ULTRAHAND};
    for (int i = 0; i < 6; ++i) {
        Reset(); DrawOotNeiCaneOfSomaria(skills[i]);
        shown = Replay("oot"); assert(shown.size() == 1 && shown[0].identity == 10);
        const bool pacci = i % 2;
        assert(grayscale == (pacci ? std::vector<bool>{true,false} : std::vector<bool>{}));
        const std::array<u8,3> hue = pacci ? std::array<u8,3>{255,215,70} : std::array<u8,3>{255,60,60};
        assert(flames == (i >= 2 ? std::vector<std::array<u8,3>>{hue} : std::vector<std::array<u8,3>>{}));
    }
    // Run the retained native switch and foreign cane dispatch statements too:
    // both must reach this callback without converting the concrete award.
    donorAlt = true;
    for (int i = 0; i < 6; ++i) {
        Reset(); NativeCaneDispatch(skills[i]);
        if (i == 5) assert(commands.empty() && dedicatedUltrahand == 1);
        else {
            shown = Replay("mm"); assert(shown.size() == 1 && shown[0].identity == 12);
            const auto nativeFlames = flames;
            const auto nativeGray = grayscale;
            CwItemDrawInfo info{};
            info.drawKind = CW_DRAW_KIND_NEI_CANE;
            info.neiLegacyCane = i + 1;
            info.itemShimmer = i % 2;
            Reset(); ForeignCaneDispatch(&info);
            shown = Replay("mm"); assert(shown.size() == 1 && shown[0].identity == 12);
            assert(flames == nativeFlames && grayscale == nativeGray && matrixDepth == 0);
        }
    }
    assert(shimmers == 2);
    CwItemDrawInfo ultrahand{};
    ultrahand.drawKind = CW_DRAW_KIND_NEI_CANE;
    ultrahand.neiLegacyCane = 6;
    Reset(); ForeignCaneDispatch(&ultrahand);
    assert(commands.empty() && dedicatedUltrahand == 2 && matrixDepth == 0);
    donorAlt = false;
    mmPresent = false;
    Reset(); DrawOotNeiCaneOfSomaria(RI_OOT_NEI_CANE_OF_SOMARIA);
    assert(commands.empty() && fallback == 1); // Retained documented missing-stock fallback.
    std::cout << "PASS MM cane GI: donor-only legacy override, native override priority, live Alt/cache changes, "
                 "stock/nested owner preservation, concrete skill tint/flame and native/foreign dispatch\n";
}
'''
with tempfile.TemporaryDirectory(prefix="mm-cane-gi-") as directory:
    path = Path(directory) / "test.cpp"
    path.write_text(preamble + source["LoadNeiRealGfx"] + "\n" + source["DrawOotNeiCaneOfSomaria"] + dispatch + checks)
    binary = Path(directory) / "test"
    flags = ["-std=c++20", "-DCOMBO_BUILD", "-Wall", "-Wextra", "-Werror", "-I" + str(ROOT)]
    if "--sanitize" in sys.argv:
        flags += ["-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-pie", "-no-pie"]
    subprocess.run([os.environ.get("CXX", "c++"), *flags, str(path), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
