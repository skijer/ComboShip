#pragma once

#include "SeasonParticleState.h"
#include <array>
#include <cstdint>

// Private cosmetic state. No snow slots, actors, save data or gameplay RNG.
namespace SoHSeasonWinter {
using Vec3 = SoHSeasonParticles::Vec3;
using View = SoHSeasonParticles::View;
constexpr size_t kMistCount = 12, kRibbonCount = 3, kRibbonColumns = 8;
struct Input {
    bool eligible = false, paused = false, mist = false, aurora = false, skyVisible = false;
    float seconds = 0, hour = 12, clipFar = 12800;
    Vec3 origin;
};
struct Wisp {
    Vec3 position, anchor;
    float age = 0, phase = 0, width = 0, height = 0, alpha = 0;
    uint32_t generation = 0;
};
struct RibbonVertex {
    Vec3 position;
    float alpha = 0;
    uint8_t red = 0, green = 0, blue = 0;
};
struct Ribbon {
    std::array<RibbonVertex, kRibbonColumns * 3> vertices{};
};
float NightWeight(float hour);
class State {
  public:
    void Reset();
    void Step(const Input& input, const View& view);
    const std::array<Wisp, kMistCount>& Wisps() const {
        return wisps;
    }
    const std::array<Ribbon, kRibbonCount>& Ribbons() const {
        return ribbons;
    }

  private:
    void Spawn(size_t index, const Input& input, const View& view);
    void BuildAurora(const Input& input, const View& view);
    float Random();
    std::array<Wisp, kMistCount> wisps{};
    std::array<Ribbon, kRibbonCount> ribbons{};
    uint32_t random = 0x57494E54;
    float elapsed = 0;
    bool initialized = false;
};
} // namespace SoHSeasonWinter
