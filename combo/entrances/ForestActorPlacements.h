#pragma once
#include <cmath>

namespace ForestActorPlacements {
// Flank the authored radius-68 rim and keep its forward walking lane clear.
// These local offsets are checked against the retained native floor/walls.
inline constexpr float kSariaLateral = -120.0f;
inline constexpr float kFlankDepth = 120.0f;
inline constexpr float kHorseLateral = 120.0f;
// Raised mushroom/tree ledge on the left of the second clearing, as marked
// in cor's placement screenshot. Retain the reclining float above its floor.
inline constexpr float kSkullKidX = 2302.0f, kSkullKidZ = -185.0f;
inline constexpr float kSkullKidProbeY = 500.0f, kSkullKidLift = 35.0f;
inline constexpr int kSkullKidYaw = -0x2000;
struct Offset {
    float x, z;
};
inline Offset SariaOffset(int portalYaw) {
    const float angle = portalYaw * (6.28318530718f / 65536.0f);
    return { kSariaLateral * std::cos(angle) + kFlankDepth * std::sin(angle),
             -kSariaLateral * std::sin(angle) + kFlankDepth * std::cos(angle) };
}
} // namespace ForestActorPlacements
