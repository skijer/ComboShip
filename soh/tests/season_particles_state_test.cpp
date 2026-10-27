#include <cassert>
#include <cmath>
#include <cstdio>

#if __has_include("soh/Enhancements/Graphics/SeasonParticleState.h")
#include "soh/Enhancements/Graphics/SeasonParticleState.h"
using namespace SoHSeasonParticles;

static View TestView() {
    return { { 0, 100, 0 }, { 0, 0, 1 }, { -1, 0, 0 }, { 0, 1, 0 }, 0.5773503f, 16.0f / 9.0f };
}
static Input TestInput(int season = 2) {
    Input input;
    input.play = reinterpret_cast<void*>(1);
    input.scene = 81;
    input.room = 0;
    input.season = season;
    input.eligible = true;
    input.frame = 1;
    return input;
}
static bool Same(Vec3 a, Vec3 b) {
    return Length(a - b) < 0.00001f;
}
int main() {
    State state;
    auto view = TestView();
    auto input = TestInput();
    state.Tick(input, view);
    assert(state.Count() == 96);
    const auto first = state.Particles();
    int bands[3]{};
    const float minimum[] = { 700, 1200, 3200 };
    const float maximum[] = { 1000, 2800, 6000 };
    for (size_t i = 0; i < state.Count(); ++i) {
        const auto& p = first[i];
        assert(p.band <= 2);
        ++bands[p.band];
        const auto screen = Project(p.position, view);
        assert(screen.depth > 0 && std::abs(screen.x) < 0.85f && std::abs(screen.y) < 0.85f);
        const float distance = Length(p.position - view.eye);
        assert(distance >= minimum[p.band] - 0.1f && distance <= maximum[p.band] + 0.1f);
    }
    assert(bands[0] == 16 && bands[1] == 32 && bands[2] == 48);
    ++input.frame;
    state.Tick(input, view);
    const auto advanced = state.Particles();
    for (size_t i = 0; i < state.Count(); ++i) {
        assert(advanced[i].position.y < first[i].position.y);
        assert(advanced[i].generation == first[i].generation);
    }
    // Interpolation redraws and pause frames must never integrate a second step.
    input.wind = { 1, 0, 1 };
    input.windSpeed = 120;
    for (int i = 0; i < 20; ++i)
        state.Tick(input, view);
    for (size_t i = 0; i < state.Count(); ++i)
        assert(Same(state.Particles()[i].position, advanced[i].position));
    input.paused = true;
    for (int i = 0; i < 20; ++i) {
        ++input.frame;
        state.Tick(input, view);
    }
    for (size_t i = 0; i < state.Count(); ++i) {
        assert(Same(state.Particles()[i].position, advanced[i].position));
        assert(state.Particles()[i].age == advanced[i].age);
        assert(state.Particles()[i].rotation == advanced[i].rotation);
    }
    input.paused = false;
    ++input.frame;
    state.Tick(input, view);
    assert(!Same(state.Particles()[0].position, advanced[0].position));

    // Camera travel recycles inside each view band, identifies replacements,
    // and starts their own fade instead of dragging a whole field with Link.
    auto beforeTravel = state.Particles();
    view.eye.x += 20000;
    ++input.frame;
    state.Tick(input, view);
    for (size_t i = 0; i < state.Count(); ++i) {
        const auto& p = state.Particles()[i];
        assert(p.generation > beforeTravel[i].generation);
        const auto screen = Project(p.position, view);
        assert(screen.depth > 0 && std::abs(screen.x) < 0.85f && std::abs(screen.y) < 0.85f);
        assert(p.age == 1);
    }
    input.eligible = false;
    state.Tick(input, view);
    assert(state.Count() == 0);
    input.eligible = true;
    state.Tick(input, view);
    assert(state.Count() == 96 && state.Particles()[0].age == 1);
    const auto reentry = state.Particles()[0];
    ++input.room;
    state.Tick(input, view);
    assert(state.Particles()[0].generation > reentry.generation);
    assert(Same(state.Particles()[0].position, reentry.position));
    const auto roomEntry = state.Particles()[0];
    ++input.scene;
    state.Tick(input, view);
    assert(state.Particles()[0].generation > roomEntry.generation);
    input.frame = 0; // Save-state rollback cannot retain particles from a future frame.
    state.Tick(input, view);
    assert(state.Particles()[0].age == 1);
    input.season = 0;
    state.Tick(input, view);
    assert(state.Count() == 96 && state.Particles()[0].age == 1);
    input.season = -1;
    state.Tick(input, view);
    assert(state.Count() == 0);
    input.season = 1;
    state.Tick(input, view);
    assert(state.Count() == 0);

    // Winter uses only64 private snow slots; a native snow emitter owns its
    // scene unmodified. Flurries change motion without increasing the budget.
    input = TestInput(3);
    view = TestView();
    state.Tick(input, view);
    assert(state.Count() == 64);
    const auto snow = state.Particles();
    State flurry;
    input.flurries = true;
    flurry.Tick(input, view);
    ++input.frame;
    flurry.Tick(input, view);
    input.flurries = false;
    state.Tick(input, view);
    assert(state.Count() == 64 && flurry.Count() == 64);
    assert(state.Particles()[0].position.y < snow[0].position.y);
    assert(!Same(state.Particles()[0].position, flurry.Particles()[0].position));
    input.nativeSnow = true;
    state.Tick(input, view);
    assert(state.Count() == 0);
    input.nativeSnow = false;
    state.Tick(input, view);
    assert(state.Count() == 64);
    State reproducible;
    input = TestInput(3);
    reproducible.Tick(input, view);
    for (size_t i = 0; i < 64; ++i)
        assert(Same(reproducible.Particles()[i].position, snow[i].position));
    state.Reset();
    assert(state.Count() == 0);
    std::puts("PASS falling particles:96/3 bands, private deterministic state, once/frame, pause, recycling, "
              "eligibility, room/scene/reentry/rollback,64 snow, native ownership, flurries");
}
#else
int main() {
    std::fputs("FAIL: standalone seasonal particles have no private native lifecycle state\n", stderr);
    return 1;
}
#endif
