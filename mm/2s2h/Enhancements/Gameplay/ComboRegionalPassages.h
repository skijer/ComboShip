#pragma once
#ifdef COMBO_BUILD
#include "global.h"
#ifdef __cplusplus
extern "C" {
#endif
void ComboRegionalPassagesMm_ReadEntrances(PlayState* play, unsigned count);
void ComboRegionalPassagesMm_ReadSpawns(PlayState* play, const ActorEntry* entries, unsigned count);
void ComboRegionalPassagesMm_Reset(void);
int ComboRegionalPassagesMm_Get(PlayState* play, int* route, Vec3f* anchor, s16* yaw);
int ComboRegionalPassagesMm_AtReturn(PlayState* play, const Vec3f* position);
int ComboRegionalPassagesMm_ApplyArrival(int route);
#ifdef __cplusplus
}
#endif
#endif
