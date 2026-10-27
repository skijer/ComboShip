// Ported from ComboShip980e6fe09 native summer atmosphere.
#pragma once

#include <array>
#include <cstdint>
#include <vector>

// Asset-free motion shared by the game, regression tests, and preview exporter.
namespace SoHSeasonSummer {
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
struct Input {
    bool eligible = false;
    bool paused = false;
    float seconds = 0;
    float hour = 12;
    float dayVisibility = 1;
    bool sunbeams = false;
    Vec3 sunDirection;
};
enum class Kind { Mote, Firefly };
constexpr size_t kMoteCount = 32, kFireflyCount = 96, kParticleCount = kMoteCount + kFireflyCount;
struct Particle {
    Kind kind = Kind::Mote;
    Vec3 position, anchor, drift;
    float age = 0, phase = 0, frequency = 0, radius = 1, alpha = 0, orbit = 0;
    uint32_t generation = 0;
};
struct BeamVertex {
    Vec3 position;
    float alpha = 0;
};
struct Beam {
    std::array<BeamVertex, 12> vertices;
};
float NightWeight(float hour);
bool InView(Vec3 position, const View& view, float margin);

class State {
  public:
    void Reset();
    void Step(const Input& input, const View& view);
    const std::array<Particle, kParticleCount>& Particles() const {
        return particles;
    }
    const std::vector<Beam>& Beams() const {
        return beams;
    }

  private:
    void Spawn(size_t index, const View& view);
    void BuildBeams(const Input& input, const View& view, float daylight);
    float Random();
    std::array<Particle, kParticleCount> particles{};
    std::vector<Beam> beams;
    uint32_t random = 0x53554D52;
    float elapsed = 0;
    bool initialized = false;
};
} // namespace SoHSeasonSummer
