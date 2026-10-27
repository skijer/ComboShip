#pragma once
#include <stdint.h>
typedef struct PlayState {
    int16_t sceneNum;
} PlayState;
enum {
    SCENE_HYRULE_FIELD = 0x51,
    SCENE_KAKARIKO_VILLAGE = 0x52,
    SCENE_ZORAS_RIVER = 0x54,
    SCENE_SACRED_FOREST_MEADOW = 0x56,
    SCENE_LON_LON_RANCH = 0x63,
};
