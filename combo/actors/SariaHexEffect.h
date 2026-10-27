#pragma once

#include <array>
#include <cmath>
#include <cstdint>

namespace SariaHexEffect {
constexpr int kHexes = 12;
constexpr int kVertexCount = kHexes * 12;
struct Vertex {
    float x, y, z;
    uint8_t r, g, b, a;
};

// Local world units around the authored actor, independent of her model scale.
// Six outer and six inner vertices form each camera-facing hollow hex ring.
inline void Build(uint32_t frame, bool seated, float facing, std::array<Vertex, kVertexCount>& out) {
    constexpr float tau = 6.28318530718f;
    const float rightX = std::cos(facing), rightZ = -std::sin(facing);
    for (int h = 0; h < kHexes; ++h) {
        const float phase = ((frame % 144u + h * 37u) % 144u) / 144.0f;
        const float seed = std::fmod(h * 0.61803398875f, 1.0f);
        const float angle = h * 2.39996323f + (frame % 2048u) * tau / 2048.0f;
        const float x = std::cos(angle) * (16.0f + seed * 8.0f);
        const float z = std::sin(angle) * (16.0f + seed * 8.0f);
        const float y = 4.0f + phase * (seated ? 20.0f : 44.0f);
        const float radius = 2.0f + seed * 2.0f;
        const auto alpha = static_cast<uint8_t>(std::sin(phase * tau * 0.5f) * 210.0f);
        for (int ring = 0; ring < 2; ++ring) {
            for (int side = 0; side < 6; ++side) {
                const float a = side * tau / 6.0f + h * 0.3f + (frame % 768u) * tau / 768.0f;
                const float size = radius * (ring ? 0.62f : 1.0f);
                const float horizontal = std::cos(a) * size;
                out[h * 12 + ring * 6 + side] = {
                    x + rightX * horizontal, y + std::sin(a) * size, z + rightZ * horizontal, 68, 255, 116, alpha
                };
            }
        }
    }
}
} // namespace SariaHexEffect
