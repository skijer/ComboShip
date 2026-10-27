// Execute the production fallback drawers and inspect real GBI packets.
// Resource lookup and CPU transforms are seams; setup/color packets are real.
#include <libultraship/libultra/gbi.h>
// Black Market reporting is an observer outside this rendering fixture.
void NeiGi_ReportBlackMarketModel(const char*, const char*, const char*) {}
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <set>
#include <string>
#include "combo/menu/ComboRewardGi.h"
using u8 = uint8_t;
using s16 = int16_t;
using s32 = int32_t;
using f32 = float;
Gfx packets[1024];
struct GraphicsContext { struct { Gfx* p = packets; } polyOpa; } gfx;
struct PlayState { struct { GraphicsContext* gfxCtx = &gfx; int frames = 9; } state; int gameplayFrames = 9; } play;
struct GetItemEntry {};
PlayState* gPlayState = &play;
Mtx matrix{};
constexpr int MTXMODE_APPLY = 1;
#define OPEN_DISPS(ctx) { auto* __gfxCtx = (ctx);
#define CLOSE_DISPS(ctx) }
#define POLY_OPA_DISP __gfxCtx->polyOpa.p
#define gSPDisplayList(pkt, path) gDma1p(pkt, G_DL_OTR_FILEPATH, path, 0, G_DL_PUSH)
#define MATRIX_FINALIZE_AND_LOAD(pkt, ctx) gSPMatrix(pkt, &matrix, G_MTX_LOAD | G_MTX_MODELVIEW)
void Matrix_Push() {}
void Matrix_Pop() {}
void Matrix_Scale(float, float, float, int) {}
void Matrix_RotateY(float, int) {}
void Matrix_RotateYF(float, int) {}
void Matrix_Translate(float, float, float, int) {}
Mtx* Matrix_NewMtx(GraphicsContext*, const char*, int) { return &matrix; }
/* NATIVE_SETUP */
void Gfx_SetupDL25_Opa(GraphicsContext* ctx) { __gSPDisplayList(ctx->polyOpa.p++, setup25); }
void Gfx_SetupDL_25Opa(GraphicsContext* ctx) { Gfx_SetupDL25_Opa(ctx); }
void Gfx_SetupDL_26Opa(GraphicsContext* ctx) { Gfx_SetupDL25_Opa(ctx); }
void Gfx_SetupDL26_Opa(GraphicsContext* ctx) { Gfx_SetupDL25_Opa(ctx); }
int ResourceMgr_IsModAsset(const char*) { return 0; }
int NeiResource_Available(const char*) { return 1; }
const char* NeiResource_Route(const char* path) {
    static std::set<std::string> paths;
    return paths.insert(std::string("__OTR__@oot:") + (path + 7)).first->c_str();
}
void* OotAssets_LoadGfx(const char* path) { return const_cast<char*>(path); }
void* OotAssets_LoadGfxDirect(const char* path) { return OotAssets_LoadGfx(path); }
Gfx* ResourceMgr_LoadGfxByName(const char* path) { return reinterpret_cast<Gfx*>(const_cast<char*>(path)); }
const char* gGiTunicCollarDL = "__OTR__objects/object_gi_clothes/gGiTunicCollarDL";
const char* gGiTunicDL = "__OTR__objects/object_gi_clothes/gGiTunicDL";
const char* gGiForestMedallionFaceDL = "__OTR__forest";
const char* gGiFireMedallionFaceDL = "__OTR__fire";
const char* gGiWaterMedallionFaceDL = "__OTR__water";
const char* gGiSpiritMedallionFaceDL = "__OTR__spirit";
const char* gGiShadowMedallionFaceDL = "__OTR__shadow";
const char* gGiLightMedallionFaceDL = "__OTR__light";
const char* gGiMedallionDL = "__OTR__medallion";
void NeiGi_DrawRewardMaterial(PlayState*, int, const char*, const char*, const char*) {
    assert(false && "Sage's Tunic miniature medallions must keep the native material");
}
/* PRODUCTION_DRAWERS */

struct Material {
    uint32_t prim, env, tint = 0;
    bool grayscale = false;
    int passes = 0;
    bool requireNeutral = true;
};
void Inspect(const Gfx* begin, const Gfx* end, Material& state) {
    for (auto* cmd = begin; cmd != end; ++cmd) {
        const auto op = cmd->words.w0 >> 24;
        if (op == G_ENDDL) return;
        if (op == G_SETPRIMCOLOR) state.prim = cmd->words.w1;
        if (op == G_SETENVCOLOR) state.env = cmd->words.w1;
        if (op == G_SETINTENSITY) state.tint = cmd->words.w1;
        if (op == G_SETGRAYSCALE) state.grayscale = cmd->words.w1 != 0;
        if (op == G_DL) Inspect(reinterpret_cast<const Gfx*>(cmd->words.w1), setup25 + std::size(setup25), state);
        if (op != G_DL_OTR_FILEPATH) continue;
        const char* path = reinterpret_cast<const char*>(cmd->words.w1);
        if (!std::strstr(path, "gGiTunic")) continue;
        if (state.requireNeutral) {
            assert(state.prim == 0xffffffffu && "tunic geometry inherited black or transparent primitive material");
            assert(state.env == 0x505050ffu && "tunic geometry inherited unrelated environment material");
        }
        assert(state.grayscale && "tunic identity tint must remain active for both passes");
        ++state.passes;
    }
}
int main() {
    for (uint32_t stale : {0x00000000u, 0xff000000u, 0x01234567u}) {
        for (int item = 0; item < 3; ++item) {
            gfx.polyOpa.p = packets;
            Material state{stale, stale};
#ifdef TEST_OOT_TUNIC
            GetItemEntry entry;
            if (item == 0) Randomizer_DrawExtSpiritBreastplate(&play, &entry);
            if (item == 1) Randomizer_DrawExtChampionsTunic(&play, &entry);
            if (item == 2) Randomizer_DrawExtSagesTunic(&play, &entry);
#else
            if (item == 0) DrawOotExtSpiritBreastplate();
            if (item == 1) DrawOotExtChampionsTunic();
            if (item == 2) DrawOotExtSagesTunic();
#endif
            Inspect(packets, gfx.polyOpa.p, state);
            const uint32_t expectedTint[] = {0xeb6e14ffu, 0x0078d7ffu,
#ifdef TEST_OOT_TUNIC
                                           0xf0f4faffu};
#else
                                           0xebf0f5ffu};
#endif
            assert(state.passes == 2 && !state.grayscale && state.tint == expectedTint[item]);
        }
    }
    // Other users of the generic tint helpers retain their material policy.
    gfx.polyOpa.p = packets;
    Material unchanged{0x12345678u, 0x98765432u, 0, false, 0, false};
#ifdef TEST_OOT_TUNIC
    DrawCustomItemDiamondTint(&play, (Gfx*)gGiTunicCollarDL, (Gfx*)gGiTunicDL, -1.f, 180, 40, 120);
#else
    DrawOotTunicTint(180, 40, 120);
#endif
    Inspect(packets, gfx.polyOpa.p, unchanged);
    assert(unchanged.prim == 0x12345678u && unchanged.env == 0x98765432u && unchanged.passes == 2);
    std::cout << "PASS actual tunic material packets: all three fallbacks remain opaque and tinted after stale black/zero-alpha draws\n";
}
