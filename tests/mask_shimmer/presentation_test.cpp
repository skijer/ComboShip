#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <map>
#include <cstdlib>
#include <string>
#include <vector>
extern "C" {
int ResourceMgr_GetGiModelBoundsForGame(const char*,const char* const*,int,float[3],float[3]) { return 0; }
#ifdef HOST_MM
#include "global.h"
#else
#include "z64.h"
#include "macros.h"
#include "functions.h"
#include "soh_assets.h"
#endif
#include "objects/gameplay_keep/gameplay_keep.h"
#define CVAR_COSMETIC(x) "gCosmetics." x
}
extern "C" s32 GetItem_GetShimmerColor(s16,uint8_t*);
void* Combo_ResolveSym(const char*,const char*);
#ifndef HOST_MM
void* Combo_ResolveSym(const char*,const char*) { assert(false); return nullptr; }
#endif
/* PRODUCTION_DRAW_PRELUDE */
#include "soh/Enhancements/randomizer/NeiGiRender.h"
#include "soh/Enhancements/randomizer/NeiGiEnergyTexture.h"

static Gfx opa[8192], xlu[2048], overlay[2048], resource;
static GraphicsContext gfx;
static PlayState play;
static Mtx matrix;
static int depth;
static bool missingSparkle, allocationFailure;
static int resourceLoads;
static std::vector<std::vector<Vtx>> arena;
static std::map<Gfx*, std::vector<Vtx>> vertexLoads;
struct Transform { float x = 0, y = 0, z = 0, scale = 1; };
static Transform transform;
static std::vector<Transform> stack, sparkleTransforms;
static std::vector<std::string> paths;
#ifdef HOST_MM
// Archive ownership boundary for the extracted native MM GI dispatcher.
static std::string legacyModPath;
#endif

extern "C" {
#ifdef HOST_MM
bool ResourceMgr_IsAltAssetsEnabled() { return false; }
int ResourceMgr_IsModAssetForGame(const char* game, const char* path) {
    assert(!strcmp(game, "mm"));
    return legacyModPath == path;
}
#endif
void gSPSegment(void* p, int segment, uintptr_t base) { __gSPSegment((Gfx*)p,segment,base); }
void gSPDisplayList(Gfx* p, Gfx* dl) {
    paths.emplace_back((const char*)dl);
    if (paths.back().find("gEffSparklesDL") != std::string::npos) {
        sparkleTransforms.push_back(transform);
    }
    __gSPDisplayList(p, dl);
}
Gfx* ResourceMgr_LoadGfxByName(const char* path) {
    if (strstr(path, "gEffSparklesDL")) ++resourceLoads;
    return missingSparkle && strstr(path, "gEffSparklesDL") ? nullptr : &resource;
}
void Matrix_Push() { stack.push_back(transform); ++depth; }
void Matrix_Pop() { assert(depth > 0); transform = stack.back(); stack.pop_back(); --depth; }
#ifdef HOST_MM
void Matrix_Scale(f32 x, f32 y, f32 z, MatrixMode) { assert(x == y && y == z); transform.scale *= x; }
void Matrix_Translate(f32 x, f32 y, f32 z, MatrixMode) {
    transform.x += x * transform.scale; transform.y += y * transform.scale; transform.z += z * transform.scale;
}
Mtx* Matrix_Finalize(GraphicsContext*) { return &matrix; }
void Gfx_SetupDL25_Xlu(GraphicsContext*) {}
void Gfx_SetupDL25_Opa(GraphicsContext*) {}
void Gfx_SetupDL26_Opa(GraphicsContext*) {}
void Graph_OpenDisps(Gfx**, Gfx*, GraphicsContext*, const char*, s32) {}
void Graph_CloseDisps(Gfx**, Gfx*, GraphicsContext*, const char*, s32) {}
#else
void Matrix_Scale(f32 x, f32 y, f32 z, u8) { assert(x == y && y == z); transform.scale *= x; }
void Matrix_Translate(f32 x, f32 y, f32 z, u8) {
    transform.x += x * transform.scale; transform.y += y * transform.scale; transform.z += z * transform.scale;
}
Mtx* Matrix_NewMtx(GraphicsContext*, char*, s32) { return &matrix; }
void Gfx_SetupDL_25Xlu(GraphicsContext*) {}
void Gfx_SetupDL_25Opa(GraphicsContext*) {}
void Gfx_SetupDL_26Opa(GraphicsContext*) {}
void Graph_OpenDisps(Gfx**, GraphicsContext*, const char*, s32) {}
void Graph_CloseDisps(Gfx**, GraphicsContext*, const char*, s32) {}
#endif
#ifdef HOST_MM
void Matrix_RotateYF(f32, MatrixMode) {}
void Matrix_RotateZF(f32, MatrixMode) { assert(false && "mask fixture unexpectedly rotated a sword"); }
#else
void Matrix_RotateY(f32, u8) {}
void Matrix_RotateZ(f32, u8) { assert(false && "mask fixture unexpectedly rotated a sword"); }
#endif
void Matrix_ReplaceRotation(MtxF*) {}
void Matrix_Get(MtxF* m) {
    *m = {}; m->xx = m->yy = m->zz = transform.scale;
    m->xw = transform.x; m->yw = transform.y; m->zw = transform.z;
}
void* Graph_Alloc(GraphicsContext*, size_t size) {
    if (allocationFailure) return nullptr;
    assert(size <= 1536 * sizeof(Vtx) && size % sizeof(Vtx) == 0);
    arena.emplace_back(size / sizeof(Vtx)); return arena.back().data();
}
void gSPVertex(Gfx* cmd, uintptr_t data, int n, int v0) {
    assert(n > 0 && n + v0 <= 32);
    auto* vertices = reinterpret_cast<const Vtx*>(data);
    vertexLoads[cmd] = std::vector<Vtx>(vertices, vertices + n);
    __gSPVertex(cmd, data, n, v0);
}
f32 Math_SinS(s16 a) { return sinf(a * 3.14159265358979323846f / 32768); }
f32 Math_CosS(s16 a) { return cosf(a * 3.14159265358979323846f / 32768); }
Color_RGB8 CVarGetColor24(const char*, Color_RGB8 color) { return color; }
#ifndef HOST_MM
int32_t CVarGetInteger(const char*, int32_t value) { return value; }
#endif
int DinFireShield_DrawItem(PlayState*, int16_t) { return 0; }
// Selected model fitting and Din layers have real dedicated sword fixtures.
// This fixture provides neither model family to the production GI dispatcher.
int ResourceMgr_GetGiModelFitForGame(const char*, const char*, float, float, int, float[2]) { return 0; }
int ResourceMgr_GetGiModelsFitForGame(const char*, const char* const*, int, float, float, int, float[2]) { return 0; }
int ResourceMgr_GetDinSwordGiProfileForGame(const char*, const char*) { return 0; }
int ResourceMgr_IsIkanaShieldGiAvailable() {
    assert(false && "mask shimmer fixture must not query the native shield pair"); return 0;
}
int ResourceMgr_GetRewardSurfaceForGame(const char*, const char*, NeiGi::Mesh*) {
    assert(false && "mask shimmer fixture must not query reward geometry"); return 0;
}
#ifdef HOST_MM
Color_RGBA8 CosmeticEditor_GetChangedColor(u8, u8, u8, u8, const char*) {
    assert(false && "mask fixture unexpectedly selected a Din layer"); return {};
}
s16 Play_GetOriginalSceneId(s16 id) { return id; }
s32 GetItem_DrawDungeonItem(PlayState*, s16, s32) { return false; }
int32_t CVarGetInteger(const char*, int32_t value) { return value; }
static s32 GetItem_BottleShimmerColor(s16, Color_RGBA8*) { return false; }
static void GetItem_DrawBottleShimmer(PlayState*, s16) { assert(false); }
#endif
}

