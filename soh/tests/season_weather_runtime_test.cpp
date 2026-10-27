#include "test_require.h"
#include "z64.h"
#include "mods/extended_inventory.h"
#include "soh/Enhancements/audio/GlobalOutdoorRain.h"
#include "soh/Enhancements/audio/GlobalOutdoorRainBridge.h"
#include "soh/Enhancements/audio/SeasonWeather.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "overlays/actors/ovl_En_Weather_Tag/z_en_weather_tag.h"
#include "overlays/actors/ovl_Door_Ana/z_door_ana.h"
#include <cmath>
#include <cstring>
#include <map>
#include <string>
#include <vector>

// Weak declarations let the first run fail on the absent behavior, before the
// new presentation controller is implemented.
extern "C" {
int32_t SeasonWeather_SeasonForPlay(const PlayState*) __attribute__((weak));
int32_t SeasonWeather_ParticleSeasonForPlay(const PlayState*) __attribute__((weak));
int32_t SeasonWeather_ParticlesEnabled(int32_t) __attribute__((weak));
int32_t SeasonWeather_FlurriesEnabled(void) __attribute__((weak));
int32_t SeasonWeather_SummerEffectEnabled(int32_t) __attribute__((weak));
int32_t SeasonWeather_LightOffset(const PlayState*, int32_t, int32_t) __attribute__((weak));
void SeasonWeather_Update(PlayState*) __attribute__((weak));
void SeasonWeather_Reset(void) __attribute__((weak));
void SeasonWeather_RecordLighting(PlayState*) __attribute__((weak));
void SeasonWeather_TestRodBridge(PlayState*);
void DoorAna_WaitClosed(DoorAna*, PlayState*);
void DoorAna_WaitOpen(DoorAna*, PlayState*);
void Seasons_PrecipTint(u8*, u8*, u8*);
void func_800766C4(PlayState*);
void func_8006FB94(EnvironmentContext*, u16);
void SeasonWeather_TestBombFade(PlayState*);
void SeasonWeather_TestBombFlash(PlayState*);
void SeasonWeather_TestNativeLighting(PlayState*);
void SeasonWeather_TestNativeOutdoorLighting(PlayState*);
void SeasonWeather_TestNativeSkyBlend(EnvironmentContext*);
void Environment_UpdateLightningStrike(PlayState*);
void EnWeatherTag_Init(Actor*, PlayState*);
void EnWeatherTag_Destroy(Actor*, PlayState*);
void EnWeatherTag_Update(Actor*, PlayState*);
void EnWeatherTag_DisabledRainLakeHylia(EnWeatherTag*, PlayState*);
void EnWeatherTag_EnabledRainLakeHylia(EnWeatherTag*, PlayState*);
void EnWeatherTag_DisabledCloudyRainThunderKakariko(EnWeatherTag*, PlayState*);
void EnWeatherTag_EnabledCloudyRainThunderKakariko(EnWeatherTag*, PlayState*);
}

GameInteractor* GameInteractor::Instance = nullptr;
extern "C" {
SaveContext gSaveContext = {};
PlayState* gPlayState = nullptr;
LightningStrike gLightningStrike = {};
s16 sLightningFlashAlpha = 0;
s32 sLightningBolts[3]; // the real flash handler only consumes this array's count
u8 gWeatherMode = 0, gSkyboxBlendingEnabled = 0, D_8011FB34 = 0, D_8011FB38 = 0;
u16 gTimeSpeed = 0;
static GameInfo sGameInfo = {};
GameInfo* gGameInfo = &sGameInfo;
u32 gBitFlags[32];
Vec3f gSfxDefaultPos = {};
f32 gSfxDefaultFreqAndVolScale = 1.0f;
s8 gSfxDefaultReverb = 0;
static int sPuzzleChimes;
// The real unopened-grotto decision runs below; audio output and the unrelated
// bomb-collider path are boundaries of this headless seasonal fixture.
void DoorAna_WaitOpen(DoorAna*, PlayState*) {
}
void Audio_PlaySfxGeneral(u16 sfxId, Vec3f*, u8, f32*, f32*, s8*) {
    REQUIRE(sfxId == NA_SE_SY_CORRECT_CHIME);
    ++sPuzzleChimes;
}
s32 Collider_DestroyCylinder(PlayState*, ColliderCylinder*) {
    return 0;
}
void Collider_UpdateCylinder(Actor*, ColliderCylinder*) {
}
s32 CollisionCheck_SetAC(PlayState*, CollisionCheckContext*, Collider*) {
    return 0;
}
void Actor_SetClosestSecretDistance(Actor*, PlayState*) {
}
// Platform and unrelated sandstorm/debug IO boundaries. The tested rain and
// cloud actor functions, range checks and event flags are production code.
f32 Actor_WorldDistXZToActor(Actor* a, Actor* b) {
    return std::hypot(a->world.pos.x - b->world.pos.x, a->world.pos.z - b->world.pos.z);
}
void Actor_Kill(Actor* actor) {
    actor->update = nullptr;
}
s16 Math_SmoothStepToS(s16*, s16, s16, s16, s16) {
    return 0;
}
DebugDispObject* DebugDisplay_AddObject(f32, f32, f32, s16, s16, s16, f32, f32, f32, u8, u8, u8, u8, s16,
                                        GraphicsContext*) {
    return nullptr;
}
void lusprintf(const char*, int32_t, int32_t, const char*, ...) {
}
static NeiSaveData sSave;
NeiSaveData* Nei_Save(void) {
    return &sSave;
}
// The seed and custom-slot stores are fixture boundaries; ownership and the
// self-healing gameplay getter above are extracted unchanged from ComboShip.
uint16_t Nei_GetOwnedItem(uint8_t slot) {
    return slot >= 24 && slot < 72 ? sSave.ownedItems[slot - 24] : 0;
}
uint8_t Seasons_RandoMode(void) {
    return NEI_SEASONS_INDIVIDUAL;
}
void Seasons_UpdateGates(void) {
}
static std::map<std::string, int32_t> sSettings;
static float sLoopGain;
static int sNatureWrites;
static int sNativeRandomCalls;
static std::vector<std::string> sDiagnosticLogs;
int32_t CVarGetInteger(const char* key, int32_t fallback) {
    auto found = sSettings.find(key);
    return found == sSettings.end() ? fallback : found->second;
}
Color_RGB8 CVarGetColor24(const char*, Color_RGB8 fallback) {
    return fallback;
}
s16 Rand_S16Offset(s16 base, s16) {
    ++sNativeRandomCalls;
    return base;
}
u8 Audio_IsNatureRainEnabled(void) {
    return 0;
}
u8 Audio_IsNatureLightningEnabled(void) {
    return 0;
}
f32 Rand_ZeroOne(void) {
    return 0.5f;
}
bool WeatherSamplePlayer_Play(const char*, float) {
    return true;
}
void Environment_AddLightningBolts(PlayState*, u8) {
}
void Environment_DrawLightningFlash(PlayState*, u8, u8, u8, u8) {
}
void Audio_SetNatureAmbienceChannelIO(u8, u8, u8) {
    ++sNatureWrites;
}
void WeatherSamplePlayer_SetLoop(const char* path, float gain) {
    sLoopGain = path ? gain : 0.0f;
}
void GlobalOutdoorRain_Log(const char* message) {
    sDiagnosticLogs.emplace_back(message);
}
}

