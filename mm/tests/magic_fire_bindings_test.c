// Execute MM's accepted sphere draw with real types/GBI and resource fixtures.
#include "global.h"
#include "2s2h/BenGui/CosmeticEditor.h"
#include "mods/nei_oot_compat.h"
#include "soh/ResourceManagerHelpers.h"
#include "align_asset_macro.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <libultraship/bridge/resourcebridge.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define REQUIRE(c)                                               \
    do {                                                         \
        if (!(c)) {                                              \
            fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); \
            exit(1);                                             \
        }                                                        \
    } while (0)
void* OotAssets_LoadGfx(const char* path);
void* OotAssets_LoadTexOrDList(const char* path);
#define THIS ((MagicFire*)thisx)
#include "magic_fire_draw_production.inc"
#undef THIS

static GraphicsContext gfx;
static PlayState play;
static Gfx commands[128], reference[128], lists[2][2][1], scroll[1];
static Vtx vertices[2][76];
static u8 textures[2][64 * 64];
static Mtx matrix;
static int alt, missing, customColors, medallion, loads[3];
static int vertexLoads, scrollCount;
static int privateAssets, privateFailures, privateQueries, privateLoads;
static u8 privatePixels[2][2048];
static const char* privatePaths[] = {
    "__OTR__custom/medallion_magic/spells/fire/s1Tex",
    "__OTR__custom/medallion_magic/spells/fire/s2Tex",
};
static const char* paths[] = {
    "__OTR__overlays/ovl_Magic_Fire/sMaterialDL",
    "__OTR__overlays/ovl_Magic_Fire/sModelDL",
    "__OTR__overlays/ovl_Magic_Fire/sTex",
};

bool ResourceMgr_IsAltAssetsEnabled(void) {
    return alt;
}
uint8_t ResourceMgr_FileAltExists(const char* path) {
    REQUIRE(medallion && alt);
    for (int i = 0; i < 2; ++i) {
        if (!strcmp(path, privatePaths[i])) {
            ++privateQueries;
            return (privateAssets & (1 << i)) != 0;
        }
    }
    REQUIRE(!"unexpected private texture path");
    return 0;
}
void* ResourceGetDataByName(const char* path) {
    REQUIRE(medallion && alt && privateAssets == 3);
    for (int i = 0; i < 2; ++i) {
        if (!strcmp(path, privatePaths[i])) {
            ++privateLoads;
            return privateFailures & (1 << i) ? NULL : privatePixels[i];
        }
    }
    REQUIRE(!"unexpected private texture load");
    return NULL;
}
void* OotAssets_LoadGfx(const char* path) {
    int index = !strcmp(path, paths[0]) ? 0 : 1;
    REQUIRE(!strcmp(path, paths[index]));
    ++loads[index];
    return missing == index ? NULL : lists[alt][index];
}
void* OotAssets_LoadTexOrDList(const char* path) {
    REQUIRE(!strcmp(path, paths[2]));
    ++loads[2];
    return missing == 2 ? NULL : textures[alt];
}
Vtx* ResourceMgr_LoadVtxByName(char* path) {
    REQUIRE(!strcmp(path, "__OTR__overlays/ovl_Magic_Fire/sSphereVtx"));
    ++vertexLoads;
    return vertices[alt];
}
int32_t CVarGetInteger(const char* key, int32_t fallback) {
    REQUIRE(strstr(key, medallion ? ".MedallionFire" : ".Dins") != NULL);
    REQUIRE(strstr(key, ".Changed") != NULL);
    return customColors;
}
Color_RGBA8 CVarGetColor(const char* key, Color_RGBA8 fallback) {
    REQUIRE(strstr(key, medallion ? ".MedallionFire" : ".Dins") != NULL);
    REQUIRE(strstr(key, ".Color") != NULL);
    return strstr(key, "Primary") ? (Color_RGBA8){ 1, 2, 3, fallback.a } : (Color_RGBA8){ 4, 5, 6, fallback.a };
}
Color_RGBA8 CosmeticEditor_GetChangedColor(u8 r, u8 g, u8 b, u8 a, const char* id) {
    char key[160];
    Color_RGBA8 fallback = { r, g, b, a };
    snprintf(key, sizeof(key), "gCosmetic.%s.Changed", id);
    if (!CVarGetInteger(key, 0))
        return fallback;
    snprintf(key, sizeof(key), "gCosmetic.%s.Color", id);
    return CVarGetColor(key, fallback);
}
Color_RGB8 CVarGetColor24(const char* key, Color_RGB8 fallback) {
    REQUIRE(!"MM effects must read native RGBA .Color storage, never SoH RGB .Value");
    return fallback;
}
void Graph_OpenDisps(Gfx** refs, Gfx* vals, GraphicsContext* context, const char* file, s32 line) {
}
void Graph_CloseDisps(Gfx** refs, Gfx* vals, GraphicsContext* context, const char* file, s32 line) {
}
void FrameInterpolation_RecordOpenChild(const void* key, int id) {
}
void FrameInterpolation_RecordCloseChild(void) {
}
void Gfx_SetupDL25_Xlu(GraphicsContext* context) {
    gDPPipeSync(context->polyXlu.p++);
}
Gfx* Gfx_SetupDL57(Gfx* p) {
    gDPPipeSync(p++);
    return p;
}
void Matrix_Scale(f32 x, f32 y, f32 z, MatrixMode mode) {
    REQUIRE(x == 0.15f && y == 0.15f && z == 0.15f && mode == MTXMODE_APPLY);
}
Mtx* Matrix_Finalize(GraphicsContext* context) {
    return &matrix;
}
Gfx* Gfx_TwoTexScrollEx(GraphicsContext* context, s32 a, u32 b, u32 c, s32 d, s32 e, s32 f, u32 g, u32 h, s32 i, s32 j,
                        s32 k, s32 l, s32 m, s32 n) {
    REQUIRE(a == 0 && b == (play.gameplayFrames * 2) % 512 && c == 511 - (play.gameplayFrames * 5) % 512);
    REQUIRE(d == 64 && e == 64 && f == 1 && g == (play.gameplayFrames * 2) % 256);
    REQUIRE(h == 255 - (play.gameplayFrames * 20) % 256 && i == 32 && j == 32);
    REQUIRE(k == 2 && l == -5 && m == 2 && n == -20);
    ++scrollCount;
    return scroll;
}
void gSPDisplayList(Gfx* p, Gfx* dl) {
    __gSPDisplayList(p, dl);
}

