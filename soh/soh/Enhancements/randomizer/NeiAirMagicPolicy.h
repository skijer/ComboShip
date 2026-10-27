#pragma once

#include "NeiUsedMagicPolicy.h"
#include <cstring>

// Item-local presentation. These meshes are also the preview's only effect
// geometry: no gameplay RNG, particles, scene assets or simulation changes.
namespace NeiAirMagic {
using NeiGi::Basis;
using NeiGi::Mesh;
using NeiGi::Point;
using NeiGi::Tau;
using NeiUsedMagic::Alpha;
struct Layers {
    Mesh surface, detail;
};
// Native builds use fast-math, which can optimize std::isfinite away.
inline bool Finite(float value) {
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    return (bits & 0x7F800000u) != 0x7F800000u;
}
inline bool Finite(Point p) {
    return Finite(p.x) && Finite(p.y) && Finite(p.z);
}
inline float Clamp(float value, float low, float high) {
    return Finite(value) ? std::clamp(value, low, high) : low;
}
inline float Wrap(float x) {
    return x - std::floor(x);
}
inline float Phase(unsigned frame) {
    return float(frame % 180u);
}
inline Point Side(Point tangent, const Basis& camera) {
    Point side = NeiGi::Cross(tangent, camera.forward);
    if (side.x * side.x + side.y * side.y + side.z * side.z < .001f)
        side = camera.right;
    return NeiGi::Unit(side);
}
inline void Surface(Mesh& m, Point a, Point b, Point sideA, Point sideB, uint32_t rgb, uint8_t alphaA, uint8_t alphaB,
                    float v0, float v1, float scroll = 0) {
    const NeiGi::EffectVertex al{ a - sideA, rgb, alphaA, scroll, v0 }, ar{ a + sideA, rgb, alphaA, 1 + scroll, v0 };
    const NeiGi::EffectVertex bl{ b - sideB, rgb, alphaB, scroll, v1 }, br{ b + sideB, rgb, alphaB, 1 + scroll, v1 };
    m.Tri(al, bl, br);
    m.Tri(al, br, ar);
}
inline void Filament(Mesh& m, Point a, Point b, float width, uint32_t color, uint8_t alphaA, uint8_t alphaB,
                     const Basis& camera) {
    const Point side = Side(b - a, camera) * width;
    NeiUsedMagic::Ribbon(m, a, b, side, side, color, alphaA, alphaB);
}
inline Layers SampleLightning(unsigned frame, Point direction, const Basis& camera = {}) {
    Layers out;
    if (!Finite(direction))
        return out;
    const Point axis = NeiGi::Unit(direction), side = Side(axis, camera), across = NeiGi::Cross(axis, side);
    const float tick = Phase(frame), crackle = std::floor(tick / 3.f);
    Point p[13];
    for (int i = 0; i < 13; ++i) {
        const float f = i / 12.f, bend = std::sin(f * Tau * .5f);
        p[i] = axis * (-66 + 80 * f) + side * (std::sin(i * 2.37f + crackle * 1.71f) * 6 * bend) +
               across * (std::cos(i * 3.17f + crackle * .83f) * 3 * bend);
    }
    for (int i = 0; i < 12; ++i) {
        const float f0 = i / 12.f, f1 = (i + 1) / 12.f;
        const Point strip = Side(p[i + 1] - p[i], camera);
        Surface(out.surface, p[i], p[i + 1], strip * 3.2f, strip * 3.2f, 0xFFF0C4, Alpha(200 * f0), Alpha(200 * f1),
                1 - f0, 1 - f1, Wrap(tick * 3 / 180.f));
        NeiGi::Band(out.detail, p[i], p[i + 1], 2.2f, 0xFFCF73, 0xFFFFF1, camera, Alpha(225 * (.3f + .7f * f1)));
    }
    for (int branch = 0; branch < 4; ++branch) {
        const int root = 3 + branch * 2;
        const float sign = (branch % 2) ? -1.f : 1.f;
        Point start = p[root];
        for (int j = 0; j < 3; ++j) {
            const Point end =
                start + axis * (3.f + j) + side * (sign * (5.f - j)) + across * (std::sin(crackle + branch + j) * 2);
            const uint8_t alpha = Alpha(170 * (1 - j / 3.f));
            NeiGi::Band(out.detail, start, end, 1.15f, 0xFFD889, 0xFFFFF4, camera, alpha);
            const Point strip = Side(end - start, camera) * 2;
            Surface(out.surface, start, end, strip, strip, 0xFFF4D0, alpha, Alpha(alpha * .6f), .25f, .85f);
            start = end;
        }
    }
    NeiGi::Glow(out.detail, p[11], 8, 0xFFD889, 75, camera);
    NeiGi::Glow(out.detail, p[12], 3.4f, 0xFFFFED, 150, camera);
    return out;
}
inline Layers SampleGust(unsigned frame, Point direction, float length, float radius, bool blow, uint32_t color,
                         const Basis& camera = {}) {
    Layers out;
    if (!Finite(direction) || !Finite(length) || !Finite(radius) || length <= 0 || radius <= 0)
        return out;
    length = Clamp(length, 0, 500);
    radius = Clamp(radius, 0, 140);
    const Point axis = NeiGi::Unit(direction);
    const Point side = NeiGi::Unit(NeiGi::Cross(axis, std::abs(axis.y) < .9f ? Point{ 0, 1, 0 } : Point{ 1, 0, 0 }));
    const Point across = NeiGi::Cross(axis, side);
    const float tick = Phase(frame), travel = tick / (blow ? 45.f : 60.f);
    constexpr int segments = 22;
    for (int strand = 0; strand < 4; ++strand) {
        const float start = Wrap(travel + strand * .25f);
        auto center = [&](float g) {
            const float f = blow ? g : 1 - g;
            const float angle = strand * Tau / 4 + f * Tau * 1.7f + (blow ? 1 : -1) * tick * Tau / 90.f;
            const float r = radius * (.06f + .94f * std::pow(f, 1.4f));
            return axis * (length * f) + (side * std::cos(angle) + across * std::sin(angle)) * r;
        };
        for (int j = 0; j < segments; ++j) {
            const float q0 = j / float(segments), q1 = (j + 1) / float(segments);
            const float g0 = start + q0 * .53f, g1 = start + q1 * .53f;
            if (g1 > 1)
                break;
            const Point a = center(g0), b = center(g1), strip = Side(b - a, camera);
            const float fade0 = std::sin(q0 * Tau * .5f) * std::sin(g0 * Tau * .5f);
            const float fade1 = std::sin(q1 * Tau * .5f) * std::sin(g1 * Tau * .5f);
            const float width = (2 + radius * .052f) * (blow ? 1 : .85f);
            Surface(out.surface, a, b, strip * width, strip * width, color, Alpha(215 * fade0), Alpha(215 * fade1),
                    1 - q0, 1 - q1, Wrap(tick * 2 / 180.f + strand * .13f));
            Filament(out.detail, a, b, .46f, color, Alpha(120 * fade0), Alpha(120 * fade1), camera);
        }
    }
    return out;
}
inline Layers SampleEnvelope(unsigned frame, float height, Point motion, const Basis& camera = {}) {
    Layers out;
    if (!Finite(height) || height <= 0 || !Finite(motion))
        return out;
    height = Clamp(height, 20, 120);
    const float radius = Clamp(height * .29f, 12, 28), tick = Phase(frame);
    const Point drift{ Clamp(-motion.x * 1.8f, -18, 18), 0, Clamp(-motion.z * 1.8f, -18, 18) };
    constexpr int segments = 24;
    for (int strand = 0; strand < 4; ++strand) {
        const float start = Wrap(tick / 90.f + strand * .25f);
        auto center = [&](float f) {
            const float angle = strand * Tau / 4 + f * Tau * 1.55f - tick * Tau / 90.f;
            const float r = radius * (.68f + .32f * std::sin(f * Tau * .5f));
            return NeiUsedMagic::Radial(angle, r, 3 + height * .88f * f) + drift * (f * f);
        };
        for (int j = 0; j < segments; ++j) {
            const float q0 = j / float(segments), q1 = (j + 1) / float(segments);
            const float f0 = start + q0 * .62f, f1 = start + q1 * .62f;
            if (f1 > 1)
                break;
            const Point a = center(f0), b = center(f1), strip = Side(b - a, camera);
            const float fade0 = std::sin(q0 * Tau * .5f) * std::sin(f0 * Tau * .5f);
            const float fade1 = std::sin(q1 * Tau * .5f) * std::sin(f1 * Tau * .5f);
            const float width = height * .057f;
            Surface(out.surface, a, b, strip * width, strip * width, strand % 2 ? 0xDDE5EF : 0xF1F5F9,
                    Alpha(190 * fade0), Alpha(190 * fade1), 1 - q0, 1 - q1, Wrap(tick * 2 / 180.f + strand * .2f));
            Filament(out.detail, a, b, .35f, 0xF4F7FC, Alpha(92 * fade0), Alpha(92 * fade1), camera);
        }
    }
    return out;
}
} // namespace NeiAirMagic