struct Fixture {
    PlayState play = {};
    Camera camera = {};
    Fixture() {
        GlobalOutdoorRain_Reset();
        if (SeasonWeather_Reset)
            SeasonWeather_Reset();
        sSettings.clear();
        sSettings["gAudioEditor.GlobalOutdoorRainOvercast"] = 0;
        sSave = {};
        gSaveContext = {};
        gSaveContext.gameMode = GAMEMODE_NORMAL;
        gSaveContext.dayTime = gSaveContext.skyboxTime = 0x8000;
        gLightningStrike = {};
        sLightningFlashAlpha = 0;
        gWeatherMode = gSkyboxBlendingEnabled = D_8011FB34 = D_8011FB38 = 0;
        gTimeSpeed = 0;
        sGameInfo = {};
        play.skyboxId = SKYBOX_NORMAL_SKY;
        play.cameraPtrs[0] = &camera;
        play.activeCamera = 0;
        gPlayState = &play;
        GlobalOutdoorRain_BeginScene(&play, 0, 0, 0);
        sLoopGain = 0;
        sNatureWrites = 0;
        sNativeRandomCalls = 0;
        sDiagnosticLogs.clear();
    }
    ~Fixture() {
        SeasonWeather_Reset();
        GlobalOutdoorRain_Reset();
        gPlayState = nullptr;
    }
    void Frame() {
        ++play.state.frames;
        SeasonWeather_Update(&play);
        GlobalOutdoorRain_Update(&play);
    }
    void Frames(int frames) {
        for (int i = 0; i < frames; ++i)
            Frame();
    }
    void Select(int season) {
        sSettings["gAudioEditor.SeasonWeather.Season"] = season;
    }
};

// First twelve actual native lighting rows preserved by the supplied Hyrule
// Field archive, normal and alt: !Hyrule Field - FINAL(1).o2r, SHA256
// 6a3c2662e03598a1b3aa4eaa6d421466dac9d0a295f121af0d5c7d864b5d49ad.
// The production time table chooses row3 at03:00/21:16, or row11 after native
// gloomy-sky interpolation. This is not evidence about Kakariko's asset rows.
static EnvLightSettings sFieldLights[] = {
    { { 70, 45, 57 },
      { 73, 73, 73 },
      { 180, 154, 138 },
      { -73, -73, -73 },
      { 20, 20, 60 },
      { 140, 120, 110 },
      -31,
      12800 },
    { { 80, 80, 80 }, { 0, 0, 0 }, { 255, 255, 255 }, { 0, 0, 0 }, { 70, 70, 90 }, { 100, 100, 120 }, -28, 12800 },
    { { 120, 90, 0 }, { 0, 0, 0 }, { 250, 135, 50 }, { 0, 0, 0 }, { 30, 30, 60 }, { 120, 70, 50 }, -29, 12800 },
    { { 40, 40, 70 }, { 0, 0, 0 }, { 20, 20, 35 }, { 0, 0, 0 }, { 50, 50, 100 }, { 0, 0, 30 }, -32, 12800 },
    { { 60, 40, 70 }, { 73, 73, 73 }, { 80, 30, 60 }, { -73, -73, -73 }, { 80, 50, 150 }, { 70, 43, 45 }, -46, 12800 },
    { { 75, 90, 100 },
      { 73, 73, 73 },
      { 55, 255, 240 },
      { -73, -73, -73 },
      { 10, 150, 190 },
      { 20, 90, 110 },
      -46,
      12800 },
    { { 60, 40, 80 }, { 73, 73, 73 }, { 60, 75, 150 }, { -73, -73, -73 }, { 60, 55, 150 }, { 50, 30, 30 }, -46, 12800 },
    { { 0, 40, 80 }, { 73, 73, 73 }, { 20, 50, 75 }, { -73, -73, -73 }, { 50, 100, 150 }, { 0, 10, 20 }, -46, 12800 },
    { { 92, 92, 92 },
      { 73, 73, 73 },
      { 160, 134, 118 },
      { -73, -73, -73 },
      { 30, 10, 10 },
      { 30, 10, 10 },
      2010,
      12800 },
    { { 95, 80, 80 },
      { 73, 73, 73 },
      { 145, 145, 130 },
      { -73, -73, -73 },
      { 40, 40, 80 },
      { 95, 95, 85 },
      2010,
      12800 },
    { { 80, 70, 70 }, { 73, 73, 73 }, { 150, 70, 35 }, { -73, -73, -73 }, { 10, 10, 25 }, { 35, 10, 10 }, 2012, 12800 },
    { { 70, 70, 90 }, { 73, 73, 73 }, { 0, 0, 15 }, { -73, -73, -73 }, { 30, 30, 80 }, { 0, 0, 10 }, 2012, 12800 },
};

static void NightLightingRetainsNativeWeatherPalette() {
    for (int season : { SEASON_AUTUMN, SEASON_WINTER }) {
        for (u16 time : { u16(0x2000), u16(21 * 65536 / 24 + 16 * 65536 / 1440) }) {
            Fixture f;
            f.Select(season);
            gSaveContext.dayTime = gSaveContext.skyboxTime = time;
            f.play.envCtx.lightSettingsList = sFieldLights;
            f.play.envCtx.numLightSettings = 12;
            SeasonWeather_TestNativeOutdoorLighting(&f.play);
            REQUIRE(f.play.envCtx.lightSettings.ambientColor[0] == 40);
            REQUIRE(f.play.envCtx.lightSettings.ambientColor[1] == 40);
            SeasonWeather_TestNativeLighting(&f.play);
            // Preserve the actual native night light, without a second RGB subtraction.
            for (int i = 0; i < 3; ++i) {
                REQUIRE(f.play.lightCtx.ambientColor[i] == sFieldLights[3].ambientColor[i]);
                REQUIRE(f.play.envCtx.dirLight2.params.dir.color[i] == sFieldLights[3].light2Color[i]);
                REQUIRE(f.play.envCtx.adjAmbientColor[i] == 0);
                REQUIRE(f.play.envCtx.adjLight1Color[i] == 0);
            }
            REQUIRE(f.play.lightCtx.ambientColor[2] > f.play.lightCtx.ambientColor[0]);
        }
    }
}

static void AllSeasonsPreserveNativeWeatherLighting() {
    for (int season : { -1, SEASON_SPRING, SEASON_SUMMER, SEASON_AUTUMN, SEASON_WINTER }) {
        for (bool followRod : { false, true }) {
            for (u16 time : { u16(0x2000), u16(6 * 65536 / 24), u16(0x8000), u16(18 * 65536 / 24),
                              u16(21 * 65536 / 24 + 16 * 65536 / 1440) }) {
                for (int globalRain : { 0, 1 }) {
                    for (int rainMode : { -1, 0, 1, 2 }) {
                        Fixture f;
                        sSave.seasonsOwned = 0xF;
                        sSave.seasonsRodOwned = 1;
                        sSave.season = season < 0 ? SEASON_OFF : season;
                        const auto saved = sSave;
                        f.Select(followRod ? -2 : season);
                        sSettings["gAudioEditor.SeasonWeather.Spring.RainMode"] = rainMode;
                        sSettings["gAudioEditor.SeasonWeather.Autumn.RainMode"] = rainMode;
                        sSettings["gAudioEditor.GlobalOutdoorRain"] = globalRain;
                        sSettings["gAudioEditor.GlobalOutdoorRainOvercast"] = 1;
                        gSaveContext.dayTime = gSaveContext.skyboxTime = time;
                        f.play.envCtx.lightSettingsList = sFieldLights;
                        f.play.envCtx.numLightSettings = 12;
                        const auto frameLights = [&]() {
                            f.Frame();
                            func_8006FB94(&f.play.envCtx, 0);
                            SeasonWeather_TestNativeOutdoorLighting(&f.play);
                            SeasonWeather_TestNativeLighting(&f.play);
                        };
                        for (int frame = 0; frame < 140; ++frame)
                            frameLights();
                        REQUIRE(SeasonWeather_SeasonForPlay(&f.play) == season);
                        REQUIRE(std::memcmp(&saved, &sSave, sizeof(saved)) == 0);
                        REQUIRE(gSaveContext.dayTime == time && gSaveContext.skyboxTime == time);
                        for (int i = 0; i < 3; ++i) {
                            REQUIRE(f.play.envCtx.adjAmbientColor[i] == 0);
                            REQUIRE(f.play.envCtx.adjLight1Color[i] == 0);
                            // The scene's native clear/cloudy palette owns world lighting.
                            // Seasonal surface colors must not subtract from that palette again.
                            REQUIRE(f.play.lightCtx.ambientColor[i] == f.play.envCtx.lightSettings.ambientColor[i]);
                            REQUIRE(f.play.envCtx.dirLight1.params.dir.color[i] ==
                                    f.play.envCtx.lightSettings.light1Color[i]);
                            REQUIRE(f.play.envCtx.dirLight2.params.dir.color[i] ==
                                    f.play.envCtx.lightSettings.light2Color[i]);
                        }
                        // Actually apply and finish the native sky restoration,
                        // including releases after manual and saved seasons.
                        f.Select(-1);
                        sSettings["gAudioEditor.GlobalOutdoorRain"] = 0;
                        for (int frame = 0; frame < 140; ++frame)
                            frameLights();
                        REQUIRE(f.play.envCtx.gloomySkyMode == 0 && f.play.envCtx.unk_21 == 0);
                        REQUIRE(f.play.envCtx.unk_1F == 0 && f.play.envCtx.unk_20 == 0);
                        for (int i = 0; i < 3; ++i) {
                            REQUIRE(f.play.lightCtx.ambientColor[i] == f.play.envCtx.lightSettings.ambientColor[i]);
                            REQUIRE(f.play.envCtx.dirLight1.params.dir.color[i] ==
                                    f.play.envCtx.lightSettings.light1Color[i]);
                            REQUIRE(f.play.envCtx.dirLight2.params.dir.color[i] ==
                                    f.play.envCtx.lightSettings.light2Color[i]);
                        }
                    }
                }
            }
        }
    }
}

