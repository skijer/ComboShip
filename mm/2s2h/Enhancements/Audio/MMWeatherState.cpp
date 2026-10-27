#include "MMWeatherState.h"

#include <algorithm>

namespace MMWeather {
void State::Reset() {
    *this = State{};
}

int State::Random(int minimum, int maximum) {
    // Never consume MM's random stream: cosmetic weather must not affect gameplay.
    randomState ^= randomState << 13;
    randomState ^= randomState >> 17;
    randomState ^= randomState << 5;
    return minimum + static_cast<int>(randomState % (maximum - minimum + 1));
}

bool State::Step(const Settings& settings, bool eligible, int ticks) {
    if (!eligible) {
        Reset();
        return false;
    }
    bool strike = false;
    for (int tick = 0; tick < ticks; ++tick) {
        if (flashTicks > 0) {
            --flashTicks;
        }
        if (settings.enabled && (!wasEnabled || settings.intermittent != wasIntermittent)) {
            phase = wetTicks > 0 ? Phase::FadeOut : Phase::Dry;
            phaseTicks = Random(120, 240);
            thunderTicks = Random(600, 1500) * 100;
        }
        wasEnabled = settings.enabled;
        wasIntermittent = settings.intermittent;
        if (!settings.enabled) {
            wetTicks = std::max(0, wetTicks - 1);
            phase = Phase::Dry;
        } else if (!settings.intermittent) {
            wetTicks = std::min(60, wetTicks + 1);
        } else {
            switch (phase) {
                case Phase::Dry:
                    if (--phaseTicks <= 0) {
                        phase = Phase::FadeIn;
                    }
                    break;
                case Phase::FadeIn:
                    if (++wetTicks >= 60) {
                        phase = Phase::Sustain;
                        phaseTicks = Random(480, 840);
                    }
                    break;
                case Phase::Sustain:
                    if (--phaseTicks <= 0) {
                        phase = Phase::FadeOut;
                    }
                    break;
                case Phase::FadeOut:
                    if (--wetTicks <= 0) {
                        wetTicks = 0;
                        phase = Phase::Dry;
                        phaseTicks = Random(1200, 1500);
                    }
                    break;
            }
        }
        if (settings.enabled && settings.thunder && wetTicks > 0) {
            // Countdown in hundredths of a simulation tick so live frequency changes
            // apply immediately without restarting the interval or the active flash.
            thunderTicks -= std::clamp(settings.thunderFrequency, 25, 200);
            if (thunderTicks <= 0 && flashTicks == 0) {
                flashTicks = 66;
                thunderTicks = Random(600, 1500) * 100;
                strike = true;
            }
        } else {
            thunderTicks = 600 * 100;
        }
    }
    return strike;
}

int State::Density() const {
    return (wetTicks * 25 + 30) / 60;
}

float State::Intensity() const {
    return wetTicks / 60.0f;
}

float State::Overcast(const Settings& settings) const {
    return settings.overcast ? Intensity() : 0.0f;
}

int State::FlashAlpha() const {
    return flashTicks > 60 ? (67 - flashTicks) * 200 / 6 : flashTicks * 200 / 60;
}
} // namespace MMWeather
