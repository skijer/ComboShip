#pragma once

#include "TempleSagesPolicy.h"

namespace TempleSages {
struct ReturnPlacement {
    int scene, room;
    Pad effect;
};

// Route order matches elemental medallion bits. Coordinates are native scene
// coordinates, not a transform of a song spawn. Normal setups 0..3 only.
// Collision/source evidence: docs/poc/sages-return-placement-evidence.md.
inline constexpr std::array<ReturnPlacement, kPadCount> kReturnPlacements = { {
    { 0x56, 0, { { 0.0f, 480.0f, -2880.0f }, -32768 } }, // Forest: closer to the temple, before the broken staircase.
    { 0x61, 1, { { 0.0f, 421.0f, -230.0f }, -32768 } },  // Fire: beside Bg_Mjin params 3.
    { 0x57, 0, { { -1045.0f, -1243.0f, 7227.0f }, -32768 } }, // Water: beside params 4.
    { 0x5C, 0, { { -847.0f, -10.0f, -1817.0f }, 0 } },        // Spirit: beside params 5.
    { 0x53, 0, { { 2230.0f, 100.0f, 390.0f }, -29697 } },     // Shadow: right side (+Z), facing the torch-room center.
} };
} // namespace TempleSages