static void SeasonLightingDoesNotOverrideNativeDarkness() {
    for (int season : { SEASON_AUTUMN, SEASON_WINTER }) {
        Fixture f;
        f.Select(season);
        f.play.envCtx.lightSettings = sFieldLights[3];
        for (int dawn : { 0, 1 }) {
            int previous = 0;
            for (int minute = dawn ? 240 : 960; minute <= (dawn ? 480 : 1200); ++minute) {
                gSaveContext.dayTime = gSaveContext.skyboxTime = minute * 65536 / 1440;
                const int offset = SeasonWeather_LightOffset(&f.play, 0, 1);
                REQUIRE(offset == 0);
                if (minute != (dawn ? 240 : 960))
                    REQUIRE(offset == previous);
                previous = offset;
            }
        }
        gSaveContext.dayTime = gSaveContext.skyboxTime = 0x2000;
        for (int i = 0; i < 3; ++i) {
            f.play.envCtx.adjAmbientColor[i] = -sFieldLights[3].ambientColor[i];
            f.play.envCtx.adjLight1Color[i] = -sFieldLights[3].light2Color[i];
        }
        SeasonWeather_TestNativeLighting(&f.play);
        for (int i = 0; i < 3; ++i) {
            REQUIRE(f.play.lightCtx.ambientColor[i] == 0);
            REQUIRE(f.play.envCtx.dirLight1.params.dir.color[i] == 0);
            REQUIRE(f.play.envCtx.dirLight2.params.dir.color[i] == 0);
        }
    }
}

static void RealLightningFlashAndReleaseKeepNativeAmbientOwnership() {
    for (int season : { -1, SEASON_SPRING, SEASON_SUMMER, SEASON_AUTUMN, SEASON_WINTER }) {
        for (bool followRod : { false, true }) {
            for (int globalRain : { 0, 1 }) {
                for (u16 time : { u16(0x2000), u16(0x8000) }) {
                    Fixture f;
                    sSave.seasonsOwned = 0xF;
                    sSave.seasonsRodOwned = 1;
                    sSave.season = season < 0 ? SEASON_OFF : season;
                    f.Select(followRod ? -2 : season);
                    gSaveContext.dayTime = gSaveContext.skyboxTime = time;
                    f.play.envCtx.lightSettingsList = sFieldLights;
                    f.play.envCtx.numLightSettings = 12;
                    int owner;
                    if (globalRain)
                        sSettings["gAudioEditor.GlobalOutdoorRain"] = 1;
                    else
                        GlobalOutdoorRain_SetNativeRequest(&f.play, &owner, 25, true);
                    f.Frame();
                    REQUIRE(f.play.envCtx.lightningMode == LIGHTNING_MODE_ON);
                    SeasonWeather_TestNativeOutdoorLighting(&f.play);
                    gLightningStrike.state = LIGHTNING_STRIKE_START;
                    gLightningStrike.flashAlphaTarget = 200;
                    for (int frame = 0; frame < 2; ++frame) {
                        Environment_UpdateLightningStrike(&f.play);
                        SeasonWeather_TestNativeLighting(&f.play);
                        for (int i = 0; i < 3; ++i) {
                            REQUIRE(SeasonWeather_LightOffset(&f.play, i, 1) == 0);
                            const int expected = std::min(255, f.play.envCtx.lightSettings.ambientColor[i] +
                                                                   f.play.envCtx.adjAmbientColor[i]);
                            REQUIRE(f.play.lightCtx.ambientColor[i] == expected);
                            REQUIRE(f.play.envCtx.dirLight1.params.dir.color[i] ==
                                    f.play.envCtx.lightSettings.light1Color[i]);
                            REQUIRE(f.play.envCtx.dirLight2.params.dir.color[i] ==
                                    f.play.envCtx.lightSettings.light2Color[i]);
                        }
                    }
                    REQUIRE(gLightningStrike.state == LIGHTNING_STRIKE_END);
                    if (globalRain)
                        sSettings["gAudioEditor.GlobalOutdoorRain"] = 0;
                    else
                        GlobalOutdoorRain_SetNativeRequest(&f.play, &owner, 0, false);
                    // Disabling either thunder owner during an active native
                    // flash must let its END phase drain the actual adj fields.
                    for (int frame = 0; frame < 80; ++frame) {
                        f.Frame();
                        Environment_UpdateLightningStrike(&f.play);
                        SeasonWeather_TestNativeLighting(&f.play);
                    }
                    REQUIRE(gLightningStrike.state == LIGHTNING_STRIKE_WAIT);
                    REQUIRE(f.play.envCtx.lightningMode == LIGHTNING_MODE_OFF);
                    for (int i = 0; i < 3; ++i) {
                        REQUIRE(f.play.envCtx.adjAmbientColor[i] == 0);
                        REQUIRE(f.play.envCtx.adjLight1Color[i] == 0);
                    }
                }
            }
        }
    }
}

static void LightingDiagnosticIsReadOnlyAndBounded() {
    REQUIRE(SeasonWeather_RecordLighting != nullptr);
    Fixture f;
    f.Select(-2);
    sSave.seasonsOwned = 1 << SEASON_WINTER;
    sSave.season = SEASON_AUTUMN; // calling the saved gameplay getter would heal/write this
    const auto saved = sSave;
    const auto saveContext = gSaveContext;
    f.play.envCtx.lightSettings = sFieldLights[3];
    f.play.lightCtx.ambientColor[2] = 50;
    const auto play = f.play;
    SeasonWeather_RecordLighting(&f.play);
    REQUIRE(sDiagnosticLogs.empty());
    sSettings["gAudioEditor.WeatherAudioDiagnostics"] = 1;
    SeasonWeather_RecordLighting(&f.play);
    REQUIRE(sDiagnosticLogs.size() == 1);
    REQUIRE(sDiagnosticLogs.back().find("[light-probe]") != std::string::npos);
    REQUIRE(sDiagnosticLogs.back().find("selection=-2") != std::string::npos);
    REQUIRE(sDiagnosticLogs.back().find("saved=2") != std::string::npos);
    REQUIRE(sDiagnosticLogs.back().find("actualAmbient=0,0,50") != std::string::npos);
    REQUIRE(sDiagnosticLogs.back().find("rawAmbient=40,40,70") != std::string::npos);
    for (int frame = 0; frame < 200; ++frame)
        SeasonWeather_RecordLighting(&f.play);
    REQUIRE(sDiagnosticLogs.size() == 1);
    REQUIRE(std::memcmp(&saved, &sSave, sizeof(saved)) == 0);
    REQUIRE(std::memcmp(&saveContext, &gSaveContext, sizeof(saveContext)) == 0);
    REQUIRE(std::memcmp(&play, &f.play, sizeof(play)) == 0);
    gLightningStrike.state = LIGHTNING_STRIKE_START;
    SeasonWeather_RecordLighting(&f.play);
    REQUIRE(sDiagnosticLogs.size() == 2);
    f.play.sceneNum++;
    SeasonWeather_RecordLighting(&f.play);
    REQUIRE(sDiagnosticLogs.size() == 3);
    sSettings["gAudioEditor.WeatherAudioDiagnostics"] = 0;
    SeasonWeather_RecordLighting(&f.play);
    REQUIRE(sDiagnosticLogs.size() == 3);
}

