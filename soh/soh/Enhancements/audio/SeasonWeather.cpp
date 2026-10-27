#include "SeasonWeather.h"
#include "GlobalOutdoorRain.h"
#include "GlobalOutdoorRainBridge.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/ShipInit.hpp"
#include "libultraship/bridge/consolevariablebridge.h"
#include "z64.h"
#include "macros.h"
#include "mods/extended_inventory.h"
#include "overlays/actors/ovl_En_Weather_Tag/z_en_weather_tag.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdint>

extern "C" {
#include "variables.h"
extern PlayState* gPlayState;
}

namespace {
constexpr int kLogicHz = 20;
constexpr int kRainDensity = 30;
PlayState* sPlay = nullptr;
int sScene = -1;
int sRoom = -1;
int sSeason = -1;
int sRainMode = -2;
uint32_t sLastFrame = UINT32_MAX;
// Presentation randomness is private: seasonal showers must not advance the
// game's actor/loot RNG, time of day, story progress or saved Rod state.
uint32_t sRandom = 0x534F4857;
GlobalOutdoorRainCycle sRain = { GlobalOutdoorRainPhase::Dry, 0, 0.0f };

PlayState* sLightingDiagnosticPlay = nullptr;
std::array<int, 26> sLightingDiagnosticKey{};

int RandomFrames(int base, int range) {
    sRandom = sRandom * 1664525u + 1013904223u;
    return base + static_cast<int>((sRandom >> 8) % static_cast<uint32_t>(range));
}

int RainMode(int season) {
    int mode = season == SEASON_SPRING ? CVarGetInteger(SOH_SEASON_WEATHER_CVAR("Spring.RainMode"), -1)
                                       : CVarGetInteger(SOH_SEASON_WEATHER_CVAR("Autumn.RainMode"), -1);
    return mode >= -1 && mode <= 2 ? mode : -1;
}

bool HasNativeSkyWeather(const PlayState* play) {
    for (const Actor* actor = play->actorCtx.actorLists[ACTORCAT_PROP].head; actor != nullptr; actor = actor->next) {
        // Cloud, snow, Lake Hylia rain and Kakariko storms all gate their native
        // activation/restoration on a clear overcast handshake. Sandstorm and
        // graveyard proximity rain use separate native paths. Retired/killed
        // actors are intentionally excluded, preserving saved Rod gameplay.
        if (actor->id == ACTOR_EN_WEATHER_TAG && actor->update != nullptr &&
            (actor->params & 0xF) <= EN_WEATHER_TAG_TYPE_THUNDERSTORM_KAKARIKO) {
            return true;
        }
    }
    return false;
}

int32_t ResolveSeasonForPlay(const PlayState* play, bool requiresSky) {
    if (play == nullptr || play->envCtx.indoors || (requiresSky && play->envCtx.skyboxDisabled) ||
        play->envCtx.customSkyboxFilter || play->csCtx.state != CS_STATE_IDLE ||
        play->gameOverCtx.state != GAMEOVER_INACTIVE || gSaveContext.gameMode != GAMEMODE_NORMAL) {
        return -1;
    }
    const Camera* camera = GET_ACTIVE_CAM(play);
    if (camera == nullptr || (camera->unk_14C & 0x100)) {
        return -1;
    }
    const int selection = CVarGetInteger(SOH_SEASON_WEATHER_CVAR("Season"), -2);
    if (selection != -2) {
        // In particular, do not call the self-healing gameplay getter for a
        // manual selection: it can write the saved Rod season.
        return selection >= 0 && selection < SEASON_COUNT ? selection : -1;
    }
    if (Seasons_SeasonCount() == 0) {
        return -1;
    }
    const uint8_t season = Seasons_GetSeason();
    return season < SEASON_COUNT ? season : -1;
}

} // namespace

extern "C" int32_t SeasonWeather_SeasonForPlay(const PlayState* play) {
    return ResolveSeasonForPlay(play, true);
}

extern "C" int32_t SeasonWeather_ParticleSeasonForPlay(const PlayState* play) {
    return ResolveSeasonForPlay(play, false);
}

