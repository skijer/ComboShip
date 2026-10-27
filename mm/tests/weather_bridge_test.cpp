#include "2s2h/Enhancements/Audio/MMWeather.h"
#include "2s2h/Enhancements/Audio/MMWeatherAudio.h"
#include "global.h"
#include "BenPort.h"
#include "ComboGameMode.h"
#include "mods/extended_inventory.h"
#include "2s2h/Rando/Rando.h"
#include "assets/misc/skyboxes/d2_cloud_static.h"
#include "assets/misc/skyboxes/d2_fine_static.h"
#include "objects/gameplay_keep/gameplay_keep.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <initializer_list>
#include <string>
#include <unordered_map>
#include <vector>

extern "C" {
int gComboGameMode = COMBO_GAME_BOTH;
}

SaveContext gSaveContext;
u8 gWeatherMode;
LightningStrike gLightningStrike;
RegEditor editor;
RegEditor* gRegEditor = &editor;
static std::unordered_map<std::string, int> settings;
static float rainGain;
static int resets;
static int rainDraws;
static int skyRebuilds;
static int flashDraws;
static int thunderTriggers;
static int boltDraws;
static bool cameraNotFixed = true;
void MMAutumnSceneFoliage_Update(const PlayState*) {
}
void MMAutumnSceneFoliage_Reset() {
}
extern "C" void MMSummerAtmosphere_Reset() {
}
extern "C" void MMWinterAtmosphere_Reset() {
}
static uint8_t nativeRainAmbience;
static uint8_t nativeThunderAmbience;
f32 D_801F4E74;
f32 D_801F4F28;
u16 gSkyboxNumStars;
u8 sSkyboxIsChanging;
s32 sEnvSkyboxNumStars;
Gfx* sSkyboxStarsDList;

extern "C" NeiSaveData* Nei_Save() {
    return &gSaveContext.save.shipSaveInfo.nei;
}
extern "C" u16 Nei_GetOwnedItem(u8 slot) {
    return Nei_Save()->ownedItems[slot - 24];
}
#include "weather_seasons.inc"

extern "C" void Skybox_Calculate128(SkyboxContext*, s32) {
    ++skyRebuilds;
}
extern "C" bool ResourceMgr_IsAltAssetsEnabled(void) {
    return false; // This fixture exercises the native sky; OoT ownership has its own fixture.
}
extern "C" void Gfx_SetupDL57_Opa(GraphicsContext*) {
}
// Rendering boundary: keep real GBI writes; omit resource submission and
// frame-interpolation bookkeeping from this asset-free harness.
#undef OPEN_DISPS_PORT_HELPERS
#undef CLOSE_DISPS_PORT_HELPERS
#define OPEN_DISPS_PORT_HELPERS(gfxCtx)
#define CLOSE_DISPS_PORT_HELPERS(gfxCtx)
#define gSPDisplayList(pkt, dl) __gSPDisplayList(pkt, (Gfx*)(dl))
#define gSPSegment(pkt, segment, base) __gSPSegment(pkt, segment, base)
#include "weather_sky.inc"

extern "C" int32_t CVarGetInteger(const char* key, int32_t fallback) {
    auto found = settings.find(key);
    return found == settings.end() ? fallback : found->second;
}
extern "C" Color_RGBA8 CVarGetColor(const char*, Color_RGBA8 fallback) {
    return fallback;
}
extern "C" s32 Play_CamIsNotFixed(PlayState*) {
    return cameraNotFixed;
}
extern "C" void Environment_DrawRainImpl(PlayState*, View*, GraphicsContext*) {
    ++rainDraws;
}
#include "weather_environment.inc"
#include "weather_player.inc"
#include "weather_bombs.inc"
#include "weather_draw_gate.inc"
#include "weather_random.inc"
extern "C" void Environment_DrawSun(PlayState*) {
}
extern "C" void Matrix_Translate(f32, f32, f32, MatrixMode) {
    ++boltDraws;
}
extern "C" void Matrix_RotateXFApply(f32) {
}
extern "C" void Matrix_RotateZF(f32, MatrixMode) {
}
extern "C" void Matrix_Scale(f32, f32, f32, MatrixMode) {
}
extern "C" void Gfx_SetupDL61_Xlu(GraphicsContext*) {
}
extern "C" void* Lib_SegmentedToVirtual(void* ptr) {
    return ptr;
}
#undef MATRIX_FINALIZE_AND_LOAD
#define MATRIX_FINALIZE_AND_LOAD(pkt, gfxCtx) ((void)(gfxCtx))
#include "weather_bolts.inc"
extern "C" void Environment_DrawLightningFlash(PlayState*, u8, u8, u8, u8) {
    ++flashDraws;
}
extern "C" void MMWeatherAudio_Reset() {
    rainGain = 0;
    ++resets;
}
extern "C" void MMWeatherAudio_SetRain(float gain) {
    rainGain = gain;
}
extern "C" void MMWeatherAudio_Thunder(float) {
}
extern "C" void Audio_SetAmbienceChannelIO(u8 channel, u8 port, u8 value) {
    if (port == CHANNEL_IO_PORT_0) {
        assert(channel == AMBIENCE_CHANNEL_LIGHTNING && value == 0);
        ++thunderTriggers;
        return;
    }
    assert(port == CHANNEL_IO_PORT_1);
    const uint32_t cmd = MMWeather_ResolveAmbienceSeqCmd(
        (SEQCMD_OP_SET_CHANNEL_IO << 28) | (SEQ_PLAYER_AMBIENCE << 24) | (port << 16) | (channel << 8) | value);
    value = cmd & 0xFF;
    if (channel == AMBIENCE_CHANNEL_RAIN) {
        nativeRainAmbience = value;
    } else if (channel == AMBIENCE_CHANNEL_LIGHTNING) {
        nativeThunderAmbience = value;
    } else {
        assert(false);
    }
}

static void AdvanceRain(PlayState* play) {
    for (int i = 0; i < 25; ++i) {
        MMWeather_Update(play);
    }
}

static void SpinAttackRainRegression() {
    static PlayState play{};
    Camera camera{};
    play.sceneId = SCENE_TOWN;
    play.cameraPtrs[0] = &camera;
    play.skyboxId = SKYBOX_NORMAL_SKY;
    play.envCtx.stormState = STORM_STATE_ON;
    play.envCtx.lightSettingOverride = LIGHT_SETTING_OVERRIDE_NONE;
    play.envCtx.lightSettings.fogNear = 950;
    for (auto& component : play.envCtx.lightSettings.fogColor) {
        component = 128;
    }
    AdvanceRain(&play);
    assert(MMWeather_RainDensity() == 25 && rainGain > 0);
    const int beforeResets = resets;
    // Exercise the real shared effect for a held charge/release and a quick
    // spin/release. Neither profile sets a player "charging" flag.
    for (const auto& attack : { std::initializer_list<float>{ 0.25f, 0.6f, 1.0f, 0.6f, 0.25f, 0.0f },
                                std::initializer_list<float>{ 1.0f, 0.6f, 0.25f, 0.0f } }) {
        for (float intensity : attack) {
            EnMThunder_AdjustLights(&play, intensity);
            auto native = play.envCtx;
            const int beforeDraws = rainDraws;
            MMWeather_Update(&play);
            DrawWeatherFromPlay(&play);
            assert(MMWeather_RainDensity() == 25 && rainGain > 0);
            assert(resets == beforeResets && rainDraws == beforeDraws + 1);
            assert(std::memcmp(&native, &play.envCtx, sizeof(native)) == 0);
        }
    }
    MMWeather_Reset();
    std::puts("PASS rain and audio persist through held charge, quick spin and release lighting");
}

