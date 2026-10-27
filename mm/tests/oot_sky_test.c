#include "global.h"
#include "BenPort.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Resource lookup, settings, matrices and frame recording are service boundaries.
// The production Skybox_Init / Skybox_Draw and their actual GBI commands run unchanged.
static int sEnabled, sAlt, sComplete = 1, sFailures, sDraws, sLookups, sBuilds;
static const char* sMissing;
static u8 sShade = 255;
static Gfx sCommands[4096];
static GraphicsContext sGraphics;
static PlayState sPlay;
PlayState* gPlayState = &sPlay;
SaveContext gSaveContext;
static _Alignas(16) unsigned char sArena[256 * 1024];
static size_t sArenaUsed;

int32_t CVarGetInteger(const char* name, int32_t fallback) {
    return !strcmp(name, "gEnhancements.Graphics.UseOotSkyTextures") ? sEnabled : fallback;
}
bool ResourceMgr_IsAltAssetsEnabled(void) {
    return sAlt;
}
uint8_t ResourceMgr_FileExists(const char* name) {
    ++sLookups;
    return sComplete && (!sMissing || !strstr(name, sMissing));
}
uint8_t MMWeather_Shade(uint8_t value) {
    return (unsigned)value * sShade / 255;
}
// Renderer fixture supplies cloud policy explicitly. The weather bridge fixture
// exercises the real policy through this same production sky getter.
uint8_t MMWeather_ResolveOotCloudBlend(uint8_t nativeBlend) {
    return nativeBlend;
}
void* THA_AllocTailAlign16(TwoHeadArena* arena, size_t size) {
    size = (size + 15) & ~15;
    if (sArenaUsed + size > sizeof(sArena))
        abort();
    void* result = sArena + sArenaUsed;
    sArenaUsed += size;
    return result;
}
void Graph_OpenDisps(Gfx** refs, Gfx* vals, GraphicsContext* gfx, const char* file, s32 line) {
}
void Graph_CloseDisps(Gfx** refs, Gfx* vals, GraphicsContext* gfx, const char* file, s32 line) {
}
void Gfx_SetupDL40_Opa(GraphicsContext* gfx) {
}
void FrameInterpolation_RecordOpenChild(const void* a, int b) {
}
void FrameInterpolation_RecordCloseChild(void) {
}
int FrameInterpolation_GetCameraEpoch(void) {
    return 0;
}
void Matrix_Translate(f32 x, f32 y, f32 z, MatrixMode mode) {
}
void Matrix_Scale(f32 x, f32 y, f32 z, MatrixMode mode) {
}
void Matrix_RotateXFApply(f32 x) {
}
void Matrix_RotateYF(f32 y, MatrixMode mode) {
}
void Matrix_RotateZF(f32 z, MatrixMode mode) {
}
Mtx* Matrix_ToMtx(Mtx* dest) {
    return dest;
}
void gSPDisplayList(Gfx* pkt, Gfx* dl) {
    __gSPDisplayList(pkt, dl);
}
void gSPSegment(void* pkt, int segment, uintptr_t target) {
    __gSPSegment((Gfx*)pkt, segment, target);
}
void gSPVertex(Gfx* pkt, uintptr_t vertices, int count, int start) {
    ++sBuilds;
    __gSPVertex((Gfx*)pkt, vertices, count, start);
}

static void Check(int ok, const char* message) {
    if (!ok) {
        ++sFailures;
        fprintf(stderr, "FAIL %s\n", message);
    }
}
static void Init(void) {
    memset(&sPlay, 0, sizeof(sPlay));
    memset(&sGraphics, 0, sizeof(sGraphics));
    sArenaUsed = 0;
    sPlay.state.gfxCtx = &sGraphics;
    Skybox_Init(&sPlay.state, &sPlay.skyboxCtx, SKYBOX_NORMAL_SKY);
    // Production Environment_UpdateSkybox owns these weather slot bindings.
    memcpy(sPlay.skyboxCtx.staticSegments[0], sSkyboxTextures[0], sizeof(sSkyboxTextures[0]));
    memcpy(sPlay.skyboxCtx.staticSegments[1], sSkyboxTextures[0], sizeof(sSkyboxTextures[0]));
    Skybox_Calculate128(&sPlay.skyboxCtx, 5);
}
static void Draw(u16 time, int blend, int skyboxId) {
    memset(sCommands, 0, sizeof(sCommands));
    sGraphics.polyOpa.p = sCommands;
    sGraphics.polyOpa.d = sCommands + ARRAY_COUNT(sCommands);
    gSaveContext.skyboxTime = time;
    Skybox_Draw(&sPlay.skyboxCtx, &sGraphics, skyboxId, blend, 0, 0, 0);
    ++sDraws;
}
static int TextureCount(const char* name) {
    int count = 0;
    for (Gfx* g = sCommands; g < sGraphics.polyOpa.p; ++g) {
        if (((g->words.w0 >> 24) & 255) != G_DL)
            continue;
        Gfx* face = (Gfx*)g->words.w1;
        for (int i = 0; i < 300; ++i) {
            unsigned op = (face[i].words.w0 >> 24) & 255;
            if (op == G_ENDDL)
                break;
            if (op == G_SETTIMG && strstr((const char*)face[i].words.w1, name))
                ++count;
        }
    }
    return count;
}
static int ColorSeen(unsigned opcode, unsigned rgba) {
    for (Gfx* g = sCommands; g < sGraphics.polyOpa.p; ++g)
        if (((g->words.w0 >> 24) & 255) == opcode && g->words.w1 == rgba)
            return 1;
    return 0;
}

