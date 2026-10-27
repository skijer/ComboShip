#pragma once
#include <stdint.h>
#include <math.h>

// Native entrances belong to their owning game. Never reinterpret one game's respawn as the other's.
typedef struct ComboFwPoint {
    int32_t game; // 0 = OOT, 1 = MM
    int32_t entrance;
    int32_t room;
    int32_t yaw;
    int32_t age; // OOT age when cast (0 adult / 1 child); MM -1. Recall retains the native save's age/form.
    float x, y, z;
    uint32_t tempSwitchFlags;
    uint32_t tempCollectFlags;
    uint32_t tempCollectFlagsLow; // MM respawn.unk_18
    // Owning-game cross-dungeon entrance captured with the point, or -1 for no ownership.
#ifdef __cplusplus
    int32_t dungeonCheckpoint = -1;
#else
    int32_t dungeonCheckpoint;
#endif
} ComboFwPoint;

static inline int ComboFw_PointValid(const ComboFwPoint* p) {
    return p && p->dungeonCheckpoint >= -1 && p->dungeonCheckpoint <= 0xFFFF && (p->game == 0 || p->game == 1) &&
           p->entrance >= 0 && p->entrance <= 0xFFFF && p->room >= 0 && p->room <= 255 && p->yaw >= -32768 &&
           p->yaw <= 32767 && ((p->game == 0 && (p->age == 0 || p->age == 1)) || (p->game == 1 && p->age == -1)) &&
           isfinite(p->x) && isfinite(p->y) && isfinite(p->z) && fabsf(p->x) <= 1000000 && fabsf(p->y) <= 1000000 &&
           fabsf(p->z) <= 1000000;
}

typedef struct ComboFwCallbacks {
    // -1 = old save without shared metadata; 0 = explicitly unset; 1 = valid point.
    int (*read)(int slot, ComboFwPoint* point);
    // NULL explicitly dispels the point, preventing an old native copy from being re-imported.
    int (*write)(int slot, const ComboFwPoint* point);
    int (*requestReturn)(int sourceGame, int slot);
} ComboFwCallbacks;

#ifdef __cplusplus
extern "C" {
#endif
#ifdef COMBO_BUILD
int ComboFw_HasPoint(int nativeSet);
void ComboFw_SyncPoint(void);
void ComboFw_PublishPoint(void);
void ComboFw_ClearPoint(void);
void ComboFw_ClearLocalPoint(void);
// 0 = same game (continue native return), 1 = queued game handoff, -1 = unavailable/invalid.
int ComboFw_RequestReturn(void);
int ComboFw_ApplyArrival(void);
int ComboFw_ConsumeArrivalAnimation(void);
// Called after destination scene commands, before its first room request. Invalid rooms use the
// ordinary entrance spawn and discard the malformed point instead of indexing unloaded resources.
void ComboFw_ValidateArrivalRoom(int roomCount);
#else
static inline int ComboFw_HasPoint(int nativeSet) {
    return nativeSet;
}
static inline void ComboFw_SyncPoint(void) {
}
static inline void ComboFw_PublishPoint(void) {
}
static inline void ComboFw_ClearPoint(void) {
}
static inline void ComboFw_ClearLocalPoint(void) {
}
static inline int ComboFw_RequestReturn(void) {
    return 0;
}
static inline int ComboFw_ApplyArrival(void) {
    return 0;
}
static inline int ComboFw_ConsumeArrivalAnimation(void) {
    return 0;
}
static inline void ComboFw_ValidateArrivalRoom(int roomCount) {
}
#endif
#ifdef __cplusplus
}
#endif
