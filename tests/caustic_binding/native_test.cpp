// Engine boundaries for the extracted production interpreter methods. The
// real cache structs, import/lookup/invalidation methods and triangle texture
// loop are inserted by native_regressions.py, rather than reimplemented here.
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#define G_IM_FMT_RGBA 0
#define G_IM_FMT_CI 2
#define G_IM_FMT_IA 3
#define G_IM_FMT_I 4
#define G_IM_FMT_YUV 1
#define G_IM_SIZ_4b 0
#define G_IM_SIZ_8b 1
#define G_IM_SIZ_16b 2
#define G_IM_SIZ_32b 3
#define G_IM_SIZ_16b_LINE_BYTES 2
#define G_TX_MIRROR 1
#define G_TX_CLAMP 2
#define G_TX_WRAP 0
#define G_TX_NOMASK 0
#define G_ZS_PRIM 4
#define G_MDSFT_TEXTFILT 12
#define G_TF_POINT 0
#define TEX_FLAG_LOAD_AS_RAW 1
#define TEX_FLAG_LOAD_AS_IMG 2
#define SHADER_MAX_TEXTURES 6
#define SHADER_FIRST_MASK_TEXTURE 2
#define SHADER_FIRST_REPLACEMENT_TEXTURE 4
#define TEXTURE_CACHE_MAX_SIZE 16
#define SPDLOG_ERROR(...) ((void)0)
#define SPDLOG_WARN(...) ((void)0)