static void OutdoorOverrideRegression() {
    static PlayState play{};
    Camera camera{};
    play.cameraPtrs[0] = &camera;
    play.skyboxId = SKYBOX_NORMAL_SKY;
    play.envCtx.stormState = STORM_STATE_OFF;
    play.envCtx.lightSettingOverride = LIGHT_SETTING_OVERRIDE_NONE;
    auto native = play.envCtx;
    // Both reported scenes and an ordinary outdoor scene must ignore room
    // storm policy, regional gloom, story lighting and ongoing native weather.
    for (auto scene : { SCENE_00KEIKOKU, SCENE_21MITURINMAE, SCENE_TOWN }) {
        play.sceneId = scene;
        for (int condition = 0; condition < 14; ++condition) {
            play.envCtx = native;
            play.skyboxId = SKYBOX_NORMAL_SKY;
            play.csCtx.state = CS_STATE_IDLE;
            gWeatherMode = WEATHER_MODE_CLEAR;
            gLightningStrike.state = LIGHTNING_STRIKE_WAIT;
            switch (condition) {
                case 0:
                    break; // Storm-disallowed room alone.
                case 1:
                    play.envCtx.precipitation[PRECIP_RAIN_MAX] = 60;
                    break;
                case 2:
                    play.envCtx.precipitation[PRECIP_SNOW_MAX] = 128;
                    break;
                case 3:
                    play.envCtx.precipitation[PRECIP_SOS_MAX] = 32;
                    break;
                case 4:
                    play.envCtx.lightningState = LIGHTNING_ON;
                    break;
                case 5:
                    play.csCtx.state = 1;
                    break;
                case 6:
                    play.envCtx.changeSkyboxState = CHANGE_SKYBOX_ACTIVE;
                    break;
                case 7:
                    play.envCtx.lightSettingOverride = 1;
                    break;
                case 8:
                    play.envCtx.adjLightSettings.fogNear = -50;
                    break;
                case 9:
                    play.envCtx.lightMode = LIGHT_MODE_SETTINGS;
                    break;
                case 10:
                    play.envCtx.customSkyboxFilter = true;
                    break;
                case 11:
                    play.envCtx.changeLightEnabled = true;
                    break;
                case 12:
                    gLightningStrike.state = LIGHTNING_STRIKE_START;
                    break;
                case 13:
                    play.skyboxId = SKYBOX_3;
                    gWeatherMode = WEATHER_MODE_2;
                    play.envCtx.changeLightEnabled = true;
                    play.envCtx.lightSettingOverride = 1;
                    play.envCtx.adjLightSettings.fogColor[0] = -100;
                    break;
            }
            auto before = play.envCtx;
            const auto weatherMode = gWeatherMode;
            const auto lightningState = gLightningStrike.state;
            AdvanceRain(&play);
            const int beforeDraws = rainDraws;
            const int beforeResets = resets;
            MMWeather_Update(&play);
            DrawWeatherFromPlay(&play);
            assert(MMWeather_RainDensity() == 25 && rainGain > 0 && MMWeather_Overcast() == 1.0f);
            assert(rainDraws == beforeDraws + 1 && resets == beforeResets);
            assert(std::memcmp(&before, &play.envCtx, sizeof(before)) == 0);
            assert(gWeatherMode == weatherMode && gLightningStrike.state == lightningState);
        }
    }
    // Actual view/lifecycle boundaries still clear rain and audio.
    for (int condition = 0; condition < 8; ++condition) {
        play.envCtx = native;
        play.skyboxId = SKYBOX_NORMAL_SKY;
        camera.stateFlags = 0;
        play.cameraPtrs[0] = &camera;
        gSaveContext.gameMode = GAMEMODE_NORMAL;
        play.gameOverCtx.state = GAMEOVER_INACTIVE;
        switch (condition) {
            case 0:
                camera.stateFlags = CAM_STATE_UNDERWATER;
                break;
            case 1:
                play.skyboxId = SKYBOX_NONE;
                break;
            case 2:
                play.skyboxId = SKYBOX_2;
                break; // Fog-only backdrop, including interior boss rooms.
            case 3:
                play.skyboxId = SKYBOX_CUTSCENE_MAP;
                break;
            case 4:
                play.envCtx.skyboxDisabled = true;
                break;
            case 5:
                play.cameraPtrs[0] = nullptr;
                break;
            case 6:
                gSaveContext.gameMode = 1;
                break;
            case 7:
                play.gameOverCtx.state = 1;
                break;
        }
        AdvanceRain(&play);
        assert(MMWeather_RainDensity() == 0 && rainGain == 0 && MMWeather_Overcast() == 0);
    }
    gSaveContext.gameMode = GAMEMODE_NORMAL;
    gWeatherMode = WEATHER_MODE_CLEAR;
    gLightningStrike.state = LIGHTNING_STRIKE_WAIT;
    MMWeather_Reset();
    std::puts("PASS Termina Field, Woodfall and Clock Town outdoor overrides; view/lifecycle boundaries");
}

static void MayorsResidenceRegression() {
    static PlayState play{};
    Camera camera{};
    play.cameraPtrs[0] = &camera;
    play.skyboxId = SKYBOX_NORMAL_SKY;
    play.envCtx.stormState = STORM_STATE_OFF;
    gSaveContext.gameMode = GAMEMODE_NORMAL;
    settings[MM_WEATHER_CVAR("Enabled")] = 1;
    settings[MM_WEATHER_CVAR("Mode")] = 0;
    settings[MM_WEATHER_CVAR("RainVolume")] = 100;
    settings[MM_WEATHER_CVAR("Overcast")] = 1;
    settings[MM_WEATHER_CVAR("Thunder")] = 1;
    for (int mode = 0; mode < 2; ++mode) {
        settings[MM_WEATHER_CVAR("Mode")] = mode;
        play.sceneId = SCENE_TOWN;
        for (int i = 0; i < 1200; ++i) {
            MMWeather_Update(&play);
        }
        play.sceneId = SCENE_SONCHONOIE;
        for (int room = 0; room < 4; ++room) {
            play.roomCtx.curRoom.num = room;
            for (auto sky : { SKYBOX_NORMAL_SKY, SKYBOX_3 }) {
                play.skyboxId = sky;
                const auto native = play.envCtx;
                const int beforeDraws = rainDraws;
                for (int i = 0; i < 1200; ++i) {
                    MMWeather_Update(&play);
                    DrawWeatherFromPlay(&play);
                    assert(MMWeather_RainDensity() == 0 && rainGain == 0 && MMWeather_Overcast() == 0);
                    assert(sMMWeatherLightningBolt.state == LIGHTNING_BOLT_INACTIVE);
                }
                assert(rainDraws == beforeDraws);
                assert(std::memcmp(&native, &play.envCtx, sizeof(native)) == 0);
            }
        }
    }
    play.sceneId = SCENE_TOWN;
    settings[MM_WEATHER_CVAR("Mode")] = 0;
    AdvanceRain(&play);
    assert(MMWeather_RainDensity() == 25 && rainGain > 0);
    MMWeather_Reset();
    std::puts("PASS Mayor's Residence rooms suppress enhanced rain/audio/overcast/lightning; Clock Town resumes");
}

static void SkyOverrideRegression() {
    static PlayState play{};
    Camera camera{};
    GraphicsContext gfx{};
    Gfx commands[32]{};
    play.state.gfxCtx = &gfx;
    play.cameraPtrs[0] = &camera;
    play.sceneId = SCENE_21MITURINMAE;
    play.skyboxId = SKYBOX_3;
    play.envCtx.lightSettingOverride = LIGHT_SETTING_OVERRIDE_NONE;
    play.envCtx.skybox1Index = play.envCtx.skybox2Index = 99;
    D_801F4E74 = 1.0f; // Fully opaque regional gloom previously skipped sky updates.
    gSaveContext.skyboxTime = CLOCK_TIME(12, 0);
    AdvanceRain(&play);
    int beforeRebuilds = skyRebuilds;
    Environment_UpdateSkybox(play.skyboxId, &play.envCtx, &play.skyboxCtx);
    assert(play.envCtx.skybox2Index == 1 && play.envCtx.skyboxBlend == 255);
    assert(play.skyboxCtx.staticSegments[1][0] == sSkyboxTextures[1][0]);
    assert(play.envCtx.skybox1Index == 0 && skyRebuilds == beforeRebuilds + 1);
    // Every native day/time palette must still select a cloudy final texture,
    // including when BOTH slot bindings need to change in the same call.
    for (int config = 0; config < SKYBOX_CONFIG_MAX; ++config) {
        play.envCtx.skyboxConfig = config;
        for (int hour : { 0, 4, 5, 6, 8, 12, 16, 17, 18, 19, 20, 23 }) {
            gSaveContext.skyboxTime = CLOCK_TIME(hour, 0);
            play.envCtx.skybox1Index = play.envCtx.skybox2Index = 99;
            play.skyboxCtx.staticSegments[0][0] = play.skyboxCtx.staticSegments[1][0] = nullptr;
            beforeRebuilds = skyRebuilds;
            Environment_UpdateSkybox(play.skyboxId, &play.envCtx, &play.skyboxCtx);
            assert(play.envCtx.skyboxConfig == config && D_801F4E74 == 1.0f);
            assert(skyRebuilds == beforeRebuilds + 1);
            assert(play.skyboxCtx.staticSegments[0][0] == sSkyboxTextures[play.envCtx.skybox1Index][0]);
            assert(play.skyboxCtx.staticSegments[1][0] == sSkyboxTextures[play.envCtx.skybox2Index][0]);
            assert((play.envCtx.skybox2Index == 1 && play.envCtx.skyboxBlend == 255) ||
                   (play.envCtx.skybox1Index == 1 && play.envCtx.skyboxBlend == 0) ||
                   (play.envCtx.skybox1Index == 1 && play.envCtx.skybox2Index == 1));
        }
    }
    play.envCtx.skyboxConfig = SKYBOX_CONFIG_0;
    gSaveContext.skyboxTime = CLOCK_TIME(12, 0);
    play.envCtx.customSkyboxFilter = true;
    play.envCtx.skyboxFilterColor[3] = 200;
    play.lightCtx.fogNear = 950;
    auto before = play.envCtx;
    auto checkFilterAlpha = [&](int regional, int custom) {
        gfx.polyOpa.p = commands;
        Environment_DrawSkyboxFilters(&play);
        int count = 0;
        for (Gfx* command = commands; command != gfx.polyOpa.p; ++command) {
            if ((command->words.w0 >> 24) == G_SETPRIMCOLOR) {
                assert((command->words.w1 & 255) == static_cast<unsigned>(count == 0 ? regional : custom));
                ++count;
            }
        }
        assert(count == 2 && std::memcmp(&before, &play.envCtx, sizeof(before)) == 0);
    };
    checkFilterAlpha(0, 0);
    settings[MM_WEATHER_CVAR("Overcast")] = 0;
    MMWeather_Update(&play);
    Environment_UpdateSkybox(play.skyboxId, &play.envCtx, &play.skyboxCtx);
    // While fully gloomy the restored native filter covers the sky. As soon
    // as it reveals the sky, the original clear texture bindings return.
    D_801F4E74 = 0.5f;
    Environment_UpdateSkybox(play.skyboxId, &play.envCtx, &play.skyboxCtx);
    assert(play.envCtx.skybox1Index == 0 && play.envCtx.skybox2Index == 0);
    D_801F4E74 = 1.0f;
    before = play.envCtx;
    checkFilterAlpha(255, 200);
    settings[MM_WEATHER_CVAR("Overcast")] = 1;
    MMWeather_Reset();
    for (int i = 0; i < 10; ++i) {
        MMWeather_Update(&play);
    }
    checkFilterAlpha(127, 100); // Partial fade composes with both native filters.
    play.skyboxId = SKYBOX_NORMAL_SKY;
    gSaveContext.save.time = CLOCK_TIME(22, 0);
    gSkyboxNumStars = 1000;
    gfx.polyOpa.p = commands;
    Environment_SetupSkyboxStars(&play);
    assert(D_801F4F28 == 0.5f && sSkyboxStarsDList != nullptr);
    AdvanceRain(&play);
    Environment_SetupSkyboxStars(&play);
    assert(D_801F4F28 == 0.0f && sSkyboxStarsDList == nullptr);
    settings[MM_WEATHER_CVAR("Overcast")] = 0;
    MMWeather_Update(&play);
    Environment_SetupSkyboxStars(&play);
    assert(D_801F4F28 == 1.0f && sSkyboxStarsDList != nullptr);
    settings[MM_WEATHER_CVAR("Overcast")] = 1;
    D_801F4E74 = 0.0f;
    MMWeather_Reset();
    std::puts("PASS production sky: all day/time palettes, gloom, slot bindings, filters, stars and restoration");
}

