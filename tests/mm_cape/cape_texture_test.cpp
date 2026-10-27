// Reuse the existing production SETTIMG test environment and baseline controls.
#define main ForeignTextureBaselineMain
#include "../mm_presentation/foreign_texture_test.cpp"
#undef main
#include <stdexcept>

extern "C" int CapeProbe_Draw(uint8_t, uint8_t, void*, uint32_t, uintptr_t[2], uint32_t*);

namespace Fast {
static void Require(bool condition, const char* reason) {
    if (!condition)
        throw std::runtime_error(reason);
}

static auto MantTexture(uint16_t width, uint16_t height, TextureType type,
                         uint32_t flags, float horizontal, float vertical) {
    auto texture = std::make_shared<Texture>();
    texture->Width = width;
    texture->Height = height;
    texture->Type = type;
    texture->Flags = flags;
    texture->HByteScale = horizontal;
    texture->VPixelScale = vertical;
    texture->ImageDataSize = width * height * (type == TextureType::RGBA16bpp ? 2 : 4);
    texture->ImageData = new uint8_t[texture->ImageDataSize]{};
    return texture;
}

static void SubmitCape(const std::shared_ptr<Texture>& texture, uint8_t alpha) {
    uintptr_t words[2]{};
    uint32_t triangles = 0;
    Require(CapeProbe_Draw(alpha, 1, texture->ImageData, 0, words, &triangles) == 1,
            "cape must submit exactly one texture binding");
    Require(triangles == 242, "cape must retain all 242 cloth triangles");
    Require((words[1] & 1) == 0, "cape texture path must be aligned for signature detection");
    F3DGfx command{};
    command.words.w0 = words[0];
    command.words.w1 = words[1];
    auto* cursor = &command;
    auto sets = instance->sets;
    gfx_set_timg_handler_rdp(instance.get(), &cursor);
    Require(instance->sets == sets + 1, "cape texture must reach renderer state");
    Require(instance->lastPixels == texture->ImageData, "cape must select current resource pixels");
    Require(instance->lastMetadata.resource == texture,
            "cape binding discarded texture resource metadata (HD pixels would decode as native RGBA16)");
    Require(instance->lastMetadata.width == texture->Width && instance->lastMetadata.height == texture->Height,
            "cape binding must preserve replacement dimensions");
    Require(instance->lastMetadata.type == texture->Type && instance->lastFlags == texture->Flags,
            "cape binding must preserve texture format and raw-image flags");
    Require(instance->lastMetadata.h_byte_scale == texture->HByteScale &&
            instance->lastMetadata.v_pixel_scale == texture->VPixelScale,
            "cape binding must preserve both replacement scale factors");
}

static void RunCapeTests() {
    const char* path = "__OTR__overlays/ovl_En_Ganon_Mant/gMantTex";
    auto hd = MantTexture(512, 1024, TextureType::RGBA32bpp, TEX_FLAG_LOAD_AS_RAW, 32, 16);
    auto native = MantTexture(32, 64, TextureType::RGBA16bpp, 0, 1, 1);
    auto rgba32 = MantTexture(32, 64, TextureType::RGBA32bpp, TEX_FLAG_LOAD_AS_RAW, 2, 1);
    auto scaled16 = MantTexture(64, 128, TextureType::RGBA16bpp, 0, 2, 2);
    for (auto texture : {hd, native, rgba32, scaled16}) {
        host->textures[path] = texture;
        SubmitCape(texture, 255);
        SubmitCape(texture, 128);
    }
    // Missing assets must omit cloth and recover when they become available.
    uintptr_t words[2]{};
    uint32_t triangles = 0;
    Require(CapeProbe_Draw(255, 1, nullptr, 0, words, &triangles) == 0 && triangles == 0,
            "an existing but unloadable cape texture must not draw stale pixels");
    host->textures.erase(path);
    Require(CapeProbe_Draw(255, 0, nullptr, 0, words, &triangles) == 0 && triangles == 0,
            "missing cape texture must not draw using stale cached pixels");
    host->textures[path] = hd;
    SubmitCape(hd, 255);
    // Native horse suppression is unchanged.
    Require(CapeProbe_Draw(255, 1, hd->ImageData, 0x00800000u, words, &triangles) == 0 && triangles == 0,
            "cape must remain suppressed while riding Epona");
    std::cout << "PASS full production cape draw: HD/native RGBA16/RGBA32 metadata, OPA/XLU, live resource changes, "
                 "missing/recovered assets, horse guard and all 242 cloth triangles\n";
}
} // namespace Fast

int main() {
    try {
        if (ForeignTextureBaselineMain() != 0)
            return 1;
        Fast::RunCapeTests();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
