// Standalone production-function fixture; see run_gfx_texture_address_tests.py.
// Moving validation after signature probing must crash the segment8/9 cases.
#include "fast/lus_gbi.h"
#include "fast/resource/type/Texture.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <memory>
#include <string>
#include <vector>

template <typename... Args> static void IgnoreDiagnostic(const Args&...) {}
#define SPDLOG_ERROR(...) IgnoreDiagnostic(__VA_ARGS__)

// External linkage lets dladdr recognize these low mapped addresses in non-PIE tests.
alignas(8) unsigned char gTestRawTexture[128] = { 0x12, 0x34, 0x56, 0x78 };
alignas(8) char gTestTexturePath[] = "__OTR__textures/test/scene_texture";

namespace Ship {
class ResourceManager {
  public:
    bool OtrSignatureCheck(const char* fileName);
    bool IsAltAssetsEnabled() { return false; }
    std::shared_ptr<IResource> LoadResourceProcess(const char* path) {
        assert(std::strcmp(path, gTestTexturePath) == 0);
        ++loads;
        return texture;
    }
    std::shared_ptr<Fast::Texture> texture;
    int loads = 0;
};
struct CrossRMRegistry {
    static std::shared_ptr<ResourceManager> Get(const std::string&) {
        return nullptr;
    }
};
#include "texture_signature.inc"
} // namespace Ship

namespace Fast {
#include "texture_metadata.inc"

class Interpreter {
  public:
    uintptr_t mSegmentPointers[16]{};
    void* SegAddr(uintptr_t address);
    void GfxDpSetTextureImage(uint32_t format, uint32_t size, uint32_t width, const void* path, uint32_t flags,
                              RawTexMetadata metadata, void* pixels) {
        assert(format == 0 && size == 2 && width == 32);
        ++sets;
        lastPath = path;
        lastFlags = flags;
        lastMetadata = metadata;
        lastPixels = pixels;
    }
    int sets = 0;
    const void* lastPath = nullptr;
    void* lastPixels = nullptr;
    uint32_t lastFlags = 0;
    RawTexMetadata lastMetadata{};
};

static auto sInterpreter = std::make_shared<Interpreter>();
static std::weak_ptr<Interpreter> mInstance = sInterpreter;
static auto sManager = std::make_shared<Ship::ResourceManager>();
static std::shared_ptr<Ship::ResourceManager> ActiveResMgr() {
    return sManager;
}

#define C0(pos, width) ((cmd->words.w0 >> (pos)) & ((1U << width) - 1))
#include "texture_address_production.inc"

static void Submit(uintptr_t address) {
    F3DGfx commands[2]{};
    commands[0].words.w0 = 0xFD10001F; // RGBA16 texture, width 32
    commands[0].words.w1 = address;
    F3DGfx* command = commands;
    assert(!gfx_set_timg_handler_rdp(sInterpreter.get(), &command));
    assert(command == commands); // Rejection must not skip the following GBI command.
}

static void Reject(uintptr_t address) {
    Submit(reinterpret_cast<uintptr_t>(gTestRawTexture));
    int sets = sInterpreter->sets;
    int loads = sManager->loads;
    Submit(address);
    assert(sInterpreter->sets == sets);
    assert(sManager->loads == loads);
    assert(sInterpreter->lastPixels == gTestRawTexture);
}

static void ValidTextures() {
#ifdef TEST_LOW_MODULE
    assert(reinterpret_cast<uintptr_t>(gTestRawTexture) < 0x10000000);
    assert(reinterpret_cast<uintptr_t>(gTestTexturePath) < 0x10000000);
#endif
    Submit(reinterpret_cast<uintptr_t>(gTestRawTexture));
    assert(sInterpreter->lastPixels == gTestRawTexture);
    assert(sInterpreter->lastFlags == 0);
    assert(sInterpreter->lastMetadata.h_byte_scale == 1);
    assert(sInterpreter->lastMetadata.v_pixel_scale == 1);

    sInterpreter->mSegmentPointers[8] = reinterpret_cast<uintptr_t>(gTestRawTexture);
    Submit(0x08000011); // Tagged segment 8, byte offset 16.
    assert(sInterpreter->lastPixels == gTestRawTexture + 16);
    sInterpreter->mSegmentPointers[8] = 0;

    auto texture = std::make_shared<Texture>();
    // A large allocation uses the mapped heap on Linux even in the non-PIE
    // variant; that variant specifically exercises low *module* addresses.
    texture->ImageData = new uint8_t[1024 * 1024]{};
    texture->ImageDataSize = 128;
    texture->Type = TextureType::RGBA32bpp;
    texture->Width = 8;
    texture->Height = 4;
    texture->HByteScale = 4;
    texture->VPixelScale = 2;
    texture->Flags = TEX_FLAG_LOAD_AS_RAW;
    sManager->texture = texture;
    Submit(reinterpret_cast<uintptr_t>(gTestTexturePath));
    assert(sInterpreter->lastPixels == texture->ImageData);
    assert(sInterpreter->lastPath == gTestTexturePath);
    assert(sInterpreter->lastFlags == TEX_FLAG_LOAD_AS_RAW);
    assert(sInterpreter->lastMetadata.resource == texture);
    assert(sInterpreter->lastMetadata.width == 8 && sInterpreter->lastMetadata.height == 4);
    assert(sInterpreter->lastMetadata.h_byte_scale == 4 && sInterpreter->lastMetadata.v_pixel_scale == 2);

    sInterpreter->mSegmentPointers[9] = reinterpret_cast<uintptr_t>(gTestTexturePath);
    Submit(0x09000001); // Bound segment may point at an OTR path, not just raw pixels.
    assert(sInterpreter->lastPixels == texture->ImageData);
    sInterpreter->mSegmentPointers[9] = 0;
    puts("PASS raw/module textures, bound segment offsets, OTR resources and HD metadata");
}
} // namespace Fast

int main(int argc, char** argv) {
    const std::string test = argc > 1 ? argv[1] : "all";
    assert(test == "all" || test == "segment8" || test == "segment9" || test == "valid");
    if (test == "all" || test == "segment8") {
        Fast::Reject(0x08000000);
        puts("PASS reported segment 8 crash address");
    }
    if (test == "all" || test == "segment9") {
        Fast::Reject(0x09000000);
        puts("PASS reported segment 9 crash address");
    }
    if (test == "all") {
        for (uintptr_t address : { 0u, 0x4000u, 0x08000001u, 0x09000001u }) {
            Fast::Reject(address);
        }
        puts("PASS null/low/unbound tagged addresses preserve current texture and command position");
    }
    if (test == "all" || test == "valid") {
        Fast::ValidTextures();
    }
}
