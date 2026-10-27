#pragma once
#include "NeiElementalArrowGi.h"

namespace NeiSpellGi {
using NeiGi::Basis;
using NeiGi::Mesh;
using NeiGi::Point;
struct Layers {
    Mesh back, veil, energy, front, shimmer;
};
inline Layers Sample(int profile, uint32_t frame, const Basis& camera = {},
                     const NeiElementalGi::Palette* tint = nullptr, bool shop = false) {
    Layers layers;
    if (!NeiArrowGi::Valid(profile))
        return layers;
    const auto color = tint ? *tint : NeiElementalGi::DefaultPalette(profile, true);
    const auto hot = NeiElementalGi::Mix(color.hot, 0xFFFFFF, .22f);
    const float t = (frame % 360u) * (NeiGi::Tau / 360.f);
    if (profile == 1) {
        for (int s = 0; s < 3; ++s)
            NeiArrowGi::Sheet(layers.veil, t + s * 1.7f, s * 1.0472f + .25f * std::sin(t), -23, 49, 13, 0xFFFFFF, 205);
        for (int s = 0; s < 2; ++s)
            NeiArrowGi::Spiral(layers.energy, t * 2.f, s * 3.14159265f, 9.5f, -21, 42, 2.f, color.edge, hot, camera,
                               155);
        NeiGi::Glow(layers.energy, { 0, -5, 0 }, 11, color.edge, 95, camera);
    } else if (profile == 2) {
        for (int s = 0; s < 3; ++s)
            NeiArrowGi::Sheet(layers.veil, -t + s * 2.f, s * 1.0472f + t, -23, 47, 13, 0xFFFFFF, 165);
        for (int s = 0; s < 3; ++s)
            NeiArrowGi::Spiral(layers.energy, t * (s == 1 ? 3.f : -3.f), s * 2.0944f, 13, -22, 44, 1.3f, color.edge,
                               hot, camera, 155);
        NeiGi::Glow(layers.energy, {}, 10, color.edge, 70, camera);
    } else {
        for (int s = 0; s < 2; ++s)
            NeiArrowGi::Sheet(layers.veil, t + s * 1.5f, s * 1.5708f + .4f * std::sin(t), -24, 47, 13, 0xFFFFFF, 170);
        NeiArrowGi::Spiral(layers.energy, t, .5f, 11, -22, 44, 1.3f, color.edge, hot, camera, 115);
        // Fluid blue refraction around a protective barrier.
        for (int ring = 0; ring < 3; ++ring)
            for (int j = 0; j < 16; ++j) {
                auto p = [&](int k) {
                    const float a = NeiGi::Tau * k / 16 + t * (ring % 2 ? -1 : 1);
                    return Point{ (14 - ring) * std::cos(a), (ring - 1) * 13.f + 3 * std::sin(a * 2 + t),
                                  (14 - ring) * std::sin(a) };
                };
                NeiGi::Band(layers.energy, p(j), p(j + 1), .65f, color.edge, hot, camera,
                            uint8_t(100 + 35 * std::sin(t * 2 + ring)));
            }
        NeiGi::Glow(layers.energy, {}, 11, color.edge, 80, camera);
    }
    for (auto* mesh : { &layers.veil, &layers.energy })
        for (size_t i = 0; i < mesh->count; ++i) {
            auto& p = mesh->vertices[i].p;
            const float envelope = (std::abs(p.x) + std::abs(p.z)) / 22 + std::abs(p.y) / 37;
            if (envelope > .88f)
                p = p * (.88f / envelope);
        }
    // NEI casing at native dimensions, with rear glass before the energy
    // and front glass after it. Its 30x50 authoring scale never reaches GI space.
    const Point p[] = { { 0, 36.7f, 0 },  { 21.7f, 0, 0 },  { 0, 0, 21.7f },
                        { -21.7f, 0, 0 }, { 0, 0, -21.7f }, { 0, -36.7f, 0 } };
    const uint32_t glass = NeiElementalGi::Mix(0xBFD4E3, color.hot, .14f);
    for (int j = 0; j < 4; ++j)
        for (int half = 0; half < 2; ++half) {
            const auto a = p[half ? 5 : 0], b = p[1 + (half ? (j + 1) % 4 : j)], c = p[1 + (half ? j : (j + 1) % 4)];
            const auto n = NeiGi::Unit(NeiGi::Cross(c - a, b - a));
            const float facing = n.x * camera.forward.x + n.y * camera.forward.y + n.z * camera.forward.z;
            auto& mesh = facing >= 0 ? layers.front : layers.back;
            const uint8_t alpha = uint8_t(13 + 24 * std::abs(facing));
            mesh.Tri({ a, glass, alpha }, { b, glass, alpha }, { c, glass, alpha });
        }
    const int edges[][2] = { { 0, 1 }, { 0, 2 }, { 0, 3 }, { 0, 4 }, { 5, 1 }, { 5, 2 },
                             { 5, 3 }, { 5, 4 }, { 1, 2 }, { 2, 3 }, { 3, 4 }, { 4, 1 } };
    for (const auto& e : edges)
        NeiGi::Band(layers.front, p[e[0]], p[e[1]], .17f, glass, 0xF3F8FF, camera, 95);
    const auto sheen = NeiGi::SampleCrystalSheen(frame, camera, 21.7f, 36.7f);
    for (size_t i = 0; i + 2 < sheen.count; i += 3)
        layers.front.Tri(sheen.vertices[i], sheen.vertices[i + 1], sheen.vertices[i + 2]);
    const auto kind = profile == 1 ? NeiGi::Kind::Fire : profile == 2 ? NeiGi::Kind::Leaf : NeiGi::Kind::Ice;
    layers.shimmer = NeiGi::SampleShimmer(frame, true, camera, kind);
    for (size_t i = 0; i < layers.shimmer.count; ++i) {
        auto& v = layers.shimmer.vertices[i];
        v.p = { v.p.x * .62f, v.p.y * .83f, v.p.z * .62f };
        if (v.rgb != 0xFFFFFF)
            v.rgb = color.shimmer;
    }
    if (shop)
        for (auto* mesh : { &layers.back, &layers.veil, &layers.energy, &layers.front, &layers.shimmer })
            for (size_t i = 0; i < mesh->count; ++i)
                mesh->vertices[i].p = mesh->vertices[i].p * .82f;
    return layers;
}
} // namespace NeiSpellGi
