#pragma once
#include "LostWoodsEntrance.h"
static inline int ComboRegionalPassages_IsRoute(int route) {
    return route >= COMBO_PORTAL_IKANA && route <= COMBO_PORTAL_SNOWHEAD;
}
static inline int ComboPassages_IsRoute(int route) {
    return ComboLostWoods_IsForestRoute(route) || ComboRegionalPassages_IsRoute(route);
}
#ifdef __cplusplus
#include "FountainSnowheadBridge.h"
#include <cmath>
#include <initializer_list>
namespace ComboRegional {
struct Point {
    float x = 0, y = 0, z = 0;
};
struct Pose {
    Point pos{};
    short yaw = 0;
    Point forward{};
};
struct Origin {
    int route = 0, entrance = -1, room = -1;
    Pose pose{};
};
constexpr float kAngle = 6.28318530718f / 65536.0f;
constexpr float kHalfWidth = 68, kHeight = 133;
inline Point Add(Point a, Point b, float scale = 1) {
    return { a.x + b.x * scale, a.y + b.y * scale, a.z + b.z * scale };
}
inline float Dot(Point a, Point b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
inline Point Sub(Point a, Point b) {
    return Add(a, b, -1);
}
inline float Distance2(Point a, Point b) {
    const auto d = Sub(a, b);
    return d.x * d.x + d.z * d.z;
}
inline Point Forward(short yaw) {
    return { std::sin(yaw * kAngle), 0, std::cos(yaw * kAngle) };
}
inline short Yaw(Point direction) {
    return static_cast<short>(std::lround(std::atan2(direction.x, direction.z) / kAngle));
}
inline bool ValidOrigin(const Origin& point, int scene) {
    const int wanted = point.route == COMBO_PORTAL_IKANA       ? 0x53
                       : point.route == COMBO_PORTAL_GREAT_BAY ? 0x57
                       : point.route == COMBO_PORTAL_SNOWHEAD  ? 0x59
                                                               : -1;
    return wanted == scene && wanted >= 0 && point.entrance >= 0 && point.room >= 0 && point.room < 32 &&
           std::isfinite(point.pose.pos.x) && std::isfinite(point.pose.pos.y) && std::isfinite(point.pose.pos.z) &&
           std::fabs(point.pose.pos.x) < 32768 && std::fabs(point.pose.pos.y) < 32768 &&
           std::fabs(point.pose.pos.z) < 32768;
}
inline bool InMouth(const Pose& mouth, Point point) {
    const auto delta = Sub(point, mouth.pos);
    const auto front = Forward(mouth.yaw);
    const float width = delta.x * front.z - delta.z * front.x, depth = Dot(delta, front);
    return std::fabs(width) < 22 && depth > 0 && depth < 34 && delta.y > -20 && delta.y < 120;
}
// Snowhead is an open valley threshold. Test feet against its sloping ground
// across the whole aperture instead of a cave-height box at a single Y.
inline bool InSnowheadMouth(const Pose& mouth, Point point) {
    const auto delta = Sub(point, mouth.pos);
    const auto front = Forward(mouth.yaw);
    const float across = delta.x * front.z - delta.z * front.x, depth = Dot(delta, front);
    float floor;
    return std::fabs(across) < 90 && depth > 0 && depth < 34 && ComboFountainValley::BodyClearAt(point.x, point.z) &&
           ComboFountainValley::FloorAt(point.x, point.z, &floor) && point.y >= floor - 5 && point.y < floor + 20;
}
inline bool SnowheadReturn(Pose* pose) {
    const auto p = ComboFountainSnowhead::kReturn;
    float floor;
    if (!pose || !ComboFountainValley::BodyClearAt(p.x, p.z) || !ComboFountainValley::FloorAt(p.x, p.z, &floor))
        return false;
    *pose = { { p.x, floor, p.z }, ComboFountainSnowhead::kYaw, Forward(ComboFountainSnowhead::kYaw) };
    return true;
}
// All dimensions are world units. Providers must reject dynamic/wet/exit floor,
// walls from actors and occupied architecture. No scene resource is modified.
template <class World> bool ValidateMouth(World& world, Point hit, Point front, Pose* result) {
    const Point right{ front.z, 0, -front.x };
    const Point mouth = Add(hit, front, 2);
    float floorY;
    if (!world.Floor(Add(mouth, front, 24), hit.y, &floorY))
        return false;
    for (float width : { -kHalfWidth, 0.0f, kHalfWidth }) {
        for (float height : { 20.0f, 85.0f, 136.0f }) {
            Point a = Add(Add(mouth, right, width), front, 24), b = Add(a, front, -80), found{}, normal{};
            a.y = b.y = floorY + height;
            if (!world.Wall(a, b, &found, &normal) || std::fabs(Dot(Sub(found, hit), front)) > 3)
                return false;
        }
        for (float depth : { 12.0f, 45.0f, 100.0f, 160.0f }) {
            Point sample = Add(Add(mouth, right, width), front, depth);
            sample.y = floorY;
            float y;
            if (!world.Floor(sample, floorY, &y) || std::fabs(y - floorY) > 12)
                return false;
            Point bottom = sample, top = sample;
            bottom.y = y + 1;
            top.y = y + kHeight + 4;
            if (!world.Clear(bottom, top))
                return false;
        }
    }
    for (float width : { -42.f, 0.f, 42.f })
        for (float height : { 20.f, 80.f, 133.f }) {
            Point a = Add(Add(mouth, right, width), front, 12), b = Add(a, front, 148), found{}, normal{};
            a.y = b.y = floorY + height;
            if (!world.Clear(a, b))
                return false;
        }
    Point apron = Add(mouth, front, 80);
    apron.y = floorY;
    if (world.Occupied(apron, 105))
        return false;
    *result = { { mouth.x, floorY, mouth.z }, Yaw(front), front };
    return true;
}
// Validate the connection as well as the mouth. Starting rays beyond a nearer
// wall must never discover a portal in a disconnected part of the room.
template <class World> bool Reachable(World& world, Pose entry, const Pose& mouth) {
    Point start = entry.pos, target = Add(mouth.pos, Forward(mouth.yaw), 100);
    float startY;
    if (!world.Floor(start, start.y, &startY))
        return false;
    start.y = startY;
    const float dx = target.x - start.x, dz = target.z - start.z;
    const float distance = std::sqrt(dx * dx + dz * dz);
    const Point right = distance > 1 ? Point{ dz / distance, 0, -dx / distance } : Point{ 1, 0, 0 };
    for (float height : { 10.f, 60.f, 125.f }) {
        Point a = start, b = target;
        a.y += height;
        b.y += height;
        if (!world.Clear(a, b))
            return false;
    }
    int steps = static_cast<int>(std::ceil(distance / 12));
    if (steps < 1)
        steps = 1;
    float previous = startY;
    for (int step = 0; step <= steps; ++step) {
        const float t = static_cast<float>(step) / steps;
        Point center{ start.x + dx * t, start.y + (target.y - start.y) * t, start.z + dz * t };
        for (float width : { -20.f, 0.f, 20.f }) {
            Point point = Add(center, right, width);
            float y;
            if (!world.Floor(point, center.y, &y))
                return false;
            if (width == 0) {
                if (std::fabs(y - previous) > 16)
                    return false;
                previous = y;
            }
            Point bottom = point, top = point;
            bottom.y = y + 1;
            top.y = y + kHeight + 4;
            if (!world.Clear(bottom, top))
                return false;
        }
    }
    return true;
}
template <class World> bool FindMouth(World& world, Pose entry, Pose* result, bool shoreline = false) {
    // Search beside the native approach, leaving the original entrance and its
    // first 120 units alone. Every candidate still needs the whole static wall.
    for (float radius : { 180.0f, 280.0f, 420.0f, 600.0f }) {
        for (int step = 0; step < 16; ++step) {
            const short angle = static_cast<short>(entry.yaw + step * 4096);
            const Point direction = Forward(angle);
            Point from = Add(entry.pos, direction, radius - 100), to = Add(entry.pos, direction, radius + 100);
            float floorY;
            if (!world.Floor(from, entry.pos.y, &floorY))
                continue;
            from.y = to.y = floorY + 60;
            Point hit{}, normal{};
            if (!world.Wall(from, to, &hit, &normal))
                continue;
            normal.y = 0;
            const float length = std::sqrt(Dot(normal, normal));
            if (length < .9f)
                continue;
            normal.x /= length;
            normal.z /= length;
            if (Dot(Sub(from, hit), normal) < 0) {
                normal.x = -normal.x;
                normal.z = -normal.z;
            }
            hit.y = floorY;
            if (Distance2(hit, entry.pos) < 140 * 140 || Dot(Sub(hit, entry.pos), Forward(entry.yaw)) < 0)
                continue;
            if (shoreline && !world.NearWater(hit))
                continue;
            Pose candidate;
            if (ValidateMouth(world, hit, normal, &candidate) && Reachable(world, entry, candidate)) {
                *result = candidate;
                return true;
            }
        }
    }
    return false;
}
} // namespace ComboRegional
#endif
