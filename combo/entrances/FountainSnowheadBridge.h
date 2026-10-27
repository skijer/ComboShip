#pragma once
#include "FountainValley.h"
#include "../render/RegionalBridgeMaterials.h"
#include <array>
#include <cmath>
#include <cstddef>
// Authored in spot08 world coordinates. No runtime placement search or pack.
namespace ComboFountainSnowhead {
struct Point {
    float x, y, z;
};
struct Triangle {
    Point a, b, c;
    bool snow;
    bool cave = false;
    RegionalBridgeMaterials::Material material = RegionalBridgeMaterials::Material::SnowheadStone;
};
constexpr float kHalfWidth = 80, kHeadroom = 180;
constexpr float kFoundationY = -1200;
constexpr short kYaw = -16384;
inline Point Grounded(float x, float z) {
    float y = 0;
    ComboFountainValley::FloorAt(x, z, &y);
    return { x, y, z };
}
inline const Point kMouth = Grounded(3600, 0);
inline const Point kReturn = Grounded(3500, 0);
constexpr float kOpeningHalfWidth = 100, kOpeningHeight = 180;
constexpr float kShimmerFront = 10;
// Visual cross-section of the whole valley, independent of the travel trigger.
// These bounds retain floor support through the full six-unit billboard depth:
// world X3584..3596 has a common floor span of Z-161.812..334.834.
constexpr float kShimmerMinAcross = -161, kShimmerMaxAcross = 334;
// Keep the local cold weather on the final 600 units of the valley walk.
// Both effects share these fixed, floor-supported anchors; the ascent stays clear.
inline const std::array<Point, 4> kWeatherAnchors = { Grounded(kMouth.x - 600, 0), Grounded(kMouth.x - 400, 0),
                                                      Grounded(kMouth.x - 200, 0), kMouth };
// Looking east into the valley, the southern bank is on the right. This
// independent waterline scramble avoids the Ice Cavern apron entirely.
inline constexpr Point kStations[] = { { 1000, 0, 850 },  { 1250, 70, 700 }, { 1400, 130, 500 }, { 1450, 180, 300 },
                                       { 1450, 210, 80 }, { 1500, 240, 0 },  { 1600, 225, 0 } };
constexpr size_t kStationCount = sizeof(kStations) / sizeof(kStations[0]);
inline constexpr float kWidths[] = { 70, 70, 65, 60, 45, 24, 24 };
struct Mesh {
    std::array<Triangle, 192> triangles{};
    size_t count = 0;
    void Quad(Point a, Point b, Point c, Point d, bool snow = false, bool cave = false) {
        const auto material =
            snow ? RegionalBridgeMaterials::Material::SnowheadSnow : RegionalBridgeMaterials::Material::SnowheadStone;
        triangles[count++] = { a, b, c, snow, cave, material };
        triangles[count++] = { a, c, d, snow, cave, material };
    }
};
inline std::array<unsigned char, 3> VertexColor(const Triangle& triangle, Point /*point*/) {
    std::array<unsigned char, 3> color =
        triangle.snow ? std::array<unsigned char, 3>{ 190, 225, 242 } : std::array<unsigned char, 3>{ 65, 107, 133 };
    return color;
}
inline Mesh BuildMesh() {
    Mesh m;
    std::array<Point, kStationCount> left{}, right{};
    for (size_t i = 0; i < kStationCount; ++i) {
        auto a = kStations[i ? i - 1 : i], b = kStations[i + 1 < kStationCount ? i + 1 : i];
        float dx = b.x - a.x, dz = b.z - a.z, len = std::hypot(dx, dz), width = kWidths[i];
        left[i] = { kStations[i].x + dz / len * width, kStations[i].y, kStations[i].z - dx / len * width };
        right[i] = { kStations[i].x - dz / len * width, kStations[i].y, kStations[i].z + dx / len * width };
    }
    // Resolve both shoulder edges on the actual donor floor, not a center-Y
    // approximation; one unit prevents Z fighting with the shared snow skin.
    for (auto* p : { &left.back(), &right.back() }) {
        ComboFountainValley::FloorAt(p->x, p->z, &p->y);
        p->y += 1;
    }
    for (size_t i = 1; i < kStationCount; ++i) {
        auto a = left[i - 1], b = right[i - 1], c = right[i], d = left[i];
        m.Quad(a, b, c, d, true);
        // Grounded stone/ice foundation extends through native underwater ground;
        // these side faces also prevent stepping through the solid causeway.
        m.Quad({ a.x, kFoundationY, a.z }, a, d, { d.x, kFoundationY, d.z });
        m.Quad(b, { b.x, kFoundationY, b.z }, { c.x, kFoundationY, c.z }, c);
        m.triangles[m.count - 2].material = m.triangles[m.count - 1].material =
            RegionalBridgeMaterials::Material::SnowheadIce;
    }
    return m;
}
} // namespace ComboFountainSnowhead
