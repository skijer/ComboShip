// Exercise MM's production tornado spawn and native dust init/update/draw.
// Resource, arena, RNG and matrix fixtures do not constitute runtime visual proof.
#include "global.h"
#include "mods/nei_oot_compat.h"
#include "2s2h/BenPort.h"
#include "2s2h/BenGui/CosmeticEditor.h"
#include <libultraship/bridge/consolevariablebridge.h>
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
#include "overlays/effects/ovl_Effect_Ss_Dust/z_eff_ss_dust.c"
#include "forest_tornado_production.inc"
#include "expansions/sw97/actors/spells/sw97_spell_appearance.h"

static GraphicsContext gfx;
static PlayState play;
static Gfx commands[128], reference[128], scrolls[4][1], allocations[4][16];
static Mtx matrix;
Mtx gIdentityMtx;
static int failedLoads;
static int alt, resources, queries, allocCount, failAllocation, scrollCount;
static s32 scrollArgs[4][10];
static EffectSsDustInitParams spawned[26];
static EffectSs particles[26];
static int spawnCount;
static int changedColors;
static Color_RGB8 customColors[6] = {
    { 11, 22, 33 }, { 44, 55, 66 }, { 77, 88, 99 }, { 111, 122, 133 }, { 144, 155, 166 }, { 177, 188, 199 },
};
static const char* colorKeys[6][2] = {
    { "gCosmetic.Magic.MedallionFirePrimary.Changed", "gCosmetic.Magic.MedallionFirePrimary.Color" },
    { "gCosmetic.Magic.MedallionFireSecondary.Changed", "gCosmetic.Magic.MedallionFireSecondary.Color" },
    { "gCosmetic.Magic.MedallionWaterPrimary.Changed", "gCosmetic.Magic.MedallionWaterPrimary.Color" },
    { "gCosmetic.Magic.MedallionWaterSecondary.Changed", "gCosmetic.Magic.MedallionWaterSecondary.Color" },
    { "gCosmetic.Magic.MedallionForestPrimary.Changed", "gCosmetic.Magic.MedallionForestPrimary.Color" },
    { "gCosmetic.Magic.MedallionForestSecondary.Changed", "gCosmetic.Magic.MedallionForestSecondary.Color" },
};
int32_t CVarGetInteger(const char* name, int32_t fallback) {
    for (int i = 0; i < 6; ++i) {
        if (!strcmp(name, colorKeys[i][0])) {
            return (changedColors & (1 << i)) != 0;
        }
    }
    REQUIRE(!"unexpected cosmetic setting");
    return fallback;
}
Color_RGBA8 CVarGetColor(const char* name, Color_RGBA8 fallback) {
    for (int i = 0; i < 6; ++i) {
        if (!strcmp(name, colorKeys[i][1])) {
            REQUIRE(changedColors & (1 << i));
            return (Color_RGBA8){ customColors[i].r, customColors[i].g, customColors[i].b, fallback.a };
        }
    }
    REQUIRE(!"unexpected cosmetic value");
    return fallback;
}
static uintptr_t rgba(Color_RGB8 rgb, u8 alpha) {
    return (uintptr_t)rgb.r << 24 | (uintptr_t)rgb.g << 16 | (uintptr_t)rgb.b << 8 | alpha;
}

static const char* privatePaths[] = {
    "__OTR__custom/medallion_magic/spells/fire/s1Tex",      "__OTR__custom/medallion_magic/spells/fire/s2Tex",
    "__OTR__custom/medallion_magic/spells/water/sTex",      "__OTR__custom/medallion_magic/spells/forest/sTex",
    "__OTR__custom/medallion_magic/spells/forest/dust1Tex", "__OTR__custom/medallion_magic/spells/forest/dust2Tex",
    "__OTR__custom/medallion_magic/spells/forest/dust3Tex", "__OTR__custom/medallion_magic/spells/forest/dust4Tex",
    "__OTR__custom/medallion_magic/spells/forest/dust5Tex", "__OTR__custom/medallion_magic/spells/forest/dust6Tex",
    "__OTR__custom/medallion_magic/spells/forest/dust7Tex", "__OTR__custom/medallion_magic/spells/forest/dust8Tex",
};

