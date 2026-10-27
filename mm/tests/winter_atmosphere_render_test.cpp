#if __has_include("2s2h/Enhancements/Graphics/MMWinterAtmosphere.h")
#include "global.h"
#include "ComboGameMode.h"
#include "2s2h/Enhancements/Audio/MMWeather.h"
#include "2s2h/Enhancements/Audio/MMWeatherAudio.h"
#include "2s2h/Enhancements/Graphics/MMWinterAtmosphere.h"
#include "2s2h/Enhancements/Graphics/MMWinterAtmosphereState.h"
#include "mods/extended_inventory.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

extern "C" {
int gComboGameMode = COMBO_GAME_BOTH;
}

SaveContext gSaveContext{};
u8 gWeatherMode;
static RegEditor editor{};
RegEditor* gRegEditor = &editor;
static std::unordered_map<std::string, int> settings;
static int scopes = 0, children = 0, stack = 0, matrices = 0, setupCalls = 0, floorQueries = 0;
static bool floorValid = true;
static float aspect = 16.0f / 9.0f;
static Gfx xlu[2048], opa[2048];
static Mtx matrix;
static Vec3f currentTranslation;
static const void* currentIdentity;
static int currentGeneration;
struct Mesh {
    std::vector<Vtx> vertices;
    Vec3f origin;
    const void* identity;
    int generation;
    float age;
};
static std::vector<Mesh> meshes;
static size_t Draw(PlayState& play);
#include "winter_native_setup.inc"
#include "winter_play_hooks.inc"

