#pragma once

#include "ForestGiParticles.h"
#include "../entrances/FountainSnowheadBridge.h"
#include "../entrances/LakeHyliaBridge.h"
#include "../entrances/LostWoodsEntrance.h"
#include <array>
#include <cstdint>

namespace RegionalPassagePresentation {
enum class Host { Oot, Mm };

inline bool KnownRoute(int route) {
    return route == COMBO_PORTAL_IKANA || route == COMBO_PORTAL_GREAT_BAY || route == COMBO_PORTAL_SNOWHEAD;
}

inline uint32_t OuterColor(int route) {
    switch (route) {
        case COMBO_PORTAL_IKANA:
            return 0xE8A13A;
        case COMBO_PORTAL_GREAT_BAY:
            return 0x174A9C;
        case COMBO_PORTAL_SNOWHEAD:
            return 0xA8DFFF;
        default:
            return 0;
    }
}

inline NeiGi::Mesh Shimmer(uint32_t frame, int route, const NeiGi::Basis& camera) {
    const uint32_t outer = OuterColor(route);
    if (!outer)
        return {};
    auto mesh = ForestGiParticles::Mystery(frame, camera);
    for (size_t i = 0; i < mesh.count; ++i)
        if (mesh.vertices[i].rgb == NeiGi::ColorHex(NeiGi::Kind::Leaf))
            mesh.vertices[i].rgb = outer;
    return mesh;
}

inline NeiGi::Mesh GraveShimmer(uint32_t frame, const NeiGi::Basis& camera) {
    auto mesh = Shimmer(frame, COMBO_PORTAL_IKANA, camera);
    for (size_t first = 0; first < mesh.count; first += ForestGiParticles::kTwinkleVertices) {
        const auto origin = mesh.vertices[first].p;
        const size_t index = first / ForestGiParticles::kTwinkleVertices;
        const float angle = (index % 10) * NeiGi::Tau / 10;
        // Native object_haka at scale0.1 occupies x±34.6,y0..48,z±36.
        // The ten crown and ten side centers leave the entire 5.6-unit NEI
        // twinkle outside that solid, while normal depth tests remain enabled.
        // They stay at the aperture's home pose when the native stone is pulled.
        const float radius = index < 10 ? 28.f : 54.f;
        NeiGi::Point center{ std::cos(angle) * radius, index < 10 ? 62.f : 18.f + (index % 3) * 10.f,
                             std::sin(angle) * radius };
        for (size_t j = 0; j < ForestGiParticles::kTwinkleVertices; ++j)
            mesh.vertices[first + j].p = center + (mesh.vertices[first + j].p - origin);
    }
    return mesh;
}

inline const char* Texture(Host host, int route) {
    if (!KnownRoute(route))
        return nullptr;
    if (host == Host::Oot)
        return route == COMBO_PORTAL_SNOWHEAD ? "__OTR__objects/object_spot08_obj/gZorasFountainIceRampMiddleSlopeTex"
                                              : "__OTR__objects/gameplay_keep/gRockFragment1Tex";
    return route == COMBO_PORTAL_SNOWHEAD ? "__OTR__objects/object_icefloe/gIcefloePlatformTex"
                                          : "__OTR__objects/gameplay_keep/gBoulderFragmentsTex";
}

constexpr size_t kArchSegments = 10;
constexpr float kShellHalfWidth = 60, kShellSpringHeight = 66;
constexpr size_t kShellQuads = 2 + kArchSegments * 3;
// Includes setup/restore, the texture load, a matrix and native debug packets.
constexpr size_t kShellOpaCommands = kShellQuads * 2 + 32;
// Native shell/setup OPEN_DISPS pairs write XLU debug nodes transiently, even
// when their CLOSE_DISPS later rewinds the unused XLU head (observed peak2).
constexpr size_t kShellXluCommands = 8;
struct ShellVertex {
    NeiGi::Point p;
    uint8_t shade;
};

inline std::array<ShellVertex, kShellQuads * 4> BuildShell() {
    std::array<ShellVertex, kShellQuads * 4> vertices{};
    size_t index = 0;
    const auto quad = [&](NeiGi::Point a, NeiGi::Point b, NeiGi::Point c, NeiGi::Point d, uint8_t shade) {
        for (auto point : { a, b, c, d })
            vertices[index++] = { point, shade };
    };
    quad({ -68, 0, 0 }, { -kShellHalfWidth, 0, 0 }, { -kShellHalfWidth, kShellSpringHeight, 0 }, { -68, 65, 0 }, 210);
    quad({ kShellHalfWidth, 0, 0 }, { 68, 0, 0 }, { 68, 65, 0 }, { kShellHalfWidth, kShellSpringHeight, 0 }, 210);
    for (size_t i = 0; i < kArchSegments; ++i) {
        const float a = i * NeiGi::Tau * .5f / kArchSegments;
        const float b = (i + 1) * NeiGi::Tau * .5f / kArchSegments;
        const float ax = std::cos(a), ay = std::sin(a), bx = std::cos(b), by = std::sin(b);
        quad({ ax * kShellHalfWidth, kShellSpringHeight + ay * kShellHalfWidth, 0 }, { ax * 68, 65 + ay * 68, 0 },
             { bx * 68, 65 + by * 68, 0 }, { bx * kShellHalfWidth, kShellSpringHeight + by * kShellHalfWidth, 0 }, 210);
        quad({ ax * kShellHalfWidth, kShellSpringHeight + ay * kShellHalfWidth, 0 },
             { bx * kShellHalfWidth, kShellSpringHeight + by * kShellHalfWidth, 0 },
             { bx * kShellHalfWidth, kShellSpringHeight + by * kShellHalfWidth, -45 },
             { ax * kShellHalfWidth, kShellSpringHeight + ay * kShellHalfWidth, -45 }, 65);
        // Matching trapezoids close only the curved aperture, never the rock
        // outside its arch. The native wall and its collision remain intact.
        quad({ ax * kShellHalfWidth, 0, -1 }, { bx * kShellHalfWidth, 0, -1 },
             { bx * kShellHalfWidth, kShellSpringHeight + by * kShellHalfWidth, -1 },
             { ax * kShellHalfWidth, kShellSpringHeight + ay * kShellHalfWidth, -1 }, 0);
    }
    return vertices;
}

template <class Vtx> inline std::array<Vtx, kShellQuads * 4> NativeShell() {
    std::array<Vtx, kShellQuads * 4> result{};
    const auto shell = BuildShell();
    for (size_t i = 0; i < result.size(); ++i) {
        auto& native = result[i].v;
        const auto& source = shell[i];
        native.ob[0] = static_cast<int16_t>(std::round(source.p.x));
        native.ob[1] = static_cast<int16_t>(std::round(source.p.y));
        native.ob[2] = static_cast<int16_t>(source.p.z);
        native.tc[0] = static_cast<int16_t>(source.p.x * 16 + source.p.z * 32);
        native.tc[1] = static_cast<int16_t>(source.p.y * -16);
        native.cn[0] = native.cn[1] = native.cn[2] = source.shade;
        native.cn[3] = 255;
    }
    return result;
}

inline float ShellTopAt(float across) {
    // Match the actual integer-packed native shell's ten straight roof edges.
    for (size_t i = 0; i < kArchSegments; ++i) {
        const float a = i * NeiGi::Tau * .5f / kArchSegments, b = (i + 1) * NeiGi::Tau * .5f / kArchSegments;
        const float ax = std::round(std::cos(a) * kShellHalfWidth), bx = std::round(std::cos(b) * kShellHalfWidth);
        if (across < bx || across > ax)
            continue;
        const float ay = std::round(kShellSpringHeight + std::sin(a) * kShellHalfWidth);
        const float by = std::round(kShellSpringHeight + std::sin(b) * kShellHalfWidth);
        return ay + (by - ay) * (across - ax) / (bx - ax);
    }
    return 0;
}

inline NeiGi::Mesh Shimmer(uint32_t frame, Host host, int route, const NeiGi::Basis& camera) {
    if (host == Host::Oot && route == COMBO_PORTAL_IKANA)
        return GraveShimmer(frame, camera);
    auto mesh = Shimmer(frame, route, camera);
    // The largest original glow is radius7 at scale0.8. Six world units also
    // cover native1/16 packing, so every full twinkle stays inside the face.
    constexpr float inset = 6;
    const bool valley = host == Host::Oot && route == COMBO_PORTAL_SNOWHEAD;
    const size_t columns = valley ? 10 : 5;
    const float halfWidth = host == Host::Mm                  ? kShellHalfWidth
                            : route == COMBO_PORTAL_GREAT_BAY ? ComboLakeHylia::kOpeningHalfWidth
                                                              : ComboFountainSnowhead::kOpeningHalfWidth;
    const float front = host == Host::Mm                  ? 10.f
                        : route == COMBO_PORTAL_GREAT_BAY ? ComboLakeHylia::kShimmerFront
                                                          : ComboFountainSnowhead::kShimmerFront;
    const float baseFloor = host == Host::Oot && route == COMBO_PORTAL_GREAT_BAY ? ComboLakeHylia::kOpeningBaseY : 0;
    const auto top = [&](float x) {
        return host == Host::Mm                  ? ShellTopAt(x)
               : route == COMBO_PORTAL_GREAT_BAY ? ComboLakeHylia::OpeningTopAt(x)
                                                 : ComboFountainSnowhead::kOpeningHeight;
    };
    for (size_t first = 0; first < mesh.count; first += ForestGiParticles::kTwinkleVertices) {
        const auto origin = mesh.vertices[first].p;
        const size_t index = first / ForestGiParticles::kTwinkleVertices;
        // Keep the twenty source twinkles at their original size. Ten columns
        // spread the Snowhead beacon between the asymmetric valley banks.
        const float x = valley ? ComboFountainSnowhead::kShimmerMinAcross + inset +
                                     float((index * 3) % columns) / float(columns - 1) *
                                         (ComboFountainSnowhead::kShimmerMaxAcross -
                                          ComboFountainSnowhead::kShimmerMinAcross - 2 * inset)
                               : (float((index * 3) % 5) * .5f - 1) * (halfWidth - inset);
        // Check both billboard extents against the shared piecewise roof;
        // a round approximation would spill beyond the Lake's sloped corners.
        float floor = baseFloor;
        if (valley) {
            // Complete billboard footprint follows the actual open valley,
            // including camera depth packing, without changing NEI twinkle art.
            for (float across : { x - inset, x, x + inset })
                for (float depth : { front - inset, front + inset }) {
                    float y;
                    if (ComboFountainValley::FloorAt(ComboFountainSnowhead::kMouth.x - depth, across, &y))
                        floor = std::fmax(floor, y - ComboFountainSnowhead::kMouth.y);
                }
        }
        const float ceiling = std::fmin(top(x - inset), top(x + inset)) - inset;
        const float row = float(index / columns) / (valley ? 1.f : 3.f);
        const NeiGi::Point center{ x, floor + inset + row * (ceiling - floor - inset), front };
        for (size_t j = 0; j < ForestGiParticles::kTwinkleVertices; ++j)
            mesh.vertices[first + j].p = center + (mesh.vertices[first + j].p - origin);
    }
    return mesh;
}

constexpr uint16_t kNativeWave = 0x28F2;
constexpr uint16_t kContinuousWave = kNativeWave - 0x800;
inline bool WavesInRange(int route, float distanceSquared, float height) {
    return route == COMBO_PORTAL_GREAT_BAY && std::isfinite(distanceSquared) && distanceSquared >= 0 &&
           distanceSquared <= 650.f * 650.f && std::isfinite(height) && std::fabs(height) <= 220.f;
}
} // namespace RegionalPassagePresentation