extern "C" int32_t SeasonWeather_LightOffset(const PlayState*, int32_t, int32_t) {
    // Keep the native C lighting bridge neutral. Clear/overcast palettes already
    // supply weather lighting; subtracting seasonal RGB here dimmed both the
    // scene and Link a second time, independently of the surface tint/effects.
    // Native lightning, bombs and room actors retain their own adj* fields.
    return 0;
}

extern "C" int32_t SeasonWeather_ParticlesEnabled(int32_t season) {
    switch (season) {
        case SEASON_SPRING:
            return CVarGetInteger(SOH_SEASON_WEATHER_CVAR("Spring.Blossoms"), 1) != 0;
        case SEASON_AUTUMN:
            return CVarGetInteger(SOH_SEASON_WEATHER_CVAR("Autumn.Leaves"), 1) != 0;
        case SEASON_WINTER:
            return CVarGetInteger(SOH_SEASON_WEATHER_CVAR("Winter.Snow"), 1) != 0;
        default:
            return false;
    }
}

extern "C" int32_t SeasonWeather_FlurriesEnabled() {
    return SeasonWeather_ParticlesEnabled(SEASON_WINTER) &&
           CVarGetInteger(SOH_SEASON_WEATHER_CVAR("Winter.Flurries"), 0) != 0;
}

extern "C" int32_t SeasonWeather_SummerEffectEnabled(int32_t effect) {
    if (!CVarGetInteger(SOH_SEASON_WEATHER_CVAR("Summer.Enabled"), 1)) {
        return false;
    }
    switch (effect) {
        case 0:
            return CVarGetInteger(SOH_SEASON_WEATHER_CVAR("Summer.Fluff"), 1) != 0;
        case 1:
            return CVarGetInteger(SOH_SEASON_WEATHER_CVAR("Summer.Fireflies"), 1) != 0;
        case 2:
            return CVarGetInteger(SOH_SEASON_WEATHER_CVAR("Summer.Sunbeams"), 0) != 0;
        default:
            return false;
    }
}

extern "C" int32_t SeasonWeather_WinterEffectEnabled(int32_t effect) {
    switch (effect) {
        case 0:
            return CVarGetInteger(SOH_SEASON_WEATHER_CVAR("Winter.Mist"), 1) != 0;
        case 1:
            return CVarGetInteger(SOH_SEASON_WEATHER_CVAR("Winter.Aurora"), 1) != 0;
        default:
            return false;
    }
}

extern "C" void SeasonWeather_Reset() {
    sPlay = nullptr;
    sScene = sRoom = sSeason = -1;
    sRainMode = -2;
    sLastFrame = UINT32_MAX;
    sRandom = 0x534F4857;
    sRain = { GlobalOutdoorRainPhase::Dry, 0, 0.0f };
    sLightingDiagnosticPlay = nullptr;
    GlobalOutdoorRain_SetSeasonRequest(nullptr, 0, 0.0f, false);
}