struct NativeSkyFixture : Fixture {
    Player player = {};
    EnWeatherTag tag = {};
    int type;
    explicit NativeSkyFixture(int tagType) : type(tagType) {
        play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
        player.actor.world.pos.x = 2000.0f;
        // Actual story flags that retain Kakariko's native thunderstorm actor.
        for (int flag : { EVENTCHKINF_USED_FOREST_TEMPLE_BLUE_WARP, EVENTCHKINF_USED_FIRE_TEMPLE_BLUE_WARP,
                          EVENTCHKINF_USED_WATER_TEMPLE_BLUE_WARP }) {
            if (type == EN_WEATHER_TAG_TYPE_THUNDERSTORM_KAKARIKO) {
                gSaveContext.eventChkInf[flag >> 4] |= 1 << (flag & 0xF);
            }
        }
    }
    void AttachTag() {
        tag.actor.id = ACTOR_EN_WEATHER_TAG;
        tag.actor.category = ACTORCAT_PROP;
        tag.actor.params = 0x0A00 | type; // native 1000-unit range, plus 100-unit restore margin
        tag.actor.update = EnWeatherTag_Update;
        play.actorCtx.actorLists[ACTORCAT_PROP].head = &tag.actor;
        EnWeatherTag_Init(&tag.actor, &play);
        REQUIRE(tag.actor.update != nullptr);
    }
    void NativeFrame() {
        ++play.state.frames;
        if (tag.actor.update) {
            EnWeatherTag_Update(&tag.actor, &play);
        }
        SeasonWeather_Update(&play);
        GlobalOutdoorRain_Update(&play);
        func_8006FB94(&play.envCtx, 0);
        SeasonWeather_TestNativeSkyBlend(&play.envCtx);
        func_800766C4(&play);
    }
    void NativeFrames(int frames) {
        for (int i = 0; i < frames; ++i)
            NativeFrame();
    }
    EnWeatherTagActionFunc Disabled() const {
        return type == EN_WEATHER_TAG_TYPE_RAIN_LAKE_HYLIA ? EnWeatherTag_DisabledRainLakeHylia
                                                           : EnWeatherTag_DisabledCloudyRainThunderKakariko;
    }
    EnWeatherTagActionFunc Enabled() const {
        return type == EN_WEATHER_TAG_TYPE_RAIN_LAKE_HYLIA ? EnWeatherTag_EnabledRainLakeHylia
                                                           : EnWeatherTag_EnabledCloudyRainThunderKakariko;
    }
};

static void NativeSkyTagsKeepTheirRangeActivationAndRestoration() {
    for (int type : { EN_WEATHER_TAG_TYPE_RAIN_LAKE_HYLIA, EN_WEATHER_TAG_TYPE_THUNDERSTORM_KAKARIKO }) {
        for (int season : { SEASON_SPRING, SEASON_AUTUMN, SEASON_WINTER }) {
            for (int globalRain : { 0, 1 }) {
                NativeSkyFixture f(type);
                f.AttachTag();
                f.Select(season);
                sSettings["gAudioEditor.SeasonWeather.Spring.RainMode"] = 1;
                sSettings["gAudioEditor.GlobalOutdoorRain"] = globalRain;
                sSettings["gAudioEditor.GlobalOutdoorRainOvercast"] = 1;
                const EnWeatherTag before = f.tag;
                f.Frame(); // the controller must not mutate or retire native weather tags
                REQUIRE(std::memcmp(&before, &f.tag, sizeof(f.tag)) == 0);
                f.NativeFrames(150);
                REQUIRE(f.play.envCtx.gloomySkyMode == 0);
                REQUIRE(f.tag.actionFunc == f.Disabled());
                f.player.actor.world.pos.x = 0;
                f.NativeFrames(150);
                REQUIRE(f.tag.actionFunc == f.Enabled());
                REQUIRE(f.play.envCtx.unk_EE[0] == (type == EN_WEATHER_TAG_TYPE_RAIN_LAKE_HYLIA ? 25 : 30));
                REQUIRE(gWeatherMode == (type == EN_WEATHER_TAG_TYPE_RAIN_LAKE_HYLIA ? 4 : 5));
                REQUIRE(f.play.envCtx.gloomySkyMode == 0);
                f.player.actor.world.pos.x = 2000;
                f.NativeFrames(150);
                REQUIRE(f.tag.actionFunc == f.Disabled());
                REQUIRE(gWeatherMode == 0);
                REQUIRE(f.play.envCtx.unk_EE[0] == (globalRain ? 25 : season == SEASON_WINTER ? 0 : 30));
                REQUIRE((f.play.envFlags[0] & (1 << 5)) == 0); // presentation cannot grow gameplay beans
            }
        }
    }
}

static void LateNativeTagsReceiveQueuedAndAppliedSeasonalSky() {
    for (int applied : { 0, 1 }) {
        NativeSkyFixture f(EN_WEATHER_TAG_TYPE_RAIN_LAKE_HYLIA);
        f.Select(SEASON_WINTER);
        f.Frame();
        REQUIRE(f.play.envCtx.gloomySkyMode == 1);
        if (applied) {
            func_8006FB94(&f.play.envCtx, 0);
            for (int i = 0; i < 100; ++i)
                SeasonWeather_TestNativeSkyBlend(&f.play.envCtx);
            REQUIRE(f.play.envCtx.unk_DE == 1);
            // Native light row one is also an activation gate. The actual
            // restore path must retire it instead of merely clearing mode one.
            f.play.envCtx.unk_1F = 1;
        }
        f.AttachTag();
        f.player.actor.world.pos.x = 0;
        if (applied) {
            gSkyboxBlendingEnabled = 1;
            f.NativeFrames(20);
            REQUIRE(f.play.envCtx.gloomySkyMode == 2 && f.play.envCtx.unk_DE == 1);
            REQUIRE(f.tag.actionFunc == f.Disabled());
            gSkyboxBlendingEnabled = 0;
        }
        f.NativeFrames(160);
        REQUIRE(f.play.envCtx.gloomySkyMode == 0);
        REQUIRE(f.play.envCtx.unk_1F != 1);
        REQUIRE(f.tag.actionFunc == f.Enabled());
        REQUIRE(f.play.envCtx.unk_EE[0] == 25);
        f.player.actor.world.pos.x = 2000;
        f.NativeFrames(150);
        REQUIRE(f.tag.actionFunc == f.Disabled());
        REQUIRE(f.play.envCtx.unk_EE[0] == 0);
    }
}

static void NativeTagsDoNotDisableTheUnmodifiedNativePresentation() {
    NativeSkyFixture f(EN_WEATHER_TAG_TYPE_RAIN_LAKE_HYLIA);
    f.AttachTag();
    f.Select(-1);
    sSettings["gAudioEditor.GlobalOutdoorRain"] = 1;
    sSettings["gAudioEditor.GlobalOutdoorRainOvercast"] = 1;
    f.Frames(50);
    REQUIRE(f.play.envCtx.gloomySkyMode == 1); // the independent baseline override still owns this setting
    REQUIRE(f.play.envCtx.unk_EE[0] == 25);
}

static void RetiredNativeTagsDoNotOverrideSavedRodWeather() {
    NativeSkyFixture f(EN_WEATHER_TAG_TYPE_RAIN_LAKE_HYLIA);
    f.AttachTag();
    Actor_Kill(&f.tag.actor); // the preserved SeasonScene retirement leaves this actor inactive
    sSave.seasonsOwned = 1 << SEASON_WINTER;
    sSave.season = SEASON_WINTER;
    f.Select(-2);
    f.Frame();
    REQUIRE(f.play.envCtx.gloomySkyMode == 1);
    REQUIRE(SeasonWeather_SeasonForPlay(&f.play) == SEASON_WINTER);
    REQUIRE(f.tag.actor.update == nullptr);
}