extern "C" int32_t CVarGetInteger(const char* key, int32_t fallback) {
    // Removing Snow independence or changing either default must fail this fixture.
    assert(std::strcmp(key, MM_WEATHER_CVAR("Winter.Snow")) != 0);
    if (std::strcmp(key, MM_WEATHER_CVAR("Winter.Mist")) == 0 ||
        std::strcmp(key, MM_WEATHER_CVAR("Winter.Aurora")) == 0)
        assert(fallback == 1);
    const auto found = settings.find(key);
    return found == settings.end() ? fallback : found->second;
}
extern "C" Color_RGBA8 CVarGetColor(const char*, Color_RGBA8 fallback) {
    return fallback;
}
extern "C" NeiSaveData* Nei_Save() {
    return &gSaveContext.save.shipSaveInfo.nei;
}
extern "C" u16 Nei_GetOwnedItem(u8 slot) {
    return Nei_Save()->ownedItems[slot - 24];
}
#include "winter_seasons.inc"
void MMAutumnSceneFoliage_Update(const PlayState*) {
}
void MMAutumnSceneFoliage_Reset() {
}
extern "C" void MMSummerAtmosphere_Reset() {
}
extern "C" void MMWeatherAudio_Reset() {
}
extern "C" void MMWeatherAudio_SetRain(float) {
}
extern "C" void MMWeatherAudio_Thunder(float) {
}
extern "C" void Audio_SetAmbienceChannelIO(u8, u8, u8) {
}
extern "C" void MMWeather_StartBolt() {
}
extern "C" void MMWeather_ClearBolts() {
}
extern "C" void Environment_DrawLightningFlash(PlayState*, u8, u8, u8, u8) {
}
extern "C" float OTRGetAspectRatio() {
    return aspect;
}
extern "C" void FrameInterpolation_RecordOpenChild(const void* identity, int generation) {
    ++children;
    if (children == 2) {
        currentIdentity = identity;
        currentGeneration = generation;
    }
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
extern "C" void Gfx_SetupDL25_Xlu(GraphicsContext* gfx) {
    ++setupCalls;
    __gSPDisplayList(gfx->polyXlu.p++, sNativeSetup25);
}
extern "C" void Matrix_Push() {
    ++stack;
}
extern "C" void Matrix_Pop() {
    assert(stack > 0);
    --stack;
}
extern "C" void Matrix_Translate(f32 x, f32 y, f32 z, MatrixMode) {
    currentTranslation = { x, y, z };
}
extern "C" Mtx* Matrix_Finalize(GraphicsContext*) {
    ++matrices;
    return &matrix;
}
extern "C" void gSPDisplayList(Gfx* pkt, Gfx* list) {
    __gSPDisplayList(pkt, list);
}
extern "C" void gSPVertex(Gfx* pkt, uintptr_t address, int count, int start) {
    assert((count == 15 || count == 24) && start == 0 && children == 2);
    const auto* vertices = reinterpret_cast<const Vtx*>(address);
    meshes.push_back({ { vertices, vertices + count },
                       currentTranslation,
                       currentIdentity,
                       currentGeneration,
                       count == 15 ? static_cast<const MMWinter::Wisp*>(currentIdentity)->age : 0 });
    __gSPVertex(pkt, address, count, start);
}
static float Terrain(Vec3f point) {
    return point.x * 0.04f + point.z * 0.02f;
}
extern "C" f32 BgCheck_AnyRaycastFloor1(CollisionContext*, CollisionPoly*, Vec3f* point) {
    assert(std::isfinite(point->x) && std::isfinite(point->y) && std::isfinite(point->z));
    ++floorQueries;
    return floorValid ? Terrain(*point) : BGCHECK_Y_MIN;
}

static size_t Draw(PlayState& play) {
    auto& gfx = *play.state.gfxCtx;
    gfx.polyXlu.p = xlu;
    gfx.polyOpa.p = opa;
    gfx.polyOpa.d = opa + 2048;
    matrices = setupCalls = 0;
    meshes.clear();
    const auto save = gSaveContext;
    const auto environment = play.envCtx;
    const auto actorLists = play.actorCtx;
    const auto collision = play.colCtx;
    const auto weather = gWeatherMode;
    const auto camera = *GET_ACTIVE_CAM(&play);
    DrawFromPlay(&play);
    assert(std::memcmp(&save, &gSaveContext, sizeof(save)) == 0);
    assert(std::memcmp(&environment, &play.envCtx, sizeof(environment)) == 0);
    assert(std::memcmp(&actorLists, &play.actorCtx, sizeof(actorLists)) == 0);
    assert(std::memcmp(&collision, &play.colCtx, sizeof(collision)) == 0);
    assert(std::memcmp(&camera, GET_ACTIVE_CAM(&play), sizeof(camera)) == 0 && gWeatherMode == weather);
    const size_t commands = gfx.polyXlu.p - xlu;
    assert(commands < 300 && matrices <= 13 && meshes.size() <= 15);
    size_t allocated = 0;
    for (const auto& mesh : meshes)
        allocated += ALIGN16(mesh.vertices.size() * sizeof(Vtx));
    assert(allocated <= 252 * sizeof(Vtx));
    assert(reinterpret_cast<const u8*>(opa + 2048) - reinterpret_cast<const u8*>(gfx.polyOpa.d) ==
           static_cast<ptrdiff_t>(allocated));
    assert(gfx.polyOpa.p == opa && scopes == 0 && children == 0 && stack == 0);
    if (!matrices) {
        assert(commands == 0 && setupCalls == 0);
        return commands;
    }
    bool textured = true, depthTest = false;
    uint32_t geometry = G_LIGHTING | G_FOG | G_CULL_BACK | G_CULL_FRONT;
    uint32_t otherHigh = 0, otherLow = 0;
    const Gfx shade = gsDPSetCombineMode(G_CC_SHADE, G_CC_SHADE);
    Gfx lastCombine{};
    auto inspect = [&](auto&& self, const Gfx* command) -> void {
        const auto opcode = command->words.w0 >> 24;
        assert(opcode != G_SETTIMG); // No skybox texture, native snow texture, or new resource dependency.
        if (opcode == G_DL && command->words.w1 == reinterpret_cast<uintptr_t>(sNativeSetup25))
            for (const auto& entry : sNativeSetup25)
                self(self, &entry);
        if (opcode == G_TEXTURE)
            textured = (command->words.w0 & 0xFF) != 0;
        if (opcode == G_SETCOMBINE)
            lastCombine = *command;
        if (opcode == G_GEOMETRYMODE)
            geometry = (geometry & (command->words.w0 & 0xFFFFFF)) | command->words.w1;
        if (opcode == G_RDPSETOTHERMODE) {
            otherHigh = command->words.w0 & 0xFFFFFF;
            otherLow = command->words.w1;
        }
        if (opcode == G_SETOTHERMODE_H || opcode == G_SETOTHERMODE_L) {
            const unsigned bits = (command->words.w0 & 0xFF) + 1;
            const unsigned shift = 32 - ((command->words.w0 >> 8) & 0xFF) - bits;
            const uint32_t mask = (0xFFFFFFFFu >> (32 - bits)) << shift;
            auto& mode = opcode == G_SETOTHERMODE_H ? otherHigh : otherLow;
            mode = (mode & ~mask) | (command->words.w1 & mask);
        }
        if (opcode == G_SETOTHERMODE_L && (command->words.w1 & Z_CMP)) {
            depthTest = true;
            assert(!(command->words.w1 & Z_UPD)); // Terrain/buildings occlude, sheets never hide later actors.
        }
        if (opcode == G_VTX || opcode == G_TRI2) {
            // Match the native Fast3D alpha, fog, clip and blend prerequisites
            // at submitted geometry, rather than only inspecting cleanup.
            assert(!textured && !(geometry & (G_LIGHTING | G_FOG | G_CULL_BACK | G_CULL_FRONT)));
            assert((geometry & (G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH)) == (G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH));
            assert(lastCombine.words.w0 == shade.words.w0 && lastCombine.words.w1 == shade.words.w1);
            assert((otherHigh & (3u << G_MDSFT_CYCLETYPE)) == G_CYC_1CYCLE);
            assert((otherLow & (G_AC_DITHER | G_ZS_PRIM | Z_UPD | CVG_X_ALPHA)) == 0);
            assert((otherLow & Z_CMP) && ((otherLow & (3u << 20)) == (G_BL_CLR_MEM << 20)) &&
                   ((otherLow & (3u << 16)) == (G_BL_1MA << 16)));
        }
    };
    for (const Gfx* command = xlu; command != gfx.polyXlu.p; ++command)
        inspect(inspect, command);
    assert(depthTest && textured && setupCalls == 2);
    assert(lastCombine.words.w0 == sNativeSetup25[2].words.w0 && lastCombine.words.w1 == sNativeSetup25[2].words.w1);
    return commands;
}
static void Advance(PlayState& play, int frames) {
    for (int i = 0; i < frames; ++i) {
        ++play.gameplayFrames;
        FrameFromNativePlayMain(&play);
    }
}
static size_t Count(size_t size) {
    return std::count_if(meshes.begin(), meshes.end(), [&](const Mesh& mesh) { return mesh.vertices.size() == size; });
}
static unsigned PeakAurora() {
    unsigned peak = 0;
    for (const auto& mesh : meshes)
        if (mesh.vertices.size() == 24)
            for (const auto& vertex : mesh.vertices)
                peak = std::max(peak, unsigned(vertex.v.cn[3]));
    return peak;
}
static MMWinter::View NativeCamera(PlayState& play, float pitch, int yaw) {
    const float p = pitch * 0.0174532925f, y = yaw * 0.0174532925f;
    MMWinter::View view{ { 0, 100, 0 },
                         { std::sin(y) * std::cos(p), -std::sin(p), std::cos(y) * std::cos(p) },
                         { -std::cos(y), 0, std::sin(y) },
                         { std::sin(y) * std::sin(p), std::cos(p), std::cos(y) * std::sin(p) },
                         0.5773503f,
                         aspect };
    auto* camera = GET_ACTIVE_CAM(&play);
    const auto at = view.eye + view.forward * 1000;
    play.view.eye = camera->eye = { view.eye.x, view.eye.y, view.eye.z };
    play.view.at = camera->at = { at.x, at.y, at.z };
    play.view.up = { view.up.x, view.up.y, view.up.z };
    play.view.fovy = camera->fov = 60;
    return view;
}
static MMWinter::Vec3 WorldVertex(const Mesh& mesh, size_t index) {
    const auto* p = mesh.vertices[index].v.ob;
    return { mesh.origin.x + p[0], mesh.origin.y + p[1], mesh.origin.z + p[2] };
}
static bool SubmittedCoreInView(const MMWinter::View& view) {
    for (const auto& mesh : meshes)
        if (mesh.vertices.size() == 24)
            for (size_t i = 0; i < mesh.vertices.size(); ++i) {
                const auto relative = WorldVertex(mesh, i) - view.eye;
                const float depth = MMSummer::Dot(relative, view.forward);
                if (mesh.vertices[i].v.cn[3] > 25 && depth > 0 &&
                    std::abs(MMSummer::Dot(relative, view.right)) < depth * view.tanHalfFov * view.aspect * 0.95f &&
                    std::abs(MMSummer::Dot(relative, view.up)) < depth * view.tanHalfFov * 0.95f)
                    return true;
            }
    return false;
}
static void MistReadableAgainstFrost(const MMWinter::View& view) {
    // Termina's accepted full frost colors are canopy CCDCEE and grass E5EDF3.
    // Model the native source-over blend from the actual submitted vertex
    // colors/alpha, and project the mesh into a 240-line native camera view.
    const unsigned frost[2][3] = { { 204, 220, 238 }, { 229, 237, 243 } };
    float peakContrast[2]{};
    float minHeight = 1000;
    size_t count = 0;
    for (const auto& mesh : meshes) {
        if (mesh.vertices.size() != 15)
            continue;
        ++count;
        const auto projectedY = [&](size_t index) {
            const auto relative = WorldVertex(mesh, index) - view.eye;
            return MMSummer::Dot(relative, view.up) / (MMSummer::Dot(relative, view.forward) * view.tanHalfFov);
        };
        const float height = 120 * std::abs(projectedY(12) - projectedY(2));
        minHeight = std::min(minHeight, height);
        assert(height >= 9 && "far ground mist must span more than a few native scanlines");
        const auto& color = mesh.vertices[7].v.cn;
        for (size_t palette = 0; palette < 2; ++palette) {
            float contrast = 0;
            for (size_t channel = 0; channel < 3; ++channel)
                contrast += std::abs(float(color[channel]) - frost[palette][channel]) * color[3] / (255 * 3);
            peakContrast[palette] = std::max(peakContrast[palette], contrast);
        }
        const auto bottom = WorldVertex(mesh, 2);
        assert(std::abs(bottom.y - Terrain({ bottom.x, bottom.y, bottom.z }) - 2) <= 0.6f);
    }
    assert(count >= 8);
    for (const float contrast : peakContrast)
        assert(contrast >= 6 && "ice-blue mist must remain distinguishable against full winter frost");
    std::printf("PASS MM mist projection/blend model: minimum %.2f native pixels, peak canopy/grass contrast %.2f/%.2f "
                "RGB levels\n",
                minHeight, peakContrast[0], peakContrast[1]);
}
static void SameMeshes(const std::vector<Mesh>& before) {
    assert(before.size() == meshes.size());
    for (size_t i = 0; i < before.size(); ++i) {
        assert(before[i].identity == meshes[i].identity && before[i].generation == meshes[i].generation);
        assert(std::memcmp(&before[i].origin, &meshes[i].origin, sizeof(Vec3f)) == 0);
        assert(before[i].age == meshes[i].age && before[i].vertices.size() == meshes[i].vertices.size());
        assert(std::memcmp(before[i].vertices.data(), meshes[i].vertices.data(),
                           before[i].vertices.size() * sizeof(Vtx)) == 0);
    }
}

static void NativePlayMainLifetime(PlayState& play) {
    const auto savedTime = gSaveContext.save.time;
    const auto savedRate = R_UPDATE_RATE;
    gSaveContext.save.time = CLOCK_TIME(23, 0);
    for (int rate : { 1, 2, 3 }) {
        R_UPDATE_RATE = rate;
        InitFromPlay();
        Advance(play, 90);
        std::printf("MM winter native Play_Main R_UPDATE_RATE=%d: mist meshes=%zu, aurora meshes=%zu, "
                    "aurora alpha=%u\n",
                    rate, Count(15), Count(24), PeakAurora());
        std::fflush(stdout);
        assert(Count(15) >= 8 && Count(24) == 3 && PeakAurora() > 25);
        for (const auto& mesh : meshes)
            if (mesh.vertices.size() == 15)
                assert(std::abs(mesh.age - 89 * rate / 60.0f) < 0.0002f);
        const auto before = meshes;
        const int queries = floorQueries;
        for (int i = 0; i < 10; ++i) {
            Draw(play);
            SameMeshes(before);
        }
        assert(floorQueries == queries);

        auto* gfx = play.state.gfxCtx;
        const auto xluEnd = gfx->polyXlu.p;
        const auto opaEnd = gfx->polyOpa.d;
        play.state.gfxCtx = nullptr;
        MMWinterAtmosphere_Draw(&play);
        assert(gfx->polyXlu.p == xluEnd && gfx->polyOpa.d == opaEnd && floorQueries == queries);
        play.state.gfxCtx = gfx;
        Draw(play);
        SameMeshes(before); // An unavailable draw context cannot reset the simulation.
    }
    gSaveContext.save.time = savedTime;
    R_UPDATE_RATE = savedRate;
    std::puts("PASS MM winter native Play_Main null-graphics update/restore/draw: mist ageing, night fade, "
              "all update rates, repeated draws and missing draw context preserve simulation");
}

int main() {
    static PlayState play{};
    static Camera camera{};
    static GraphicsContext gfx{};
    static Actor player{}, nativeSnow{};
    play.state.gfxCtx = &gfx;
    play.sceneId = SCENE_TOWN;
    play.skyboxId = SKYBOX_NORMAL_SKY;
    play.envCtx.lightSettingOverride = LIGHT_SETTING_OVERRIDE_NONE;
    play.cameraPtrs[0] = &camera;
    play.view.eye = camera.eye = { 0, 100, 0 };
    play.view.at = camera.at = { 0, 100, 1000 };
    play.view.up = { 0, 1, 0 };
    play.view.fovy = camera.fov = 60;
    play.view.zFar = 12800;
    player.world.pos = { 0, 0, 0 };
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player;
    nativeSnow.id = ACTOR_OBJECT_KANKYO;
    nativeSnow.params = 3;
    nativeSnow.update = reinterpret_cast<ActorFunc>(1);
    play.actorCtx.actorLists[ACTORCAT_ITEMACTION].first = &nativeSnow;
    settings[MM_WEATHER_CVAR("Season")] = SEASON_WINTER;
    settings[MM_WEATHER_CVAR("Winter.Snow")] = 0;
    gSaveContext.save.time = CLOCK_TIME(12, 0);
    R_UPDATE_RATE = 3;
    const auto playerBefore = player, snowBefore = nativeSnow;
    const auto saveBefore = gSaveContext;
    NativePlayMainLifetime(play);
    InitFromPlay();
    Advance(play, 30);
    assert(Count(15) >= 8 && Count(24) == 0 && matrices == static_cast<int>(Count(15)) && floorQueries > 0);
    for (const auto& mesh : meshes) {
        const float clearance = mesh.origin.y - Terrain(mesh.origin);
        assert(clearance >= 17 && clearance <= 51); // Cached terrain can move by a fraction between samples.
        for (size_t i = 0; i < mesh.vertices.size(); ++i) {
            const auto alpha = mesh.vertices[i].v.cn[3];
            assert(alpha <= 62);
            if (i / 5 != 1 || i % 5 == 0 || i % 5 == 4)
                assert(alpha == 0);
        }
    }
    const auto mist = meshes;
    MistReadableAgainstFrost(NativeCamera(play, 0, 0));
    for (float pitch : { 10.0f, 15.0f }) {
        const auto view = NativeCamera(play, pitch, 0);
        Draw(play);
        MistReadableAgainstFrost(view);
    }
    NativeCamera(play, 0, 0);
    Draw(play);
    SameMeshes(mist);
    const int queries = floorQueries;
    for (int i = 0; i < 20; ++i) {
        UpdateFromPlay(&play);
        Draw(play);
        SameMeshes(mist);
    }
    assert(floorQueries == queries); // Repeated render frames cannot age or re-query the terrain.
    for (int pause = 0; pause < 3; ++pause) {
        play.pauseCtx.state = pause == 0 ? PAUSE_STATE_MAIN : PAUSE_STATE_OFF;
        play.pauseCtx.debugEditor = pause == 1 ? DEBUG_EDITOR_INVENTORY_INIT : DEBUG_EDITOR_NONE;
        IREG(72) = pause == 2;
        Advance(play, 3);
        SameMeshes(mist);
        assert(floorQueries == queries);
    }
    play.pauseCtx.state = PAUSE_STATE_MAIN;
    play.pauseCtx.debugEditor = DEBUG_EDITOR_NONE;
    IREG(72) = 0;
    camera.eye.x += 20;
    camera.at.x += 20;
    Draw(play);
    for (const auto& mesh : meshes) {
        const auto old = std::find_if(mist.begin(), mist.end(),
                                      [&](const Mesh& previous) { return previous.identity == mesh.identity; });
        assert(old != mist.end());
        assert(std::memcmp(&old->origin, &mesh.origin, sizeof(Vec3f)) == 0);
    }
    camera.eye.x -= 20;
    camera.at.x -= 20;
    settings[MM_WEATHER_CVAR("Winter.Mist")] = 0;
    assert(Draw(play) == 0);
    settings[MM_WEATHER_CVAR("Winter.Mist")] = 1;
    Draw(play);
    SameMeshes(mist);
    play.pauseCtx.state = PAUSE_STATE_OFF;
    settings[MM_WEATHER_CVAR("Winter.Mist")] = 0;
    assert(Draw(play) == 0); // Aurora remains daylight-gated with snow and mist off.
    gSaveContext.save.time = CLOCK_TIME(23, 0);
    Advance(play, 30);
    assert(Count(24) == 3 && Count(15) == 0 && matrices == 1);
    bool blue = false, green = false;
    for (const auto& mesh : meshes)
        for (size_t i = 0; i < mesh.vertices.size(); ++i) {
            const auto& color = mesh.vertices[i].v.cn;
            blue |= color[3] && color[2] > color[1];
            green |= color[3] && color[1] > color[2];
            if (i / 8 != 1 || i % 8 == 0 || i % 8 == 7)
                assert(color[3] == 0);
        }
    assert(blue && green);
    const auto aurora = meshes;
    const unsigned fullNight = PeakAurora();
    for (const float nativeAspect : { 4.0f / 3.0f, 16.0f / 9.0f }) {
        aspect = nativeAspect;
        for (float pitch : { 10.0f, 15.0f })
            for (int yaw = 0; yaw < 360; yaw += 30) {
                const auto view = NativeCamera(play, pitch, yaw);
                Draw(play);
                assert(Count(24) == 3 && SubmittedCoreInView(view));
                assert(PeakAurora() == fullNight);
            }
    }
    aspect = 16.0f / 9.0f;
    NativeCamera(play, 0, 0);
    Draw(play);
    for (int hour : { 18, 6 }) {
        gSaveContext.save.time = CLOCK_TIME(hour, 0);
        assert(Draw(play) == 0);
    }
    for (int hour : { 19, 5 }) {
        gSaveContext.save.time = CLOCK_TIME(hour, 0);
        Draw(play);
        assert(PeakAurora() * 2 >= fullNight - 2 && PeakAurora() * 2 <= fullNight + 2);
    }
    for (int hour : { 20, 21, 22, 23, 0, 1, 2, 3, 4 }) {
        gSaveContext.save.time = CLOCK_TIME(hour, 0);
        Draw(play);
        assert(PeakAurora() == fullNight);
    }
    gSaveContext.save.time = CLOCK_TIME(23, 0);
    camera.at = { 1000, 100, 0 };
    Draw(play);
    SameMeshes(aurora); // Yaw cannot drag a compass ribbon around the screen.
    camera.at = { 0, 100, 1000 };
    play.view.zFar = 2000;
    Draw(play);
    assert(Count(24) == 3);
    for (const auto& mesh : meshes)
        for (const auto& vertex : mesh.vertices) {
            const auto* p = vertex.v.ob;
            assert(std::sqrt(float(p[0]) * p[0] + float(p[1]) * p[1] + float(p[2]) * p[2]) < 2000);
        }
    play.view.zFar = 200;
    assert(Draw(play) == 0);
    play.view.zFar = 12800;
    play.skyboxId = SKYBOX_3;
    assert(Draw(play) == 0); // MM's alternative outdoor backdrop must not acquire an aurora.
    play.skyboxId = SKYBOX_NORMAL_SKY;
    settings[MM_WEATHER_CVAR("Winter.Aurora")] = 0;
    assert(Draw(play) == 0);
    settings[MM_WEATHER_CVAR("Winter.Mist")] = 1;
    Draw(play);
    assert(Count(15) >= 8 && Count(24) == 0);
    settings[MM_WEATHER_CVAR("Winter.Aurora")] = 1;
    Draw(play);
    assert(Count(15) >= 8 && Count(24) == 3);
    const int oldWispGeneration = meshes[0].generation;
    const int oldAuroraGeneration = meshes.back().generation;
    for (int boundary = 0; boundary < 5; ++boundary) {
        switch (boundary) {
            case 0:
                ++play.roomCtx.curRoom.num;
                break;
            case 1:
                ++play.sceneId;
                break;
            case 2:
                play.gameplayFrames = 0;
                break;
            case 3:
                DestroyFromPlay();
                break;
            case 4:
                InitFromPlay();
                break;
        }
        Draw(play);
        assert(Count(15) == 0 && matrices == 0);
        Advance(play, 30);
        assert(Count(15) > 0 && Count(24) == 3);
        assert(meshes[0].generation != oldWispGeneration && meshes.back().generation != oldAuroraGeneration);
    }
    floorValid = false;
    ++play.roomCtx.curRoom.num;
    settings[MM_WEATHER_CVAR("Winter.Aurora")] = 0;
    Advance(play, 30);
    assert(Draw(play) == 0); // Missing collision cannot produce camera-height sheets.
    floorValid = true;
    settings[MM_WEATHER_CVAR("Winter.Aurora")] = 1;
    for (int gate = 0; gate < 11; ++gate) {
        play.sceneId = SCENE_TOWN;
        switch (gate) {
            case 0:
                play.csCtx.state = 1;
                break;
            case 1:
                play.envCtx.lightSettingOverride = 1;
                break;
            case 2:
                play.envCtx.customSkyboxFilter = true;
                break;
            case 3:
                camera.stateFlags = CAM_STATE_UNDERWATER;
                break;
            case 4:
                play.skyboxId = SKYBOX_NONE;
                break;
            case 5:
                play.envCtx.skyboxDisabled = true;
                break;
            case 6:
                play.sceneId = SCENE_SONCHONOIE;
                break;
            case 7:
                gSaveContext.gameMode = 1;
                break;
            case 8:
                play.gameOverCtx.state = 1;
                break;
            case 9:
                play.transitionTrigger = TRANS_TRIGGER_START;
                break;
            case 10:
                play.transitionMode = TRANS_MODE_INSTANCE_RUNNING;
                break;
        }
        assert(Draw(play) == 0);
        play.csCtx.state = CS_STATE_IDLE;
        play.envCtx.lightSettingOverride = LIGHT_SETTING_OVERRIDE_NONE;
        play.envCtx.customSkyboxFilter = play.envCtx.skyboxDisabled = false;
        play.skyboxId = SKYBOX_NORMAL_SKY;
        camera.stateFlags = 0;
        gSaveContext.gameMode = GAMEMODE_NORMAL;
        play.gameOverCtx.state = GAMEOVER_INACTIVE;
        play.transitionTrigger = TRANS_TRIGGER_OFF;
        play.transitionMode = TRANS_MODE_OFF;
    }
    play.sceneId = SCENE_TOWN;
    for (int season : { -1, SEASON_SPRING, SEASON_SUMMER, SEASON_AUTUMN }) {
        settings[MM_WEATHER_CVAR("Season")] = season;
        assert(Draw(play) == 0);
    }
    gSaveContext.save.shipSaveInfo.nei = {};
    gSaveContext.save.shipSaveInfo.nei.season = SEASON_OFF;
    settings[MM_WEATHER_CVAR("Season")] = -2;
    assert(Draw(play) == 0); // The actual Follow Game/Rod resolver cannot grant weather ownership.
    settings[MM_WEATHER_CVAR("Season")] = SEASON_WINTER;
    const auto unowned = gSaveContext.save.shipSaveInfo.nei;
    Advance(play, 30);
    assert(Count(15) > 0 && Count(24) == 3);
    assert(std::memcmp(&unowned, &gSaveContext.save.shipSaveInfo.nei, sizeof(unowned)) == 0);
    assert(std::memcmp(&playerBefore, &player, sizeof(player)) == 0);
    assert(std::memcmp(&snowBefore, &nativeSnow, sizeof(nativeSnow)) == 0);
    // Manual winter weather reaches the playable covered forest without
    // changing its sky or inventing an aurora behind the canopy.
    const auto forestControlSave = gSaveContext;
    play.sceneId = SCENE_LOST_WOODS;
    play.roomCtx.curRoom.num = 0;
    play.skyboxId = SKYBOX_2;
    gSaveContext.fileNum = 0;
    gSaveContext.sceneLayer = 0;
    gSaveContext.save.entrance = ENTRANCE(LOST_WOODS, 0);
    gSaveContext.save.cutsceneIndex = 0;
    Advance(play, 30);
    assert(Count(15) >= 8 && Count(24) == 0 && play.skyboxId == SKYBOX_2);
    std::puts("PASS manual winter in opening Lost Woods: visible mist with the authored covered sky");
    gSaveContext = forestControlSave;
    gSaveContext.save.time = saveBefore.save.time;
    gSaveContext.save.shipSaveInfo.nei = saveBefore.save.shipSaveInfo.nei;
    assert(std::memcmp(&saveBefore, &gSaveContext, sizeof(saveBefore)) == 0);
    MMWinterAtmosphere_Update(nullptr);
    assert(Draw(play) == 0);
    play.activeCamId = -1;
    MMWinterAtmosphere_Update(&play);
    play.activeCamId = 0;
    play.cameraPtrs[0] = nullptr;
    MMWinterAtmosphere_Update(&play);
    play.cameraPtrs[0] = &camera;
    play.state.gfxCtx = nullptr;
    MMWinterAtmosphere_Draw(&play);
    std::puts("PASS MM winter renderer/controller: terrain feathering, snow independence, actual story/interior/Rod "
              "gates, pause/live controls, frame/scene/room/game lifetimes, blue/green night ribbons, far clip, "
              "native camera frustum, frost contrast/footprint, full alpha/depth material, native texture restoration "
              "and actor/save/environment immutability");
}
#else
#include <cstdio>
int main() {
    std::fputs("FAIL: MM winter renderer is missing\n", stderr);
    return 1;
}
#endif