// Model the native editor's .Changed/.Color storage boundary, preserving draw alpha.
Color_RGBA8 CosmeticEditor_GetChangedColor(u8 r, u8 g, u8 b, u8 a, const char* id) {
    char key[160];
    Color_RGBA8 fallback = { r, g, b, a };
    snprintf(key, sizeof(key), "gCosmetic.%s.Changed", id);
    if (!CVarGetInteger(key, 0))
        return fallback;
    snprintf(key, sizeof(key), "gCosmetic.%s.Color", id);
    return CVarGetColor(key, fallback);
}
Color_RGB8 CVarGetColor24(const char* name, Color_RGB8 fallback) {
    REQUIRE(!"MM effects must read native RGBA .Color storage, never SoH RGB .Value");
    return fallback;
}
bool ResourceMgr_IsAltAssetsEnabled(void) {
    return alt;
}
uint8_t ResourceMgr_FileAltExists(const char* path) {
    REQUIRE(alt);
    for (int i = 0; i < ARRAY_COUNT(privatePaths); ++i) {
        if (!strcmp(path, privatePaths[i])) {
            ++queries;
            return (resources & (1 << i)) != 0;
        }
    }
    REQUIRE(!"unexpected private resource name");
    return 0;
}
void* ResourceGetDataByName(const char* path) {
    for (int i = 0; i < ARRAY_COUNT(privatePaths); ++i) {
        if (!strcmp(path, privatePaths[i]))
            return (failedLoads & (1 << i)) ? NULL : (void*)path;
    }
    REQUIRE(!"unexpected resource load");
    return NULL;
}
void Graph_OpenDisps(Gfx** refs, Gfx* values, GraphicsContext* context, const char* file, s32 line) {
}
void Graph_CloseDisps(Gfx** refs, Gfx* values, GraphicsContext* context, const char* file, s32 line) {
}
void* Lib_SegmentedToVirtual(void* ptr) {
    return ptr;
}
void* Graph_Alloc(GraphicsContext* context, size_t size) {
    REQUIRE(context == &gfx && allocCount < 4 && size <= sizeof(allocations[0]));
    return failAllocation ? NULL : allocations[allocCount++];
}
Gfx* Gfx_TwoTexScroll(GraphicsContext* context, s32 a, u32 b, u32 c, s32 d, s32 e, s32 f, u32 g, u32 h, s32 i, s32 j) {
    s32 args[] = { a, b, c, d, e, f, g, h, i, j };
    REQUIRE(context == &gfx && scrollCount < 4);
    memcpy(scrollArgs[scrollCount], args, sizeof(args));
    gSPEndDisplayList(scrolls[scrollCount]);
    return scrolls[scrollCount++];
}
void gSPSegment(void* command, int segment, uintptr_t target) {
    __gSPSegment((Gfx*)command, segment, target);
}
void gSPDisplayList(Gfx* command, Gfx* list) {
    __gSPDisplayList(command, list);
}
Gfx* Gfx_SetupDL(Gfx* command, u32 index) {
    gDPPipeSync(command++);
    return command;
}
void FrameInterpolation_RecordOpenChild(const void* key, int id) {
}
void FrameInterpolation_RecordCloseChild(void) {
}
void lusprintf(const char* file, int32_t line, int32_t level, const char* fmt, ...) {
}
void Matrix_Push(void) {
}
void Matrix_Pop(void) {
}
void Matrix_TranslateRotateZYX(Vec3f* pos, Vec3s* rot) {
}
void Matrix_Scale(f32 x, f32 y, f32 z, MatrixMode mode) {
}
Mtx* Matrix_Finalize(GraphicsContext* context) {
    return &matrix;
}
void SkinMatrix_SetTranslate(MtxF* mf, f32 x, f32 y, f32 z) {
}
void SkinMatrix_SetScale(MtxF* mf, f32 x, f32 y, f32 z) {
}
void SkinMatrix_MtxFMtxFMult(MtxF* a, MtxF* b, MtxF* out) {
}
Mtx* SkinMatrix_MtxFToNewMtx(GraphicsContext* context, MtxF* mf) {
    return &matrix;
}
void Math_Vec3f_Copy(Vec3f* dest, Vec3f* src) {
    *dest = *src;
}
f32 Rand_ZeroFloat(f32 max) {
    return 0;
}
f32 Rand_ZeroOne(void) {
    return 0;
}
void EffectSs_Spawn(PlayState* context, s32 type, s32 priority, void* params) {
    REQUIRE(context == &play && type == EFFECT_SS_DUST && priority == 128 && spawnCount < 26);
    spawned[spawnCount] = *(EffectSsDustInitParams*)params;
    u32 initialized = EffectSsDust_Init(context, spawnCount, &particles[spawnCount], params);
    REQUIRE(initialized == 1);
    ++spawnCount;
}

