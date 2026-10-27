#include "2s2h/Enhancements/Audio/MMWeatherState.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>

using namespace MMWeather;

static void Advance(State& state, const Settings& settings, int ticks, int step = 3) {
    for (int i = 0; i < ticks; i += step) {
        state.Step(settings, true, step);
    }
}

int main() {
    Settings settings;
    State state;
    Advance(state, settings, 600);
    assert(state.Density() == 0 && state.FlashAlpha() == 0 && state.Overcast(settings) == 0.0f);

    settings.enabled = true;
    State initialThunder;
    for (int i = 0; i < 599; ++i) {
        assert(!initialThunder.Step(settings, true, 1));
    }
    State faster, slower;
    Settings fastSettings = settings;
    fastSettings.thunderFrequency = 200;
    Settings slowSettings = settings;
    slowSettings.thunderFrequency = 25;
    int fastStrike = 0, slowStrike = 0;
    for (int tick = 1; tick < 6000 && slowStrike == 0; ++tick) {
        if (faster.Step(fastSettings, true, 1) && fastStrike == 0) {
            fastStrike = tick;
        }
        if (slower.Step(slowSettings, true, 1)) {
            slowStrike = tick;
        }
    }
    assert(fastStrike > 0 && slowStrike >= fastStrike * 7);
    Advance(state, settings, 30);
    assert(state.Density() > 0 && state.Density() < 25);
    Advance(state, settings, 30);
    assert(state.Density() == 25);
    settings.overcast = false;
    assert(state.Overcast(settings) == 0.0f);
    settings.overcast = true;
    assert(state.Overcast(settings) == 1.0f);

    state.Step(settings, false, 3);
    assert(state.Density() == 0 && state.FlashAlpha() == 0);
    Advance(state, settings, 60);
    assert(state.Density() == 25);

    settings.enabled = false;
    Advance(state, settings, 30);
    assert(state.Density() > 0 && state.Density() < 25);
    Advance(state, settings, 30);
    assert(state.Density() == 0);

    settings.enabled = true;
    settings.intermittent = true;
    State oneTick, threeTicks;
    bool sawRain = false, sawDryAfterRain = false;
    for (int i = 0; i < 6000; i += 3) {
        Advance(oneTick, settings, 3, 1);
        Advance(threeTicks, settings, 3, 3);
        assert(oneTick.Density() == threeTicks.Density());
        assert(oneTick.FlashAlpha() == threeTicks.FlashAlpha());
        sawDryAfterRain |= sawRain && oneTick.Density() == 0;
        sawRain |= oneTick.Density() == 25;
    }
    assert(sawRain && sawDryAfterRain);
    int density = oneTick.Density();
    int flash = oneTick.FlashAlpha();
    for (int i = 0; i < 1000; ++i) {
        oneTick.Step(settings, true, 0);
    }
    assert(oneTick.Density() == density && oneTick.FlashAlpha() == flash);

    settings.intermittent = false;
    state.Reset();
    bool strike = false;
    for (int i = 0; i < 4000 && !strike; ++i) {
        strike = state.Step(settings, true, 1);
    }
    assert(strike && state.FlashAlpha() > 0);
    settings.thunder = false;
    settings.enabled = false;
    assert(!state.Step(settings, true, 1));
    assert(state.FlashAlpha() > 0);
    Advance(state, settings, 120);
    assert(state.FlashAlpha() == 0 && state.Density() == 0);
    state.Reset();
    assert(state.Density() == 0 && state.FlashAlpha() == 0);

    // A mode change retains a smooth fade, then enters the new cycle.
    settings.enabled = true;
    Advance(state, settings, 60);
    settings.intermittent = true;
    state.Step(settings, true, 3);
    assert(state.Density() > 0);
    Advance(state, settings, 120);
    assert(state.Density() == 0);
    std::puts("PASS MM weather state: defaults, view eligibility, fades, modes, timing, pause, flash release, reset");
}