static void SeasonalSkyYieldPreservesScriptOwnershipAndThenNativeTags() {
    NativeSkyFixture f(EN_WEATHER_TAG_TYPE_RAIN_LAKE_HYLIA);
    f.Select(SEASON_WINTER);
    f.Frame();
    func_8006FB94(&f.play.envCtx, 0);
    for (int i = 0; i < 100; ++i)
        SeasonWeather_TestNativeSkyBlend(&f.play.envCtx);
    f.AttachTag();
    f.player.actor.world.pos.x = 0;
    f.play.csCtx.state = 1;
    GlobalOutdoorRain_SetScriptedRain(&f.play, 20);
    f.NativeFrames(10);
    REQUIRE(f.play.envCtx.gloomySkyMode == 1);
    REQUIRE(f.play.envCtx.unk_EE[0] == 20);
    // The story's own clear command queues native restoration. Seasonal
    // presentation must keep yielding while that original handshake finishes.
    GlobalOutdoorRain_SetScriptedRain(&f.play, 0);
    f.play.envCtx.gloomySkyMode = 2;
    f.play.csCtx.state = CS_STATE_IDLE;
    f.NativeFrames(160);
    REQUIRE(f.play.envCtx.gloomySkyMode == 0);
    REQUIRE(f.tag.actionFunc == f.Enabled());
    REQUIRE(f.play.envCtx.unk_EE[0] == 25);
}

static void ManualPresentationDoesNotChangeSavedRodState() {
    Fixture f;
    sSave.season = SEASON_WINTER; // deliberately unowned: the gameplay getter would self-heal it
    sSave.extEquipOwnedBits = 0x13579;
    const NeiSaveData before = sSave;
    for (int season : { -1, SEASON_SPRING, SEASON_SUMMER, SEASON_AUTUMN, SEASON_WINTER }) {
        f.Select(season);
        REQUIRE(SeasonWeather_SeasonForPlay(&f.play) == season);
        f.Frame();
        REQUIRE(std::memcmp(&before, &sSave, sizeof(sSave)) == 0);
    }
    f.Select(-2);
    REQUIRE(SeasonWeather_SeasonForPlay(&f.play) == -1);
    REQUIRE(std::memcmp(&before, &sSave, sizeof(sSave)) == 0);
    sSave.seasonsOwned = 1 << SEASON_AUTUMN;
    sSave.season = SEASON_AUTUMN;
    REQUIRE(SeasonWeather_SeasonForPlay(&f.play) == SEASON_AUTUMN);
    sSave.season = SEASON_OFF;
    REQUIRE(SeasonWeather_SeasonForPlay(&f.play) == -1);
    f.Select(999);
    REQUIRE(SeasonWeather_SeasonForPlay(&f.play) == -1);
}

static void ParticleSwitchesAreIndependentAndPreserveNativeSlots() {
    Fixture f;
    REQUIRE(SeasonWeather_ParticlesEnabled(SEASON_SPRING));
    REQUIRE(SeasonWeather_ParticlesEnabled(SEASON_AUTUMN));
    REQUIRE(SeasonWeather_ParticlesEnabled(SEASON_WINTER));
    REQUIRE(!SeasonWeather_ParticlesEnabled(SEASON_SUMMER));
    REQUIRE(!SeasonWeather_ParticlesEnabled(-1));
    REQUIRE(!SeasonWeather_FlurriesEnabled());
    sSettings["gAudioEditor.SeasonWeather.Winter.Flurries"] = 1;
    REQUIRE(SeasonWeather_FlurriesEnabled());
    sSettings["gAudioEditor.SeasonWeather.Winter.Snow"] = 0;
    REQUIRE(!SeasonWeather_FlurriesEnabled());
    sSettings["gAudioEditor.SeasonWeather.Spring.Blossoms"] = 0;
    REQUIRE(!SeasonWeather_ParticlesEnabled(SEASON_SPRING));
    REQUIRE(SeasonWeather_ParticlesEnabled(SEASON_AUTUMN));
    REQUIRE(SeasonWeather_SummerEffectEnabled(0));
    REQUIRE(SeasonWeather_SummerEffectEnabled(1));
    REQUIRE(!SeasonWeather_SummerEffectEnabled(2));
    REQUIRE(!SeasonWeather_SummerEffectEnabled(3));
    sSettings["gAudioEditor.SeasonWeather.Summer.Enabled"] = 0;
    REQUIRE(!SeasonWeather_SummerEffectEnabled(0));
    REQUIRE(!SeasonWeather_SummerEffectEnabled(1));
    f.play.envCtx.unk_EE[2] = 7;
    f.play.envCtx.unk_EE[3] = 23;
    for (int season = 0; season < SEASON_COUNT; ++season) {
        f.Select(season);
        f.Frames(50);
        REQUIRE(f.play.envCtx.unk_EE[2] == 7);
        REQUIRE(f.play.envCtx.unk_EE[3] == 23);
        REQUIRE(f.play.actorCtx.actorLists[ACTORCAT_ITEMACTION].length == 0);
    }
}

static void DefaultSpringIsDryAndDefaultAutumnIsThirtyDrops() {
    Fixture f;
    sSave.seasonsOwned = (1 << SEASON_SPRING) | (1 << SEASON_AUTUMN);
    sSave.season = SEASON_SPRING;
    f.Frames(50);
    REQUIRE(f.play.envCtx.unk_EE[0] == 0 && sLoopGain == 0);
    sSave.season = SEASON_AUTUMN;
    f.Frame();
    REQUIRE(f.play.envCtx.unk_EE[0] == 30);
    REQUIRE(std::fabs(sLoopGain - 0.5f) < 0.0001f);
    REQUIRE(f.play.envCtx.lightningMode == LIGHTNING_MODE_OFF);
    REQUIRE(GlobalOutdoorRain_HasRainIntent());
    uint8_t r = 20, g = 30, b = 40;
    REQUIRE(!GlobalOutdoorRain_GetRenderColor(&r, &g, &b));
    REQUIRE(r == 20 && g == 30 && b == 40);
}

static void NativeAndScriptedRainRemainAuthoritative() {
    Fixture f;
    f.Select(SEASON_AUTUMN);
    int actor;
    GlobalOutdoorRain_SetNativeRequest(&f.play, &actor, 25, 1);
    f.Frame();
    REQUIRE(f.play.envCtx.unk_EE[0] == 25);
    REQUIRE(f.play.envCtx.lightningMode == LIGHTNING_MODE_ON);
    f.Select(SEASON_SUMMER);
    f.Frame();
    REQUIRE(f.play.envCtx.unk_EE[0] == 25);
    f.Select(SEASON_SPRING);
    sSettings["gAudioEditor.SeasonWeather.Spring.RainMode"] = 0;
    f.Frame();
    REQUIRE(f.play.envCtx.unk_EE[0] == 25);
    GlobalOutdoorRain_BeginScene(&f.play, 40, 1, 0);
    f.Frame();
    REQUIRE(f.play.envCtx.unk_EE[0] == 40);
    f.play.csCtx.state = 1;
    GlobalOutdoorRain_SetScriptedRain(&f.play, 0);
    f.Frame();
    REQUIRE(f.play.envCtx.unk_EE[0] == 0 && sLoopGain == 0);
    REQUIRE(SeasonWeather_SeasonForPlay(&f.play) == -1);
    f.play.csCtx.state = CS_STATE_IDLE;
    f.Frame();
    REQUIRE(f.play.envCtx.unk_EE[0] == 40);
}

