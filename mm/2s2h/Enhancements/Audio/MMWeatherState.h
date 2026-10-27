#pragma once

#include <cstdint>

namespace MMWeather {
struct Settings {
    bool enabled = false;
    bool intermittent = false;
    bool overcast = true;
    bool thunder = true;
    int thunderFrequency = 100;
};

// All time is measured in 60 Hz simulation ticks, independent of rendering FPS.
class State {
  public:
    void Reset();
    bool Step(const Settings& settings, bool eligible, int ticks);
    int Density() const;
    float Intensity() const;
    float Overcast(const Settings& settings) const;
    int FlashAlpha() const;

  private:
    enum class Phase { Dry, FadeIn, Sustain, FadeOut };
    int Random(int minimum, int maximum);
    Phase phase = Phase::Dry;
    int wetTicks = 0;
    int phaseTicks = 0;
    int thunderTicks = 0;
    int flashTicks = 0;
    bool wasEnabled = false;
    bool wasIntermittent = false;
    uint32_t randomState = 0x4D4D5241;
};
} // namespace MMWeather
