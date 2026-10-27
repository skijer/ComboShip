#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

// Find the compact horizontal cap in the loaded collision, so Prelude can move
// the stump without leaving its reward at an obsolete position. No scene bytes
// or collision surfaces are changed.
namespace MagicMirrorAnchor {
struct Point {
    float x, y, z;
};
struct Triangle {
    std::array<Point, 3> p;
};
inline bool Finite(const Point& p) {
    return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z);
}
inline bool Near(const Point& a, const Point& b) {
    return std::abs(a.x - b.x) < 1 && std::abs(a.y - b.y) < 1 && std::abs(a.z - b.z) < 1;
}
inline bool Touch(const Triangle& a, const Triangle& b) {
    for (const auto& p : a.p)
        for (const auto& q : b.p)
            if (Near(p, q))
                return true;
    return false;
}
inline float Area(const Triangle& t) {
    return std::abs((t.p[1].x - t.p[0].x) * (t.p[2].z - t.p[0].z) - (t.p[1].z - t.p[0].z) * (t.p[2].x - t.p[0].x)) *
           0.5f;
}
inline bool Candidate(const Triangle& triangle) {
    for (const auto& p : triangle.p)
        if (!Finite(p) || std::abs(p.y - triangle.p[0].y) >= 1)
            return false;
    return Area(triangle) > 1;
}
inline bool Find(const std::vector<Triangle>& source, Point& result) {
    std::vector<Triangle> triangles;
    for (const auto& t : source) {
        if (Candidate(t))
            triangles.push_back(t);
        if (triangles.size() > 2048)
            return false;
    }
    std::vector<bool> visited(triangles.size());
    bool found = false;
    for (size_t first = 0; first < triangles.size(); ++first) {
        if (visited[first])
            continue;
        std::vector<size_t> component{ first };
        visited[first] = true;
        float minX = triangles[first].p[0].x, maxX = minX, minZ = triangles[first].p[0].z, maxZ = minZ, area = 0;
        for (size_t next = 0; next < component.size(); ++next) {
            const auto& t = triangles[component[next]];
            area += Area(t);
            for (const auto& p : t.p) {
                minX = std::min(minX, p.x);
                maxX = std::max(maxX, p.x);
                minZ = std::min(minZ, p.z);
                maxZ = std::max(maxZ, p.z);
            }
            for (size_t other = 0; other < triangles.size(); ++other)
                if (!visited[other] && Touch(t, triangles[other])) {
                    visited[other] = true;
                    component.push_back(other);
                }
        }
        const float width = maxX - minX, depth = maxZ - minZ;
        // The authored stump is a 12-triangle radial fan, with 12 rim vertices
        // and a shared central hub. Its footprint survives Prelude translation
        // and rotation; height and distance from the origin are not identities.
        if (component.size() != 12 || width < 100 || width > 122 || depth < 100 || depth > 122 ||
            area < width * depth * 0.65f || area > width * depth * 0.9f)
            continue;
        const Point center{ (minX + maxX) * 0.5f, triangles[first].p[0].y, (minZ + maxZ) * 0.5f };
        std::vector<Point> vertices;
        bool radial = true;
        for (const auto index : component) {
            bool hub = false;
            for (const auto& p : triangles[index].p) {
                hub |= Near(p, center);
                if (std::none_of(vertices.begin(), vertices.end(), [&](const Point& q) { return Near(p, q); }))
                    vertices.push_back(p);
            }
            radial &= hub;
        }
        if (!radial || vertices.size() != 13)
            continue;
        if (found)
            return false; // An ambiguous duplicate must not steal the reward/arrival.
        result = center;
        found = true;
    }
    return found;
}
inline bool CanClaim(const Point& player, const Point& cap) {
    const float x = player.x - cap.x, z = player.z - cap.z, y = player.y - cap.y;
    return Finite(player) && Finite(cap) && x * x + z * z <= 45 * 45 && std::abs(y) <= 5;
}
} // namespace MagicMirrorAnchor
