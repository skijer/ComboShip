#pragma once
#include "BlackMarketFrame.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace BlackMarketVisual {
inline constexpr char kSealsCvar[] = "gCombo.BlackMarket.AnimatedItemSeals";
constexpr int kLockTextureWidth = 32, kLockTextureHeight = 8;
constexpr int kLockSegments = 8, kLockLoops = 2, kLockQuads = kLockSegments * kLockLoops;
constexpr int kLockVertices = kLockQuads * 4;
constexpr float kLockVertexScale = 64.f;

// IA8 strip with transparent edges and sparse glyphs. S wraps; T clamps.
constexpr auto MakeLockTexture() {
    std::array<uint8_t, kLockTextureWidth * kLockTextureHeight> pixels{};
    for (int t = 0; t < kLockTextureHeight; ++t)
        for (int s = 0; s < kLockTextureWidth; ++s) {
            const bool edge = t == 0 || t == kLockTextureHeight - 1;
            const int rune = s % 16;
            const bool glyph = (rune == 2 && t >= 2 && t <= 5) || (rune >= 2 && rune <= 6 && (t == 2 || t == 5)) ||
                               (rune == 6 && t == 3) || (rune == 10 && t >= 2 && t <= 5) || (rune == 12 && t == 3) ||
                               (rune == 13 && t == 4);
            const int intensity = glyph ? 15 : t == 1 || t == 6 ? 12 : 7;
            const int alpha = edge ? 0 : glyph ? 15 : t == 1 || t == 6 ? 9 : 4;
            pixels[t * kLockTextureWidth + s] = uint8_t(intensity * 16 + alpha);
        }
    return pixels;
}
alignas(8) inline constexpr auto kLockTexture = MakeLockTexture();
struct LockVertex {
    int16_t x, y, z, s, t;
};
struct LockMesh {
    Point center{};
    std::array<LockVertex, kLockVertices> vertices{};
    std::array<uint8_t, 4> color{};
    bool valid = false;
};

// Padding changes the clearance and ribbon thickness, preserving the captured
// model's size. Ordinary callers retain the original fit at paddingScale=1.
inline LockMesh BuildLock(const Frame& frame, uint32_t tick, bool selected, float paddingScale = 1.f) {
    LockMesh mesh;
    if (!frame.valid || !Finite(paddingScale) || paddingScale <= 0)
        return mesh;
    const float low[] = { frame.minimum.x, frame.minimum.y, frame.minimum.z };
    const float high[] = { frame.maximum.x, frame.maximum.y, frame.maximum.z };
    float half[3]{};
    for (int axis = 0; axis < 3; ++axis) {
        if (!Finite(low[axis]) || !Finite(high[axis]) || high[axis] < low[axis])
            return mesh;
        half[axis] = (high[axis] - low[axis]) * .5f;
        if (!Finite(half[axis]) || half[axis] > 300.f)
            return mesh;
    }
    mesh.center = { low[0] + half[0], low[1] + half[1], low[2] + half[2] };
    if (!Finite(mesh.center))
        return mesh;
    mesh.color = selected ? std::array<uint8_t, 4>{ 220, 170, 255, 165 } : std::array<uint8_t, 4>{ 185, 120, 250, 140 };
    const float rx = (std::max(half[0], 2.4f * paddingScale) + 1.4f * paddingScale) / .875f;
    const float ry = (std::max(half[1], 3.2f * paddingScale) + 1.4f * paddingScale) / .875f;
    const float rz = (std::max(half[2], 2.4f * paddingScale) + 1.4f * paddingScale) / .875f;
    const float width = std::clamp(std::min(half[0], half[1]) * .07f, .5f * paddingScale, .85f * paddingScale);
    const float length = std::hypot(rx, ry);
    constexpr float corners[kLockSegments][2] = { { 1, .75f },   { .75f, 1 },   { -.75f, 1 }, { -1, .75f },
                                                  { -1, -.75f }, { -.75f, -1 }, { .75f, -1 }, { 1, -.75f } };
    for (int loop = 0; loop < kLockLoops; ++loop) {
        const float sign = loop ? -1.f : 1.f;
        const float wx = width * ry / length, wy = -sign * width * rx / length;
        const int phase = int((tick * 8u + uint32_t(loop * 256)) & 1023u);
        for (int segment = 0; segment < kLockSegments; ++segment)
            for (int corner = 0; corner < 4; ++corner) {
                const int step = segment + (corner >= 2 ? 1 : 0);
                const auto& point = corners[step % kLockSegments];
                const float side = corner == 0 || corner == 3 ? -1.f : 1.f;
                const float x = sign * rx * point[0] + side * wx;
                const float y = ry * point[0] + side * wy;
                const float z = (rz + loop * .45f * paddingScale) * point[1];
                mesh.vertices[(loop * kLockSegments + segment) * 4 + corner] = {
                    int16_t(std::lround(x * kLockVertexScale)), int16_t(std::lround(y * kLockVertexScale)),
                    int16_t(std::lround(z * kLockVertexScale)), int16_t(step * 512 + (loop ? -phase : phase)),
                    int16_t(side < 0 ? 0 : (kLockTextureHeight - 1) * 32)
                };
            }
    }
    mesh.valid = true;
    return mesh;
}
} // namespace BlackMarketVisual
