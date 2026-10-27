#include "soh/Enhancements/Graphics/SeasonFoliageTint.h"
#include "test_require.h"
#include <iostream>

using namespace SeasonFoliage;

static std::shared_ptr<Fast::Texture> Make(std::vector<char> bytes, unsigned width = 1, unsigned height = 1) {
    auto init = std::make_shared<Ship::ResourceInitData>();
    init->Path = "fixture/ground-tint-polish";
    auto image = std::make_shared<Fast::Texture>(init);
    image->Type = Fast::TextureType::RGBA32bpp;
    image->Flags = 0;
    image->Width = width;
    image->Height = height;
    image->ImageDataSize = bytes.size();
    image->mImageBuffer = std::make_shared<std::vector<char>>(std::move(bytes));
    image->ImageData = reinterpret_cast<uint8_t*>(image->mImageBuffer->data());
    return image;
}

static std::shared_ptr<Fast::Texture> Pixel(unsigned r, unsigned g, unsigned b) {
    return Make({ char(r), char(g), char(b), char(83) });
}

static unsigned Mean(const std::shared_ptr<Fast::Texture>& image) {
    return (unsigned(image->ImageData[0]) + image->ImageData[1] + image->ImageData[2]) / 3;
}

int main() {
    // Representative dark Kokiri grass: before this polish its Winter mean is
    // only 57/255, and Autumn's is 31/255, before scene lighting.
    auto dark = Pixel(40, 54, 3);
    const auto autumn = BuildTexture(dark, Palette(2, Surface::Ground));
    REQUIRE(autumn && Mean(autumn) >= 55 && Mean(autumn) <= 90);
    REQUIRE(autumn->ImageData[0] > autumn->ImageData[1] && autumn->ImageData[1] > autumn->ImageData[2]);
    const auto winter = BuildTexture(dark, Palette(3, Surface::Ground));
    REQUIRE(winter && Mean(winter) >= 145 && Mean(winter) <= 180);
    REQUIRE(winter->ImageData[2] > winter->ImageData[0]);
    const auto spring = BuildTexture(dark, Palette(0, Surface::Ground));
    REQUIRE(spring && Mean(spring) >= 70 && Mean(spring) <= 95);
    REQUIRE(spring->ImageData[0] > spring->ImageData[2] && spring->ImageData[2] > spring->ImageData[1]);
    for (int season : { 0, 2, 3 }) {
        unsigned previous = 0;
        for (unsigned g = 8; g <= 240; g += 8) {
            auto input = Pixel(g / 2, g, g / 4);
            const auto output = BuildTexture(input, Palette(season, Surface::Ground));
            REQUIRE(output && Mean(output) > previous && "ground detail must not collapse to a flat shadow floor");
            previous = Mean(output);
            REQUIRE(output->ImageData[3] == 83);
            REQUIRE(input->ImageData[0] == g / 2 && input->ImageData[1] == g);
        }
    }
    for (int season : { 0, 2, 3 }) {
        for (uint32_t override : { 0xF3974FFFu, 0xEF484CFFu }) {
            const uint32_t color = season == 0 ? 0xF6C1D9FFu : season == 2 ? override : 0xDCECF5FFu;
            const auto expected = BuildTexture(dark, { color, season == 2 ? 256u : 461u });
            const auto actual = BuildTexture(dark, Palette(season, Surface::Ground, override));
            REQUIRE(expected && actual && *expected->mImageBuffer == *actual->mImageBuffer);
        }
    }
    // Literal role hues pin seasonal output while retaining the accepted exposure
    // and blend alpha over all sampled RGB inputs.
    std::vector<char> samples;
    for (unsigned r = 0; r < 256; r += 16)
        for (unsigned g = 0; g < 256; g += 16)
            for (unsigned b = 0; b < 256; b += 16)
                samples.insert(samples.end(), { char(r), char(g), char(b), char((r + g + b) % 256) });
    auto sampled = Make(samples, 64, 64);
    for (Surface surface : { Surface::Canopy, Surface::Understory }) {
        const bool canopy = surface == Surface::Canopy;
        for (int season : { 0, 2, 3 }) {
            const uint32_t color = season == 0   ? (canopy ? 0xF6C1D9E0u : 0xF6C1D9FFu)
                                   : season == 2 ? (canopy ? 0xEF484CFFu : 0xF26846FFu)
                                                 : (canopy ? 0xE2EEE9FFu : 0xDCECF5FFu);
            auto expected = BuildTexture(sampled, { color, season == 2 ? 256u : 461u });
            auto actual = BuildTexture(sampled, Palette(season, surface));
            REQUIRE(expected && actual && *expected->mImageBuffer == *actual->mImageBuffer);
        }
    }
    REQUIRE(!BuildTexture(dark, Palette(1, Surface::Ground)));
    REQUIRE(!BuildTexture(dark, Palette(-1, Surface::Ground)));
    // All 65,536 packed colors retain the alpha bit and every non-green texel.
    std::vector<char> packed;
    for (unsigned value = 0; value < 65536; ++value) {
        packed.push_back(char(value >> 8));
        packed.push_back(char(value));
    }
    auto native = Make(packed, 256, 256);
    native->Type = Fast::TextureType::RGBA16bpp;
    for (int season : { 0, 2, 3 }) {
        auto output = BuildTexture(native, Palette(season, Surface::Ground));
        REQUIRE(output && *native->mImageBuffer == packed);
        for (unsigned value = 0; value < 65536; ++value) {
            const unsigned at = value * 2;
            REQUIRE((output->ImageData[at + 1] & 1) == (value & 1));
            const unsigned r = value >> 11, g = (value >> 6) & 31, b = (value >> 1) & 31;
            if (g <= r || g <= b)
                REQUIRE(output->ImageData[at] == (value >> 8) && output->ImageData[at + 1] == (value & 255));
        }
    }
    std::cout << "PASS native OoT ground polish: copper/glacial/blush, dark detail, private-forest overrides, "
                 "24576 non-ground palette samples, all 65536 packed colors, source/alpha preservation\n";
}
