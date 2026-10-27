#pragma once
#include <math.h>

// A local MM destination, deliberately absent from every entrance-shuffle pool.
#define COMBO_BLACK_MARKET_SCENE 0x02
#define COMBO_BLACK_MARKET_ENTRANCE 0x1200
#define COMBO_BLACK_MARKET_DOOR_X 410.0f
#define COMBO_BLACK_MARKET_DOOR_Z (-300.0f)
#define COMBO_BLACK_MARKET_DOOR_YAW (-16384)

// Granny's native customer door is at (-10, 0, 420), facing the counter.
// Keep the arrival inside that door and outside the local return trigger.
#define COMBO_BLACK_MARKET_SPAWN_X (-11)
#define COMBO_BLACK_MARKET_SPAWN_Y 0
#define COMBO_BLACK_MARKET_SPAWN_Z 381
#define COMBO_BLACK_MARKET_SPAWN_YAW (-32768)
#define COMBO_BLACK_MARKET_EXIT_Z 400

static inline int ComboBlackMarket_InDoor(float x, float y, float z, float floor) {
    return fabsf(x - COMBO_BLACK_MARKET_DOOR_X) < 34.0f && fabsf(z - COMBO_BLACK_MARKET_DOOR_Z) < 34.0f &&
           y > floor - 15.0f && y < floor + 90.0f;
}

static inline int ComboBlackMarket_AtExit(float x, float y, float z) {
    return x > -42.0f && x < 42.0f && y > -20.0f && y < 100.0f && z > COMBO_BLACK_MARKET_EXIT_Z;
}