#ifdef HOST_MM
#define COMBO_MASK_SHIMMER_HOST_MM
#endif
#include "ComboMaskShimmer.h"
#ifdef HOST_MM
#define COMBO_COLLECTIBLE_SHIMMER_HOST_MM
#endif
#include "ComboCollectibleShimmerHost.h"
#ifdef HOST_MM
#undef COMBO_COLLECTIBLE_SHIMMER_HOST_MM
#endif
// The extracted native dispatcher now shares collectible eligibility. This
// fixture keeps that separate opt-in disabled; palette sampling must stay idle.
extern "C" void OOT_SampleCollectibleShimmerColor(int32_t, uint8_t[4]) {
    assert(false && "mask fixture unexpectedly sampled collectible colors");
}
extern "C" void MM_SampleCollectibleShimmerColor(int32_t, uint8_t[4]) {
    assert(false && "mask fixture unexpectedly sampled collectible colors");
}
#include "ComboItemEffectColors.h"
#include "ComboBottleContents.h"
/* PRODUCTION_FOREIGN_LINKAGE */
using NeiGi::Kind;
static bool HasResource(const char*) { return false; }
#include "ComboSwordGiFit.h"
#ifdef HOST_MM
#define COMBO_DIN_SWORD_GI_HOST_MM
#endif
#include "ComboDinSwordGi.h"
#ifdef HOST_MM
#undef COMBO_DIN_SWORD_GI_HOST_MM
#endif
extern "C" {
#ifdef HOST_MM
#define Gfx_SetupDL_25Xlu Gfx_SetupDL25_Xlu
#define Matrix_NewMtx(ctx, file, line) Matrix_Finalize(ctx)
#define Gfx_SetupDL_25Opa Gfx_SetupDL25_Opa
#define Gfx_SetupDL_26Opa Gfx_SetupDL26_Opa
#define NEI_GI_ROTATE_Y Matrix_RotateYF
#define Matrix_RotateZ Matrix_RotateZF
#endif
#include "soh/Enhancements/randomizer/NeiGiMeshRenderer.inc"
#ifdef HOST_MM
#undef Gfx_SetupDL_25Xlu
#undef Matrix_NewMtx
#undef Gfx_SetupDL_25Opa
#undef Gfx_SetupDL_26Opa
#undef NEI_GI_ROTATE_Y
#undef Matrix_RotateZ
#endif
}