static unsigned opcode(const Gfx* command) {
    return (command->words.w0 >> 24) & 0xFF;
}
static void resetDraw(void) {
    memset(commands, 0, sizeof(commands));
    memset(allocations, 0, sizeof(allocations));
    memset(scrollArgs, 0, sizeof(scrollArgs));
    gfx.polyXlu.p = commands;
    allocCount = scrollCount = queries = 0;
}

static uintptr_t drawDust(EffectSs* effect, size_t* count) {
    const EffectSs before = *effect;
    resetDraw();
    EffectSsDust_Draw(&play, 0, effect);
    REQUIRE(!memcmp(effect, &before, sizeof(before)));
    *count = gfx.polyXlu.p - commands;
    for (size_t i = 0; i < *count; ++i) {
        if (opcode(&commands[i]) == G_MOVEWORD) {
            REQUIRE((commands[i].words.w0 & 0xFFFF) == 8 * 4);
            uintptr_t texture = commands[i].words.w1;
            commands[i].words.w1 = 0;
            return texture;
        }
    }
    REQUIRE(!"dust texture segment missing");
    return 0;
}

static void testForestDustIsolation(void) {
    const char* native[] = { gEffDust1Tex, gEffDust2Tex, gEffDust3Tex, gEffDust4Tex,
                             gEffDust5Tex, gEffDust6Tex, gEffDust7Tex, gEffDust8Tex };
    // Preserve each existing low-bit draw mode, even when the new bit is set.
    for (int flags = 0; flags < 8; ++flags) {
        EffectSsDustInitParams init = { .pos = { 1, 2, 3 },
                                        .primColor = { 220, 250, 255, 220 },
                                        .envColor = { 150, 200, 230, 140 },
                                        .scale = 200,
                                        .scaleStep = 30,
                                        .life = 16,
                                        .drawFlags = flags,
                                        .updateMode = 0 };
        EffectSs dust = { 0 };
        EffectSsDust_Init(&play, 0, &dust, &init);
        for (int frame = 0; frame < 8; ++frame) {
            dust.rTexIndex = frame;
            dust.rDrawFlags = flags;
            alt = 1;
            resources = 0xFFF;
            size_t count, current;
            uintptr_t chosen = drawDust(&dust, &count);
            REQUIRE(chosen == (uintptr_t)native[frame] && queries == 0);
            memcpy(reference, commands, count * sizeof(Gfx));
            dust.rDrawFlags |= 0x100;
            // Removing any single animation frame falls back as one set.
            for (int missing = -1; missing < 8; ++missing) {
                resources = missing < 0 ? 0xFF0 : (0xFF0 & ~(1 << (missing + 4)));
                chosen = drawDust(&dust, &current);
                REQUIRE(!strcmp((char*)chosen, missing < 0 ? privatePaths[frame + 4] : native[frame]));
                REQUIRE(current == count && !memcmp(reference, commands, count * sizeof(Gfx)));
            }
            resources = 0xFFF;
            for (int failed = 0; failed < 8; ++failed) {
                failedLoads = 1 << (failed + 4);
                chosen = drawDust(&dust, &current);
                REQUIRE(chosen == (uintptr_t)native[frame]);
                REQUIRE(current == count && !memcmp(reference, commands, count * sizeof(Gfx)));
            }
            failedLoads = 0;
            alt = 0;
            resources = 0xFFF;
            chosen = drawDust(&dust, &current);
            REQUIRE(chosen == (uintptr_t)native[frame] && queries == 0);
            REQUIRE(current == count && !memcmp(reference, commands, count * sizeof(Gfx)));
            alt = 1;
            chosen = drawDust(&dust, &current);
            REQUIRE(!strcmp((char*)chosen, privatePaths[frame + 4]));
        }
    }
}

