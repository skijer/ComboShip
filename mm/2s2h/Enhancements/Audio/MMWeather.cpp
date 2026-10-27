#include "MMWeather.h"
#include "MMWeatherAudio.h"
#include "MMWeatherState.h"
#include "2s2h/Enhancements/Graphics/AutumnSceneFoliage.h"
#include "2s2h/Enhancements/Graphics/MMSummerAtmosphere.h"
#include "2s2h/Enhancements/Graphics/MMWinterAtmosphere.h"
#include "2s2h/Enhancements/Gameplay/ComboOpeningForest.h"
#ifdef COMBO_BUILD
#include "entrances/SoloForest.h"
#endif

#include <algorithm>
#include "global.h"
#include "mods/extended_inventory.h"
#include <libultraship/bridge/consolevariablebridge.h>

namespace {
MMWeather::State sState;
MMWeather::State sSpringState;
MMWeather::State sAutumnState;
MMWeather::Settings sSettings;
PlayState* sPlay = nullptr;
int sScene = -1;
int sRoom = -1;
int sSeason = -1;
int sSpringRainMode = -1;
int sAutumnRainMode = -1;
bool sNativeStormAmbienceMuted = false;
bool sWritingSeasonAmbience = false;
uint8_t sNativeRainAmbience = 0;
uint8_t sNativeThunderAmbience = 0;
uint32_t sPresentationRandom = 0x4D4D5646;

bool OutdoorEligible(const PlayState* play) {
    if (play == nullptr) {
        return false;
    }
    const Camera* camera = GET_ACTIVE_CAM(play);
    bool outdoorSky =
        play->sceneId != SCENE_SONCHONOIE && (play->skyboxId == SKYBOX_NORMAL_SKY || play->skyboxId == SKYBOX_3);
#ifdef COMBO_BUILD
    // Playable Lost Woods keeps the prologue's covered SKYBOX_2. Its two
    // exploration rooms support the shared weather controls and all seasons;
    // other special-sky interiors and the real intro/title layers stay native.
    outdoorSky |=
        play->skyboxId == SKYBOX_2 &&
        (ComboOpeningForest_IsExploration(play) ||
         ComboSoloForest_IsMmVisit(play->sceneId, play->roomCtx.curRoom.num, gSaveContext.gameMode,
                                   gSaveContext.fileNum, gSaveContext.sceneLayer, gSaveContext.save.cutsceneIndex)) &&
        (play->roomCtx.curRoom.num == 0 || play->roomCtx.curRoom.num == 1);
#endif
    return gSaveContext.gameMode == GAMEMODE_NORMAL && play->gameOverCtx.state == GAMEOVER_INACTIVE && outdoorSky &&
           !play->envCtx.skyboxDisabled && camera != nullptr && !(camera->stateFlags & CAM_STATE_UNDERWATER);
}

int SelectedSeason() {
    const int selection = CVarGetInteger(MM_WEATHER_CVAR("Season"), -2);
    if (selection != -2) {
        // Manual presentation never changes Rod ownership or the saved season.
        return selection >= 0 && selection < SEASON_COUNT ? selection : -1;
    }
    const uint8_t season = Seasons_GetSeason();
    return season < SEASON_COUNT ? season : -1;
}

int RainMode(int season) {
    const int mode = season == SEASON_SPRING ? CVarGetInteger(MM_WEATHER_CVAR("Spring.RainMode"), -1)
                                             : CVarGetInteger(MM_WEATHER_CVAR("Autumn.RainMode"), -1);
    return mode >= -1 && mode <= 2 ? mode : -1;
}

int SeasonRainDensity() {
    if (sSeason == SEASON_AUTUMN) {
        return (sAutumnState.Density() * 3) / 5;
    }
    if (sSeason != SEASON_SPRING) {
        return 0;
    }
    return sSpringRainMode == -1 ? 30 : (sSpringState.Density() * 6) / 5;
}

float SeasonRainIntensity() {
    if (sSeason == SEASON_AUTUMN) {
        return sAutumnState.Intensity() * 0.6f;
    }
    if (sSeason != SEASON_SPRING) {
        return 0.0f;
    }
    return sSpringRainMode == -1 ? (SeasonRainDensity() > 0 ? 1.0f : 0.0f) : sSpringState.Intensity();
}

void StepSeasonRain(MMWeather::State& state, int mode, bool active, int ticks) {
    MMWeather::Settings rain;
    rain.enabled = active && mode != 0;
    rain.intermittent = mode != 1;
    rain.thunder = false;
    // A deselected effect clears immediately, including while paused.
    state.Step(rain, rain.enabled, ticks);
}

void ApplyStormAmbience(uint8_t rain, uint8_t thunder) {
    sWritingSeasonAmbience = true;
    Audio_SetAmbienceChannelIO(AMBIENCE_CHANNEL_RAIN, CHANNEL_IO_PORT_1, rain);
    Audio_SetAmbienceChannelIO(AMBIENCE_CHANNEL_LIGHTNING, CHANNEL_IO_PORT_1, thunder);
    sWritingSeasonAmbience = false;
}

} // namespace

