#pragma once

#include <array>
#include <cmath>
#include <cstdint>

namespace ComboForestPortalEffect {
constexpr int kRimSegments = 26;
constexpr int kGlints = 6;
constexpr int kVertexCount = kRimSegments * 4 + kGlints * 5;
struct Vertex {
    float x, y;
    uint8_t r, g, b, a;
};
struct Point {
    float x, y;
};

inline Point RimPoint(int index, float inset, bool packed) {
    const float radius = (packed ? 68.0f : 44.0f) - inset;
    const float center = packed ? 66.0f : 65.0f;
    if (index == 0)
        return { -radius, 0.0f };
    if (index == kRimSegments)
        return { radius, 0.0f };
    const float angle = 3.14159265f * (1.0f - (index - 1) / 24.0f);
    return { radius * std::cos(angle), center + radius * std::sin(angle) };
}

// Shared by the native renderer and the asset preview exporter. No random
// state or persistent geometry is mutated; paused gameplay freezes the phase.
inline void Build(uint32_t frame, bool packed, std::array<Vertex, kVertexCount>& vertices) {
    int count = 0;
    auto vertex = [&](Point p, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        vertices[count++] = { p.x, p.y, r, g, b, a };
    };
    for (int i = 0; i < kRimSegments; ++i) {
        const float wave = 0.5f + 0.5f * std::sin(frame * 0.055f - i * 0.55f);
        const uint8_t alpha = static_cast<uint8_t>(85 + wave * 100);
        vertex(RimPoint(i, -4, packed), 96, 32, 176, 0);
        vertex(RimPoint(i, 1, packed), 112, 48, 208, alpha);
        vertex(RimPoint(i + 1, 1, packed), 112, 48, 208, alpha);
        vertex(RimPoint(i + 1, -4, packed), 96, 32, 176, 0);
    }
    for (int i = 0; i < kGlints; ++i) {
        const float phase = std::fmod(frame * 0.035f + i * (kRimSegments / float(kGlints)), float(kRimSegments));
        const int edge = static_cast<int>(phase);
        const float amount = phase - edge;
        const Point a = RimPoint(edge, 0, packed), b = RimPoint(edge + 1, 0, packed);
        const Point p{ a.x + (b.x - a.x) * amount, a.y + (b.y - a.y) * amount };
        const float size = 2.4f;
        vertex(p, 188, 118, 255, 220);
        for (Point offset : { Point{ -size, 0 }, Point{ 0, size }, Point{ size, 0 }, Point{ 0, -size } })
            vertex({ p.x + offset.x, p.y + offset.y }, 128, 64, 224, 0);
    }
}
} // namespace ComboForestPortalEffect