static void SeasonWeatherRegression() {
    static PlayState play{};
    Camera camera{};
    play.sceneId = SCENE_TOWN;
    play.skyboxId = SKYBOX_NORMAL_SKY;
    play.cameraPtrs[0] = &camera;
    play.envCtx.stormState = STORM_STATE_OFF;
    play.envCtx.lightSettingOverride = LIGHT_SETTING_OVERRIDE_NONE;
    settings.clear();
    MMWeather_Reset();
    auto& nei = gSaveContext.save.shipSaveInfo.nei;
    nei.seasonsOwned = 0x0F;
    nei.season = SEASON_SPRING;
    gSaveContext.save.day = 1;
    const auto native = play.envCtx;
    MMWeather_Update(&play);
    assert(MMWeather_RainDensity() == 30 && rainGain > 0 && MMWeather_Overcast() > 0);
    assert(std::memcmp(&native, &play.envCtx, sizeof(native)) == 0);
    int draws = rainDraws;
    DrawWeatherFromPlay(&play);
    assert(rainDraws == draws + 1);
    // Default Spring owns persistent rain on all three days.
    for (int day : { 1, 2, 3 }) {
        gSaveContext.save.day = day;
        MMWeather_Update(&play);
        assert(MMWeather_RainDensity() == 30 && rainGain > 0);
        uint8_t first = 1, second = 1, blend = 255;
        MMWeather_ApplySky(&first, &second, &blend);
        assert(first == 0 && second == 1 && blend == 191); // Soft Spring rain sky, even over native CLOUD.
    }
    gSaveContext.save.day = 2;
    play.envCtx.stormState = STORM_STATE_ON;
    play.envCtx.precipitation[PRECIP_RAIN_MAX] = play.envCtx.precipitation[PRECIP_RAIN_CUR] = 60;
    play.envCtx.lightningState = LIGHTNING_ON;
    gWeatherMode = WEATHER_MODE_RAIN;
    Audio_SetAmbienceChannelIO(AMBIENCE_CHANNEL_RAIN, CHANNEL_IO_PORT_1, 1);
    Audio_SetAmbienceChannelIO(AMBIENCE_CHANNEL_LIGHTNING, CHANNEL_IO_PORT_1, 1);
    MMWeather_Update(&play);
    assert(MMWeather_RainDensity() == 30 && rainGain == 1.0f);
    draws = rainDraws;
    DrawWeatherFromPlay(&play);
    assert(rainDraws == draws + 1);
    auto storm = play.envCtx;
    nei.season = SEASON_SUMMER;
    assert(MMWeather_SeasonForPlay(&play) == SEASON_SUMMER); // before the cached update
    MMWeather_Update(&play);
    assert(MMWeather_SeasonClearsRain() && nativeRainAmbience == 0 && nativeThunderAmbience == 0);
    draws = rainDraws;
    DrawWeatherFromPlay(&play);
    assert(rainDraws == draws && std::memcmp(&storm, &play.envCtx, sizeof(storm)) == 0);
    assert(gWeatherMode == WEATHER_MODE_RAIN);
    uint8_t first = 1, second = 1, blend = 255;
    MMWeather_ApplySky(&first, &second, &blend);
    assert(first == 0 && second == 0 && blend == 0);
    for (int day : { 1, 2, 3 }) {
        gSaveContext.save.day = day;
        MMWeather_Update(&play);
        first = second = 1;
        blend = 255;
        MMWeather_ApplySky(&first, &second, &blend);
        assert(first == 0 && second == 0 && blend == 0);
        assert(MMWeather_SeasonClearsRain());
    }
    nei.season = SEASON_AUTUMN;
    MMWeather_Update(&play);
    assert(MMWeather_RainDensity() == 0 && MMWeather_Overcast() == 0);
    assert(nativeRainAmbience == 0 && nativeThunderAmbience == 0);
    draws = rainDraws;
    DrawWeatherFromPlay(&play);
    assert(rainDraws == draws);
    first = second = 1;
    MMWeather_ApplySky(&first, &second, &blend);
    assert(first == 0 && second == 0); // Autumn's dry cycle clears even a native Day 2 storm.
    play.envCtx.stormState = STORM_STATE_OFF;
    play.envCtx.precipitation[PRECIP_RAIN_CUR] = play.envCtx.precipitation[PRECIP_RAIN_MAX] = 0;
    gWeatherMode = WEATHER_MODE_CLEAR;
    first = second = 1;
    blend = 255;
    MMWeather_ApplySky(&first, &second, &blend);
    assert(first == 0 && second == 0 && blend == 0); // Ordinary dry native CLOUD clears.
    nei.season = SEASON_WINTER;
    MMWeather_Update(&play);
    assert(MMWeather_RainDensity() == 0 && rainGain == 0 && MMWeather_Overcast() > 0);
    first = second = blend = 0;
    MMWeather_ApplySky(&first, &second, &blend);
    assert(first == 1 && second == 1 && blend == 0); // Winter remains overcast over native FINE.
    nei.season = SEASON_SUMMER;
    MMWeather_Update(&play);
    assert(MMWeather_RainDensity() == 0 && MMWeather_Overcast() == 0);
    nei.season = SEASON_OFF;
    MMWeather_Update(&play);
    assert(MMWeather_RainDensity() == 0 && MMWeather_Overcast() == 0);
    nei.season = SEASON_SPRING;
    gSaveContext.save.day = 1;
    play.skyboxId = SKYBOX_NONE;
    MMWeather_Update(&play);
    assert(MMWeather_RainDensity() == 0 && rainGain == 0);
    play.skyboxId = SKYBOX_NORMAL_SKY;
    play.csCtx.state = 1;
    MMWeather_Update(&play);
    assert(MMWeather_Season() < 0 && MMWeather_RainDensity() == 0);
    play.csCtx.state = CS_STATE_IDLE;
    play.envCtx.lightSettingOverride = 1;
    MMWeather_Update(&play);
    assert(MMWeather_Season() < 0 && MMWeather_Overcast() == 0);
    // Explicit enhancement settings retain priority even during a story light override.
    settings[MM_WEATHER_CVAR("Enabled")] = 1;
    for (int i = 0; i < 25; ++i)
        MMWeather_Update(&play);
    assert(MMWeather_RainDensity() == 25 && rainGain > 0);
    nei.seasonsOwned = 0;
    std::memset(&play.envCtx, 0, sizeof(play.envCtx));
    gWeatherMode = WEATHER_MODE_CLEAR;
    MMWeather_Reset();
    std::puts("PASS seasonal rain, snow sky, clear weather, Off, interior/story restore and user override");
}

static void AutumnShowerRegression() {
    static PlayState play{};
    Camera camera{};
    play.sceneId = SCENE_00KEIKOKU;
    play.skyboxId = SKYBOX_NORMAL_SKY;
    play.cameraPtrs[0] = &camera;
    play.envCtx.lightSettingOverride = LIGHT_SETTING_OVERRIDE_NONE;
    settings.clear();
    MMWeather_Reset();
    auto& nei = gSaveContext.save.shipSaveInfo.nei;
    nei.seasonsOwned = 0x0F;
    nei.season = SEASON_AUTUMN;
    gSaveContext.save.day = 1;
    const auto native = play.envCtx;
    bool wet = false, dryAfterWet = false;
    for (int frame = 0; frame < 2500; ++frame) {
        ++play.gameplayFrames;
        MMWeather_Update(&play);
        const int density = MMWeather_RainDensity();
        assert(density >= 0 && density <= 15);
        assert(MMWeather_Overcast() <= 0.5f);
        assert(rainGain >= 0 && rainGain <= 0.6f);
        uint8_t first = 1, second = 1, blend = 255;
        MMWeather_ApplySky(&first, &second, &blend);
        const uint8_t cover = static_cast<uint8_t>(255.0f * MMWeather_Overcast());
        assert(first == 0 && blend == cover && second == (cover > 0 ? 1 : 0));
        if (density > 0) {
            wet = true;
            assert(rainGain > 0 && MMWeather_Overcast() > 0);
        } else if (wet)
            dryAfterWet = true;
        assert(std::memcmp(&native, &play.envCtx, sizeof(native)) == 0);
    }
    assert(wet && dryAfterWet);
    const int density = MMWeather_RainDensity();
    play.pauseCtx.state = PAUSE_STATE_MAIN;
    for (int frame = 0; frame < 100; ++frame)
        MMWeather_Update(&play);
    assert(MMWeather_RainDensity() == density);
    play.pauseCtx.state = PAUSE_STATE_OFF;
    nei.season = SEASON_OFF;
    MMWeather_Update(&play);
    assert(MMWeather_RainDensity() == 0 && rainGain == 0 && MMWeather_Overcast() == 0);
    nei.season = SEASON_AUTUMN;
    play.skyboxId = SKYBOX_NONE;
    for (int frame = 0; frame < 1000; ++frame)
        MMWeather_Update(&play);
    assert(MMWeather_RainDensity() == 0 && rainGain == 0);
    nei.seasonsOwned = 0;
    MMWeather_Reset();
    std::puts("PASS gentle autumn wet/dry cycle, audio/sky agreement, pause, Off and indoor cleanup");
}