#ifdef HOST_MM
extern "C" { PlayState* gPlayState = &play; }
void DrawOotGetItemOpaXlu(const char* a, Gfx**, const char* b, Gfx**) { paths.emplace_back(a); paths.emplace_back(b); }
#endif
/* PRODUCTION_RPG_DRAWS */
#ifdef HOST_MM
bool DrawNeiRealOpa(const char* path, Gfx**, u8*, float, bool) { paths.emplace_back(path); return true; }
void DrawOotRodStandIn(Gfx**, u8, u8, u8) { assert(false); }
#else
void DrawCustomItemDiamondByPath(PlayState*, const char* path, Gfx**, u8*, float) { paths.emplace_back(path); }
#endif
/* PRODUCTION_SEASON_DRAWS */
/* PRODUCTION_NEI_NATIVE_DISPATCH */
#ifndef HOST_MM
struct ComboForeignDrawInfo { const char* dls[2]; };
static int restoredBlueSegments, blueScrollCalls;
void OOT_RestoreForeignSegs(PlayState*, const int32_t* segs, int32_t count) {
    assert(count==1 && segs[0]==8); ++restoredBlueSegments;
}
extern "C" Gfx* Gfx_TwoTexScrollEx(GraphicsContext*, s32 tile1, u32 x1, u32 y1,
    s32 w1, s32 h1, s32 tile2, u32 x2, u32 y2, s32 w2, s32 h2,
    s32 dx1, s32 dy1, s32 dx2, s32 dy2) {
    assert(tile1==0 && x1==0 && y1==0 && w1==16 && h1==32);
    assert(tile2==1 && x2==47 && y2==uint32_t(-47*8) && w2==16 && h2==32);
    assert(dx1==0 && dy1==0 && dx2==1 && dy2==-8); ++blueScrollCalls; return &resource;
}
#endif
/* PRODUCTION_BLUE_FIRE */

static void ModelDraw(PlayState*, s16 id) {
    gSPDisplayList(gfx.polyOpa.p++, (Gfx*)"__OTR__fixture/mask");
    // Model callbacks can leave scaling/translation behind (native remains do).
#ifdef HOST_MM
    if (id == GID_REMAINS_ODOLWA || id == GID_REMAINS_GOHT || id == GID_REMAINS_GYORG || id == GID_REMAINS_TWINMOLD) {
        Matrix_Scale(.02f, .02f, .02f, MTXMODE_APPLY);
        Matrix_Translate(5000, 5000, 5000, MTXMODE_APPLY);
    }
#endif
}
static struct { void (*drawFunc)(PlayState*, s16); void* drawResources[1]; } sDrawItemTable[256];
/* PRODUCTION_NATIVE */

