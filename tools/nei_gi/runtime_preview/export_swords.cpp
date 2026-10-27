// Export the production sword samplers with camera axes in each rotating GI.
// No preview-specific particles are generated here.
#include "soh/Enhancements/randomizer/NeiGiEffectPolicy.h"
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>

namespace {
void WriteMesh(std::ofstream& out, const NeiGi::Mesh& mesh) {
    for (size_t i = 0; i < mesh.count; ++i) {
        const auto& v = mesh.vertices[i];
        // The native runtime uploads signed int16 positions at 1/16 units.
        const float p[] = {static_cast<int16_t>(std::lround(v.p.x * 16)) / 16.f,
                           static_cast<int16_t>(std::lround(v.p.y * 16)) / 16.f,
                           static_cast<int16_t>(std::lround(v.p.z * 16)) / 16.f};
        const uint8_t rgba[] = {uint8_t(v.rgb >> 16), uint8_t(v.rgb >> 8), uint8_t(v.rgb), v.alpha};
        out.write(reinterpret_cast<const char*>(p), sizeof(p));
        out.write(reinterpret_cast<const char*>(rgba), sizeof(rgba));
    }
}
} // namespace

int main(int argc, char** argv) {
    if (argc != 5)
        return 2;
    const auto frames = static_cast<uint32_t>(std::strtoul(argv[2], nullptr, 10));
    const float elevation = std::strtof(argv[3], nullptr) * NeiGi::Tau / 360;
    const auto kind = static_cast<NeiGi::Kind>(std::strtol(argv[4], nullptr, 10));
    if (!frames || frames > 720 || !std::isfinite(elevation))
        return 2;
    std::ofstream out(argv[1], std::ios::binary);
    out.write("SWFX", 4);
    const uint32_t version = 1;
    out.write(reinterpret_cast<const char*>(&version), sizeof(version));
    out.write(reinterpret_cast<const char*>(&frames), sizeof(frames));
    for (uint32_t index = 0; index < frames; ++index) {
        // Match .02 radians per gameplay frame, sampling one complete turn.
        const auto frame = static_cast<uint32_t>(std::lround(index * NeiGi::Tau / (.02f * frames)));
        const float yaw = frame * .02f;
        const float sx = std::sin(elevation), cx = std::cos(elevation);
        const float sy = std::sin(yaw), cy = std::cos(yaw);
        const NeiGi::Basis camera{{cy, 0, sy}, {sx * sy, cx, -sx * cy}, {-cx * sy, sx, cx * cy}};
        const auto intrinsic = NeiGi::SampleSpecial(kind, frame, camera);
        const auto shimmer = NeiGi::SampleShimmer(frame, true, camera, kind);
        if (intrinsic.count > 1536 || shimmer.count > 1536 || intrinsic.count % 3 || shimmer.count % 3)
            return 3;
        const uint32_t intrinsicCount = intrinsic.count, shimmerCount = shimmer.count;
        out.write(reinterpret_cast<const char*>(&frame), sizeof(frame));
        out.write(reinterpret_cast<const char*>(&yaw), sizeof(yaw));
        out.write(reinterpret_cast<const char*>(&intrinsicCount), sizeof(intrinsicCount));
        out.write(reinterpret_cast<const char*>(&shimmerCount), sizeof(shimmerCount));
        WriteMesh(out, intrinsic);
        WriteMesh(out, shimmer);
    }
    if (!out)
        return 1;
    std::cout << "Exported " << frames << " production sword effect frames, kind " << int(kind)
              << "; intrinsic and shimmer enabled\n";
}
