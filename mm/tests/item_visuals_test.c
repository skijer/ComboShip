#include "global.h"
#include "din_fire_shield.h"
#include "2s2h/BenGui/CosmeticEditor.h"
#include "2s2h/Enhancements/ItemVisuals.h"
#include "2s2h/Rando/DungeonItemVisuals.h"
#include "assets/objects/gameplay_keep/gameplay_keep.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "ComboBottleContents.h"
#define COMBO_MASK_SHIMMER_HOST_MM
#include "ComboMaskShimmer.h"
#undef COMBO_MASK_SHIMMER_HOST_MM
#include <libultraship/bridge/consolevariablebridge.h>
#define COMBO_COLLECTIBLE_SHIMMER_HOST_MM
#include "ComboCollectibleShimmerHost.h"
#undef COMBO_COLLECTIBLE_SHIMMER_HOST_MM

// Collectible effects have their own enabled-path suite. Keep this fixture's
// dungeon/bottle dispatch isolated while executing the real eligibility helper.
void MM_SampleCollectibleShimmerColor(int32_t profile, uint8_t color[4]) {
    assert(false && "item visuals fixture unexpectedly sampled collectible colors");
}
void NeiGi_DrawTintedShimmerOverlay(PlayState* play, const uint8_t color[4], const char* owner) {
    assert(false && "item visuals fixture unexpectedly drew collectible shimmer");
}

static Gfx sOpa[256], sXlu[256], sResource;
static GraphicsContext sGfx;
static PlayState sPlay;
static Mtx sMatrix;
static int sTint, sShimmer, sMissing, sEdited, sDepth, sDraws, sTransforms;
static int sEmblemEdited, sOverlayDraws;
static uint8_t sOverlayColor[4];
static unsigned sOverlayFrame;
static unsigned sChateauDraws;
// The shared renderer's geometry is tested by run_mask_shimmer_tests.py.
// This native dispatch fixture checks the C boundary, palette and frame.
void NeiGi_DrawShimmerOverlay(PlayState* play, const uint8_t color[4], const char* owner) {
    assert(owner == NULL);
    ++sOverlayDraws;
    memcpy(sOverlayColor, color, 4);
    sOverlayFrame = play->gameplayFrames;
}
// Chateau geometry has its own production-renderer suite; observe the native dispatch.
void NeiGi_DrawChateauShimmer(PlayState* play) {
    assert(play == &sPlay);
    ++sChateauDraws;
}
static float sTrace[128];
static char sLastColorId[64];
static const char* sBody = "fixture/body";
static const char* sGlass = "fixture/glass";

// Din rendering has its own production-module suite; exercise native item visuals here.
int DinFireShield_DrawItem(PlayState* play, int16_t drawId) {
    return 0;
}

static void FixtureDraw(PlayState* play, s16 id) {
    ++sDraws;
    gSPDisplayList(sGfx.polyOpa.p++, (Gfx*)sBody);
    if (id == GID_COMPASS || id == GID_KEY_BOSS)
        gSPDisplayList(sGfx.polyXlu.p++, (Gfx*)sGlass);
}
static struct {
    void (*drawFunc)(PlayState*, s16);
    void* drawResources[1];
} sDrawItemTable[256];
int32_t CVarGetInteger(const char* name, int32_t fallback) {
    if (!strcmp(name, "gEnhancements.DungeonItemColors"))
        return sTint;
    if (!strcmp(name, "gEnhancements.BottleShimmer"))
        return sShimmer;
    if (strstr(name, "gCosmetic.Items.") == name && strstr(name, "Emblem.Changed"))
        return sEmblemEdited;
    return fallback;
}
Color_RGBA8 CosmeticEditor_GetChangedColor(u8 r, u8 g, u8 b, u8 a, const char* id) {
    strcpy(sLastColorId, id);
    int edited = strstr(id, "Emblem") ? sEmblemEdited : sEdited;
    return edited ? (Color_RGBA8){ 17, 123, 241, a } : (Color_RGBA8){ r, g, b, a };
}
bool ResourceMgr_IsAltAssetsEnabled(void) {
    return false; // This suite deliberately verifies the vanilla fallback.
}
u8 ResourceMgr_FileExists(const char* p) {
    assert(strstr(p, "__OTR__objects/combo_dungeon_keys/") == p);
    return false; // Bundled key roots are absent in this fallback fixture.
}
u8 ResourceMgr_FileAltExists(const char* p) {
    return false; // No replacement assets are available either.
}
Gfx* ResourceMgr_LoadGfxByName(const char* p) {
    return sMissing ? NULL : &sResource;
}
void gSPDisplayList(Gfx* pkt, Gfx* dl) {
    __gSPDisplayList(pkt, dl);
}
void Graph_OpenDisps(Gfx** r, Gfx* v, GraphicsContext* g, const char* f, s32 l) {
}
void Graph_CloseDisps(Gfx** r, Gfx* v, GraphicsContext* g, const char* f, s32 l) {
}
void Gfx_SetupDL25_Xlu(GraphicsContext* gfx) {
}
void Gfx_SetupDL25_Opa(GraphicsContext* gfx) {
}
Mtx* Matrix_Finalize(GraphicsContext* gfx) {
    return &sMatrix;
}
void Matrix_Push(void) {
    ++sDepth;
}
void Matrix_Pop(void) {
    --sDepth;
    assert(sDepth >= 0);
}
void Matrix_Translate(f32 x, f32 y, f32 z, MatrixMode mode) {
    assert(sTransforms + 3 <= 128);
    sTrace[sTransforms++] = x;
    sTrace[sTransforms++] = y;
    sTrace[sTransforms++] = z;
}
void Matrix_Scale(f32 x, f32 y, f32 z, MatrixMode mode) {
    assert(x > 0 && x < 1 && x == y && y == z);
}
void Matrix_ReplaceRotation(MtxF* m) {
}
void FrameInterpolation_RecordOpenChild(const void* a, int b) {
}
void FrameInterpolation_RecordCloseChild(void) {
}
f32 Math_SinS(s16 a) {
    return sinf(a * 3.14159265358979323846f / 32768);
}
f32 Math_CosS(s16 a) {
    return cosf(a * 3.14159265358979323846f / 32768);
}
s16 Play_GetOriginalSceneId(s16 sceneId) {
    return sceneId == SCENE_INISIE_R ? SCENE_INISIE_N : sceneId;
}

