// Exact current production meshes, plus the prior Mirror material as reference.
#include "../../combo/gameplay/MagicMirrorCast.h"
#include <fstream>

int main(int argc, char** argv) {
    if (argc != 2)
        return 2;
    std::ofstream out(argv[1], std::ios::binary);
    constexpr float x = 24 * NeiGi::Tau / 360, y = -18 * NeiGi::Tau / 360;
    const float sx = std::sin(x), cx = std::cos(x), sy = std::sin(y), cy = std::cos(y);
    const NeiGi::Basis camera{ { cy, 0, sy }, { sx * sy, cx, -sx * cy }, { -cx * sy, sx, cx * cy } };
    auto append = [&](const NeiGi::Mesh& mesh, uint32_t material) {
        const uint32_t count = mesh.count;
        out.write(reinterpret_cast<const char*>(&material), 4);
        out.write(reinterpret_cast<const char*>(&count), 4);
        for (size_t i = 0; i < mesh.count; ++i) {
            auto vertex = mesh.vertices[i];
            const float p[] = { std::round(vertex.p.x * 16) / 16, std::round(vertex.p.y * 16) / 16,
                                std::round(vertex.p.z * 16) / 16 };
            const uint8_t rgba[] = { uint8_t(vertex.rgb >> 16), uint8_t(vertex.rgb >> 8), uint8_t(vertex.rgb),
                                     vertex.alpha };
            const float uv[] = { vertex.u, vertex.v };
            out.write(reinterpret_cast<const char*>(p), 12);
            out.write(reinterpret_cast<const char*>(rgba), 4);
            out.write(reinterpret_cast<const char*>(uv), 8);
        }
    };
    for (bool forest : { false, true }) {
        const uint32_t layers = forest ? 3 : 2;
        out.write(reinterpret_cast<const char*>(&layers), 4);
        if (forest)
            append(ComboMagicMirrorCast::SampleSeal(1, 255), 11);
        constexpr auto palette = NeiUsedMagic::PortalPalette::Forest;
        append(NeiUsedMagic::SamplePortal(45, 1, 255, camera, palette), 0);
        append(NeiUsedMagic::SamplePortalSurface(45, 1, 255, palette), forest ? 12 : 2);
    }
    return !out;
}
