// Export the exact GI sampler at the same position/UV precision as native GBI.
#include "soh/Enhancements/randomizer/NeiElementalArrowGi.h"
#include <fstream>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    std::ofstream out(argv[1], std::ios::binary);
    const uint32_t frames = 360;
    out.write(reinterpret_cast<const char*>(&frames), 4);
    const float x = 12 * NeiGi::Tau / 360, y = 25 * NeiGi::Tau / 360;
    const NeiGi::Basis camera{
        {std::cos(y), 0, std::sin(y)},
        {std::sin(x) * std::sin(y), std::cos(x), -std::sin(x) * std::cos(y)},
        {-std::cos(x) * std::sin(y), std::sin(x), std::cos(x) * std::cos(y)}};
    auto mesh = [&](const NeiGi::Mesh& m, bool textured) {
        const uint32_t count = m.count;
        out.write(reinterpret_cast<const char*>(&count), 4);
        for (size_t i = 0; i < m.count; ++i) {
            const auto& v = m.vertices[i];
            const float p[] = {std::round(v.p.x * 16) / 16, std::round(v.p.y * 16) / 16,
                               std::round(v.p.z * 16) / 16};
            const uint8_t rgba[] = {uint8_t(v.rgb >> 16), uint8_t(v.rgb >> 8), uint8_t(v.rgb), v.alpha};
            auto uv = [](float value) {
                const int packed = std::lround(value * 32 * 32);
                return (((packed * 65535) >> 16) / 32.f + .5f) / 32;
            };
            const float coords[] = {textured ? uv(v.u) : 0, textured ? uv(v.v) : 0};
            out.write(reinterpret_cast<const char*>(p), sizeof(p));
            out.write(reinterpret_cast<const char*>(rgba), sizeof(rgba));
            out.write(reinterpret_cast<const char*>(coords), sizeof(coords));
        }
    };
    for (uint32_t frame = 0; frame < frames; ++frame) {
        for (int profile = 1; profile <= 3; ++profile) {
            const auto layers = NeiArrowGi::Sample(profile, frame, camera);
            mesh(layers.veil, true); mesh(layers.energy, false); mesh(layers.shimmer, false);
        }
    }
    if (!out) return 1;
    std::cout << "Exported 360 frames of production arrow aura geometry\n";
}
