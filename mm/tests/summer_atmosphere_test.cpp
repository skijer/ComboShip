#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>

#if __has_include("2s2h/Enhancements/Graphics/MMSummerAtmosphereState.h")
#include "2s2h/Enhancements/Graphics/MMSummerAtmosphereState.h"

using namespace MMSummer;

static View TestView() {
    return { { 0, 100, 0 }, { 0, 0, 1 }, { -1, 0, 0 }, { 0, 1, 0 }, 0.5773503f, 16.0f / 9.0f };
}

static Input TestInput(float hour = 12) {
    return { true, false, 0.05f, hour, 1.0f, false, { 0.45f, 0.8f, -0.25f } };
}

static size_t Visible(const State& state, Kind kind) {
    size_t result = 0;
    for (const auto& particle : state.Particles()) {
        result += particle.kind == kind && particle.alpha > 0.01f;
    }
    return result;
}

// At a settled summer night the extra swarm must be visible, and even its
// dim pulse must read against foliage. These checks exercise the actual state
// consumed by the native renderer instead of checking tuning constants.
static void AssertNightReadability(const State& state, const View& view) {
    size_t visible = 0, central = 0;
    float totalAlpha = 0, brightest = 0;
    for (const auto& p : state.Particles()) {
        if (p.kind != Kind::Firefly || !InView(p.position, view, 1.0f))
            continue;
        visible += p.alpha > 0.01f;
        const auto relative = p.position - view.eye;
        const float depth = Dot(relative, view.forward);
        assert(p.radius / depth > 0.00245f); // Soft halo stays readable at the far tier.
        const float x = std::abs(Dot(relative, view.right)) / (depth * view.tanHalfFov * view.aspect);
        const float y = std::abs(Dot(relative, view.up)) / (depth * view.tanHalfFov);
        if (std::max(x, y) < 0.90f && p.age >= 0.8f) {
            ++central;
            totalAlpha += p.alpha;
            brightest = std::max(brightest, p.alpha);
            assert(p.alpha >= 0.46f); // Entry/edge fading is deliberately excluded.
        }
    }
    assert(visible >= 88 && visible <= 100); // Denser swarm with a fixed render budget.
    assert(central >= 80 && totalAlpha / central > 0.70f);
    assert(brightest > 0.98f);
}

int main() {
    State state;
    auto view = TestView();
    auto input = TestInput();
    for (int i = 0; i < 40; ++i)
        state.Step(input, view);
    assert(Visible(state, Kind::Mote) >= 24);
    assert(Visible(state, Kind::Firefly) == 0);
    // Far seeds still occupy several pixels at a 720p/60-degree view;
    // a dust-sized radius hides the seed tuft even with the correct texture.
    for (const auto& p : state.Particles()) {
        if (p.kind == Kind::Mote)
            assert(p.radius / Dot(p.position - view.eye, view.forward) > 0.0027f);
    }
    assert(state.Beams().size() == 0); // Shafts remain an independent review option.
    input.hour = 23;
    state.Step(input, view);
    assert(Visible(state, Kind::Mote) == 0);
    assert(Visible(state, Kind::Firefly) >= 30);
    AssertNightReadability(state, view);
    assert(NightWeight(18) > 0.49f && NightWeight(18) < 0.51f);
    assert(NightWeight(17) == 0 && NightWeight(19) == 1);
    assert(NightWeight(0) == NightWeight(24));
    assert(NightWeight(5) == 1 && NightWeight(7) == 0);

    const auto frozen = state.Particles();
    input.paused = true;
    for (int i = 0; i < 30; ++i)
        state.Step(input, view);
    for (size_t i = 0; i < frozen.size(); ++i) {
        assert(Length(state.Particles()[i].position - frozen[i].position) == 0);
        assert(state.Particles()[i].alpha == frozen[i].alpha);
    }
    // Time changes while paused must update night fading without moving or
    // respawning the denser swarm. Dusk/dawn stay smooth and daytime stays clear.
    for (const auto hour : { 17.0f, 18.0f, 19.0f, 5.0f, 6.0f, 7.0f }) {
        input.hour = hour;
        state.Step(input, view);
        const float weight = hour == 18 || hour == 6 ? 0.5f : hour == 19 || hour == 5 ? 1.0f : 0.0f;
        for (size_t i = 0; i < frozen.size(); ++i) {
            const auto& p = state.Particles()[i];
            assert(Length(p.position - frozen[i].position) == 0);
            assert(p.generation == frozen[i].generation && p.age == frozen[i].age);
            if (p.kind == Kind::Firefly)
                assert(std::abs(p.alpha - frozen[i].alpha * weight) < 0.000001f);
        }
    }
    input.hour = 23;
    input.paused = false;

    // Ordinary motion stays continuous; any recycle is separately identified
    // and starts transparent, rather than interpolating across the scene.
    for (int frame = 0; frame < 1000; ++frame) {
        const auto before = state.Particles();
        state.Step(input, view);
        for (size_t i = 0; i < before.size(); ++i) {
            const auto& p = state.Particles()[i];
            assert(std::isfinite(p.position.x) && std::isfinite(p.position.y) && std::isfinite(p.position.z));
            assert(p.radius > 0 && p.radius <= (p.kind == Kind::Mote ? 10.0f : 5.8f));
            assert(p.alpha >= 0 && p.alpha <= 1);
            if (p.generation == before[i].generation) {
                assert(Length(p.position - before[i].position) < 8.0f);
            } else {
                assert(p.alpha == 0);
            }
        }
    }
    // Camera travel repopulates all depth tiers and keeps the new particles
    // inside the frustum, without relying on native snow actors or RNG.
    view.eye.x += 5000;
    state.Step(input, view);
    for (const auto& p : state.Particles())
        assert(InView(p.position, view, 1.10f));

    input.hour = 12;
    input.sunbeams = true;
    state.Step(input, view);
    assert(state.Beams().size() == 3);
    for (const auto& beam : state.Beams()) {
        for (size_t i = 0; i < beam.vertices.size(); ++i) {
            assert(std::isfinite(beam.vertices[i].position.x));
            assert(beam.vertices[i].alpha >= 0 && beam.vertices[i].alpha <= 0.12f);
            if (i < 3 || i >= 9 || i % 3 != 1)
                assert(beam.vertices[i].alpha == 0);
        }
        assert(beam.vertices[4].alpha > 0);
    }
    input.dayVisibility = 0; // Added/native rain dims sunlight only.
    state.Step(input, view);
    assert(state.Beams().empty() && Visible(state, Kind::Mote) == 0);
    input.hour = 23;
    state.Step(input, view);
    assert(Visible(state, Kind::Firefly) > 0);

    input.eligible = false;
    state.Step(input, view);
    assert(Visible(state, Kind::Mote) == 0 && Visible(state, Kind::Firefly) == 0);
    assert(state.Beams().empty());
    input = TestInput();
    state.Step(input, view);
    for (const auto& p : state.Particles())
        assert(p.alpha == 0);
    std::puts("PASS summer: day/night/dusk, pause, continuous flight, recycling, camera travel, beams, rain, reset");
}
#else
int main() {
    std::fputs("FAIL: summer has no daytime motes, nighttime fireflies, or sunbeam state\n", stderr);
    return 1;
}
#endif
