#include "global.h"
#include "2s2h/Enhancements/Graphics/MMSummerAtmosphere.h"
#include "2s2h/Enhancements/Graphics/MMSummerAtmosphereState.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
#include <cmath>
#include "summer_native_camera.inc"

SaveContext gSaveContext;
RegEditor editor;
RegEditor* gRegEditor = &editor;
static int season = SEASON_SUMMER, enabled = 1, shafts = 0, rain = 0, fluffEnabled = 1, firefliesEnabled = 1;
static int matrices = 0, stack = 0, children = 0, scopes = 0, vertexLoads = 0;
static Gfx xlu[1024], opa[512];
static Mtx matrix;
static std::vector<const uint8_t*> textures;
static std::vector<Vec3f> translations;
static std::vector<float> particleAges;
static std::vector<unsigned> opacities;

extern "C" int32_t CVarGetInteger(const char* key, int32_t fallback) {
    if (std::strcmp(key, MM_SUMMER_CVAR("Enabled")) == 0)
        return enabled;
    if (std::strcmp(key, MM_SUMMER_CVAR("Sunbeams")) == 0)
        return shafts;
    if (std::strcmp(key, MM_SUMMER_CVAR("Fluff")) == 0)
        return fluffEnabled;
    if (std::strcmp(key, MM_SUMMER_CVAR("Fireflies")) == 0)
        return firefliesEnabled;
    return fallback;
}
extern "C" int MMWeather_SeasonForPlay(const PlayState*) {
    return season;
}
extern "C" int MMWeather_RainDensity() {
    return rain;
}
extern "C" int MMWeather_SeasonClearsRain() {
    return gSaveContext.save.day == 2 && season == SEASON_SUMMER;
}
extern "C" float MMWeather_Overcast() {
    return 0;
}
extern "C" float OTRGetAspectRatio() {
    return 16.0f / 9.0f;
}
extern "C" void FrameInterpolation_RecordOpenChild(const void* identity, int) {
    ++children;
    // The outer OPEN_DISPS identity is a filename; only its particle children
    // carry Particle identities.
    if (children == 2)
        particleAges.push_back(static_cast<const MMSummer::Particle*>(identity)->age);
}
extern "C" void FrameInterpolation_RecordCloseChild() {
    assert(children > 0);
    --children;
}
extern "C" void Graph_OpenDisps(Gfx**, Gfx*, GraphicsContext*, const char*, s32) {
    ++scopes;
}
extern "C" void Graph_CloseDisps(Gfx**, Gfx*, GraphicsContext*, const char*, s32) {
    assert(scopes > 0);
    --scopes;
}
extern "C" void Gfx_SetupDL25_Xlu(GraphicsContext*) {
}
extern "C" void Matrix_Push() {
    ++stack;
}
extern "C" void Matrix_Pop() {
    assert(stack > 0);
    --stack;
}
extern "C" void Matrix_Translate(f32 x, f32 y, f32 z, MatrixMode) {
    translations.push_back({ x, y, z });
}
extern "C" void Matrix_Mult(MtxF*, MatrixMode) {
}
extern "C" void Matrix_Scale(f32 x, f32 y, f32, MatrixMode) {
    assert(x > 0 && y > 0);
}
extern "C" Mtx* Matrix_Finalize(GraphicsContext*) {
    ++matrices;
    return &matrix;
}
extern "C" void gSPDisplayList(Gfx* pkt, Gfx* list) {
    __gSPDisplayList(pkt, list);
}
extern "C" void gSPVertex(Gfx* pkt, uintptr_t address, int count, int start) {
    assert(count == 12 && start == 0);
    ++vertexLoads;
    __gSPVertex(pkt, address, count, start);
}

