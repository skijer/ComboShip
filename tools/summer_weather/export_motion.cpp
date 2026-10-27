#include "2s2h/Enhancements/Graphics/MMSummerAtmosphereState.h"
#include <cstdio>
#include <cstring>

int main(int argc, char** argv) {
    MMSummer::State state;
    const MMSummer::View view = { { 0, 100, 0 }, { 0, 0, 1 }, { -1, 0, 0 }, { 0, 1, 0 }, 0.5773503f, 16.0f / 9.0f };
    MMSummer::Input input = { true, false, 0.05f, 12, 1, true, { 0.45f, 0.8f, -0.25f } };
    const bool initial = argc > 1 && std::strcmp(argv[1], "initial") == 0;
    const bool clip = argc > 1 && std::strcmp(argv[1], "clip") == 0;
    const int count = initial ? 1 : clip ? 321 : 241;
    std::puts("[");
    for (int frame = 0; frame < count; ++frame) {
        if (clip) {
            input.hour = frame < 161 ? 12 : frame < 221 ? 18 : 23;
            input.sunbeams = frame >= 81 && frame < 221;
        } else if (!initial) {
            input.hour = frame < 81 ? 12 : frame < 121 ? 18 : 23;
            input.dayVisibility = frame >= 181 && frame < 201 ? 0 : 1;
            input.paused = frame >= 201 && frame < 221;
            input.sunbeams = frame < 221;
            input.eligible = frame < 235 || frame >= 240;
        }
        state.Step(input, view);
        if (frame) std::puts(",");
        std::printf("{\"frame\":%d,\"input\":{\"hour\":%.1f,\"dayVisibility\":%.1f,\"paused\":%s,\"sunbeams\":%s,\"eligible\":%s},\"particles\":[",
                    frame, input.hour, input.dayVisibility, input.paused ? "true" : "false",
                    input.sunbeams ? "true" : "false", input.eligible ? "true" : "false");
        size_t i = 0;
        for (const auto& p : state.Particles()) {
            if (i++) std::printf(",");
            std::printf("{\"kind\":%d,\"position\":[%.6f,%.6f,%.6f],\"drift\":[%.6f,%.6f,%.6f],"
                        "\"phase\":%.6f,\"frequency\":%.6f,\"orbit\":%.6f,\"radius\":%.6f,\"alpha\":%.6f}",
                        p.kind == MMSummer::Kind::Firefly, p.position.x, p.position.y, p.position.z,
                        p.drift.x, p.drift.y, p.drift.z, p.phase, p.frequency, p.orbit, p.radius, p.alpha);
        }
        std::printf("],\"beams\":[");
        size_t b = 0;
        for (const auto& beam : state.Beams()) {
            if (b++) std::printf(",");
            std::printf("[");
            size_t v = 0;
            for (const auto& vertex : beam.vertices) {
                if (v++) std::printf(",");
                std::printf("[%.6f,%.6f,%.6f,%.6f]", vertex.position.x, vertex.position.y,
                            vertex.position.z, vertex.alpha);
            }
            std::printf("]");
        }
        std::printf("]}");
    }
    std::puts("\n]");
}
