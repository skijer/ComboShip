#pragma once
#include <array>
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <vector>

namespace BlackMarketShelves {
struct Point {
    int16_t x, y, z;
};
struct Box {
    Point low, high;
};
struct Vertex {
    Point position;
    int16_t s, t;
    std::array<uint8_t, 4> color;
};
// The accepted geometry is shared; Granny textures are runtime references only.
inline constexpr char kWoodTexture[] = "alt/scenes/shared/mahouya_scene/sh_maho_ki_01_2CA00";
inline constexpr char kAtlasTexture[] = "alt/scenes/shared/mahouya_scene/sh_maho_obj_01_43580";
inline constexpr char kFlowTexture[] = "__OTR__custom/skiejer_market/texture/arcane_stone";
inline constexpr std::array<uint8_t, 4> kDefaultTint{ { 218, 210, 255, 255 } };
enum class Material { Flow, Wood, Velvet, Trim, Veil };
inline Material FaceMaterial(size_t box, size_t face, bool granny) {
    if (!granny)
        return box % 6 == 2 ? Material::Veil : Material::Flow;
    if (box % 6 == 2 || (box % 6 >= 3 && face == 4))
        return Material::Velvet;
    if (box % 6 >= 3 && face == 0)
        return Material::Trim;
    return Material::Wood;
}
inline std::array<int, 2> TextureSize(Material material) {
    return material == Material::Wood ? std::array<int, 2>{ 32, 64 } : std::array<int, 2>{ 64, 64 };
}
inline std::array<int, 4> TextureRect(Material material) {
    // Coordinates in 1/256 of the original texture. These regions are sampled
    // directly from the user's intact atlas, never cropped into a bundled asset.
    if (material == Material::Velvet)
        return { 98, 66, 188, 84 };
    if (material == Material::Trim)
        return { 0, 0, 96, 88 };
    return { 0, 0, 256, 256 };
}
inline std::array<uint8_t, 4> Tint(bool changed, std::array<uint8_t, 4> value = kDefaultTint) {
    if (!changed)
        return kDefaultTint;
    value[3] = 255;
    return value;
}
inline constexpr std::array<Box, 12> kBoxes = { { { { 14, 38, 81 }, { 18, 141, 84 } },
                                                  { { 88, 38, 81 }, { 92, 141, 84 } },
                                                  { { 18, 42, 81 }, { 88, 141, 84 } },
                                                  { { 14, 38, 76 }, { 92, 42, 133 } },
                                                  { { 14, 89, 76 }, { 92, 93, 133 } },
                                                  { { 14, 141, 76 }, { 92, 145, 133 } },
                                                  { { -92, 38, 81 }, { -88, 141, 84 } },
                                                  { { -18, 38, 81 }, { -14, 141, 84 } },
                                                  { { -88, 42, 81 }, { -18, 141, 84 } },
                                                  { { -92, 38, 76 }, { -14, 42, 133 } },
                                                  { { -92, 89, 76 }, { -14, 93, 133 } },
                                                  { { -92, 141, 76 }, { -14, 145, 133 } } } };
inline constexpr std::array<Box, 2> kCollision = { { { { 14, 0, 76 }, { 92, 145, 133 } },
                                                     { { -92, 0, 76 }, { -14, 145, 133 } } } };
inline Point RoomPoint(Point p) {
    // Native shop banks face the customer at +/-30 degrees. Place this whole
    // fixture around the counter instead of aiming obliquely up a frontal wall.
    const float side = p.x < 0 ? -1.f : 1.f;
    const float x = p.x - side * 52.5f, z = p.z - 110.f;
    return { int16_t(std::lround(side * 60.f + .8660254038f * x - side * .5f * z)), p.y,
             int16_t(std::lround(107.5f + side * .5f * x + .8660254038f * z)) };
}
inline std::array<Point, 8> Corners(const Box& b) {
    std::array<Point, 8> corners{ { { b.low.x, b.low.y, b.low.z },
                                    { b.high.x, b.low.y, b.low.z },
                                    { b.high.x, b.high.y, b.low.z },
                                    { b.low.x, b.high.y, b.low.z },
                                    { b.low.x, b.low.y, b.high.z },
                                    { b.high.x, b.low.y, b.high.z },
                                    { b.high.x, b.high.y, b.high.z },
                                    { b.low.x, b.high.y, b.high.z } } };
    for (auto& corner : corners)
        corner = RoomPoint(corner);
    return corners;
}
inline constexpr std::array<std::array<int, 4>, 6> kFaces = { { { { 4, 5, 6, 7 } },
                                                                { { 1, 0, 3, 2 } },
                                                                { { 5, 1, 2, 6 } },
                                                                { { 0, 4, 7, 3 } },
                                                                { { 7, 6, 2, 3 } },
                                                                { { 0, 1, 5, 4 } } } };
inline const std::vector<Vertex>& Vertices(bool granny = false) {
    static const auto variants = [] {
        std::array<std::vector<Vertex>, 2> variants;
        for (int granny = 0; granny < 2; ++granny) {
            std::vector<Vertex> result;
            constexpr std::array<uint8_t, 6> shades{ { 220, 110, 175, 150, 235, 100 } };
            size_t boxIndex = 0;
            for (const auto& box : kBoxes) {
                const auto corners = Corners(box);
                for (size_t face = 0; face < kFaces.size(); ++face) {
                    auto shade = shades[face];
                    if (boxIndex % 6 == 2)
                        shade = shade * 4 / 5;
                    if (!granny)
                        shade = boxIndex % 6 == 2 ? shade * 3 / 5 : std::min(255, int(shade) * 6 / 5);
                    const std::array<uint8_t, 4> color{ { shade, shade, shade, 255 } };
                    const auto material = FaceMaterial(boxIndex, face, granny);
                    const auto rect = TextureRect(material);
                    const auto size = TextureSize(material);
                    int uv[4][2] = {
                        { rect[0], rect[1] }, { rect[2], rect[1] }, { rect[2], rect[3] }, { rect[0], rect[3] }
                    };
                    if (!granny) {
                        // World-space density prevents a tall post or thin board
                        // from stretching one whole cloud tile into a stripe.
                        const int width = face == 2 || face == 3 ? box.high.z - box.low.z : box.high.x - box.low.x;
                        const int height = face >= 4 ? box.high.z - box.low.z : box.high.y - box.low.y;
                        const int scale = material == Material::Veil ? 100 : 48;
                        const int right = width * 256 / scale, bottom = height * 256 / scale;
                        const int mapped[4][2] = { { 0, 0 }, { right, 0 }, { right, bottom }, { 0, bottom } };
                        for (int i = 0; i < 4; ++i)
                            for (int j = 0; j < 2; ++j)
                                uv[i][j] = mapped[i][j];
                    }
                    for (int i = 0; i < 4; ++i)
                        result.push_back({ corners[kFaces[face][i]], int16_t(uv[i][0] * size[0] / 8),
                                           int16_t(uv[i][1] * size[1] / 8), color });
                }
                ++boxIndex;
            }
            variants[granny] = std::move(result);
        }
        return variants;
    }();
    return variants[granny ? 1 : 0];
}
} // namespace BlackMarketShelves
