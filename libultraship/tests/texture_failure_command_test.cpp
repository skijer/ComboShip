// Production SETTIMG handlers; archive lookup and graphics-state sink are doubles.
#include "fast/lus_gbi.h"
#include "fast/resource/type/Texture.h"
#include <cassert>
#include <cstring>
#include <dlfcn.h>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

struct Error {
    std::string format;
    std::vector<std::string> fields;
};
static std::vector<Error> errors;
template <typename T> static std::string Field(const T& value) {
    std::ostringstream out;
    out << value;
    return out.str();
}
template <typename... Args> static void Capture(const char* format, const Args&... args) {
    errors.push_back({ format, { Field(args)... } });
}
#define SPDLOG_ERROR(...) Capture(__VA_ARGS__)

namespace Ship {
class ArchiveManager {
  public:
    std::map<uint64_t, std::string> names;
    const char* HashToCString(uint64_t hash) {
        auto it = names.find(hash);
        return it == names.end() ? nullptr : it->second.c_str();
    }
};
class ResourceManager {
  public:
    bool OtrSignatureCheck(const char*);
    bool IsAltAssetsEnabled() { return alt; }
    std::shared_ptr<ArchiveManager> GetArchiveManager() { return archive; }
    std::shared_ptr<IResource> LoadResourceProcess(const char* path) {
        ++loads;
        auto it = textures.find(path);
        return it == textures.end() ? nullptr : it->second;
    }
    bool alt = true;
    int loads = 0;
    std::map<std::string, std::shared_ptr<Fast::Texture>> textures;
    std::shared_ptr<ArchiveManager> archive = std::make_shared<ArchiveManager>();
};
struct CrossRMRegistry {
    static std::shared_ptr<ResourceManager> Get(const std::string&) { return nullptr; }
};
#include "texture_signature.inc"
} // namespace Ship

