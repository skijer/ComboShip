#if __has_include("soh/Enhancements/Graphics/SeasonParticles.h")
#include "z64.h"
extern "C" {
#include "functions.h"
#include "variables.h"
#include "macros.h"
}
#include "soh/Enhancements/Graphics/SeasonParticles.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/ShipInit.hpp"
#include "soh/Enhancements/audio/SeasonWeather.h"
#include "soh/Enhancements/audio/GlobalOutdoorRain.h"
#include "soh/Enhancements/audio/GlobalOutdoorRainBridge.h"
#include "overlays/actors/ovl_En_Weather_Tag/z_en_weather_tag.h"
#include "mods/extended_inventory.h"
#include "soh/Enhancements/Graphics/SeasonParticleState.h"
#include "soh/Enhancements/Graphics/SeasonSummerState.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

SaveContext gSaveContext{};
GameInteractor* GameInteractor::Instance = nullptr;
extern "C" {
PlayState* gPlayState = nullptr;
LightningStrike gLightningStrike{};
u8 gWeatherMode, gSkyboxBlendingEnabled, D_8011FB34, D_8011FB38;
u16 gTimeSpeed;
u32 gBitFlags[32];
void EnWeatherTag_Init(Actor*, PlayState*);
void EnWeatherTag_Update(Actor*, PlayState*);
}
static GameInfo gameInfo{};
GameInfo* gGameInfo = &gameInfo;
static int season = 2, enabled = 1, flurries = 0;
static int summerEffects[3] = { 1, 1, 0 };
static int summerEnabled = 1;
static int winterEffects[2] = { 0, 0 };
static bool floorValid = true;
static int floorQueries = 0;
static int matrices = 0, stack = 0, children = 0, scopes = 0, beamVertices = 0, setupCalls = 0;
static Gfx xlu[4096], opa[1024];
static Vtx dynamicVertices[512];
static size_t allocatedVertices = 0;
static Mtx matrix;
static std::vector<Vec3f> translations;
static Vec3f currentTranslation;
static std::vector<Vec3f> meshOrigins;
static std::vector<Gfx*> displayLists;
static std::vector<unsigned> alphas;
static float aspect = 16.0f / 9.0f;
static float rainGain = 0;
static int natureWrites = 0;
static NeiSaveData neiSave{};
#include "season_particles_native_setup.inc"
extern "C" int32_t CVarGetInteger(const char* key, int32_t fallback) {
    if (std::strcmp(key, SOH_SEASON_WEATHER_CVAR("Season")) == 0)
        return season;
    if (std::strcmp(key, SOH_SEASON_WEATHER_CVAR("Spring.Blossoms")) == 0 ||
        std::strcmp(key, SOH_SEASON_WEATHER_CVAR("Autumn.Leaves")) == 0 ||
        std::strcmp(key, SOH_SEASON_WEATHER_CVAR("Winter.Snow")) == 0)
        return enabled;
    if (std::strcmp(key, SOH_SEASON_WEATHER_CVAR("Winter.Flurries")) == 0)
        return flurries;
    if (std::strcmp(key, SOH_SEASON_WEATHER_CVAR("Winter.Mist")) == 0)
        return winterEffects[0];
    if (std::strcmp(key, SOH_SEASON_WEATHER_CVAR("Winter.Aurora")) == 0)
        return winterEffects[1];
    if (std::strcmp(key, SOH_SEASON_WEATHER_CVAR("Summer.Enabled")) == 0)
        return summerEnabled;
    if (std::strcmp(key, SOH_SEASON_WEATHER_CVAR("Summer.Fluff")) == 0)
        return summerEffects[0];
    if (std::strcmp(key, SOH_SEASON_WEATHER_CVAR("Summer.Fireflies")) == 0)
        return summerEffects[1];
    if (std::strcmp(key, SOH_SEASON_WEATHER_CVAR("Summer.Sunbeams")) == 0)
        return summerEffects[2];
    return fallback;
}
extern "C" uint8_t Seasons_SeasonCount() {
    assert(false);
    return 0;
}
extern "C" uint8_t Seasons_GetSeason() {
    assert(false);
    return 0;
}
extern "C" NeiSaveData* Nei_Save() {
    return &neiSave;
}
extern "C" Color_RGB8 CVarGetColor24(const char*, Color_RGB8 fallback) {
    return fallback;
}
extern "C" s16 Rand_S16Offset(s16 base, s16) {
    return base;
}
extern "C" u8 Audio_IsNatureRainEnabled() {
    return 0;
}
extern "C" void Audio_SetNatureAmbienceChannelIO(u8, u8, u8) {
    ++natureWrites;
}
extern "C" void WeatherSamplePlayer_SetLoop(const char* path, float gain) {
    rainGain = path ? gain : 0;
}
extern "C" void GlobalOutdoorRain_Log(const char*) {
}
extern "C" f32 Actor_WorldDistXZToActor(Actor* a, Actor* b) {
    return std::hypot(a->world.pos.x - b->world.pos.x, a->world.pos.z - b->world.pos.z);
}
extern "C" void Actor_Kill(Actor* actor) {
    actor->update = nullptr;
}
extern "C" s16 Math_SmoothStepToS(s16*, s16, s16, s16, s16) {
    return 0;
}
extern "C" DebugDispObject* DebugDisplay_AddObject(f32, f32, f32, s16, s16, s16, f32, f32, f32, u8, u8, u8, u8, s16,
                                                   GraphicsContext*) {
    return nullptr;
}
extern "C" void lusprintf(const char*, int32_t, int32_t, const char*, ...) {
}
extern "C" {
#include "season_particles_rain.inc"
}
extern "C" float OTRGetAspectRatio() {
    return aspect;
}
extern "C" void FrameInterpolation_RecordOpenChild(const void*, int) {
    ++children;
}
extern "C" void FrameInterpolation_RecordCloseChild() {
    assert(children > 0);
    --children;
}
extern "C" void Graph_OpenDisps(Gfx**, GraphicsContext*, const char*, s32) {
    ++scopes;
}
extern "C" void Graph_CloseDisps(Gfx**, GraphicsContext*, const char*, s32) {
    assert(scopes > 0);
    --scopes;
}
extern "C" void Gfx_SetupDL_25Xlu(GraphicsContext* gfx) {
    ++setupCalls;
    // Submit SoH's exact production setup material, including its textured
    // combiner, cycle/render mode and geometry flags.
    __gSPDisplayList(gfx->polyXlu.p++, sSetupDL[SETUPDL_25]);
}
extern "C" void Matrix_Push() {
    ++stack;
}
extern "C" void Matrix_Pop() {
    assert(stack > 0);
    --stack;
}
extern "C" void Matrix_Translate(f32 x, f32 y, f32 z, u8) {
    currentTranslation = { x, y, z };
    translations.push_back({ x, y, z });
}
extern "C" void Matrix_Mult(MtxF*, u8) {
}
extern "C" void Matrix_Scale(f32 x, f32 y, f32, u8) {
    assert(x > 0 && y > 0);
}
extern "C" void Matrix_RotateZ(f32 angle, u8) {
    assert(std::isfinite(angle));
}
extern "C" Mtx* Matrix_NewMtx(GraphicsContext*, char*, s32) {
    ++matrices;
    return &matrix;
}
extern "C" void* Graph_Alloc(GraphicsContext*, size_t size) {
    assert(size % sizeof(Vtx) == 0 && allocatedVertices + size / sizeof(Vtx) <= 512);
    auto* result = dynamicVertices + allocatedVertices;
    allocatedVertices += size / sizeof(Vtx);
    return result;
}
extern "C" void gSPDisplayList(Gfx* pkt, Gfx* list) {
    displayLists.push_back(list);
    __gSPDisplayList(pkt, list);
}
extern "C" void gSPVertex(Gfx* pkt, uintptr_t address, int count, int start) {
    assert(count > 0 && count <= 32 && start == 0);
    ++beamVertices;
    meshOrigins.push_back(currentTranslation);
    __gSPVertex(pkt, address, count, start);
}
extern "C" void gSPSegment(void* pkt, int segment, uintptr_t address) {
    __gSPSegment(static_cast<Gfx*>(pkt), segment, address);
}
extern "C" f32 BgCheck_AnyRaycastFloor1(CollisionContext*, CollisionPoly*, Vec3f* point) {
    ++floorQueries;
    return floorValid ? point->x * 0.04f + point->z * 0.02f : BGCHECK_Y_MIN;
}