static void Reset(int frame) {
    memset(opa, 0, sizeof(opa)); memset(xlu, 0, sizeof(xlu));
    gfx.polyOpa.p = opa; gfx.polyXlu.p = xlu;
    gfx.polyOpa.d = std::end(opa);
    gfx.polyXlu.d = std::end(xlu);
    gfx.overlay.p = overlay;
    gfx.overlay.d = std::end(overlay);
    play.state.gfxCtx = &gfx; play.gameplayFrames = frame;
    paths.clear(); sparkleTransforms.clear(); stack.clear(); depth = 0; transform = {};
    arena.clear(); vertexLoads.clear(); resourceLoads = 0;
    play.billboardMtxF = {}; play.billboardMtxF.xx = play.billboardMtxF.yy = play.billboardMtxF.zz = 1;
}
static std::vector<Vtx> ExpandedVertices(bool foreign, const char* owner = "mm") {
    std::vector<Vtx> result, cache;
    int owners = 0, grayscale = 1;
    for (Gfx* cmd = xlu; cmd < gfx.polyXlu.p; ++cmd) {
        unsigned op = cmd->words.w0 >> 24;
        if (op == G_COMBO_RM_PUSH) { assert(foreign && !strcmp((const char*)cmd->words.w1, owner)); ++owners; }
        if (op == G_COMBO_RM_POP) --owners;
        if (op == G_SETGRAYSCALE) { grayscale = cmd->words.w1; assert(grayscale == 0); }
        if (op == G_VTX) cache = vertexLoads.at(cmd);
        auto triangle = [&](uintptr_t bits) {
            assert(owners == (foreign ? 1 : 0) && grayscale == 0);
            for (int shift : {16, 8, 0}) result.push_back(cache.at(((bits >> shift) & 255) / 2));
        };
        if (op == G_TRI1 || op == G_TRI2) triangle(cmd->words.w0);
        if (op == G_TRI2) triangle(cmd->words.w1);
    }
    assert(owners == 0 && grayscale == 0);
    return result;
}
extern "C" void Fixture_DrawCShimmer(PlayState*, const uint8_t color[4]);
static std::vector<Vtx> Check(const uint8_t color[4], bool foreign, const char* owner = "mm", Kind palette = Kind::Neutral) {
    assert(depth == 0 && stack.empty() && transform.scale == 1 && transform.x == 0 && transform.y == 0 && transform.z == 0);
    assert(sparkleTransforms.empty());
    const auto actual = ExpandedVertices(foreign, owner);
    const auto expected = NeiGi::SampleShimmer(play.gameplayFrames, true, NeiGi_CameraBasis(&play), palette);
    assert(actual.size() == expected.count && actual.size() == 360);
    const bool ordinary = color[0] == 220 && color[1] == 225 && color[2] == 240;
    const bool dark = color[0] == 0 && color[1] == 0 && color[2] == 0;
    const uint32_t tint = uint32_t(color[0]) << 16 | uint32_t(color[1]) << 8 | color[2];
    for (size_t i = 0; i < actual.size(); ++i) {
        const auto& vertex = expected.vertices[i];
        assert(actual[i].v.ob[0] == std::lround(vertex.p.x * 16));
        assert(actual[i].v.ob[1] == std::lround(vertex.p.y * 16));
        assert(actual[i].v.ob[2] == std::lround(vertex.p.z * 16));
        uint32_t rgb = vertex.rgb;
        if (palette == Kind::Neutral && !ordinary) rgb = vertex.rgb == 0xFFFFFF ? (dark ? 0x505060 : 0xFFFFFF) : tint;
        assert(actual[i].v.cn[0] == (rgb >> 16));
        assert(actual[i].v.cn[1] == ((rgb >> 8) & 255));
        assert(actual[i].v.cn[2] == (rgb & 255));
        assert(actual[i].v.cn[3] == vertex.alpha);
    }
    uintptr_t prim = 0, env = 0;
    for (Gfx* cmd = xlu; cmd < gfx.polyXlu.p; ++cmd) {
        if ((cmd->words.w0 >> 24) == G_SETPRIMCOLOR) prim = cmd->words.w1;
        if ((cmd->words.w0 >> 24) == G_SETENVCOLOR) env = cmd->words.w1;
    }
    assert(prim == 0xffffffff && env == 0xffffffff);
    return actual;
}
int main() {
    const uint8_t mmMaskColors[24][3] = {
        {220,65,65},{95,70,140},{64,144,255},{135,95,205},
        {245,145,205},{50,220,90},{255,200,55},{220,225,240},
        {255,220,80},{90,205,95},{205,125,80},{240,64,64},
        {220,185,145},{175,180,220},{135,95,205},{245,165,205},
        {235,80,85},{64,144,255},{230,145,85},{225,210,155},
        {235,190,75},{215,220,200},{205,105,65},{0,0,0}
    };
#ifdef HOST_MM
    const int nativeMasks[24]={GID_MASK_POSTMAN,GID_MASK_ALL_NIGHT,GID_MASK_BLAST,GID_MASK_STONE,
        GID_MASK_GREAT_FAIRY,GID_MASK_DEKU,GID_MASK_KEATON,GID_MASK_BREMEN,GID_MASK_BUNNY,GID_MASK_DON_GERO,
        GID_MASK_SCENTS,GID_MASK_GORON,GID_MASK_ROMANI,GID_MASK_CIRCUS_LEADER,GID_MASK_KAFEIS_MASK,GID_MASK_COUPLE,
        GID_MASK_TRUTH,GID_MASK_ZORA,GID_MASK_KAMARO,GID_MASK_GIBDO,GID_MASK_GARO,GID_MASK_CAPTAIN,GID_MASK_GIANT,
        GID_MASK_FIERCE_DEITY};
#endif
    for(int mask=0;mask<24;++mask) {
        uint8_t color[4];assert(ComboMmMaskShimmerColor(mask,color));
        assert(!memcmp(color,mmMaskColors[mask],3)&&color[3]==255);
#ifdef HOST_MM
        uint8_t native[4];assert(GetItem_GetShimmerColor(nativeMasks[mask],native));
        assert(!memcmp(color,native,4));
#endif
        Reset(47);ComboDrawMaskShimmer(&play,nullptr,color,"mm");Check(color,true);
    }
    const uint8_t ootMaskColors[9][3] = {
        {255,200,55},{220,215,180},{155,100,70},{255,220,80},{215,150,85},
        {100,180,230},{230,105,70},{235,80,85},{230,60,60}
    };
#ifndef HOST_MM
    const int nativeMasks[8]={GID_MASK_KEATON,GID_MASK_SKULL,GID_MASK_SPOOKY,GID_MASK_BUNNY,GID_MASK_GORON,
        GID_MASK_ZORA,GID_MASK_GERUDO,GID_MASK_TRUTH};
#endif
    for(int mask=0;mask<9;++mask) {
        uint8_t color[4];assert(ComboOotMaskShimmerColor(mask,color));
        assert(!memcmp(color,ootMaskColors[mask],3)&&color[3]==255);
#ifndef HOST_MM
        if(mask<8){uint8_t native[4];assert(GetItem_GetShimmerColor(nativeMasks[mask],native));assert(!memcmp(color,native,4));}
#endif
        Reset(47);ComboDrawMaskShimmer(&play,nullptr,color,"oot");Check(color,true,"oot");
    }
    puts("PASS complete 24 MM/9 OoT mask palettes and foreign emitted vertices, including Blast blue/white and Stone purple");
    for (int season = 1; season <= 4; ++season) {
        Reset(47);
#ifdef HOST_MM
        DrawOotNeiSeason(season);
#else
        DrawSeasonCommon(&play, season - 1);
#endif
        assert(paths.empty() && !vertexLoads.empty() && depth == 0);
#ifdef HOST_MM
        Reset(47);
        assert(MM_TryDrawNeiGi(static_cast<RandoItemId>(RI_OOT_NEI_SEASON_SPRING + season - 1)));
        assert(paths.empty() && !vertexLoads.empty() && depth == 0 && ownerLookups == 0);
#endif
    }
#ifdef HOST_MM
    gPlayState = nullptr;
    assert(!MM_TryDrawNeiGi(RI_OOT_NEI_SEASON_SUMMER));
    gPlayState = &play;
    Reset(47);
    // A local replacement keeps its legacy geometry while the owner lookup
    // supplies independent identity when that module is available.
    legacyModPath = "objects/object_nei_fire_rod/Cylinder_001_opaque_dl";
    assert(!MM_TryDrawNeiGi(RI_OOT_NEI_FIRE_ROD));
    assert(ownerLookups == 2 && paths.empty() && vertexLoads.empty());
    legacyModPath.clear();
    assert(!MM_TryDrawNeiGi(RI_OOT_NEI_WHIP)); // Fixture has no dormant OoT module.
    assert(ownerLookups == 4);
#endif
    Reset(47);
#ifdef HOST_MM
    DrawOotNeiSeason(5);
#else
    Randomizer_DrawNeiRodOfSeasons(&play, nullptr);
#endif
    assert(paths.size() == 1 && paths[0].find("rod_of_seasons") != std::string::npos);
    for (auto& row : sDrawItemTable) { row.drawFunc = ModelDraw; row.drawResources[0] = (void*)"__OTR__fixture/mask"; }
    int eligible = 0;
    for (int id = 0; id < 256; ++id) {
        uint8_t color[4]{};
        if (!GetItem_GetShimmerColor(id, color)) continue;
        const auto palette =
#ifdef HOST_MM
            id == GID_CHATEAU ? Kind::ChateauRomani :
#endif
            Kind::Neutral;
        ++eligible;
        Reset(47); GetItem_Draw(&play, id); auto first = Check(color, false, "mm", palette); assert(paths.size() == 1);
        Reset(47); GetItem_Draw(&play, id);
        auto second = Check(color, false, "mm", palette);
        assert(first.size() == second.size());
        for (size_t i = 0; i < first.size(); ++i) assert(!memcmp(&first[i], &second[i], sizeof(Vtx)));
        missingSparkle = true; Reset(47); GetItem_Draw(&play, id);
        Check(color, false, "mm", palette); assert(resourceLoads == 0);
        missingSparkle = false;
        allocationFailure = true; Reset(47); GetItem_Draw(&play, id);
        assert(paths.size() == 1 && vertexLoads.empty() && depth == 0 && stack.empty());
        assert(ExpandedVertices(false).empty());
        allocationFailure = false;
    }
#ifdef HOST_MM
    assert(eligible == 36); // Masks/remains, fairies, receipt contents, Seahorse and Chateau.
    uint8_t sunMask[4];const uint8_t sunGold[4]={255,205,70,255};
    assert(GetItem_GetShimmerColor(GID_MASK_SUN,sunMask)&&!memcmp(sunMask,sunGold,4));
    for (int id : {GID_FAIRY,GID_FAIRY_2}) {
        uint8_t pink[4]{};
        const uint8_t expectedPink[4]={255,160,235,255};
        assert(GetItem_GetShimmerColor(id,pink) && !memcmp(pink,expectedPink,4));
    }
    uint8_t goht[4], keaton[4], bremen[4];
    assert(GetItem_GetShimmerColor(GID_REMAINS_GOHT, goht));
    assert(goht[0] > goht[1] && goht[0] > goht[2]);
    assert(GetItem_GetShimmerColor(GID_MASK_KEATON, keaton));
    assert(GetItem_GetShimmerColor(GID_MASK_BREMEN, bremen));
    assert(memcmp(keaton, bremen, 3) != 0);
    const int special[] = { GID_MASK_DEKU, GID_MASK_GORON, GID_MASK_ZORA, GID_MASK_FIERCE_DEITY };
    const uint8_t expected[4][3] = { {50,220,90}, {240,64,64}, {64,144,255}, {0,0,0} };
    for (int i = 0; i < 4; ++i) { uint8_t color[4]; assert(GetItem_GetShimmerColor(special[i], color)); assert(!memcmp(color, expected[i], 3)); }
#else
    uint8_t blueBottle[4]{};
    assert(GetItem_GetShimmerColor(GID_POTION_BLUE, blueBottle));
    assert(blueBottle[0]==100 && blueBottle[1]==160 && blueBottle[2]==255);
    assert(eligible > 8);
#endif
    for (int profile : {CW_SHIMMER_MUSHROOM,CW_SHIMMER_PRINCESS,CW_SHIMMER_GOLD_DUST,CW_SHIMMER_FAIRY,CW_SHIMMER_SEAHORSE}) {
        uint8_t color[4]; assert(ComboBottleShimmer_Color(profile,color));
        Reset(47); NeiGi_DrawShimmerOverlay(&play,color,nullptr); Check(color,false);
    }
    for (int frame : {0,27,719,65535}) {
        Reset(frame); ComboBottleShimmer_DrawMotes(&play,CW_SHIMMER_FAIRY);
        const auto actual=ExpandedVertices(false);
        const auto expected=NeiGi::SampleBottleMotes(CW_SHIMMER_FAIRY,frame,true,NeiGi_CameraBasis(&play));
        assert(actual.size()==192 && actual.size()==expected.count && resourceLoads==0);
        for(size_t i=0;i<actual.size();++i) {
            const auto& v=expected.vertices[i];
            assert(actual[i].v.ob[0]==std::lround(v.p.x*16) && actual[i].v.ob[1]==std::lround(v.p.y*16) && actual[i].v.ob[2]==std::lround(v.p.z*16));
            assert(actual[i].v.cn[0]==255 && actual[i].v.cn[1]==160 && actual[i].v.cn[2]==235 && actual[i].v.cn[3]==v.alpha);
        }
        assert(depth==0 && stack.empty() && transform.scale==1 && transform.x==0 && transform.y==0 && transform.z==0);
        uintptr_t prim=0,env=0;
        for(Gfx* cmd=xlu;cmd<gfx.polyXlu.p;++cmd) {
            if((cmd->words.w0>>24)==G_SETPRIMCOLOR)prim=cmd->words.w1;
            if((cmd->words.w0>>24)==G_SETENVCOLOR)env=cmd->words.w1;
        }
        assert(prim==0xffffffff && env==0xffffffff);
    }
    for(int profile:{0,int(CW_SHIMMER_PRINCESS),int(CW_SHIMMER_SEAHORSE)}) {
        Reset(47);ComboBottleShimmer_DrawMotes(&play,profile);assert(gfx.polyXlu.p==xlu && vertexLoads.empty());
    }
    Reset(47);allocationFailure=true;ComboBottleShimmer_DrawMotes(&play,CW_SHIMMER_FAIRY);
    assert(ExpandedVertices(false).empty() && depth==0 && stack.empty());allocationFailure=false;
    for(int remaining=0;remaining<64;++remaining) {
        Reset(47);gfx.polyOpa.d=opa+remaining;gfx.polyXlu.d=xlu+remaining;
        ComboBottleShimmer_DrawMotes(&play,CW_SHIMMER_FAIRY);
        assert(gfx.polyOpa.p<=gfx.polyOpa.d && gfx.polyXlu.p<=gfx.polyXlu.d && depth==0 && stack.empty());
    }
    puts("PASS exact bottle five-star colors and real-GBI fairy motes, state/matrix restoration, failed allocation and arena boundaries");
    for (int profile = 0; profile < 9; ++profile) {
        uint8_t color[4]; ComboMaskShimmerColor(profile, color);
        Reset(47); ComboDrawMaskShimmer(&play, "__OTR__@mm:objects/gameplay_keep/gEffSparklesDL", color, "mm"); Check(color, true);
    }
#ifndef HOST_MM
    Reset(47); play.state.frames=47;
    ComboForeignDrawInfo blueRecipe{{"__OTR__@oot:stick","__OTR__@oot:flame"}};
    OOT_DrawForeignBlueFire(&play,&blueRecipe);
    assert(paths.size()==2 && paths[0]==blueRecipe.dls[0] && paths[1]==blueRecipe.dls[1]);
    assert(restoredBlueSegments==1 && blueScrollCalls==1 && depth==0 && stack.empty());
    assert(transform.x==0 && transform.y==0 && transform.scale==1);
    const auto statDraws = {Randomizer_DrawDefenseUpgrade, Randomizer_DrawSpeedUpgrade,
        Randomizer_DrawPowerUpgrade, Randomizer_DrawCrawlSpeedUpgrade,
        Randomizer_DrawClimbSpeedUpgrade, Randomizer_DrawPushSpeedUpgrade};
    const int statProfiles[] = {0,1,2,4,5,6};
    int statIndex=0;
    for (auto draw: statDraws) {
        uint8_t color[4]; ComboRpgShimmerColor(statProfiles[statIndex++], color);
        Reset(47); draw(&play, nullptr); Check(color, false); assert(paths.size()==1);
    }
#endif
#ifdef HOST_MM
    Reset(47); DrawOotRutosLetter();
    uint8_t letterColor[4]; ComboMaskShimmerColor(0,letterColor); Check(letterColor, true, "oot");
    assert(paths.size()==2);
    const uint8_t aliasColor[4]={100,160,255,255};
    Reset(47); DrawOotBottleWithShimmer(GID_POTION_BLUE,aliasColor);
    Check(aliasColor,true,"mm"); assert(paths.size()==1);
    const uint8_t fairyColor[4]={255,160,235,255};
    for (int id : {GID_FAIRY,GID_FAIRY_2}) {
        Reset(47); DrawOotBottleWithShimmer(id,fairyColor);
        Check(fairyColor,false,"mm"); assert(paths.size()==1); // Exactly one 360-vertex hex overlay.
    }
#endif
    for (int profile=0; profile<7; ++profile) {
        uint8_t color[4]; assert(ComboRpgShimmerColor(profile, color));
        Reset(47); ComboDrawMaskShimmer(&play, nullptr, color, nullptr); Check(color, false);
    }
    // Execute the shared GI body with both engines' real GBI. Its model and
    // optional crystal pass must enclose every child effect in one owner scope.
    for (Kind kind : {Kind::Sand, Kind::Tornado, Kind::Water, Kind::Meteor,
                      Kind::Storm, Kind::Shadow, Kind::Slate, Kind::Hourglass,
                      Kind::DarkCrystal, Kind::SwordAura, Kind::CaneBlue})
      for (bool shimmer : {false, true}) {
        Reset(47);
        constexpr float center[3] = {0,0,0};
        const auto catalog = std::find_if(
            std::begin(NeiGi::kFrameBounds), std::end(NeiGi::kFrameBounds),
            [kind](const auto &frame) { return frame.effect == kind; });
        assert(catalog != std::end(NeiGi::kFrameBounds));
        const std::string root =
            std::string("__OTR__@oot:objects/nei_gi_redesign/") + catalog->slug;
        const std::string bodyPath = root + "/gi_dl",
                          skinPath = root + "/gi_xlu_dl";
        const char *body = bodyPath.c_str();
        const char *skin = skinPath.c_str();
        NeiGi_DrawPresentation(&play,body,skin,1,int(kind),center,shimmer,"oot");
        const auto actual=ExpandedVertices(true,"oot");
        auto expected=NeiGi::SampleSpecial(kind,47,NeiGi_CameraBasis(&play));
        auto glitter=NeiGi::SampleShimmer(47,shimmer,NeiGi_CameraBasis(&play),kind);
        assert(actual.size()==expected.count+glitter.count);
        for(size_t i=0;i<actual.size();++i) {
            const auto& v=i<expected.count ? expected.vertices[i] : glitter.vertices[i-expected.count];
            assert(actual[i].v.ob[0]==std::lround(v.p.x*16));
            assert(actual[i].v.ob[1]==std::lround(v.p.y*16));
            assert(actual[i].v.ob[2]==std::lround(v.p.z*16));
            assert(actual[i].v.cn[3]==v.alpha);
        }
        int scopes[2]={}, models[2]={};
        int stream=0;
        for(auto range : {std::pair(opa,gfx.polyOpa.p),std::pair(xlu,gfx.polyXlu.p)}) {
            for(Gfx* cmd=range.first;cmd<range.second;++cmd) {
                const unsigned op=cmd->words.w0>>24;
                if(op==G_COMBO_RM_PUSH) {assert(!strcmp((const char*)cmd->words.w1,"oot"));++scopes[stream];}
                if(op==G_COMBO_RM_POP) --scopes[stream];
                if(op==G_DL_OTR_FILEPATH) {
                    assert(scopes[stream]==1);
                    assert(!strcmp((const char*)cmd->words.w1,stream ? skin : body));
                    ++models[stream];
                }
            }
            assert(scopes[stream]==0 && models[stream]==1); ++stream;
        }
        assert(depth==0 && stack.empty() && transform.scale==1 && resourceLoads==0);
      }
    for (int season=1; season<=5; ++season) for (int frame: {0,47,119,179,180,359,719,65535}) {
        Reset(frame); NeiGi_DrawSeasonOverlay(&play, season, "oot");
        const auto actual=ExpandedVertices(true, "oot");
        auto expected=NeiGi::SampleSeason(frame, season, NeiGi_CameraBasis(&play));
        const auto rays=NeiGi::SampleSeasonSunRays(frame, season, NeiGi_CameraBasis(&play));
        for(size_t i=0;i<rays.count;++i) expected.vertices[expected.count++]=rays.vertices[i];
        assert(!actual.empty() && actual.size()==expected.count && actual.size()<=720);
        assert(depth==0 && stack.empty() && resourceLoads==0);
        for(size_t i=0;i<actual.size();++i) {
            const auto& v=expected.vertices[i];
            assert(std::isfinite(v.p.x) && std::isfinite(v.p.y) && std::isfinite(v.p.z));
            assert(actual[i].v.ob[0]==std::lround(v.p.x*16));
            assert(actual[i].v.ob[1]==std::lround(v.p.y*16));
            assert(actual[i].v.ob[2]==std::lround(v.p.z*16));
            assert(actual[i].v.cn[3]==v.alpha);
        }
        Reset(frame); NeiGi_DrawSeasonOverlay(&play, season, "oot");
        const auto repeat=ExpandedVertices(true, "oot");
        assert(repeat.size()==actual.size());
        for(size_t i=0;i<repeat.size();++i) assert(!memcmp(&repeat[i], &actual[i], sizeof(Vtx)));
    }
    Reset(47); NeiGi_DrawSeasonOverlay(&play, 0, nullptr); assert(vertexLoads.empty());
    // The summer corona has one bounded, stable I8 tile. Its scroll is emitted
    // by the real host renderer, independently of archive/resource selection.
    uintptr_t sunImage = 0;
    uint32_t scrollAt47 = 0, scrollAt51 = 0;
    for (int frame : {47, 51}) {
        Reset(frame); NeiGi_DrawSeasonOverlay(&play, 2, "oot");
        int images = 0, scrolls = 0;
        for (Gfx* cmd = xlu; cmd < gfx.polyXlu.p; ++cmd) {
            const unsigned op = cmd->words.w0 >> 24;
            if (op == G_SETTIMG) {
                ++images;
                assert(((cmd->words.w0 >> 21) & 7) == G_IM_FMT_I);
                if (sunImage) assert(sunImage == cmd->words.w1);
                sunImage = cmd->words.w1;
            }
            if (op == G_SETTILESIZE && (cmd->words.w0 & 0x00ffffff)) {
                ++scrolls;
                (frame == 47 ? scrollAt47 : scrollAt51) = cmd->words.w0 & 0x00ffffff;
            }
        }
        assert(images == 1 && scrolls == 1 && sunImage);
        assert(resourceLoads == 0 && depth == 0 && stack.empty());
        ExpandedVertices(true, "oot");
    }
    assert(scrollAt47 != scrollAt51);
    for (int season = 1; season <= 5; ++season) {
        Reset(227); // The rod is in its summer phase.
        for (int slot = 0; slot < 8; ++slot) NeiGi_DrawSeasonOverlay(&play, season, "oot");
        size_t vertexBytes = 0;
        for (const auto& allocation : arena) vertexBytes += allocation.size() * sizeof(Vtx);
        assert(vertexBytes < 65536 && gfx.polyXlu.p - xlu < 2048);
        assert(depth == 0 && stack.empty() && transform.scale == 1 && paths.empty());
        ExpandedVertices(true, "oot");
        printf("Season %d: eight items use %zu vertex bytes and %td XLU commands\n",
               season, vertexBytes, gfx.polyXlu.p - xlu);
    }
    Reset(47); allocationFailure=true; NeiGi_DrawSeasonOverlay(&play, 4, "oot");
    assert(ExpandedVertices(true,"oot").empty() && depth==0); allocationFailure=false;
    // The authored Rod composes one model with cycling weather through the
    // shared renderer. This must survive the art/season boundary in both hosts.
    for (int frame : {0, 227, 407, 587}) {
        Reset(frame);
        constexpr float center[3] = {0, 0, 0};
        constexpr const char* rod = "__OTR__@oot:objects/nei_gi_redesign/rod_of_seasons/gi_dl";
        NeiGi_DrawPresentation(&play, rod, nullptr, 1, int(Kind::SeasonCycle), center, false, "oot");
        auto expected = NeiGi::SampleSeason(frame, 5, NeiGi_CameraBasis(&play));
        auto rays = NeiGi::SampleSeasonSunRays(frame, 5, NeiGi_CameraBasis(&play));
        auto actual = ExpandedVertices(true, "oot");
        assert(actual.size() == expected.count + rays.count && !actual.empty());
        int models = 0;
        for (Gfx* cmd = opa; cmd < gfx.polyOpa.p; ++cmd) {
            if ((cmd->words.w0 >> 24) == G_DL_OTR_FILEPATH) {
                assert(!strcmp((const char*)cmd->words.w1, rod));
                ++models;
            }
        }
        assert(models == 1 && resourceLoads == 0);
        assert(depth == 0 && stack.empty() && transform.scale == 1);
    }
    uint8_t ordinary[4]; ComboMaskShimmerColor(0, ordinary);
    Reset(47); ComboDrawMaskShimmer(&play, nullptr, ordinary, "oot"); Check(ordinary, true, "oot");
    allocationFailure = true; Reset(47); ComboDrawMaskShimmer(&play, nullptr, ordinary, "oot");
    assert(ExpandedVertices(true, "oot").empty() && depth == 0 && stack.empty()); allocationFailure = false;
    for (int frame : {0, 47, 179, 180, 65535}) {
        Reset(frame); Fixture_DrawCShimmer(&play, ordinary); Check(ordinary, false);
        Reset(frame); play.billboardMtxF.xx = play.billboardMtxF.yy = 0;
        play.billboardMtxF.xy = -1; play.billboardMtxF.yx = 1;
        Fixture_DrawCShimmer(&play, ordinary); Check(ordinary, false);
    }
    Reset(47); GetItem_Draw(&play, GID_BOMB); assert(paths.size() == 1 && sparkleTransforms.empty());
    puts("PASS both-host weather-only native seasons, rod isolation, sun I8 scroll, bounded shop submission and retained mask/remains shimmer");
}
