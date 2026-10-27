#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include "actors/PlacedSariaABI.h"

namespace PlacedSariaABITest {
/* Frozen pre-scale caller layout, independent of the production declaration. */
struct FrozenV1 {
    void* owner;
    void* skeleton;
    void* animation;
    const char* eyes[3];
    const char* mouth;
    const char* heads[3];
    const char* poseLimbs[17];
    int altEnabled;
    int custom;
};
struct LegacyCaller {
    FrozenV1 output{};
    std::array<unsigned char, 32> tail;
    LegacyCaller() {
        tail.fill(0xa5);
    }
    bool Intact() const {
        return std::all_of(tail.begin(), tail.end(), [](auto byte) { return byte == 0xa5; });
    }
};
static_assert(sizeof(ComboPlacedSariaResources) == sizeof(FrozenV1));
static_assert(offsetof(ComboPlacedSariaResources, custom) == offsetof(FrozenV1, custom));

template <typename Load, typename Release> bool CheckV2(Load load, Release release, float expectedScale) {
    struct Caller {
        ComboPlacedSariaResourcesV2 output;
        std::array<unsigned char, 32> tail;
    } caller;
    for (size_t shortSize : { size_t(0), sizeof(FrozenV1), sizeof(caller.output) - 1 }) {
        std::memset(&caller, 0xa5, sizeof(caller));
        std::array<unsigned char, sizeof(caller)> before;
        std::memcpy(before.data(), &caller, sizeof(caller));
        if (load(0, &caller.output, shortSize) || std::memcmp(&caller, before.data(), sizeof(caller)))
            return false; // Reject undersized buffers before any write.
    }
    if (load(0, nullptr, sizeof(caller.output)))
        return false;
    for (size_t size : { sizeof(caller.output), sizeof(caller) }) {
        std::memset(&caller, 0xa5, sizeof(caller));
        if (!load(0, &caller.output, size))
            return false;
        const bool valid = caller.output.resources.owner && caller.output.resources.skeleton &&
                           std::abs(caller.output.drawScale - expectedScale) < 1e-6f &&
                           std::all_of(caller.tail.begin(), caller.tail.end(), [](auto byte) { return byte == 0xa5; });
        release(caller.output.resources.owner);
        if (!valid)
            return false;
    }
    return true;
}
} // namespace PlacedSariaABITest