static void CheckPackManifest(const char* manifest) {
    if (manifest == NULL)
        return;
    FILE* file = fopen(manifest, "r");
    if (!file)
        abort();
    char contents[16384] = { 0 };
    fread(contents, 1, sizeof(contents) - 1, file);
    fclose(file);
    // Inspect the actual production-generated resource references for every
    // phase, checking that each emitted face exists in the supplied archive.
    static const u16 times[] = { CLOCK_TIME(5, 0), CLOCK_TIME(12, 0), CLOCK_TIME(17, 30), CLOCK_TIME(23, 0) };
    for (int time = 0; time < 4; ++time) {
        Draw(times[time], 128, SKYBOX_NORMAL_SKY);
        for (Gfx* g = sCommands; g < sGraphics.polyOpa.p; ++g) {
            if (((g->words.w0 >> 24) & 255) != G_DL)
                continue;
            Gfx* face = (Gfx*)g->words.w1;
            for (int i = 0; i < 300; ++i) {
                unsigned op = (face[i].words.w0 >> 24) & 255;
                if (op == G_ENDDL)
                    break;
                if (op == G_SETTIMG) {
                    const char* path = (const char*)face[i].words.w1;
                    Check(!strncmp(path, "__OTR__alt/", 11) && strstr(contents, path + 7),
                          "emitted OoT sky texture exists in inspected archive");
                }
            }
        }
    }
}