static size_t Draw(PlayState& play, bool expectTexture = true) {
    auto& gfx = *play.state.gfxCtx;
    gfx.polyXlu.p = xlu;
    gfx.polyOpa.p = opa;
    gfx.polyOpa.d = opa + 512;
    matrices = vertexLoads = 0;
    textures.clear();
    translations.clear();
    particleAges.clear();
    opacities.clear();
    MMSummerAtmosphere_Draw(&play);
    const size_t commands = gfx.polyXlu.p - xlu;
    assert(commands < 520);  // 128 dusk particles, two texture binds, and optional shafts.
    assert(matrices <= 129); // Fixed 32 mote + 96 firefly budget and one optional beam transform.
    assert(stack == 0 && children == 0 && scopes == 0);
    bool privateIA = false, depthTest = false;
    for (const Gfx* p = xlu; p < gfx.polyXlu.p; ++p) {
        const unsigned opcode = p->words.w0 >> 24;
        if (opcode == G_SETPRIMCOLOR)
            opacities.push_back(p->words.w1 & 0xFF);
        if (opcode == G_SETTIMG) {
            privateIA = ((p->words.w0 >> 21) & 7) == G_IM_FMT_IA;
            textures.push_back(reinterpret_cast<const uint8_t*>(p->words.w1));
        }
        if (opcode == G_SETOTHERMODE_L && (p->words.w1 & Z_CMP))
            depthTest = true;
    }
    if (expectTexture && matrices)
        assert(privateIA && depthTest);
    return commands;
}