extern "C" void MMWeather_Reset() {
    MMSummerAtmosphere_Reset();
    MMWinterAtmosphere_Reset();
    MMAutumnSceneFoliage_Reset();
    if (sNativeStormAmbienceMuted) {
        ApplyStormAmbience(sNativeRainAmbience, sNativeThunderAmbience);
    }
    sNativeStormAmbienceMuted = false;
    sState.Reset();
    sSpringState.Reset();
    sAutumnState.Reset();
    sPlay = nullptr;
    sScene = sRoom = -1;
    sSeason = -1;
    sSpringRainMode = sAutumnRainMode = -1;
    sPresentationRandom = 0x4D4D5646;
    MMWeather_ClearBolts();
    MMWeatherAudio_Reset();
}

extern "C" void MMWeather_Update(PlayState* play) {
    if (play == nullptr) {
        MMWeather_Reset();
        return;
    }
    if (sPlay != play || sScene != play->sceneId || sRoom != play->roomCtx.curRoom.num) {
        MMWeather_Reset();
        sPlay = play;
        sScene = play->sceneId;
        sRoom = play->roomCtx.curRoom.num;
    }
    sSettings.enabled = CVarGetInteger(MM_WEATHER_CVAR("Enabled"), 0) != 0;
    sSettings.intermittent = CVarGetInteger(MM_WEATHER_CVAR("Mode"), 0) == 1;
    sSettings.overcast = CVarGetInteger(MM_WEATHER_CVAR("Overcast"), 1) != 0;
    sSettings.thunder = CVarGetInteger(MM_WEATHER_CVAR("Thunder"), 1) != 0;
    sSettings.thunderFrequency = CVarGetInteger(MM_WEATHER_CVAR("ThunderFrequency"), 100);
    // Outdoor rain is an explicit presentation override. Room storm policy,
    // story lighting and native weather cannot stop it. In particular, both
    // held and quick spins adjust fog throughout the effect and its release.
    // The Mayor's Residence is an interior (scene_table.h), even when its
    // room exposes an outdoor skybox. Keep all of its rooms out of global rain.
    const bool eligible = OutdoorEligible(play);
    // Both the Rod and manual seasons defer to story presentation. The separate
    // global rain override keeps its existing independent eligibility above.
    const int season = MMWeather_SeasonForPlay(play);
    const int springMode = RainMode(SEASON_SPRING);
    const int autumnMode = RainMode(SEASON_AUTUMN);
    if (season != sSeason || springMode != sSpringRainMode) {
        sSpringState.Reset();
    }
    if (season != sSeason || autumnMode != sAutumnRainMode) {
        sAutumnState.Reset();
    }
    sSeason = season;
    sSpringRainMode = springMode;
    sAutumnRainMode = autumnMode;
    MMAutumnSceneFoliage_Update(play);
    if (MMWeather_SeasonClearsRain() || sNativeStormAmbienceMuted) {
        // Native channels stop before precipitation reaches zero. Preserve their
        // actual IO requests, including En_Test4's stop at a rain target of eight.
        sNativeStormAmbienceMuted = MMWeather_SeasonClearsRain();
        ApplyStormAmbience(sNativeStormAmbienceMuted ? 0 : sNativeRainAmbience,
                           sNativeStormAmbienceMuted ? 0 : sNativeThunderAmbience);
    }
    const int ticks = play->pauseCtx.state == PAUSE_STATE_OFF ? std::clamp<int>(R_UPDATE_RATE, 1, 3) : 0;
    const bool strike = sState.Step(sSettings, eligible, ticks);
    StepSeasonRain(sSpringState, sSpringRainMode, sSeason == SEASON_SPRING && sSpringRainMode != -1, ticks);
    StepSeasonRain(sAutumnState, sAutumnRainMode, sSeason == SEASON_AUTUMN, ticks);
    if (!eligible) {
        MMWeather_ClearBolts();
        MMWeatherAudio_Reset();
        return;
    }
    const float seasonRain = SeasonRainIntensity();
    MMWeatherAudio_SetRain(std::max(sState.Intensity(), seasonRain) *
                           std::clamp(CVarGetInteger(MM_WEATHER_CVAR("RainVolume"), 100), 0, 100) / 100.0f);
    if (strike) {
        MMWeather_StartBolt();
        MMWeatherAudio_Thunder(1.0f);
    }
}