extern "C" void SeasonWeather_RecordLighting(PlayState* play) {
    if (play == nullptr || !CVarGetInteger(CVAR_AUDIO("WeatherAudioDiagnostics"), 0)) {
        sLightingDiagnosticPlay = nullptr;
        return;
    }
    // Read the raw save, not the self-healing gameplay getter. A diagnostic
    // must not grant Rod ownership or repair an unowned saved season.
    const auto* saved = Nei_Save();
    const int savedSeason = saved ? saved->season : -1;
    const int selection = CVarGetInteger(SOH_SEASON_WEATHER_CVAR("Season"), -2);
    const auto& env = play->envCtx;
    const auto& light = play->lightCtx;
    const Camera* camera = play->activeCamera >= 0 && play->activeCamera < 4 ? GET_ACTIVE_CAM(play) : nullptr;
    const int cameraFlags = camera ? camera->unk_14C : -1;
    const int ambientCollapsed = light.ambientColor[0] <= 1 && light.ambientColor[1] <= 1 &&
                                 (env.lightSettings.ambientColor[0] > 1 || env.lightSettings.ambientColor[1] > 1);
    // One snapshot per scene/room/season, 45-minute native time bucket, state
    // handoff, or ambient-collapse transition. Do no per-frame string work.
    const std::array<int, 26> key = {
        play->sceneNum,
        play->roomCtx.curRoom.num,
        selection,
        sSeason,
        savedSeason,
        gSaveContext.dayTime >> 11,
        gSaveContext.skyboxTime >> 11,
        env.unk_1F,
        env.unk_20,
        env.unk_21,
        env.gloomySkyMode,
        env.unk_DE,
        env.lightningMode,
        gLightningStrike.state,
        gWeatherMode,
        env.indoors,
        play->activeCamera,
        cameraFlags,
        env.unk_BF,
        R_ENV_DISABLE_DBG,
        env.skyboxDisabled,
        play->csCtx.state,
        ambientCollapsed,
        env.customSkyboxFilter,
        gSaveContext.gameMode,
        play->gameOverCtx.state,
    };
    if (sLightingDiagnosticPlay == play && sLightingDiagnosticKey == key)
        return;
    sLightingDiagnosticPlay = play;
    sLightingDiagnosticKey = key;
    const int minutes = gSaveContext.dayTime * 1440 / 65536;
    const int skyMinutes = gSaveContext.skyboxTime * 1440 / 65536;
    char message[1400];
    std::snprintf(
        message, sizeof(message),
        "[light-probe] scene=%d room=%d hour=%02d:%02d skyHour=%02d:%02d selection=%d saved=%d resolved=%d "
        "actualAmbient=%u,%u,%u actualSun=%u,%u,%u actualMoon=%u,%u,%u "
        "rawAmbient=%u,%u,%u rawSun=%u,%u,%u rawMoon=%u,%u,%u adjAmbient=%d,%d,%d adjDirectional=%d,%d,%d "
        "fog=%u,%u,%u/%d/%d rawFog=%u,%u,%u/%d/%d adjFog=%d,%d,%d/%d/%d "
        "sunDir=%d,%d,%d moonDir=%d,%d,%d nativeIndex=%u,%u blend=%u/%u/%u gloomy=%u/%u lightning=%u/%u weather=%u "
        "rawCount=%u indoorOverride=%u debugDisabled=%d camera=%d/%d indoors=%u skyDisabled=%u story=%u "
        "skyFilter=%u gameMode=%d gameOver=%d ambientCollapsed=%d",
        play->sceneNum, play->roomCtx.curRoom.num, minutes / 60, minutes % 60, skyMinutes / 60, skyMinutes % 60,
        selection, savedSeason, sSeason, light.ambientColor[0], light.ambientColor[1], light.ambientColor[2],
        env.dirLight1.params.dir.color[0], env.dirLight1.params.dir.color[1], env.dirLight1.params.dir.color[2],
        env.dirLight2.params.dir.color[0], env.dirLight2.params.dir.color[1], env.dirLight2.params.dir.color[2],
        env.lightSettings.ambientColor[0], env.lightSettings.ambientColor[1], env.lightSettings.ambientColor[2],
        env.lightSettings.light1Color[0], env.lightSettings.light1Color[1], env.lightSettings.light1Color[2],
        env.lightSettings.light2Color[0], env.lightSettings.light2Color[1], env.lightSettings.light2Color[2],
        env.adjAmbientColor[0], env.adjAmbientColor[1], env.adjAmbientColor[2], env.adjLight1Color[0],
        env.adjLight1Color[1], env.adjLight1Color[2], light.fogColor[0], light.fogColor[1], light.fogColor[2],
        light.fogNear, light.fogFar, env.lightSettings.fogColor[0], env.lightSettings.fogColor[1],
        env.lightSettings.fogColor[2], env.lightSettings.fogNear, env.lightSettings.fogFar, env.adjFogColor[0],
        env.adjFogColor[1], env.adjFogColor[2], env.adjFogNear, env.adjFogFar, env.dirLight1.params.dir.x,
        env.dirLight1.params.dir.y, env.dirLight1.params.dir.z, env.dirLight2.params.dir.x, env.dirLight2.params.dir.y,
        env.dirLight2.params.dir.z, env.unk_1F, env.unk_20, env.unk_21, env.unk_22, env.unk_24, env.gloomySkyMode,
        env.unk_DE, env.lightningMode, gLightningStrike.state, gWeatherMode, env.numLightSettings, env.unk_BF,
        R_ENV_DISABLE_DBG, play->activeCamera, cameraFlags, env.indoors, env.skyboxDisabled, play->csCtx.state,
        env.customSkyboxFilter, gSaveContext.gameMode, play->gameOverCtx.state, ambientCollapsed);
    GlobalOutdoorRain_Log(message);
}