namespace Ship {
struct IResource { inline static const std::string gAltAssetPrefix = "alt/"; };
struct ConsoleVariables {
    int reads{};
    std::optional<int> value;
    int GetInteger(const char* name, int fallback) {
        assert(std::string_view(name) == "gEnhancements.PreludeMaterialBindingReuse" && fallback == 0);
        ++reads;
        return value.value_or(fallback);
    }
};
struct Context {
    ConsoleVariables variables;
    static Context* GetRawInstance() { static Context instance; return &instance; }
    ConsoleVariables* GetConsoleVariables() { return &variables; }
};
}
namespace Fast {
enum class TextureType {
    Error = 0, RGBA32bpp = 1, RGBA16bpp = 2, Palette4bpp = 3, Palette8bpp = 4,
    Grayscale4bpp = 5, Grayscale8bpp = 6, GrayscaleAlpha4bpp = 7,
    GrayscaleAlpha8bpp = 8, GrayscaleAlpha16bpp = 9,
};
struct ResourceInitData { std::string Path; };
struct Texture {
    uint8_t* ImageData{};
    uint32_t ImageDataSize{};
    std::shared_ptr<ResourceInitData> init;
    std::shared_ptr<ResourceInitData> GetInitData() const { return init; }
};
/* PRODUCTION TYPES */
struct RenderingState { TextureCacheNode* mTextures[SHADER_MAX_TEXTURES]{}; };
struct ColorCombiner { bool usedTextures[2]{true, true}; };
struct MaskedTextureEntry { uint8_t* mask; uint8_t* replacementData; };
struct CostReport { int cacheHits{}, cacheMisses{}, cacheEvictions{}, textureImport{}; };
struct RenderCostProbe {
    CostReport report;
    bool Active() const { return true; }
    CostReport& MutableReport() { return report; }
};
struct RenderCostScope { explicit RenderCostScope(int*) {} };
struct RenderTextureScope { RenderTextureScope(RenderCostProbe*, const std::string&) {} };
struct Backend {
    int selections{}, samplerChanges{}, framebufferSelections{}, uploads{};
    uint32_t bound[SHADER_MAX_TEXTURES]{};
    uint32_t nextId = 1;
    uint32_t NewTexture() { return nextId++; }
    void SelectTexture(int slot, uint32_t id) { ++selections; bound[slot] = id; }
    void SetSamplerParameters(int, bool, uint32_t, uint32_t) { ++samplerChanges; }
    void SelectTextureFb(int) { ++framebufferSelections; bound[0] = 9999; }
};
struct F3DGfx { struct { uint32_t w0{}, w1{}; } words; };
struct Interpreter {
    RDP storage{};
    RDP* mRdp = &storage;
    RenderingState mRenderingState{};
    GfxTextureCache mTextureCache{};
    RenderCostProbe mRenderCost;
    Backend backend;
    Backend* mRapi = &backend;
    bool mPreludeMaterialBindingReuseEnabled{};
    const Texture* mPreludeMaterialBoundResources[2]{};
    bool mFbActive{};
    std::map<std::string, MaskedTextureEntry, std::less<>> mMaskedTextures;
    std::unordered_map<uintptr_t, int> mFbTextures;
    std::unordered_map<const void*, std::shared_ptr<Texture>> mResolvedResourceCache;
    uint64_t mDiagnosticCacheClears{}, mDiagnosticCacheDeletes{};
    unsigned flushes{};
    uint32_t widths[2]{}, heights[2]{}, tiles[2]{}, clampMask{};
    bool drawTextures[2]{true, true};
    float packedUv[8]{};
    void Flush() { ++flushes; }
    void Prepare(bool is_rect = false);
    void BeginRun();
    void ImportTexture(int i, int tile, bool replacement);
    bool TextureCacheLookup(int i, const TextureCacheKey& key);
    bool TryReusePreludeMaterialBinding(int i, uint32_t tile, const bool usedTextures[2]);
    static std::string_view GetBaseTexturePath(std::string_view path);
    void TextureCacheClear();
    void TextureCacheDelete(const uint8_t* address);
    void TextureCacheDeleteByPalette(const uint8_t* address, size_t size);
    void GfxDpSetTile(uint8_t, uint32_t, uint32_t, uint32_t, uint8_t, uint32_t,
                      uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
    void GfxDpSetTileSize(uint8_t, uint16_t, uint16_t, uint16_t, uint16_t);
    void ImportTextureMask(int, int) { ++backend.uploads; }
#define DECODE(name) void name(int, bool) { ++backend.uploads; }
    DECODE(ImportTextureRgba16) DECODE(ImportTextureRgba32) DECODE(ImportTextureIA4)
    DECODE(ImportTextureIA8) DECODE(ImportTextureIA16) DECODE(ImportTextureI4)
    DECODE(ImportTextureI8) DECODE(ImportTextureCi4) DECODE(ImportTextureCi8)
    DECODE(ImportTextureRaw) DECODE(ImportTextureImg)
#undef DECODE
};
/* PRODUCTION METHODS */
void Interpreter::Prepare(bool is_rect) {
    ColorCombiner value;
    std::copy(std::begin(drawTextures), std::end(drawTextures), value.usedTextures);
    ColorCombiner* comb = &value;
    uint32_t tex_width[2]{}, tex_height[2]{}, tex_width2[2]{}, tex_height2[2]{}, effective_tile[2]{};
    uint32_t tm = 0;
    (void)is_rect;
    /* PRODUCTION TEXTURE LOOP */
    std::copy(std::begin(tex_width), std::end(tex_width), widths);
    std::copy(std::begin(tex_height), std::end(tex_height), heights);
    std::copy(std::begin(effective_tile), std::end(effective_tile), tiles);
    clampMask = tm;
    LoadedVertex vertex{}; vertex.u = 16 * 32; vertex.v = 8 * 32;
    LoadedVertex* v_arr[] = { &vertex };
    bool usedTextures[] = { comb->usedTextures[0], comb->usedTextures[1] };
    float mBufVbo[8]{};
    size_t mBufVboLen = 0;
    int i = 0;
    /* PRODUCTION UV LOOP */
    std::copy(std::begin(mBufVbo), std::end(mBufVbo), packedUv);
}
void Interpreter::BeginRun() {
    /* PRODUCTION RUN PREFIX */
}
}
using namespace Fast;

static void Check(bool condition, const char* message) {
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
struct Fixture {
    Interpreter gfx;
    std::vector<uint8_t> wall = std::vector<uint8_t>(2048);
    std::vector<uint8_t> caustic = std::vector<uint8_t>(2048);
    std::shared_ptr<Texture> resources[2];
    Fixture(const char* firstPath = "scenes/shared/spot00_scene/s00_siro_02_5C000",
            const char* secondPath = "custom/prelude/textures/field_r4_caustics") {
        gfx.mPreludeMaterialBindingReuseEnabled = true;
        gfx.mTextureCache.map.reserve(TEXTURE_CACHE_MAX_SIZE);
        for (int i = 0; i < 2; ++i) {
            resources[i] = std::make_shared<Texture>();
            resources[i]->init = std::make_shared<ResourceInitData>();
            resources[i]->init->Path = i ? secondPath : firstPath;
            resources[i]->ImageData = i ? caustic.data() : wall.data();
            resources[i]->ImageDataSize = 2048;
            auto& loaded = gfx.mRdp->loaded_texture[i];
            loaded.addr = resources[i]->ImageData;
            loaded.orig_size_bytes = loaded.size_bytes = 2048;
            loaded.line_size_bytes = loaded.full_image_line_size_bytes = 64;
            loaded.raw_tex_metadata.resource = resources[i];
            loaded.raw_tex_metadata.type = TextureType::RGBA16bpp;
            loaded.raw_tex_metadata.width = loaded.raw_tex_metadata.height = 32;
            auto& tile = gfx.mRdp->texture_tile[i];
            tile.fmt = G_IM_FMT_RGBA; tile.siz = G_IM_SIZ_16b; tile.tmem_index = i;
            tile.line_size_bytes = 64; tile.lrs = tile.lrt = 124;
            tile.masks = tile.maskt = 5;
        }
        Dirty(); gfx.Prepare(); ResetCounts();
    }
    void Dirty() { gfx.mRdp->textures_changed[0] = gfx.mRdp->textures_changed[1] = true; }
    void ResetCounts() {
        gfx.flushes = 0; gfx.backend.selections = gfx.backend.samplerChanges = gfx.backend.uploads = 0;
    }
};

int main() {
    {
        struct Format { uint8_t format, size; TextureType type; };
        const Format formats[] = {
            { G_IM_FMT_RGBA, G_IM_SIZ_16b, TextureType::RGBA16bpp },
            { G_IM_FMT_RGBA, G_IM_SIZ_32b, TextureType::RGBA32bpp },
            { G_IM_FMT_IA, G_IM_SIZ_4b, TextureType::GrayscaleAlpha4bpp },
            { G_IM_FMT_IA, G_IM_SIZ_8b, TextureType::GrayscaleAlpha8bpp },
            { G_IM_FMT_IA, G_IM_SIZ_16b, TextureType::GrayscaleAlpha16bpp },
            { G_IM_FMT_I, G_IM_SIZ_4b, TextureType::Grayscale4bpp },
            { G_IM_FMT_I, G_IM_SIZ_8b, TextureType::Grayscale8bpp },
        };
        for (const auto& format : formats) {
            Fixture f;
            f.gfx.drawTextures[1] = false;
            f.gfx.mRdp->loaded_texture[1] = {};
            f.gfx.mRdp->texture_tile[1] = {};
            f.gfx.mRdp->texture_tile[0].fmt = format.format;
            f.gfx.mRdp->texture_tile[0].siz = format.size;
            f.gfx.mRdp->loaded_texture[0].raw_tex_metadata.type = format.type;
            f.Dirty(); f.gfx.Prepare(); f.ResetCounts();
            f.gfx.GfxDpSetTileSize(0, 12, 24, 136, 148); f.gfx.Prepare();
            Check(f.gfx.flushes == 0 && f.gfx.backend.selections == 0 && f.gfx.backend.uploads == 0,
                  "single-slot RGBA/IA/I animated materials must reuse without requiring an unused resource");
            Check(f.gfx.packedUv[0] == 13.0f / f.gfx.widths[0] &&
                  f.gfx.packedUv[1] == 2.0f / f.gfx.heights[0],
                  "single-slot material animation must preserve the production scrolling UV calculation");
        }
        Fixture f;
        f.gfx.drawTextures[0] = false; f.gfx.mRdp->loaded_texture[0] = {};
        f.gfx.mRdp->texture_tile[0] = {};
        f.Dirty(); f.gfx.Prepare();
        Check(f.gfx.backend.selections == 0 && f.gfx.flushes == 0,
              "a shader using only slot 1 must validate only its used resource");
    }
    {
        const char* paths[][2] = {
            { "scenes/shared/spot10_scene/wall", "custom/prelude/textures/lost_woods_caustics" },
            { "scenes/shared/spot10_scene/water", "custom/prelude/textures/water_motion" },
            { "alt/scenes/nonmq/Z2_00KEIKOKU_scene/ground", "alt/custom/materials/second_texture" },
            { "objects/example/base", "objects/example/detail" },
        };
        for (const auto& context : paths) {
            Fixture f(context[0], context[1]);
            f.Dirty(); f.gfx.Prepare();
            Check(f.gfx.flushes == 0 && f.gfx.backend.selections == 0 && f.gfx.backend.uploads == 0,
                  "compatible resource-backed two-texture draws must reuse across scenes and filenames");
            f.gfx.mPreludeMaterialBindingReuseEnabled = false;
            f.Dirty(); f.gfx.Prepare();
            Check(f.gfx.flushes == 2 && f.gfx.backend.selections == 2,
                  "cross-scene draws must retain native behavior with the option disabled");
        }
    }
    {
        Fixture f;
        const uint32_t ids[] = { f.gfx.backend.bound[0], f.gfx.backend.bound[1] };
        f.Dirty(); f.gfx.Prepare();
        Check(f.gfx.flushes == 0 && f.gfx.backend.selections == 0,
              "unchanged authored two-texture caustics draw must skip redundant flush/import");
        Check(f.gfx.backend.bound[0] == ids[0] && f.gfx.backend.bound[1] == ids[1] &&
              f.gfx.backend.uploads == 0, "reuse changed the selected texture or uploaded pixels");
        Check(!f.gfx.mRdp->textures_changed[0] && !f.gfx.mRdp->textures_changed[1],
              "handled dirty textures were not consumed");
        auto first = f.gfx.mTextureCache.lru.front().it;
        f.gfx.mTextureCache.lru.splice(f.gfx.mTextureCache.lru.end(), f.gfx.mTextureCache.lru,
                                     first->second.lru_location);
        bool used[] = { true, true };
        Check(f.gfx.TryReusePreludeMaterialBinding(0, 0, used) &&
              &*f.gfx.mTextureCache.lru.back().it == f.gfx.mRenderingState.mTextures[0],
              "reuse must touch the current node in the real LRU");
    }
    {
        Fixture f; f.gfx.mPreludeMaterialBindingReuseEnabled = false;
        f.Dirty(); f.gfx.Prepare();
        Check(f.gfx.flushes == 2 && f.gfx.backend.selections == 2,
              "disabled option must retain original flush/import sequence");
    }
    {
        Fixture f; f.gfx.GfxDpSetTileSize(1, 12, 24, 136, 148); f.gfx.Prepare();
        Check(f.gfx.backend.selections == 0 && f.gfx.mRdp->texture_tile[1].uls == 12 &&
              f.gfx.mRdp->texture_tile[1].ult == 24 && f.gfx.widths[1] == 32 &&
              f.gfx.packedUv[2] == 13.0f / 32 && f.gfx.packedUv[3] == 2.0f / 32,
              "scroll changes must reach real packed UVs without a rebind");
        f.ResetCounts(); f.gfx.mRdp->other_mode_h = 2U << G_MDSFT_TEXTFILT;
        f.Dirty(); f.gfx.Prepare();
        Check(f.gfx.backend.selections == 0 && f.gfx.backend.samplerChanges == 2 && f.gfx.flushes == 2,
              "filter changes must retain sampler updates and their flushes");
        f.ResetCounts(); f.gfx.GfxDpSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 8, 256, 1, 0,
                                          G_TX_MIRROR, 5, 0, G_TX_MIRROR, 5, 0);
        f.gfx.Prepare();
        Check(f.gfx.backend.selections == 0 && f.gfx.backend.samplerChanges == 1 && f.gfx.flushes == 1,
              "SetTile wrap/mirror changes must retain sampler updates and flushes");
    }
    {
        Fixture f; bool used[] = { true, true };
        f.resources[0]->ImageData = f.caustic.data();
        f.gfx.mRdp->loaded_texture[0].addr = f.caustic.data();
        f.gfx.ImportTexture(0, 0, false);
        Check(!f.gfx.TryReusePreludeMaterialBinding(0, 0, used),
              "shared texture nodes must retain the original sampler/binding path");
    }
    {
        Fixture f; bool both[] = { true, true }, one[] = { true, false };
        auto eligible = [&] { return f.gfx.TryReusePreludeMaterialBinding(1, 1, both); };
        Check(f.gfx.TryReusePreludeMaterialBinding(0, 0, one), "used single texture must qualify");
        Check(!f.gfx.TryReusePreludeMaterialBinding(1, 1, one), "unused texture slot must not qualify");
        bool none[] = { false, false };
        Check(!f.gfx.TryReusePreludeMaterialBinding(0, 0, none), "untextured draw must not qualify");
        Check(!f.gfx.TryReusePreludeMaterialBinding(-1, 0, both) &&
              !f.gfx.TryReusePreludeMaterialBinding(2, 2, both) &&
              !f.gfx.TryReusePreludeMaterialBinding(0, 1, both), "unexpected slots/tiles must fall back");
        f.gfx.mRdp->first_tile_index = 1; Check(!eligible(), "nonzero first tile must fall back");
        f.gfx.mRdp->first_tile_index = 0;
        f.resources[1]->init->Path += "_other"; Check(eligible(), "filename changes must not reject a compatible bound resource");
        f.resources[1]->init->Path = "alt/custom/prelude/textures/field_r4_caustics";
        Check(eligible(), "Alt resource must qualify by binding compatibility");
        f.gfx.mMaskedTextures["custom/prelude/textures/field_r4_caustics"] = { f.caustic.data(), nullptr };
        Check(!eligible(), "Alt stripping must still find a newly registered mask");
        f.gfx.mMaskedTextures.clear();
        f.resources[0]->init->Path = "scenes/shared/other_scene/wall";
        Check(eligible(), "another scene must qualify by binding compatibility");
        f.resources[0]->init->Path = "scenes/shared/spot00_scene/s00_siro_02_5C000";
        f.gfx.mRdp->loaded_texture[0].masked = true; Check(!eligible(), "masked draw must fall back");
        f.gfx.mRdp->loaded_texture[0].masked = false;
        f.gfx.mRdp->loaded_texture[1].blended = true; Check(!eligible(), "blended draw must fall back");
        f.gfx.mRdp->loaded_texture[1].blended = false;
        f.gfx.mRdp->texture_tile[0].fmt = G_IM_FMT_CI; Check(!eligible(), "CI base texture must fall back");
        f.gfx.mRdp->texture_tile[0].fmt = G_IM_FMT_RGBA;
        f.gfx.mRdp->loaded_texture[1].addr = nullptr; Check(!eligible(), "empty TMEM must fall back");
        f.gfx.mRdp->loaded_texture[1].addr = f.caustic.data() + 1;
        Check(!eligible(), "partial resource loads must fall back");
        f.gfx.mRdp->loaded_texture[1].addr = f.caustic.data();
        f.resources[1]->ImageDataSize = 1; Check(!eligible(), "invalid resource size must fall back");
        f.resources[1]->ImageDataSize = 2048;
        auto init = f.resources[1]->init; f.resources[1]->init.reset();
        Check(!eligible(), "missing resource init must fall back"); f.resources[1]->init = init;
        f.gfx.mFbTextures[(uintptr_t)f.wall.data()] = 7;
        Check(!eligible(), "framebuffer mirror in either slot must fall back");
        f.gfx.mFbTextures.clear(); f.gfx.mFbActive = true;
        Check(!eligible(), "offscreen framebuffer draw must fall back"); f.gfx.mFbActive = false;
        f.gfx.mRdp->other_mode_l |= G_ZS_PRIM;
        Check(!eligible(), "primitive-depth draw must fall back"); f.gfx.mRdp->other_mode_l = 0;
        f.gfx.mRenderingState.mTextures[1] = nullptr;
        Check(!eligible(), "invalidated bound cache node must fall back");
    }
    {
        const uint8_t unsupported[][2] = {
            { G_IM_FMT_RGBA, G_IM_SIZ_4b }, { G_IM_FMT_RGBA, G_IM_SIZ_8b },
            { G_IM_FMT_IA, G_IM_SIZ_32b }, { G_IM_FMT_I, G_IM_SIZ_16b },
            { G_IM_FMT_I, G_IM_SIZ_32b }, { G_IM_FMT_CI, G_IM_SIZ_4b },
            { G_IM_FMT_CI, G_IM_SIZ_8b }, { G_IM_FMT_YUV, G_IM_SIZ_16b },
        };
        bool used[] = { true, false };
        for (const auto& format : unsupported) {
            Fixture f; auto& tile = f.gfx.mRdp->texture_tile[0];
            tile.fmt = format[0]; tile.siz = format[1];
            f.gfx.ImportTexture(0, 0, false); // Establish a live node even for invalid imports.
            Check(!f.gfx.TryReusePreludeMaterialBinding(0, 0, used),
                  "palette/YUV or unsupported format-size combinations must fall back despite a live cache node");
        }
        for (TextureType type : { TextureType::Palette4bpp, TextureType::Palette8bpp }) {
            Fixture f; f.gfx.mRdp->loaded_texture[0].raw_tex_metadata.type = type;
            Check(!f.gfx.TryReusePreludeMaterialBinding(0, 0, used),
                  "palette resource metadata must fall back even when its tile claims RGBA");
        }
    }
    {
        Fixture f; bool both[] = { true, true };
        // Change each real cache-key component while a matching resource remains loaded.
        for (int component = 0; component < 3; ++component) {
            auto& tile = f.gfx.mRdp->texture_tile[1];
            auto& loaded = f.gfx.mRdp->loaded_texture[1];
            if (component == 0) ++tile.palette;
            if (component == 1) tile.siz = G_IM_SIZ_32b;
            if (component == 2) loaded.orig_size_bytes /= 2;
            Check(!f.gfx.TryReusePreludeMaterialBinding(1, 1, both), "changed cache key must fall back");
            tile.palette = 0; tile.siz = G_IM_SIZ_16b; loaded.orig_size_bytes = 2048;
        }
        auto other = std::make_shared<Texture>(*f.resources[1]);
        f.gfx.mRdp->loaded_texture[1].raw_tex_metadata.resource = other;
        Check(!f.gfx.TryReusePreludeMaterialBinding(1, 1, both), "different resource identity must fall back");
        f.gfx.mRdp->loaded_texture[1].raw_tex_metadata.resource = f.resources[1];
        std::vector<uint8_t> moved(2048);
        f.resources[1]->ImageData = moved.data(); f.gfx.mRdp->loaded_texture[1].addr = moved.data();
        Check(!f.gfx.TryReusePreludeMaterialBinding(1, 1, both), "changed image address must miss the old cache key");
        f.resources[1]->ImageData = f.caustic.data(); f.gfx.mRdp->loaded_texture[1].addr = f.caustic.data();
        auto* stale = f.gfx.mRenderingState.mTextures[1];
        f.gfx.TextureCacheDelete(f.caustic.data());
        // Simulate an old pointer surviving in a foreign caller: lookup must
        // reject it without reading freed memory. ASan verifies this case.
        f.gfx.mRenderingState.mTextures[1] = stale;
        Check(!f.gfx.TryReusePreludeMaterialBinding(1, 1, both), "erased map node must never be dereferenced");
        f.gfx.mRenderingState.mTextures[1] = nullptr;
        f.Dirty(); f.gfx.Prepare();
        Check(f.gfx.backend.selections >= 1 && f.gfx.backend.uploads == 1,
              "cache deletion must import/upload the next draw");
        f.gfx.TextureCacheClear(); f.ResetCounts(); f.Dirty(); f.gfx.Prepare();
        Check(f.gfx.backend.selections == 2 && f.gfx.backend.uploads == 2,
              "cache clear must import/upload both next textures");
    }
    {
        Fixture f; bool both[] = { true, true }; uint8_t addresses[TEXTURE_CACHE_MAX_SIZE]{};
        for (const auto& byte : addresses) {
            TextureCacheKey key = { &byte, {}, G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 2048 };
            f.gfx.TextureCacheLookup(4, key);
        }
        Check(!f.gfx.mRenderingState.mTextures[0] && !f.gfx.mRenderingState.mTextures[1] &&
              !f.gfx.TryReusePreludeMaterialBinding(1, 1, both), "LRU eviction must retire both old bindings");
        f.ResetCounts(); f.Dirty(); f.gfx.Prepare();
        Check(f.gfx.backend.selections == 2 && f.gfx.backend.uploads == 2,
              "evicted textures must be rebound and uploaded");
    }
    {
        Fixture f; F3DGfx command{}; F3DGfx* pointer = &command;
        gfx_set_timg_fb_handler_custom(&f.gfx, &pointer);
        f.ResetCounts(); f.Dirty(); f.gfx.Prepare();
        Check(f.gfx.backend.selections == 2, "direct framebuffer selection must invalidate reuse trust");
        f.gfx.mFbTextures[(uintptr_t)f.wall.data()] = 1;
        f.gfx.ImportTexture(0, 0, false); f.gfx.mFbTextures.clear();
        f.ResetCounts(); f.Dirty(); f.gfx.Prepare();
        Check(f.gfx.backend.selections == 2, "mirror framebuffer selection must invalidate reuse trust");
    }
    {
        Fixture f;
        auto& variables = Ship::Context::GetRawInstance()->variables;
        variables.value = 1; variables.reads = 0; f.gfx.BeginRun();
        Check(variables.reads == 1 && f.gfx.mPreludeMaterialBindingReuseEnabled &&
              !f.gfx.mPreludeMaterialBoundResources[0] && !f.gfx.mPreludeMaterialBoundResources[1],
              "Run must sample the CVar once and discard previous-frame binding trust");
        f.Dirty(); f.gfx.Prepare(); f.ResetCounts();
        variables.value = 0; f.Dirty(); f.gfx.Prepare();
        Check(f.gfx.backend.selections == 0 && variables.reads == 1,
              "option changes must wait for the next Run rather than reread per draw");
        f.gfx.BeginRun(); f.Dirty(); f.gfx.Prepare();
        Check(!f.gfx.mPreludeMaterialBindingReuseEnabled && f.gfx.backend.selections == 2 && variables.reads == 2,
              "next Run must restore original behavior after disabling the option");
        variables.value.reset(); f.gfx.BeginRun();
        Check(!f.gfx.mPreludeMaterialBindingReuseEnabled, "unset option must default to disabled");
        f.gfx.mPreludeMaterialBindingReuseEnabled = true;
        f.ResetCounts(); f.Dirty(); f.gfx.Prepare(true);
        Check(f.gfx.backend.selections == 2, "rectangle draws must retain the original binding path");
    }
    std::puts("PASS: production cross-scene binding reuse/dirty loop, sampler/UV path, compatibility guards, LRU, cache invalidation and FB fallbacks");
}