namespace Fast {
#include "texture_metadata.inc"
class Interpreter {
  public:
    uintptr_t mSegmentPointers[16]{};
    void* SegAddr(uintptr_t);
    void GfxDpSetTextureImage(uint32_t format, uint32_t size, uint32_t width, const void*, uint32_t,
                              RawTexMetadata metadata, void* pixels) {
        assert(format == 0 && size == 2 && width == 32);
        ++sets;
        lastMetadata = metadata;
        lastPixels = pixels;
    }
    int sets = 0;
    RawTexMetadata lastMetadata{};
    void* lastPixels = nullptr;
};
static auto host = std::make_shared<Ship::ResourceManager>();
static std::shared_ptr<Ship::ResourceManager> ActiveResMgr() { return host; }
#define C0(pos, width) ((cmd->words.w0 >> (pos)) & ((1U << width) - 1))
#include "texture_failure_production.inc"

static Interpreter renderer;
static void SubmitRaw(const char* path) {
    F3DGfx commands[3]{};
    commands[0].words.w0 = 0xFD10001F;
    commands[0].words.w1 = reinterpret_cast<uintptr_t>(path);
    commands[1].words.w0 = 0x01003006; // The next vertex load must execute.
    commands[2].words.w0 = 0x05000204;
    auto* cursor = commands;
    assert(!gfx_set_timg_handler_rdp(&renderer, &cursor));
    ++cursor; // gfx_step's common increment, unchanged in production.
    if (cursor != commands + 1) {
        std::cerr << "FAIL raw missing texture skipped the following vertex command\n";
        std::exit(1);
    }
    assert(cursor->words.w0 == 0x01003006);
}

static void SubmitHash(uint64_t hash) {
    F3DGfx commands[4]{};
    commands[0].words.w0 = 0x2010001F;
    commands[1].words.w0 = hash >> 32;
    commands[1].words.w1 = hash & 0xFFFFFFFF;
    commands[2].words.w0 = 0x01003006;
    commands[3].words.w0 = 0x05000204;
    auto* cursor = commands;
    assert(!gfx_set_timg_otr_hash_handler_custom(&renderer, &cursor));
    ++cursor;
    if (cursor != commands + 2) {
        std::cerr << "FAIL hash texture failure skipped the following vertex command\n";
        std::exit(1);
    }
    assert(cursor->words.w0 == 0x01003006);
}

static auto MakeTexture() {
    auto result = std::make_shared<Fast::Texture>();
    result->ImageData = new uint8_t[4096]{};
    result->Width = result->Height = 32;
    result->Type = TextureType::RGBA32bpp;
    result->HByteScale = result->VPixelScale = 1;
    return result;
}

int Test(const std::string& selected) {
    const char* path = "__OTR__scenes/shared/spot01_scene/probe_texture";
    const uint64_t hash = 0x123456789ABCDEF0ULL;
    if (selected == "all" || selected == "raw") {
        SubmitRaw(path);
        std::cout << "PASS missing raw OTR texture preserves following command\n";
    }
    if (selected == "all" || selected == "unknown-hash") {
        SubmitHash(hash);
        std::cout << "PASS unknown hash preserves following command\n";
    }
    host->archive->names[hash] = path;
    if (selected == "all" || selected == "missing-hash") {
        SubmitHash(hash);
        std::cout << "PASS known hash with absent resource preserves following command\n";
    }
    if (selected != "all") return 0;

    auto tex = MakeTexture();
    host->textures[path] = tex;
    SubmitRaw(path);
    SubmitHash(hash);
    assert(renderer.sets == 2 && renderer.lastPixels == tex->ImageData);
    assert(renderer.lastMetadata.resource == tex && renderer.lastMetadata.width == 32);
    auto empty = MakeTexture();
    delete[] empty->ImageData;
    empty->ImageData = nullptr;
    host->textures[path] = empty;
    SubmitRaw(path);
    SubmitHash(hash);
    assert(renderer.sets == 2 && renderer.lastPixels == tex->ImageData);
    host->textures.clear();
    for (int i = 0; i < 20; ++i) {
        SubmitRaw(path);
        SubmitHash(hash);
    }
    assert(renderer.sets == 2 && renderer.lastPixels == tex->ImageData);
    SubmitRaw(reinterpret_cast<const char*>(0x08000000));
    SubmitRaw(reinterpret_cast<const char*>(0x08000001));
    assert(renderer.sets == 2);
#ifdef HAS_FAILURE_TRACE
    // New trace identifies each reason, path/hash and active manager/Alt context.
    bool named = false, unknown = false, unresolved = false, invalidImage = false;
    for (const auto& error : errors) {
        assert(error.fields.size() == 7);
        named |= error.fields[2] == path;
        unknown |= error.fields[1] == "unknown-hash";
        unresolved |= error.fields[1] == "unresolved-address";
        invalidImage |= error.fields[1] == "invalid-image-data";
    }
    assert(named && unknown && unresolved && invalidImage);
    const auto beforeRepeat = errors.size();
    SubmitRaw(path);
    SubmitHash(hash);
    assert(errors.size() == beforeRepeat);
    host->alt = false;
    SubmitRaw(path);
    assert(errors.size() == beforeRepeat + 1);
    const auto previousHost = host;
    host = std::make_shared<Ship::ResourceManager>();
    SubmitRaw(path);
    assert(errors.size() == beforeRepeat + 2);
    host = previousHost;
    for (int i = 0; i < 200; ++i) {
        const auto bad = "__OTR__scenes/probe/missing_" + std::to_string(i);
        SubmitRaw(bad.c_str());
    }
    assert(errors.size() == 129); // 128 contexts plus one overflow notice.
    const auto reports = errors.size();
    SubmitRaw(path);
    assert(errors.size() == reports);
    // Saturation is only a diagnostic limit: recovery must still bind normally.
    host->textures[path] = tex;
    SubmitRaw(path);
    SubmitHash(hash);
    assert(renderer.sets == 4 && renderer.lastPixels == tex->ImageData);
    std::cout << "PASS bounded named failure tracing, owner/Alt contexts, repeat suppression and recovery\n";
#endif
    std::cout << "PASS successful raw/hash binding and unchanged prior texture on failure\n";
    return 0;
}
} // namespace Fast

int main(int argc, char** argv) {
    return Fast::Test(argc > 1 ? argv[1] : "all");
}
