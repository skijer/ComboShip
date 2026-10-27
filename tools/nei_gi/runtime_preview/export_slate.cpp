// Export the actual shared Slate sampler with the camera transformed into each
// rotating GI frame. Shimmer is deliberately disabled for the halo diagnosis.
#include "soh/Enhancements/randomizer/NeiGiEffectPolicy.h"
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 5)
        return 2;
    const auto frames = static_cast<uint32_t>(std::strtoul(argv[2], nullptr, 10));
    const float elevation = std::strtof(argv[3], nullptr) * NeiGi::Tau / 360;
    const auto kind = static_cast<NeiGi::Kind>(std::strtol(argv[4], nullptr, 10));
    if (!frames || frames > 720 || !NeiGi::IsSlate(kind))
        return 2;
    std::ofstream out(argv[1], std::ios::binary);
    out.write(reinterpret_cast<const char*>(&frames), sizeof(frames));
    for (uint32_t index = 0; index < frames; ++index) {
        // Runtime GI spin advances .02 radians per gameplay frame. Sampling one
        // complete turn retains the production sampler's actual frame timing.
        const auto frame = static_cast<uint32_t>(std::lround(index * NeiGi::Tau / (.02f * frames)));
        const float yaw = frame * .02f;
        const float sx = std::sin(elevation), cx = std::cos(elevation);
        const float sy = std::sin(yaw), cy = std::cos(yaw);
        const NeiGi::Basis camera{{cy, 0, sy}, {sx * sy, cx, -sx * cy}, {-cx * sy, sx, cx * cy}};
        const auto mesh = NeiGi::SampleSpecial(kind, frame, camera);
        const uint32_t count = mesh.count;
        out.write(reinterpret_cast<const char*>(&frame), sizeof(frame));
        out.write(reinterpret_cast<const char*>(&yaw), sizeof(yaw));
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));
        for (size_t i = 0; i < mesh.count; ++i) {
            const auto& v = mesh.vertices[i];
            // Exactly match the production int16 Vtx upload followed by /16.
            const float p[] = {static_cast<int16_t>(std::lround(v.p.x * 16)) / 16.f,
                               static_cast<int16_t>(std::lround(v.p.y * 16)) / 16.f,
                               static_cast<int16_t>(std::lround(v.p.z * 16)) / 16.f};
            const uint8_t rgba[] = {uint8_t(v.rgb >> 16), uint8_t(v.rgb >> 8), uint8_t(v.rgb), v.alpha};
            out.write(reinterpret_cast<const char*>(p), sizeof(p));
            out.write(reinterpret_cast<const char*>(rgba), sizeof(rgba));
        }
    }
    if (!out)
        return 1;
    std::cout << "Exported " << frames << " production Slate halo frames; shimmer off\n";
}
