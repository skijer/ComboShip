#include "SeasonWinterState.h"
#include <algorithm>
#include <cmath>

namespace SoHSeasonWinter {
static float Smooth(float value) {
    value = std::clamp(value, 0.0f, 1.0f);
    return value * value * (3 - 2 * value);
}
float NightWeight(float hour) {
    hour = std::fmod(std::fmod(hour, 24.0f) + 24.0f, 24.0f);
    if (hour < 4 || hour >= 20)
        return 1;
    if (hour < 6)
        return 1 - Smooth((hour - 4) / 2);
    if (hour <= 18)
        return 0;
    return Smooth((hour - 18) / 2);
}
float State::Random() {
    random ^= random << 13;
    random ^= random >> 17;
    random ^= random << 5;
    return (random >> 8) / 16777216.0f;
}
void State::Reset() {
    for (auto& wisp : wisps) {
        const auto generation = wisp.generation;
        wisp = {};
        wisp.generation = generation;
    }
    ribbons = {};
    random = 0x57494E54;
    elapsed = 0;
    initialized = false;
}
void State::Spawn(size_t index, const Input& input, const View& view) {
    auto& wisp = wisps[index];
    Vec3 forward = { view.forward.x, 0, view.forward.z };
    if (SoHSeasonParticles::Length(forward) < 0.001f)
        forward = { 0, 0, 1 };
    forward = SoHSeasonParticles::Normalize(forward);
    const Vec3 right = { -forward.z, 0, forward.x };
    const float depth = 320 + (index / 4 + Random() * 0.65f) * 430;
    const float lateral =
        (-0.95f + (index % 4 + 0.15f + Random() * 0.7f) * 0.475f) * depth * view.tanHalfFov * view.aspect;
    wisp.anchor = { view.eye.x, input.origin.y, view.eye.z };
    wisp.anchor = wisp.anchor + forward * depth + right * lateral;
    wisp.position = wisp.anchor;
    wisp.width = 180 + Random() * 190;
    wisp.height = 10 + Random() * 10;
    wisp.phase = Random() * 6.2831853f;
    wisp.age = wisp.alpha = 0;
    ++wisp.generation;
}
void State::BuildAurora(const Input& input, const View& view) {
    for (auto& ribbon : ribbons)
        for (auto& vertex : ribbon.vertices)
            vertex.alpha = 0;
    const float night = NightWeight(input.hour);
    if (!input.aurora || !input.skyVisible || night <= 0 || input.clipFar <= 200)
        return;
    const float radius = std::min(input.clipFar * 0.62f, 7000.0f);
    const float entry = Smooth(elapsed / 1.2f);
    // Fixed compass arcs, translated like the native skybox. Camera yaw does
    // not rotate these ribbons; mountains/buildings occlude them via depth.
    for (size_t band = 0; band < ribbons.size(); ++band) {
        for (size_t column = 0; column < kRibbonColumns; ++column) {
            const float u = column / float(kRibbonColumns - 1);
            const float angle = band * 2.0943951f - 1.20f + u * 2.40f;
            const float wave = std::sin(angle * 2.1f + elapsed * 0.12f + band);
            // Native third-person cameras look down by 10-15 degrees with a
            // 60-degree FOV. Keep the colored core in their visible sky;
            // the former 16-25 degree elevation clipped it above the frame.
            const float center = radius * (0.18f + wave * 0.045f);
            const float height = radius * (0.12f + 0.04f * std::sin(u * 4.5f + elapsed * 0.08f + band));
            const float hue = 0.5f + 0.5f * std::sin(angle * 2 + band + elapsed * 0.025f);
            const float edge = std::sin(u * 3.14159265f);
            for (size_t row = 0; row < 3; ++row) {
                auto& vertex = ribbons[band].vertices[column * 3 + row];
                vertex.position = view.eye + Vec3{ radius * std::sin(angle), center + (float(row) - 1) * height,
                                                   radius * std::cos(angle) };
                vertex.red = static_cast<uint8_t>(70 + 45 * hue);
                vertex.green = static_cast<uint8_t>(155 + 90 * hue);
                vertex.blue = static_cast<uint8_t>(245 - 55 * hue);
                vertex.alpha = row == 1 ? 0.44f * night * entry * std::max(edge, 0.0f) *
                                              (0.82f + 0.18f * std::sin(elapsed * 0.13f + u * 5 + band))
                                        : 0;
            }
        }
    }
}
void State::Step(const Input& input, const View& view) {
    if (!input.eligible) {
        Reset();
        return;
    }
    const float dt = input.paused ? 0 : std::clamp(input.seconds, 0.0f, 0.1f);
    elapsed += dt;
    for (size_t i = 0; i < wisps.size(); ++i) {
        auto& wisp = wisps[i];
        if (!initialized)
            Spawn(i, input, view);
        else if (dt > 0) {
            wisp.age += dt;
            const float phase = wisp.phase + wisp.age * 0.22f;
            wisp.position = wisp.anchor + Vec3{ 8 * wisp.age + 18 * (std::sin(phase) - std::sin(wisp.phase)), 0,
                                                12 * (std::cos(phase) - std::cos(wisp.phase)) };
            const auto projected = SoHSeasonParticles::Project(wisp.position, view);
            if (projected.depth < 120 || projected.depth > 2400 || std::abs(projected.x) > 1.65f)
                Spawn(i, input, view);
        }
        wisp.alpha = input.mist
                         ? 0.24f * Smooth(wisp.age / 0.8f) * (0.82f + 0.18f * std::sin(wisp.phase + wisp.age * 0.28f))
                         : 0;
    }
    initialized = true;
    BuildAurora(input, view);
}
} // namespace SoHSeasonWinter
