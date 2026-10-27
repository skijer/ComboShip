// Use real Fast::Texture ownership/destruction and the unchanged production copy
// function. The optional atlas is the exact POC4 checkpoint RGBA payload.
#include <fast/resource/type/Texture.h>
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <limits>
#include <memory>
#include <map>
#include <string>
#include <vector>

namespace Ship {
struct ResourceManager {
    std::map<std::string, std::shared_ptr<IResource>> resources;
    unsigned loads = 0;
    std::shared_ptr<IResource> LoadResourceProcess(const char* path) {
        ++loads;
        if (std::strncmp(path, "__OTR__", 7) == 0)
            path += 7;
        const auto found = resources.find(path);
        return found == resources.end() ? nullptr : found->second;
    }
};
struct Context {
    std::shared_ptr<ResourceManager> active;
    static Context* GetRawInstance() {
        static Context context;
        return &context;
    }
    std::shared_ptr<ResourceManager> GetResourceManager() {
        return active;
    }
};
struct CrossRMRegistry {
    static inline std::map<std::string, std::shared_ptr<ResourceManager>> managers;
    static std::shared_ptr<ResourceManager> Get(const std::string& owner) {
        const auto found = managers.find(owner);
        return found == managers.end() ? nullptr : found->second;
    }
};
} // namespace Ship
std::shared_ptr<Ship::IResource> GetResourceByName(const char* path) {
    return Ship::Context::GetRawInstance()->GetResourceManager()->LoadResourceProcess(path);
}
#include "autumn_surface_texture_production.inc"

static std::shared_ptr<Fast::Texture> Texture(const std::vector<uint8_t>& pixels, uint16_t width, uint16_t height) {
    auto metadata = std::make_shared<Ship::ResourceInitData>();
    metadata->Path = "scenes/nonmq/Z2_00KEIKOKU/mm3d_field_poc4/textures/tl_ki_02";
    auto texture = std::make_shared<Fast::Texture>(metadata);
    texture->Type = Fast::TextureType::RGBA32bpp;
    texture->Width = width;
    texture->Height = height;
    texture->Flags = TEX_FLAG_LOAD_AS_IMG;
    texture->HByteScale = 2;
    texture->VPixelScale = 4;
    texture->ImageDataSize = pixels.size();
    texture->ImageData = new uint8_t[pixels.size()];
    std::memcpy(texture->ImageData, pixels.data(), pixels.size());
    return texture;
}

static void CheckMetadata(const Fast::Texture& source, const Fast::Texture& copy) {
    assert(copy.ImageData != source.ImageData && copy.mImageBuffer && !source.mImageBuffer);
    assert(copy.Width == source.Width && copy.Height == source.Height && copy.Type == source.Type &&
           copy.Flags == source.Flags && copy.ImageDataSize == source.ImageDataSize &&
           copy.HByteScale == source.HByteScale && copy.VPixelScale == source.VPixelScale);
}

