#pragma once

#include <fast/resource/type/Texture.h>
#include <algorithm>
#include <cstdint>
#include <memory>
#include <vector>

namespace SeasonFoliage {
enum class Surface { Ground, Canopy, Understory };
enum class PixelMask {
    Green,
    GrassTrim,
    KakarikoCliffGrass,
    MeadowWallGrass,
    MeadowNativeCliffGrass,
    MeadowRuinsGrass,
    CliffGrass,
    RiverAltCliffGrass,
    RanchMasonryGrass,
    RanchAtlasGrass,
    RiverGroundAtlasGrass,
    RiverFrameGrass
};
struct Tint {
    uint32_t color = 0;
    unsigned lightScale = 256;
    unsigned shadowLift = 0; // 0..255: lift dark ground without flattening its texture.
};
// Atlas bounds in sixteenths, independent of native/HD texture dimensions.
// Defaults select the whole texture; authored exceptions protect nearby bark/stone.
struct Region {
    unsigned left = 0, top = 0, right = 16, bottom = 16;
};

inline Tint Palette(int season, Surface surface, uint32_t autumnColor = 0) {
    switch (season) {
        case 0:
            if (surface == Surface::Ground && !autumnColor)
                return { 0xF6C1D9FFu, 461, 24 }; // Gentle lift; retain the accepted spring blush.
            return { surface == Surface::Canopy ? 0xF6C1D9E0u : 0xF6C1D9FFu, 461 };
        case 2:
            if (surface == Surface::Ground && !autumnColor)
                return { 0xF3974FFFu, 320, 24 }; // Copper grass and trim; retain the accepted exposure.
            return { autumnColor ? autumnColor : surface == Surface::Understory ? 0xF26846FFu : 0xEF484CFFu, 256 };
        case 3:
            if (surface == Surface::Ground && !autumnColor)
                return { 0xDCECF5FFu, 461, 128 }; // Pale frost, including the darkest grass shadows.
            return { surface == Surface::Canopy ? 0xE2EEE9FFu : 0xDCECF5FFu, 461 };
        default:
            return {};
    }
}

// Fixed normalized 64-cell masks from inspected River grass/stone atlases.
// Keep their tree, cliff and Triforce-frame interiors in the original artwork.
inline constexpr uint64_t kRiverGroundAtlasGrass[64] = {
    0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL,
    0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL,
    0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL,
    0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL,
    0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL,
    0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL, 0xfc00c01ff8006067ULL,
    0xfc00e017f410e03fULL, 0xfc00c001e400000fULL, 0x000000006000001fULL, 0x000000004000001cULL, 0x0000000000000000ULL,
    0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
    0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
    0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x00000000ffffffffULL, 0x000000007ffffffeULL,
    0x000000003ffffffcULL, 0x000000001ffffffcULL, 0x0000000007ffffb0ULL, 0x0000000007ffefa0ULL, 0x0000000000ffe484ULL,
    0x0000000004ffd800ULL, 0x000000000478b800ULL, 0x0000000001206010ULL, 0x000000001f00051eULL, 0x000000001e00079eULL,
    0x000000003fc81f9eULL, 0x000000002ffa7f5eULL, 0x000000007ffe7ffeULL, 0x00000000ffffffffULL
};
inline constexpr uint64_t kRiverFrameGrass[64] = {
    0xffffffffffffffffULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL,
    0xffffffffffffffffULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL,
    0xffffffffffffffffULL, 0xffffffffffffffffULL, 0xffffff8fffffffffULL, 0xffffe60001ffffffULL, 0xffff00000000ffffULL,
    0xffff000000001fffULL, 0xfffc000000001fffULL, 0xfff8000000001fffULL, 0xfff0000000001fffULL, 0xfff0000000000fffULL,
    0xfff0000000000fffULL, 0xfff0000000000fffULL, 0xfff0000000000fffULL, 0xffe0000000001fffULL, 0xffe0000000000fffULL,
    0xffe0000000000fffULL, 0xffe0000000000fffULL, 0xffe0000000000fffULL, 0xffe0000000000fffULL, 0xfff0000000000fffULL,
    0xfff0000000000fffULL, 0xfff0000000000fffULL, 0xfff0000000000fffULL, 0xfff0000000000fffULL, 0xfff0000000000fffULL,
    0xffe0000000000fffULL, 0xfff0000000000fffULL, 0xfff0000000000fffULL, 0xfff0000000000fffULL, 0xfff0000000000fffULL,
    0xfff0000000000fffULL, 0xfff8000000001fffULL, 0xfff0000000001fffULL, 0xfff0000000001fffULL, 0xfff0000000000fffULL,
    0xfff0000000000fffULL, 0xfff0000000000fffULL, 0xffe0000000001fffULL, 0xffe0000000001fffULL, 0xfff0000000001fffULL,
    0xfff0000000001fffULL, 0xfff8000000007fffULL, 0xfffe00000000ffffULL, 0xffffe0000001ffffULL, 0xfffffffe081fffffULL,
    0xffffffffffffffffULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL,
    0xffffffffffffffffULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL
};

// Tint copies are made from the original texture, never the preceding season.
// Djipi's RGBA16 resources with LOAD_AS_RAW carry four RGBA bytes per pixel.
inline std::shared_ptr<Fast::Texture> BuildTexture(const std::shared_ptr<Fast::Texture>& source, Tint tint,
                                                   Region region = {}, PixelMask pixelMask = PixelMask::Green) {
    if (!source || !tint.color || !source->ImageData || !source->Width || !source->Height)
        return nullptr;
    if (region.left >= region.right || region.top >= region.bottom || region.right > 16 || region.bottom > 16)
        return nullptr;
    const bool packed = source->Type == Fast::TextureType::RGBA16bpp && source->Flags == 0;
    const bool raw = (source->Flags & TEX_FLAG_LOAD_AS_RAW) != 0;
    const bool rgba32 = source->Type == Fast::TextureType::RGBA32bpp && source->Flags == 0;
    if (!packed && !raw && !rgba32)
        return nullptr;
    const size_t stride = packed ? 2 : 4;
    if (size_t(source->Width) * source->Height * stride != source->ImageDataSize)
        return nullptr;
    const unsigned red = tint.color >> 24;
    const unsigned green = (tint.color >> 16) & 255;
    const unsigned blue = (tint.color >> 8) & 255;
    const unsigned blend = tint.color & 255;
    const unsigned targetSum = red + green + blue;
    if (!targetSum || !blend)
        return nullptr;
    auto copy = std::make_shared<Fast::Texture>(source->GetInitData());
    copy->Type = source->Type;
    copy->Flags = source->Flags;
    copy->Width = source->Width;
    copy->Height = source->Height;
    copy->HByteScale = source->HByteScale;
    copy->VPixelScale = source->VPixelScale;
    copy->ImageDataSize = source->ImageDataSize;
    const auto* first = reinterpret_cast<const char*>(source->ImageData);
    copy->mImageBuffer = std::make_shared<std::vector<char>>(first, first + source->ImageDataSize);
    copy->ImageData = reinterpret_cast<uint8_t*>(copy->mImageBuffer->data());
    bool changed = false;
    const bool wholeTexture = region.left == 0 && region.top == 0 && region.right == 16 && region.bottom == 16;
    for (size_t i = 0; i < copy->ImageDataSize; i += stride) {
        // Kakariko's cliff atlas has grass at both edges. Retain its rock face
        // between the top eighth and bottom sixteenth, including greenish moss.
        if (pixelMask == PixelMask::KakarikoCliffGrass) {
            const size_t y = (i / stride) / source->Width;
            if (y * 16 >= source->Height * 2u && y * 16 < source->Height * 15u)
                continue;
        }
        // Actual exported mixed atlases have bounded foliage. Pixel origins
        // use normalized regions so replacement dimensions do not widen them.
        const auto inside = [&](Region r) {
            const size_t x = (i / stride) % source->Width, y = (i / stride) / source->Width;
            return x * 16 >= source->Width * r.left && x * 16 < source->Width * r.right &&
                   y * 16 >= source->Height * r.top && y * 16 < source->Height * r.bottom;
        };
        if (pixelMask == PixelMask::MeadowWallGrass && !(inside({ 0, 0, 16, 4 }) || inside({ 0, 14, 16, 16 })))
            continue;
        if (pixelMask == PixelMask::MeadowNativeCliffGrass && !(inside({ 0, 0, 16, 4 }) || inside({ 0, 13, 16, 16 })))
            continue;
        if (pixelMask == PixelMask::MeadowRuinsGrass && !(inside({ 0, 0, 8, 4 }) || inside({ 0, 15, 8, 16 })))
            continue;
        if (pixelMask == PixelMask::CliffGrass && !(inside({ 0, 0, 16, 3 }) || inside({ 0, 14, 16, 16 })))
            continue;
        if (pixelMask == PixelMask::RiverAltCliffGrass && !(inside({ 0, 0, 16, 3 }) || inside({ 0, 15, 16, 16 })))
            continue;
        if (pixelMask == PixelMask::RanchMasonryGrass && !(inside({ 0, 0, 8, 1 }) || inside({ 7, 15, 16, 16 })))
            continue;
        if (pixelMask == PixelMask::RanchAtlasGrass &&
            !(inside({ 0, 7, 2, 8 }) || inside({ 4, 7, 6, 8 }) || inside({ 0, 15, 2, 16 }) ||
              inside({ 8, 15, 11, 16 }) || inside({ 12, 0, 14, 16 }) || inside({ 14, 15, 16, 16 })))
            continue;
        const uint64_t* contour = pixelMask == PixelMask::RiverGroundAtlasGrass ? kRiverGroundAtlasGrass
                                  : pixelMask == PixelMask::RiverFrameGrass     ? kRiverFrameGrass
                                                                                : nullptr;
        if (contour) {
            const size_t x = (i / stride) % source->Width, y = (i / stride) / source->Width;
            if (!(contour[y * 64 / source->Height] & (uint64_t{ 1 } << (x * 64 / source->Width))))
                continue;
        }
        if (!wholeTexture) {
            const size_t pixelIndex = i / stride;
            const unsigned x = pixelIndex % source->Width, y = pixelIndex / source->Width;
            if (x * 16 < source->Width * region.left || x * 16 >= source->Width * region.right ||
                y * 16 < source->Height * region.top || y * 16 >= source->Height * region.bottom)
                continue;
        }
        auto* pixel = copy->ImageData + i;
        const unsigned native = packed ? (unsigned(pixel[0]) << 8) | pixel[1] : 0;
        const unsigned r = packed ? native >> 11 : pixel[0];
        const unsigned g = packed ? (native >> 6) & 31 : pixel[1];
        const unsigned b = packed ? (native >> 1) & 31 : pixel[2];
        if (g <= r || g <= b)
            continue; // Rocks, bark, flowers and alpha retain their exact source pixels.
        // The Field paving's gray stone is slightly green too. Select its
        // saturated seam grass without recoloring those stone faces. Match the
        // same separation in native 5-bit and raw/HD 8-bit channels.
        if (pixelMask == PixelMask::GrassTrim &&
            (g * 100 <= r * 120 || g * 100 <= b * 120 || g - r < (packed ? 2u : 10u)))
            continue;
        const unsigned maximum = packed ? 31u : 255u;
        unsigned light = (r + g + b) * tint.lightScale;
        if (tint.shadowLift) {
            const unsigned white = 3 * maximum * 256;
            const unsigned lift = std::min(255u, tint.shadowLift);
            // Compress toward white smoothly instead of clamping shadows to a
            // flat floor. Zero lift preserves the original arithmetic exactly.
            light = (std::min(white, light) * (255 - lift) + white * lift) / 255;
        }
        const auto channel = [&](unsigned original, unsigned target) {
            const unsigned normalized = std::min(maximum, light * target / (targetSum * 256));
            return (original * (255 - blend) + normalized * blend) / 255;
        };
        const unsigned tr = channel(r, red), tg = channel(g, green), tb = channel(b, blue);
        if (packed) {
            const unsigned tinted = (tr << 11) | (tg << 6) | (tb << 1) | (native & 1);
            pixel[0] = tinted >> 8;
            pixel[1] = tinted & 255;
        } else {
            pixel[0] = tr;
            pixel[1] = tg;
            pixel[2] = tb;
        }
        changed |= tr != r || tg != g || tb != b;
    }
    return changed ? copy : nullptr;
}
} // namespace SeasonFoliage
