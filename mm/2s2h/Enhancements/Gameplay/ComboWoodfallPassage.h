#pragma once
#ifdef COMBO_BUILD
#include "global.h"
#define COMBO_WOODFALL_PASSAGE_HALF_WIDTH 64.0f
#ifdef __cplusplus
extern "C" {
#endif
void ComboWoodfallPassage_ReadSpawns(PlayState* play, const ActorEntry* entries, unsigned count);
void ComboWoodfallPassage_Reset(void);
int ComboWoodfallPassage_Get(PlayState* play, Vec3f* anchor, s16* yaw);
int ComboWoodfallPassage_AtReturn(PlayState* play, const Vec3f* position);
#ifdef __cplusplus
}
#endif
#endif
