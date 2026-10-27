#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <libultraship/libultra/gbi.h>
#include "tests/test_require.h"
#include "2s2h/ShipInit.hpp"

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP_MAX(a, b) MIN(a, b)
#define CLAMP(a, b, c) MIN(MAX(a, b), c)
#define SHADER_MAX_TEXTURES 6
#define TEXTURE_CACHE_MAX_SIZE 1024
#define ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))
using u8 = uint8_t;
using f32 = float;
struct Color_RGBA8 {
    uint8_t r, g, b, a;
};
struct ImVec4 {
    float x, y, z, w;
    ImVec4() : x(0), y(0), z(0), w(0) {
    }
    ImVec4(float r, float g, float b, float a) : x(r), y(g), z(b), w(a) {
    }
};

namespace Fast {
struct DisplayList {
    std::vector<Gfx> Instructions;
};
#include "cache_types.inc"
struct Interpreter {
    GfxTextureCache mTextureCache;
    uint64_t mDiagnosticCacheClears = 0;
    uint64_t mDiagnosticCacheDeletes = 0;
    std::unordered_map<const char*, int> mResolvedResourceCache;
    struct {
        TextureCacheNode* mTextures[SHADER_MAX_TEXTURES]{};
    } mRenderingState;
    void TextureCacheClear();
    void TextureCacheDelete(const uint8_t*);
    void TextureCacheDeleteByPalette(const uint8_t*, size_t);
};
#include "cache_production.inc"
} // namespace Fast
static auto interpreter = std::make_shared<Fast::Interpreter>();

// Resource/window services are the boundary. Cache eviction itself is production code.
namespace Ship {
struct Window {
    virtual ~Window() = default;
};
struct Resource {
    struct {
        bool IsCustom = false;
    } init;
    std::array<uint8_t, 2048> bytes{};
    auto* GetInitData() {
        return &init;
    }
    void* GetRawPointer() {
        return bytes.data();
    }
};
struct ResourceManager {
    std::map<std::string, std::shared_ptr<Resource>> resources;
    std::shared_ptr<Resource> LoadResource(const char* path) {
        return resources[path];
    }
};
struct Context {
    ResourceManager resources;
    std::shared_ptr<Window> window;
    static Context* GetRawInstance() {
        static Context context;
        return &context;
    }
    ResourceManager* GetResourceManager() {
        return &resources;
    }
    std::shared_ptr<Window> GetWindow() {
        return window;
    }
};
} // namespace Ship
namespace Fast {
struct Fast3dWindow : Ship::Window {
    std::weak_ptr<Interpreter> GetInterpreterWeak() {
        return interpreter;
    }
};
} // namespace Fast
void gfx_texture_cache_clear() {
    interpreter->TextureCacheClear();
}

#include "hud_declarations.inc"
static std::map<std::string, int> ints;
static std::map<std::string, Color_RGBA8> colors;
extern "C" int32_t CVarGetInteger(const char* key, int32_t fallback) {
    auto it = ints.find(key);
    return it == ints.end() ? fallback : it->second;
}
void CVarSetInteger(const char* key, int32_t value) {
    ints[key] = value;
}
float CVarGetFloat(const char*, float fallback) {
    return fallback;
}
Color_RGBA8 CVarGetColor(const char* key, Color_RGBA8 fallback) {
    auto it = colors.find(key);
    return it == colors.end() ? fallback : it->second;
}
void CVarSetColor(const char* key, Color_RGBA8 color) {
    colors[key] = color;
}
static CosmeticOption& CosmeticEditor_GetOptionMutable(const char* id) {
    return cosmeticOptions.at(id);
}
static bool CosmeticEditorIsSuppressed(const char*) {
    return false;
}
static bool CosmeticEditorIsSuppressed(const CosmeticOption&) {
    return false;
}
void RefreshDynamicCosmeticsStateIfNeeded() {
}
static bool customPlayer = false;
bool IsCustomHumanModelActive() {
    return customPlayer;
}
bool IsCustomDekuModelActive() {
    return customPlayer;
}
bool IsCustomGoronModelActive() {
    return customPlayer;
}
bool IsCustomZoraModelActive() {
    return customPlayer;
}
bool IsCustomKafeiModelActive() {
    return customPlayer;
}
void ResourceMgr_PatchGfxByName(const char*, const char*, int, Gfx) {
}
void ResourceMgr_UnpatchGfxByName(const char*, const char*) {
}
const char* kCosmeticRainbowSpeedCvar = "gCosmetics.RainbowSpeed";
const char* kCosmeticRainbowSyncCvar = "gCosmetics.RainbowSync";
int sCosmeticRainbowHue = 0;
static constexpr const char* RAINBOW_SYNC_CVAR = "gCosmetics.RainbowSync";
#include "pixel_production.inc"
#include "color_production.inc"
#include "rainbow_production.inc"