static void GlobalRainComposesAboveSeasonalRain() {
    Fixture f;
    f.Select(SEASON_AUTUMN);
    f.Frame();
    REQUIRE(f.play.envCtx.unk_EE[0] == 30);
    sSettings["gAudioEditor.GlobalOutdoorRain"] = 1;
    f.Frames(50);
    REQUIRE(f.play.envCtx.unk_EE[0] == 25);
    uint8_t r = 0, g = 0, b = 0;
    REQUIRE(GlobalOutdoorRain_GetRenderColor(&r, &g, &b));
    f.Select(SEASON_SUMMER);
    f.Frame();
    REQUIRE(f.play.envCtx.unk_EE[0] == 25);
    sSettings["gAudioEditor.GlobalOutdoorRain"] = 0;
    f.Frames(50);
    REQUIRE(f.play.envCtx.unk_EE[0] == 0);
    f.Select(SEASON_AUTUMN);
    f.Frame();
    REQUIRE(f.play.envCtx.unk_EE[0] == 30);
}

static void SpringShowersUseTheSharedOvercastSetting() {
    Fixture f;
    f.Select(SEASON_SPRING);
    sSettings["gAudioEditor.SeasonWeather.Spring.RainMode"] = 1;
    sSettings["gAudioEditor.GlobalOutdoorRainOvercast"] = 1;
    f.Frames(50);
    REQUIRE(f.play.envCtx.unk_EE[0] == 30);
    REQUIRE(f.play.envCtx.gloomySkyMode == 1);
    func_8006FB94(&f.play.envCtx, 0); // the native engine consumes the queued overcast
    REQUIRE(f.play.envCtx.unk_DE == 1);
    sSettings["gAudioEditor.GlobalOutdoorRainOvercast"] = 0;
    f.Frame();
    REQUIRE(f.play.envCtx.gloomySkyMode == 2);
    REQUIRE(f.play.envCtx.unk_EE[0] == 30);
}

static void IntermittentSeasonRainHasPrivatePausedCadence() {
    Fixture f;
    f.Select(SEASON_SPRING);
    sSettings["gAudioEditor.SeasonWeather.Spring.RainMode"] = 2;
    const u16 dayTime = gSaveContext.dayTime = 0x3456;
    bool sawRain = false, sawDryAfterRain = false, sawRainAgain = false;
    for (int i = 0; i < 2000; ++i) {
        f.Frame();
        int density = f.play.envCtx.unk_EE[0];
        if (density > 0) {
            if (sawDryAfterRain)
                sawRainAgain = true;
            sawRain = true;
        } else if (sawRain) {
            sawDryAfterRain = true;
        }
        REQUIRE(density >= 0 && density <= 30);
        REQUIRE(gSaveContext.dayTime == dayTime);
        if (density == 30) {
            f.play.pauseCtx.state = 1;
            float gain = sLoopGain;
            f.Frames(200);
            REQUIRE(f.play.envCtx.unk_EE[0] == 30);
            REQUIRE(std::fabs(sLoopGain - gain) < 0.0001f);
            f.play.pauseCtx.state = 0;
        }
        if (sawRainAgain)
            break;
    }
    REQUIRE(sawRain && sawDryAfterRain && sawRainAgain);
    REQUIRE(sNativeRandomCalls == 0);
    sSettings["gAudioEditor.SeasonWeather.Spring.RainMode"] = 0;
    f.Frame();
    REQUIRE(f.play.envCtx.unk_EE[0] == 0 && sLoopGain == 0);
    sSettings["gAudioEditor.SeasonWeather.Spring.RainMode"] = 1;
    f.Frames(50);
    REQUIRE(f.play.envCtx.unk_EE[0] == 30);
    // Autumn's separate Off setting survives while Spring is changed.
    sSettings["gAudioEditor.SeasonWeather.Autumn.RainMode"] = 0;
    f.Select(SEASON_AUTUMN);
    f.Frame();
    REQUIRE(f.play.envCtx.unk_EE[0] == 0);
}

static void SeasonsCannotActivateStormsGrottosOrConsumeTheRealSong() {
    for (int season = 0; season < SEASON_SLOTS; ++season) {
        for (int presentation : { -2, -1, SEASON_SPRING, SEASON_SUMMER, SEASON_AUTUMN, SEASON_WINTER }) {
            for (bool indoors : { false, true }) {
                Fixture f;
                f.Select(presentation);
                sSave.seasonsOwned = 0x0F;
                sSave.season = season;
                f.play.envCtx.indoors = indoors;
                Flags_SetEnv(&f.play, 7); // unrelated environment state survives
                DoorAna grotto = {};
                grotto.actor.params = 0x100; // native unopened Storms grotto
                grotto.actionFunc = DoorAna_WaitClosed;
                sPuzzleChimes = 0;
                for (int frame = 0; frame < 3; ++frame) {
                    SeasonWeather_TestRodBridge(&f.play);
                    f.Frame();
                    DoorAna_WaitClosed(&grotto, &f.play);
                    REQUIRE(grotto.actionFunc == DoorAna_WaitClosed);
                    REQUIRE(sPuzzleChimes == 0);
                    REQUIRE(f.play.envFlags[0] == (1 << 7));
                }
                // Native En_Okarina_Effect owns flag 5 when the actual song
                // reaches its activation frame. Seasonal updates must leave it.
                Flags_SetEnv(&f.play, 5);
                SeasonWeather_TestRodBridge(&f.play);
                f.Frame();
                DoorAna_WaitClosed(&grotto, &f.play);
                REQUIRE(grotto.actionFunc == DoorAna_WaitOpen);
                REQUIRE(sPuzzleChimes == 1);
                REQUIRE(f.play.envFlags[0] == ((1 << 7) | (1 << 5)));
            }
        }
    }
}

static void SavedRodGameplayAndNativeParticleColorStaySeparate() {
    Fixture f;
    f.Select(-1);
    SeasonWeather_TestRodBridge(&f.play);
    REQUIRE((f.play.envFlags[0] & (1 << 5)) == 0);
    REQUIRE(f.play.envCtx.unk_EE[0] == 0);
    f.play.envFlags[0] = 0;
    f.Select(SEASON_AUTUMN);
    SeasonWeather_TestRodBridge(&f.play);
    REQUIRE((f.play.envFlags[0] & (1 << 5)) == 0);
    f.play.envCtx.indoors = 1;
    SeasonWeather_TestRodBridge(&f.play);
    REQUIRE((f.play.envFlags[0] & (1 << 5)) == 0);
    f.play.envCtx.indoors = 0;
    f.play.envCtx.unk_EE[2] = 11;
    f.play.envCtx.unk_EE[3] = 31;
    sSave.seasonsOwned = 1 << SEASON_SPRING;
    sSave.season = SEASON_SPRING;
    u8 r = 0, g = 0, b = 0;
    Seasons_PrecipTint(&r, &g, &b);
    REQUIRE(r == 200 && g == 200 && b == 200);
    SeasonWeather_TestRodBridge(&f.play);
    REQUIRE(f.play.envCtx.unk_EE[2] == 11 && f.play.envCtx.unk_EE[3] == 31);
}

static void RealEnvironmentRainAndSkyRestoreComposeWithSeason() {
    Fixture f;
    f.Select(SEASON_AUTUMN);
    for (int i = 0; i < 160; ++i) {
        f.Frame();
        func_800766C4(&f.play);
    }
    REQUIRE(f.play.envCtx.unk_EE[0] == 30 && f.play.envCtx.unk_EE[1] == 30);
    // The real overcast restoration used to clear the rain target. The shared
    // resolver's rain-intent signal must protect the seasonal owner too.
    f.play.envCtx.gloomySkyMode = 2;
    f.play.envCtx.unk_DE = 1;
    func_8006FB94(&f.play.envCtx, 0);
    REQUIRE(f.play.envCtx.unk_EE[0] == 30);
    f.play.envCtx.unk_F2[0] = 40; // native Song of Storms priority
    for (int i = 0; i < 100; ++i) {
        f.Frame();
        func_800766C4(&f.play);
    }
    REQUIRE(f.play.envCtx.unk_EE[1] == 40);
    f.play.envCtx.unk_F2[0] = 0;
    f.play.csCtx.state = 1;
    GlobalOutdoorRain_SetScriptedRain(&f.play, 0);
    for (int i = 0; i < 180; ++i) {
        f.Frame();
        func_800766C4(&f.play);
    }
    REQUIRE(f.play.envCtx.unk_EE[0] == 0 && f.play.envCtx.unk_EE[1] == 0);
}

