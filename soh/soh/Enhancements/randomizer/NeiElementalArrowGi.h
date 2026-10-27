#pragma once
#include "NeiGiEffectPolicy.h"
#include "NeiElementalGiColors.h"
#include <algorithm>

// Receipt-only energy. The native/selected arrow core keeps its own resource;
// these meshes never bind a projectile actor's material or dynamic segment.
namespace NeiArrowGi {
using NeiGi::Basis;
using NeiGi::Mesh;
using NeiGi::Point;
// Identical native core in both games; retain its diagonal orientation.
constexpr Point Tip{ -15.f, -18.f, 0.f };
// The native broadhead spans (-15,-18,0) through (2,1,9). A mantle centered
// above its point surrounds the whole head instead of burning beside it.
constexpr Point HeadCenter{ -8.5f, -10.f, 1.f };
struct Layers {
    Mesh veil, energy, shimmer;
};
inline bool Valid(int profile) {
    return profile >= 1 && profile <= 3;
}
inline NeiGi::Kind Kind(int profile) {
    return profile == 1 ? NeiGi::Kind::Fire : profile == 2 ? NeiGi::Kind::Ice : NeiGi::Kind::Light;
}
inline const char* Texture(int profile) {
    return profile == 1   ? "__OTR__objects/nei_elemental_arrow_gi/fire"
           : profile == 2 ? "__OTR__objects/nei_elemental_arrow_gi/ice"
                          : "__OTR__objects/nei_elemental_arrow_gi/light";
}
inline void Sheet(Mesh& mesh, float t, float yaw, float low, float height, float width, uint32_t color,
                  uint8_t opacity) {
    // Ten curved strips keep the textured field wrapped around the shaft.
    // Separate planes move at different phases without rewriting cached texels.
    const Point across{ std::cos(yaw), 0, std::sin(yaw) };
    const Point depth{ -std::sin(yaw), 0, std::cos(yaw) };
    auto vertex = [&](int row, bool right) {
        const float s = row / 10.f;
        const float bend = std::sin(s * 4.4f - t * 2 + yaw) * (1.2f + s * 1.4f);
        const float half = width * (.8f + .2f * std::sin(s * 3.1f + t + yaw)) * (1.f - .62f * s);
        const Point p = across * ((right ? half : -half) + bend) +
                        depth * (2.2f + 1.4f * std::sin(s * 4.1f + t + yaw)) + Point{ 0, low + height * s, 0 };
        const float fade = std::min(1.f, std::min(s * 6, (1 - s) * 5));
        return NeiGi::EffectVertex{ p, color, uint8_t(opacity * fade), right ? 1.f : 0.f, 1.f - s };
    };
    for (int row = 0; row < 10; ++row) {
        const auto a = vertex(row, false), b = vertex(row, true);
        const auto c = vertex(row + 1, true), d = vertex(row + 1, false);
        mesh.Tri(a, b, c);
        mesh.Tri(a, c, d);
    }
}
inline void Spiral(Mesh& mesh, float t, float phase, float radius, float low, float height, float width, uint32_t edge,
                   uint32_t core, const Basis& camera, uint8_t alpha) {
    auto point = [&](int j) {
        const float s = j / 12.f, a = phase + s * NeiGi::Tau * 1.1f - t;
        const float r = radius * (.75f + .25f * std::sin(s * 3.14f));
        return Point{ std::cos(a) * r, low + height * s, std::sin(a) * r };
    };
    for (int j = 0; j < 12; ++j) {
        const float taper = std::sin((j + .5f) * 3.14159265f / 12);
        NeiGi::Band(mesh, point(j), point(j + 1), width * taper, edge, core, camera, uint8_t(alpha * taper));
    }
}
inline void Mantle(Mesh& mesh, const Basis& camera, float phase, uint8_t alpha) {
    constexpr int rings = 5, segments = 16;
    auto vertex = [&](int ring, int j) {
        const float latitude = ring * (NeiGi::Tau * .25f) / rings;
        const float angle = j * NeiGi::Tau / segments;
        const float x = std::sin(latitude) * std::cos(angle), y = std::sin(latitude) * std::sin(angle);
        const float pulse = 1.f + .035f * std::sin(latitude) * std::sin(angle * 3 + latitude * 4 - phase * 2);
        const Point p = NeiGi::Plane(camera, 13.5f * x * pulse, 14.f * y * pulse, 13.5f * std::cos(latitude) * pulse);
        const float v = std::fmax(
            0.f, std::fmin(1.f, .5f - .5f * y +
                                    .035f * std::sin(phase + angle * 2) * std::sin(latitude) * std::cos(latitude)));
        return NeiGi::EffectVertex{ p, 0xFFFFFF, uint8_t(alpha * (.25f + .75f * std::cos(latitude))), .5f + .5f * x,
                                    v };
    };
    for (int ring = 0; ring < rings; ++ring)
        for (int j = 0; j < segments; ++j) {
            mesh.Tri(vertex(ring, j), vertex(ring + 1, j), vertex(ring + 1, j + 1));
            if (ring)
                mesh.Tri(vertex(ring, j), vertex(ring + 1, j + 1), vertex(ring, j + 1));
        }
}
inline Layers Sample(int profile, uint32_t frame, const Basis& camera = {},
                     const NeiElementalGi::Palette* tint = nullptr, bool shop = false) {
    Layers layers;
    if (!Valid(profile))
        return layers;
    const auto color = tint ? *tint : NeiElementalGi::DefaultPalette(profile);
    const auto hot = NeiElementalGi::Mix(color.hot, 0xFFFFFF, .25f);
    const float t = (frame % 360u) * (NeiGi::Tau / 360.f);
    Mantle(layers.veil, camera, t, profile == 1 ? 210 : profile == 2 ? 175 : 185);
    if (profile == 1) {
        for (int s = 0; s < 3; ++s)
            Sheet(layers.veil, t + s * 1.3f, s * 1.0472f + .14f * std::sin(t), -5, 23 + 2 * std::sin(t * 2 + s), 6.1f,
                  0xFFFFFF, 175);
        for (int r = 0; r < 2; ++r)
            Spiral(layers.energy, t * 2, r * 3.14159265f, 4.8f, -4, 22, 2.3f, color.edge, hot, camera, 180);
        NeiGi::Glow(layers.energy, camera.forward * 10, 10, color.edge, 95, camera);
        for (int i = 0; i < 5; ++i) {
            const float f = std::fmod((frame % 360u) / 90.f + i / 5.f, 1.f), fade = std::sin(f * 3.14159265f);
            const Point p{ std::sin(t * 3 + i * 1.8f) * 5, 2 + f * 20, std::cos(t * 3 + i * 1.8f) * 4 };
            NeiGi::Band(layers.energy, p, p + Point{ .3f, 1.4f, 0 }, .4f * fade, color.edge, hot, camera,
                        uint8_t(205 * fade));
        }
    } else if (profile == 2) {
        for (int s = 0; s < 2; ++s)
            Sheet(layers.veil, -t + s, s * 1.5708f - .4f * std::sin(t), -7, 24, 6.5f, 0xFFFFFF, 145);
        Spiral(layers.energy, -t, .4f, 5, -5, 19, 1, color.edge, hot, camera, 130);
        NeiGi::Glow(layers.energy, camera.forward * 10, 10, color.edge, 70, camera);
        for (int i = 0; i < 6; ++i) {
            const float f = std::fmod((frame % 360u) / 180.f + i / 6.f, 1.f), fade = std::sin(f * 3.14159265f);
            const float a = i * 2.39996f + t, roll = t * (i % 2 ? -2 : 2) + i;
            const Point p{ std::cos(a) * (5 + f * 2), 10 - 16 * f, std::sin(a) * (5 + f * 2) };
            const size_t start = layers.energy.count;
            NeiGi::IceCrystal(layers.energy, p, { std::sin(roll), 1, std::cos(roll) }, 3.1f, .8f, roll,
                              uint8_t(185 * fade * fade), camera);
            for (size_t v = start; v < layers.energy.count; ++v)
                layers.energy.vertices[v].rgb = NeiElementalGi::Mix(color.edge, hot, .65f);
        }
    } else {
        for (int s = 0; s < 2; ++s)
            Sheet(layers.veil, t + s, s * 1.5708f + t, -8, 25, 7, 0xFFFFFF, 135);
        NeiGi::Glow(layers.energy, camera.forward * 10, 10, color.edge, 100, camera);
        NeiGi::Glow(layers.energy, camera.forward * 10, 4, hot, 160, camera);
        for (int ring = 0; ring < 2; ++ring) {
            auto p = [&](int j) {
                const float a = NeiGi::Tau * j / 16 + t * (ring ? -1 : 1);
                return Point{ 9 * std::cos(a), (ring ? 5.f : 2.f) * std::sin(a), (ring ? 3.f : 8.f) * std::sin(a) };
            };
            for (int j = 0; j < 14; ++j)
                NeiGi::Band(layers.energy, p(j), p(j + 1), .6f, color.edge, hot, camera,
                            uint8_t(150 * std::sin((j + .5f) * 3.14159265f / 14)));
        }
        for (int ray = 0; ray < 7; ++ray) {
            const float a = NeiGi::Tau * ray / 7 + t, pulse = .75f + .25f * std::sin(t * 2 + ray);
            const auto dir = NeiGi::Plane(camera, std::cos(a), std::sin(a));
            NeiGi::Band(layers.energy, dir * 3, dir * (12 * pulse), .5f * pulse, color.edge, hot, camera, 100);
        }
    }
    // The accepted veil artwork is monochrome. Vertex color gives it a hot
    // center and elemental edge, including live primary/secondary picker edits.
    for (size_t i = 0; i < layers.veil.count; ++i) {
        auto& v = layers.veil.vertices[i];
        const float heat = std::max(0.f, 1.f - std::abs(v.p.y) / 24.f);
        v.rgb = NeiElementalGi::Mix(color.edge, hot, .25f + .65f * heat);
    }
    for (auto* mesh : { &layers.veil, &layers.energy })
        for (size_t i = 0; i < mesh->count; ++i)
            mesh->vertices[i].p = HeadCenter + mesh->vertices[i].p * (shop ? .75f : 1.f);
    layers.shimmer = NeiGi::SampleShimmer(frame, true, camera, Kind(profile));
    for (size_t i = 0; i < layers.shimmer.count; ++i) {
        auto& v = layers.shimmer.vertices[i];
        v.p = { 4 + v.p.x * .55f, 5 + v.p.y * .64f, v.p.z * .55f };
        if (v.rgb != 0xFFFFFF)
            v.rgb = color.shimmer;
    }
    return layers;
}
} // namespace NeiArrowGi
