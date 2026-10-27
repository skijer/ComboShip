#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

// Cosmetic state only: independent of actors, environment precipitation,
// gameplay randomness, and save data. The renderer consumes these positions.
namespace SoHSeasonParticles {
struct Vec3 {
    float x = 0, y = 0, z = 0;
    Vec3 operator+(Vec3 other) const;
    Vec3 operator-(Vec3 other) const;
    Vec3 operator*(float scale) const;
};
float Dot(Vec3 a, Vec3 b);
float Length(Vec3 value);
Vec3 Normalize(Vec3 value);
struct View {
    Vec3 eye, forward, right, up;
    float tanHalfFov = 0.5773503f;
    float aspect = 4.0f / 3.0f;
};
struct Projection {
    float x, y, depth;
};
Projection Project(Vec3 position, const View& view);
struct Input {
    const void* play = nullptr;
    int scene = -1, room = -1, season = -1;
    uint32_t frame = 0;
    bool eligible = false, paused = false, compact = false, flurries = false, nativeSnow = false;
    Vec3 wind;
    float windSpeed = 0;
};
constexpr size_t kFallingCount = 96, kSnowCount = 64;
struct Particle {
    Vec3 position;
    float age = 0, rotation = 0, speed = 0, phaseX = 0, phaseZ = 0;
    uint32_t generation = 0;
    uint8_t band = 0;
};
class State {
  public:
    void Reset();
    void Tick(const Input& input, const View& view);
    size_t Count() const {
        return count;
    }
    const std::array<Particle, kFallingCount>& Particles() const {
        return particles;
    }

  private:
    float Random();
    void Spawn(size_t index, const Input& input, const View& view);
    std::array<Particle, kFallingCount> particles{};
    const void* play = nullptr;
    int scene = -1, room = -1, season = -1;
    uint32_t random = 0x53454153, lastFrame = 0, motionFrames = 0;
    size_t count = 0;
    bool initialized = false;
};
} // namespace SoHSeasonParticles
