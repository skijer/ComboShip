// Offline exporter of the actual private MM fixture. It also reserves the
// complete authored FrameFit envelope, including thin/off-center item seals.
#include "combo/BlackMarketMmShopLayout.h"
#include "combo/BlackMarketLock.h"
#include "soh/soh/Enhancements/randomizer/NeiGiFrameFit.h"
#include <cassert>
#include <cstdio>
#include <string_view>

static void PrintPoint(BlackMarketShelves::Point p) {
    std::printf("[%d,%d,%d]", p.x, p.y, p.z);
}

int main(int argc, char** argv) {
    using namespace BlackMarketMmShopLayout;
    const bool granny = argc > 1 && std::string_view(argv[1]) == "granny";
    // Exercise the production fit to obtain its full permitted height and
    // spinning radius. Feather is the tallest authored shelf replacement.
    const NeiGi::FrameBounds tall{"rocs_feather", {0, -1000, 0}, {0, 1000, 0}, 1,
                                   NeiGi::Kind::Neutral, {}};
    const NeiGi::FrameBounds wide{"fixture", {-1000, 0, 0}, {1000, 1, 0}, 2000,
                                   NeiGi::Kind::Neutral, {}};
    const auto vertical = NeiGi::FrameFit(tall, 1, true);
    const auto horizontal = NeiGi::FrameFit(wide, 1, true);
    const float radius = 1000 * horizontal.scale * kStockScale;
    const float bottom = (-1000 * vertical.scale + vertical.lift) * kStockScale;
    const float top = (1000 * vertical.scale + vertical.lift) * kStockScale;
    const float halfHeight = (top - bottom) * .5f;
    // Bound rx-half over every possible AABB half-size, including zero-sized
    // thin items at either extreme. The .85 ribbon bound, .45 second-loop
    // offset and one full 1/64 vertex unit conservatively enclose BuildLock.
    // This is the same universal derivation used by check_shop_layout.py;
    // actor bob is zero for this native OoT presentation.
    const auto extension = [](float half, float floor, float extra) {
        const float padding = kSealPaddingScale;
        return std::max((floor * padding + 1.4f * padding) / .875f,
                        (std::max(half, floor * padding) + 1.4f * padding) / .875f - half) +
               extra * padding + 1.f / 64;
    };
    const float ex = extension(radius, 2.4f, .85f);
    const float ey = extension(halfHeight, 3.2f, .85f);
    const float ez = extension(radius, 2.4f, .45f);
    const std::array<float, 3> minimum{-radius - ex, bottom - ey, -radius - ez};
    const std::array<float, 3> maximum{radius + ex, top + ey, radius + ez};
    // Check the derived reservation against real quantized native ribbons for
    // both centered and maximally off-center thin/full AABBs in each axis.
    const float bodyLow[3]{-radius, bottom, -radius};
    const float bodyHigh[3]{radius, top, radius};
    for (int x = 0; x < 5; ++x)
        for (int y = 0; y < 5; ++y)
            for (int z = 0; z < 5; ++z) {
                const int choices[3]{x, y, z};
                float lo[3], hi[3];
                for (int axis = 0; axis < 3; ++axis) {
                    const auto choice = choices[axis];
                    const float a = bodyLow[axis], b = bodyHigh[axis], mid = (a + b) * .5f;
                    lo[axis] = choice == 2 ? b : choice == 4 ? mid : a;
                    hi[axis] = choice == 0 ? a : choice == 3 ? mid : b;
                }
                const auto mesh = BlackMarketVisual::BuildLock(
                    { { lo[0], lo[1], lo[2] }, { hi[0], hi[1], hi[2] }, true }, 0, false, kSealPaddingScale);
                assert(mesh.valid);
                for (const auto& vertex : mesh.vertices) {
                    const float point[3]{mesh.center.x + vertex.x / BlackMarketVisual::kLockVertexScale,
                                         mesh.center.y + vertex.y / BlackMarketVisual::kLockVertexScale,
                                         mesh.center.z + vertex.z / BlackMarketVisual::kLockVertexScale};
                    for (int axis = 0; axis < 3; ++axis)
                        assert(point[axis] >= minimum[axis] && point[axis] <= maximum[axis]);
                }
            }
    std::printf("{\"stock_scale\":%.9g,\"seal_padding_scale\":%.9g,\"envelope\":[[%.9g,%.9g,%.9g],[%.9g,%.9g,%.9g]],"
                "\"corners\":[",
                kStockScale, kSealPaddingScale, minimum[0], minimum[1], minimum[2], maximum[0], maximum[1], maximum[2]);
    bool comma = false;
    for (const auto& corners : ShelfCorners()) {
        if (comma) std::printf(",");
        comma = true;
        std::printf("[");
        for (size_t i = 0; i < corners.size(); ++i) {
            if (i) std::printf(",");
            PrintPoint(corners[i]);
        }
        std::printf("]");
    }
    std::printf("],\"vertices\":[");
    comma = false;
    for (const auto& v : Vertices(granny)) {
        if (comma) std::printf(",");
        comma = true;
        std::printf("[%d,%d,%d,%d,%d,%u,%u,%u,%u]", v.position.x, v.position.y, v.position.z, v.s, v.t,
                    v.color[0], v.color[1], v.color[2], v.color[3]);
    }
    std::printf("],\"materials\":[");
    comma = false;
    for (size_t box = 0; box < kBoxes.size(); ++box)
        for (size_t face = 0; face < BlackMarketShelves::kFaces.size(); ++face) {
            if (comma) std::printf(",");
            comma = true;
            std::printf("%d", int(BlackMarketShelves::FaceMaterial(box, face, granny)));
        }
    std::printf("],\"flow_texture\":\"%s\",\"tint\":[218,210,255,255],\"wood_texture\":\"%s\","
                "\"atlas_texture\":\"%s\",\"native_seal_cases\":125,\"runtime_proven\":false}\n",
                BlackMarketShelves::kFlowTexture + 7, BlackMarketShelves::kWoodTexture,
                BlackMarketShelves::kAtlasTexture);
}
