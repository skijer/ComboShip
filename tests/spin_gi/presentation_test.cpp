#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_set>
extern "C" {
void NeiGi_ReportBlackMarketModel(const char*,const char*,const char*) {}
#ifdef HOST_MM
#include "global.h"
#else
#include "z64.h"
#include "macros.h"
#include "functions.h"
#endif
#include "objects/gameplay_keep/gameplay_keep.h"
#ifndef HOST_MM
// The donor recipe still names MM resources when compiled against the OoT host.
static const char* gGreatSpinAttackDiskDL = "__OTR__objects/gameplay_keep/gGreatSpinAttackDiskDL";
static const char* gGreatSpinAttackCylinderDL = "__OTR__objects/gameplay_keep/gGreatSpinAttackCylinderDL";
#endif
Color_RGBA8 CosmeticEditor_GetChangedColor(u8, u8, u8, u8, const char*);
}
#include "ComboItemDrawABI.h"

static Gfx opa[256], xlu[256], scroll[16];
static GraphicsContext gfx;
static PlayState play;
static Mtx matrix;
static int depth, queries, draws;
static bool edited;
static Color_RGBA8 changed{17, 123, 241, 255};
static std::string paths[2];
static int scrollX1, scrollX2;

extern "C" {
Color_RGBA8 CosmeticEditor_GetChangedColor(u8 r, u8 g, u8 b, u8 a, const char* id) {
    assert(!strcmp(id, "Effects.GreatSpinBurst"));
    queries++;
    return edited ? changed : Color_RGBA8{r, g, b, a};
}
void gSPDisplayList(Gfx* p, Gfx* dl) {
    assert(draws < 2);
    paths[draws++] = (const char*)dl;
    __gSPDisplayList(p, dl);
}
void gSPSegment(void* p, int segment, uintptr_t target) { __gSPSegment(p, segment, target); }
void Matrix_Push() { depth++; }
void Matrix_Pop() { assert(depth > 0); depth--; }
#ifdef HOST_MM
void Matrix_Scale(f32 x, f32 y, f32 z, MatrixMode) { assert(x == .012f && x == y && y == z); }
void Matrix_RotateZYX(s16 x, s16 y, s16 z, MatrixMode) { assert(x == 0x1800 && z == 0); }
Mtx* Matrix_Finalize(GraphicsContext*) { return &matrix; }
void Gfx_SetupDL25_Xlu(GraphicsContext*) {}
void Graph_OpenDisps(Gfx**, Gfx*, GraphicsContext*, const char*, s32) {}
void Graph_CloseDisps(Gfx**, Gfx*, GraphicsContext*, const char*, s32) {}
#else
void Matrix_Scale(f32 x, f32 y, f32 z, u8) { assert(x == .012f && x == y && y == z); }
void Matrix_RotateZYX(s16 x, s16 y, s16 z, u8) { assert(x == 0x1800 && z == 0); }
Mtx* Matrix_NewMtx(GraphicsContext*, char*, s32) { return &matrix; }
void Gfx_SetupDL_25Xlu(GraphicsContext*) {}
void Graph_OpenDisps(Gfx**, GraphicsContext*, const char*, s32) {}
void Graph_CloseDisps(Gfx**, GraphicsContext*, const char*, s32) {}
#endif
void FrameInterpolation_RecordOpenChild(const void*, int) {}
void FrameInterpolation_RecordCloseChild() {}
Gfx* Gfx_TwoTexScrollEx(GraphicsContext*, s32, u32 x1, u32, s32 w1, s32 h1,
                        s32, u32 x2, u32, s32 w2, s32 h2, s32 xs1, s32 ys1, s32 xs2, s32 ys2) {
    assert(w1 == 64 && h1 == 32 && w2 == 8 && h2 == 8);
    assert(xs1 == -30 && xs2 == -20 && ys1 == 0 && ys2 == 0);
    scrollX1 = x1; scrollX2 = x2;
    return scroll;
}
}
/* PRODUCTION_RECIPE */
#ifdef HOST_MM
#define COMBO_SPIN_GI_HOST_MM
#endif
#include "ComboSpinAttackGi.h"

#ifndef HOST_MM
static bool donorReady;
static bool donorOwnerReady = true;
static int donorResult = 1, invalidDonor, fallbackDraws, donorQueries;
namespace Ship {
struct CrossRMRegistry {
    static bool Get(const char* game) {
        assert(!strcmp(game, "mm"));
        return donorOwnerReady;
    }
};
}
int32_t SpinDonor(const char* name, CwItemDrawInfo* out) {
    assert(!strcmp(name, "Great Spin Attack"));
    if (donorResult != 1) return donorResult;
    MM_FillSpinAttackGi(out);
    if (invalidDonor == 1) out->drawKind = CW_DRAW_KIND_SIMPLE;
    if (invalidDonor == 2) out->dlistCount = 1;
    if (invalidDonor == 3) out->dlists[0] = nullptr;
    if (invalidDonor == 4) out->dlists[1] = "bad path";
    return 1;
}
void* Combo_ResolveSym(const char* game, const char* symbol) {
    assert(!strcmp(game, "2ship") && !strcmp(symbol, "MM_GetItemDrawInfo"));
    ++donorQueries;
    return donorReady ? (void*)SpinDonor : nullptr;
}
const char* ComboInternRoutedPath(const std::string& path) {
    static std::unordered_set<std::string> paths;
    return paths.insert(path).first->c_str();
}
extern "C" void GetItem_Draw(PlayState*, s16 gid) { assert(gid == GID_SWORD_KOKIRI); fallbackDraws++; }
/* PRODUCTION_OOT_ALIAS */
#endif