static std::shared_ptr<Ship::Resource> resource(const char* path, bool custom = false) {
    auto result = std::make_shared<Ship::Resource>();
    result->init.IsCustom = custom;
    for (size_t i = 0; i < result->bytes.size(); i += 2) {
        result->bytes[i] = 0xF8;
        result->bytes[i + 1] = 0x01; // opaque red RGBA5551
    }
    result->bytes[2] = 0x7C;
    result->bytes[3] = 0x00; // transparent pixel with color data to restore
    Ship::Context::GetRawInstance()->resources.resources[path] = result;
    return result;
}
static void cache(const uint8_t* address, uint32_t size = 2048, const uint8_t* palette0 = nullptr,
                  const uint8_t* palette1 = nullptr) {
    Fast::TextureCacheKey key{
        address, { palette0, palette1 }, uint8_t(palette0 || palette1 ? G_IM_FMT_CI : G_IM_FMT_RGBA), 2, 0, size
    };
    auto& cache = interpreter->mTextureCache;
    auto [entry, added] = cache.map.emplace(key, Fast::TextureCacheValue{});
    if (added) {
        entry->second.texture_id = cache.map.size();
        entry->second.lru_location = cache.lru.insert(cache.lru.end(), { entry });
    }
    interpreter->mRenderingState.mTextures[0] = &*entry;
}
static bool cached(const uint8_t* address) {
    return std::any_of(interpreter->mTextureCache.map.begin(), interpreter->mTextureCache.map.end(),
                       [address](const auto& entry) { return entry.first.texture_addr == address; });
}
static void selectColor(const char* id, Color_RGBA8 color, bool changed = true) {
    auto& option = cosmeticOptions.at(id);
    CVarSetColor(option.valuesCvar, color);
    CVarSetInteger(option.changedCvar, changed);
    CosmeticEditorRefreshElement(option);
}
extern "C" void TestSetHudColor(const char* id, uint8_t r, uint8_t g, uint8_t b, int changed) {
    selectColor(id, { r, g, b, 255 }, changed);
}
extern "C" void TestResetHudColors() {
    ints.clear();
    colors.clear();
}
extern "C" void TestHudDrawColors();