static size_t draw(MagicFire* fire) {
    MagicFire before = *fire;
    memset(commands, 0, sizeof(commands));
    gfx.polyXlu.p = commands;
    MagicFireDins_Draw(&fire->actor, &play);
    REQUIRE(!memcmp(&before, fire, sizeof(before)));
    return gfx.polyXlu.p - commands;
}
static void checkI4Load(int index, int tile) {
    Gfx* load = &commands[index];
    static const unsigned ops[] = { G_SETTIMG,     G_SETTILE, G_RDPLOADSYNC, G_LOADBLOCK,
                                    G_RDPPIPESYNC, G_SETTILE, G_SETTILESIZE };
    for (int i = 0; i < ARRAY_COUNT(ops); ++i)
        REQUIRE((load[i].words.w0 >> 24) == ops[i]);
    REQUIRE(!strcmp((const char*)load[0].words.w1, privatePaths[tile]));
    REQUIRE(((load[0].words.w0 >> 21) & 7) == G_IM_FMT_I);
    REQUIRE(((load[0].words.w0 >> 19) & 3) == G_IM_SIZ_16b);
    REQUIRE(((load[1].words.w1 >> 24) & 7) == G_TX_LOADTILE);
    REQUIRE((load[1].words.w0 & 0x1FF) == tile * 0x100);
    REQUIRE(((load[3].words.w1 >> 12) & 0xFFF) == 1023); // 2048 bytes per 64x64 I4 sheet.
    REQUIRE((load[3].words.w1 & 0xFFF) == CALC_DXT_4b(64));
    REQUIRE(((load[5].words.w0 >> 21) & 7) == G_IM_FMT_I);
    REQUIRE(((load[5].words.w0 >> 19) & 3) == G_IM_SIZ_4b);
    REQUIRE(((load[5].words.w0 >> 9) & 0x1FF) == 4); // 32-byte row.
    REQUIRE((load[5].words.w0 & 0x1FF) == tile * 0x100);
    REQUIRE(((load[5].words.w1 >> 24) & 7) == tile);
    REQUIRE(((load[5].words.w1 >> 18) & 3) == G_TX_WRAP);
    REQUIRE(((load[5].words.w1 >> 8) & 3) == G_TX_WRAP);
    REQUIRE(((load[5].words.w1 >> 14) & 15) == 6);
    REQUIRE(((load[5].words.w1 >> 4) & 15) == 6);
    REQUIRE(((load[5].words.w1 >> 10) & 15) == (tile ? 14 : 0));
    REQUIRE((load[5].words.w1 & 15) == (tile ? 14 : 15));
    REQUIRE(((load[6].words.w1 >> 24) & 7) == tile);
    REQUIRE(((load[6].words.w1 >> 12) & 0xFFF) == 252);
    REQUIRE((load[6].words.w1 & 0xFFF) == 252);
}
static void checkBindings(MagicFire* fire) {
    int beforeLoads[3];
    memcpy(beforeLoads, loads, sizeof(loads));
    privateQueries = privateLoads = 0;
    size_t count = draw(fire);
    int usePrivate = medallion && alt && privateAssets == 3 && !privateFailures;
    int material = -1, model = -1, texture = -1, scrollIndex = -1;
    int privateIndices[2] = { -1, -1 }, textureCount = 0;
    for (size_t i = 0; i < count; ++i) {
        unsigned op = commands[i].words.w0 >> 24;
        uintptr_t p = commands[i].words.w1;
        if (op == G_SETTIMG) {
            REQUIRE(p && !(p & 1) && !strncmp((const char*)p, "__OTR__", 7));
            if (textureCount == 0) {
                REQUIRE(!strcmp((const char*)p, paths[2]));
                texture = i;
            } else {
                REQUIRE(usePrivate && textureCount <= 2);
                privateIndices[textureCount - 1] = i;
                checkI4Load(i, textureCount - 1);
            }
            ++textureCount;
        } else if (op == G_DL && p == (uintptr_t)scroll) {
            scrollIndex = i;
        } else if (op == G_DL && !(p & 1)) {
            REQUIRE(p && !strncmp((const char*)p, "__OTR__", 7));
            if (!strcmp((const char*)p, paths[0]))
                material = i;
            if (!strcmp((const char*)p, paths[1]))
                model = i;
        }
    }
    REQUIRE(texture >= 0 && material > texture && model == scrollIndex + 1);
    if (usePrivate) {
        REQUIRE(textureCount == 3 && privateIndices[0] == material + 1);
        REQUIRE(privateIndices[1] == material + 8 && scrollIndex == material + 15);
        REQUIRE(privateQueries == 2 && privateLoads == 2);
    } else {
        REQUIRE(textureCount == 1 && scrollIndex == material + 1);
        if (!medallion || !alt)
            REQUIRE(privateQueries == 0 && privateLoads == 0);
    }
    for (int i = 0; i < 3; ++i)
        REQUIRE(loads[i] == beforeLoads[i] + 1);
    for (int i = 0; i < 60; ++i) {
        REQUIRE(vertices[alt][sVertexIndices[i]].n.a == (u8)(s32)(fire->alphaMultiplier * (i < 36 ? 255 : 76)));
    }
    if (customColors) {
        int prim = 0, env = 0;
        for (size_t i = 0; i < count; ++i) {
            unsigned op = commands[i].words.w0 >> 24;
            if (op == G_SETPRIMCOLOR && (commands[i].words.w1 >> 8) == 0x010203)
                ++prim;
            if (op == G_SETENVCOLOR && (commands[i].words.w1 >> 8) == 0x040506)
                ++env;
        }
        REQUIRE(prim == 1 && env == 1);
    }
}
int main(void) {
    play.state.gfxCtx = &gfx;
    play.gameplayFrames = 42;
    missing = -1;
    for (medallion = 0; medallion < 2; ++medallion) {
        MagicFire fire = { 0 };
        fire.actor.id = medallion ? ACTOR_SW97_MAGIC_FIRE : ACTOR_OOT_DINS_FIRE;
        fire.action = 1;
        fire.alphaMultiplier = 0.75f;
        fire.screenTintIntensity = 0.5f;
        for (int frame = 0; frame < 4; ++frame) {
            alt = frame & 1;
            customColors = frame >= 2;
            checkBindings(&fire);
            fire.action = frame < 2 ? 2 : 3;
            fire.alphaMultiplier -= 0.125f;
        }
        for (int phase = 1; phase <= 3; ++phase) {
            fire.action = phase;
            fire.alphaMultiplier = (4 - phase) * 0.25f;
            for (alt = 0; alt < 2; ++alt) {
                for (customColors = 0; customColors < 2; ++customColors) {
                    privateAssets = privateFailures = 0;
                    size_t nativeCount = draw(&fire);
                    memcpy(reference, commands, sizeof(reference));
                    for (privateAssets = 0; privateAssets < 4; ++privateAssets) {
                        for (privateFailures = 0; privateFailures < 4; ++privateFailures) {
                            checkBindings(&fire);
                            if (!(medallion && alt && privateAssets == 3 && !privateFailures)) {
                                REQUIRE(gfx.polyXlu.p - commands == nativeCount);
                                REQUIRE(!memcmp(commands, reference, sizeof(commands)));
                            }
                        }
                    }
                }
            }
        }
        // Re-resolve presence/loading and Alt state while the same instance exists.
        alt = 1;
        privateAssets = 3;
        privateFailures = 0;
        checkBindings(&fire);
        privateFailures = 2;
        checkBindings(&fire);
        privateFailures = 0;
        privateAssets = 1;
        checkBindings(&fire);
        privateAssets = 3;
        checkBindings(&fire);
        alt = 0;
        checkBindings(&fire);
        alt = 1;
        checkBindings(&fire);
        for (missing = 0; missing < 3; ++missing) {
            int before = vertexLoads;
            REQUIRE(draw(&fire) == 0 && vertexLoads == before);
        }
        missing = -1;
        fire.action = 0;
        REQUIRE(draw(&fire) == 0);
    }
    puts("PASS: MM Din/Fire medallion named HD texture + display-list bindings, Alt/load freshness, "
         "independent optional Fire I4 pair, complete/missing/failed/live-toggle fallbacks, Din isolation, "
         "live colors, native sphere scroll/fades and draw-only gameplay preservation");
    return 0;
}