static void GatedSeasonWeatherRegression() {
    static PlayState play{};
    Camera camera{};
    play.sceneId = SCENE_TOWN;
    play.skyboxId = SKYBOX_NORMAL_SKY;
    play.cameraPtrs[0] = &camera;
    play.envCtx.lightSettingOverride = LIGHT_SETTING_OVERRIDE_NONE;
    settings.clear();
    MMWeather_Reset();
    gSaveContext = {};
    gSaveContext.save.day = 1;
    gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
    gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[RO_ROD_OF_SEASONS] = NEI_SEASONS_GATED;
    auto& nei = *Nei_Save();
    nei.seasonsRodOwned = 1;
    nei.season = SEASON_OFF;
    assert(MMWeather_SeasonForPlay(&play) == -1 && nei.seasonsGates == 0);

    WEEKEVENTREG(WEEKEVENTREG_CLEARED_GREAT_BAY_TEMPLE >> 8) |= WEEKEVENTREG_CLEARED_GREAT_BAY_TEMPLE & 0xFF;
    nei.season = SEASON_SPRING;
    assert(MMWeather_SeasonForPlay(&play) == SEASON_SPRING);
    MMWeather_Update(&play);
    assert(MMWeather_Season() == SEASON_SPRING && MMWeather_RainDensity() == 30 && rainGain > 0);
    assert(nei.seasonsOwned == 0 && nei.seasonsGates == (1u << SEASON_SPRING));

    WEEKEVENTREG(WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE >> 8) |= WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE & 0xFF;
    Seasons_SetSeason(SEASON_WINTER);
    MMWeather_Update(&play);
    uint8_t first = 0, second = 0, blend = 255;
    MMWeather_ApplySky(&first, &second, &blend);
    assert(MMWeather_Season() == SEASON_WINTER && first == 1 && second == 1 && blend == 0);
    // Real completion adapters latch unlocks across a Song of Time flag reset.
    WEEKEVENTREG(WEEKEVENTREG_CLEARED_GREAT_BAY_TEMPLE >> 8) &= ~(WEEKEVENTREG_CLEARED_GREAT_BAY_TEMPLE & 0xFF);
    WEEKEVENTREG(WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE >> 8) &= ~(WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE & 0xFF);
    assert(MMWeather_SeasonForPlay(&play) == SEASON_WINTER);
    Seasons_SetSeason(SEASON_OFF);
    MMWeather_Update(&play);
    assert(MMWeather_Season() == -1 && MMWeather_RainDensity() == 0 && MMWeather_Overcast() == 0);
    // An invalid selection heals through the same available-season query as the wheel.
    nei.season = SEASON_SUMMER;
    assert(MMWeather_SeasonForPlay(&play) == SEASON_SPRING && nei.season == SEASON_SPRING);
    nei.seasonsRodOwned = 0;
    MMWeather_Update(&play);
    assert(MMWeather_Season() == -1 && MMWeather_RainDensity() == 0 && rainGain == 0);
    assert(nei.seasonsOwned == 0 && nei.seasonsRodOwned == 0 && nei.seasonsGates != 0);
    gSaveContext = {};
    MMWeather_Reset();
    std::puts("PASS gated Rod weather uses real completion unlocks, persisted gates, Off and Rod ownership");
}

static void ManualSeasonWeatherRegression() {
    R_UPDATE_RATE = 3;
    static PlayState play{};
    Camera camera{};
    play.sceneId = SCENE_TOWN;
    play.skyboxId = SKYBOX_NORMAL_SKY;
    play.cameraPtrs[0] = &camera;
    play.envCtx.lightSettingOverride = LIGHT_SETTING_OVERRIDE_NONE;
    settings.clear();
    MMWeather_Reset();
    gSaveContext = {};
    gSaveContext.save.day = 2;
    gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
    gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[RO_ROD_OF_SEASONS] = NEI_SEASONS_GATED;
    auto& nei = *Nei_Save();
    nei.season = SEASON_OFF;
    const auto progress = nei;
    const auto native = play.envCtx;
    assert(MMWeather_SeasonForPlay(&play) == -1); // Follow requires the Rod.
    for (int season : { SEASON_SPRING, SEASON_SUMMER, SEASON_AUTUMN, SEASON_WINTER }) {
        settings[MM_WEATHER_CVAR("Season")] = season;
        assert(MMWeather_SeasonForPlay(&play) == season);
        MMWeather_Update(&play);
        assert(MMWeather_Season() == season);
        assert(std::memcmp(&progress, &nei, sizeof(nei)) == 0);
        assert(std::memcmp(&native, &play.envCtx, sizeof(native)) == 0);
    }
    settings[MM_WEATHER_CVAR("Season")] = SEASON_SPRING;
    settings[MM_WEATHER_CVAR("Spring.RainMode")] = 1;
    AdvanceRain(&play);
    assert(MMWeather_RainDensity() == 30 && rainGain == 1.0f); // Even on Day 2.
    play.pauseCtx.state = PAUSE_STATE_MAIN;
    for (int i = 0; i < 100; ++i)
        MMWeather_Update(&play);
    assert(MMWeather_RainDensity() == 30);
    settings[MM_WEATHER_CVAR("Spring.RainMode")] = 0;
    MMWeather_Update(&play);
    assert(MMWeather_RainDensity() == 0 && rainGain == 0 && MMWeather_Overcast() == 0);
    settings[MM_WEATHER_CVAR("Spring.RainMode")] = 1;
    MMWeather_Update(&play);
    assert(MMWeather_RainDensity() == 0); // Paused controls do not advance the simulation.
    play.pauseCtx.state = PAUSE_STATE_OFF;
    AdvanceRain(&play);
    assert(MMWeather_RainDensity() == 30);
    play.pauseCtx.state = PAUSE_STATE_MAIN;
    settings[MM_WEATHER_CVAR("Season")] = -1;
    MMWeather_Update(&play);
    assert(MMWeather_Season() == -1 && MMWeather_RainDensity() == 0 && rainGain == 0);
    settings[MM_WEATHER_CVAR("Season")] = SEASON_AUTUMN;
    settings[MM_WEATHER_CVAR("Autumn.RainMode")] = 1;
    play.pauseCtx.state = PAUSE_STATE_OFF;
    AdvanceRain(&play);
    assert(MMWeather_RainDensity() == 15 && rainGain == 0.6f && MMWeather_Overcast() == 0.5f);
    settings[MM_WEATHER_CVAR("Season")] = SEASON_SPRING;
    AdvanceRain(&play);
    assert(MMWeather_RainDensity() == 30); // Each season retains its own rain setting.
    settings[MM_WEATHER_CVAR("Spring.RainMode")] = 2;
    bool wet = false, dryAfterWet = false;
    for (int frame = 0; frame < 2500; ++frame) {
        MMWeather_Update(&play);
        const int density = MMWeather_RainDensity();
        assert(density >= 0 && density <= 30);
        wet |= density > 0;
        dryAfterWet |= wet && density == 0;
    }
    assert(wet && dryAfterWet);
    for (int condition = 0; condition < 6; ++condition) {
        play.envCtx = native;
        play.csCtx.state = CS_STATE_IDLE;
        camera.stateFlags = 0;
        play.skyboxId = SKYBOX_NORMAL_SKY;
        switch (condition) {
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
        }
        assert(MMWeather_SeasonForPlay(&play) == -1);
        MMWeather_Update(&play);
        assert(MMWeather_RainDensity() == 0 && rainGain == 0);
    }
    play.envCtx = native;
    play.csCtx.state = CS_STATE_IDLE;
    camera.stateFlags = 0;
    play.skyboxId = SKYBOX_NORMAL_SKY;
    settings[MM_WEATHER_CVAR("Season")] = -2;
    assert(MMWeather_SeasonForPlay(&play) == -1);
    // Native bypasses the Rod resolver even when a gated Rod is available.
    nei.seasonsRodOwned = 1;
    nei.season = SEASON_SUMMER;
    const auto beforeNative = nei;
    settings[MM_WEATHER_CVAR("Season")] = -1;
    MMWeather_Update(&play);
    assert(MMWeather_Season() == -1 && std::memcmp(&beforeNative, &nei, sizeof(nei)) == 0);
    settings[MM_WEATHER_CVAR("Season")] = SEASON_SUMMER;
    Audio_SetAmbienceChannelIO(AMBIENCE_CHANNEL_RAIN, CHANNEL_IO_PORT_1, 1);
    Audio_SetAmbienceChannelIO(AMBIENCE_CHANNEL_LIGHTNING, CHANNEL_IO_PORT_1, 1);
    MMWeather_Update(&play);
    assert(nativeRainAmbience == 0 && nativeThunderAmbience == 0);
    play.pauseCtx.state = PAUSE_STATE_MAIN;
    settings[MM_WEATHER_CVAR("Season")] = -1;
    MMWeather_Update(&play);
    assert(nativeRainAmbience == 1 && nativeThunderAmbience == 1 && !MMWeather_SeasonClearsRain());
    // Explicit seasonal modes suppress actual native rain without changing its source.
    for (int selected : { SEASON_SPRING, SEASON_AUTUMN }) {
        settings[MM_WEATHER_CVAR("Season")] = selected;
        settings[selected == SEASON_SPRING ? MM_WEATHER_CVAR("Spring.RainMode") : MM_WEATHER_CVAR("Autumn.RainMode")] =
            0;
        play.envCtx.stormState = STORM_STATE_ON;
        play.envCtx.precipitation[PRECIP_RAIN_CUR] = play.envCtx.precipitation[PRECIP_RAIN_MAX] = 60;
        gWeatherMode = WEATHER_MODE_RAIN;
        const auto storm = play.envCtx;
        MMWeather_Update(&play);
        assert(MMWeather_SeasonClearsRain() && nativeRainAmbience == 0 && nativeThunderAmbience == 0);
        const int beforeDraws = rainDraws;
        DrawWeatherFromPlay(&play);
        assert(rainDraws == beforeDraws && MMWeather_RainDensity() == 0 && rainGain == 0);
        uint8_t first = 1, second = 1, blend = 255;
        MMWeather_ApplySky(&first, &second, &blend);
        assert(first == 0 && second == 0 && blend == 0);
        assert(std::memcmp(&storm, &play.envCtx, sizeof(storm)) == 0 && gWeatherMode == WEATHER_MODE_RAIN);
        play.csCtx.state = 1;
        MMWeather_Update(&play);
        assert(MMWeather_Season() == -1 && !MMWeather_SeasonClearsRain());
        assert(nativeRainAmbience == 1 && nativeThunderAmbience == 1);
        DrawWeatherFromPlay(&play);
        assert(rainDraws == beforeDraws + 1); // Story/native weather resumes, even while paused.
        play.csCtx.state = CS_STATE_IDLE;
    }
    play.envCtx = native;
    gWeatherMode = WEATHER_MODE_CLEAR;
    settings[MM_WEATHER_CVAR("Season")] = -1;
    // The independent global override still works with Native selected.
    play.pauseCtx.state = PAUSE_STATE_OFF;
    settings[MM_WEATHER_CVAR("Enabled")] = 1;
    AdvanceRain(&play);
    assert(MMWeather_RainDensity() == 25 && rainGain == 1.0f);
    settings.clear();
    gSaveContext = {};
    MMWeather_Reset();
    std::puts("PASS manual seasons without progression, per-season rain, paused selectors, eligibility and Native "
              "restoration");
}