int main() {
    static PlayState play{};
    static Camera camera{};
    static GraphicsContext gfx{};
    play.state.gfxCtx = &gfx;
    play.cameraPtrs[0] = &camera;
    play.view.up = { 0, 1, 0 };
    camera.eye = { 0, 100, 0 };
    camera.at = { 0, 100, 1000 };
    camera.fov = 60;
    play.envCtx.sunPos = { 450, 800, -250 };
    gSaveContext.save.time = CLOCK_TIME(12, 0);
    R_UPDATE_RATE = 3;
    for (int i = 0; i < 30; ++i)
        MMSummerAtmosphere_Update(&play);
    Draw(play);
    assert(matrices == 32 && vertexLoads == 0);
    // A dandelion seed has a tuft and trailing stem, rather than the
    // vertically symmetric glow used by a firefly. Catch a shared-glow fallback.
    assert(textures.size() == 1);
    const auto* fluff = textures[0];
    bool asymmetric = false;
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 16; ++x)
            asymmetric |= fluff[y * 16 + x] != fluff[(15 - y) * 16 + x];
    assert(asymmetric);
    // Actual guLookAtF screen coordinates must agree with the spawn grid:
    // a seed in the leftmost column must appear at screen left, not mirrored.
    float cameraMatrix[4][4];
    guLookAtF(cameraMatrix, 0, 100, 0, 0, 100, 1000, 0, 1, 0);
    auto screenX = [&](Vec3f p) {
        const float x =
            p.x * cameraMatrix[0][0] + p.y * cameraMatrix[1][0] + p.z * cameraMatrix[2][0] + cameraMatrix[3][0];
        const float z =
            p.x * cameraMatrix[0][2] + p.y * cameraMatrix[1][2] + p.z * cameraMatrix[2][2] + cameraMatrix[3][2];
        return x / (-z * std::tan(camera.fov * 0.00872664626f) * OTRGetAspectRatio());
    };
    assert(screenX(translations[0]) < -0.60f && screenX(translations[7]) > 0.60f);
    shafts = 1;
    MMSummerAtmosphere_Update(&play);
    Draw(play);
    assert(matrices == 33 && vertexLoads == 3);
    assert(gfx.polyOpa.d == opa + 512 - 3 * sizeof(Vtx) * 12 / sizeof(Gfx));
    gSaveContext.save.time = CLOCK_TIME(23, 0);
    MMSummerAtmosphere_Update(&play);
    Draw(play);
    assert(matrices == 96 && vertexLoads == 0);
    assert(textures.size() == 1 && textures[0] != fluff);
    const auto* glow = textures[0];
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 16; ++x)
            assert(glow[y * 16 + x] == glow[(15 - y) * 16 + x]);
    // Count alone does not guarantee readable fireflies. At the supplied
    // clip's 10:15 pm, sample actual emitted color alpha over several pulses.
    // Exclude intentional entry/recycle and screen-edge fades from the floor.
    gSaveContext.save.time = CLOCK_TIME(22, 15);
    unsigned minimumOpacity = 255, maximumOpacity = 0;
    size_t glowSamples = 0;
    double totalOpacity = 0;
    for (int frame = 0; frame < 600; ++frame) {
        MMSummerAtmosphere_Update(&play);
        Draw(play);
        assert(opacities.size() == translations.size() && opacities.size() == particleAges.size());
        for (size_t i = 0; i < opacities.size(); ++i) {
            const auto& position = translations[i];
            const float depth = position.z - camera.eye.z;
            const float screenY = (position.y - camera.eye.y) / (depth * std::tan(camera.fov * 0.00872664626f));
            if (particleAges[i] < 1 || std::abs(screenX(position)) > 0.85f || std::abs(screenY) > 0.85f)
                continue;
            minimumOpacity = std::min(minimumOpacity, opacities[i]);
            maximumOpacity = std::max(maximumOpacity, opacities[i]);
            totalOpacity += opacities[i];
            ++glowSamples;
        }
    }
    assert(glowSamples > 16000);
    const double meanOpacity = totalOpacity / (glowSamples * 255.0);
    std::printf("Night firefly emitted opacity: min %.3f, mean %.3f, peak %.3f (%zu samples)\n", minimumOpacity / 255.0,
                meanOpacity, maximumOpacity / 255.0, glowSamples);
    std::fflush(stdout);
    assert(minimumOpacity >= 120); // Brighter dim phase without discarding smooth pulse/fades.
    assert(meanOpacity >= 0.70);
    assert(maximumOpacity >= 250 && maximumOpacity <= 255); // Existing glow texture softens the brighter core.
    gSaveContext.save.time = CLOCK_TIME(18, 0);
    MMSummerAtmosphere_Update(&play);
    Draw(play);
    assert(textures.size() == 2 && textures[0] == fluff && textures[1] == glow);
    shafts = 0;
    MMSummerAtmosphere_Update(&play);
    Draw(play);
    const int duskParticles = matrices;
    // Independent live controls apply immediately, including while paused.
    play.pauseCtx.state = PAUSE_STATE_MAIN;
    fluffEnabled = 0;
    MMSummerAtmosphere_Update(&play);
    Draw(play);
    assert(textures.size() == 1 && textures[0] == glow && matrices > 0 && matrices <= 96);
    const int duskFireflies = matrices;
    firefliesEnabled = 0;
    MMSummerAtmosphere_Update(&play);
    Draw(play);
    assert(textures.empty() && matrices == 0);
    fluffEnabled = 1;
    MMSummerAtmosphere_Update(&play);
    Draw(play);
    assert(textures.size() == 1 && textures[0] == fluff && matrices > 0 && matrices <= 32);
    assert(matrices + duskFireflies == duskParticles);
    firefliesEnabled = 1;
    play.pauseCtx.state = PAUSE_STATE_OFF;
    // Shafts remain independent of both particle controls.
    gSaveContext.save.time = CLOCK_TIME(12, 0);
    fluffEnabled = firefliesEnabled = 0;
    shafts = 1;
    MMSummerAtmosphere_Update(&play);
    Draw(play, false);
    assert(textures.empty() && matrices == 1 && vertexLoads == 3);
    shafts = 0;
    MMSummerAtmosphere_Update(&play);
    Draw(play, false);
    assert(matrices == 0 && vertexLoads == 0);
    shafts = 1;
    fluffEnabled = firefliesEnabled = 1;
    gSaveContext.save.time = CLOCK_TIME(12, 0);
    rain = 30;
    MMSummerAtmosphere_Update(&play);
    Draw(play);
    assert(matrices == 0 && vertexLoads == 0);
    rain = 0;
    for (int value : { -1, SEASON_SPRING, SEASON_AUTUMN, SEASON_WINTER }) {
        season = value;
        MMSummerAtmosphere_Update(&play);
        assert(Draw(play, false) == 0);
    }
    season = SEASON_SUMMER;
    enabled = 0;
    MMSummerAtmosphere_Update(&play);
    assert(Draw(play, false) == 0);
    enabled = 1;
    shafts = 0;
    MMSummerAtmosphere_Reset();
    assert(Draw(play, false) == 0);
    MMSummerAtmosphere_Update(nullptr);
    MMSummerAtmosphere_Update(&play);
    Draw(play);
    assert(matrices == 0); // Fresh entries fade in rather than reusing stale sprites.
    std::puts("PASS summer renderer: private IA texture, depth testing, command budget, balanced scopes/matrices, "
              "shaft lifetime, night/rain/season gates, disable/reset/re-entry");
}