#include "item_visuals_production.inc"

static void Reset(void) {
    memset(sOpa, 0, sizeof(sOpa));
    memset(sXlu, 0, sizeof(sXlu));
    sGfx.polyOpa.p = sOpa;
    sGfx.polyXlu.p = sXlu;
    sPlay.state.gfxCtx = &sGfx;
    sPlay.sceneId = SCENE_SEA;
    sDepth = sDraws = sTransforms = 0;
}
static void CheckTint(int owner, int gid) {
    static const u32 colors[] = { 0xEC78BA, 0x81AD46, 0x635AB7, 0xB1A553 };
    static const char* ids[] = { "Items.Woodfall", "Items.Snowhead", "Items.GreatBay", "Items.StoneTower" };
    Reset();
    assert(GetItem_DrawDungeonItem(&sPlay, gid, owner));
    assert(!strcmp(sLastColorId, ids[owner]));
    FixtureDraw(&sPlay, GID_BOMB);
    int tint = 0, bodies = 0;
    u32 rgba = 0;
    for (Gfx* g = sOpa; g < sGfx.polyOpa.p; ++g) {
        unsigned op = g->words.w0 >> 24;
        if (op == G_SETINTENSITY)
            rgba = g->words.w1;
        if (op == G_SETGRAYSCALE)
            tint = g->words.w1;
        if (op == G_DL) {
            assert(tint == (bodies == 0));
            if (!bodies) {
                assert((rgba >> 8) == (sEdited ? 0x117BF1 : colors[owner]));
                assert((rgba & 255) > 0 && (rgba & 255) < 255);
            }
            ++bodies;
        }
    }
    assert(bodies == 2 && tint == 0);
    for (Gfx* g = sXlu; g < sGfx.polyXlu.p; ++g)
        assert((g->words.w0 >> 24) != G_SETGRAYSCALE);
}
int main(void) {
    Color_RGBA8 poeColor;
    assert(GetItem_BottleShimmerColor(GID_POE, &poeColor));
    assert(poeColor.r == 100 && poeColor.g == 0 && poeColor.b == 200);
    assert(GetItem_BottleShimmerColor(GID_BIG_POE, &poeColor));
    assert(poeColor.r == 150 && poeColor.g == 200 && poeColor.b == 0);
    for (int i = 0; i < 256; ++i) {
        sDrawItemTable[i].drawFunc = FixtureDraw;
        sDrawItemTable[i].drawResources[0] = (void*)sBody;
    }
    const RandoItemId owners[4][4] = {
        { RI_WOODFALL_SMALL_KEY, RI_WOODFALL_BOSS_KEY, RI_WOODFALL_MAP, RI_WOODFALL_COMPASS },
        { RI_SNOWHEAD_SMALL_KEY, RI_SNOWHEAD_BOSS_KEY, RI_SNOWHEAD_MAP, RI_SNOWHEAD_COMPASS },
        { RI_GREAT_BAY_SMALL_KEY, RI_GREAT_BAY_BOSS_KEY, RI_GREAT_BAY_MAP, RI_GREAT_BAY_COMPASS },
        { RI_STONE_TOWER_SMALL_KEY, RI_STONE_TOWER_BOSS_KEY, RI_STONE_TOWER_MAP, RI_STONE_TOWER_COMPASS },
    };
    const int gids[] = { GID_KEY_SMALL, GID_KEY_BOSS, GID_DUNGEON_MAP, GID_COMPASS };
    sTint = 1;
    for (int edit = 0; edit < 2; ++edit) {
        sEdited = edit;
        for (int d = 0; d < 4; ++d)
            for (int k = 0; k < 4; ++k) {
                assert(DungeonItem_GetOwner(owners[d][k]) == d);
                CheckTint(d, gids[k]);
            }
    }
    assert(DungeonItem_GetOwner(RI_OOT_MAP_WATER_TEMPLE) == -1);
    assert(DungeonItem_GetOwner(RI_NONE) == -1);
    Reset();
    assert(!GetItem_DrawDungeonItem(&sPlay, GID_BOMB, 0));
    assert(!GetItem_DrawDungeonItem(&sPlay, GID_COMPASS, -1));
    assert(!GetItem_DrawDungeonItem(&sPlay, GID_COMPASS, 4));
    sTint = 0;
    assert(!GetItem_DrawDungeonItem(&sPlay, GID_COMPASS, 0));
    sTint = 1;
    sMissing = 1;
    assert(!GetItem_DrawDungeonItem(&sPlay, GID_COMPASS, 0));
    assert(sGfx.polyOpa.p == sOpa && sGfx.polyXlu.p == sXlu && sDraws == 0);
    sMissing = 0;
    // Emblem edits also reach the vanilla boss gem without the body-color checkbox.
    sTint = 0;
    sEmblemEdited = 1;
    for (int d = 0; d < 4; ++d) {
        Reset();
        assert(GetItem_DrawDungeonItem(&sPlay, GID_KEY_BOSS, d));
        assert(strstr(sLastColorId, "Emblem"));
        int gemTint = 0, gems = 0;
        for (Gfx* g = sXlu; g < sGfx.polyXlu.p; ++g) {
            unsigned op = g->words.w0 >> 24;
            if (op == G_SETGRAYSCALE)
                gemTint = g->words.w1;
            if (op == G_DL) {
                assert(gemTint);
                ++gems;
            }
        }
        assert(gems == 1 && !gemTint);
        for (Gfx* g = sOpa; g < sGfx.polyOpa.p; ++g)
            assert((g->words.w0 >> 24) != G_SETGRAYSCALE || g->words.w1 == 0);
    }
    sEmblemEdited = 0;
    sTint = 1;
    const s16 scenes[] = { SCENE_MITURIN, SCENE_HAKUGIN_BS, SCENE_SEA, SCENE_INISIE_R };
    const char* ids[] = { "Items.Woodfall", "Items.Snowhead", "Items.GreatBay", "Items.StoneTower" };
    for (int d = 0; d < 4; ++d) {
        Reset();
        sPlay.sceneId = scenes[d];
        GetItem_Draw(&sPlay, GID_DUNGEON_MAP);
        assert(!strcmp(sLastColorId, ids[d]));
    }
    Reset();
    sPlay.sceneId = SCENE_CLOCKTOWER;
    GetItem_Draw(&sPlay, GID_COMPASS);
    assert(sGfx.polyOpa.p == sOpa + 1 && sOpa[0].words.w0 >> 24 == G_DL);
    const int bottles[] = { GID_POTION_RED, GID_POTION_GREEN, GID_POTION_BLUE, GID_FAIRY,
                            GID_FAIRY_2,    GID_POE,          GID_BIG_POE };
    sShimmer = 1;
    for (int b = 0; b < 7; ++b) {
        sPlay.gameplayFrames = 31;
        float trace[128];
        for (int repeat = 0; repeat < 2; ++repeat) {
            Reset();
            GetItem_Draw(&sPlay, bottles[b]);
            int motes = 0;
            for (Gfx* g = sXlu; g < sGfx.polyXlu.p; ++g)
                if (g->words.w0 >> 24 == G_DL && !strcmp((const char*)g->words.w1, gEffSparklesDL))
                    ++motes;
            assert(motes == 3 && sDraws == 1 && sDepth == 0 && sTransforms == 9);
            if (!repeat)
                memcpy(trace, sTrace, sizeof(trace));
            else
                assert(!memcmp(trace, sTrace, sTransforms * sizeof(float)));
        }
        ++sPlay.gameplayFrames;
        Reset();
        GetItem_Draw(&sPlay, bottles[b]);
        assert(memcmp(trace, sTrace, sTransforms * sizeof(float)));
    }
    Reset();
    GetItem_Draw(&sPlay, GID_BOTTLE);
    assert(sGfx.polyXlu.p == sXlu);
    sShimmer = 0;
    Reset();
    GetItem_Draw(&sPlay, GID_POE);
    assert(sGfx.polyXlu.p == sXlu);
    Reset();
    GetItem_Draw(&sPlay, GID_CHATEAU);
    assert(sChateauDraws == 1 && sDraws == 1 && sDepth == 0);
    sMissing = 1;
    Reset();
    GetItem_Draw(&sPlay, GID_POE);
    assert(sDraws == 0);
    GetItem_Draw(&sPlay, GID_CHATEAU);
    assert(sChateauDraws == 1); // Missing model must not draw a detached shimmer.
    puts("PASS 16 owner IDs, 32 tint scopes, exclusions, live hex, missing resources, 7 deterministic three-mote "
         "bottles");
    return 0;
}