static void DefaultSeasonPolicyRegression(bool checkSpring = true) {
    static PlayState play{};
    Camera camera{};
    play.sceneId = SCENE_00KEIKOKU;
    play.skyboxId = SKYBOX_NORMAL_SKY;
    play.cameraPtrs[0] = &camera;
    play.envCtx.lightSettingOverride = LIGHT_SETTING_OVERRIDE_NONE;
    settings.clear();
    R_UPDATE_RATE = 3;
    for (int day : { 2, 1, 3 }) {
        gSaveContext.save.day = day;
        for (bool storm : { true, false }) {
            play.envCtx.stormState = storm ? STORM_STATE_ON : STORM_STATE_OFF;
            play.envCtx.precipitation[PRECIP_RAIN_CUR] = play.envCtx.precipitation[PRECIP_RAIN_MAX] = storm ? 60 : 0;
            play.envCtx.lightningState = storm ? LIGHTNING_ON : LIGHTNING_OFF;
            gWeatherMode = storm ? WEATHER_MODE_RAIN : WEATHER_MODE_CLEAR;
            const auto native = play.envCtx;
            uint8_t first = 1, second = 1, blend = 255;
            if (checkSpring) {
                settings[MM_WEATHER_CVAR("Season")] = SEASON_SPRING;
                MMWeather_Reset();
                MMWeather_Update(&play);
                assert(MMWeather_RainDensity() == 30 && rainGain == 1.0f && MMWeather_Overcast() == 0.75f);
                assert(MMWeather_SeasonClearsRain());
                MMWeather_ApplySky(&first, &second, &blend);
                assert(first == 0 && second == 1 && blend == 191);
                assert(std::memcmp(&native, &play.envCtx, sizeof(native)) == 0);
            }

            settings[MM_WEATHER_CVAR("Season")] = SEASON_AUTUMN;
            MMWeather_Reset();
            bool wet = false, dry = false;
            for (int frame = 0; frame < 2500; ++frame) {
                MMWeather_Update(&play);
                first = second = 1;
                blend = 255;
                MMWeather_ApplySky(&first, &second, &blend);
                const int beforeDraws = rainDraws;
                DrawWeatherFromPlay(&play);
                if (MMWeather_RainDensity() > 0) {
                    wet = true;
                    assert(rainDraws == beforeDraws + 1 && rainGain > 0);
                } else {
                    dry |= rainGain == 0 && MMWeather_Overcast() == 0;
                    assert(rainDraws == beforeDraws);
                }
                const uint8_t cover = static_cast<uint8_t>(255.0f * MMWeather_Overcast());
                assert(first == 0 && second == (cover > 0 ? 1 : 0) && blend == cover);
                assert(MMWeather_SeasonClearsRain() && std::memcmp(&native, &play.envCtx, sizeof(native)) == 0);
            }
            assert(wet && dry);
            settings[MM_WEATHER_CVAR("Season")] = SEASON_WINTER;
            MMWeather_Update(&play);
            assert(MMWeather_SeasonClearsRain() && MMWeather_RainDensity() == 0 && rainGain == 0);
            first = second = blend = 0;
            MMWeather_ApplySky(&first, &second, &blend);
            assert(first == 1 && second == 1 && blend == 0);
        }
    }
    settings.clear();
    gSaveContext = {};
    gWeatherMode = WEATHER_MODE_CLEAR;
    MMWeather_Reset();
    std::puts("PASS Spring all three days, Autumn independent wet/dry clouds, Winter overcast despite native storms");
}

static uint8_t NativeOotCloudBlend(const PlayState& play) {
    const int blend = std::clamp<int>(play.envCtx.skyboxBlend, 0, 255);
    return (play.skyboxCtx.staticSegments[0][0] == sSkyboxTextures[1][0] ? 255 - blend : 0) +
           (play.skyboxCtx.staticSegments[1][0] == sSkyboxTextures[1][0] ? blend : 0);
}

static void OotDefaultCloudRegression() {
    static PlayState play{};
    Camera camera{};
    play.cameraPtrs[0] = &camera;
    settings.clear();
    settings[MM_WEATHER_CVAR("Season")] = -1;
    settings["gEnhancements.Graphics.UseOotSkyTextures"] = 1;
    gSaveContext = {};
    gWeatherMode = WEATHER_MODE_CLEAR;
    MMWeather_Reset();
    for (int scene : { SCENE_00KEIKOKU, SCENE_TOWN }) {
        for (int hour : { 12, 23 }) {
            play.envCtx = {};
            play.sceneId = scene;
            play.skyboxId = SKYBOX_NORMAL_SKY;
            play.csCtx.state = CS_STATE_IDLE;
            camera.stateFlags = 0;
            play.envCtx.lightSettingOverride = LIGHT_SETTING_OVERRIDE_NONE;
            play.envCtx.skyboxConfig = SKYBOX_CONFIG_25;
            play.envCtx.skybox1Index = play.envCtx.skybox2Index = 99;
            gSaveContext.skyboxTime = CLOCK_TIME(hour, 0);
            MMWeather_Update(&play);
            Environment_UpdateSkybox(play.skyboxId, &play.envCtx, &play.skyboxCtx);
            const auto native = play.envCtx;
            const auto bindings = play.skyboxCtx;
            assert(NativeOotCloudBlend(play) == (hour == 12 ? 255 : 0));
            assert(Skybox_GetOotCloudBlend(&play.skyboxCtx, play.envCtx.skyboxBlend) == 0);
            assert(std::memcmp(&native, &play.envCtx, sizeof(native)) == 0);
            assert(std::memcmp(&bindings, &play.skyboxCtx, sizeof(bindings)) == 0);
            for (int condition = 0; condition < 16; ++condition) {
                play.envCtx = native;
                play.sceneId = scene;
                play.skyboxId = SKYBOX_NORMAL_SKY;
                play.csCtx.state = CS_STATE_IDLE;
                camera.stateFlags = 0;
                gWeatherMode = WEATHER_MODE_CLEAR;
                switch (condition) {
                    case 0:
                        play.envCtx.stormState = STORM_STATE_ON;
                        break;
                    case 1:
                        gWeatherMode = WEATHER_MODE_RAIN;
                        break;
                    case 2:
                        play.envCtx.precipitation[PRECIP_RAIN_CUR] = 60;
                        break;
                    case 3:
                        play.envCtx.precipitation[PRECIP_RAIN_MAX] = 60;
                        break;
                    case 4:
                        play.envCtx.precipitation[PRECIP_SOS_MAX] = 32;
                        break;
                    case 5:
                        gWeatherMode = WEATHER_MODE_SNOW;
                        break;
                    case 6:
                        play.envCtx.precipitation[PRECIP_SNOW_CUR] = 128;
                        break;
                    case 7:
                        play.envCtx.precipitation[PRECIP_SNOW_MAX] = 128;
                        break;
                    case 8:
                        play.csCtx.state = 1;
                        break;
                    case 9:
                        play.envCtx.lightSettingOverride = 1;
                        break;
                    case 10:
                        play.envCtx.customSkyboxFilter = true;
                        break;
                    case 11:
                        camera.stateFlags = CAM_STATE_UNDERWATER;
                        break;
                    case 12:
                        play.sceneId = SCENE_SONCHONOIE;
                        break;
                    case 13:
                        play.skyboxId = SKYBOX_NONE;
                        break;
                    case 14:
                        play.skyboxId = SKYBOX_3;
                        break;
                    case 15:
                        play.envCtx.skyboxDisabled = true;
                        break;
                }
                MMWeather_Update(&play);
                assert(Skybox_GetOotCloudBlend(&play.skyboxCtx, play.envCtx.skyboxBlend) == NativeOotCloudBlend(play));
            }
        }
    }
    settings.clear();
    gWeatherMode = WEATHER_MODE_CLEAR;
    MMWeather_Reset();
    std::puts(
        "PASS real OoT getter clears dry config25 Field/Town noon/night, preserves weather/story/view boundaries");
}

