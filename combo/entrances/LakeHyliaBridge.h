#pragma once
#ifdef __cplusplus
#include <algorithm>
#include <cmath>
#include <vector>

// Authored world-space geometry. No scene/actor/triangle search chooses this pose.
// spot06 collision631/632: x=-2245,z3637..3797,y=-1313..-1033.
// The cave attaches to that marked bank below the laboratory, wholly lakeward
// of the protected wall. The outer-only collar overlaps the original bank;
// the deep hollow and travel path never penetrate the laboratory mesh.
namespace ComboLakeHylia {
struct Point {
    float x, y, z;
};
struct Triangle {
    Point a, b, c;
};
struct Quad {
    Point a, b, c, d;
    unsigned char shade;
    bool collides;
};
struct Geometry {
    std::vector<Quad> solid, water;
};
inline constexpr Point kMouth{ -2185, -1305, 3717 };
inline constexpr short kYaw = 16384; // +X: the approach, return, shimmer and waves.
inline constexpr Point kBank{ -2310, -1033, 3860 };
inline constexpr Point kApron{ -1875, -1305, 3717 };
inline constexpr float kFacadeX = -1915, kBackX = -2237;
// Outer-only rear collar overlaps the measured straight/angled bank wall.
// The walkable vault, back cap and shared mouth retain their authored positions.
inline constexpr float kRockJoinX = -2270;
inline constexpr float kPlinthBaseY = -1405;
// One exact hollow profile is shared by the vault, black back cap and shimmer.
inline constexpr float kOpeningBaseY = 0, kOpeningHalfWidth = 90, kOpeningSideHeight = 150;
inline constexpr float kOpeningShoulderHalfWidth = 52, kOpeningShoulderHeight = 190, kOpeningHeight = 216;
inline constexpr float OpeningTopAt(float across) {
    const float width = across < 0 ? -across : across;
    if (width > kOpeningHalfWidth)
        return 0;
    if (width >= kOpeningShoulderHalfWidth)
        return kOpeningSideHeight + (kOpeningHalfWidth - width) * ((kOpeningShoulderHeight - kOpeningSideHeight) /
                                                                   (kOpeningHalfWidth - kOpeningShoulderHalfWidth));
    return kOpeningHeight - width * ((kOpeningHeight - kOpeningShoulderHeight) / kOpeningShoulderHalfWidth);
}
inline constexpr Point kOpeningSection[] = { { 0, kOpeningBaseY, -kOpeningHalfWidth },
                                             { 0, OpeningTopAt(-kOpeningHalfWidth), -kOpeningHalfWidth },
                                             { 0, OpeningTopAt(-kOpeningShoulderHalfWidth),
                                               -kOpeningShoulderHalfWidth },
                                             { 0, OpeningTopAt(0), 0 },
                                             { 0, OpeningTopAt(kOpeningShoulderHalfWidth), kOpeningShoulderHalfWidth },
                                             { 0, OpeningTopAt(kOpeningHalfWidth), kOpeningHalfWidth },
                                             { 0, kOpeningBaseY, kOpeningHalfWidth } };
// Local +Z faces lakeward after kYaw. Place the field against the actual black
// back cap, ten units in front, without moving the departure/return mouth.
inline constexpr float kShimmerFront = kBackX - kMouth.x + 10;
inline constexpr float kFullWaterY = -1313, kDrainedWaterY = -1993, kChannelFloorY = -1333;
struct ChannelSection {
    float x, nearZ, farZ;
};
// An eroded side course leaves the central route (z3717 +/-20) dry. The
// lakeward end flares through an open notch in the authored apron.
inline constexpr ChannelSection kChannel[] = { { kBackX, 3746, 3785 }, { -2191, 3742, 3789 },    { -2145, 3747, 3794 },
                                               { -2099, 3740, 3789 },  { -2053, 3744, 3794 },    { -2007, 3740, 3797 },
                                               { -1961, 3745, 3793 },  { kFacadeX, 3741, 3791 }, { -1870, 3740, 3798 },
                                               { -1825, 3738, 3810 } };
inline constexpr float kChannelBankInset = 4;
inline ChannelSection ChannelAt(float x) {
    for (size_t i = 1; i < sizeof(kChannel) / sizeof(kChannel[0]); ++i) {
        if (x > kChannel[i].x)
            continue;
        const auto a = kChannel[i - 1], b = kChannel[i];
        const float t = std::clamp((x - a.x) / (b.x - a.x), 0.f, 1.f);
        return { x, a.nearZ + (b.nearZ - a.nearZ) * t, a.farZ + (b.farZ - a.farZ) * t };
    }
    auto result = kChannel[sizeof(kChannel) / sizeof(kChannel[0]) - 1];
    result.x = x;
    return result;
}
inline ChannelSection WetChannelAt(float x, float surfaceY) {
    auto result = ChannelAt(x);
    const float inset = kChannelBankInset * std::clamp((kMouth.y - surfaceY) / (kMouth.y - kChannelFloorY), 0.f, 1.f);
    result.nearZ += inset;
    result.farZ -= inset;
    return result;
}
// Retain the established dry variant when the native lake is drained. The
// short outflow must not be extended down through the exposed lake bed.
inline bool HasInlet(float lakeSurface) {
    return std::isfinite(lakeSurface) && lakeSurface >= kChannelFloorY + 2;
}
inline float StreamHeight(float lakeSurface) {
    return std::clamp(lakeSurface, kChannelFloorY + 2, kFullWaterY);
}
inline constexpr float kStreamLipX = -1825, kStreamJoinX = kStreamLipX + 1;
inline constexpr float kStreamSourceY = kMouth.y - 1, kStreamLipY = kMouth.y - 3;
inline constexpr float kStreamFallTopY = kStreamLipY - .5f;
inline constexpr size_t kMaxWaterQuads = 27 + 1 + 8;
inline float ChannelSurfaceHeight(float x) {
    const float t = std::clamp((x - kBackX) / (kStreamLipX - kBackX), 0.f, 1.f);
    return kStreamSourceY + (kStreamLipY - kStreamSourceY) * t;
}
inline float WaterFlowDistance(Point p) {
    if (p.x <= kStreamLipX)
        return p.x - kBackX;
    const float channel = kStreamLipX - kBackX;
    if (p.y >= kStreamFallTopY)
        return channel + std::hypot(p.x - kStreamLipX, p.y - kStreamLipY);
    const float edge = std::hypot(kStreamJoinX - kStreamLipX, kStreamFallTopY - kStreamLipY);
    return channel + edge + kStreamFallTopY - p.y;
}
inline std::vector<Quad> BuildWaterGeometry(float lakeSurface) {
    std::vector<Quad> water;
    if (!HasInlet(lakeSurface))
        return water;
    water.reserve(kMaxWaterQuads);
    const auto point = [&](float x, float y, bool farBank) {
        const auto bank = WetChannelAt(x, y);
        return Point{ x, y, farBank ? bank.farZ : bank.nearZ };
    };
    const auto quad = [&](float x0, float y0, float x1, float y1) {
        water.push_back(
            { point(x0, y0, false), point(x0, y0, true), point(x1, y1, true), point(x1, y1, false), 170, false });
    };
    // Follow every stone-bank bend before crossing the lip. The descending
    // surface is fed from the cave, rather than set to one level by the lake.
    for (size_t i = 1; i < sizeof(kChannel) / sizeof(kChannel[0]); ++i) {
        const float a = kChannel[i - 1].x, b = kChannel[i].x;
        for (int j = 0; j < 3; ++j) {
            const float x0 = a + (b - a) * j / 3, x1 = a + (b - a) * (j + 1) / 3;
            quad(x0, ChannelSurfaceHeight(x0), x1, ChannelSurfaceHeight(x1));
        }
    }
    // Turn over the edge, then fall at a fixed X against the front face.
    // The receiving lake determines the bottom; there is no horizontal fan.
    quad(kStreamLipX, kStreamLipY, kStreamJoinX, kStreamFallTopY);
    const float contactY = StreamHeight(lakeSurface);
    const int falls = std::clamp(int(std::ceil((kStreamFallTopY - contactY) / 4)), 1, 8);
    for (int i = 0; i < falls; ++i)
        quad(kStreamJoinX, kStreamFallTopY + (contactY - kStreamFallTopY) * i / falls, kStreamJoinX,
             kStreamFallTopY + (contactY - kStreamFallTopY) * (i + 1) / falls);
    return water;
}
inline Geometry BuildGeometry() {
    Geometry mesh;
    auto quad = [&](Point a, Point b, Point c, Point d, unsigned char shade = 170, bool collision = true) {
        mesh.solid.push_back({ a, b, c, d, shade, collision });
    };
    // Collision is directional: inner surfaces face hollow free space; outer
    // surfaces face away from the solid rock. Drawing's no-cull mode is unrelated.
    auto reverseQuad = [&](Point a, Point b, Point c, Point d, unsigned char shade = 170, bool collision = true) {
        quad(a, d, c, b, shade, collision);
    };
    // Fit each floor/bank segment to the same channel profile. The inset bed
    // forms visible stone sides; water meets those sides below the dry lip.
    for (size_t i = 1; i < sizeof(kChannel) / sizeof(kChannel[0]); ++i) {
        const auto a = kChannel[i - 1], b = kChannel[i];
        const float north = a.x >= kFacadeX ? 3615 : 3627;
        const float south = a.x >= kFacadeX ? 3815 : 3807;
        quad({ a.x, kMouth.y, north }, { a.x, kMouth.y, a.nearZ }, { b.x, kMouth.y, b.nearZ }, { b.x, kMouth.y, north },
             160);
        quad({ a.x, kMouth.y, a.farZ }, { a.x, kMouth.y, south }, { b.x, kMouth.y, south }, { b.x, kMouth.y, b.farZ },
             135);
        quad({ a.x, kChannelFloorY, a.nearZ + kChannelBankInset }, { a.x, kChannelFloorY, a.farZ - kChannelBankInset },
             { b.x, kChannelFloorY, b.farZ - kChannelBankInset }, { b.x, kChannelFloorY, b.nearZ + kChannelBankInset },
             75);
        reverseQuad({ a.x, kMouth.y, a.nearZ }, { b.x, kMouth.y, b.nearZ },
                    { b.x, kChannelFloorY, b.nearZ + kChannelBankInset },
                    { a.x, kChannelFloorY, a.nearZ + kChannelBankInset }, 90);
        reverseQuad({ a.x, kChannelFloorY, a.farZ - kChannelBankInset },
                    { b.x, kChannelFloorY, b.farZ - kChannelBankInset }, { b.x, kMouth.y, b.farZ },
                    { a.x, kMouth.y, a.farZ }, 90);
    }
    // A 322-unit hollow rock vault, progressively darker at its deep end.
    for (int ring = 0; ring < 7; ++ring) {
        float x0 = kBackX + (kFacadeX - kBackX) * ring / 7;
        float x1 = kBackX + (kFacadeX - kBackX) * (ring + 1) / 7;
        for (int side = 0; side < 6; ++side) {
            auto a = kOpeningSection[side], b = kOpeningSection[side + 1];
            reverseQuad({ x0, kMouth.y + a.y, kMouth.z + a.z }, { x0, kMouth.y + b.y, kMouth.z + b.z },
                        { x1, kMouth.y + b.y, kMouth.z + b.z }, { x1, kMouth.y + a.y, kMouth.z + a.z }, 30 + ring * 20);
        }
    }
    // Recessed cap is behind the rear trigger; the front remains truly hollow.
    reverseQuad({ kBackX, kMouth.y, kMouth.z - kOpeningHalfWidth }, { kBackX, kMouth.y, kMouth.z + kOpeningHalfWidth },
                { kBackX, kMouth.y + kOpeningHeight, kMouth.z + kOpeningHalfWidth },
                { kBackX, kMouth.y + kOpeningHeight, kMouth.z - kOpeningHalfWidth }, 0);
    // Jagged rock reveal around the facade, with thickness rather than a flat arch.
    for (int side = 0; side < 6; ++side) {
        auto a = kOpeningSection[side], b = kOpeningSection[side + 1];
        auto outer = [](Point p) {
            p.z *= 1.28f;
            p.y = p.y * 1.13f + 6;
            return p;
        };
        auto oa = outer(a), ob = outer(b);
        reverseQuad({ kFacadeX, kMouth.y + a.y, kMouth.z + a.z }, { kFacadeX, kMouth.y + b.y, kMouth.z + b.z },
                    { kFacadeX + 18, kMouth.y + ob.y, kMouth.z + ob.z },
                    { kFacadeX + 18, kMouth.y + oa.y, kMouth.z + oa.z }, 175);
        // Outer roof joins the bank. It is visible from above and from the lab.
        reverseQuad(
            { kRockJoinX, kMouth.y + oa.y, kMouth.z + oa.z }, { kFacadeX + 18, kMouth.y + oa.y, kMouth.z + oa.z },
            { kFacadeX + 18, kMouth.y + ob.y, kMouth.z + ob.z }, { kRockJoinX, kMouth.y + ob.y, kMouth.z + ob.z }, 140);
    }
    // Modest shore apron and vertical rock skirt. The native basin rises to
    // within20 units of its north lip; its embedded base never forms a ramp.
    for (float z : { 3615.f, 3815.f }) {
        const Point a{ kRockJoinX, kPlinthBaseY, z }, b{ -1825, kPlinthBaseY, z }, c{ -1825, kMouth.y, z },
            d{ kRockJoinX, kMouth.y, z };
        if (z < kMouth.z)
            reverseQuad(a, b, c, d, 125);
        else
            quad(a, b, c, d, 125);
    }
    const auto intake = ChannelAt(-1825);
    // Leave a real lake-connected opening above the bed rather than drawing
    // water through the solid vertical front of the apron.
    reverseQuad({ -1825, kMouth.y, 3615 }, { -1825, kPlinthBaseY, 3615 }, { -1825, kPlinthBaseY, intake.nearZ },
                { -1825, kMouth.y, intake.nearZ }, 120);
    reverseQuad({ -1825, kChannelFloorY, intake.nearZ + kChannelBankInset }, { -1825, kPlinthBaseY, intake.nearZ },
                { -1825, kPlinthBaseY, intake.farZ }, { -1825, kChannelFloorY, intake.farZ - kChannelBankInset }, 80);
    reverseQuad({ -1825, kMouth.y, intake.farZ }, { -1825, kPlinthBaseY, intake.farZ }, { -1825, kPlinthBaseY, 3815 },
                { -1825, kMouth.y, 3815 }, 120);
    // Close the small beveled corners below the open water slot. A midpoint
    // splits each triangular cap into two valid collision/draw triangles.
    const float middleY = (kPlinthBaseY + kChannelFloorY) / 2;
    reverseQuad({ -1825, kMouth.y, intake.nearZ }, { -1825, kPlinthBaseY, intake.nearZ },
                { -1825, middleY, intake.nearZ + kChannelBankInset / 2 },
                { -1825, kChannelFloorY, intake.nearZ + kChannelBankInset }, 120);
    quad({ -1825, kMouth.y, intake.farZ }, { -1825, kPlinthBaseY, intake.farZ },
         { -1825, middleY, intake.farZ - kChannelBankInset / 2 },
         { -1825, kChannelFloorY, intake.farZ - kChannelBankInset }, 120);
    // The native lab bank stays untouched. Access uses the existing lake/shore
    // and standard ledge behavior; no switchback, landing or raised support slab.
    // Preview/control geometry at the full lake level. The draw provider uses
    // the same builder with the current native water surface.
    mesh.water = BuildWaterGeometry(kFullWaterY);
    return mesh;
}
} // namespace ComboLakeHylia
#endif