static void DuplicateUpdatesAndSceneReuseDoNotLeakOwnership() {
    Fixture f;
    f.Select(SEASON_SPRING);
    sSettings["gAudioEditor.SeasonWeather.Spring.RainMode"] = 1;
    f.Frame();
    int density = f.play.envCtx.unk_EE[0];
    float gain = sLoopGain;
    SeasonWeather_Update(&f.play); // Rod update + frame hook on the same native frame
    REQUIRE(f.play.envCtx.unk_EE[0] == density && sLoopGain == gain);
    f.Frame();
    REQUIRE(f.play.envCtx.unk_EE[0] > density);
    f.Select(SEASON_AUTUMN);
    f.Frame();
    REQUIRE(f.play.envCtx.unk_EE[0] == 30 && sLoopGain > 0);
    GlobalOutdoorRain_OnPlayDestroy();
    SeasonWeather_Reset();
    // Engine scene initialization at the same PlayState address.
    f.play.envCtx = {};
    f.Select(-1);
    GlobalOutdoorRain_BeginScene(&f.play, 0, 0, 0);
    f.Frame();
    REQUIRE(f.play.envCtx.unk_EE[0] == 0 && sLoopGain == 0);
    REQUIRE(f.play.envCtx.gloomySkyMode == 0);
    REQUIRE(f.play.envCtx.adjLight1Color[0] == 0 && f.play.envCtx.adjAmbientColor[0] == 0);
}

static void EligibilityAndLightingRespectSceneState() {
    Fixture f;
    f.Select(SEASON_WINTER);
    f.play.envCtx.adjLight1Color[0] = 9;
    f.play.envCtx.adjAmbientColor[0] = 12;
    f.Frame();
    REQUIRE(f.play.envCtx.adjLight1Color[0] == 9);
    REQUIRE(f.play.envCtx.adjAmbientColor[0] == 12);
    REQUIRE(SeasonWeather_LightOffset(&f.play, 0, 0) == 0);
    REQUIRE(SeasonWeather_LightOffset(&f.play, 0, 1) == 0);
    REQUIRE(f.play.envCtx.gloomySkyMode == 1);
    f.Frame();
    REQUIRE(f.play.envCtx.adjAmbientColor[0] == 12); // no shared-field writes
    f.Select(-1);
    f.Frame();
    REQUIRE(f.play.envCtx.adjLight1Color[0] == 9);
    REQUIRE(f.play.envCtx.adjAmbientColor[0] == 12);
    REQUIRE(SeasonWeather_LightOffset(&f.play, 0, 0) == 0);
    REQUIRE(f.play.envCtx.gloomySkyMode == 0); // an unapplied request needs no native restore handshake
    f.play.envCtx.gloomySkyMode = 0;
    f.Select(SEASON_WINTER);
    f.Frame();
    f.play.envCtx.adjAmbientColor[0] = 100; // a native writer took this channel
    f.play.envCtx.indoors = 1;
    f.Frame();
    REQUIRE(SeasonWeather_SeasonForPlay(&f.play) == -1);
    REQUIRE(f.play.envCtx.adjAmbientColor[0] == 100);
    REQUIRE(f.play.envCtx.adjLight1Color[0] == 9);
    f.play.envCtx.indoors = 0;
    f.camera.unk_14C |= 0x100;
    REQUIRE(SeasonWeather_SeasonForPlay(&f.play) == -1);
    f.camera.unk_14C = 0;
    f.play.envCtx.skyboxDisabled = 1;
    REQUIRE(SeasonWeather_SeasonForPlay(&f.play) == -1);
    f.play.envCtx.skyboxDisabled = 0;
    f.play.envCtx.customSkyboxFilter = 1;
    REQUIRE(SeasonWeather_SeasonForPlay(&f.play) == -1);
    f.play.envCtx.customSkyboxFilter = 0;
    f.play.gameOverCtx.state = GAMEOVER_DEATH_START;
    REQUIRE(SeasonWeather_SeasonForPlay(&f.play) == -1);
    f.play.gameOverCtx.state = GAMEOVER_INACTIVE;
    REQUIRE(SeasonWeather_SeasonForPlay(nullptr) == -1);
    // An open-air scene does not need the normal skybox for seasonal particles.
    f.play.skyboxId = SKYBOX_NONE;
    REQUIRE(SeasonWeather_SeasonForPlay(&f.play) == SEASON_WINTER);
}

static void ParticleEligibilityDoesNotTakeOwnershipOfTheSky() {
    Fixture f;
    REQUIRE(SeasonWeather_ParticleSeasonForPlay != nullptr);
    f.play.envCtx.skyboxDisabled = true;
    for (int season : { SEASON_SPRING, SEASON_SUMMER, SEASON_AUTUMN, SEASON_WINTER }) {
        f.Select(season);
        const auto before = sSave;
        REQUIRE(SeasonWeather_SeasonForPlay(&f.play) == -1);
        REQUIRE(SeasonWeather_ParticleSeasonForPlay(&f.play) == season);
        REQUIRE(std::memcmp(&before, &sSave, sizeof(before)) == 0);
    }
    f.Select(-2);
    sSave.season = SEASON_WINTER;
    REQUIRE(SeasonWeather_ParticleSeasonForPlay(&f.play) == -1);
    REQUIRE(sSave.season == SEASON_WINTER); // Unowned follows neither weather nor the self-healing getter.
    sSave.seasonsRodOwned = 1;
    sSave.seasonsOwned = 1 << SEASON_AUTUMN;
    sSave.season = SEASON_AUTUMN;
    REQUIRE(SeasonWeather_ParticleSeasonForPlay(&f.play) == SEASON_AUTUMN);
    for (int selection : { -1, 9 }) {
        f.Select(selection);
        REQUIRE(SeasonWeather_ParticleSeasonForPlay(&f.play) == -1);
    }
    f.Select(SEASON_AUTUMN);
    for (int gate = 0; gate < 6; ++gate) {
        f.play.envCtx.indoors = gate == 0;
        f.play.envCtx.customSkyboxFilter = gate == 1;
        f.play.csCtx.state = gate == 2 ? 1 : CS_STATE_IDLE;
        f.play.gameOverCtx.state = gate == 3 ? GAMEOVER_DEATH_START : GAMEOVER_INACTIVE;
        f.camera.unk_14C = gate == 4 ? 0x100 : 0;
        gSaveContext.gameMode = gate == 5 ? GAMEMODE_FILE_SELECT : GAMEMODE_NORMAL;
        REQUIRE(SeasonWeather_ParticleSeasonForPlay(&f.play) == -1);
    }
    REQUIRE(SeasonWeather_ParticleSeasonForPlay(nullptr) == -1);
}

static void NativeBombFadeKeepsNativeLightingOwnership() {
    for (int season : { SEASON_AUTUMN, SEASON_WINTER }) {
        Fixture f;
        f.Select(season);
        f.Frame();
        for (int i = 0; i < 3; ++i) {
            f.play.envCtx.lightSettings.ambientColor[i] = 100;
            f.play.envCtx.lightSettings.light1Color[i] = 100;
            f.play.envCtx.lightSettings.light2Color[i] = 100;
        }
        SeasonWeather_TestBombFlash(&f.play);
        SeasonWeather_TestNativeLighting(&f.play);
        for (int i = 0; i < 3; ++i) {
            REQUIRE(f.play.envCtx.adjLight1Color[i] == 250);
            REQUIRE(f.play.envCtx.adjAmbientColor[i] == 250);
            REQUIRE(f.play.lightCtx.ambientColor[i] == 255);
            REQUIRE(f.play.envCtx.dirLight1.params.dir.color[i] == 255);
            REQUIRE(f.play.envCtx.dirLight2.params.dir.color[i] == 255);
        }
        for (int frame = 0; frame < 10; ++frame) {
            SeasonWeather_TestBombFade(&f.play);
            f.Frame();
            SeasonWeather_TestNativeLighting(&f.play);
            for (int i = 0; i < 3; ++i) {
                REQUIRE(f.play.envCtx.adjLight1Color[i] == 250 - 25 * (frame + 1));
                REQUIRE(f.play.envCtx.adjAmbientColor[i] == 250 - 25 * (frame + 1));
            }
        }
        for (int i = 0; i < 3; ++i) {
            REQUIRE(f.play.envCtx.adjLight1Color[i] == 0);
            REQUIRE(f.play.envCtx.adjAmbientColor[i] == 0);
        }
        f.Select(-1);
        f.Frame();
        REQUIRE(f.play.envCtx.adjLight1Color[0] == 0 && f.play.envCtx.adjAmbientColor[0] == 0);
    }
}