static void OotEnhancedCloudRegression() {
    static PlayState play{};
    Camera camera{};
    play.cameraPtrs[0] = &camera;
    settings.clear();
    settings[MM_WEATHER_CVAR("Season")] = -1;
    settings[MM_WEATHER_CVAR("Enabled")] = 1;
    settings["gEnhancements.Graphics.UseOotSkyTextures"] = 1;
    gSaveContext = {};
    gWeatherMode = WEATHER_MODE_CLEAR;
    R_UPDATE_RATE = 3;
    for (int scene : { SCENE_00KEIKOKU, SCENE_TOWN }) {
        for (int hour : { 12, 23 }) {
            for (int mode : { 0, 1 }) {
                play.envCtx = {};
                play.sceneId = scene;
                play.skyboxId = SKYBOX_NORMAL_SKY;
                play.envCtx.lightSettingOverride = LIGHT_SETTING_OVERRIDE_NONE;
                play.envCtx.skyboxConfig = SKYBOX_CONFIG_25;
                play.envCtx.skybox1Index = play.envCtx.skybox2Index = 99;
                gSaveContext.skyboxTime = CLOCK_TIME(hour, 0);
                settings[MM_WEATHER_CVAR("Mode")] = mode;
                settings[MM_WEATHER_CVAR("Overcast")] = 1;
                MMWeather_Reset();
                bool wet = false, dryAfterWet = false;
                const int frames = mode == 1 ? 2500 : 25;
                for (int frame = 0; frame < frames; ++frame) {
                    MMWeather_Update(&play);
                    Environment_UpdateSkybox(play.skyboxId, &play.envCtx, &play.skyboxCtx);
                    const auto native = play.envCtx;
                    const auto bindings = play.skyboxCtx;
                    const uint8_t expected = static_cast<uint8_t>(255.0f * MMWeather_Overcast());
                    assert(Skybox_GetOotCloudBlend(&play.skyboxCtx, play.envCtx.skyboxBlend) == expected);
                    wet |= expected > 0;
                    dryAfterWet |= wet && expected == 0;
                    assert(std::memcmp(&native, &play.envCtx, sizeof(native)) == 0);
                    assert(std::memcmp(&bindings, &play.skyboxCtx, sizeof(bindings)) == 0);
                }
                assert(wet && (mode == 0 || dryAfterWet));
                settings[MM_WEATHER_CVAR("Overcast")] = 0;
                MMWeather_Update(&play);
                Environment_UpdateSkybox(play.skyboxId, &play.envCtx, &play.skyboxCtx);
                assert(Skybox_GetOotCloudBlend(&play.skyboxCtx, play.envCtx.skyboxBlend) == 0);
                for (int condition = 0; condition < 4; ++condition) {
                    play.envCtx.precipitation[PRECIP_RAIN_CUR] = 0;
                    play.envCtx.precipitation[PRECIP_SOS_MAX] = 0;
                    play.envCtx.precipitation[PRECIP_SNOW_MAX] = 0;
                    gWeatherMode = WEATHER_MODE_CLEAR;
                    if (condition == 0)
                        play.envCtx.precipitation[PRECIP_RAIN_CUR] = 60;
                    else if (condition == 1)
                        play.envCtx.precipitation[PRECIP_SOS_MAX] = 32;
                    else if (condition == 2)
                        play.envCtx.precipitation[PRECIP_SNOW_MAX] = 128;
                    else
                        gWeatherMode = WEATHER_MODE_SNOW;
                    MMWeather_Update(&play);
                    assert(Skybox_GetOotCloudBlend(&play.skyboxCtx, play.envCtx.skyboxBlend) ==
                           NativeOotCloudBlend(play));
                }
                gWeatherMode = WEATHER_MODE_CLEAR;
            }
        }
    }
    settings.clear();
    MMWeather_Reset();
    std::puts("PASS real OoT getter follows explicit overcast fade/dry cycle and checkbox with Native season selected");
}

static void NativeLightningProgressRegression() {
    static PlayState play{};
    Camera camera{};
    GraphicsContext gfx{};
    Gfx opaque[128]{}, translucent[128]{};
    play.state.gfxCtx = &gfx;
    play.sceneId = SCENE_00KEIKOKU;
    play.skyboxId = SKYBOX_NORMAL_SKY;
    play.cameraPtrs[0] = &camera;
    play.view.at.z = 1.0f;
    play.lightCtx.fogNear = 1000;
    play.envCtx.lightSettingOverride = LIGHT_SETTING_OVERRIDE_NONE;
    play.envCtx.lightningState = LIGHTNING_ON;
    play.envCtx.adjLightSettings.ambientColor[0] = 80;
    play.envCtx.adjLightSettings.ambientColor[1] = 80;
    play.envCtx.adjLightSettings.ambientColor[2] = 100;
    settings.clear();
    settings[MM_WEATHER_CVAR("Season")] = SEASON_SUMMER;
    MMWeather_Reset();
    MMWeather_Update(&play);
    gLightningStrike = {};
    gLightningStrike.state = LIGHTNING_STRIKE_END;
    sLightningFlashAlpha = 100;
    for (auto& bolt : sLightningBolts)
        bolt.state = LIGHTNING_BOLT_INACTIVE;
    Environment_AddLightningBolts(&play, 1);
    Rand_Seed(0x13579);
    const auto beforeRandom = sRandInt;
    const int beforeFlash = flashDraws;
    gfx.polyOpa.p = opaque;
    gfx.polyXlu.p = translucent;
    Environment_Draw(&play); // Real Summer draw path must advance END and native bolt START.
    assert(sLightningFlashAlpha == 90 && play.envCtx.adjLightSettings.ambientColor[0] == 70);
    assert(play.envCtx.adjLightSettings.ambientColor[1] == 70 && play.envCtx.adjLightSettings.ambientColor[2] == 90);
    assert(sLightningBolts[0].state == LIGHTNING_BOLT_WAIT && sRandInt != beforeRandom);
    assert(flashDraws == beforeFlash);
    for (int frame = 0; frame < 25; ++frame) {
        gfx.polyOpa.p = opaque;
        gfx.polyXlu.p = translucent;
        Environment_Draw(&play);
    }
    assert(gLightningStrike.state == LIGHTNING_STRIKE_WAIT && sLightningBolts[0].state == LIGHTNING_BOLT_INACTIVE);
    settings[MM_WEATHER_CVAR("Season")] = -1;
    MMWeather_Update(&play);
    const int beforeGeometry = boltDraws;
    gfx.polyOpa.p = opaque;
    gfx.polyXlu.p = translucent;
    Environment_Draw(&play);
    assert(boltDraws == beforeGeometry && flashDraws == beforeFlash); // No replay after Off.
    settings.clear();
    gSaveContext = {};
    MMWeather_Reset();
    std::puts("PASS real Summer draw advances native END, bolts and original RNG without flash or Off replay");
}

struct NativeLightningFrame {
    LightningStrike strike;
    AdjLightSettings lights;
    LightningBolt bolts[3];
    s16 alpha;
    u32 random;
};

static std::vector<NativeLightningFrame> NativeLightningTrace(int season) {
    static PlayState play{};
    Camera camera{};
    GraphicsContext gfx{};
    Gfx opaque[128]{}, translucent[128]{};
    play = {};
    play.state.gfxCtx = &gfx;
    play.sceneId = SCENE_TOWN;
    play.skyboxId = SKYBOX_NORMAL_SKY;
    play.cameraPtrs[0] = &camera;
    play.view.at.z = 1.0f;
    play.lightCtx.fogNear = 1000;
    play.envCtx.lightSettingOverride = LIGHT_SETTING_OVERRIDE_NONE;
    play.envCtx.lightningState = LIGHTNING_ON;
    play.envCtx.adjLightSettings.ambientColor[0] = 20;
    play.envCtx.adjLightSettings.ambientColor[1] = -10;
    play.envCtx.adjLightSettings.ambientColor[2] = -30;
    settings.clear();
    settings[MM_WEATHER_CVAR("Season")] = season;
    MMWeather_Reset();
    MMWeather_Update(&play);
    gLightningStrike = {};
    gLightningStrike.delayTimer = 501.0f;
    sLightningFlashAlpha = 0;
    std::memset(sLightningBolts, 0, sizeof(sLightningBolts));
    for (auto& bolt : sLightningBolts)
        bolt.state = LIGHTNING_BOLT_INACTIVE;
    Rand_Seed(0x2369857);
    std::vector<NativeLightningFrame> trace;
    for (int frame = 0; frame < 36; ++frame) {
        if (frame == 8) {
            play.envCtx.adjLightSettings.ambientColor[0] -= 220;
            play.envCtx.adjLightSettings.ambientColor[1] -= 30;
            play.envCtx.adjLightSettings.ambientColor[2] -= 200;
        }
        gfx.polyOpa.p = opaque;
        gfx.polyXlu.p = translucent;
        Environment_Draw(&play);
        NativeLightningFrame captured{};
        captured.strike = gLightningStrike;
        captured.lights = play.envCtx.adjLightSettings;
        std::memcpy(captured.bolts, sLightningBolts, sizeof(sLightningBolts));
        captured.alpha = sLightningFlashAlpha;
        captured.random = sRandInt;
        trace.push_back(captured);
    }
    return trace;
}

