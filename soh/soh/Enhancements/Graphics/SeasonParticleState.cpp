#include "SeasonParticleState.h"

#include <algorithm>
#include <cmath>

namespace SoHSeasonParticles {
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
    return length > 0.0001f ? value * (1.0f / length) : Vec3{ 0, 0, 1 };
}
Projection Project(Vec3 position, const View& view) {
    const Vec3 relative = position - view.eye;
    const float depth = Dot(relative, view.forward);
    if (depth <= 0.001f)
        return { 2, 2, depth };
    return { Dot(relative, view.right) / (depth * view.tanHalfFov * view.aspect),
             Dot(relative, view.up) / (depth * view.tanHalfFov), depth };
}
float State::Random() {
    random ^= random << 13;
    random ^= random >> 17;
    random ^= random << 5;
    return (random >> 8) / 16777216.0f;
}
void State::Reset() {
    // Epochs remain monotonic across scene/room/reentry and save-state resets:
    // interpolation must never join a new field to an old field at this address.
    for (auto& particle : particles) {
        const uint32_t generation = particle.generation + 1;
        particle = {};
        particle.generation = generation;
    }
    play = nullptr;
    scene = room = season = -1;
    random = 0x53454153;
    lastFrame = motionFrames = 0;
    count = 0;
    initialized = false;
}
void State::Spawn(size_t index, const Input& input, const View& view) {
    auto& particle = particles[index];
    if (input.season == 3) {
        // Match SoH's native snow camera volume and fall speed, using private
        // slots and RNG. No native actor or precipitation count is touched.
        const Vec3 center = view.eye + view.forward * 80.0f;
        particle.position = center + Vec3{ (Random() - 0.5f) * 160.0f, 80.0f, (Random() - 0.5f) * 160.0f };
        particle.speed = Random() * 5.0f + 0.5f;
        particle.phaseX = Random() * 3.6f;
        particle.phaseZ = Random() * 3.6f;
        particle.band = 0;
    } else {
        // ComboShip980e6fe09 native falling-season distribution:16 near,
        // 32 middle,48 far, with a permuted jittered12x8 view layout.
        constexpr float minimum[] = { 700, 1200, 3200 };
        constexpr float maximum[] = { 1000, 2800, 6000 };
        particle.band = index < 16 ? 0 : index < 48 ? 1 : 2;
        const size_t cell = (index * 37) % kFallingCount;
        const float x = (((cell % 12 + 0.15f + Random() * 0.7f) / 12.0f) * 2 - 1) * 0.85f;
        const float y = (((cell / 12 + 0.15f + Random() * 0.7f) / 8.0f) * 2 - 1) * 0.85f;
        float radius = minimum[particle.band] + Random() * (maximum[particle.band] - minimum[particle.band]);
        if (particle.band != 0 && input.compact)
            radius = std::max(radius * 0.3f, 600.0f);
        const Vec3 ray =
            view.forward + view.right * (x * view.tanHalfFov * view.aspect) + view.up * (y * view.tanHalfFov);
        particle.position = view.eye + Normalize(ray) * radius;
    }
    particle.age = 1;
    particle.rotation = static_cast<float>(index * 1300);
    ++particle.generation;
}
void State::Tick(const Input& input, const View& view) {
    const bool falling = input.season == 0 || input.season == 2;
    if (!input.play || !input.eligible || (!falling && input.season != 3) || (input.season == 3 && input.nativeSnow)) {
        if (initialized)
            Reset();
        return;
    }
    if (play != input.play || scene != input.scene || room != input.room || season != input.season ||
        (initialized && input.frame < lastFrame)) {
        Reset();
        play = input.play;
        scene = input.scene;
        room = input.room;
        season = input.season;
    }
    count = falling ? kFallingCount : kSnowCount;
    if (!initialized) {
        for (size_t i = 0; i < count; ++i)
            Spawn(i, input, view);
        initialized = true;
        lastFrame = input.frame;
        return;
    }
    const bool advance = !input.paused && lastFrame != input.frame;
    lastFrame = input.frame;
    if (!advance)
        return;
    ++motionFrames;
    const float windLength = std::hypot(input.wind.x, input.wind.z);
    const float wind = std::clamp(input.windSpeed / 120.0f, 0.0f, 1.0f);
    for (size_t i = 0; i < count; ++i) {
        auto& particle = particles[i];
        if (falling) {
            const float phase = motionFrames * 0.055f + i * 2.4f;
            const float flutter = 0.9f + particle.band * 0.45f;
            particle.position.x += std::sin(phase) * flutter;
            particle.position.z += std::cos(phase * 0.7f) * flutter;
            if (windLength > 0.001f) {
                particle.position.x -= input.wind.x / windLength * wind * (3 + particle.band);
                particle.position.z -= input.wind.z / windLength * wind * (3 + particle.band);
            }
            particle.position.y -= 2.6f + particle.band * 0.8f + (i & 7) * 0.24f;
            particle.age = std::min(particle.age + 1, 24.0f);
            particle.rotation += 180;
            const auto screen = Project(particle.position, view);
            if (screen.depth <= 0 || std::abs(screen.x) >= 1 || std::abs(screen.y) >= 1 ||
                Length(particle.position - view.eye) >= 8000)
                Spawn(i, input, view);
        } else {
            particle.phaseX += 0.0005f * Random();
            particle.phaseZ += 0.0005f * Random();
            particle.position.x += std::sin(particle.phaseX);
            particle.position.z += std::cos(particle.phaseZ);
            particle.position.y -= particle.speed;
            if (input.flurries) {
                const float phase = motionFrames * 0.055f + i * 2.4f;
                particle.position.x += std::sin(phase) * 1.6f;
                particle.position.z += std::cos(phase * 0.7f) * 1.6f;
            }
            const Vec3 relative = particle.position - (view.eye + view.forward * 80.0f);
            if (relative.y < -80 || std::abs(relative.x) > 80 || std::abs(relative.z) > 80)
                Spawn(i, input, view);
        }
    }
}
} // namespace SoHSeasonParticles
