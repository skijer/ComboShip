// Export the production song policy, using the renderer's 1/16-unit position
// packing. This preview has no substitute particle implementation.
#include "NeiGiSongEffectPolicy.h"
#include <cmath>
#include <fstream>

int main(int argc, char** argv) {
    if (argc != 2)
        return 2;
    std::ofstream out(argv[1], std::ios::binary);
    constexpr float x = 12 * NeiGi::Tau / 360, y = 25 * NeiGi::Tau / 360;
    const float sx = std::sin(x), cx = std::cos(x), sy = std::sin(y), cy = std::cos(y);
    const NeiGi::Basis camera{ { cy, 0, sy }, { sx * sy, cx, -sx * cy }, { -cx * sy, sx, cx * cy } };
    for (uint32_t frame = 0; frame < 180; ++frame) {
        for (int song : { CW_SONG_OOT_BOLERO, CW_SONG_OOT_SERENADE }) {
            const auto mesh = NeiGi::SampleSong(song, frame, camera);
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
        }
    }
    return out ? 0 : 1;
}
