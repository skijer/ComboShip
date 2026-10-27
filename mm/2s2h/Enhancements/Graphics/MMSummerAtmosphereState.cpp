#include "MMSummerAtmosphereState.h"

#include <algorithm>
#include <cmath>

namespace MMSummer {
Vec3 Vec3::operator+(Vec3 other) const {
    return { x + other.x, y + other.y, z + other.z };
}
Vec3 Vec3::operator-(Vec3 other) const {
    return { x - other.x, y - other.y, z - other.z };
}
Vec3 Vec3::operator*(float scale) const {
    return { x * scale, y * scale, z * scale };
}
float Dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
float Length(Vec3 value) {
    return std::sqrt(Dot(value, value));
}
Vec3 Normalize(Vec3 value) {
    const float length = Length(value);
    return length > 0.0001f ? value * (1.0f / length) : Vec3{ 0, 1, 0 };
}
static float Smooth(float value) {
    value = std::clamp(value, 0.0f, 1.0f);
    return value * value * (3.0f - 2.0f * value);
}
float NightWeight(float hour) {
    hour = std::fmod(hour + 24.0f, 24.0f);
    if (hour < 5 || hour >= 19)
        return 1;
    if (hour < 7)
        return 1 - Smooth((hour - 5) / 2);
    if (hour < 17)
        return 0;
    return Smooth((hour - 17) / 2);
}
bool InView(Vec3 position, const View& view, float margin) {
    const Vec3 relative = position - view.eye;
    const float depth = Dot(relative, view.forward);
    return depth >= 100 && depth <= 2600 &&
           std::abs(Dot(relative, view.right)) <= depth * view.tanHalfFov * view.aspect * margin &&
           std::abs(Dot(relative, view.up)) <= depth * view.tanHalfFov * margin;
}
float State::Random() {
    random ^= random << 13;
    random ^= random >> 17;
    random ^= random << 5;
    return (random >> 8) / 16777216.0f;
}
void State::Reset() {
    for (auto& p : particles)
        p.alpha = 0;
    beams.clear();
    random = 0x53554D52;
    elapsed = 0;
    initialized = false;
}
void State::Spawn(size_t index, const View& view) {
    auto& p = particles[index];
    p.kind = index < kMoteCount ? Kind::Mote : Kind::Firefly;
    const size_t ordinal = p.kind == Kind::Mote ? index : index - kMoteCount;
    const float x = -0.90f + (ordinal % 8 + 0.2f + Random() * 0.6f) * (1.80f / 8);
    const float y = p.kind == Kind::Mote
                        ? -0.80f + (ordinal / 8 + 0.2f + Random() * 0.6f) * (1.60f / 4)
                        : -0.80f + (ordinal / 8 + 0.2f + Random() * 0.6f) * (1.10f / ((kFireflyCount + 7) / 8));
    // 20% near, 40% middle, 40% far; no circle of effects around Link.
    const size_t tier = ordinal % 10;
    const float depth = tier < 2 ? 200 + Random() * 180 : tier < 6 ? 500 + Random() * 550 : 1200 + Random() * 1100;
    p.anchor = view.eye + view.forward * depth + view.right * (x * depth * view.tanHalfFov * view.aspect) +
               view.up * (y * depth * view.tanHalfFov);
    p.position = p.anchor;
    p.phase = Random() * 6.2831853f;
    p.frequency = 0.6f + Random() * 0.5f;
    p.orbit = p.kind == Kind::Firefly ? 6 + Random() * 16 : 2 + Random() * 4;
    p.drift = view.right * (p.kind == Kind::Mote ? 3 + Random() * 4 : Random() - 0.5f);
    p.drift.y += p.kind == Kind::Mote ? 1 + Random() * 2 : 0;
    // Keep daytime tufts unchanged; broaden the night glow in the existing
    // textured quad, with a fixed radius cap and no additional draw pass.
    p.radius =
        std::clamp(depth * (p.kind == Kind::Mote ? 0.0045f : 0.0036f), 0.45f, p.kind == Kind::Mote ? 10.0f : 5.8f);
    p.age = p.alpha = 0;
    ++p.generation;
}
void State::BuildBeams(const Input& input, const View& view, float daylight) {
    beams.clear();
    const auto sun = Normalize(input.sunDirection);
    if (!input.sunbeams || daylight <= 0.01f || sun.y <= 0.15f)
        return;
    // Three softly feathered, depth-tested strips. No framebuffer distortion,
    // screen tint, scene material edits, or dedicated actor slots.
    for (size_t index = 0; index < 3; ++index) {
        Beam beam;
        const float depth = 700.0f + index * 520.0f;
        const float halfHeight = depth * view.tanHalfFov;
        const Vec3 top = view.eye + view.forward * depth + view.up * (halfHeight * 1.05f) +
                         view.right * ((-0.58f + index * 0.60f) * halfHeight * view.aspect);
        const float length = halfHeight * 1.95f / sun.y;
        const float opacity = 0.10f * daylight * Smooth((sun.y - 0.15f) / 0.45f) *
                              (0.86f + 0.14f * std::sin(elapsed * 0.24f + index * 1.4f));
        constexpr float rows[] = { 0, 0.22f, 0.75f, 1 };
        for (size_t row = 0; row < 4; ++row) {
            const float width = halfHeight * (0.035f + 0.12f * rows[row]);
            const Vec3 center = top - sun * (length * rows[row]);
            for (size_t column = 0; column < 3; ++column) {
                beam.vertices[row * 3 + column] = { center + view.right * ((static_cast<float>(column) - 1) * width),
                                                    row > 0 && row < 3 && column == 1 ? opacity : 0 };
            }
        }
        beams.push_back(beam);
    }
}
void State::Step(const Input& input, const View& view) {
    if (!input.eligible) {
        Reset();
        return;
    }
    const float dt = input.paused ? 0 : std::clamp(input.seconds, 0.0f, 0.1f);
    elapsed += dt;
    const float night = NightWeight(input.hour);
    const float daylight = (1 - night) * std::clamp(input.dayVisibility, 0.0f, 1.0f);
    for (size_t i = 0; i < particles.size(); ++i) {
        auto& p = particles[i];
        if (!initialized) {
            Spawn(i, view);
        } else if (dt > 0) {
            p.age += dt;
            const float angle = p.age * p.frequency + p.phase;
            const Vec3 hover = { p.orbit * (std::sin(angle) - std::sin(p.phase)),
                                 p.orbit * 0.45f * (std::sin(angle * 0.7f) - std::sin(p.phase * 0.7f)),
                                 p.orbit * 0.6f * (std::cos(angle * 0.8f) - std::cos(p.phase * 0.8f)) };
            p.position = p.anchor + p.drift * p.age + hover;
            if (!InView(p.position, view, 1.10f))
                Spawn(i, view);
        }
        const Vec3 relative = p.position - view.eye;
        const float depth = std::max(100.0f, Dot(relative, view.forward));
        const float x = std::abs(Dot(relative, view.right)) / (depth * view.tanHalfFov * view.aspect);
        const float y = std::abs(Dot(relative, view.up)) / (depth * view.tanHalfFov);
        const float edge = Smooth((1.08f - std::max(x, y)) / 0.18f);
        const float pulse = 0.5f + 0.5f * std::sin(p.age * p.frequency + p.phase);
        // Lift the dim pulse and let its core reach full opacity at night.
        // Entry, edge and day/night fades still govern the existing soft glow.
        p.alpha = Smooth(p.age / 0.8f) * edge *
                  (p.kind == Kind::Mote ? daylight * (0.58f + 0.22f * pulse) : night * (0.48f + 0.52f * pulse));
    }
    initialized = true;
    BuildBeams(input, view, daylight);
}
} // namespace MMSummer
