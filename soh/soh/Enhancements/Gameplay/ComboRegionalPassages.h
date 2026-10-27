#pragma once
#ifdef COMBO_BUILD
#include "global.h"
#ifdef __cplusplus
extern "C" {
#endif
void ComboRegionalPassagesOot_ReadEntrances(PlayState* play, unsigned count);
void ComboRegionalPassagesOot_ReadSpawns(PlayState* play, const ActorEntry* entries, unsigned count);
void ComboRegionalPassagesOot_Reset(void);
int ComboRegionalPassagesOot_Get(PlayState* play, int* route, Vec3f* anchor, s16* yaw);
int ComboRegionalPassagesOot_AtDeparture(PlayState* play, const Vec3f* position);
int ComboRegionalPassagesOot_OwnsGraveFall(PlayState* play);
void ComboRegionalPassagesOot_CancelDeparture(void);
int ComboRegionalPassagesOot_ApplyArrival(int route);
int ComboRegionalPassagesOot_IsArrivalPending(PlayState* play);
#ifdef __cplusplus
}
#endif
#endif