static void CheckVisibleCenter() {
    constexpr const char* path = "__OTR__@oot:textures/icon_item_custom/gItemIconSeasonSpringTex";
    constexpr const char* key = "textures/icon_item_custom/gItemIconSeasonSpringTex";
    auto mm = std::make_shared<Ship::ResourceManager>();
    auto oot = std::make_shared<Ship::ResourceManager>();
    Ship::Context::GetRawInstance()->active = mm;
    Ship::CrossRMRegistry::managers = { { "mm", mm }, { "oot", oot } };
    mm->resources[key] = Texture({ 255, 255, 255, 0, 255, 255, 255, 255 }, 2, 1);
    for (const unsigned scale : { 1u, 4u }) {
        const unsigned width = 8 * scale, height = 4 * scale;
        std::vector<uint8_t> pixels(width * height * 4, 0);
        for (unsigned y = scale; y < 3 * scale; ++y) {
            for (unsigned x = 0; x < 4 * scale; ++x) {
                pixels[(y * width + x) * 4 + 3] = 255;
            }
        }
        pixels.back() = 31; // A faint glow cannot move the solid artwork's center.
        auto texture = Texture(pixels, width, height);
        oot->resources[key] = texture;
        float x = -1, y = -1;
        assert(MM_GetTextureVisibleCenter(path, &x, &y) && x == 0.25f && y == 0.5f);
        assert(std::equal(pixels.begin(), pixels.end(), texture->ImageData));
        for (const unsigned invalid : { 0u, 1u, 2u, 3u, 4u }) {
            auto bad = Texture(pixels, width, height);
            if (invalid == 0)
                bad->Type = Fast::TextureType::RGBA16bpp;
            if (invalid == 1)
                bad->ImageDataSize -= 1;
            if (invalid == 2)
                bad->Width = 0;
            if (invalid == 3)
                bad->Width = bad->Height = 65535;
            if (invalid == 4)
                std::memset(bad->ImageData, 0, bad->ImageDataSize);
            oot->resources[key] = bad;
            x = y = -1;
            assert(!MM_GetTextureVisibleCenter(path, &x, &y) && x == -1 && y == -1);
        }
    }
    assert(mm->loads == 0 && Ship::Context::GetRawInstance()->active == mm);
    oot->resources.clear();
    float x = -1, y = -1;
    assert(!MM_GetTextureVisibleCenter(path, &x, &y));
    assert(!MM_GetTextureVisibleCenter(nullptr, &x, &y));
    assert(!MM_GetTextureVisibleCenter(path, nullptr, &y));
    assert(!MM_GetTextureVisibleCenter(path, &x, nullptr));
    for (const char* invalid : { "__OTR__@missing:textures/icon_item_custom/gItemIconSeasonSpringTex", "__OTR__@oot",
                                 "__OTR__@:textures/icon_item_custom/gItemIconSeasonSpringTex" }) {
        assert(!MM_GetTextureVisibleCenter(invalid, &x, &y));
    }
    Ship::CrossRMRegistry::managers.erase("oot");
    assert(!MM_GetTextureVisibleCenter(path, &x, &y) && mm->loads == 0);
    assert(MM_GetTextureVisibleCenter("__OTR__textures/icon_item_custom/gItemIconSeasonSpringTex", &x, &y) &&
           x == 0.75f && y == 0.5f);
    Ship::Context::GetRawInstance()->active.reset();
    assert(!MM_GetTextureVisibleCenter("__OTR__textures/icon_item_custom/gItemIconSeasonSpringTex", &x, &y));
    Ship::CrossRMRegistry::managers.clear();
    std::puts(
        "PASS native/HD visible icon centers, exact cross-game routing, untouched pixels and malformed/empty guards");
}

static void CheckIndexedVineOwnership() {
    auto source = Texture({ 1, 2 }, 2, 1);
    source->Type = Fast::TextureType::Palette8bpp;
    source->Flags = 0;
    source->HByteScale = source->VPixelScale = 1;
    std::vector<uint8_t> colors(512, 0);
    const uint16_t leaf = (5 << 11) | (18 << 6) | (5 << 1) | 1;
    const uint16_t wood = (9 << 11) | (6 << 6) | (3 << 1);
    colors[2] = leaf >> 8;
    colors[3] = leaf & 255;
    colors[4] = wood >> 8;
    colors[5] = wood & 255;
    auto palette = Texture(colors, 16, 16);
    palette->Type = Fast::TextureType::RGBA16bpp;
    palette->Flags = 0;
    auto decoded = ExpandIndexedSurface(source, palette);
    assert(decoded && decoded->mImageBuffer && decoded->Type == Fast::TextureType::RGBA32bpp &&
           decoded->Flags == TEX_FLAG_LOAD_AS_RAW && decoded->HByteScale == 4 && decoded->VPixelScale == 1 &&
           decoded->Width == 2 && decoded->Height == 1 && decoded->GetInitData() == source->GetInitData());
    const std::vector<uint8_t> expected{ 41, 148, 41, 255, 74, 49, 24, 0 };
    assert(std::equal(expected.begin(), expected.end(), decoded->ImageData));
    auto tinted = BuildSurfaceTexture(decoded, 0xDCECF5FF, 1.8f);
    assert(tinted && tinted->mImageBuffer && std::memcmp(tinted->ImageData, decoded->ImageData, 3) != 0);
    assert(tinted->ImageData[3] == 255 && std::memcmp(tinted->ImageData + 4, decoded->ImageData + 4, 4) == 0);
    decoded.reset(); // The recolored copy must retain its own expanded bytes.
    assert(tinted->ImageData[7] == 0 && source->ImageData[0] == 1 && source->ImageData[1] == 2 &&
           std::equal(colors.begin(), colors.end(), palette->ImageData));
    palette->ImageDataSize = 511;
    assert(ExpandIndexedSurface(source, palette) == nullptr);
    palette->ImageDataSize = 512;
    source->ImageDataSize = 1;
    assert(ExpandIndexedSurface(source, palette) == nullptr);
    source->ImageDataSize = 2;
    palette->Flags = TEX_FLAG_LOAD_AS_RAW;
    assert(ExpandIndexedSurface(source, palette) == nullptr);
    puts("PASS real indexed-vine ownership, exact decoded wood/alpha, shared palette and malformed input guards");
}

