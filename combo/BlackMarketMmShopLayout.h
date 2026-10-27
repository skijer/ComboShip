#pragma once

#include "BlackMarketShelves.h"

// Private MM layout for the OoT Data4 camera. The accepted OoT fixture and all
// ordinary shops keep BlackMarketShelves' original geometry unchanged.
namespace BlackMarketMmShopLayout {
using BlackMarketShelves::Box;
using BlackMarketShelves::Point;
using BlackMarketShelves::Vertex;
inline constexpr float kStockScale = .27f;
inline constexpr float kSealPaddingScale = .5f;
// Bring the scene around the keeper rather than moving the shop eye back to
// fit tall display banks. Move both banks 12 units toward the customer;
// the compact upper row and short separator keep complete item/seal bounds clear.
inline constexpr int16_t kOffsetY = -6;
inline constexpr int16_t kOffsetZ = -58;

// Compact rows and a short front lip leave a clear straight path from both
// rows to the native confirmation position. In particular, the upper offer
// clears the middle board before descending through that board's height.
inline constexpr std::array<Box, 12> kBoxes = { {
    { { 14, 44, 80 }, { 18, 106, 83 } },
    { { 88, 44, 80 }, { 92, 106, 83 } },
    { { 18, 46, 80 }, { 88, 106, 83 } },
    { { 14, 44, 78 }, { 92, 46, 99 } },
    { { 14, 76, 78 }, { 92, 77, 90 } },
    { { 14, 106, 78 }, { 92, 109, 99 } },
    { { -92, 44, 80 }, { -88, 106, 83 } },
    { { -18, 44, 80 }, { -14, 106, 83 } },
    { { -88, 46, 80 }, { -18, 106, 83 } },
    { { -92, 44, 78 }, { -14, 46, 99 } },
    { { -92, 76, 78 }, { -14, 77, 90 } },
    { { -92, 106, 78 }, { -14, 109, 99 } },
} };

inline auto ShelfCorners() {
    std::array<std::array<Point, 8>, 12> result;
    for (size_t i = 0; i < kBoxes.size(); ++i) {
        result[i] = BlackMarketShelves::Corners(kBoxes[i]);
        for (auto& point : result[i]) {
            point.y += kOffsetY;
            point.z += kOffsetZ;
        }
    }
    return result;
}

inline auto CollisionCorners() {
    constexpr std::array<Box, 2> volumes{ {
        { { 14, 0, 78 }, { 92, 109, 99 } },
        { { -92, 0, 78 }, { -14, 109, 99 } },
    } };
    auto result = std::array<std::array<Point, 8>, 2>{ BlackMarketShelves::Corners(volumes[0]),
                                                       BlackMarketShelves::Corners(volumes[1]) };
    for (auto& bank : result)
        for (auto& point : bank) {
            point.y = std::max<int16_t>(0, point.y + kOffsetY);
            point.z += kOffsetZ;
        }
    return result;
}

inline const std::vector<Vertex>& Vertices(bool granny = false) {
    static const auto variants = [] {
        std::array<std::vector<Vertex>, 2> result;
        const auto corners = ShelfCorners();
        for (int granny = 0; granny < 2; ++granny) {
            // Keep the existing materials, face shades and Granny atlas UVs.
            result[granny] = BlackMarketShelves::Vertices(granny);
            for (size_t box = 0; box < kBoxes.size(); ++box) {
                for (size_t face = 0; face < BlackMarketShelves::kFaces.size(); ++face) {
                    const auto material = BlackMarketShelves::FaceMaterial(box, face, granny);
                    const auto size = BlackMarketShelves::TextureSize(material);
                    const auto& b = kBoxes[box];
                    const int width = face == 2 || face == 3 ? b.high.z - b.low.z : b.high.x - b.low.x;
                    const int height = face >= 4 ? b.high.z - b.low.z : b.high.y - b.low.y;
                    const int density = material == BlackMarketShelves::Material::Veil ? 100 : 48;
                    const int right = width * 256 / density, bottom = height * 256 / density;
                    const int uv[4][2]{ { 0, 0 }, { right, 0 }, { right, bottom }, { 0, bottom } };
                    for (int i = 0; i < 4; ++i) {
                        auto& vertex = result[granny][box * 24 + face * 4 + i];
                        vertex.position = corners[box][BlackMarketShelves::kFaces[face][i]];
                        if (!granny) {
                            vertex.s = int16_t(uv[i][0] * size[0] / 8);
                            vertex.t = int16_t(uv[i][1] * size[1] / 8);
                        }
                    }
                }
            }
        }
        return result;
    }();
    return variants[granny ? 1 : 0];
}
} // namespace BlackMarketMmShopLayout
