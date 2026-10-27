#include "soh/Enhancements/Graphics/SeasonFoliageTint.h"
#include "test_require.h"
#include <iostream>

static std::shared_ptr<Fast::Texture> Make(Fast::TextureType type, unsigned flags,
                                           std::initializer_list<uint8_t> pixels) {
    auto init = std::make_shared<Ship::ResourceInitData>();
    init->Path = "fixture/foliage";
    auto tex = std::make_shared<Fast::Texture>(init);
    tex->Type = type;
    tex->Flags = flags;
    tex->Width = 2;
    tex->Height = 1;
    tex->HByteScale = 8;
    tex->VPixelScale = 4;
    tex->mImageBuffer = std::make_shared<std::vector<char>>(pixels.begin(), pixels.end());
    tex->ImageData = reinterpret_cast<uint8_t*>(tex->mImageBuffer->data());
    tex->ImageDataSize = pixels.size();
    return tex;
}

int main() {
    using namespace SeasonFoliage;
    const auto hd = Make(Fast::TextureType::RGBA16bpp, TEX_FLAG_LOAD_AS_RAW, { 40, 110, 25, 83, 95, 85, 70, 255 });
    const auto before = *hd->mImageBuffer;
    // Independently derived RGBA output pins the confirmed role hues at the texture
    // boundary, including each role's existing exposure and source alpha.
    struct RoleOutput {
        int season;
        Surface surface;
        uint8_t red, green, blue;
    };
    for (const auto role :
         { RoleOutput{ 2, Surface::Ground, 138, 86, 45 }, RoleOutput{ 2, Surface::Canopy, 108, 32, 34 },
           RoleOutput{ 2, Surface::Understory, 101, 43, 29 }, RoleOutput{ 3, Surface::Ground, 169, 182, 189 },
           RoleOutput{ 3, Surface::Canopy, 102, 107, 105 }, RoleOutput{ 3, Surface::Understory, 98, 106, 110 } }) {
        const auto shown = BuildTexture(hd, Palette(role.season, role.surface));
        REQUIRE(shown && shown->ImageData[0] == role.red && shown->ImageData[1] == role.green &&
                shown->ImageData[2] == role.blue && "confirmed seasonal role hue/exposure output differs");
        REQUIRE(shown->ImageData[3] == 83 && *hd->mImageBuffer == before);
        REQUIRE(std::equal(shown->ImageData + 4, shown->ImageData + 8, hd->ImageData + 4));
    }
    const auto autumn = BuildTexture(hd, Palette(2, Surface::Ground));
    REQUIRE(autumn && "Djipi raw RGBA pixels must receive autumn despite native RGBA16 type");
    REQUIRE(autumn->ImageData[0] > autumn->ImageData[1] && autumn->ImageData[1] > autumn->ImageData[2]);
    REQUIRE(autumn->ImageData[3] == 83);
    REQUIRE(std::equal(autumn->ImageData + 4, autumn->ImageData + 8, hd->ImageData + 4));
    REQUIRE(*hd->mImageBuffer == before && "source pixels must remain exact");
    REQUIRE(autumn->GetInitData() == hd->GetInitData() && autumn->Type == hd->Type && autumn->Flags == hd->Flags);
    REQUIRE(autumn->Width == hd->Width && autumn->Height == hd->Height);
    REQUIRE(autumn->HByteScale == hd->HByteScale && autumn->VPixelScale == hd->VPixelScale);
    const auto winter = BuildTexture(hd, Palette(3, Surface::Ground));
    REQUIRE(winter && winter->ImageData[2] > winter->ImageData[0]);
    REQUIRE(unsigned(winter->ImageData[0]) + winter->ImageData[1] + winter->ImageData[2] > 40 + 110 + 25);
    REQUIRE(winter->ImageData[3] == hd->ImageData[3]);
    const auto springGround = BuildTexture(hd, Palette(0, Surface::Ground));
    REQUIRE(springGround && "spring ground grass must receive blush instead of matching summer");
    REQUIRE(springGround->ImageData[0] > springGround->ImageData[2] &&
            springGround->ImageData[2] > springGround->ImageData[1]);
    REQUIRE(springGround->ImageData[3] == 83);
    REQUIRE(std::equal(springGround->ImageData + 4, springGround->ImageData + 8, hd->ImageData + 4));
    REQUIRE(!BuildTexture(hd, Palette(1, Surface::Canopy)) && "summer must restore source");
    const auto canopy = Make(Fast::TextureType::RGBA16bpp, TEX_FLAG_LOAD_AS_RAW, { 60, 110, 70, 83, 95, 85, 70, 255 });
    const auto spring = BuildTexture(canopy, Palette(0, Surface::Canopy));
    REQUIRE(spring && spring->ImageData[0] > spring->ImageData[1] && spring->ImageData[2] > spring->ImageData[1]);
    REQUIRE(unsigned(spring->ImageData[0]) + spring->ImageData[1] + spring->ImageData[2] > (60 + 110 + 70) * 15 / 10);
    const auto blush = BuildTexture(hd, Palette(0, Surface::Understory));
    REQUIRE(blush && blush->ImageData[0] == 118 && blush->ImageData[1] == 92 && blush->ImageData[2] == 104 &&
            "spring understory must show full blush without a residual green blend");
    REQUIRE(blush && blush->ImageData[0] > blush->ImageData[1] && "spring grass must show a readable blush");
    REQUIRE(unsigned(blush->ImageData[0]) + blush->ImageData[1] + blush->ImageData[2] > (40 + 110 + 25) * 14 / 10);
    REQUIRE(blush->ImageData[3] == hd->ImageData[3]);
    // Packed RGBA5551 retains both opaque and transparent alpha bits and rock texels.
    const auto native = Make(Fast::TextureType::RGBA16bpp, 0, { 0x26, 0x87, 0x7B, 0xD0 });
    const auto nativeBefore = *native->mImageBuffer;
    const auto nativeAutumn = BuildTexture(native, Palette(2, Surface::Ground));
    REQUIRE(nativeAutumn && (nativeAutumn->ImageData[1] & 1) == 1);
    REQUIRE(nativeAutumn->ImageData[2] == 0x7B && nativeAutumn->ImageData[3] == 0xD0);
    REQUIRE(*native->mImageBuffer == nativeBefore);
    const auto rgba = Make(Fast::TextureType::RGBA32bpp, 0, { 40, 110, 25, 0, 95, 85, 70, 255 });
    REQUIRE(BuildTexture(rgba, Palette(2, Surface::Ground))->ImageData[3] == 0);
    rgba->ImageDataSize = 7;
    REQUIRE(!BuildTexture(rgba, Palette(2, Surface::Ground)));
    rgba->ImageDataSize = 8;
    rgba->Type = Fast::TextureType::Palette8bpp;
    REQUIRE(!BuildTexture(rgba, Palette(2, Surface::Ground)));
    REQUIRE(!BuildTexture(nullptr, Palette(2, Surface::Ground)));
    std::cout << "PASS native/HD seasonal foliage: spring scope, autumn/winter palettes, source/alpha/rock/metadata "
                 "preservation\n";
}
