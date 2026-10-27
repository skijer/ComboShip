#pragma once

// Local passage coordinates, shared by the trigger, arrival and MM doorway.
// These are scene coordinates, never entrance-shuffle destinations.
enum {
    COMBO_PORTAL_MARKET = 0,
    COMBO_PORTAL_LOST_WOODS = 1,
    COMBO_PORTAL_OPENING_FOREST = 2,
    COMBO_PORTAL_MAGIC_MIRROR = 3,
    COMBO_PORTAL_WOODFALL = 4,
    COMBO_PORTAL_IKANA = 5,
    COMBO_PORTAL_GREAT_BAY = 6,
    COMBO_PORTAL_SNOWHEAD = 7,
};

static inline int ComboLostWoods_IsOpeningForestRoute(int route) {
    return route == COMBO_PORTAL_OPENING_FOREST || route == COMBO_PORTAL_MAGIC_MIRROR;
}

static inline int ComboLostWoods_IsForestRoute(int route) {
    return route == COMBO_PORTAL_LOST_WOODS || route == COMBO_PORTAL_WOODFALL ||
           ComboLostWoods_IsOpeningForestRoute(route);
}

static inline int ComboLostWoods_IsPlayableSlot(int fileNum) {
    return fileNum >= 0 && fileNum < 3;
}

static inline int ComboLostWoods_InOotTunnel(int scene, int room, float x, float y, float z) {
    return scene == 0x5B && room == 0 && x > -65.0f && x < 65.0f && y > -30.0f && y < 120.0f && z < -360.0f &&
           z > -560.0f;
}

static inline int ComboLostWoods_InOotFairyTunnel(int scene, int room, float x, float y, float z) {
    // Authored pink tunnel. Arm before its slot-11 Fairy exit floor at Z=360..400.
    return scene == 0x5B && room == 1 && x > 750.0f && x < 850.0f && y > -20.0f && y < 120.0f && z > 320.0f &&
           z < 400.0f;
}

static inline int ComboLostWoods_InOotWoodfallTunnel(int scene, int room, float x, float y, float z) {
    // Room 7's east dead end. Claim before its native Kokiri exit floor at X=2000.
    // The north tunnel and west room-8 connection remain separate.
    return scene == 0x5B && room == 7 && x > 1840.0f && x < 2160.0f && y > -30.0f && y < 120.0f && z > -2460.0f &&
           z < -2340.0f;
}

static inline int ComboLostWoods_InMmPassage(int scene, int room, float x, float y, float z) {
    // The north-east wall BETWEEN the maze's north/east corridors, in the entry clearing.
    const float lateral = (x + z) * 0.70710678f;
    const float depth = (x - z - 320.0f) * 0.70710678f;
    return scene == 0x64 && room == 1 && lateral > -42.0f && lateral < 42.0f && depth > -12.0f && depth < 60.0f &&
           y > -20.0f && y < 130.0f;
}