extern "C" int MMWeather_Season() {
    return sSeason;
}

extern "C" int MMWeather_SeasonForPlay(const PlayState* play) {
    if (!OutdoorEligible(play) || play->csCtx.state != CS_STATE_IDLE ||
        play->envCtx.lightSettingOverride != LIGHT_SETTING_OVERRIDE_NONE || play->envCtx.customSkyboxFilter) {
        return -1;
    }
    return SelectedSeason();
}

extern "C" int MMWeather_FoliageSeasonForPlay(const PlayState* play) {
#ifdef COMBO_BUILD
    // Preserve the established opening-forest foliage selection independently
    // of the shared weather controls and without changing its native lighting.
    if (play &&
        (ComboOpeningForest_IsExploration(play) ||
         ComboSoloForest_IsMmVisit(play->sceneId, play->roomCtx.curRoom.num, gSaveContext.gameMode,
                                   gSaveContext.fileNum, gSaveContext.sceneLayer, gSaveContext.save.cutsceneIndex))) {
        const Camera* camera = GET_ACTIVE_CAM(play);
        if (play->gameOverCtx.state == GAMEOVER_INACTIVE && !play->envCtx.skyboxDisabled && camera != nullptr &&
            !(camera->stateFlags & CAM_STATE_UNDERWATER) && play->csCtx.state == CS_STATE_IDLE &&
            play->envCtx.lightSettingOverride == LIGHT_SETTING_OVERRIDE_NONE && !play->envCtx.customSkyboxFilter)
            return SelectedSeason();
    }
#endif
    return MMWeather_SeasonForPlay(play);
}

extern "C" int MMWeather_SeasonClearsRain() {
    // Every eligible season owns its rain presentation, including the default
    // Spring rain and Autumn wet/dry cycle. Native targets and timers stay live.
    return sSeason >= SEASON_SPRING && sSeason < SEASON_COUNT;
}

extern "C" uint32_t MMWeather_ResolveAmbienceSeqCmd(uint32_t cmd) {
    if (((cmd >> 24) & 0xF) != SEQ_PLAYER_AMBIENCE) {
        return cmd;
    }
    const uint32_t op = cmd >> 28;
    if (op == SEQCMD_OP_PLAY_SEQUENCE || op == SEQCMD_OP_STOP_SEQUENCE) {
        sNativeRainAmbience = sNativeThunderAmbience = 0;
        return cmd;
    }
    if (op != SEQCMD_OP_SET_CHANNEL_IO || ((cmd >> 16) & 0xFF) != CHANNEL_IO_PORT_1 || sWritingSeasonAmbience) {
        return cmd;
    }
    const uint32_t channel = (cmd >> 8) & 0xF;
    if (channel == AMBIENCE_CHANNEL_RAIN) {
        sNativeRainAmbience = cmd & 0xFF;
    } else if (channel == AMBIENCE_CHANNEL_LIGHTNING) {
        sNativeThunderAmbience = cmd & 0xFF;
    } else {
        return cmd;
    }
    return sNativeStormAmbienceMuted ? cmd & ~0xFFu : cmd;
}

extern "C" int MMWeather_RainDensity() {
    return std::max(sState.Density(), SeasonRainDensity());
}

