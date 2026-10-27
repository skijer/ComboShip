#pragma once

#include <stdint.h>

#define MM_WEATHER_CVAR(name) "gAudioEditor.MMWeather." name

#ifdef __cplusplus
extern "C" {
#endif
struct PlayState;
void MMWeather_Update(struct PlayState* play);
void MMWeather_Reset(void);
// Eligible presentation season, or -1 for Native/Off/unowned/interior/story.
// Season cvar: -2 follows the game/Rod (default), -1 is Native, 0..3 are manual.
// Manual presentation never grants ownership or changes the saved gameplay season.
int MMWeather_Season(void);
// Resolve current eligibility before the environment update (actors and HUD confirmations).
int MMWeather_SeasonForPlay(const struct PlayState* play);
// Scene foliage also supports safe ComboShip visits to the opening forest's special skybox.
int MMWeather_FoliageSeasonForPlay(const struct PlayState* play);
// Eligible seasons suppress native rain presentation while its source stays live.
// The independent global rain override still composes.
int MMWeather_SeasonClearsRain(void);
// Observe native storm channel requests before composing Summer's ambience mute.
uint32_t MMWeather_ResolveAmbienceSeqCmd(uint32_t cmd);
int MMWeather_RainDensity(void);
float MMWeather_Overcast(void);
uint8_t MMWeather_Shade(uint8_t value);
// Normalize ordinary dry MM clouds only when sampling the selected OoT artwork.
uint8_t MMWeather_ResolveOotCloudBlend(uint8_t nativeBlend);
void MMWeather_ApplySky(uint8_t* first, uint8_t* second, uint8_t* blend);
void MMWeather_RainColor(uint8_t* red, uint8_t* green, uint8_t* blue);
void MMWeather_DrawLightning(struct PlayState* play);
float MMWeather_RandomFloat(void);
// Environment-owned geometry for the enhancement, separate from native bolt slots.
void MMWeather_StartBolt(void);
void MMWeather_ClearBolts(void);
#ifdef __cplusplus
}
#endif