int main(int argc, char** argv) {
    CheckIndexedVineOwnership();
    CheckVisibleCenter();
    const std::vector<uint8_t> original = { 30, 90, 20, 128, 70, 50, 30, 255, 64, 64, 64, 0 };
    auto source = Texture(original, 3, 1);
    auto copy = BuildSurfaceTexture(source);
    assert(copy != nullptr);
    const std::vector<uint8_t> expected = { 71, 44, 23, 128, 70, 50, 30, 255, 64, 64, 64, 0 };
    assert(std::equal(expected.begin(), expected.end(), copy->ImageData));
    assert(std::equal(original.begin(), original.end(), source->ImageData));
    CheckMetadata(*source, *copy);
    assert(copy->GetInitData() == source->GetInitData());
    source.reset(); // The copy owns its buffer independently of the original.
    assert(std::equal(expected.begin(), expected.end(), copy->ImageData));
    auto winterSource = Texture(original, 3, 1);
    auto winterCopy = BuildSurfaceTexture(winterSource, 0xBED2C2B0);
    const std::vector<uint8_t> winterExpected = { 39, 61, 37, 128, 70, 50, 30, 255, 64, 64, 64, 0 };
    assert(winterCopy != nullptr && winterCopy->ImageData != copy->ImageData);
    assert(std::equal(winterExpected.begin(), winterExpected.end(), winterCopy->ImageData));
    assert(std::equal(original.begin(), original.end(), winterSource->ImageData));
    CheckMetadata(*winterSource, *winterCopy);
    auto frostCopy = BuildSurfaceTexture(winterSource, 0xDCECF5FF, 1.8f);
    const std::vector<uint8_t> frostExpected = { 79, 84, 88, 128, 70, 50, 30, 255, 64, 64, 64, 0 };
    assert(frostCopy != nullptr && std::equal(frostExpected.begin(), frostExpected.end(), frostCopy->ImageData));
    assert(std::equal(original.begin(), original.end(), winterSource->ImageData));
    CheckMetadata(*winterSource, *frostCopy);
    assert(BuildSurfaceTexture(winterSource, 0x000000FF) == nullptr);

    for (unsigned invalid = 0; invalid < 5; ++invalid) {
        auto texture = Texture(original, 3, 1);
        if (invalid == 0)
            texture->Type = Fast::TextureType::RGBA16bpp;
        if (invalid == 1)
            texture->Width = 0;
        if (invalid == 2)
            texture->ImageDataSize -= 1;
        if (invalid == 3)
            texture->Width = texture->Height = 65535;
        if (invalid == 4) {
            delete[] texture->ImageData;
            texture->ImageData = nullptr;
        }
        assert(BuildSurfaceTexture(texture) == nullptr);
    }
    assert(BuildSurfaceTexture(Texture({ 80, 60, 30, 255 }, 1, 1)) == nullptr);
    std::puts("PASS real Fast::Texture grass-only copy, bark/alpha/metadata, ownership and invalid-input guards");

    if (argc > 1) {
        std::ifstream file(argv[1], std::ios::binary);
        assert(file.good());
        const std::vector<uint8_t> pixels((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        assert(pixels.size() == 1024u * 2048u * 4);
        source = Texture(pixels, 1024, 2048);
        copy = BuildSurfaceTexture(source);
        assert(copy != nullptr);
        CheckMetadata(*source, *copy);
        assert(std::equal(pixels.begin(), pixels.end(), source->ImageData));
        for (size_t i = 0; i < pixels.size(); i += 4) {
            assert(copy->ImageData[i + 3] == pixels[i + 3]);
            if (pixels[i + 1] <= pixels[i] || pixels[i + 1] <= pixels[i + 2]) {
                assert(std::memcmp(copy->ImageData + i, pixels.data() + i, 4) == 0);
            } else {
                const unsigned originalSum = unsigned(pixels[i]) + pixels[i + 1] + pixels[i + 2];
                const unsigned tintedSum =
                    unsigned(copy->ImageData[i]) + copy->ImageData[i + 1] + copy->ImageData[i + 2];
                if (originalSum <= 496) {
                    assert(tintedSum <= originalSum && tintedSum + 2 >= originalSum);
                }
            }
        }
        const auto sample = [&](unsigned x, unsigned y, const std::vector<uint8_t>& expectedPixel) {
            const auto offset = (y * 1024u + x) * 4;
            assert(std::equal(expectedPixel.begin(), expectedPixel.end(), copy->ImageData + offset));
        };
        // Fixed samples independently read from the checkpoint image: four
        // grass regions become copper, and both exposed wood regions stay exact.
        sample(25, 50, { 130, 81, 42, 255 });
        sample(512, 920, { 167, 104, 54, 255 });
        sample(100, 1460, { 111, 69, 36, 255 });
        sample(500, 1950, { 96, 60, 31, 255 });
        sample(500, 500, { 103, 72, 51, 255 });
        sample(200, 700, { 105, 84, 45, 255 });
        winterCopy = BuildSurfaceTexture(source, 0xBED2C2B0);
        assert(winterCopy != nullptr && winterCopy->ImageData != copy->ImageData);
        CheckMetadata(*source, *winterCopy);
        for (size_t i = 0; i < pixels.size(); i += 4) {
            assert(winterCopy->ImageData[i + 3] == pixels[i + 3]);
            if (pixels[i + 1] <= pixels[i] || pixels[i + 1] <= pixels[i + 2]) {
                assert(std::memcmp(winterCopy->ImageData + i, pixels.data() + i, 4) == 0);
            } else {
                const unsigned originalSum = unsigned(pixels[i]) + pixels[i + 1] + pixels[i + 2];
                const unsigned tintedSum =
                    unsigned(winterCopy->ImageData[i]) + winterCopy->ImageData[i + 1] + winterCopy->ImageData[i + 2];
                if (originalSum <= 721)
                    assert(tintedSum <= originalSum && tintedSum + 4 >= originalSum);
            }
        }
        for (const auto& [offset, pixel] :
             { std::pair{ (50u * 1024 + 25) * 4, std::vector<uint8_t>{ 83, 98, 71, 255 } },
               std::pair{ (920u * 1024 + 512) * 4, std::vector<uint8_t>{ 107, 122, 94, 255 } },
               std::pair{ (1460u * 1024 + 100) * 4, std::vector<uint8_t>{ 73, 83, 59, 255 } },
               std::pair{ (1950u * 1024 + 500) * 4, std::vector<uint8_t>{ 62, 76, 48, 255 } } })
            assert(std::equal(pixel.begin(), pixel.end(), winterCopy->ImageData + offset));
        frostCopy = BuildSurfaceTexture(source, 0xDCECF5FF, 1.8f);
        assert(frostCopy != nullptr);
        CheckMetadata(*source, *frostCopy);
        for (size_t i = 0; i < pixels.size(); i += 4) {
            assert(frostCopy->ImageData[i + 3] == pixels[i + 3]);
            if (pixels[i + 1] <= pixels[i] || pixels[i + 1] <= pixels[i + 2]) {
                assert(std::memcmp(frostCopy->ImageData + i, pixels.data() + i, 4) == 0);
            } else {
                const unsigned originalSum = unsigned(pixels[i]) + pixels[i + 1] + pixels[i + 2];
                const unsigned frostedSum =
                    unsigned(frostCopy->ImageData[i]) + frostCopy->ImageData[i + 1] + frostCopy->ImageData[i + 2];
                if (originalSum <= 400) {
                    const unsigned liftedSum = originalSum * 9 / 5;
                    assert(frostedSum <= liftedSum && frostedSum + 3 >= liftedSum);
                }
            }
        }
        for (const auto& [offset, pixel] :
             { std::pair{ (50u * 1024 + 25) * 4, std::vector<uint8_t>{ 143, 153, 159, 255 } },
               std::pair{ (920u * 1024 + 512) * 4, std::vector<uint8_t>{ 184, 197, 205, 255 } },
               std::pair{ (1460u * 1024 + 100) * 4, std::vector<uint8_t>{ 122, 131, 136, 255 } },
               std::pair{ (1950u * 1024 + 500) * 4, std::vector<uint8_t>{ 106, 113, 118, 255 } },
               std::pair{ (500u * 1024 + 500) * 4, std::vector<uint8_t>{ 103, 72, 51, 255 } },
               std::pair{ (700u * 1024 + 200) * 4, std::vector<uint8_t>{ 105, 84, 45, 255 } } })
            assert(std::equal(pixel.begin(), pixel.end(), frostCopy->ImageData + offset));
        assert(std::equal(pixels.begin(), pixels.end(), source->ImageData));
        std::puts("PASS exact 1024x2048 POC4 atlas: four grass regions, preserved mean intensity, every non-green "
                  "pixel and alpha byte, immutable source");
        std::puts("PASS Termina snow-white grass lift across the real atlas; bark/alpha stay byte-exact");
    }
}