extern "C" float MMWeather_Overcast() {
    const float seasonal = sSeason == SEASON_AUTUMN   ? sAutumnState.Intensity() * 0.5f
                           : sSeason == SEASON_SPRING ? SeasonRainIntensity() * 0.75f
                           : sSeason == SEASON_WINTER ? 0.65f
                                                      : 0.0f;
    return std::max(sState.Overcast(sSettings), seasonal);
}

extern "C" uint8_t MMWeather_Shade(uint8_t value) {
    return static_cast<uint8_t>(value * (1.0f - 0.35f * MMWeather_Overcast()));
}

extern "C" uint8_t MMWeather_ResolveOotCloudBlend(uint8_t nativeBlend) {
    // Ordinary MM CLOUD geometry is blue in the native renderer. Only the OoT
    // artwork getter normalizes that dry baseline; keep MM bindings untouched.
    if (sSeason >= 0 || !OutdoorEligible(sPlay) || sPlay->skyboxId != SKYBOX_NORMAL_SKY ||
        sPlay->csCtx.state != CS_STATE_IDLE || sPlay->envCtx.lightSettingOverride != LIGHT_SETTING_OVERRIDE_NONE ||
        sPlay->envCtx.customSkyboxFilter) {
        return nativeBlend;
    }
    const auto& env = sPlay->envCtx;
    const bool nativeWeather = env.stormState == STORM_STATE_ON || gWeatherMode == WEATHER_MODE_RAIN ||
                               gWeatherMode == WEATHER_MODE_SNOW || env.precipitation[PRECIP_RAIN_CUR] > 0 ||
                               env.precipitation[PRECIP_RAIN_MAX] > 0 || env.precipitation[PRECIP_SOS_MAX] > 0 ||
                               env.precipitation[PRECIP_SNOW_CUR] > 0 || env.precipitation[PRECIP_SNOW_MAX] > 0;
    return nativeWeather ? nativeBlend : static_cast<uint8_t>(255.0f * MMWeather_Overcast());
}

extern "C" void MMWeather_ApplySky(uint8_t* first, uint8_t* second, uint8_t* blend) {
    // Native CLOUD also describes ordinary daytime cloud geometry. Start the
    // active season's presentation from its requested baseline instead of
    // inheriting that texture as an already-overcast OoT sky. Explicit outdoor
    // weather still composes afterward; SeasonForPlay retains story/interior gates.
    if (sSeason == SEASON_WINTER) {
        *first = *second = 1;
        *blend = 0;
    } else if (MMWeather_SeasonClearsRain()) {
        *first = *second = *blend = 0;
    }
    const float amount = MMWeather_Overcast();
    if (amount > 0.0f) {
        // MM's texture 1 is its cloudy sky. Only compose the rendered texture blend;
        // retain the native day-specific configuration and color palette.
        if (*first == 0 && *second == 0) {
            *second = 1;
            *blend = static_cast<uint8_t>(255.0f * amount);
        } else if (*first == 0 && *second == 1) {
            *blend = static_cast<uint8_t>(*blend + (255 - *blend) * amount);
        } else if (*first == 1 && *second == 0) {
            *blend = static_cast<uint8_t>(*blend * (1.0f - amount));
        }
    }
}

extern "C" void MMWeather_RainColor(uint8_t* red, uint8_t* green, uint8_t* blue) {
    if (MMWeather_RainDensity() > 0) {
        const Color_RGBA8 fallback = { 150, 255, 255, 255 };
        const auto color = CVarGetColor(MM_WEATHER_CVAR("RainColor"), fallback);
        *red = color.r;
        *green = color.g;
        *blue = color.b;
    }
}

extern "C" void MMWeather_DrawLightning(PlayState* play) {
    if (sState.FlashAlpha() > 0) {
        Environment_DrawLightningFlash(play, 200, 200, 255, sState.FlashAlpha());
    }
}

extern "C" float MMWeather_RandomFloat() {
    sPresentationRandom ^= sPresentationRandom << 13;
    sPresentationRandom ^= sPresentationRandom >> 17;
    sPresentationRandom ^= sPresentationRandom << 5;
    return (sPresentationRandom >> 8) / 16777216.0f;
}