static size_t Draw(PlayState& play) {
    auto& gfx = *play.state.gfxCtx;
    gfx.polyXlu.p = xlu;
    gfx.polyOpa.p = opa;
    gfx.polyOpa.d = opa + 1024;
    translations.clear();
    meshOrigins.clear();
    displayLists.clear();
    alphas.clear();
    matrices = beamVertices = setupCalls = 0;
    allocatedVertices = 0;
    GameInteractor::Instance->ExecuteHooks<GameInteractor::OnPlayDrawEnd>();
    const size_t commands = gfx.polyXlu.p - xlu;
    assert(commands < 2048);
    assert(stack == 0 && children == 0 && scopes == 0);
    assert(matrices <= (season == SEASON_SUMMER ? 129 : 96)); // Summer dusk: 128 sprites plus optional beams.
    bool texture = true, depthTest = false;
    uint32_t geometry = G_LIGHTING | G_FOG | G_CULL_BACK | G_CULL_FRONT;
    uint32_t otherHigh = 0, otherLow = 0;
    const Gfx shade = gsDPSetCombineMode(G_CC_SHADE, G_CC_SHADE);
    Gfx lastCombine{};
    auto inspect = [&](auto&& self, const Gfx* p) -> void {
        const unsigned opcode = p->words.w0 >> 24;
        if (opcode == G_DL && p->words.w1 == reinterpret_cast<uintptr_t>(sSetupDL[SETUPDL_25])) {
            for (const auto& command : sSetupDL[SETUPDL_25])
                self(self, &command);
        }
        if (opcode == G_SETCOMBINE)
            lastCombine = *p;
        if (opcode == G_TEXTURE)
            texture = (p->words.w0 & 0xFF) != 0;
        if (opcode == G_SETPRIMCOLOR)
            alphas.push_back(p->words.w1 & 0xFF);
        if (opcode == G_GEOMETRYMODE)
            geometry = (geometry & (p->words.w0 & 0xFFFFFF)) | p->words.w1;
        if (opcode == G_RDPSETOTHERMODE) {
            otherHigh = p->words.w0 & 0xFFFFFF;
            otherLow = p->words.w1;
        }
        if (opcode == G_SETOTHERMODE_H || opcode == G_SETOTHERMODE_L) {
            const unsigned bits = (p->words.w0 & 0xFF) + 1;
            const unsigned shift = 32 - ((p->words.w0 >> 8) & 0xFF) - bits;
            const uint32_t mask = (0xFFFFFFFFu >> (32 - bits)) << shift;
            auto& mode = opcode == G_SETOTHERMODE_H ? otherHigh : otherLow;
            mode = (mode & ~mask) | (p->words.w1 & mask);
        }
        if (opcode == G_SETOTHERMODE_L && (p->words.w1 & Z_CMP))
            depthTest = true;
        const auto vertexCount = (p->words.w0 >> 12) & 0xFF;
        if (season == 3 && opcode == G_VTX && (vertexCount == 15 || vertexCount == 24)) {
            assert(!texture && !(geometry & (G_LIGHTING | G_FOG | G_CULL_BACK | G_CULL_FRONT)));
            assert((geometry & (G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH)) == (G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH));
            assert(lastCombine.words.w0 == shade.words.w0 && lastCombine.words.w1 == shade.words.w1);
            assert((otherHigh & (3u << G_MDSFT_CYCLETYPE)) == G_CYC_1CYCLE);
            assert((otherLow & (G_AC_DITHER | G_ZS_PRIM | Z_UPD | CVG_X_ALPHA)) == 0);
            assert((otherLow & Z_CMP) && ((otherLow & (3u << 20)) == (G_BL_CLR_MEM << 20)) &&
                   ((otherLow & (3u << 16)) == (G_BL_1MA << 16)));
        }
    };
    for (auto* p = xlu; p < gfx.polyXlu.p; ++p)
        inspect(inspect, p);
    if (matrices) {
        assert(texture && depthTest);
        assert(lastCombine.words.w0 == sSetupDL[SETUPDL_25][2].words.w0);
        assert(lastCombine.words.w1 == sSetupDL[SETUPDL_25][2].words.w1);
        assert(setupCalls == 2 || setupCalls == 4); // Each drawn layer restores its material.
    } else {
        assert(commands == 0 && setupCalls == 0); // Disabled/all-culled layers preserve their consumer.
    }
    return commands;
}
static bool Same(Vec3f a, Vec3f b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}
static void Advance(PlayState& play, int frames) {
    for (int frame = 0; frame < frames; ++frame) {
        ++play.gameplayFrames;
        Draw(play);
    }
}
static SoHSeasonParticles::View NativeAuroraCamera(PlayState& play, float pitch, int yaw) {
    const float p = pitch * 0.0174532925f, y = yaw * 0.0174532925f;
    SoHSeasonParticles::View view{ { 0, 100, 0 },
                                   { std::sin(y) * std::cos(p), -std::sin(p), std::cos(y) * std::cos(p) },
                                   { -std::cos(y), 0, std::sin(y) },
                                   { std::sin(y) * std::sin(p), std::cos(p), std::cos(y) * std::sin(p) },
                                   0.5773503f,
                                   aspect };
    auto* camera = GET_ACTIVE_CAM(&play);
    const auto at = view.eye + view.forward * 1000;
    play.view.eye = camera->eye = { view.eye.x, view.eye.y, view.eye.z };
    play.view.lookAt = camera->at = { at.x, at.y, at.z };
    play.view.up = { view.up.x, view.up.y, view.up.z };
    play.view.fovy = camera->fov = 60;
    return view;
}
static bool SubmittedAuroraCoreInView(const SoHSeasonParticles::View& view) {
    assert(allocatedVertices == 72 && meshOrigins.size() == 3);
    for (size_t i = 0; i < allocatedVertices; ++i) {
        const auto& vertex = dynamicVertices[i];
        const auto& origin = meshOrigins[i / 24];
        const auto* p = vertex.v.ob;
        const auto projected = SoHSeasonParticles::Project({ origin.x + p[0], origin.y + p[1], origin.z + p[2] }, view);
        if (vertex.v.cn[3] > 25 && projected.depth > 0 && std::abs(projected.x) < 0.95f &&
            std::abs(projected.y) < 0.95f)
            return true;
    }
    return false;
}

