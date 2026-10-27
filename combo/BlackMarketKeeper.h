#pragma once

#include <cstdint>

namespace BlackMarketKeeper {
// The retained Sway resource is the native crossed-arm forward lean. Keep the
// original skeleton, root translation, scale and raised-counter hand height.
// Stand on Granny's staff side (Z<93), leaning toward customers at positive Z.
alignas(2) inline constexpr char kAnimation[] = "__OTR__custom/skiejer_market/keeper/ShopGalMMDLevelSwayAnim";
inline constexpr float kX = 0.0f;
inline constexpr float kY = 1.0f;
inline constexpr float kZ = 86.0f;
inline constexpr float kScale = 0.01f;
inline constexpr int16_t kYaw = 0;
} // namespace BlackMarketKeeper