static void NativeLightingClampPreservesColorsInEverySeason() {
    Fixture f;
    const u8 ambient[] = { 20, 90, 200 }, light1[] = { 80, 140, 220 }, light2[] = { 70, 60, 190 };
    const u8 fog[] = { 20, 150, 230 };
    const s16 adjAmbient[] = { -50, 20, 60 }, adjLight[] = { -20, 10, 20 }, adjFog[] = { -50, 10, 50 };
    for (int i = 0; i < 3; ++i) {
        f.play.envCtx.lightSettings.ambientColor[i] = ambient[i];
        f.play.envCtx.lightSettings.light1Color[i] = light1[i];
        f.play.envCtx.lightSettings.light2Color[i] = light2[i];
        f.play.envCtx.lightSettings.fogColor[i] = fog[i];
        f.play.envCtx.adjAmbientColor[i] = adjAmbient[i];
        f.play.envCtx.adjLight1Color[i] = adjLight[i];
        f.play.envCtx.adjFogColor[i] = adjFog[i];
    }
    const auto requireColors = [&](const u8* expectedAmbient, const u8* expectedLight1, const u8* expectedLight2) {
        SeasonWeather_TestNativeLighting(&f.play); // exact current Environment_Update clamp block
        for (int i = 0; i < 3; ++i) {
            REQUIRE(f.play.lightCtx.ambientColor[i] == expectedAmbient[i]);
            REQUIRE(f.play.envCtx.dirLight1.params.dir.color[i] == expectedLight1[i]);
            REQUIRE(f.play.envCtx.dirLight2.params.dir.color[i] == expectedLight2[i]);
            REQUIRE(f.play.envCtx.adjAmbientColor[i] == adjAmbient[i]);
            REQUIRE(f.play.envCtx.adjLight1Color[i] == adjLight[i]);
            REQUIRE(f.play.envCtx.adjFogColor[i] == adjFog[i]);
        }
        REQUIRE(f.play.lightCtx.fogColor[0] == 0 && f.play.lightCtx.fogColor[1] == 160 &&
                f.play.lightCtx.fogColor[2] == 255);
    };
    const u8 nativeAmbient[] = { 0, 110, 255 }, nativeLight1[] = { 60, 150, 240 }, nativeLight2[] = { 50, 70, 210 };
    for (int season : { -1, -2, SEASON_SPRING, SEASON_SUMMER, SEASON_AUTUMN, SEASON_WINTER }) {
        f.Select(season);
        requireColors(nativeAmbient, nativeLight1, nativeLight2);
    }
    gLightningStrike.state = LIGHTNING_STRIKE_START;
    requireColors(nativeAmbient, nativeLight1, nativeLight2);
    gLightningStrike.state = LIGHTNING_STRIKE_WAIT;
}

static void LightOffsetKeepsNativeLightningAndBounds() {
    Fixture f;
    f.Select(SEASON_WINTER);
    REQUIRE(SeasonWeather_LightOffset(&f.play, 0, 0) == 0);
    REQUIRE(SeasonWeather_LightOffset(&f.play, 2, 1) == 0);
    REQUIRE(SeasonWeather_LightOffset(&f.play, -1, 1) == 0);
    REQUIRE(SeasonWeather_LightOffset(&f.play, 3, 0) == 0);
    gLightningStrike.state = LIGHTNING_STRIKE_START;
    REQUIRE(SeasonWeather_LightOffset(&f.play, 0, 0) == 0);
    REQUIRE(SeasonWeather_LightOffset(&f.play, 2, 1) == 0);
    gLightningStrike.state = LIGHTNING_STRIKE_WAIT;
    f.Select(SEASON_AUTUMN);
    REQUIRE(SeasonWeather_LightOffset(&f.play, 0, 0) == 0);
    REQUIRE(SeasonWeather_LightOffset(&f.play, 2, 1) == 0);
    f.Select(SEASON_SPRING);
    REQUIRE(SeasonWeather_LightOffset(&f.play, 0, 0) == 0);
}

static void ScriptedRainRetainsItsExistingSkyOwnership() {
    Fixture f;
    f.Select(-1);
    sSettings["gAudioEditor.GlobalOutdoorRain"] = 1;
    sSettings["gAudioEditor.GlobalOutdoorRainOvercast"] = 1;
    f.Frames(50);
    REQUIRE(f.play.envCtx.gloomySkyMode == 1);
    f.play.csCtx.state = 1;
    GlobalOutdoorRain_SetScriptedRain(&f.play, 20);
    f.Frame();
    REQUIRE(f.play.envCtx.unk_EE[0] == 20);
    REQUIRE(f.play.envCtx.gloomySkyMode == 1);
}

int main() {
    REQUIRE(SeasonWeather_SeasonForPlay != nullptr);
    REQUIRE(SeasonWeather_Update != nullptr);
    REQUIRE(SeasonWeather_Reset != nullptr);
    REQUIRE(SeasonWeather_ParticlesEnabled != nullptr);
    REQUIRE(SeasonWeather_FlurriesEnabled != nullptr);
    REQUIRE(SeasonWeather_SummerEffectEnabled != nullptr);
    REQUIRE(SeasonWeather_LightOffset != nullptr);
    AllSeasonsPreserveNativeWeatherLighting();
    ManualPresentationDoesNotChangeSavedRodState();
    ParticleSwitchesAreIndependentAndPreserveNativeSlots();
    DefaultSpringIsDryAndDefaultAutumnIsThirtyDrops();
    NativeAndScriptedRainRemainAuthoritative();
    GlobalRainComposesAboveSeasonalRain();
    SpringShowersUseTheSharedOvercastSetting();
    IntermittentSeasonRainHasPrivatePausedCadence();
    EligibilityAndLightingRespectSceneState();
    ParticleEligibilityDoesNotTakeOwnershipOfTheSky();
    SeasonsCannotActivateStormsGrottosOrConsumeTheRealSong();
    SavedRodGameplayAndNativeParticleColorStaySeparate();
    RealEnvironmentRainAndSkyRestoreComposeWithSeason();
    DuplicateUpdatesAndSceneReuseDoNotLeakOwnership();
    NativeBombFadeKeepsNativeLightingOwnership();
    NativeLightingClampPreservesColorsInEverySeason();
    NightLightingRetainsNativeWeatherPalette();
    SeasonLightingDoesNotOverrideNativeDarkness();
    RealLightningFlashAndReleaseKeepNativeAmbientOwnership();
    LightingDiagnosticIsReadOnlyAndBounded();
    LightOffsetKeepsNativeLightningAndBounds();
    ScriptedRainRetainsItsExistingSkyOwnership();
    NativeSkyTagsKeepTheirRangeActivationAndRestoration();
    LateNativeTagsReceiveQueuedAndAppliedSeasonalSky();
    NativeTagsDoNotDisableTheUnmodifiedNativePresentation();
    RetiredNativeTagsDoNotOverrideSavedRodWeather();
    SeasonalSkyYieldPreservesScriptOwnershipAndThenNativeTags();
    puts("PASS SoH seasonal presentation: saved Rod state, independent controls, native particle slots, actual "
         "weather-tag entry/exit, queued/applied/scripted sky handoffs, private cadence, real night/overcast palette "
         "setup, unmodified world light in all seasons, native flash/release, read-only lighting snapshots and bomb "
         "decay");
}
