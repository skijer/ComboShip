#pragma once
#include "LakeHyliaBridge.h"

namespace ComboLakeHylia {
// Drawing alone consumes this partition. BuildGeometry() remains the sole
// collision/water owner, so changing the bank materials cannot move a floor.
struct SurfaceGeometry {
    std::vector<Quad> stone, sand;
};
inline SurfaceGeometry BuildSurfaceGeometry() {
    SurfaceGeometry result;
    const auto mesh = BuildGeometry();
    const auto interpolate = [](Point a, Point b, float t) {
        return Point{ a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t };
    };
    const auto coverage = [](float x, bool farBank) {
        const float distance = x - kBackX;
        return .50f + .10f * std::sin(distance * .031f + (farBank ? 1.2f : 0.f)) +
               .04f * std::sin(distance * .079f + (farBank ? .7f : 2.f));
    };
    for (auto q : mesh.solid) {
        const bool floor = q.a.y == kMouth.y && q.b.y == kMouth.y && q.c.y == kMouth.y && q.d.y == kMouth.y;
        const auto channelA = ChannelAt(q.a.x), channelB = ChannelAt(q.d.x);
        const bool nearBank =
            floor && std::fabs(q.b.z - channelA.nearZ) < .001f && std::fabs(q.c.z - channelB.nearZ) < .001f;
        const bool farBank =
            floor && std::fabs(q.a.z - channelA.farZ) < .001f && std::fabs(q.d.z - channelB.farZ) < .001f;
        if (!nearBank && !farBank) {
            if (q.shade == 0) {
                // The old rectangle ended exactly at the vault edges. Give
                // its black visual skin an overlap to cover raster slivers;
                // the collision cap and shared mouth/shimmer never move.
                for (auto* p : { &q.a, &q.b, &q.c, &q.d }) {
                    p->z += p->z < kMouth.z ? -2.f : 2.f;
                    p->y += p->y == kMouth.y ? -2.f : 2.f;
                }
            }
            result.stone.push_back(q);
            continue;
        }
        // Subdivide only the visual floor. Both materials share the exact
        // computed edge and original stream bank, with no raised decals.
        for (int section = 0; section < 3; ++section) {
            const float t0 = section / 3.f, t1 = (section + 1) / 3.f;
            const auto a = interpolate(q.a, q.d, t0), d = interpolate(q.a, q.d, t1);
            const auto b = interpolate(q.b, q.c, t0), c = interpolate(q.b, q.c, t1);
            const auto edgeA = interpolate(a, b, nearBank ? 1.f - coverage(a.x, false) : coverage(a.x, true));
            const auto edgeB = interpolate(d, c, nearBank ? 1.f - coverage(d.x, false) : coverage(d.x, true));
            const Quad left{ a, edgeA, edgeB, d, q.shade, false };
            const Quad right{ edgeA, b, c, edgeB, q.shade, false };
            result.stone.push_back(nearBank ? left : right);
            result.sand.push_back(nearBank ? right : left);
        }
    }
    return result;
}
} // namespace ComboLakeHylia
