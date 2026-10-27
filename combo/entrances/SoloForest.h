#pragma once
#include "../ComboGameMode.h"
#include <math.h>

// Exploration-only copies of the retained custom opening forest. The native
// intro/flashback scene and all combined-game portal guards remain separate.
#define COMBO_SOLO_FOREST_MM_SCENE 0x03
#define COMBO_SOLO_FOREST_MM_ENTRANCE 0x1600
#define COMBO_SOLO_FOREST_OOT_SCENE 0x6F
#define COMBO_SOLO_FOREST_OOT_ENTRANCE 0x618
#define COMBO_SOLO_FOREST_OOT_EXIT 0x61C
#define COMBO_SOLO_FOREST_STUMP_X (-695.0f)
#define COMBO_SOLO_FOREST_STUMP_Z (-313.0f)
#define COMBO_SOLO_FOREST_STUMP_YAW 12288

static inline int ComboSoloForest_Visit(int scene, int expected, int room, int gameMode, int slot, int layer,
                                        int cutscene) {
    return scene == expected && (room == 0 || room == 1) && gameMode == 0 && slot >= 0 && slot < 3 && layer == 0 &&
           cutscene == 0;
}
static inline int ComboSoloForest_IsMmVisit(int scene, int room, int gameMode, int slot, int layer, int cutscene) {
#ifdef COMBO_BUILD
    return gComboGameMode == COMBO_GAME_MM &&
           ComboSoloForest_Visit(scene, COMBO_SOLO_FOREST_MM_SCENE, room, gameMode, slot, layer, cutscene);
#else
    return 0;
#endif
}
static inline int ComboSoloForest_IsOotVisit(int scene, int room, int gameMode, int slot, int layer, int cutscene) {
#ifdef COMBO_BUILD
    return gComboGameMode == COMBO_GAME_OOT && ComboSoloForest_Visit(scene, COMBO_SOLO_FOREST_OOT_SCENE, room, gameMode,
                                                                     slot, layer >= 0 && layer < 4 ? 0 : -1, cutscene);
#else
    return 0;
#endif
}
static inline int ComboSoloForest_AtStumpWidth(float x, float y, float z, float halfWidth) {
    const float angle = COMBO_SOLO_FOREST_STUMP_YAW * (6.28318530718f / 65536.0f);
    const float dx = x - COMBO_SOLO_FOREST_STUMP_X, dz = z - COMBO_SOLO_FOREST_STUMP_Z;
    const float lateral = dx * cosf(angle) - dz * sinf(angle);
    const float depth = dx * sinf(angle) + dz * cosf(angle);
    return lateral > -halfWidth && lateral < halfWidth && depth > -62 && depth < 12 && y > -40 && y < 130;
}
static inline int ComboSoloForest_AtStump(float x, float y, float z) {
    return ComboSoloForest_AtStumpWidth(x, y, z, 58);
}