static void RainWithDisabledSkyPreservesParticles() {
    static PlayState play{};
    static Camera camera{};
    static GraphicsContext gfx{};
    static Actor player{}, nativeSnow{};
    static EnWeatherTag tag{};
    auto* previousPlay = gPlayState;
    const auto saved = gSaveContext;
    gPlayState = &play;
    play.state.gfxCtx = &gfx;
    play.sceneNum = SCENE_LOST_WOODS;
    play.cameraPtrs[0] = &camera;
    play.skyboxId = SKYBOX_NORMAL_SKY;
    play.view.eye = camera.eye = { 0, 100, 0 };
    play.view.lookAt = camera.at = { 0, 100, 1000 };
    play.view.up = { 0, 1, 0 };
    play.view.fovy = camera.fov = 60;
    play.view.zFar = 12800;
    player.world.pos = { 0, 100, -300 };
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player;
    R_UPDATE_RATE = 3;
    for (int selected : { SEASON_SPRING, SEASON_AUTUMN, SEASON_WINTER }) {
        season = selected;
        enabled = 1;
        winterEffects[0] = winterEffects[1] = 1;
        gSaveContext.dayTime = selected == SEASON_WINTER ? CLOCK_TIME(23, 0) : CLOCK_TIME(12, 0);
        for (int source : std::array<int, 3>{ -1, EN_WEATHER_TAG_TYPE_RAIN_LAKE_HYLIA,
                                              EN_WEATHER_TAG_TYPE_THUNDERSTORM_GRAVEYARD }) {
            SeasonParticles_Reset();
            SeasonWeather_Reset();
            GlobalOutdoorRain_Reset();
            play.envCtx = {};
            play.envCtx.skyboxDisabled = true;
            play.envCtx.unk_EE[2] = 11;
            play.envCtx.unk_EE[3] = 31;
            play.actorCtx.actorLists[ACTORCAT_PROP].head = nullptr;
            play.actorCtx.actorLists[ACTORCAT_ITEMACTION].head = nullptr;
            gWeatherMode = gSkyboxBlendingEnabled = 0;
            GlobalOutdoorRain_BeginScene(&play, source < 0 ? 40 : 0, source < 0, 0);
            if (source >= 0) {
                tag = {};
                tag.actor.id = ACTOR_EN_WEATHER_TAG;
                tag.actor.params = 0x0A00 | source;
                tag.actor.update = EnWeatherTag_Update;
                play.actorCtx.actorLists[ACTORCAT_PROP].head = &tag.actor;
                EnWeatherTag_Init(&tag.actor, &play);
                assert(tag.actor.update != nullptr);
            }
            for (int frame = 0; frame < 64; ++frame) {
                ++play.state.frames;
                ++play.gameplayFrames;
                if (source >= 0)
                    EnWeatherTag_Update(&tag.actor, &play);
                SeasonWeather_Update(&play);
                GlobalOutdoorRain_Update(&play);
                func_800766C4(&play);
                const auto environment = play.envCtx;
                const auto light = play.lightCtx;
                const auto save = gSaveContext;
                const float gain = rainGain;
                const int writes = natureWrites;
                Draw(play);
                assert(std::memcmp(&environment, &play.envCtx, sizeof(environment)) == 0);
                assert(std::memcmp(&light, &play.lightCtx, sizeof(light)) == 0);
                assert(std::memcmp(&save, &gSaveContext, sizeof(save)) == 0);
                assert(rainGain == gain && natureWrites == writes);
            }
            std::printf("SoH sky-disabled rain coexistence season=%d source=%d: falling=%zu mist vertices=%zu\n",
                        selected, source, displayLists.size(), allocatedVertices);
            std::fflush(stdout);
            assert(SeasonWeather_SeasonForPlay(&play) == -1); // Sky/light/rain eligibility retains its native gate.
            assert(play.envCtx.unk_EE[0] == (source < 0 ? 40 : 25) && play.envCtx.unk_EE[1] > 0);
            assert(play.envCtx.unk_EE[2] == 11 && play.envCtx.unk_EE[3] == 31);
            assert(rainGain == 0.5f && GlobalOutdoorRain_HasRainIntent());
            assert(displayLists.size() == (selected == SEASON_WINTER ? 64 : 96));
            assert(selected != SEASON_WINTER || (allocatedVertices >= 15 && allocatedVertices % 15 == 0));
            if (selected == SEASON_WINTER) {
                nativeSnow.id = ACTOR_OBJECT_KANKYO;
                nativeSnow.params = 3;
                nativeSnow.update = reinterpret_cast<ActorFunc>(1);
                play.actorCtx.actorLists[ACTORCAT_ITEMACTION].head = &nativeSnow;
                Draw(play);
                assert(displayLists.empty() && allocatedVertices >= 15 && allocatedVertices % 15 == 0);
                play.actorCtx.actorLists[ACTORCAT_ITEMACTION].head = nullptr;
            }
            for (int gate = 0; gate < 6; ++gate) {
                play.envCtx.indoors = gate == 0;
                play.envCtx.customSkyboxFilter = gate == 1;
                play.csCtx.state = gate == 2 ? 1 : CS_STATE_IDLE;
                play.gameOverCtx.state = gate == 3 ? GAMEOVER_DEATH_START : GAMEOVER_INACTIVE;
                camera.unk_14C = gate == 4 ? 0x100 : 0;
                gSaveContext.gameMode = gate == 5 ? GAMEMODE_FILE_SELECT : GAMEMODE_NORMAL;
                assert(Draw(play) == 0);
            }
            play.envCtx.indoors = play.envCtx.customSkyboxFilter = 0;
            play.csCtx.state = CS_STATE_IDLE;
            play.gameOverCtx.state = GAMEOVER_INACTIVE;
            camera.unk_14C = 0;
            gSaveContext.gameMode = GAMEMODE_NORMAL;
        }
    }
    play.actorCtx.actorLists[ACTORCAT_PROP].head = nullptr;
    play.envCtx = {};
    play.envCtx.skyboxDisabled = true;
    play.envCtx.sunPos = { 450, 800, -250 };
    season = SEASON_SUMMER;
    summerEffects[0] = summerEffects[2] = 1;
    summerEffects[1] = 0;
    gSaveContext.dayTime = CLOCK_TIME(12, 0);
    SeasonParticles_Reset();
    Advance(play, 30);
    assert(matrices == 32 && beamVertices == 0); // Drifting fluff survives; hidden sky cannot make sunbeams.
    summerEffects[0] = 0;
    summerEffects[1] = 1;
    gSaveContext.dayTime = CLOCK_TIME(23, 0);
    Advance(play, 30);
    assert(matrices == 96 && beamVertices == 0); // Fireflies also need no visible sky.
    season = SEASON_WINTER;
    enabled = 0;
    winterEffects[0] = 0;
    winterEffects[1] = 1;
    gSaveContext.dayTime = CLOCK_TIME(23, 0);
    SeasonParticles_Reset();
    Advance(play, 30);
    assert(Draw(play) == 0); // Aurora also requires a visible normal sky.
    SeasonParticles_Reset();
    SeasonWeather_Reset();
    GlobalOutdoorRain_Reset();
    gSaveContext = saved;
    gPlayState = previousPlay;
    enabled = 1;
    season = SEASON_AUTUMN;
    winterEffects[0] = winterEffects[1] = 0;
    summerEffects[0] = summerEffects[1] = 1;
    summerEffects[2] = 0;
    std::puts("PASS authored/native rain with sky-disabled outdoor particles: separate sky eligibility, native "
              "rain/audio/counters/lighting preservation, actual weather tags, story/interior/underwater gates");
}

