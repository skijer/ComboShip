#pragma once

#include <array>
#include <cmath>
#include <cstdint>

namespace WoodsMysteryHexEffect {
constexpr int kHexes = 36;
constexpr int kVertexCount = kHexes * 12;
struct Vertex {
    float x, y, z;
    uint8_t r, g, b, a;
};

// Stateless geometry follows gameplay time, so pausing never advances the effect.
// Each hex is a hollow ring: six outer and six inner vertices, with no filled center.
inline void Build(uint32_t frame, std::array<Vertex, kVertexCount>& out) {
    constexpr float tau = 6.28318530718f;
    // Accepted wood's outer edge: across, height, depth from the mouth anchor.
    // The feet recede into the trunk; following that curve leaves the actual
    // doorway clear and keeps the shimmer attached to the diagonal entrance.
    constexpr float rim[][3] = { { -50.205f, 1, 57.276f },
                                 { -61.518f, 15, 57.276f },
                                 { -66.468f, 48, 45.255f },
                                 { -60.811f, 80, 32.527f },
                                 { -46.669f, 102, 16.971f },
                                 { -28.284f, 120, 4.243f },
                                 { 0, 128, 0 },
                                 { 29.698f, 120, 4.243f },
                                 { 50.912f, 104, 16.971f },
                                 { 60.811f, 80, 32.527f },
                                 { 66.468f, 48, 45.255f },
                                 { 61.518f, 15, 57.276f },
                                 { 50.205f, 1, 57.276f } };
    for (int h = 0; h < kHexes; ++h) {
        const float phase = ((frame % 192u + h * 37u) % 192u) / 192.0f;
        const float seed = std::fmod(h * 0.61803398875f, 1.0f);
        const float drift = std::sin((frame % 1024u) * tau / 1024.0f + h * .83f) * .22f;
        const float position = (h + .5f + drift) * 12.0f / kHexes;
        const int segment = static_cast<int>(position);
        const float t = position - segment;
        const float x = (rim[segment][0] + (rim[segment + 1][0] - rim[segment][0]) * t) * 1.05f;
        const float y = rim[segment][1] + (rim[segment + 1][1] - rim[segment][1]) * t + 2.0f +
                        std::sin(phase * 3.14159265359f) * 2.0f;
        const float depth = rim[segment][2] + (rim[segment + 1][2] - rim[segment][2]) * t;
        const float z = -depth + 8.0f + std::sin(h * 1.71f + (frame % 512u) * tau / 512.0f) * 2.0f;
        const float radius = h % 9 == 0 ? 4.5f : 1.8f + seed * 2.3f;
        const float fade = std::sin(phase * 3.14159265359f);
        const auto alpha = static_cast<uint8_t>(fade * 210.0f);
        for (int ring = 0; ring < 2; ++ring) {
            for (int side = 0; side < 6; ++side) {
                const float a = side * tau / 6 + h * 0.3f + (frame % 768u) * tau / 768.0f;
                const float size = radius * (ring ? 0.62f : 1.0f);
                out[h * 12 + ring * 6 + side] = {
                    x + std::cos(a) * size, y + std::sin(a) * size, z, 68, 255, 116, alpha
                };
            }
        }
    }
}
} // namespace WoodsMysteryHexEffect
