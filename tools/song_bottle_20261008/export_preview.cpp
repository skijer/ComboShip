// Quantized production particle/shimmer triangles, in actual submission order.
#include "NeiGiSongEffectPolicy.h"
#include <fstream>

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    std::ofstream out(argv[1], std::ios::binary);
    const int songs[] = { CW_SONG_HEALING, CW_SONG_SONATA, CW_SONG_LULLABY_INTRO, CW_SONG_LULLABY,
                         CW_SONG_NOVA, CW_SONG_ELEGY, CW_SONG_OATH, CW_SONG_INVERTED_TIME, CW_SONG_DOUBLE_TIME };
    constexpr float x = 12 * NeiGi::Tau / 360, y = 25 * NeiGi::Tau / 360;
    const NeiGi::Basis camera{ { std::cos(y), 0, std::sin(y) },
                              { std::sin(x) * std::sin(y), std::cos(x), -std::sin(x) * std::cos(y) },
                              { -std::cos(x) * std::sin(y), std::sin(x), std::cos(x) * std::cos(y) } };
    auto write = [&](const NeiGi::Mesh& mesh) {
        const uint32_t count = mesh.count;
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));
        for (size_t i = 0; i < mesh.count; ++i) {
            const auto& v = mesh.vertices[i];
            const float point[] = { std::round(v.p.x * 16) / 16, std::round(v.p.y * 16) / 16,
                                    std::round(v.p.z * 16) / 16 };
            const uint8_t rgba[] = { uint8_t(v.rgb >> 16), uint8_t(v.rgb >> 8), uint8_t(v.rgb), v.alpha };
            out.write(reinterpret_cast<const char*>(point), sizeof(point));
            out.write(reinterpret_cast<const char*>(rgba), sizeof(rgba));
        }
    };
    for (uint32_t frame = 0; frame < 240; frame += 3) {
        for (int song : songs) {
            auto shimmer = NeiGi::SampleShimmer(frame, true, camera);
            // Same identity recoloring as NeiGi_DrawShimmerOverlay.
            for (size_t i = 0; i < shimmer.count; ++i)
                if (shimmer.vertices[i].rgb != 0xFFFFFF)
                    shimmer.vertices[i].rgb = ComboSongColorHex(song);
            write(shimmer);
            write(NeiGi::SampleSong(song, frame, camera));
        }
    }
    return out ? 0 : 1;
}