int main() {
    static PlayState play{};
    static Camera camera{};
    static GraphicsContext gfx{};
    static Actor player{};
    play.state.gfxCtx = &gfx;
    gPlayState = &play;
    static GameInteractor interactor;
    GameInteractor::Instance = &interactor;
    ShipInit::InitAll();
    assert(GameInteractor::RegisteredGameHooks<GameInteractor::OnPlayDrawEnd>::functions.size() == 2);
    RainWithDisabledSkyPreservesParticles();
    play.sceneNum = SCENE_HYRULE_FIELD;
    play.skyboxId = SKYBOX_NORMAL_SKY;
    play.roomCtx.curRoom.num = 0;
    play.cameraPtrs[0] = &camera;
    play.view.eye = camera.eye = { 0, 100, 0 };
    play.view.lookAt = camera.at = { 0, 100, 1000 };
    play.view.up = { 0, 1, 0 };
    play.view.fovy = camera.fov = 60;
    player.world.pos = { 0, 100, -300 };
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player;
    gSaveContext.dayTime = CLOCK_TIME(12, 0);
    R_UPDATE_RATE = 3;
    const auto saveBefore = gSaveContext;
    const auto environmentBefore = play.envCtx;
    const auto playerBefore = player;
    Advance(play, 25);
    assert(matrices == 96 && displayLists.size() == 96);
    auto autumn = translations;
    auto autumnList = displayLists[0];
    Draw(play);
    for (size_t i = 0; i < 96; ++i)
        assert(Same(translations[i], autumn[i]));
    play.pauseCtx.state = 6;
    Advance(play, 10);
    for (size_t i = 0; i < 96; ++i)
        assert(Same(translations[i], autumn[i]));
    play.pauseCtx.state = 0;
    play.pauseCtx.debugState = 1;
    Advance(play, 3);
    for (size_t i = 0; i < 96; ++i)
        assert(Same(translations[i], autumn[i]));
    play.pauseCtx.debugState = 0;
    play.pauseCtx.state = 6;
    enabled = 0;
    assert(Draw(play) == 0);
    enabled = 1;
    season = 0;
    Draw(play);
    assert(matrices == 96 && displayLists[0] != autumnList);
    auto petalList = displayLists[0];
    play.view.eye.x += 50000;
    play.view.lookAt.x += 50000;
    assert(Draw(play) == 0); // A paused, fully culled untextured layer emits no setup or restoration.
    play.view.eye.x -= 50000;
    play.view.lookAt.x -= 50000;
    Draw(play);
    assert(matrices == 96);
    // Reference blossoms have five rounded petals. The former two-lobed,
    // pointed silhouette reads as a heart and must fail this shape check.
    assert((petalList[0].words.w0 >> 24) == G_VTX);
    const auto* petalVertices = reinterpret_cast<const Vtx*>(petalList[0].words.w1);
    const unsigned petalVertexCount = (petalList[0].words.w0 >> 12) & 0xFF;
    assert(petalVertexCount > 5 && petalVertexCount <= 32);
    auto radiusSquared = [&](unsigned index) {
        const auto& vertex = petalVertices[index].v.ob;
        return vertex[0] * vertex[0] + vertex[1] * vertex[1];
    };
    unsigned lobes = 0;
    for (unsigned i = 1; i < petalVertexCount; ++i) {
        const unsigned previous = i == 1 ? petalVertexCount - 1 : i - 1;
        const unsigned next = i == petalVertexCount - 1 ? 1 : i + 1;
        lobes += radiusSquared(i) > radiusSquared(previous) && radiusSquared(i) > radiusSquared(next);
    }
    assert(lobes == 5);
    // A smaller detached petal accompanies each intact blossom, using the
    // same bounded owner and its existing motion and interpolation epochs.
    assert(displayLists[0] != displayLists[1]);
    bool curved = false;
    for (unsigned i = 0; i < petalVertexCount; ++i)
        curved |= petalVertices[i].v.ob[2] != 0;
    assert(curved);
    play.pauseCtx.state = 0;
    season = 3;
    Draw(play);
    assert(matrices == 64);
    static Actor fairy{};
    fairy.id = ACTOR_OBJECT_KANKYO;
    fairy.params = 0;
    fairy.update = reinterpret_cast<ActorFunc>(1);
    play.actorCtx.actorLists[ACTORCAT_ITEMACTION].head = &fairy;
    assert(Draw(play) > 0 && matrices == 64); // Kokiri fairy emitter remains independent.
    const auto fairyBefore = fairy;
    static Actor snow{};
    snow.id = ACTOR_OBJECT_KANKYO;
    snow.params = 3;
    snow.update = reinterpret_cast<ActorFunc>(1);
    fairy.next = &snow;
    assert(Draw(play) == 0); // Live native snow owns winter instead of a duplicate.
    snow.update = nullptr;
    assert(Draw(play) > 0 && matrices == 64);
    fairy.next = nullptr;
    assert(std::memcmp(&fairy, &fairyBefore, sizeof(fairy)) == 0);
    play.actorCtx.actorLists[ACTORCAT_ITEMACTION].head = nullptr;
    assert(std::memcmp(&gSaveContext, &saveBefore, sizeof(saveBefore)) == 0);
    assert(std::memcmp(&play.envCtx, &environmentBefore, sizeof(environmentBefore)) == 0);
    assert(std::memcmp(&player, &playerBefore, sizeof(playerBefore)) == 0);
    for (int value : { -1, 0, 2, 3 }) {
        season = value;
        play.transitionTrigger = TRANS_TRIGGER_START;
        assert(Draw(play) == 0);
        play.transitionTrigger = TRANS_TRIGGER_OFF;
        play.transitionMode = TRANS_MODE_INSTANCE_RUNNING;
        assert(Draw(play) == 0);
        play.transitionMode = TRANS_MODE_OFF;
        camera.unk_14C = 0x100;
        assert(Draw(play) == 0);
        camera.unk_14C = 0;
    }
    season = 2;
    for (int gate = 0; gate < 3; ++gate) {
        play.envCtx.indoors = gate == 0;
        play.csCtx.state = gate == 1 ? 1 : CS_STATE_IDLE;
        play.gameOverCtx.state = gate == 2 ? GAMEOVER_DEATH_START : GAMEOVER_INACTIVE;
        assert(Draw(play) == 0);
    }
    play.envCtx.indoors = play.envCtx.skyboxDisabled = 0;
    play.csCtx.state = CS_STATE_IDLE;
    play.gameOverCtx.state = GAMEOVER_INACTIVE;
    season = 1;
    assert(!SeasonWeather_ParticlesEnabled(1));
    assert(SeasonWeather_SummerEffectEnabled(0));
    play.envCtx.sunPos = { 450, 800, -250 };
    Advance(play, 30);
    assert(matrices == 32 && beamVertices == 0);
    summerEnabled = 0;
    assert(Draw(play) == 0);
    summerEnabled = 1;
    Advance(play, 30);
    assert(matrices == 32);
    summerEffects[2] = 1;
    Draw(play);
    assert(matrices == 33 && beamVertices == 3 && allocatedVertices == 36);
    gSaveContext.dayTime = CLOCK_TIME(23, 0);
    Draw(play);
    assert(matrices == 96 && beamVertices == 0);
    summerEffects[2] = 0;
    gSaveContext.dayTime = CLOCK_TIME(18, 0);
    Draw(play);
    assert(matrices > 96 && matrices <= 128);
    const auto dusk = translations;
    play.pauseCtx.state = 6;
    summerEffects[0] = 0;
    Draw(play);
    assert(matrices > 0 && matrices <= 96);
    for (const auto& point : translations)
        assert(std::any_of(dusk.begin(), dusk.end(), [&](Vec3f p) { return Same(point, p); }));
    summerEffects[1] = 0;
    summerEffects[2] = 0;
    assert(Draw(play) == 0);
    gSaveContext.dayTime = CLOCK_TIME(12, 0);
    summerEffects[2] = 1;
    Draw(play);
    assert(matrices == 1 && beamVertices == 3);
    play.envCtx.unk_EE[1] = 30;
    assert(Draw(play) == 0);
    play.envCtx.unk_EE[1] = 0;
    play.envCtx.sunMoonDisabled = 1;
    assert(Draw(play) == 0);
    play.envCtx.sunMoonDisabled = 0;
    summerEffects[0] = summerEffects[1] = 1;
    summerEffects[2] = 0;
    GameInteractor::Instance->ExecuteHooks<GameInteractor::OnPlayDestroy>();
    Draw(play);
    assert(matrices == 0); // New summer entries fade in.
    play.pauseCtx.state = 0;
    Advance(play, 30);
    assert(matrices == 32);
    GameInteractor::Instance->ExecuteHooks<GameInteractor::OnSceneInit>(play.sceneNum);
    assert(Draw(play) == 0);
    Advance(play, 30);
    GameInteractor::Instance->ExecuteHooks<GameInteractor::OnExitGame>(0);
    assert(Draw(play) == 0);
    Advance(play, 30);
    ++play.roomCtx.curRoom.num;
    assert(Draw(play) == 0);
    Advance(play, 30);
    ++play.sceneNum;
    assert(Draw(play) == 0);
    Advance(play, 30);
    play.gameplayFrames = 0;
    assert(Draw(play) == 0);
    // Removing the snow-independent winter draw path must fail this test.
    play.pauseCtx.state = 0;
    play.skyboxId = SKYBOX_NORMAL_SKY;
    play.view.zFar = 12800;
    season = 3;
    enabled = 0;
    winterEffects[0] = 1;
    winterEffects[1] = 0;
    gSaveContext.dayTime = CLOCK_TIME(12, 0);
    const auto winterSave = gSaveContext;
    const auto winterEnvironment = play.envCtx;
    Advance(play, 30);
    assert(Draw(play) > 0 && matrices > 0 && matrices <= 12 && allocatedVertices > 0 && displayLists.empty());
    assert(floorQueries > 0);
    assert(allocatedVertices <= 252);
    // Every mist sheet is grounded on the sampled sloped terrain, not camera Y.
    for (size_t i = 0; i < allocatedVertices; i += 15) {
        const auto& center = dynamicVertices[i + 7];
        const auto origin = meshOrigins[i / 15];
        const float ground = (center.v.ob[0] + origin.x) * 0.04f + (center.v.ob[2] + origin.z) * 0.02f;
        const float clearance = center.v.ob[1] + origin.y - ground;
        assert(clearance >= 5 && clearance <= 40);
    }
    const auto mistVertices = std::vector<Vtx>(dynamicVertices, dynamicVertices + allocatedVertices);
    const int samples = floorQueries;
    play.pauseCtx.state = 6;
    Draw(play);
    assert(floorQueries == samples && allocatedVertices == mistVertices.size());
    assert(std::memcmp(dynamicVertices, mistVertices.data(), allocatedVertices * sizeof(Vtx)) == 0);
    const auto oldOrigins = meshOrigins;
    play.view.eye.x += 20;
    play.view.lookAt.x += 20;
    Draw(play);
    assert(allocatedVertices == mistVertices.size() && meshOrigins.size() == oldOrigins.size());
    for (size_t i = 0; i < meshOrigins.size(); ++i) {
        const float oldCenter = oldOrigins[i].x + mistVertices[i * 15 + 7].v.ob[0];
        // SoH interpolates matrix translations but consumes current vertices.
        const float middle = (oldOrigins[i].x + meshOrigins[i].x) * 0.5f + dynamicVertices[i * 15 + 7].v.ob[0];
        assert(std::abs(middle - oldCenter) <= 1);
    }
    play.view.eye.x -= 20;
    play.view.lookAt.x -= 20;
    Draw(play);
    winterEffects[0] = 0;
    assert(Draw(play) == 0);
    winterEffects[0] = 1;
    assert(Draw(play) > 0);
    play.pauseCtx.state = 0;
    floorValid = false;
    ++play.roomCtx.curRoom.num;
    Advance(play, 30);
    assert(Draw(play) == 0); // Missing collision cannot produce floating fog.
    floorValid = true;
    winterEffects[0] = 0;
    winterEffects[1] = 1;
    assert(Draw(play) == 0); // Aurora cannot appear in daylight.
    gSaveContext.dayTime = CLOCK_TIME(23, 0);
    Advance(play, 30);
    assert(Draw(play) > 0 && matrices == 1 && allocatedVertices == 72);
    bool blue = false, green = false;
    for (size_t i = 0; i < allocatedVertices; ++i) {
        const auto& vertex = dynamicVertices[i];
        blue |= vertex.v.cn[2] > vertex.v.cn[1] && vertex.v.cn[3] > 0;
        green |= vertex.v.cn[1] > vertex.v.cn[2] && vertex.v.cn[3] > 0;
    }
    assert(blue && green);
    for (const float nativeAspect : { 4.0f / 3.0f, 16.0f / 9.0f }) {
        aspect = nativeAspect;
        for (float pitch : { 10.0f, 15.0f })
            for (int yaw = 0; yaw < 360; yaw += 30) {
                const auto view = NativeAuroraCamera(play, pitch, yaw);
                Draw(play);
                assert(matrices == 1 && SubmittedAuroraCoreInView(view));
            }
    }
    aspect = 16.0f / 9.0f;
    NativeAuroraCamera(play, 0, 0);
    Draw(play);
    const auto nightAurora = std::vector<Vtx>(dynamicVertices, dynamicVertices + allocatedVertices);
    for (int hour : { 20, 21, 22, 23, 0, 1, 2, 3, 4 }) {
        gSaveContext.dayTime = CLOCK_TIME(hour, 0);
        Draw(play);
        assert(allocatedVertices == nightAurora.size());
        assert(std::memcmp(dynamicVertices, nightAurora.data(), allocatedVertices * sizeof(Vtx)) == 0);
    }
    gSaveContext.dayTime = CLOCK_TIME(23, 0);
    play.view.zFar = 2000;
    Draw(play);
    for (size_t i = 0; i < allocatedVertices; ++i) {
        const auto& vertex = dynamicVertices[i];
        const float length = std::sqrt(float(vertex.v.ob[0]) * vertex.v.ob[0] + float(vertex.v.ob[1]) * vertex.v.ob[1] +
                                       float(vertex.v.ob[2]) * vertex.v.ob[2]);
        assert(length < 2000);
    }
    play.skyboxId = SKYBOX_NONE;
    assert(Draw(play) == 0);
    play.skyboxId = SKYBOX_NORMAL_SKY;
    winterEffects[0] = 1;
    enabled = 1;
    Advance(play, 30);
    assert(Draw(play) > 0 && displayLists.size() == 64 && allocatedVertices > 0 && matrices > 65 && matrices <= 77);
    enabled = 0;
    fairy.next = &snow;
    snow.update = reinterpret_cast<ActorFunc>(1);
    play.actorCtx.actorLists[ACTORCAT_ITEMACTION].head = &fairy;
    Advance(play, 30);
    assert(Draw(play) > 0 && displayLists.empty()); // Native snow cannot suppress mist/aurora.
    assert(std::memcmp(&play.envCtx, &winterEnvironment, sizeof(winterEnvironment)) == 0);
    gSaveContext.dayTime = winterSave.dayTime;
    assert(std::memcmp(&gSaveContext, &winterSave, sizeof(winterSave)) == 0);
    for (int gate = 0; gate < 3; ++gate) {
        play.envCtx.indoors = gate == 0;
        play.csCtx.state = gate == 1 ? 1 : CS_STATE_IDLE;
        camera.unk_14C = gate == 2 ? 0x100 : 0;
        assert(Draw(play) == 0);
    }
    std::puts("PASS winter renderer: snow-independent terrain mist and night-only blue/green aurora, pause/toggles, "
              "native camera frustum, full-night timing, alpha/depth material, clipping, native snow coexistence, "
              "gates and ownership");
    std::puts("PASS native renderer:96leaf/petal,64snow,32seed/96firefly/3beam; command/matrix budget, texture "
              "restoration, pause/toggle/gates/reset, native actor/save/environment ownership");
}
#else
#include <cstdio>
int main() {
    std::fputs("FAIL: standalone seasonal native renderer is missing\n", stderr);
    return 1;
}
#endif
