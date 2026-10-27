#pragma once

#include <stdint.h>

#define SOH_SEASON_WEATHER_CVAR(name) "gAudioEditor.SeasonWeather." name

#ifdef __cplusplus
extern "C" {
#endif

struct PlayState;
// -2 follows the owned, saved Rod season (default); -1 selects Native;
// 0..3 select Spring/Summer/Autumn/Winter presentation without changing gameplay.
// Returns -1 for Native/Off/unowned, interiors, underwater and story presentation.
int32_t SeasonWeather_SeasonForPlay(const struct PlayState* play);
// Ground particles can coexist with authored rain beneath an outdoor canopy
// whose scene disables the skybox. Retains every other presentation guard.
// Sky-dependent effects must additionally check their own visible-sky flag.
int32_t SeasonWeather_ParticleSeasonForPlay(const struct PlayState* play);
// Compatibility hook for the native lighting clamp. Returns zero: clear/cloudy
// scene palettes own world lighting without an additional seasonal RGB shade.
// Native lightning, bombs and room actors retain their shared adj* fields.
int32_t SeasonWeather_LightOffset(const struct PlayState* play, int32_t channel, int32_t ambient);
int32_t SeasonWeather_ParticlesEnabled(int32_t season);
int32_t SeasonWeather_FlurriesEnabled(void);
// 0: drifting fluff, 1: nighttime fireflies, 2: soft sunbeams.
int32_t SeasonWeather_SummerEffectEnabled(int32_t effect);
// 0: freezing mist, 1: nighttime aurora. Independent of falling snow.
int32_t SeasonWeather_WinterEffectEnabled(int32_t effect);
void SeasonWeather_Update(struct PlayState* play);
// Read-only final/native light snapshot under the existing weather diagnostics
// checkbox. Called after environment setup and world draw; logging is bounded.
void SeasonWeather_RecordLighting(struct PlayState* play);
// Discards scene-owned presentation without dereferencing an outgoing PlayState.
void SeasonWeather_Reset(void);

#ifdef __cplusplus
}
#endif
