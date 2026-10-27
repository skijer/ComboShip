#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
namespace RegionalSnow {
constexpr size_t kCount = 72, kChillCount = 18;
struct Particle {
    float x, y, z, alpha;
};
inline float Opacity(float distance) {
    return std::clamp((650.f - distance) / 300.f, 0.f, 1.f);
}
inline float Smooth(float value) {
    value = std::clamp(value, 0.f, 1.f);
    return value * value * (3.f - 2.f * value);
}
// Fade the whole wisp before any of its vertices reach either clip plane.
// The margin includes its projected half-height (or a flake's radius).
inline float DepthOpacity(float depth, float radius, float nearPlane, float farPlane) {
    const float nearDepth = std::max(24.f, nearPlane + radius + 12.f);
    const float farDepth = std::min(2400.f, farPlane - radius - 12.f);
    return Smooth((depth - nearDepth) / 120.f) * Smooth((farDepth - depth) / 240.f);
}
// Local actor time freezes during pause/cutscene; fixed authored anchors avoid
// camera-following flakes. Continuous downward motion, soft recycle at bounds.
inline Particle Flake(uint32_t tick, size_t index) {
    const float phase = float((index * 73) % 220);
    const float y = 220.f - std::fmod(phase + float(tick % 22000) * 1.15f, 220.f);
    const float sway = float(tick % 3600) * .025f + float(index);
    return { float(int((index * 137) % 401) - 200) + std::sin(sway) * 16.f, y,
             float(int((index * 193) % 401) - 200) + std::cos(sway) * 12.f,
             std::min({ 1.f, y / 24.f, (220.f - y) / 24.f }) };
}
// A low translucent plume drifts west while softly fading at both ends of
// its cycle. It shares actor-local time, never the global weather/effect pool.
inline Particle Chill(uint32_t tick, size_t index) {
    const float phase = float((tick + index * 17) % 120) / 120.f;
    return { float(int(index % 3) - 1) * 22.f - phase * 90.f, 12.f + phase * 18.f,
             float(int(index % 2) * 2 - 1) * 18.f + std::sin(phase * 6.2831853f) * 12.f,
             std::sin(phase * 3.14159265f) };
}
} // namespace RegionalSnow
