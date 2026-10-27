#pragma once
#include "z64.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
// Presentation only. Positions/aim remain owned by the native item handlers.
void NeiAirMagic_DrawLightning(PlayState* play, const Vec3f* origin, const Vec3f* direction);
void NeiAirMagic_DrawGust(PlayState* play, const Vec3f* origin, const Vec3f* direction, float length, float radius,
                          bool blow, unsigned color);
void NeiAirMagic_DrawEnvelope(PlayState* play, Player* player);
#ifdef __cplusplus
}
#endif