static void TestAllRainbowCosmetics() {
    ints.clear();
    colors.clear();
    interpreter->TextureCacheClear();
    uint8_t scene = 0, arm = 0, head = 0, unrelatedCi = 0;
    std::array<uint8_t, 512> otherPalette{};
    auto goron = resource("objects/object_link_goron/object_link_goron_Tex_00CEB8");
    auto arms = resource("objects/object_link_zora/object_link_zora_TLUT_00C578");
    auto hat = resource("objects/object_link_zora/object_link_zora_TLUT_005000");
    auto shield = resource("objects/object_link_zora/object_link_zora_Tex_010228");
    auto boomerang = resource("objects/gameplay_keep/gameplay_keep_Tex_0700B0");
    // First initialize each row, then measure repeated color updates. A one-time
    // palette change and a per-frame full-cache flush are different regressions.
    for (auto& [id, option] : cosmeticOptions) {
        if (option.supportsRainbow) {
            selectColor(id.c_str(), { 80, 160, 240, 255 });
            CVarSetInteger(option.rainbowCvar, 1);
        }
    }
    auto material = std::make_shared<Fast::DisplayList>();
    material->Instructions.resize(2);
    CustomCosmeticEntry entry{};
    entry.option = MakeCosmeticOption("test.custom", "test.custom.Color", "test.custom.Rainbow", "test.custom.Locked",
                                      "test.custom.Changed", "Custom material", COSMETICS_GROUP_MAX,
                                      { 80, 90, 100, 255 }, false, true, false);
    entry.bindings = { { "custom/material", material, 0, true, 77, 3, 4 },
                       { "custom/material", material, 1, false, 91, 0, 0 } };
    customCosmeticEntries.push_back(entry);
    CVarSetInteger(entry.option.changedCvar, 1);
    CVarSetInteger(entry.option.rainbowCvar, 1);
    auto customTexture = resource("test/custom/texture", true);
    const auto originalCustom = customTexture->bytes;
    cache(&scene);
    cache(&unrelatedCi, 32, otherPalette.data(), otherPalette.data() + 256);
    cache(customTexture->bytes.data());
    interpreter->mResolvedResourceCache["scene"] = 1;
    uint32_t previousRupee = 0, previousCustom = 0;
    int rupeeChanges = 0, customChanges = 0;
    for (int frame = 0; frame < 120; ++frame) {
        cache(goron->bytes.data());
        cache(shield->bytes.data());
        cache(boomerang->bytes.data());
        cache(&arm, 32, arms->bytes.data(), arms->bytes.data() + 256); // CI8, both halves
        cache(&head, 32, nullptr, hat->bytes.data() + 256);            // CI4 using the upper palette half
        CosmeticEditorUpdateTick();
        REQUIRE(cached(&scene) && cached(&unrelatedCi) && cached(customTexture->bytes.data()));
        REQUIRE(interpreter->mResolvedResourceCache.size() == 1);
        REQUIRE(!cached(goron->bytes.data()) && !cached(shield->bytes.data()) && !cached(boomerang->bytes.data()));
        REQUIRE(!cached(&arm) && !cached(&head));
        REQUIRE(interpreter->mRenderingState.mTextures[0] == nullptr);
        Gfx rupee{};
        gDPSetPrimColorOverride(&rupee, 0, 0, 200, 255, 100, 63, "HUD.RupeeIcon");
        const auto color = CVarGetColor(cosmeticOptions.at("HUD.RupeeIcon").valuesCvar, {});
        REQUIRE(rupee.words.w1 == ((uint32_t(color.r) << 24) | (color.g << 16) | (color.b << 8) | 63));
        rupeeChanges += rupee.words.w1 != previousRupee;
        previousRupee = rupee.words.w1;
        const auto custom = CVarGetColor(entry.option.valuesCvar, {});
        const uint32_t rgb = (uint32_t(custom.r) << 24) | (custom.g << 16) | (custom.b << 8);
        REQUIRE(material->Instructions[0].words.w1 == (rgb | 77));
        REQUIRE(material->Instructions[1].words.w1 == (rgb | 91));
        REQUIRE((material->Instructions[0].words.w0 & 0xFFFF) == 0x0304);
        customChanges += rgb != previousCustom;
        previousCustom = rgb;
        REQUIRE(customTexture->bytes == originalCustom);
    }
    REQUIRE(rupeeChanges > 100 && customChanges > 100); // the cycle boundary can repeat a quantized color
    puts("PASS: all built-in rainbow rows and custom prim/env materials retain unrelated textures for 120 ticks");
    puts("PASS: rupee icon hex/rainbow and custom material alpha remain live without texture rewrites");
    const auto goronOriginal = resource("test/original")->bytes;
    cache(goron->bytes.data());
    cache(&arm, 32, arms->bytes.data(), arms->bytes.data() + 256);
    selectColor("Player.GoronTunic", {}, false);
    selectColor("Player.ZoraTunic", {}, false);
    REQUIRE(goron->bytes == goronOriginal);
    REQUIRE(arms->bytes == goronOriginal && hat->bytes == goronOriginal);
    REQUIRE(shield->bytes == goronOriginal && boomerang->bytes == goronOriginal);
    REQUIRE(!cached(goron->bytes.data()) && !cached(&arm) && cached(&scene) && cached(&unrelatedCi));
    // A custom texture/palette is never rewritten by a native cosmetic row.
    for (auto res : { goron, arms, hat, shield, boomerang }) {
        res->init.IsCustom = true;
        cache(res->bytes.data());
    }
    cache(&arm, 32, arms->bytes.data(), arms->bytes.data() + 256);
    CosmeticEditorUpdateTick();
    for (auto res : { goron, arms, hat, shield, boomerang }) {
        REQUIRE(res->bytes == goronOriginal && cached(res->bytes.data()));
    }
    REQUIRE(cached(&arm) && cached(&scene) && cached(&unrelatedCi));
    puts("PASS: palette reset restores native pixels; custom palettes/textures keep their cache and pixels");
}

