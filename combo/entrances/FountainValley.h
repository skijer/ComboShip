#pragma once
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#define COMBO_VALLEY_BACKDROP_SIGNATURE 1
#define COMBO_VALLEY_BODY_CLEARANCE 1

// Narrow geometry derived from the existing Fountain valley. Not a donor
// collision-header replacement: native water, cameras, exits and actors remain owned by the scene.
namespace ComboFountainValley {
struct Point {
    float x, y, z;
};
struct Triangle {
    Point a, b, c;
    bool floor;
};
inline constexpr std::array<Triangle, 43> kTriangles = { {
    { { 1724, 378, -280 }, { 1516, 400, -380 }, { 1565, 317, -227 }, true },
    { { 1772, 284, 163 }, { 1563, 209, 18 }, { 1579, 233, 157 }, true },
    { { 1751, 274, -44 }, { 1563, 209, 18 }, { 1772, 284, 163 }, true },
    { { 1751, 274, -44 }, { 1557, 226, -35 }, { 1563, 209, 18 }, true },
    { { 1751, 274, -44 }, { 1724, 378, -280 }, { 1557, 226, -35 }, true },
    { { 1557, 226, -35 }, { 1724, 378, -280 }, { 1565, 317, -227 }, true },
    { { 1812, 363, 381 }, { 1622, 319, 334 }, { 1621, 400, 516 }, true },
    { { 1812, 363, 381 }, { 1772, 284, 163 }, { 1622, 319, 334 }, true },
    { { 1622, 319, 334 }, { 1772, 284, 163 }, { 1579, 233, 157 }, true },
    { { 2093, 351, 136 }, { 1772, 284, 163 }, { 1812, 363, 381 }, true },
    { { 2093, 351, 136 }, { 1812, 363, 381 }, { 2062, 734, 212 }, false },
    { { 1812, 363, 381 }, { 1621, 400, 516 }, { 1719, 756, 606 }, false },
    { { 2114, 350, -334 }, { 1724, 378, -280 }, { 1751, 274, -44 }, true },
    { { 2114, 350, -334 }, { 1751, 274, -44 }, { 2114, 292, -102 }, true },
    { { 2114, 292, -102 }, { 1751, 274, -44 }, { 1772, 284, 163 }, true },
    { { 2114, 292, -102 }, { 1772, 284, 163 }, { 2093, 351, 136 }, true },
    { { 1724, 378, -280 }, { 2114, 350, -334 }, { 1796, 589, -371 }, false },
    { { 3825, 662, -431 }, { 3684, 217, -160 }, { 4121, 180, -380 }, false },
    { { 3825, 662, -431 }, { 3085, 738, -338 }, { 3684, 217, -160 }, false },
    { { 3247, 254, -169 }, { 3684, 217, -160 }, { 3085, 738, -338 }, false },
    { { 2797, 292, -289 }, { 3247, 254, -169 }, { 3085, 738, -338 }, false },
    { { 4083, 180, 101 }, { 3554, 234, 355 }, { 3924, 670, 140 }, false },
    { { 3924, 670, 140 }, { 3554, 234, 355 }, { 3437, 707, 407 }, false },
    { { 2443, 323, 62 }, { 2093, 351, 136 }, { 2062, 734, 212 }, false },
    { { 2062, 734, 212 }, { 2420, 718, 86 }, { 2443, 323, 62 }, false },
    { { 2420, 718, 86 }, { 2811, 712, 208 }, { 2443, 323, 62 }, false },
    { { 2834, 293, 125 }, { 2443, 323, 62 }, { 2811, 712, 208 }, false },
    { { 3174, 267, 338 }, { 2834, 293, 125 }, { 2811, 712, 208 }, false },
    { { 2811, 712, 208 }, { 3437, 707, 407 }, { 3174, 267, 338 }, false },
    { { 3437, 707, 407 }, { 3554, 234, 355 }, { 3174, 267, 338 }, false },
    { { 2439, 322, -352 }, { 2114, 350, -334 }, { 2114, 292, -102 }, true },
    { { 2439, 322, -352 }, { 2114, 292, -102 }, { 2441, 294, -130 }, true },
    { { 2441, 294, -130 }, { 2114, 292, -102 }, { 2443, 323, 62 }, true },
    { { 2443, 323, 62 }, { 2114, 292, -102 }, { 2093, 351, 136 }, true },
    { { 2834, 293, 125 }, { 2441, 294, -130 }, { 2443, 323, 62 }, true },
    { { 2797, 292, -289 }, { 2441, 294, -130 }, { 2834, 293, 125 }, true },
    { { 2797, 292, -289 }, { 2439, 322, -352 }, { 2441, 294, -130 }, true },
    { { 2797, 292, -289 }, { 2834, 293, 125 }, { 3247, 254, -169 }, true },
    { { 3247, 254, -169 }, { 2834, 293, 125 }, { 3174, 267, 338 }, true },
    { { 3554, 234, 355 }, { 3247, 254, -169 }, { 3174, 267, 338 }, true },
    { { 3684, 217, -160 }, { 3247, 254, -169 }, { 3554, 234, 355 }, true },
    { { 4121, 180, -380 }, { 3684, 217, -160 }, { 4083, 180, 101 }, true },
    { { 4083, 180, 101 }, { 3684, 217, -160 }, { 3554, 234, 355 }, true },
} };
inline bool HeightInTriangle(const Triangle& t, float x, float z, float* y) {
    const float den = (t.b.z - t.c.z) * (t.a.x - t.c.x) + (t.c.x - t.b.x) * (t.a.z - t.c.z);
    if (std::fabs(den) < .001f)
        return false;
    const float a = ((t.b.z - t.c.z) * (x - t.c.x) + (t.c.x - t.b.x) * (z - t.c.z)) / den;
    const float b = ((t.c.z - t.a.z) * (x - t.c.x) + (t.a.x - t.c.x) * (z - t.c.z)) / den;
    if (a < -.00001f || b < -.00001f || a + b > 1.00001f)
        return false;
    *y = a * t.a.y + b * t.b.y + (1 - a - b) * t.c.y;
    return true;
}
// Actual fitted terrain height. Use BodyClearAt as well for player placements;
// height alone does not assert headroom or clearance from the natural side hills.
// No profile argument: vanilla and Alt use exactly the same corridor.
inline bool FloorAt(float x, float z, float* y) {
    if (!y || !std::isfinite(x) || !std::isfinite(z))
        return false;
    bool found = false;
    float best = -32768;
    for (const auto& t : kTriangles) {
        float candidate;
        if (t.floor && HeightInTriangle(t, x, z, &candidate) && candidate > best) {
            found = true;
            best = candidate;
        }
    }
    if (found)
        *y = best;
    return found;
}
// Rectangles have complete projected floor support and clear80-high body prisms.
// The terminal edge is slanted; x4080 keeps the full z+-40 band on that floor.
// Reserve the adult radius18 (child12) inside each rectangle.
struct BodyBand {
    float minX, maxX, halfWidth;
};
inline constexpr std::array<BodyBand, 3> kBodyBands = { BodyBand{ 1580, 2000, 60 }, BodyBand{ 1700, 2000, 120 },
                                                        BodyBand{ 1600, 4080, 40 } };
inline bool BodyClearAt(float x, float z, float radius = 18) {
    if (!std::isfinite(x) || !std::isfinite(z) || !std::isfinite(radius) || radius < 0)
        return false;
    for (const auto& band : kBodyBands)
        if (x - radius >= band.minX && x + radius <= band.maxX && std::fabs(z) + radius <= band.halfWidth)
            return true;
    return false;
}
// All six native room setups call this same list. These four commands are
// precisely the eight low tree-screen triangles, not the distant tree canopy.
inline constexpr std::array<size_t, 4> kBackdropSlots = { 27, 28, 42, 43 };
template <class Commands> inline bool NativeBackdropLayout(const Commands& c, uintptr_t vertexBase = 0) {
    if (c.size() != 45)
        return false;
    const auto eq = [&](size_t i, uint32_t a, uint32_t b) { return c[i].words.w0 == a && c[i].words.w1 == b; };
    // The renderer replaces each hash-vertex offset with its resolved pointer.
    // Accept only the pointer belonging to the same verified vertex resource.
    const auto vertex = [&](size_t i, uintptr_t offset) {
        return c[i].words.w0 == 0x3200600c &&
               (c[i].words.w1 == offset || (vertexBase && c[i].words.w1 == vertexBase + offset));
    };
    return vertex(25, 0) && eq(26, 0x5e7003a0, 0x9feb4255) && vertex(40, 0x60) && eq(41, 0x5e7003a0, 0x9feb4255) &&
           eq(27, 0x06000204, 0x00000406) && eq(28, 0x06080a00, 0x00080006) && eq(42, 0x06000204, 0x00000406) &&
           eq(43, 0x0604080a, 0x00040a06) && eq(44, 0xdf000000, 0);
}
} // namespace ComboFountainValley
