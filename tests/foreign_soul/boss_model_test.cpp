#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
extern "C" {
#include "ultra64.h"
// Both libultra and MM expose legacy controller names. The engine's definitions win.
#undef BTN_A
#undef BTN_B
#undef BTN_Z
#undef BTN_START
#undef BTN_DUP
#undef BTN_DDOWN
#undef BTN_DLEFT
#undef BTN_DRIGHT
#undef BTN_L
#undef BTN_R
#undef BTN_CUP
#undef BTN_CDOWN
#undef BTN_CLEFT
#undef BTN_CRIGHT
#include "global.h"
#include "objects/gameplay_keep/gameplay_keep.h"
#include "objects/object_boss_hakugin/object_boss_hakugin.h"
#include "objects/object_boss03/object_boss03.h"
#include "objects/object_boss01/object_boss01.h"
#include "objects/object_boss02/object_boss02.h"
}
#include "ComboItemDrawABI.h"
#include "Rando/Types.h"
#include "../soh/assets/objects/object_mo/object_mo.h"
// Only the owner's asset path constants are needed; no owner engine state crosses.
static const char* gGiBlueFireFlameDL = "__OTR__objects/object_gi_fire/gGiBlueFireFlameDL";
static const char* gBossSoulSkullDL = "__OTR__objects/object_boss_soul/gGIBossSoulSkullDL";
#define RANDO_ENUM_BEGIN(name) enum name {
#define RANDO_ENUM_ITEM(name) name,
#define RANDO_ENUM_END(name) };
#include "../soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerGet.h"
#define CVAR_RANDOMIZER_ENHANCEMENT(x) x
static bool simpler;
static int CVarGetInteger(const char*, int) { return simpler; }
static Gfx opa[256], xlu[256], scrolls[4][16], empty[8];
static Mtx matrix;
static GraphicsContext gfx;
static PlayState play;
PlayState* gPlayState = &play;
static int depth, scrollCount;
static std::vector<std::string> draws, owners;
static std::vector<float> rotations;
extern "C" {
void gSPDisplayList(Gfx* p, Gfx* dl) { draws.emplace_back((const char*)dl); __gSPDisplayList(p, dl); }
void gSPSegment(void* p, int s, uintptr_t t) { __gSPSegment(p, s, t); }
void Matrix_Push() { depth++; }
void Matrix_Pop() { assert(depth > 0); depth--; }
void Matrix_Translate(f32, f32, f32, MatrixMode) {}
void Matrix_Scale(f32, f32, f32, MatrixMode) {}
void Matrix_RotateXF(f32 a, MatrixMode) { rotations.push_back(a); }
void Matrix_RotateZF(f32 a, MatrixMode) { rotations.push_back(a); }
void Matrix_ReplaceRotation(MtxF*) {}
Mtx* Matrix_Finalize(GraphicsContext*) { return &matrix; }
void Gfx_SetupDL25_Xlu(GraphicsContext*) {}
void Graph_OpenDisps(Gfx**, Gfx*, GraphicsContext*, const char*, s32) {}
void Graph_CloseDisps(Gfx**, Gfx*, GraphicsContext*, const char*, s32) {}
void* Graph_Alloc(GraphicsContext*, size_t n) { assert(n == sizeof(empty)); return empty; }
f32 Rand_ZeroOne() { return 0; }
void FrameInterpolation_RecordOpenChild(const void*, int) {}
void FrameInterpolation_RecordCloseChild() {}
Gfx* Gfx_TwoTexScrollEx(GraphicsContext*, s32, u32 x1, u32 y1, s32 w1, s32 h1,
                        s32, u32 x2, u32 y2, s32 w2, s32 h2, s32 xs1, s32 ys1, s32 xs2, s32 ys2) {
    assert(scrollCount < 4);
    if (scrollCount == 0) {
        assert(w1 == 16 && h1 == 32 && w2 == 16 && h2 == 32 && xs2 == 2 && ys2 == -6);
    } else if (scrollCount == 1) {
        assert(x1 == play.state.frames * 3 && y1 == x1 && x2 == (u32)(play.state.frames * -3) && y2 == x2);
        assert(xs1 == 3 && ys1 == 3 && xs2 == -3 && ys2 == -3);
    } else if (scrollCount == 2) {
        assert(x1 == play.state.frames * 3 && y1 == 0 && x2 == 0 && y2 == (u32)(play.state.frames * -5));
        assert(xs1 == 3 && ys1 == 0 && xs2 == 0 && ys2 == -5);
    }
    return scrolls[scrollCount++];
}
}
/* PRODUCTION_RECIPE */
/* PRODUCTION_NATIVE_FLAME */
struct ComboForeignDrawInfoOOT {
    int count;
    const char* dls[CW_DRAW_MAX_DLISTS];
    uint8_t primColorXlu[4], envColorXlu[4];
};
#define MM_FOREIGN_PIN_XLU() gSPComboRMPush(POLY_XLU_DISP++, "oot")
static void MM_RestoreForeignSegs(const int32_t* segs, int count) {
    for (int i = 0; i < count; ++i) gSPSegment(gfx.polyXlu.p++, segs[i], (uintptr_t)empty);
}
// This adjacent core/flame regression deliberately takes the helper's
// unavailable-resource fallback. The dedicated Morpha fixture executes the
// real helper together with both actual integration bodies.
static int tentacleRequests, tentacleAvailabilityRequests;
static int NeiResource_Available(const char* path) {
    assert(!strcmp(path, gMorphaTentacleBaseDL));
    ++tentacleAvailabilityRequests;
    return 0;
}
static bool ComboDrawMorphaTentacleGi(PlayState* host, const char* owner,
                                     bool (*available)(const char*, const char*)) {
    assert(host == &play && !strcmp(owner, "oot"));
    ++tentacleRequests;
    assert(!available(gMorphaTentacleBaseDL, owner));
    return false;
}
/* PRODUCTION_DRAW */
static int32_t MM_FillEnemySoulAnim(RandoItemId, CwItemAnimDrawInfo*) { return 0; }
static int32_t MM_FillMinifrogAnim(RandoItemId, CwItemAnimDrawInfo*) { return 0; }
/* PRODUCTION_MM_RECIPE */
int main() {
    CwItemDrawInfo recipe{};
    assert(BossRecipe(RG_MORPHA_SOUL, &recipe) == 1);
    assert(recipe.dlistCount == 3); // RED: baseline selects the generic skull instead.
    assert(recipe.drawKind == CW_DRAW_KIND_OOT_MORPHA_SOUL && CwMinDlistsForKind(recipe.drawKind) == 3);
    assert(!strcmp(recipe.dlists[1], gMorphaCoreMembraneDL));
    assert(!strcmp(recipe.dlists[2], gMorphaCoreNucleusDL));
    assert(recipe.primColorXlu[0] == 85 && recipe.primColorXlu[1] == 180 && recipe.primColorXlu[2] == 223);
    assert(ExportedDependency(RG_NONE) == 0);
    assert(ExportedDependency(RG_MORPHA_SOUL) == 2); // Cosmetic selection remains live after a grant latch.
    for (int frame : {0, 7, 600}) {
        gfx.polyOpa.p = opa; gfx.polyOpa.d = empty + 8; gfx.polyXlu.p = xlu;
        play.state.gfxCtx = &gfx; play.state.frames = frame;
        draws.clear(); rotations.clear(); scrollCount = depth = 0;
        tentacleRequests = tentacleAvailabilityRequests = 0;
        std::string membrane = "__OTR__@oot:" + std::string(recipe.dlists[1] + 7);
        std::string nucleus = "__OTR__@oot:" + std::string(recipe.dlists[2] + 7);
        ComboForeignDrawInfoOOT info{};
        info.count = 3; info.dls[1] = membrane.c_str(); info.dls[2] = nucleus.c_str();
        memcpy(info.primColorXlu, recipe.primColorXlu, 4);
        MM_DrawForeignMorphaSoul(&info);
        assert(tentacleRequests == 0 && tentacleAvailabilityRequests == 0);
        assert(draws.size() == 3 && draws[0] == gameplay_keep_DL_01ACF0);
        assert(draws[1] == membrane && draws[2] == nucleus);
        assert(scrollCount == 3 && depth == 0 && gfx.polyOpa.p == opa);
        assert(rotations.size() == 2 && fabs(rotations[0] - frame * .1f) < .0001f && fabs(rotations[1] - frame * .16f) < .0001f);
        int restored8 = 0, restored9 = 0, modelPrims = 0, modelEnvs = 0;
        for (Gfx* p = xlu; p < gfx.polyXlu.p; ++p) {
            if (p->words.w0 >> 24 == G_MOVEWORD && p->words.w1 == (uintptr_t)empty) {
                restored8 += (p->words.w0 & 0xffff) == 32;
                restored9 += (p->words.w0 & 0xffff) == 36;
            }
            if (p->words.w0 >> 24 == G_SETPRIMCOLOR && (p->words.w0 & 0xffff) == 0x8080 && p->words.w1 == 0xffffffffU) modelPrims++;
            if (p->words.w0 >> 24 == G_SETENVCOLOR && p->words.w1 == 0x00dcff80U) modelEnvs++;
        }
        assert(restored8 >= 2 && restored9 == 1 && modelPrims == 2 && modelEnvs == 1);
    }
    simpler = true; recipe = {};
    assert(BossRecipe(RG_MORPHA_SOUL, &recipe) == 1 && recipe.dlistCount == 2);
    assert(!strcmp(recipe.dlists[1], gBossSoulSkullDL));
    for (int slot = 0; slot < 9; ++slot) {
        assert(!OOT_BossSoulUsesSkeleton((RandomizerGet)(RG_GOHMA_SOUL + slot)));
    }
    simpler = false;
    for (int slot = 0; slot < 9; ++slot) {
        assert(OOT_BossSoulUsesSkeleton((RandomizerGet)(RG_GOHMA_SOUL + slot)) == (slot != 5));
    }
    for (auto id : {RI_SOUL_BOSS_GOHT, RI_SOUL_BOSS_GYORG, RI_SOUL_BOSS_ODOLWA, RI_SOUL_BOSS_TWINMOLD}) {
        CwItemAnimDrawInfo anim{};
        assert(MM_FillAnimDrawInfo(id, &anim) == 1);
        assert(MM_HasAnimDraw(id));
        assert(anim.opa && anim.skelPath && anim.animPath && anim.flameAfter && anim.flameBillboardFirst);
        assert(!strcmp(anim.flameDlPath, gameplay_keep_DL_01ACF0));
        assert(anim.translatePre[1] == (id == RI_SOUL_BOSS_TWINMOLD ? 0.0f : -20.0f) && anim.hiddenLimb == -1);
        if (id == RI_SOUL_BOSS_GOHT) {
            assert(!strcmp(anim.skelPath, gGohtSkel) && !strcmp(anim.animPath, gGohtRunAnim));
            assert(anim.limbCount == GOHT_LIMB_MAX && anim.scale == .005f);
            assert(anim.segCount == 1 && anim.segs[0].segment == 8);
            assert(!strcmp(anim.segs[0].path, gGohtMetalPlateWithCirclePatternTex));
            assert(anim.flameColor[0] == 10 && anim.flameColor[1] == 138 && anim.flameColor[2] == 46);
            assert(anim.flameScale[0] == 30);
        } else if (id == RI_SOUL_BOSS_GYORG) {
            assert(!strcmp(anim.skelPath, gGyorgSkel) && !strcmp(anim.animPath, gGyorgGentleSwimmingAnim));
            assert(anim.limbCount == GYORG_LIMB_MAX && anim.scale == .05f && anim.segCount == 0);
            assert(anim.flameColor[0] == 19 && anim.flameColor[1] == 99 && anim.flameColor[2] == 165);
            assert(anim.flameScale[0] == 3);
        } else if (id == RI_SOUL_BOSS_ODOLWA) {
            assert(!strcmp(anim.skelPath, gOdolwaSkel) && !strcmp(anim.animPath, gOdolwaReadyAnim));
            assert(anim.limbCount == ODOLWA_LIMB_MAX && anim.scale == .005f && anim.segCount == 0);
            assert(anim.flameColor[0] == 145 && anim.flameColor[1] == 20 && anim.flameColor[2] == 133);
            assert(anim.flameScale[0] == 25);
        } else {
            assert(!strcmp(anim.skelPath, gTwinmoldHeadSkel) && !strcmp(anim.animPath, gTwinmoldHeadFlyAnim));
            assert(anim.limbCount == TWINMOLD_HEAD_LIMB_MAX && anim.scale == .06f);
            assert(anim.segCount == 1 && !strcmp(anim.segs[0].path, gTwinmoldBlueSkinTex));
            assert(anim.flameColor[0] == 168 && anim.flameColor[1] == 180 && anim.flameColor[2] == 20);
            assert(anim.flameScale[0] == 3);
        }
    }
    assert(MM_HasAnimDraw(RI_SOUL_BOSS_TWINMOLD));
    puts("PASS production Morpha recipe/consumer: owner paths, native MM flame, model scroll/rotations/colors and segment cleanup; native MM boss skeleton exports");
}