int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);
    Ship::Context::GetRawInstance()->window = std::make_shared<Fast::Fast3dWindow>();
    interpreter->mTextureCache.map.reserve(TEXTURE_CACHE_MAX_SIZE);
    auto heart = resource("objects/gameplay_keep/gDropRecoveryHeartTex");
    auto small = resource("objects/gameplay_keep/gDropMagicSmallTex");
    auto large = resource("objects/gameplay_keep/gDropMagicLargeTex");
    const auto original = heart->bytes;
    uint8_t landscape = 0;
    cache(&landscape);
    interpreter->mResolvedResourceCache["landscape"] = 7;
    cache(heart->bytes.data());
    cache(heart->bytes.data(), 1024); // every variant of this address must expire
    selectColor("HUD.Hearts", { 0, 255, 0, 255 });
    REQUIRE(cached(&landscape));
    REQUIRE(interpreter->mResolvedResourceCache.size() == 1);
    REQUIRE(!cached(heart->bytes.data()));
    REQUIRE(interpreter->mRenderingState.mTextures[0] == nullptr);
    REQUIRE(heart->bytes[0] == 0x07 && heart->bytes[1] == 0xC1);
    REQUIRE(heart->bytes[2] == 0 && heart->bytes[3] == 0);
    CVarSetInteger(kHeartsOption.rainbowCvar, 1);
    CVarSetInteger(kMagicOption.rainbowCvar, 1);
    CVarSetInteger(kMagicOption.changedCvar, 1);
    uint32_t lastColor = 0;
    for (int frame = 0; frame < 120; ++frame) {
        cache(heart->bytes.data());
        cache(small->bytes.data());
        cache(large->bytes.data());
        CosmeticEditorUpdateTick();
        REQUIRE(cached(&landscape));
        REQUIRE(interpreter->mResolvedResourceCache.size() == 1);
        REQUIRE(!cached(heart->bytes.data()) && !cached(small->bytes.data()) && !cached(large->bytes.data()));
        REQUIRE(heartsColorDL[0].words.w1 != lastColor);
        lastColor = heartsColorDL[0].words.w1;
    }
    puts("PASS: 120 rainbow ticks retain scene textures and expire only recolored pickups");
    cache(heart->bytes.data());
    selectColor("HUD.Hearts", {}, false);
    REQUIRE(heart->bytes == original);
    REQUIRE(!cached(heart->bytes.data()) && cached(&landscape));
    puts("PASS: reset restores exact original pixels and alpha without flushing scene textures");
    heart->init.IsCustom = true;
    cache(heart->bytes.data());
    selectColor("HUD.Hearts", { 0, 0, 255, 255 });
    REQUIRE(heart->bytes == original && cached(heart->bytes.data()) && cached(&landscape));
    Ship::Context::GetRawInstance()->resources.resources.erase("objects/gameplay_keep/gDropRecoveryHeartTex");
    CosmeticEditorUpdateTick();
    REQUIRE(cached(&landscape));
    puts("PASS: custom and missing pickup textures preserve pixels and cache");
    for (const char* id : { "HUD.DDHearts", "HUD.InfiniteMagic" }) {
        REQUIRE(cosmeticOptions.contains(id));
        auto& option = cosmeticOptions.at(id);
        REQUIRE(option.supportsRainbow);
        CVarSetInteger(option.rainbowCvar, 1);
        CosmeticEditorUpdateTick();
        REQUIRE(CVarGetInteger(option.changedCvar, 0));
        const auto before = CVarGetColor(option.valuesCvar, {});
        CosmeticEditorUpdateTick();
        const auto after = CVarGetColor(option.valuesCvar, {});
        REQUIRE(before.r != after.r || before.g != after.g || before.b != after.b);
        REQUIRE(cached(&landscape));
    }
    puts("PASS: Double Defense and Infinite Magic are independent rainbow-enabled HUD entries");
    selectColor("HUD.Hearts", { 53, 167, 225, 255 });
    selectColor("HUD.Magic", { 230, 93, 171, 255 });
    selectColor("HUD.DDHearts", { 17, 34, 51, 255 });
    selectColor("HUD.InfiniteMagic", { 18, 52, 86, 255 });
    REQUIRE(heartsColorDL[0].words.w1 == 0x35A7E1FF);
    REQUIRE(heartsColorDL[1].words.w1 == 0x35A7E1FF);
    REQUIRE(magicColorDL[0].words.w1 == 0xE65DABFF);
    REQUIRE(magicColorDL[1].words.w1 == 0xE65DABFF);
    REQUIRE(cached(&landscape));
    puts("PASS: native 3D pickup prim/grayscale commands retain base Hearts/Magic hex independently of HUD upgrades");
    TestHudDrawColors();
    TestAllRainbowCosmetics();
}