static void NativeLightningParityRegression() {
    const int initialGeometry = boltDraws;
    const int initialThunder = thunderTriggers;
    const auto off = NativeLightningTrace(-1);
    assert(boltDraws > initialGeometry && thunderTriggers == initialThunder + 1);
    const int nativeGeometry = boltDraws;
    const int nativeThunder = thunderTriggers;
    const int nativeFlashes = flashDraws;
    const auto summer = NativeLightningTrace(SEASON_SUMMER);
    assert(boltDraws == nativeGeometry && thunderTriggers == nativeThunder && flashDraws == nativeFlashes);
    assert(off.size() == summer.size());
    for (size_t frame = 0; frame < off.size(); ++frame) {
        const auto& a = off[frame];
        const auto& b = summer[frame];
        assert(a.strike.state == b.strike.state && a.strike.delayTimer == b.strike.delayTimer);
        assert(a.strike.flashAlphaTarget == b.strike.flashAlphaTarget && a.alpha == b.alpha && a.random == b.random);
        assert(std::memcmp(&a.lights, &b.lights, sizeof(a.lights)) == 0);
        for (int i = 0; i < 3; ++i) {
            assert(a.bolts[i].state == b.bolts[i].state && a.bolts[i].delayTimer == b.bolts[i].delayTimer);
            assert(a.bolts[i].textureIndex == b.bolts[i].textureIndex);
            assert(a.bolts[i].pitch == b.bolts[i].pitch && a.bolts[i].roll == b.bolts[i].roll);
            assert(std::memcmp(&a.bolts[i].pos, &b.bolts[i].pos, sizeof(Vec3f)) == 0);
            assert(std::memcmp(&a.bolts[i].offset, &b.bolts[i].offset, sizeof(Vec3f)) == 0);
        }
    }
    settings.clear();
    MMWeather_Reset();
    std::puts("PASS Off/Summer native strike, raw fades, bolt lifecycle and RNG parity; flash/thunder/geometry mask");
}

static void AssertNativeAmbient(s16 red, s16 green, s16 blue) {
    s16 contribution[3];
    Environment_GetNativeLightningAmbient(contribution);
    assert(contribution[0] == red && contribution[1] == green && contribution[2] == blue);
}

static void NativeAmbientOwnershipRegression() {
    static PlayState play{};
    Camera camera{};
    play.sceneId = SCENE_TOWN;
    play.skyboxId = SKYBOX_NORMAL_SKY;
    play.cameraPtrs[0] = &camera;
    play.envCtx.lightSettingOverride = LIGHT_SETTING_OVERRIDE_NONE;
    play.envCtx.lightningState = LIGHTNING_ON;
    settings.clear();
    settings[MM_WEATHER_CVAR("Season")] = SEASON_SUMMER;
    MMWeather_Reset();
    MMWeather_Update(&play);
    Environment_ClearNativeLightningAmbient();
    const s16 offset[] = { 20, -30, 5 };
    for (int i = 0; i < 3; ++i) {
        play.envCtx.lightSettings.ambientColor[i] = 100 + 10 * i;
        play.envCtx.adjLightSettings.ambientColor[i] = offset[i];
    }
    gLightningStrike = {};
    gLightningStrike.state = LIGHTNING_STRIKE_START;
    gLightningStrike.flashAlphaTarget = 200;
    sLightningFlashAlpha = 0;
    Environment_UpdateLightningStrike(&play);
    AssertNativeAmbient(80, 80, 100);
    Environment_UpdateLightningStrike(&play);
    AssertNativeAmbient(160, 160, 200);
    assert(play.envCtx.adjLightSettings.ambientColor[0] == 180);
    assert(play.envCtx.adjLightSettings.ambientColor[1] == 130);
    assert(play.envCtx.adjLightSettings.ambientColor[2] == 205);
    const auto raw = play.envCtx.adjLightSettings;
    DisplayAmbientFromEnvironment(&play);
    assert(play.lightCtx.ambientColor[0] == 120 && play.lightCtx.ambientColor[1] == 80 &&
           play.lightCtx.ambientColor[2] == 125);
    assert(std::memcmp(&raw, &play.envCtx.adjLightSettings, sizeof(raw)) == 0);
    Environment_UpdateLightningStrike(&play);
    AssertNativeAmbient(150, 150, 190);
    DisplayAmbientFromEnvironment(&play);
    assert(play.lightCtx.ambientColor[0] == 120 && play.lightCtx.ambientColor[1] == 80 &&
           play.lightCtx.ambientColor[2] == 125);

    // END follows raw red for both red/green; blue uses its own raw condition.
    const s16 unequal[] = { 10, 20, 30 };
    Environment_RestoreNativeLightningAmbient(unequal);
    play.envCtx.adjLightSettings.ambientColor[0] = -1;
    play.envCtx.adjLightSettings.ambientColor[1] = 50;
    play.envCtx.adjLightSettings.ambientColor[2] = 11;
    sLightningFlashAlpha = 100;
    Environment_UpdateLightningStrike(&play);
    AssertNativeAmbient(10, 20, 20);
    assert(play.envCtx.adjLightSettings.ambientColor[0] == -1 && play.envCtx.adjLightSettings.ambientColor[1] == 50 &&
           play.envCtx.adjLightSettings.ambientColor[2] == 1);
    const s16 tail[] = { 4, 6, 20 };
    Environment_RestoreNativeLightningAmbient(tail);
    play.envCtx.adjLightSettings.ambientColor[0] = 1;
    play.envCtx.adjLightSettings.ambientColor[1] = -25;
    play.envCtx.adjLightSettings.ambientColor[2] = -5;
    Environment_UpdateLightningStrike(&play);
    AssertNativeAmbient(0, 0, 20);
    assert(play.envCtx.adjLightSettings.ambientColor[0] == -9 && play.envCtx.adjLightSettings.ambientColor[1] == -35 &&
           play.envCtx.adjLightSettings.ambientColor[2] == -5);
    gLightningStrike.state = LIGHTNING_STRIKE_WAIT;
    gLightningStrike.delayTimer = 501.0f;
    Environment_UpdateLightningStrike(&play);
    AssertNativeAmbient(0, 0, 20); // WAIT->START must retain a restored/raw contribution.

    const s16 owned[] = { 80, 80, 100 };
    Environment_RestoreNativeLightningAmbient(owned);
    for (int condition = 0; condition < 4; ++condition) {
        static Player player{};
        play.roomCtx.curRoom.type = ROOM_TYPE_NORMAL;
        play.unk_18880 = 0;
        cameraNotFixed = true;
        play.actorCtx.actorLists[ACTORCAT_PLAYER].first = nullptr;
        if (condition == 0)
            play.roomCtx.curRoom.type = ROOM_TYPE_BOSS;
        else if (condition == 1)
            cameraNotFixed = false;
        else if (condition == 2)
            play.unk_18880 = 1;
        else {
            player.stateFlags1 = PLAYER_STATE1_2;
            play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
        }
        assert(Environment_AdjustLights(&play, 0.5f, 900.0f, 0.1f, 1.0f) == 0);
        AssertNativeAmbient(80, 80, 100);
    }
    play.roomCtx.curRoom.type = ROOM_TYPE_NORMAL;
    play.unk_18880 = 0;
    cameraNotFixed = true;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = nullptr;
    assert(Environment_AdjustLights(&play, 0.5f, 900.0f, 0.1f, 0.0f) == 1);
    AssertNativeAmbient(80, 80, 100); // Fog-only spin lighting does not replace ambient.
    assert(Environment_AdjustLights(&play, 0.5f, 900.0f, 0.1f, 1.0f) == 1);
    AssertNativeAmbient(0, 0, 0);
    Environment_RestoreNativeLightningAmbient(owned);
    Color_RGB8 target = { 20, 70, 150 };
    Environment_LerpAmbientColor(&play, &target, 1.0f);
    AssertNativeAmbient(0, 0, 0);
    assert(play.envCtx.adjLightSettings.ambientColor[0] == -80 && play.envCtx.adjLightSettings.ambientColor[1] == -40 &&
           play.envCtx.adjLightSettings.ambientColor[2] == 30);

    for (int actor = 0; actor < 2; ++actor) {
        auto replace = actor == 0 ? ReplaceBomAmbient : ReplaceBombfAmbient;
        auto fade = actor == 0 ? FadeBomAmbient : FadeBombfAmbient;
        Environment_RestoreNativeLightningAmbient(owned);
        replace(&play); // Actual full overwrite at the bomb explosion site.
        AssertNativeAmbient(0, 0, 0);
        gLightningStrike.state = LIGHTNING_STRIKE_START;
        gLightningStrike.flashAlphaTarget = 100;
        sLightningFlashAlpha = 0;
        Environment_UpdateLightningStrike(&play);
        AssertNativeAmbient(80, 80, 100);
        fade(&play); // Actual additive fade keeps the native contribution owned.
        AssertNativeAmbient(80, 80, 100);
        for (int i = 0; i < 3; ++i)
            assert(Environment_GetAmbientAdjustment(&play.envCtx, i) == 225);
        Environment_UpdateLightningStrike(&play);
        AssertNativeAmbient(70, 70, 90);
        for (int i = 0; i < 3; ++i)
            assert(Environment_GetAmbientAdjustment(&play.envCtx, i) == 225);
        replace(&play);
        AssertNativeAmbient(0, 0, 0);
        Environment_UpdateLightningStrike(&play);
        AssertNativeAmbient(0, 0, 0);
        for (int i = 0; i < 3; ++i)
            assert(Environment_GetAmbientAdjustment(&play.envCtx, i) == 240);
    }

    Environment_ClearNativeLightningAmbient();
    for (int i = 0; i < 3; ++i)
        play.envCtx.adjLightSettings.ambientColor[i] = 10;
    gLightningStrike.state = LIGHTNING_STRIKE_START;
    gLightningStrike.flashAlphaTarget = 100;
    sLightningFlashAlpha = 0;
    Environment_UpdateLightningStrike(&play);
    const auto snapshot = play.envCtx.adjLightSettings;
    SnapshotPlayerAmbient(&play); // Actual transformation snapshot statements.
    struct_8085D848_unk_00 transformLights[3] = {
        { 650, { 0, 0, 0 }, { 10, 20, 30 } },
        { 300, { 200, 200, 255 }, { 0, 0, 0 } },
        { 600, { 0, 0, 0 }, { 0, 0, 200 } },
    };
    func_80854EFC(&play, 0.5f, transformLights); // Actual full interpolation replaces ownership.
    AssertNativeAmbient(0, 0, 0);
    RestorePlayerAmbient(&play);
    AssertNativeAmbient(80, 80, 100);
    assert(std::memcmp(&snapshot, &play.envCtx.adjLightSettings, sizeof(snapshot)) == 0);
    for (int i = 0; i < 3; ++i)
        assert(Environment_GetAmbientAdjustment(&play.envCtx, i) == 10);
    settings[MM_WEATHER_CVAR("Season")] = -1;
    MMWeather_Update(&play);
    for (int i = 0; i < 3; ++i) {
        assert(Environment_GetAmbientAdjustment(&play.envCtx, i) == snapshot.ambientColor[i]);
    }
    Environment_ClearNativeLightningAmbient();
    settings.clear();
    MMWeather_Reset();
    std::puts(
        "PASS actual native ambient adds/conditional fades, overwrite/additive bombs, fog returns, player snapshot");
}