int main(int argc, char** argv) {
    sEnabled = sAlt = 1;
    Init();
    Gfx nativeLists[12][150];
    Vtx nativeVertices[5 * 32];
    memcpy(nativeLists, sPlay.skyboxCtx.dListBuf, sizeof(nativeLists));
    memcpy(nativeVertices, sPlay.skyboxCtx.roomVtx, sizeof(nativeVertices));
    Draw(CLOCK_TIME(12, 0), 0, SKYBOX_NORMAL_SKY);
    Check(TextureCount("alt/textures/vr_fine1_static/gDaySkybox") == 160,
          "complete OoT pack replaces all five clear-day faces");
    Check(!TextureCount("misc/d2_"), "OoT selection does not mix native sky faces");
    Check(ColorSeen(G_SETPRIMCOLOR, 0xFFFFFF00), "precolored sky uses neutral light, not MM palette tint");
    int lookups = sLookups, builds = sBuilds;
    Draw(CLOCK_TIME(12, 30), 0, SKYBOX_NORMAL_SKY);
    Check(sLookups == lookups && sBuilds == builds, "stable sky reuses lookups and display lists");

    Draw(CLOCK_TIME(7, 0), 0, SKYBOX_NORMAL_SKY);
    Check(TextureCount("gSunriseSkybox") == 80 && TextureCount("gDaySkybox") == 80,
          "dawn to day binds both time variants");
    Check(ColorSeen(G_SETPRIMCOLOR, 0xFFFFFF7F), "dawn transition interpolates at midpoint");
    Draw(CLOCK_TIME(17, 30), 0, SKYBOX_NORMAL_SKY);
    Check(TextureCount("gSunsetSkybox") == 160, "sunset sky reaches its full color");
    Draw(CLOCK_TIME(18, 30), 0, SKYBOX_NORMAL_SKY);
    Check(TextureCount("gSunsetSkybox") == 80 && TextureCount("gNightSkybox") == 80,
          "sunset to night binds both time variants");
    Draw(CLOCK_TIME(23, 0), 0, SKYBOX_NORMAL_SKY);
    Check(TextureCount("gNightSkybox") == 160, "night never falls back to daylight artwork");
    Draw(0xFFFF, 0, SKYBOX_NORMAL_SKY);
    Check(TextureCount("gNightSkybox") == 160, "last clock tick remains night");

    memcpy(sPlay.skyboxCtx.staticSegments[1], sSkyboxTextures[1], sizeof(sSkyboxTextures[1]));
    Draw(CLOCK_TIME(7, 0), 128, SKYBOX_NORMAL_SKY);
    Check(TextureCount("gSunriseSkybox") == 80 && TextureCount("gDaySkybox") == 80 &&
              TextureCount("gSunriseOvercastSkybox") == 80 && TextureCount("gDayOvercastSkybox") == 80,
          "time and cloud transitions coexist without dropping either blend");
    Check(ColorSeen(G_SETENVCOLOR, 0x80), "cloud overlay emits native weather blend opacity");
    Check(!memcmp(nativeLists, sPlay.skyboxCtx.dListBuf, sizeof(nativeLists)) &&
              !memcmp(nativeVertices, sPlay.skyboxCtx.roomVtx, sizeof(nativeVertices)),
          "OoT sky preserves native geometry and fallback display lists");
    CheckPackManifest(argc > 1 ? argv[1] : NULL);
    sShade = 180;
    Draw(CLOCK_TIME(12, 0), 255, SKYBOX_3);
    Check(TextureCount("gDayOvercastSkybox") == 160 && !TextureCount("gDaySkybox"),
          "fully overcast regional sky uses a single cloudy pass");
    Check(ColorSeen(G_SETPRIMCOLOR, 0xB4B4B400), "weather dimming is retained on full-color sky");

    memcpy(sPlay.skyboxCtx.staticSegments[0], sSkyboxTextures[1], sizeof(sSkyboxTextures[1]));
    memcpy(sPlay.skyboxCtx.staticSegments[1], sSkyboxTextures[0], sizeof(sSkyboxTextures[0]));
    Draw(CLOCK_TIME(12, 0), 64, SKYBOX_NORMAL_SKY);
    Check(ColorSeen(G_SETENVCOLOR, 0xBF), "reverse cloudy-to-clear fade preserves its weight");
    memcpy(sPlay.skyboxCtx.staticSegments[1], sSkyboxTextures[1], sizeof(sSkyboxTextures[1]));
    Draw(CLOCK_TIME(23, 0), 64, SKYBOX_NORMAL_SKY);
    Check(TextureCount("gNightOvercastSkybox") == 160 && !TextureCount("gNightSkybox"),
          "native all-cloudy sky remains cloudy independent of blend");

    sEnabled = 0;
    Draw(CLOCK_TIME(12, 0), 0, SKYBOX_NORMAL_SKY);
    Check(TextureCount("misc/d2_") == 160, "disabling option restores native sky");
    sEnabled = 1;
    sAlt = 0;
    Draw(CLOCK_TIME(12, 0), 0, SKYBOX_NORMAL_SKY);
    Check(TextureCount("misc/d2_") == 160, "Alternate Assets off restores native sky");
    sAlt = 1;
    Draw(CLOCK_TIME(12, 0), 0, SKYBOX_2);
    Check(!TextureCount("alt/textures/"), "special skies do not enter the OoT renderer");
    sMissing = "vr_cloud3_static/gNightOvercastSkybox5Tex";
    Init();
    Draw(CLOCK_TIME(12, 0), 0, SKYBOX_NORMAL_SKY);
    Check(TextureCount("misc/d2_") == 160, "partial pack falls back as a whole, even missing future phase");
    sMissing = NULL;
    sComplete = 0;
    Init();
    Draw(CLOCK_TIME(12, 0), 0, SKYBOX_NORMAL_SKY);
    Check(TextureCount("misc/d2_") == 160, "absent pack keeps native and MM replacement textures");
    sComplete = 1;
    sAlt = 0;
    Init();
    sAlt = 1;
    Draw(CLOCK_TIME(12, 0), 0, SKYBOX_NORMAL_SKY);
    Check(TextureCount("misc/d2_") == 160, "enabling after scene entry waits safely for reload");
    Init();
    Draw(CLOCK_TIME(12, 0), 0, SKYBOX_NORMAL_SKY);
    Check(TextureCount("gDaySkybox") == 160, "scene reentry adopts newly enabled complete pack");
    printf("MM OoT sky: %d production draws, %d failures\n", sDraws, sFailures);
    return sFailures != 0;
}
