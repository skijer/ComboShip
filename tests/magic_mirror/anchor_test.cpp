#include "combo/gameplay/MagicMirrorAnchor.h"
#include <cassert>
#include <cstdio>
#include <limits>
#include <vector>

using namespace MagicMirrorAnchor;

std::vector<Triangle> Cap(float x, float y, float z) {
    std::vector<Triangle> result;
    constexpr float pi = 3.14159265358979323846f;
    for (int i = 0; i < 12; ++i) {
        const float a = i * pi / 6, b = (i + 1) * pi / 6;
        result.push_back({ { Point{x, y, z}, Point{x + 55 * std::cos(a), y, z + 56 * std::sin(a)},
                              Point{x + 55 * std::cos(b), y, z + 56 * std::sin(b)} } });
    }
    return result;
}

int main() {
    Point anchor{999, 999, 999};
    auto cap = Cap(-377, 80, 416);
    // The serialized user export's unrelated small ledge must not steal the placement.
    cap.push_back({ { Point{1067, 134, -34}, Point{1080, 134, -34}, Point{1080, 134, 34} } });
    cap.push_back({ { Point{1067, 134, -34}, Point{1080, 134, 34}, Point{1067, 134, 34} } });
    assert(Find(cap, anchor));
    assert(std::abs(anchor.x + 377) < 1 && std::abs(anchor.y - 80) < 1 && std::abs(anchor.z - 416) < 1);
    auto moved = Cap(1500, 320, -1300);
    assert(Find(moved, anchor));
    assert(std::abs(anchor.x - 1500) < 1 && std::abs(anchor.y - 320) < 1 && std::abs(anchor.z + 1300) < 1);
    auto ambiguous = cap;
    ambiguous.insert(ambiguous.end(), moved.begin(), moved.end());
    assert(!Find(ambiguous, anchor)); // Never guess between two matching stumps.
    auto roof = moved;
    roof.resize(6);
    assert(!Find(roof, anchor)); // Similar footprint, different topology.
    auto duplicates = moved;
    for (auto& triangle : duplicates) triangle = moved[0];
    assert(!Find(duplicates, anchor));
    auto slope = moved;
    for (auto& triangle : slope) triangle.p[0].y += 20;
    assert(!Find(slope, anchor));
    auto invalid = moved;
    for (auto& triangle : invalid) triangle.p[0].x = std::numeric_limits<float>::quiet_NaN();
    assert(!Find(invalid, anchor));
    assert(!Find({}, anchor));
    assert(!CanClaim({200, 0, -300}, {200, 120, -300}));
    assert(CanClaim({200, 120, -300}, {200, 120, -300}));
    assert(CanClaim({245, 125, -300}, {200, 120, -300}));
    assert(CanClaim({200, 115, -345}, {200, 120, -300}));
    assert(!CanClaim({245.1f, 120, -300}, {200, 120, -300}));
    assert(!CanClaim({200, 125.1f, -300}, {200, 120, -300}));
    assert(!CanClaim({200, 114.9f, -300}, {200, 120, -300}));
    assert(!CanClaim({500, 0, -300}, {200, 120, -300}));
    assert(!CanClaim({200, -500, -300}, {200, 120, -300}));
    assert(!CanClaim({200, std::numeric_limits<float>::quiet_NaN(), -300}, {200, 120, -300}));
    puts("PASS mirror placement follows moved stump and pickup requires contact with its top");
}