extern "C" void SeasonWeather_Update(PlayState* play) {
    if (play == nullptr) {
        SeasonWeather_Reset();
        return;
    }
    if (sPlay != play || sScene != play->sceneNum || sRoom != play->roomCtx.curRoom.num) {
        SeasonWeather_Reset();
        sPlay = play;
        sScene = play->sceneNum;
        sRoom = play->roomCtx.curRoom.num;
    }
    const int season = SeasonWeather_SeasonForPlay(play);
    const bool rainySeason = season == SEASON_SPRING || season == SEASON_AUTUMN;
    const int mode = rainySeason ? RainMode(season) : 0;
    if (season != sSeason || mode != sRainMode) {
        sSeason = season;
        sRainMode = mode;
        sRain = { GlobalOutdoorRainPhase::Dry, mode == 2 ? RandomFrames(2 * kLogicHz, 3 * kLogicHz) : 0, 0.0f };
        // SoH's original Autumn weather is thirty steady drops immediately.
        // Spring's original default is dry; these defaults are independent of MM's days.
        if (season == SEASON_AUTUMN && mode == -1) {
            sRain = { GlobalOutdoorRainPhase::Sustain, 0, 1.0f };
        }
    }
    const bool activeRain = rainySeason && (mode > 0 || (season == SEASON_AUTUMN && mode == -1));
    if (!activeRain) {
        sRain = { GlobalOutdoorRainPhase::Dry, 0, 0.0f };
    } else if (sLastFrame != play->state.frames && play->pauseCtx.state == 0 && play->pauseCtx.debugState == 0) {
        const auto oldPhase = sRain.phase;
        GlobalOutdoorRain_AdvanceCycle(
            sRain, mode == 2 ? GlobalOutdoorRainMode::Intermittent : GlobalOutdoorRainMode::Persistent, true,
            20 * kLogicHz, 8 * kLogicHz, 1.0f / kLogicHz);
        if (mode == 2 && oldPhase != sRain.phase) {
            if (sRain.phase == GlobalOutdoorRainPhase::Dry) {
                sRain.framesRemaining = RandomFrames(20 * kLogicHz, 6 * kLogicHz);
            } else if (sRain.phase == GlobalOutdoorRainPhase::Sustain) {
                sRain.framesRemaining = RandomFrames(8 * kLogicHz, 7 * kLogicHz);
            }
        }
    }
    sLastFrame = play->state.frames;
    const bool contributesSky = season == SEASON_AUTUMN || season == SEASON_WINTER || activeRain;
    GlobalOutdoorRain_SetSeasonSkyAllowed(play, !contributesSky || !HasNativeSkyWeather(play));
    GlobalOutdoorRain_SetSeasonRequest(play, GlobalOutdoorRain_ScaleDensity(kRainDensity, sRain.intensity),
                                       sRain.intensity, season == SEASON_AUTUMN || season == SEASON_WINTER);
}

static void RegisterSeasonWeather() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameFrameUpdate>(
        []() { SeasonWeather_Update(gPlayState); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDrawEnd>(
        []() { SeasonWeather_RecordLighting(gPlayState); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>([]() { SeasonWeather_Reset(); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnExitGame>([](int32_t) { SeasonWeather_Reset(); });
}

static RegisterShipInitFunc initSeasonWeather(RegisterSeasonWeather);