static void testForestDustColors(void) {
    EffectSsDustInitParams init = { .primColor = { 220, 250, 255, 220 },
                                    .envColor = { 150, 200, 230, 140 },
                                    .scale = 200,
                                    .life = 16,
                                    .drawFlags = 0x100 };
    EffectSs dust = { 0 };
    EffectSsDust_Init(&play, 0, &dust, &init);
    for (int tagged = 0; tagged < 2; ++tagged) {
        dust.rDrawFlags = tagged ? 0x100 : 0;
        changedColors = 0;
        alt = 0;
        resources = 0;
        size_t count;
        drawDust(&dust, &count);
        memcpy(reference, commands, count * sizeof(Gfx));
        for (int mask = 0; mask < 4; ++mask) {
            changedColors = mask << 4;
            for (int mode = 0; mode < 3; ++mode) {
                alt = mode != 0;
                resources = mode == 2 ? 0xFFF : 0;
                size_t current;
                drawDust(&dust, &current);
                REQUIRE(current == count);
                int colors = 0;
                for (size_t i = 0; i < count; ++i) {
                    if (opcode(&commands[i]) == G_SETPRIMCOLOR) {
                        REQUIRE(commands[i].words.w1 ==
                                rgba(tagged && (mask & 1) ? customColors[4] : (Color_RGB8){ 220, 250, 255 }, 255));
                        commands[i].words.w1 = reference[i].words.w1;
                        ++colors;
                    } else if (opcode(&commands[i]) == G_SETENVCOLOR) {
                        REQUIRE(commands[i].words.w1 ==
                                rgba(tagged && (mask & 2) ? customColors[5] : (Color_RGB8){ 150, 200, 230 }, 140));
                        commands[i].words.w1 = reference[i].words.w1;
                        ++colors;
                    }
                }
                REQUIRE(colors == 2 && !memcmp(reference, commands, count * sizeof(Gfx)));
                ++customColors[4].g;
                ++customColors[5].b;
            }
        }
    }
    changedColors = 0;
}

static void testTornadoSpawnAndLifetime(void) {
    Vec3f center = { 10, 20, 30 };
    spawnCount = 0;
    alt = 0;
    MagicWind_SpawnTornadoVFX(&play, &center, 200);
    REQUIRE(spawnCount == 26);
    for (int i = 0; i < 26; ++i) {
        const int ring = i < 12, streak = i >= 20;
        const Vec3f pos = { ring ? 160 : streak ? 110 : 10, 20, 30 };
        const Vec3f velocity = { streak ? 1.5f : 0, ring ? 5 : streak ? 22 : 14, ring ? 12 : streak ? 0 : 6 };
        const Vec3f accel = { 0, streak ? 0 : 0.3f, 0 };
        const Color_RGBA8 prim = streak ? (Color_RGBA8){ 255, 255, 255, 240 } : (Color_RGBA8){ 220, 250, 255, 220 };
        const Color_RGBA8 env = streak ? (Color_RGBA8){ 180, 220, 240, 160 } : (Color_RGBA8){ 150, 200, 230, 140 };
        REQUIRE(spawned[i].pos.x == pos.x && spawned[i].pos.y == pos.y && spawned[i].pos.z == pos.z);
        REQUIRE(spawned[i].velocity.x == velocity.x && spawned[i].velocity.y == velocity.y &&
                spawned[i].velocity.z == velocity.z);
        REQUIRE(!memcmp(&spawned[i].accel, &accel, sizeof(accel)));
        REQUIRE(!memcmp(&spawned[i].primColor, &prim, sizeof(prim)));
        REQUIRE(!memcmp(&spawned[i].envColor, &env, sizeof(env)));
        REQUIRE(spawned[i].scale == (streak ? 220 : 200));
        REQUIRE(spawned[i].scaleStep == (streak ? 60 : 30));
        REQUIRE(spawned[i].life == (ring ? 16 : streak ? 22 : 18));
        REQUIRE(spawned[i].drawFlags == 0x100 && spawned[i].updateMode == 0);
        EffectSsDustInitParams controlInit = spawned[i];
        EffectSs control = { 0 };
        controlInit.drawFlags = 0;
        EffectSsDust_Init(&play, i, &control, &controlInit);
        // The new bit is the only permitted difference after init and updates.
        while (control.life > 0) {
            particles[i].update(&play, i, &particles[i]);
            control.update(&play, i, &control);
            EffectSs normalized = particles[i];
            normalized.rDrawFlags &= ~0x100;
            REQUIRE(!memcmp(&normalized, &control, sizeof(control)));
            alt = control.life & 1;
            resources = 0xFF0;
            size_t count;
            uintptr_t chosen = drawDust(&particles[i], &count);
            REQUIRE(!strcmp((char*)chosen,
                            alt ? privatePaths[control.rTexIndex + 4]
                                : (const char*[]){ gEffDust1Tex, gEffDust2Tex, gEffDust3Tex, gEffDust4Tex, gEffDust5Tex,
                                                   gEffDust6Tex, gEffDust7Tex, gEffDust8Tex }[control.rTexIndex]));
            --particles[i].life;
            --control.life;
        }
    }
}