static void EnhancedLightningIndependenceRegression() {
    static PlayState play{};
    Camera camera{};
    GraphicsContext gfx{};
    Gfx opaque[128]{}, translucent[128]{};
    play.state.gfxCtx = &gfx;
    play.sceneId = SCENE_TOWN;
    play.skyboxId = SKYBOX_NORMAL_SKY;
    play.cameraPtrs[0] = &camera;
    play.view.at.z = 1.0f;
    play.lightCtx.fogNear = 1000;
    play.envCtx.lightSettingOverride = LIGHT_SETTING_OVERRIDE_NONE;
    play.envCtx.lightningState = LIGHTNING_OFF;
    settings.clear();
    settings[MM_WEATHER_CVAR("Season")] = SEASON_SUMMER;
    settings[MM_WEATHER_CVAR("Enabled")] = 1;
    R_UPDATE_RATE = 3;
    MMWeather_Reset();
    AdvanceRain(&play);
    gLightningStrike = {};
    std::memset(sLightningBolts, 0, sizeof(sLightningBolts));
    for (auto& bolt : sLightningBolts)
        bolt.state = LIGHTNING_BOLT_INACTIVE;
    Environment_AddLightningBolts(&play, 1);
    MMWeather_StartBolt();
    Rand_Seed(101);
    for (int i = 0; i < 5; ++i)
        Rand_ZeroOne();
    const auto expectedRandom = sRandInt;
    Rand_Seed(101);
    const int beforeGeometry = boltDraws;
    for (int frame = 0; frame < 5; ++frame) {
        gfx.polyOpa.p = opaque;
        gfx.polyXlu.p = translucent;
        Environment_Draw(&play);
    }
    assert(sRandInt == expectedRandom && boltDraws == beforeGeometry + 2);
    assert(sLightningBolts[0].state == LIGHTNING_BOLT_DRAW && sMMWeatherLightningBolt.state == LIGHTNING_BOLT_DRAW);
    const int beforeEnhancedFlash = flashDraws;
    for (int frame = 0; frame < 3000 && flashDraws == beforeEnhancedFlash; ++frame) {
        MMWeather_Update(&play);
        MMWeather_DrawLightning(&play);
    }
    assert(flashDraws > beforeEnhancedFlash);
    settings.clear();
    MMWeather_Reset();
    std::puts("PASS enhanced bolt geometry and flash remain independent while masked native slots/RNG advance");
}

int main(int argc, char** argv) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    if (argc == 2) {
        if (std::strcmp(argv[1], "season-defaults") == 0)
            DefaultSeasonPolicyRegression();
        else if (std::strcmp(argv[1], "autumn-native-cycle") == 0)
            DefaultSeasonPolicyRegression(false);
        else if (std::strcmp(argv[1], "oot-default-cloud") == 0)
            OotDefaultCloudRegression();
        else if (std::strcmp(argv[1], "oot-enhanced-cloud") == 0)
            OotEnhancedCloudRegression();
        else if (std::strcmp(argv[1], "lightning-progress") == 0)
            NativeLightningProgressRegression();
        else if (std::strcmp(argv[1], "lightning-parity") == 0)
            NativeLightningParityRegression();
        else if (std::strcmp(argv[1], "ambient-ownership") == 0)
            NativeAmbientOwnershipRegression();
        else if (std::strcmp(argv[1], "enhanced-lightning") == 0)
            EnhancedLightningIndependenceRegression();
        else
            assert(false);
        return 0;
    }
    AutumnShowerRegression();
    static PlayState play{};
    Camera camera{};
    play.cameraPtrs[0] = &camera;
    play.skyboxId = SKYBOX_NORMAL_SKY;
    play.envCtx.stormState = STORM_STATE_ON;
    play.envCtx.lightSettingOverride = LIGHT_SETTING_OVERRIDE_NONE;
    R_UPDATE_RATE = 3;
    for (auto& bolt : sLightningBolts) {
        bolt.state = LIGHTNING_BOLT_INACTIVE;
    }
    auto native = play.envCtx;
    settings["gAudioEditor.GlobalOutdoorRain"] = 1;
    settings["gAudioEditor.GlobalOutdoorRainMode"] = 1;
    for (int i = 0; i < 25; ++i) {
        MMWeather_Update(&play);
    }
    assert(MMWeather_RainDensity() == 0 && rainGain == 0);
    settings[MM_WEATHER_CVAR("Enabled")] = 1;
    for (int i = 0; i < 25; ++i) {
        MMWeather_Update(&play);
    }
    assert(MMWeather_RainDensity() == 25 && rainGain > 0);
    DrawWeatherFromPlay(&play);
    assert(rainDraws == 1); // Actual Play draw caller must admit enhanced-only rain.
    settings[MM_WEATHER_CVAR("RainVolume")] = 0;
    MMWeather_Update(&play);
    assert(rainGain == 0 && MMWeather_RainDensity() == 25);
    settings[MM_WEATHER_CVAR("RainVolume")] = 100;
    settings[MM_WEATHER_CVAR("Overcast")] = 0;
    MMWeather_Update(&play);
    assert(MMWeather_Overcast() == 0 && MMWeather_Shade(255) == 255);
    settings[MM_WEATHER_CVAR("Overcast")] = 1;
    MMWeather_Update(&play);
    assert(std::memcmp(&native, &play.envCtx, sizeof(native)) == 0);
    assert(gWeatherMode == WEATHER_MODE_CLEAR && gLightningStrike.state == LIGHTNING_STRIKE_WAIT);
    uint8_t first = 0, second = 0, blend = 0;
    MMWeather_ApplySky(&first, &second, &blend);
    assert(first == 0 && second == 1 && blend == 255);

    play.envCtx = native;
    for (int i = 0; i < 10; ++i) {
        MMWeather_Update(&play);
    }
    int density = MMWeather_RainDensity();
    play.pauseCtx.state = PAUSE_STATE_MAIN;
    for (int i = 0; i < 1000; ++i) {
        MMWeather_Update(&play);
    }
    assert(MMWeather_RainDensity() == density);
    MMWeather_Reset();
    assert(MMWeather_RainDensity() == 0 && rainGain == 0 && resets > 0);
    // Same PlayState address and same scene after reset must start a new fade.
    play.pauseCtx.state = PAUSE_STATE_OFF;
    MMWeather_Update(&play);
    assert(MMWeather_RainDensity() < 5);
    play.roomCtx.curRoom.num++;
    MMWeather_Update(&play);
    assert(MMWeather_RainDensity() < 5);
    for (int i = 0; i < 600; ++i) {
        MMWeather_Update(&play);
    }
    for (const auto& bolt : sLightningBolts) {
        assert(bolt.state == LIGHTNING_BOLT_INACTIVE); // Added strikes cannot occupy native slots.
    }
    assert(sMMWeatherLightningBolt.state != LIGHTNING_BOLT_INACTIVE);
    camera.stateFlags = CAM_STATE_UNDERWATER;
    MMWeather_Update(&play);
    assert(sMMWeatherLightningBolt.state == LIGHTNING_BOLT_INACTIVE);
    camera.stateFlags = 0;
    Environment_AddLightningBolts(&play, 1);
    MMWeather_Reset();
    assert(sLightningBolts[0].state == LIGHTNING_BOLT_START); // Native slots survive enhancement cleanup.
    play.envCtx = native;
    for (int i = 0; i < 600; ++i) {
        MMWeather_Update(&play);
    }
    assert(sMMWeatherLightningBolt.state != LIGHTNING_BOLT_INACTIVE);
    MMWeather_Reset();
    assert(sMMWeatherLightningBolt.state == LIGHTNING_BOLT_INACTIVE);
    std::puts("PASS MM weather bridge: native state preservation, sky, pause, scene/room reset");
    SpinAttackRainRegression();
    OutdoorOverrideRegression();
    SkyOverrideRegression();
    MayorsResidenceRegression();
    SeasonWeatherRegression();
    GatedSeasonWeatherRegression();
    ManualSeasonWeatherRegression();
    DefaultSeasonPolicyRegression();
    OotDefaultCloudRegression();
    OotEnhancedCloudRegression();
    NativeLightningProgressRegression();
    NativeLightningParityRegression();
    NativeAmbientOwnershipRegression();
    EnhancedLightningIndependenceRegression();
}
