#include <cassert>
#include <cmath>
#include <cstdio>

#if __has_include("soh/Enhancements/Graphics/SeasonWinterState.h")
#include "soh/Enhancements/Graphics/SeasonWinterState.h"
using namespace SoHSeasonWinter;

static size_t VisibleMist(const State& state) {
    size_t visible = 0;
    for (const auto& wisp : state.Wisps())
        visible += wisp.alpha > 0.01f;
    return visible;
}
static bool VisibleAurora(const State& state) {
    for (const auto& ribbon : state.Ribbons())
        for (const auto& vertex : ribbon.vertices)
            if (vertex.alpha > 0.01f)
                return true;
    return false;
}
static void AuroraInNativeCamera(const State& state) {
    // Ordinary third-person views look downward. The shared MM/OoT ribbons
    // must retain a colored core in that sky frustum, beyond merely issuing
    // off-screen meshes with nonzero alpha.
    for (float aspect : { 4.0f / 3.0f, 16.0f / 9.0f })
        for (float pitch : { 10.0f, 15.0f })
            for (int yaw = 0; yaw < 360; yaw += 30) {
                const float p = pitch * 0.0174532925f, y = yaw * 0.0174532925f;
                View view{ { 0, 100, 0 },
                           { std::sin(y) * std::cos(p), -std::sin(p), std::cos(y) * std::cos(p) },
                           { -std::cos(y), 0, std::sin(y) },
                           { std::sin(y) * std::sin(p), std::cos(p), std::cos(y) * std::sin(p) },
                           0.5773503f,
                           aspect };
                bool coreInView = false;
                for (const auto& ribbon : state.Ribbons())
                    for (const auto& vertex : ribbon.vertices) {
                        const auto relative = vertex.position - view.eye;
                        const float depth = SoHSeasonParticles::Dot(relative, view.forward);
                        coreInView |=
                            vertex.alpha > 0.1f && depth > 0 &&
                            std::abs(SoHSeasonParticles::Dot(relative, view.right)) <
                                depth * view.tanHalfFov * view.aspect * 0.95f &&
                            std::abs(SoHSeasonParticles::Dot(relative, view.up)) < depth * view.tanHalfFov * 0.95f;
                    }
                assert(coreInView && "night aurora core must reach the native third-person sky frustum");
            }
}
int main() {
    View view{ { 0, 100, 0 }, { 0, 0, 1 }, { -1, 0, 0 }, { 0, 1, 0 }, 0.5773503f, 16.0f / 9.0f };
    Input input;
    input.eligible = input.mist = input.aurora = input.skyVisible = true;
    input.seconds = 0.05f;
    input.hour = 12;
    input.origin = { 0, 0, 0 };
    input.clipFar = 12800;
    State state;
    for (int i = 0; i < 40; ++i)
        state.Step(input, view);
    assert(VisibleMist(state) >= 8 && !VisibleAurora(state));
    assert(NightWeight(12) == 0 && NightWeight(18) == 0 && NightWeight(6) == 0);
    assert(NightWeight(23) == 1 && NightWeight(0) == 1);
    assert(NightWeight(19) > 0.4f && NightWeight(19) < 0.6f);
    assert(NightWeight(5) > 0.4f && NightWeight(5) < 0.6f);
    input.hour = 23;
    state.Step(input, view);
    assert(VisibleMist(state) >= 8 && VisibleAurora(state));
    AuroraInNativeCamera(state);
    auto evolvingSky = state;
    for (int frame = 0; frame < 3000; ++frame) {
        evolvingSky.Step(input, view);
        if (frame % 20 == 0)
            AuroraInNativeCamera(evolvingSky); // Entire slow wave cycle, not only the initial arc placement.
    }
    auto wisps = state.Wisps();
    auto ribbons = state.Ribbons();
    input.paused = true;
    for (int i = 0; i < 100; ++i)
        state.Step(input, view);
    for (size_t i = 0; i < wisps.size(); ++i) {
        const auto& old = wisps[i];
        const auto& now = state.Wisps()[i];
        assert(old.position.x == now.position.x && old.position.z == now.position.z && old.alpha == now.alpha);
    }
    // Turning the camera must not rotate the aurora with the screen.
    view.forward = { 1, 0, 0 };
    view.right = { 0, 0, 1 };
    state.Step(input, view);
    for (size_t i = 0; i < ribbons.size(); ++i)
        for (size_t j = 0; j < ribbons[i].vertices.size(); ++j) {
            const auto a = ribbons[i].vertices[j].position, b = state.Ribbons()[i].vertices[j].position;
            assert(a.x == b.x && a.y == b.y && a.z == b.z);
        }
    input.mist = false;
    state.Step(input, view);
    assert(VisibleMist(state) == 0 && VisibleAurora(state));
    input.aurora = false;
    input.mist = true;
    state.Step(input, view);
    assert(VisibleMist(state) >= 8 && !VisibleAurora(state));
    input.aurora = true;
    input.skyVisible = false;
    state.Step(input, view);
    assert(!VisibleAurora(state));
    input.skyVisible = true;
    input.clipFar = 2000;
    state.Step(input, view);
    for (const auto& ribbon : state.Ribbons())
        for (const auto& vertex : ribbon.vertices) {
            assert(SoHSeasonParticles::Length(vertex.position - view.eye) < 2000);
            assert(std::isfinite(vertex.alpha) && vertex.alpha >= 0 && vertex.alpha <= 0.55f);
        }
    const auto oldGeneration = state.Wisps()[0].generation;
    input.eligible = false;
    state.Step(input, view);
    assert(VisibleMist(state) == 0 && !VisibleAurora(state));
    input.eligible = true;
    input.paused = false;
    view.eye.x += 100000;
    input.origin.x += 100000;
    state.Step(input, view);
    assert(VisibleMist(state) == 0);                     // Fresh entries fade in after scene/reset.
    assert(state.Wisps()[0].generation > oldGeneration); // Re-entry must not reuse an interpolation lifetime.
    for (int i = 0; i < 40; ++i)
        state.Step(input, view);
    assert(VisibleMist(state) >= 8 && VisibleAurora(state));
    std::puts(
        "PASS winter state: independent mist/aurora, day/night/dusk/dawn, pause, sky orientation, clipping and reset");
}
#else
int main() {
    std::fputs("FAIL: snow-independent winter atmosphere is missing\n", stderr);
    return 1;
}
#endif
