#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t GlobalOutdoorRain_GetRenderColor(uint8_t* red, uint8_t* green, uint8_t* blue);
struct PlayState;
// Requests are idempotent per actor and belong to exactly one scene lifetime.
void GlobalOutdoorRain_SetNativeRequest(struct PlayState* play, const void* owner, int32_t density, int32_t thunder);
void GlobalOutdoorRain_BeginScene(struct PlayState* play, int32_t density, int32_t thunder, int32_t diagnostics);
void GlobalOutdoorRain_Resolve(struct PlayState* play);
void GlobalOutdoorRain_SetScriptedRain(struct PlayState* play, int32_t density);
// Presentation-only request; native actors, scene policy, cutscenes and the
// explicit global rain overlay retain priority. Never owns native snow/fairies.
// Passing null discards the request without touching an outgoing PlayState.
void GlobalOutdoorRain_SetSeasonRequest(struct PlayState* play, int32_t density, float intensity, int32_t overcast);
// An active seasonal sky overlay defers to living native sky-weather actors.
// This gates sky presentation only; native/seasonal/global rain requests stay live.
void GlobalOutdoorRain_SetSeasonSkyAllowed(struct PlayState* play, int32_t allowed);
int32_t GlobalOutdoorRain_HasRainIntent(void);
void GlobalOutdoorRain_RecordDraw(struct PlayState* play, int32_t underwater, int32_t suppressed, float cameraY,
                                  float waterY, float viewY);
// Implemented by the regular game logger, not stderr.
void GlobalOutdoorRain_Log(const char* message);

#ifdef __cplusplus
}
#endif
