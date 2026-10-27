// Execute the production native load-block and raw-image upload paths. GPU
// submission records bytes; native CI4/CI8 GBI format/stride remains unchanged.
namespace RoadVineUploadFixture {
struct Metadata {
    unsigned width = 0, height = 0;
    float h_byte_scale = 1, v_pixel_scale = 1;
    Fast::TextureType type{};
    std::shared_ptr<Fast::Texture> resource;
};
struct State {
    struct {
        const uint8_t* addr;
        unsigned siz, width, tex_flags;
        Metadata raw_tex_metadata;
    } texture_to_load{};
    struct {
        unsigned tmem_index = 0, line_size_bytes = 0, siz = 0;
    } texture_tile[8];
    struct {
        unsigned orig_size_bytes = 0, size_bytes = 0, full_image_line_size_bytes = 0, line_size_bytes = 0;
        const uint8_t* addr = nullptr;
        unsigned tex_flags = 0;
        Metadata raw_tex_metadata;
        bool masked = false, blended = false;
    } loaded_texture[2];
    bool textures_changed[2]{};
};
struct Api {
    int GetMaxTextureSize() {
        return 4096;
    }
};
static std::string_view GetBaseTexturePath(const std::string& path) {
    return path;
}
struct Interpreter {
    State state;
    State* mRdp = &state;
    Api api;
    Api* mRapi = &api;
    struct Mask {
        const uint8_t* replacementData;
    };
    std::map<std::string_view, Mask> mMaskedTextures;
    uint8_t mTexUploadBuffer[8192]{};
    unsigned uploadedWidth = 0, uploadedHeight = 0;
    std::vector<uint8_t> uploaded;
    void GfxDpLoadBlock(uint8_t, uint32_t, uint32_t, uint32_t, uint32_t);
    void ImportTextureRaw(int, bool);
    void ImportTextureCi4(int, bool) {
        assert(false && "Converted vines must bypass CI4 decoding");
    }
    void ImportTextureCi8(int, bool) {
        assert(false && "Converted vines must bypass the shared CI8 palette");
    }
    void UploadTextureMeasured(const uint8_t* rgba, uint32_t width, uint32_t height) {
        uploadedWidth = width;
        uploadedHeight = height;
        uploaded.assign(rgba, rgba + size_t(width) * height * 4);
    }
};
using RawTexMetadata = Metadata;
#define SUPPORT_CHECK(condition) assert(condition)
#include "autumn_road_vine_upload_production.inc"
#undef SUPPORT_CHECK

static void CheckIndexedGroundUpload(const std::shared_ptr<Fast::Texture>& image, bool ci4) {
    Interpreter renderer;
    const unsigned rowBytes = image->Width / (ci4 ? 2 : 1);
    const unsigned renderLine = ((rowBytes + 7) / 8) * 8;
    const unsigned loadPixels = image->Width * image->Height / (ci4 ? 4 : 2) - 1;
    renderer.state.texture_to_load = { image->ImageData,
                                       G_IM_SIZ_16b,
                                       1,
                                       image->Flags,
                                       { image->Width, image->Height, image->HByteScale, image->VPixelScale,
                                         image->Type, image } };
    renderer.state.texture_tile[0].line_size_bytes = renderLine;
    renderer.state.texture_tile[0].siz = ci4 ? G_IM_SIZ_4b : G_IM_SIZ_8b;
    renderer.GfxDpLoadBlock(7, 0, 0, loadPixels, 0);
    renderer.ImportTextureRaw(0, false);
    assert(renderer.uploadedWidth == image->Width && renderer.uploadedHeight == image->Height);
    assert(renderer.uploaded.size() == image->ImageDataSize &&
           std::equal(renderer.uploaded.begin(), renderer.uploaded.end(), image->ImageData));
}

static void CheckUpload(const std::shared_ptr<Fast::Texture>& image, const std::vector<Gfx>& native) {
    Interpreter renderer;
    unsigned imageSize = 0, loadPixels = 0, loadDxt = 0, renderLine = 0, renderSize = 0;
    bool afterPalette = false;
    for (size_t i = 0; i < native.size(); ++i) {
        const auto& cmd = native[i];
        const auto op = cmd.words.w0 >> 24;
        if (op == G_SETTIMG_OTR_HASH) {
            afterPalette = (cmd.words.w0 >> 21 & 7) == G_IM_FMT_CI;
            if (afterPalette)
                imageSize = cmd.words.w0 >> 19 & 3;
        } else if (afterPalette && op == G_LOADBLOCK) {
            loadPixels = cmd.words.w1 >> 12 & 4095;
            loadDxt = cmd.words.w1 & 4095;
        } else if (afterPalette && op == G_SETTILE && (cmd.words.w1 >> 24 & 7) == G_TX_RENDERTILE) {
            renderLine = (cmd.words.w0 >> 9 & 511) * 8;
            renderSize = cmd.words.w0 >> 19 & 3;
        }
        if (op == G_SETTIMG_OTR_HASH || op == G_VTX_OTR_HASH || op == G_MARKER)
            ++i;
    }
    assert(imageSize == G_IM_SIZ_16b && loadPixels == 511 && renderLine == 16 && renderSize == G_IM_SIZ_8b);
    renderer.state.texture_to_load = { image->ImageData,
                                       imageSize,
                                       1,
                                       image->Flags,
                                       { image->Width, image->Height, image->HByteScale, image->VPixelScale,
                                         image->Type, image } };
    renderer.state.texture_tile[0].line_size_bytes = renderLine;
    renderer.state.texture_tile[0].siz = renderSize;
    renderer.GfxDpLoadBlock(7, 0, 0, loadPixels, loadDxt);
    renderer.ImportTextureRaw(0, false);
    assert(renderer.uploadedWidth == 16 && renderer.uploadedHeight == 64);
    assert(renderer.uploaded.size() == image->ImageDataSize &&
           std::equal(renderer.uploaded.begin(), renderer.uploaded.end(), image->ImageData));
}
} // namespace RoadVineUploadFixture