static void testSpellAppearanceFallback(void) {
    Gfx arena[32], originalScroll[1];
    Color_RGBA8 primary = { 11, 22, 33, 44 }, secondary = { 55, 66, 77, 88 };
    for (int element = 2; element <= 3; ++element) {
        for (int mode = 0; mode < 4; ++mode) {
            alt = mode != 0;
            resources = mode >= 2 ? 1 << element : 0;
            failedLoads = mode == 3 ? 1 << element : 0;
            gfx.polyOpa.d = arena + ARRAY_COUNT(arena);
            Gfx* wrapper = Sw97_SpellScrollWithAppearance(&gfx, originalScroll, privatePaths[element],
                                                          privatePaths[element], G_IM_SIZ_8b, NULL, NULL);
            if (mode != 2)
                REQUIRE(wrapper == originalScroll);
            gfx.polyOpa.d = arena + ARRAY_COUNT(arena);
            wrapper = Sw97_SpellScrollWithAppearance(&gfx, originalScroll, privatePaths[element], privatePaths[element],
                                                     G_IM_SIZ_8b, &primary, &secondary);
            REQUIRE(wrapper != originalScroll);
            int images = 0, colors = 0, links = 0;
            for (int i = 0; i < 15; ++i) {
                const unsigned op = opcode(&wrapper[i]);
                if (op == G_ENDDL)
                    break;
                if (op == G_SETTIMG) {
                    REQUIRE(wrapper[i].words.w1 == (uintptr_t)privatePaths[element]);
                    ++images;
                } else if (op == G_LOADBLOCK) {
                    REQUIRE(((wrapper[i].words.w1 >> 12) & 0xFFF) == 2047);
                } else if (op == G_SETPRIMCOLOR) {
                    REQUIRE(wrapper[i].words.w1 == 0x0B1621FF);
                    ++colors;
                } else if (op == G_SETENVCOLOR) {
                    REQUIRE(wrapper[i].words.w1 == 0x37424D00);
                    ++colors;
                } else if (op == G_DL) {
                    REQUIRE(wrapper[i].words.w1 == (uintptr_t)originalScroll);
                    ++links;
                }
            }
            REQUIRE(images == (mode == 2 ? 2 : 0) && colors == 2 && links == 1);
        }
    }
    failedLoads = 0;
}

int main(void) {
    play.state.gfxCtx = &gfx;
    play.state.frames = play.gameplayFrames = 42;
    testSpellAppearanceFallback();
    testForestDustIsolation();
    testForestDustColors();
    testTornadoSpawnAndLifetime();
    puts("PASS: MM forest dust complete-set selection, missing/failed art fallback, live colors, Alt toggles, "
         "native draw modes, tornado spawn/init/update/lifetime, Water/Forest spell texture fallbacks");
    return 0;
}
