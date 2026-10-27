// Pool generation does not use ShipUtils' GUI APIs. Keep the native game/rando
// headers while replacing that unrelated include boundary for this headless
// test.
#pragma once
#define SHIP_UTILS_H
#include "PR/ultratypes.h"
#include <nlohmann/json.hpp>
extern "C" s32 Ship_Random(s32 min, s32 max);