int main() {
    for (bool native : {true, false}) {
        for (bool hex : {false, true}) {
            for (int frame : {0, 7, 600}) {
                std::memset(&gfx, 0, sizeof(gfx));
                gfx.polyOpa.p = opa; gfx.polyXlu.p = xlu;
                play.state.gfxCtx = &gfx; play.gameplayFrames = frame;
                draws = queries = 0; edited = hex;
                CwItemDrawInfo recipe{};
                assert(MM_FillSpinAttackGi(&recipe) == 1);
                assert(recipe.drawKind == CW_DRAW_KIND_MM_SPIN_ATTACK && recipe.stateDependent == 2);
                assert(recipe.dlistCount == 2 && CwMinDlistsForKind(recipe.drawKind) == 2);
                assert(std::string(recipe.dlists[0]).ends_with("gGreatSpinAttackDiskDL"));
                assert(std::string(recipe.dlists[1]).ends_with("gGreatSpinAttackCylinderDL"));
                const std::string disk = std::string("__OTR__@mm:") + (recipe.dlists[0] + 7);
                const std::string cylinder = std::string("__OTR__@mm:") + (recipe.dlists[1] + 7);
                ComboDrawSpinAttackGi(&play, native ? recipe.dlists[0] : disk.c_str(),
                                     native ? recipe.dlists[1] : cylinder.c_str(), recipe.scale,
                                     recipe.primColorXlu, native ? nullptr : "mm");
                assert(draws == 2 && queries == 1 && depth == 0 && gfx.polyOpa.p == opa);
                assert(scrollX1 == 255 - (frame * 30 & 255) && scrollX2 == 255 - (frame * 20 & 255));
                int prims = 0, bindings = 0, scopes = 0, ends = 0, grayOff = 0;
                for (Gfx* p = xlu; p < gfx.polyXlu.p; ++p) {
                    unsigned op = p->words.w0 >> 24;
                    if (op == G_SETPRIMCOLOR) {
                        assert(p->words.w1 == (hex ? 0x117bf1ffU : 0xffffaaffU));
                        prims++;
                    }
                    if (op == G_MOVEWORD && (p->words.w0 & 0xffff) == 32) {
                        bindings++;
                        if (bindings == 1) assert(p->words.w1 == (uintptr_t)scroll);
                        else {
                            const auto* empty = (const Gfx*)p->words.w1;
                            for (int i = 0; i < 8; ++i) assert(empty[i].words.w0 >> 24 == G_ENDDL);
                        }
                    }
                    if (op == G_COMBO_RM_PUSH) { assert(!strcmp((const char*)p->words.w1, "mm")); scopes++; }
                    if (op == G_COMBO_RM_POP) ends++;
                    if (op == G_SETGRAYSCALE) { assert(p->words.w1 == 0); grayOff++; }
                }
                assert(prims == 1 && bindings == 2 && scopes == !native && ends == scopes && grayOff == 1);
                assert(paths[0] == (native ? recipe.dlists[0] : disk));
                assert(paths[1] == (native ? recipe.dlists[1] : cylinder));
            }
        }
    }
#ifndef HOST_MM
    CwItemDrawInfo alias{};
    assert(OOT_DescribeMmSpinAttackDraw(nullptr) == 0);
    assert(OOT_DescribeMmSpinAttackDraw(&alias) == CW_DRAW_NOT_READY);
    Randomizer_DrawMmGreatSpinAttack(&play, nullptr);
    assert(fallbackDraws == 1);
    donorReady = true;
    for (int bad : {1, 2, 3, 4}) {
        invalidDonor = bad;
        assert(OOT_DescribeMmSpinAttackDraw(&alias) == 0);
    }
    invalidDonor = 0; donorResult = CW_DRAW_NOT_READY;
    assert(OOT_DescribeMmSpinAttackDraw(&alias) == CW_DRAW_NOT_READY);
    donorResult = 1; draws = queries = 0;
    gfx.polyOpa.p = opa; gfx.polyXlu.p = xlu;
    Randomizer_DrawMmGreatSpinAttack(&play, nullptr);
    assert(fallbackDraws == 1 && draws == 2 && queries == 1 && depth == 0 && gfx.polyOpa.p == opa);
    assert(paths[0].starts_with("__OTR__@mm:") && paths[1].starts_with("__OTR__@mm:"));
    donorOwnerReady = false;
    const int queriesBefore = donorQueries;
    Randomizer_DrawMmGreatSpinAttack(&play, nullptr);
    assert(fallbackDraws == 2 && draws == 2 && donorQueries == queriesBefore);
#endif
    puts("PASS real-header spin GI: native and @mm resources, live burst hex, animated scroll, opaque alpha, XLU-only and matrix/segment/RM cleanup");
}
