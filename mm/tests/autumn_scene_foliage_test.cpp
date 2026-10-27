// Exercise production transformation/lifecycle and extracted renderer handlers
// with real GBI/MM headers. Archive/cache services and GPU submission are seams.
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <vector>
#include <nlohmann/json.hpp>
#include <ship/utils/StrHash64.h>
#include "2s2h/Enhancements/Graphics/AutumnSceneFoliage.h"
#include "entrances/SoloForest.h"
extern "C" {
#include "global.h"
}

namespace Ship {
struct ResourceInitData {
    std::string Path;
};
struct IResource {
    virtual ~IResource() = default;
    std::shared_ptr<ResourceInitData> initData;
    std::shared_ptr<ResourceInitData> GetInitData() {
        return initData;
    }
};
static std::string activeGame = "oot";
struct OwnRMScope {
    std::string previous = activeGame;
    explicit OwnRMScope(const char* game) {
        activeGame = game;
    }
    ~OwnRMScope() {
        activeGame = previous;
    }
};
struct CrossRMRegistry {
    inline static void (*teardown)() = nullptr;
    static void RegisterTeardownListener(void (*listener)()) {
        teardown = listener;
    }
};
struct File {
    std::shared_ptr<std::vector<char>> Buffer;
    size_t BufferOffset = 0;
};
struct Archive {
    std::map<std::string, std::shared_ptr<File>> rawFiles;
    std::map<std::string, unsigned> reads;
    std::shared_ptr<File> LoadFile(const std::string& path) {
        ++reads[path];
        auto found = rawFiles.find(path);
        return found == rawFiles.end() ? nullptr : found->second;
    }
    std::string GetPath() {
        return "fixture.o2r";
    }
};
struct ArchiveManager {
    std::map<std::string, std::shared_ptr<IResource>> files;
    std::map<std::string, std::shared_ptr<Archive>> owners;
    std::shared_ptr<Archive> defaultOwner = std::make_shared<Archive>();
    std::string archiveLookup;
    bool HasFile(const std::string& path) {
        return files.contains(path);
    }
    std::shared_ptr<Archive> GetArchiveFromFile(const std::string& path) {
        archiveLookup = path;
        const auto found = files.find(path);
        if (found == files.end() || !found->second)
            return nullptr;
        const auto owner = owners.find(path);
        return owner == owners.end() ? defaultOwner : owner->second;
    }
    std::shared_ptr<std::vector<std::string>> ListFiles(const std::string& mask) {
        auto result = std::make_shared<std::vector<std::string>>();
        const auto prefix = mask.substr(0, mask.find('*'));
        for (const auto& [path, resource] : files) {
            if (path.starts_with(prefix))
                result->push_back(path);
        }
        return result;
    }
};
struct ResourceManager {
    std::map<std::string, std::shared_ptr<IResource>> resources;
    std::shared_ptr<ArchiveManager> archive = std::make_shared<ArchiveManager>();
    bool alt = false;
    unsigned loads = 0;
    bool OtrSignatureCheck(const char* path);
    bool IsAltAssetsEnabled() {
        return alt;
    }
    void CacheExternalResource(const std::string& path, std::shared_ptr<IResource> resource) {
        assert(activeGame == "mm");
        resources[path] = std::move(resource);
    }
    auto GetArchiveManager() {
        return archive;
    }
    std::shared_ptr<IResource> GetCachedResource(const std::string& path, bool exact = false) {
        assert(activeGame == "mm" && exact);
        auto i = resources.find(path);
        return i == resources.end() ? nullptr : i->second;
    }
    std::shared_ptr<IResource> LoadResource(const std::string& path, bool exact = false) {
        ++loads;
        if (auto r = GetCachedResource(path, exact))
            return r;
        auto i = archive->files.find(path);
        if (i == archive->files.end() || i->second == nullptr)
            i = archive->files.find(path + ".meta");
        return i == archive->files.end() ? nullptr : resources[path] = i->second;
    }
};
struct Context {
    std::shared_ptr<ResourceManager> rm = std::make_shared<ResourceManager>();
    static Context* GetRawInstance() {
        static Context context;
        return &context;
    }
    auto GetResourceManager() {
        return rm;
    }
};
} // namespace Ship
namespace Fast {
#define TEX_FLAG_LOAD_AS_RAW (1 << 0)
enum class TextureType { Error = 0, RGBA32bpp = 1, RGBA16bpp = 2, Palette4bpp = 3, Palette8bpp = 4 };
struct Texture : Ship::IResource {
    explicit Texture(std::shared_ptr<Ship::ResourceInitData> data = nullptr) {
        initData = std::move(data);
    }
    TextureType Type = TextureType::RGBA32bpp;
    uint16_t Width = 1024, Height = 512;
    uint32_t Flags = 0, ImageDataSize = 0;
    float HByteScale = 1, VPixelScale = 1;
    uint8_t* ImageData = nullptr;
    std::shared_ptr<std::vector<char>> mImageBuffer;
};
struct DisplayList : Ship::IResource {
    std::vector<Gfx> Instructions;
};
} // namespace Fast
static int season = SEASON_AUTUMN;
extern "C" int MMWeather_SeasonForPlay(const PlayState*) {
    return season;
}
extern "C" int MMWeather_FoliageSeasonForPlay(const PlayState*) {
    return season;
}

static std::map<std::string, unsigned> sForestReports;
template <class... Args> static void ForestReport(const char* format, Args&&...) {
    ++sForestReports[format];
}
#define SPDLOG_INFO(...) ForestReport(__VA_ARGS__)
#define SPDLOG_WARN(...) ForestReport(__VA_ARGS__)
// The runner counts coverage-only parser walks in its extracted test copy.
static size_t sCoverageWalks = 0;
#include "autumn_scene_foliage_production.inc"

static Gfx Command(uintptr_t w0, uintptr_t w1) {
    Gfx command{};
    command.words.w0 = w0;
    command.words.w1 = w1;
    return command;
}
static std::vector<Gfx> MakeMaterial(const char* texture) {
    const uint64_t hash = CRC64(texture);
    return { Command(uintptr_t(G_SETTIMG_OTR_HASH) << 24, 0), Command(hash >> 32, hash & 0xFFFFFFFF),
             gsDPSetCombineMode(G_CC_MODULATEIA, G_CC_MODULATEIA), gsDPSetPrimColor(0, 0, 180, 200, 160, 192),
             gsSP1Triangle(0, 1, 2, 0) };
}
static std::shared_ptr<Fast::DisplayList> List(const char* texture) {
    auto list = std::make_shared<Fast::DisplayList>();
    list->Instructions = MakeMaterial(texture);
    auto unrelated = MakeMaterial("scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_029090");
    list->Instructions.insert(list->Instructions.end(), unrelated.begin(), unrelated.end());
    list->Instructions.push_back(gsSPEndDisplayList());
    return list;
}
static bool Equal(const std::vector<Gfx>& a, const std::vector<Gfx>& b) {
    if (a.size() != b.size())
        return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i].words.w0 != b[i].words.w0 || a[i].words.w1 != b[i].words.w1)
            return false;
    }
    return true;
}
static std::vector<Gfx> RemoveTint(const std::vector<Gfx>& source) {
    std::vector<Gfx> result;
    for (size_t i = 0; i < source.size(); ++i) {
        const auto op = source[i].words.w0 >> 24;
        if (op == G_SETGRAYSCALE || op == G_SETINTENSITY)
            continue;
        result.push_back(source[i]);
        if (op == G_SETTIMG_OTR_HASH || op == G_VTX_OTR_HASH || op == G_DL_OTR_HASH || op == G_MARKER ||
            op == G_MTX_OTR) {
            result.push_back(source.at(++i));
        }
    }
    return result;
}
static unsigned CountMaterialTriangles(const std::vector<Gfx>& commands, const char* texture, uint32_t color = 0) {
    const auto target = CRC64(texture);
    uint64_t currentTexture = 0;
    uint32_t currentColor = 0;
    bool tint = false;
    unsigned count = 0;
    for (size_t i = 0; i < commands.size(); ++i) {
        const auto op = commands[i].words.w0 >> 24;
        if (op == G_SETGRAYSCALE)
            tint = commands[i].words.w1 != 0;
        if (op == G_SETINTENSITY)
            currentColor = commands[i].words.w1;
        if (op == G_SETTIMG_OTR_HASH) {
            const auto payload = commands.at(i + 1);
            currentTexture = (uint64_t(payload.words.w0) << 32) | payload.words.w1;
        } else if (op == G_SETTIMG || op == G_DL || op == G_DL_OTR_HASH) {
            currentTexture = 0;
        }
        if ((op == G_TRI1 || op == G_TRI2) && currentTexture == target &&
            (color == 0 || (tint && currentColor == color)))
            count += op == G_TRI2 ? 2 : 1;
        if (op == G_SETTIMG_OTR_HASH || op == G_VTX_OTR_HASH || op == G_DL_OTR_HASH || op == G_MARKER ||
            op == G_MTX_OTR)
            ++i;
    }
    assert(!tint); // Seasonal state must not leak past a tinted batch.
    return count;
}
#include "autumn_scene_moon_tree_fixture.h"

static void CheckScopedForestCommands(const Variant& variant, const std::vector<Gfx>& original,
                                      int selectedSeason = SEASON_AUTUMN) {
    assert(Equal(variant.original, original));
    const auto& commands = variant.commands;
    assert(commands.size() == 5);
    assert(SameCommand(commands[0], gsSPGrayscale(selectedSeason == SEASON_WINTER ? 3 : 2)));
    if (selectedSeason == SEASON_WINTER)
        assert(SameCommand(commands[1], gsDPSetGrayscaleColor(226, 238, 233, 255)));
    else
        assert(SameCommand(commands[1], gsDPSetGrayscaleColor(239, 72, 76, 255)));
    const auto* draw = reinterpret_cast<const Gfx*>(commands[2].words.w1);
    assert(draw != variant.original.data()); // The renderer writes resolved texture pointers into executed lists.
    assert(Equal(std::vector<Gfx>(draw, draw + original.size()), original));
    assert(SameCommand(commands[2], gsSPDisplayList(draw)));
    assert(SameCommand(commands[3], gsSPGrayscale(false)));
    assert(SameCommand(commands[4], gsSPEndDisplayList()));
}

#include "autumn_scene_forest_fixture.h"
#include "autumn_scene_middle_forest_fixture.h"
#include "autumn_scene_renderer_fixture.h"
static void CheckForestDrawOwners() {
    constexpr const char* wallpaper = "scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_034098";
    constexpr const char* canopy = "scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_037098";
    assert(CountMaterialTriangles(kActualForestCommands, wallpaper) == 14);
    assert(CountMaterialTriangles(kActualForestCommands, canopy) == 8);
    const auto wallHash = CRC64(wallpaper);
    const auto canopyHash = CRC64(canopy);
    unsigned cases = 0;
    for (const auto* suffix : { "", "_scene" }) {
        for (const auto* prefix : { "", "alt/" }) {
            for (const auto* setup : { "", "Set_001200", "Set_001340", "Set_0014C0", "Set_0015D0", "Set_001750",
                                       "Set_0019F0", "Set_001B30", "Set_001C90", "Set_001E60" }) {
                // Start with both private, then mixed native/private, then native.
                for (const unsigned privateMask : { 3, 1, 2, 0 }) {
                    auto original = kActualForestCommands;
                    for (size_t i = 0; i < original.size(); ++i) {
                        const auto op = original[i].words.w0 >> 24;
                        if (op == G_SETTIMG_OTR_HASH) {
                            const auto hash = (uint64_t(original[i + 1].words.w0) << 32) | original[i + 1].words.w1;
                            const auto replacement =
                                hash == wallHash && (privateMask & 1) ? CRC64("custom/test_private_forest/wallpaper")
                                : hash == canopyHash && (privateMask & 2) ? CRC64("custom/test_private_forest/canopy")
                                                                          : hash;
                            original[i + 1] = Command(replacement >> 32, replacement & 0xFFFFFFFF);
                        }
                        if (op == G_SETTIMG_OTR_HASH || op == G_VTX_OTR_HASH || op == G_DL_OTR_HASH || op == G_MARKER)
                            ++i;
                    }
                    ClearCache();
                    auto rm = std::make_shared<Ship::ResourceManager>();
                    Ship::Context::GetRawInstance()->rm = rm;
                    const auto path = std::string(prefix) + "scenes/nonmq/Z2_00KEIKOKU" + suffix +
                                      "/Z2_00KEIKOKU_room_00" + setup + "DL_0241B0";
                    auto resource = std::make_shared<Fast::DisplayList>();
                    resource->Instructions = original;
                    rm->archive->files[path] = resource;
                    rm->resources[path] = resource;
                    rm->alt = std::string(prefix) == "alt/";
                    PlayState play{};
                    play.sceneId = SCENE_00KEIKOKU;
                    season = SEASON_AUTUMN;
                    MMAutumnSceneFoliage_Update(&play);
                    if ((resource->Instructions[0].words.w0 >> 24) != G_DL) {
                        std::fprintf(stderr, "FAIL %s: both private forest layers remain untinted\n", path.c_str());
                        assert(false);
                    }
                    const auto branch = resource->Instructions[0];
                    const auto& variant = sVariants.at(resource.get());
                    CheckScopedForestCommands(variant, original);
                    assert(branch.words.w1 == reinterpret_cast<uintptr_t>(variant.commands.data()));
                    for (size_t i = 1; i < original.size(); ++i)
                        assert(SameCommand(resource->Instructions[i], original[i]));
                    auto* draw = reinterpret_cast<Gfx*>(variant.commands[2].words.w1);
                    for (size_t i = 0; i < original.size(); ++i) {
                        const auto op = draw[i].words.w0 >> 24;
                        if (op == G_SETTIMG_OTR_HASH)
                            draw[i].words.w1 = uintptr_t(0x10000000) + i * 256;
                        if (op == G_SETTIMG_OTR_HASH || op == G_VTX_OTR_HASH || op == G_DL_OTR_HASH || op == G_MARKER)
                            ++i;
                    }
                    for (unsigned i = 0; i < 100; ++i)
                        MMAutumnSceneFoliage_Update(&play);
                    assert(SameCommand(resource->Instructions[0], branch) && rm->loads == 0 && sVariants.size() == 1);
                    season = SEASON_OFF;
                    MMAutumnSceneFoliage_Update(&play);
                    assert(Equal(resource->Instructions, original));
                    season = SEASON_AUTUMN;
                    MMAutumnSceneFoliage_Update(&play);
                    assert(SameCommand(resource->Instructions[0], branch) && sVariants.size() == 1 && rm->loads == 0);
                    ClearCache();
                    assert(Equal(resource->Instructions, original));
                    ++cases;
                }
            }
        }
    }
    std::printf("PASS %u actual forest draw cases: 14 wallpaper + 8 canopy triangles, native/private/mixed bindings, "
                "both namespaces, Alt, all ten setups, exact restoration and cache reuse\n",
                cases);
    for (const auto* name : { "Z2_00KEIKOKU_room_01DL_0241B0", "Z2_00KEIKOKU_room_00Set_999999DL_0241B0",
                              "Z2_00KEIKOKU_room_00DL_0241B1" }) {
        ClearCache();
        auto rm = std::make_shared<Ship::ResourceManager>();
        Ship::Context::GetRawInstance()->rm = rm;
        const auto path = std::string("scenes/nonmq/Z2_00KEIKOKU/") + name;
        auto resource = List("custom/test_private_forest/unrelated");
        const auto original = resource->Instructions;
        rm->archive->files[path] = resource;
        rm->resources[path] = resource;
        PlayState play{};
        play.sceneId = SCENE_00KEIKOKU;
        season = SEASON_AUTUMN;
        MMAutumnSceneFoliage_Update(&play);
        assert(Equal(resource->Instructions, original) && sVariants.empty());
    }
    ClearCache();
    std::puts("PASS forest owner matching excludes other rooms, unknown setups and neighboring draws");
}

static void CheckActualMaterial(const char* path, const std::vector<Gfx>& original, const char* texture,
                                unsigned expectedTriangles) {
    const auto variant = BuildVariant(original);
    assert(!variant.empty() && Equal(RemoveTint(variant), original));
    assert(CountMaterialTriangles(original, texture) == expectedTriangles);
    const auto tinted = CountMaterialTriangles(
        variant, texture, std::string(texture).find("foliage_poc3/") != std::string::npos ? 0xDE496BFF : 0xEF484CFF);
    if (tinted != expectedTriangles)
        std::fprintf(stderr, "%s: confirmed-role tint covers %u/%u triangles\n", path, tinted, expectedTriangles);
    assert(tinted == expectedTriangles);

    ClearCache();
    auto rm = std::make_shared<Ship::ResourceManager>();
    Ship::Context::GetRawInstance()->rm = rm;
    auto resource = std::make_shared<Fast::DisplayList>();
    resource->Instructions = original;
    rm->archive->files[path] = resource;
    rm->resources[path] = resource;
    rm->alt = std::string(path).starts_with("alt/");
    PlayState play{};
    play.sceneId = SCENE_00KEIKOKU;
    season = SEASON_AUTUMN;
    MMAutumnSceneFoliage_Update(&play);
    const auto branch = resource->Instructions[0];
    assert((branch.words.w0 >> 24) == G_DL);
    const auto& applied = sVariants.at(resource.get()).commands;
    assert(branch.words.w1 == reinterpret_cast<uintptr_t>(applied.data()));
    if (applied.size() == 5 && (applied[0].words.w0 >> 24) == G_SETGRAYSCALE) {
        CheckScopedForestCommands(sVariants.at(resource.get()), original);
    } else {
        assert(Equal(applied, variant));
    }
    for (unsigned i = 0; i < 100; ++i)
        MMAutumnSceneFoliage_Update(&play);
    assert(SameCommand(resource->Instructions[0], branch) && rm->loads == 0 && sVariants.size() == 1);
    season = SEASON_OFF;
    MMAutumnSceneFoliage_Update(&play);
    assert(Equal(resource->Instructions, original));
    season = SEASON_AUTUMN;
    MMAutumnSceneFoliage_Update(&play);
    assert(SameCommand(resource->Instructions[0], branch) && sVariants.size() == 1 && rm->loads == 0);
    ClearCache();
    assert(Equal(resource->Instructions, original));
    std::printf("PASS %s: %u/%u confirmed-role triangles; exact commands, cache reuse and Off restoration\n", path,
                tinted, expectedTriangles);
}
static void CheckPrivateBrushMaterials() {
    // A terrain match makes BuildVariant nonempty, so the root fallback cannot
    // cover later private leaves. Material identity must survive that boundary.
    for (const auto* suffix : { "", "_scene" }) {
        for (const auto* prefix : { "", "alt/" }) {
            const auto scene = std::string(prefix) + "scenes/nonmq/Z2_00KEIKOKU" + suffix + "/";
            const auto terrain = scene + "Z2_00KEIKOKUTex_01C650";
            const auto leaves = scene + "foliage_poc3/leaves_rgba";
            const auto stems = scene + "foliage_poc3/stems_rgba";
            const auto unrelated = scene + "foliage_poc3/unrelated_rgba";
            std::vector<Gfx> original;
            for (const auto& texture : { terrain, leaves, stems, unrelated }) {
                const auto material = MakeMaterial(texture.c_str());
                original.insert(original.end(), material.begin(), material.end());
            }
            original.push_back(gsSPEndDisplayList());
            const auto variant = BuildVariant(original);
            assert(!variant.empty() && Equal(RemoveTint(variant), original));
            assert(CountMaterialTriangles(variant, terrain.c_str(), 0xF3974FFF) == 1);
            assert(CountMaterialTriangles(variant, leaves.c_str(), 0xDE496BFF) == 1);
            assert(CountMaterialTriangles(variant, stems.c_str(), 0xDE496BFF) == 1);
            assert(CountMaterialTriangles(variant, unrelated.c_str()) == 1);
            assert(CountMaterialTriangles(variant, unrelated.c_str(), 0xEF484CFF) == 0);
            assert(CountMaterialTriangles(variant, unrelated.c_str(), 0xF3974FFF) == 0);
        }
    }
    std::puts(
        "PASS mixed terrain/private brush materials in both namespaces and Alt hashes; unrelated draws preserved");
}
static void CheckActualSceneRenderer(const char* path, int sceneId, const std::vector<Gfx>& original,
                                     const std::vector<uint32_t>& expected,
                                     const std::vector<std::string>& texturePaths, int selectedSeason = SEASON_AUTUMN) {
    for (const auto& texture : texturePaths)
        ForestRendererFixture::manager.paths[CRC64(texture.c_str())] = texture;
    for (const bool alt : { false, true }) {
        ClearCache();
        auto rm = std::make_shared<Ship::ResourceManager>();
        Ship::Context::GetRawInstance()->rm = rm;
        auto resource = std::make_shared<Fast::DisplayList>();
        resource->Instructions = original;
        const std::string resourcePath = (alt ? "alt/" : "") + std::string(path);
        rm->archive->files[resourcePath] = resource;
        rm->resources[resourcePath] = resource;
        rm->alt = alt;
        ForestRendererFixture::manager.alt = alt;
        ForestRendererFixture::manager.refreshed = false;
        PlayState play{};
        play.sceneId = sceneId;
        for (const int phase : { selectedSeason, SEASON_OFF, selectedSeason, SEASON_OFF, selectedSeason }) {
            season = phase;
            MMAutumnSceneFoliage_Update(&play);
            const auto stats = ForestRendererFixture::Draw(resource->Instructions);
            assert(stats.palettes == (phase == selectedSeason ? expected : std::vector<uint32_t>(expected.size(), 0)));
        }
    }
    ClearCache();
    season = SEASON_AUTUMN;
    std::printf("PASS actual scene renderer %s: native/Alt tint survives rendered Off frames and pointer writebacks\n",
                path);
}

static std::vector<uint32_t> TrianglePalettes(const std::vector<Gfx>& commands) {
    std::vector<uint32_t> palettes;
    bool tint = false;
    uint32_t color = 0;
    for (size_t i = 0; i < commands.size(); ++i) {
        const auto op = commands[i].words.w0 >> 24;
        if (op == G_SETGRAYSCALE)
            tint = commands[i].words.w1 != 0;
        else if (op == G_SETINTENSITY)
            color = commands[i].words.w1;
        else if (op == G_TRI1 || op == G_TRI2)
            palettes.insert(palettes.end(), op == G_TRI2 ? 2 : 1, tint ? color : 0);
        if (op == G_SETTIMG_OTR_HASH || op == G_VTX_OTR_HASH || op == G_DL_OTR_HASH || op == G_MARKER ||
            op == G_MTX_OTR)
            ++i;
    }
    assert(!tint);
    return palettes;
}

static void CheckActualSceneMaterial(const char* path, int sceneId, const std::vector<Gfx>& original,
                                     const std::vector<uint32_t>& expected, int selectedSeason = SEASON_AUTUMN) {
    const bool selected = std::any_of(expected.begin(), expected.end(), [](uint32_t color) { return color != 0; });
    const auto transformed = BuildVariant(original, selectedSeason, sceneId);
    if (selected && transformed.empty())
        std::fprintf(stderr, "FAIL actual scene material %s: selected grass/forest remains untinted\n", path);
    if (!selected && !transformed.empty())
        std::fprintf(stderr, "FAIL actual scene material %s: unexpected whole-material tint\n", path);
    assert(selected == !transformed.empty());
    if (selected) {
        assert(Equal(RemoveTint(transformed), original));
        assert(TrianglePalettes(transformed) == expected);
    }
    for (const auto* suffix : { "", "_scene" }) {
        for (const bool alt : { false, true }) {
            ClearCache();
            auto rm = std::make_shared<Ship::ResourceManager>();
            Ship::Context::GetRawInstance()->rm = rm;
            std::string resourcePath = path;
            if (resourcePath.starts_with("scenes/nonmq/"))
                resourcePath.insert(resourcePath.find('/', 13), suffix);
            else if (*suffix != '\0')
                continue; // Actor-owned object roots have no generated scene alias.
            if (alt)
                resourcePath.insert(0, "alt/");
            auto resource = std::make_shared<Fast::DisplayList>();
            resource->Instructions = original;
            rm->archive->files[resourcePath] = resource;
            rm->resources[resourcePath] = resource;
            rm->alt = alt;
            PlayState play{};
            play.sceneId = sceneId;
            season = selectedSeason;
            MMAutumnSceneFoliage_Update(&play);
            if (selected) {
                assert(resource->Instructions[0].words.w0 >> 24 == G_DL);
                const auto& variant = CurrentVariants().at(resource.get());
                if (ForestListColor(path))
                    CheckScopedForestCommands(variant, original, selectedSeason);
                else
                    assert(TrianglePalettes(variant.commands) == expected);
            } else {
                assert(Equal(resource->Instructions, original) && CurrentVariants().empty());
            }
            season = SEASON_OFF;
            MMAutumnSceneFoliage_Update(&play);
            assert(Equal(resource->Instructions, original));
            season = selectedSeason;
            MMAutumnSceneFoliage_Update(&play);
            if (selected)
                assert(resource->Instructions[0].words.w0 >> 24 == G_DL);
            else
                assert(Equal(resource->Instructions, original));
        }
    }
    ClearCache();
    season = SEASON_AUTUMN;
}
static void CheckRecoveredLeafMaterials() {
    const struct {
        const char* texture;
        const char* palette;
        const char* owner;
        int scene;
    } cases[] = {
        { "misc/scene_texture_06/scene_texture_06_Tex_000C20", "misc/scene_texture_06/scene_texture_06_TLUT_001420",
          "scenes/nonmq/Z2_20SICHITAI/Z2_20SICHITAI_room_00DL_ForestControl", SCENE_20SICHITAI },
        { "misc/scene_texture_06/scene_texture_06_Tex_001440", "misc/scene_texture_06/scene_texture_06_TLUT_001C40",
          "scenes/nonmq/Z2_20SICHITAI2/Z2_20SICHITAI2_room_00DL_ForestControl", SCENE_20SICHITAI2 },
        { "misc/scene_texture_06/scene_texture_06_Tex_000400", "misc/scene_texture_06/scene_texture_06_TLUT_000C00",
          "scenes/nonmq/Z2_22DEKUCITY/Z2_22DEKUCITY_room_00DL_ForestControl", SCENE_22DEKUCITY },
        { "objects/object_mtoride/gWoodfallSceneryBranchAndLeavesTex",
          "objects/object_mtoride/gWoodfallSceneryBranchAndLeavesTLUT",
          "objects/object_mtoride/gWoodfallSceneryTempleDL", SCENE_21MITURINMAE },
    };
    for (const auto& selected : cases) {
        for (const auto* prefix : { "", "alt/" }) {
            std::vector<Gfx> original;
            const auto image = [&](const std::string& name, uintptr_t word) {
                const auto hash = CRC64(name.c_str());
                original.push_back(Command(word, 0));
                original.push_back(Command(hash >> 32, hash & 0xFFFFFFFF));
            };
            image(std::string(prefix) + selected.palette, 0x20100000);
            original.push_back(Command(0xF50001F0, 0x07000000));
            original.push_back(Command(0xF0000000, 0x0703C000));
            image(std::string(prefix) + selected.texture, 0x20500000);
            original.push_back(Command(0xF5500000, 0x0705C150));
            original.push_back(Command(0xF3000000, 0x073FF400));
            original.push_back(Command(0xF5400400, 0x00F5C150));
            original.push_back(Command(0xF2000000, 0x0007C1FC));
            original.push_back(Command(0xFA000080, 0xFFFFFFFF));
            original.push_back(gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0));
            image(std::string(prefix) + "objects/object_mtoride/gWoodfallSceneryTreeBarkTex", 0x20100000);
            original.push_back(Command(0xF5100000, 0x07054050));
            original.push_back(Command(0xF3000000, 0x073FF100));
            original.push_back(Command(0xF5101000, 0x00054050));
            original.push_back(gsSP1Triangle(0, 1, 2, 0));
            original.push_back(gsSPEndDisplayList());
            for (int selectedSeason : { SEASON_AUTUMN, SEASON_WINTER, SEASON_SPRING }) {
                const uint32_t color = std::string(selected.texture).starts_with("objects/") ? 0
                                       : selectedSeason == SEASON_AUTUMN                     ? 0xEF484CFF
                                       : selectedSeason == SEASON_WINTER                     ? 0xE2EEE9FF
                                                                                             : 0xF6C1D9E0;
                CheckActualSceneMaterial(selected.owner, selected.scene, original, { color, color, 0 }, selectedSeason);
            }
        }
    }
    std::puts("PASS recovered shared forest/upper leaves: native/Alt identities, three seasons and separate bark");
}
static void CheckMiddleForestMaterial() {
    constexpr const char* path = "scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKU_room_00DL_003928";
    constexpr const char* color = "misc/scene_texture_08/scene_texture_08_Tex_005800";
    constexpr const char* mask = "misc/scene_texture_08/scene_texture_08_Tex_005000";
    const std::vector<uint32_t> expected(32, 0xF26846FF);
    assert(CountMaterialTriangles(kActualMiddleForestCommands, mask) == 32);
    for (const bool explicitAlt : { false, true }) {
        auto original = kActualMiddleForestCommands;
        const std::string colorPath = (explicitAlt ? "alt/" : "") + std::string(color);
        const std::string maskPath = (explicitAlt ? "alt/" : "") + std::string(mask);
        for (const auto& [index, texture] : { std::pair{ 12u, colorPath }, std::pair{ 20u, maskPath } }) {
            const auto hash = CRC64(texture.c_str());
            original[index] = Command(hash >> 32, hash & 0xFFFFFFFF);
        }
        // This real two-texture draw must be selected by the primary color
        // image, while preserving the secondary image, combine and alpha words.
        CheckActualSceneMaterial(path, SCENE_00KEIKOKU, original, expected);
        CheckActualSceneRenderer(path, SCENE_00KEIKOKU, original, expected, { colorPath, maskPath });
        const auto unselected = CRC64("scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_02B050");
        original[12] = Command(unselected >> 32, unselected & 0xFFFFFFFF);
        assert(BuildVariant(original).empty()); // A selected secondary mask alone cannot tint other surfaces.
    }
    std::puts(
        "PASS exact 32-face middle forest: primary RGBA plus secondary IA, native/Alt identities and Off restoration");
}
static void CheckBackwallGrassMaterial() {
    constexpr const char* path = "scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKU_room_00DL_027238";
    const std::vector<uint32_t> expected(26, 0xF3974FFF);
    for (const bool explicitAlt : { false, true }) {
        auto original = kActualBackwallGrassCommands;
        std::vector<std::string> textures;
        for (const auto& [index, suffix] : { std::pair{ 12u, "007800" }, std::pair{ 20u, "006800" } }) {
            const std::string texture =
                (explicitAlt ? "alt/" : "") + std::string("misc/scene_texture_08/scene_texture_08_Tex_") + suffix;
            const auto hash = CRC64(texture.c_str());
            original[index] = Command(hash >> 32, hash & 0xFFFFFFFF);
            textures.push_back(texture);
        }
        CheckActualSceneMaterial(path, SCENE_00KEIKOKU, original, expected);
        CheckActualSceneRenderer(path, SCENE_00KEIKOKU, original, expected, textures);
    }
    std::puts("PASS exact 26-face backwall grass: both texture layers preserved, native/Alt and Off restoration");
}
static std::shared_ptr<Fast::Texture> SurfaceTexture() {
    auto texture = std::make_shared<Fast::Texture>();
    texture->Width = 3;
    texture->Height = 1;
    texture->ImageDataSize = 12;
    texture->Flags = 2;
    texture->HByteScale = 2;
    texture->VPixelScale = 4;
    texture->mImageBuffer = std::make_shared<std::vector<char>>();
    for (const uint8_t byte : { 30, 90, 20, 128, 70, 50, 30, 255, 64, 64, 64, 0 })
        texture->mImageBuffer->push_back(static_cast<char>(byte));
    texture->ImageData = reinterpret_cast<uint8_t*>(texture->mImageBuffer->data());
    return texture;
}
static unsigned CheckSurfacePixels(const std::shared_ptr<Fast::Texture>& original,
                                   const std::shared_ptr<Fast::Texture>& copy) {
    assert(copy != original && copy->ImageData != original->ImageData);
    assert(copy->Type == original->Type && copy->Width == original->Width && copy->Height == original->Height &&
           copy->Flags == original->Flags && copy->ImageDataSize == original->ImageDataSize &&
           copy->HByteScale == original->HByteScale && copy->VPixelScale == original->VPixelScale);
    const bool packed = original->Type == Fast::TextureType::RGBA16bpp && original->Flags == 0;
    const unsigned stride = packed ? 2 : 4;
    unsigned changed = 0;
    for (size_t i = 0; i < original->ImageDataSize; i += stride) {
        const auto* source = original->ImageData + i;
        const auto* tinted = copy->ImageData + i;
        const unsigned word = packed ? unsigned(source[0]) << 8 | source[1] : 0;
        const unsigned r = packed ? word >> 11 : source[0];
        const unsigned g = packed ? word >> 6 & 31 : source[1];
        const unsigned b = packed ? word >> 1 & 31 : source[2];
        assert(packed ? (source[1] & 1) == (tinted[1] & 1) : source[3] == tinted[3]);
        if (g <= r || g <= b)
            assert(std::equal(source, source + stride, tinted));
        changed += !std::equal(source, source + stride, tinted);
    }
    assert(changed != 0);
    return changed;
}
static void CheckWoodfallSurface(const char* logicalPath, const std::shared_ptr<Fast::Texture>& supplied,
                                 int targetScene = SCENE_21MITURINMAE, const char* ownerPath = nullptr,
                                 const std::vector<Gfx>* ownerCommands = nullptr) {
    const bool scenePath = std::string(logicalPath).starts_with("scenes/nonmq/");
    for (const auto* suffix : { "", "_scene" }) {
        if (!scenePath && *suffix != '\0')
            continue;
        for (const auto* prefix : { "", "alt/" }) {
            ClearCache();
            auto rm = std::make_shared<Ship::ResourceManager>();
            Ship::Context::GetRawInstance()->rm = rm;
            rm->alt = *prefix != '\0';
            std::string path = logicalPath;
            if (scenePath)
                path.insert(path.find('/', 13), suffix);
            path.insert(0, prefix);
            auto source = supplied;
            const auto original = *source->mImageBuffer;
            rm->resources[path] = source;
            if (ownerPath != nullptr) {
                auto owner = std::make_shared<Fast::DisplayList>();
                owner->Instructions = *ownerCommands;
                const auto resolved = std::string(prefix) + ownerPath;
                rm->resources[resolved] = owner;
                rm->archive->files[resolved] = owner;
            }
            auto unrelated = SurfaceTexture();
            rm->resources[path + "_unrelated"] = unrelated;
            PlayState play{};
            play.sceneId = targetScene;
            for (int selectedSeason : { SEASON_AUTUMN, SEASON_WINTER, SEASON_SPRING }) {
                season = selectedSeason;
                MMAutumnSceneFoliage_Update(&play);
                auto copy = std::dynamic_pointer_cast<Fast::Texture>(rm->resources.at(path));
                if (copy == source)
                    std::fprintf(stderr, "FAIL Woodfall surface %s: green pixels still use the original texture\n",
                                 path.c_str());
                CheckSurfacePixels(source, copy);
                assert(*source->mImageBuffer == original && rm->resources.at(path + "_unrelated") == unrelated);
                for (unsigned i = 0; i < 100; ++i)
                    MMAutumnSceneFoliage_Update(&play);
                assert(rm->resources.at(path) == copy && rm->loads == 0);
                for (int inactive : { SEASON_OFF, SEASON_SUMMER, -1 }) {
                    season = inactive;
                    MMAutumnSceneFoliage_Update(&play);
                    assert(rm->resources.at(path) == source && *source->mImageBuffer == original);
                    season = selectedSeason;
                    MMAutumnSceneFoliage_Update(&play);
                    assert(rm->resources.at(path) == copy);
                }
                play.sceneId = SCENE_BACKTOWN;
                MMAutumnSceneFoliage_Update(&play);
                assert(rm->resources.at(path) == source);
                play.sceneId = targetScene;
                MMAutumnSceneFoliage_Update(&play);
                assert(rm->resources.at(path) == copy);
            }
            if (*prefix != '\0') {
                rm->alt = false;
                MMAutumnSceneFoliage_Update(&play);
                assert(rm->resources.at(path) == source);
            }
            auto replacement = SurfaceTexture();
            rm->resources[path] = replacement;
            MMAutumnSceneFoliage_Reset();
            assert(rm->resources.at(path) == replacement && *source->mImageBuffer == original);
        }
    }
    ClearCache();
    season = SEASON_AUTUMN;
    std::printf(
        "PASS Woodfall selective surface %s: three seasons, native/Alt aliases, exact non-green/alpha and Off\n",
        logicalPath);
}
static std::shared_ptr<Fast::Texture> ReadActualTexture(const char* file, unsigned type, unsigned width,
                                                        unsigned height, unsigned flags, float hScale, float vScale) {
    std::ifstream input(file, std::ios::binary);
    assert(input);
    auto texture = std::make_shared<Fast::Texture>();
    texture->Type = static_cast<Fast::TextureType>(type);
    texture->Width = width;
    texture->Height = height;
    texture->Flags = flags;
    texture->HByteScale = hScale;
    texture->VPixelScale = vScale;
    texture->mImageBuffer =
        std::make_shared<std::vector<char>>(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    texture->ImageDataSize = texture->mImageBuffer->size();
    texture->ImageData = reinterpret_cast<uint8_t*>(texture->mImageBuffer->data());
    return texture;
}
using TestPixel = std::array<unsigned, 4>;
static std::vector<TestPixel> TexturePixels(const std::shared_ptr<Fast::Texture>& texture) {
    std::vector<TestPixel> pixels;
    const unsigned type = static_cast<unsigned>(texture->Type);
    const auto* data = texture->ImageData;
    for (size_t i = 0; i < size_t(texture->Width) * texture->Height; ++i) {
        if (type == 1) {
            pixels.push_back({ data[i * 4], data[i * 4 + 1], data[i * 4 + 2], data[i * 4 + 3] });
        } else if (type == 2) {
            const unsigned value = unsigned(data[i * 2]) << 8 | data[i * 2 + 1];
            pixels.push_back({ (value >> 11) * 255 / 31, (value >> 6 & 31) * 255 / 31, (value >> 1 & 31) * 255 / 31,
                               (value & 1) * 255 });
        } else {
            assert(type == 5 || type == 7); // Actual native I4/IA4 companion inputs.
            const unsigned value = i % 2 == 0 ? data[i / 2] >> 4 : data[i / 2] & 15;
            const unsigned intensity = type == 5 ? value * 17 : (value >> 1) * 255 / 7;
            pixels.push_back({ intensity, intensity, intensity, type == 5 ? intensity : (value & 1) * 255 });
        }
    }
    return pixels;
}
namespace WoodfallPaletteFixture {
struct State {
    struct {
        uint8_t siz;
        const uint8_t* addr;
    } texture_to_load{};
    struct {
        uint16_t tmem;
    } texture_tile[8]{};
    uint8_t palette_staging[2][256]{};
    const uint8_t* palettes[2]{};
    const uint8_t* palette_dram_addr[2]{};
};
struct Interpreter {
    State state;
    State* mRdp = &state;
    void GfxDpLoadTlut(uint8_t tile, uint32_t high_index);
};
#define SUPPORT_CHECK(condition) assert(condition)
#include "autumn_scene_palette_production.inc"
#undef SUPPORT_CHECK
} // namespace WoodfallPaletteFixture
static void CheckWoodfallMixedLeaves(const std::shared_ptr<Fast::Texture>& palette,
                                     const std::vector<uint8_t>& indices) {
    const char* imagePath = "objects/object_mtoride/gWoodfallSceneryBranchAndLeavesTex";
    const char* palettePath = "objects/object_mtoride/gWoodfallSceneryBranchAndLeavesTLUT";
    // A whole-triangle grayscale wrapper changes the brown stem too.
    auto material = MakeMaterial(imagePath);
    material.push_back(gsSPEndDisplayList());
    for (int selected : { SEASON_AUTUMN, SEASON_WINTER, SEASON_SPRING })
        assert(BuildVariant(material, selected, SCENE_21MITURINMAE).empty());
    const auto originalPalette = *palette->mImageBuffer;
    for (const char* prefix : { "", "alt/" }) {
        ClearCache();
        auto rm = std::make_shared<Ship::ResourceManager>();
        Ship::Context::GetRawInstance()->rm = rm;
        rm->alt = true;
        const auto path = std::string(prefix) + palettePath;
        rm->resources[path] = palette;
        PlayState play{};
        play.sceneId = SCENE_21MITURINMAE;
        for (int selected : { SEASON_AUTUMN, SEASON_WINTER, SEASON_SPRING, SEASON_OFF }) {
            season = selected;
            MMAutumnSceneFoliage_Update(&play);
            const auto copy = std::dynamic_pointer_cast<Fast::Texture>(rm->resources.at(path));
            if (selected == SEASON_OFF) {
                assert(copy == palette);
            } else {
                CheckSurfacePixels(palette, copy);
            }
            WoodfallPaletteFixture::Interpreter renderer;
            renderer.state.texture_to_load = { G_IM_SIZ_16b, copy->ImageData };
            renderer.state.texture_tile[7].tmem = 496; // Native CI4 palette 15.
            renderer.GfxDpLoadTlut(7, 15);
            assert(renderer.state.palette_dram_addr[1] == copy->ImageData);
            const uint8_t* uploaded = renderer.state.palettes[1] + 224;
            unsigned green = 0, brown = 0;
            for (uint8_t byte : indices) {
                for (unsigned index : { unsigned(byte >> 4), unsigned(byte & 15) }) {
                    const unsigned original =
                        unsigned(palette->ImageData[index * 2]) << 8 | palette->ImageData[index * 2 + 1];
                    const unsigned observed = unsigned(uploaded[index * 2]) << 8 | uploaded[index * 2 + 1];
                    if (!(original & 1))
                        continue;
                    const unsigned r = original >> 11, g = original >> 6 & 31, b = original >> 1 & 31;
                    if (g > r && g > b) {
                        ++green;
                        if (selected != SEASON_OFF)
                            assert(observed != original);
                    } else {
                        ++brown;
                        assert(observed == original);
                    }
                    assert((original & 1) == (observed & 1));
                }
            }
            assert(green != 0 && brown != 0 && *palette->mImageBuffer == originalPalette);
        }
        ClearCache();
    }
    season = SEASON_AUTUMN;
    std::puts(
        "PASS Woodfall mixed CI4 leaves: green-only palette, brown stem/alpha/source exact, native TLUT load and Off");
}
static void CheckWoodfallMixedLeaves() {
    auto palette = std::make_shared<Fast::Texture>();
    palette->Type = Fast::TextureType::RGBA16bpp;
    palette->Width = palette->Height = 4;
    palette->ImageDataSize = 32;
    palette->mImageBuffer = std::make_shared<std::vector<char>>(32, 0);
    // Representative native green leaf and opaque brown stem entries.
    for (const auto& [index, value] : { std::pair{ 1u, uint16_t((5 << 11) | (18 << 6) | (5 << 1) | 1) },
                                        std::pair{ 9u, uint16_t((9 << 11) | (6 << 6) | (3 << 1) | 1) } }) {
        (*palette->mImageBuffer)[index * 2] = value >> 8;
        (*palette->mImageBuffer)[index * 2 + 1] = value & 255;
    }
    palette->ImageData = reinterpret_cast<uint8_t*>(palette->mImageBuffer->data());
    CheckWoodfallMixedLeaves(palette, { 0x19, 0x91 });
    auto raw = SurfaceTexture();
    raw->Type = Fast::TextureType::RGBA16bpp;
    raw->Flags = TEX_FLAG_LOAD_AS_RAW;
    CheckWoodfallSurface("objects/object_mtoride/gWoodfallSceneryBranchAndLeavesTex", raw);
    ClearCache();
    auto rm = std::make_shared<Ship::ResourceManager>();
    Ship::Context::GetRawInstance()->rm = rm;
    const char* wall = "objects/object_mtoride/gWoodfallSceneryPurifiedWalls2Tex";
    const char* leaves = "objects/object_mtoride/gWoodfallSceneryBranchAndLeavesTex";
    rm->resources[wall] = rm->resources[leaves] = raw;
    PlayState play{};
    play.sceneId = SCENE_21MITURINMAE;
    season = SEASON_SPRING;
    MMAutumnSceneFoliage_Update(&play);
    auto wallCopy = std::dynamic_pointer_cast<Fast::Texture>(rm->resources.at(wall));
    auto leafCopy = std::dynamic_pointer_cast<Fast::Texture>(rm->resources.at(leaves));
    CheckSurfacePixels(raw, wallCopy);
    CheckSurfacePixels(raw, leafCopy);
    assert(wallCopy != leafCopy && leafCopy->ImageData[0] > wallCopy->ImageData[0]);
    season = SEASON_OFF;
    MMAutumnSceneFoliage_Update(&play);
    assert(rm->resources.at(wall) == raw && rm->resources.at(leaves) == raw);
    ClearCache();
    season = SEASON_AUTUMN;
    std::puts("PASS shared Woodfall RGBA aliases keep distinct blossom brightness and exact Off restoration");
}
static void CheckActualSurfaceDraw(const char* ownerPath, int sceneId, const std::vector<Gfx>& original,
                                   const char* surfacePath, const std::shared_ptr<Fast::Texture>& surface,
                                   const char* companionPath, const std::shared_ptr<Fast::Texture>& companion,
                                   unsigned triangles, bool surfaceFirst) {
    // The actual two-cycle RGB equations are (TEXEL1 * TEXEL0) * SHADE
    // for walls/Road, and (TEXEL1 * TEXEL0_ALPHA) * SHADE for the I4 detail.
    const auto combine = std::find_if(original.begin(), original.end(),
                                      [](const Gfx& command) { return command.words.w0 >> 24 == G_SETCOMBINE; });
    assert(combine != original.end() && combine->words.w1 == 0xFF0F93FF);
    assert(combine->words.w0 == 0xFC20AC04 || combine->words.w0 == 0xFC242C04);
    const bool intensityAlpha = combine->words.w0 == 0xFC242C04;
    const auto sourcePixels = TexturePixels(surface);
    const auto companionPixels = TexturePixels(companion);
    if (intensityAlpha)
        for (const auto& pixel : companionPixels)
            assert(pixel[0] == pixel[1] && pixel[1] == pixel[2]);
    const std::set<TestPixel> companionSamples(companionPixels.begin(), companionPixels.end());
    const auto sourceBytes = *surface->mImageBuffer;
    const auto companionBytes = *companion->mImageBuffer;
    for (bool alt : { false, true }) {
        ClearCache();
        auto rm = std::make_shared<Ship::ResourceManager>();
        Ship::Context::GetRawInstance()->rm = rm;
        rm->alt = alt;
        const auto prefix = std::string(alt ? "alt/" : "");
        auto root = std::make_shared<Fast::DisplayList>();
        root->Instructions = original;
        rm->resources[prefix + ownerPath] = root;
        rm->archive->files[prefix + ownerPath] = root;
        rm->resources[prefix + surfacePath] = surface;
        rm->resources[prefix + companionPath] = companion;
        for (const auto* path : { surfacePath, companionPath })
            ForestRendererFixture::manager.paths[CRC64(path)] = path;
        PlayState play{};
        play.sceneId = sceneId;
        for (int selectedSeason : { SEASON_AUTUMN, SEASON_OFF, SEASON_WINTER, SEASON_SPRING, SEASON_SUMMER, -1 }) {
            season = selectedSeason;
            MMAutumnSceneFoliage_Update(&play);
            const auto stats = ForestRendererFixture::Draw(root->Instructions);
            assert(stats.triangles == triangles && stats.palettes == std::vector<uint32_t>(triangles, 0));
            assert(stats.textures.size() == 2);
            const auto shown = stats.textures[surfaceFirst ? 0 : 1];
            assert(stats.textures[surfaceFirst ? 1 : 0] == companion);
            const bool active =
                selectedSeason == SEASON_AUTUMN || selectedSeason == SEASON_WINTER || selectedSeason == SEASON_SPRING;
            if (active)
                CheckSurfacePixels(surface, shown);
            else
                assert(shown == surface);
            assert(*surface->mImageBuffer == sourceBytes && *companion->mImageBuffer == companionBytes);
            const auto shownPixels = TexturePixels(shown);
            unsigned changedOutput = 0, protectedPixels = 0;
            for (size_t i = 0; i < sourcePixels.size(); ++i) {
                const auto& before = sourcePixels[i];
                const auto& after = shownPixels[i];
                const bool green = before[1] > before[0] && before[1] > before[2];
                protectedPixels += !green;
                assert(before[3] == after[3]);
                for (const auto& sample : companionSamples) {
                    bool changed = false;
                    for (unsigned channel = 0; channel < 3; ++channel) {
                        const unsigned factor = intensityAlpha ? sample[3] : sample[channel];
                        const unsigned oldRgb = before[channel] * factor / 255;
                        const unsigned newRgb = after[channel] * factor / 255;
                        if (!green)
                            assert(oldRgb == newRgb); // Rendered bark/stone, not just original byte buffers.
                        changed |= oldRgb != newRgb;
                    }
                    changedOutput += changed;
                }
            }
            assert(protectedPixels != 0 && (active ? changedOutput != 0 : changedOutput == 0));
            auto observed = root->Instructions;
            for (const auto& slot : RendererPointerSlots(original))
                observed[slot.index].words.w1 = original[slot.index].words.w1;
            assert(Equal(observed, original)); // Only the renderer's image/VTX pointer writebacks differ.
        }
    }
    ClearCache();
    season = SEASON_AUTUMN;
    std::printf(
        "PASS actual surface renderer %s: %u triangles; green output changes, non-green/alpha/metadata and Off exact\n",
        ownerPath, triangles);
}
static void CheckWoodfallSurfaces() {
    for (const char* path : { "objects/object_mtoride/gWoodfallSceneryPurifiedWalls2Tex",
                              "scenes/nonmq/Z2_21MITURINMAE/Z2_21MITURINMAETex_005190" })
        CheckWoodfallSurface(path, SurfaceTexture());
}
static void CheckRoadTrim() {
    const char* owner = "scenes/nonmq/Z2_24KEMONOMITI/Z2_24KEMONOMITI_room_00DL_005FF0";
    for (const char* prefix : { "", "alt/" }) {
        auto original =
            MakeMaterial((std::string(prefix) + "misc/scene_texture_08/scene_texture_08_Tex_007000").c_str());
        original.push_back(gsSPEndDisplayList());
        for (const auto& [selected, color] :
             { std::pair{ SEASON_AUTUMN, 0xF3974FFFu }, std::pair{ SEASON_WINTER, 0xDCECF5FFu },
               std::pair{ SEASON_SPRING, 0xF6C1D9FFu } })
            CheckActualSceneMaterial(owner, SCENE_24KEMONOMITI, original, { color }, selected);
    }
    assert(MaterialColor(CRC64("misc/scene_texture_08/scene_texture_08_Tex_001000"), SEASON_SPRING,
                         SCENE_24KEMONOMITI) == 0); // Ground uses green-only texture copies, never a whole-draw tint.
    assert(MaterialColor(CRC64("misc/scene_texture_08/scene_texture_08_Tex_007000"), SEASON_AUTUMN,
                         SCENE_ROMANYMAE) == 0); // This POC does not reclassify Milk Road's shared material.
    std::puts("PASS Road decorative grass trim: three palettes, native/Alt and both scene namespaces");
}

static std::vector<TestPixel> IndexedPixels(const std::shared_ptr<Fast::Texture>& source,
                                            const std::shared_ptr<Fast::Texture>& palette) {
    const auto colors = TexturePixels(palette);
    std::vector<TestPixel> pixels;
    const bool ci4 = source->Type == Fast::TextureType::Palette4bpp;
    for (size_t i = 0; i < size_t(source->Width) * source->Height; ++i) {
        const unsigned byte = source->ImageData[ci4 ? i / 2 : i];
        pixels.push_back(colors.at(ci4 ? (i % 2 == 0 ? byte >> 4 : byte & 15) : byte));
    }
    return pixels;
}

namespace RoadVineUploadFixture {
static void CheckIndexedGroundUpload(const std::shared_ptr<Fast::Texture>& image, bool ci4);
}
static void CheckSpringIndexedGround(const char* imagePath, const char* palettePath,
                                     const std::shared_ptr<Fast::Texture>& source,
                                     const std::shared_ptr<Fast::Texture>& palette) {
    source->initData = std::make_shared<Ship::ResourceInitData>();
    source->initData->Path = imagePath;
    const auto original = IndexedPixels(source, palette);
    const auto sourceBytes = *source->mImageBuffer, paletteBytes = *palette->mImageBuffer;
    for (bool alt : { false, true })
        for (const char* suffix : { "", "_scene" }) {
            ClearCache();
            auto rm = std::make_shared<Ship::ResourceManager>();
            Ship::Context::GetRawInstance()->rm = rm;
            rm->alt = alt;
            const auto resolve = [&](std::string path) {
                path.insert(path.find('/', 13), suffix);
                return std::string(alt ? "alt/" : "") + path;
            };
            const auto image = resolve(imagePath), colors = resolve(palettePath);
            rm->resources[image] = source;
            PlayState play{};
            play.sceneId = SCENE_00KEIKOKU;
            season = SEASON_SPRING;
            MMAutumnSceneFoliage_Update(&play);
            assert(rm->resources.at(image) == source); // Wait for the real palette, without cold-loading it.
            rm->resources[colors] = palette;
            MMAutumnSceneFoliage_Update(&play);
            const auto shown = std::dynamic_pointer_cast<Fast::Texture>(rm->resources.at(image));
            if (shown == source)
                std::fprintf(stderr, "FAIL indexed spring grass still matches summer: %s\n", imagePath);
            assert(shown != source && shown->Type == Fast::TextureType::RGBA32bpp);
            assert(shown->HByteScale == source->HByteScale * (source->Type == Fast::TextureType::Palette4bpp ? 8 : 4));
            assert(shown->VPixelScale == source->VPixelScale);
            RoadVineUploadFixture::CheckIndexedGroundUpload(shown, source->Type == Fast::TextureType::Palette4bpp);
            const auto tinted = TexturePixels(shown);
            assert(tinted.size() == original.size());
            unsigned changed = 0;
            for (size_t i = 0; i < original.size(); ++i) {
                const auto before = original[i], after = tinted[i];
                assert(after[3] == before[3]);
                if (before[1] > before[0] && before[1] > before[2]) {
                    assert(after[0] > after[1] && after[2] >= after[1]);
                    changed += after != before;
                } else {
                    assert(after == before);
                }
            }
            assert(changed && rm->resources.at(colors) == palette);
            assert(*source->mImageBuffer == sourceBytes && *palette->mImageBuffer == paletteBytes);
            for (int inactive : { SEASON_SUMMER, SEASON_OFF }) {
                season = inactive;
                MMAutumnSceneFoliage_Update(&play);
                assert(rm->resources.at(image) == source && rm->resources.at(colors) == palette);
            }
            assert(rm->loads == 0);
        }
    ClearCache();
    season = SEASON_AUTUMN;
    std::printf(
        "PASS actual indexed spring ground %s: decoded blush, exact non-green/alpha/shared palette and restore\n",
        imagePath);
}

static void CheckRoadVines(const std::shared_ptr<Fast::Texture>& source,
                           const std::shared_ptr<Fast::Texture>& palette) {
    const auto decoded = IndexedPixels(source, palette);
    const auto imageBytes = *source->mImageBuffer, paletteBytes = *palette->mImageBuffer;
    for (const char* suffix : { "", "_scene" })
        for (bool alt : { false, true }) {
            ClearCache();
            auto rm = std::make_shared<Ship::ResourceManager>();
            Ship::Context::GetRawInstance()->rm = rm;
            rm->alt = alt;
            const std::string folder = std::string(alt ? "alt/" : "") + "scenes/nonmq/Z2_24KEMONOMITI" + suffix + "/";
            const auto path = folder + "Z2_24KEMONOMITITex_005730";
            const auto palettePath = folder + "Z2_24KEMONOMITITLUT_007950";
            rm->resources[path] = source;
            rm->resources[palettePath] = palette;
            PlayState play{};
            play.sceneId = SCENE_24KEMONOMITI;
            for (int selected : { SEASON_AUTUMN, SEASON_WINTER, SEASON_SPRING, SEASON_SUMMER, SEASON_OFF, -1 }) {
                season = selected;
                MMAutumnSceneFoliage_Update(&play);
                const auto copy = std::dynamic_pointer_cast<Fast::Texture>(rm->resources.at(path));
                const bool active = selected == SEASON_AUTUMN || selected == SEASON_WINTER || selected == SEASON_SPRING;
                if (active) {
                    if (copy == source)
                        std::fprintf(stderr, "FAIL Road climbing vines %s: native green image is unchanged\n",
                                     path.c_str());
                    assert(copy != source && copy->Type == Fast::TextureType::RGBA32bpp);
                    assert(copy->Flags == TEX_FLAG_LOAD_AS_RAW && copy->Width == source->Width &&
                           copy->Height == source->Height && copy->HByteScale == source->HByteScale * 4 &&
                           copy->VPixelScale == source->VPixelScale);
                    const auto shown = TexturePixels(copy);
                    assert(shown.size() == decoded.size());
                    unsigned green = 0, protectedPixels = 0;
                    for (size_t i = 0; i < decoded.size(); ++i) {
                        const auto& before = decoded[i];
                        assert(shown[i][3] == before[3]);
                        if (before[1] > before[0] && before[1] > before[2]) {
                            ++green;
                            assert(shown[i] != before);
                            if (selected == SEASON_SPRING && before == TestPixel{ 41, 148, 41, 255 })
                                assert((shown[i] == TestPixel{ 155, 121, 136, 255 }));
                        } else {
                            ++protectedPixels;
                            assert(shown[i] == before);
                        }
                    }
                    assert(green && protectedPixels);
                    MMAutumnSceneFoliage_Update(&play);
                    assert(rm->resources.at(path) == copy); // One immutable copy, no repeated conversion.
                } else {
                    assert(copy == source);
                }
                assert(rm->resources.at(palettePath) == palette && *palette->mImageBuffer == paletteBytes &&
                       *source->mImageBuffer == imageBytes); // Shop/bark also use this shared palette.
            }
            // A palette replacement must invalidate the image's converted copy.
            season = SEASON_WINTER;
            MMAutumnSceneFoliage_Update(&play);
            const auto oldCopy = rm->resources.at(path);
            auto replacement = std::make_shared<Fast::Texture>(*palette);
            replacement->mImageBuffer = std::make_shared<std::vector<char>>(paletteBytes);
            replacement->ImageData = reinterpret_cast<uint8_t*>(replacement->mImageBuffer->data());
            rm->resources[palettePath] = replacement;
            MMAutumnSceneFoliage_Update(&play);
            assert(rm->resources.at(path) != oldCopy && rm->resources.at(palettePath) == replacement);
            play.sceneId = SCENE_00KEIKOKU;
            MMAutumnSceneFoliage_Update(&play);
            assert(rm->resources.at(path) == source);
        }
    ClearCache();
    season = SEASON_AUTUMN;
    std::puts("PASS Road CI8 vines: exact decoded wood/alpha, unchanged shared palette, aliases and palette reload");
}

static void CheckRoadVines() {
    auto source = std::make_shared<Fast::Texture>();
    source->Type = Fast::TextureType::Palette8bpp;
    source->Width = 2;
    source->Height = 1;
    source->ImageDataSize = 2;
    source->mImageBuffer = std::make_shared<std::vector<char>>(std::initializer_list<char>{ 1, 2 });
    source->ImageData = reinterpret_cast<uint8_t*>(source->mImageBuffer->data());
    auto palette = std::make_shared<Fast::Texture>();
    palette->Type = Fast::TextureType::RGBA16bpp;
    palette->Width = palette->Height = 16;
    palette->ImageDataSize = 512;
    palette->mImageBuffer = std::make_shared<std::vector<char>>(512, 0);
    for (const auto& [index, value] :
         { std::pair{ 1, (5 << 11) | (18 << 6) | (5 << 1) | 1 }, std::pair{ 2, (9 << 11) | (6 << 6) | (3 << 1) } }) {
        (*palette->mImageBuffer)[index * 2] = value >> 8;
        (*palette->mImageBuffer)[index * 2 + 1] = value & 255;
    }
    palette->ImageData = reinterpret_cast<uint8_t*>(palette->mImageBuffer->data());
    CheckRoadVines(source, palette);
    CheckWoodfallSurface("scenes/nonmq/Z2_24KEMONOMITI/Z2_24KEMONOMITITex_005730", SurfaceTexture(),
                         SCENE_24KEMONOMITI);
    ClearCache();
    auto rm = std::make_shared<Ship::ResourceManager>();
    Ship::Context::GetRawInstance()->rm = rm;
    const std::string imagePath = "scenes/nonmq/Z2_24KEMONOMITI/Z2_24KEMONOMITITex_005730";
    const std::string palettePath = "scenes/nonmq/Z2_24KEMONOMITI/Z2_24KEMONOMITITLUT_007950";
    rm->resources[imagePath] = source;
    rm->resources[palettePath] = palette;
    rm->alt = true;
    rm->archive->files["alt/" + palettePath] = palette;
    PlayState play{};
    play.sceneId = SCENE_24KEMONOMITI;
    season = SEASON_WINTER;
    MMAutumnSceneFoliage_Update(&play);
    assert(rm->resources.at(imagePath) == source); // The cold Alt palette must win over native fallback.
    rm->resources["alt/" + palettePath] = palette;
    MMAutumnSceneFoliage_Update(&play);
    assert(rm->resources.at(imagePath) != source && rm->resources.at(palettePath) == palette);
    auto outside = SurfaceTexture();
    rm->resources[imagePath] = outside;
    MMAutumnSceneFoliage_Reset();
    assert(rm->resources.at(imagePath) == outside);
    rm->alt = false;
    // Dark HD-style green: the full existing blush produces a clear red/blue
    // separation, while the brown/neutral controls and every alpha stay exact.
    outside->ImageData[0] = 20;
    outside->ImageData[1] = 65;
    outside->ImageData[2] = 12;
    season = SEASON_SPRING;
    MMAutumnSceneFoliage_Update(&play);
    const auto blush = std::dynamic_pointer_cast<Fast::Texture>(rm->resources.at(imagePath));
    assert(blush->ImageData[0] == 65 && blush->ImageData[1] == 51 && blush->ImageData[2] == 57 &&
           blush->ImageData[3] == 128);
    season = SEASON_SUMMER;
    MMAutumnSceneFoliage_Update(&play);
    assert(rm->resources.at(imagePath) == outside);
    ClearCache();
    season = SEASON_AUTUMN;
}

static void CheckRoadRandoGrassTarget(const char* path, const std::vector<Gfx>& original, unsigned triangles,
                                      bool foliage, bool cold = false, int targetScene = SCENE_24KEMONOMITI) {
    const int previousSeason = season;
    for (bool alt : { false, true }) {
        ClearCache();
        auto rm = std::make_shared<Ship::ResourceManager>();
        Ship::Context::GetRawInstance()->rm = rm;
        rm->alt = alt;
        const auto resolved = std::string(alt ? "alt/" : "") + path;
        auto resource = std::make_shared<Fast::DisplayList>();
        resource->Instructions = original;
        rm->archive->files[resolved] = resource;
        if (!cold)
            rm->resources[resolved] = resource;
        PlayState play{};
        play.sceneId = targetScene;
        for (const auto& [selected, color] :
             { std::pair{ SEASON_AUTUMN, 0xF26846FFu }, std::pair{ SEASON_WINTER, 0xDCECF5FFu },
               std::pair{ SEASON_SPRING, 0xF6C1D9FFu } }) {
            season = selected;
            MMAutumnSceneFoliage_Update(&play);
            if (foliage) {
                if (Equal(resource->Instructions, original))
                    std::fprintf(stderr, "FAIL Road randomizer grass %s: selected foliage draw is unchanged\n", path);
                assert(!Equal(resource->Instructions, original));
                const auto& variant = CurrentVariants().at(resource.get());
                assert(Equal(variant.draw, original));
                assert(variant.commands[0].words.w1 == (selected == SEASON_AUTUMN ? 2 : 3));
                // Grouped-grass hooks enter with an outer actor tint; the leaf
                // wrapper must set one palette and leave the reward flower native.
                std::vector<Gfx> parent{ gsDPSetGrayscaleColor(color >> 24, color >> 16 & 255, color >> 8 & 255,
                                                               color & 255),
                                         gsSPGrayscale(selected == SEASON_AUTUMN ? 2 : 3),
                                         gsSPDisplayList(resource->Instructions.data()),
                                         gsSP1Triangle(0, 1, 2, 0),
                                         gsSPGrayscale(false),
                                         gsSPEndDisplayList() };
                const auto stats = ForestRendererFixture::Draw(parent);
                assert(stats.triangles == triangles + 1 && stats.palettes.back() == 0);
                assert(std::all_of(stats.palettes.begin(), stats.palettes.end() - 1,
                                   [&](uint32_t seen) { return seen == color; }));
            } else {
                assert(Equal(resource->Instructions, original));
            }
            for (int inactive : { SEASON_OFF, SEASON_SUMMER, -1 }) {
                season = inactive;
                MMAutumnSceneFoliage_Update(&play);
                assert(Equal(resource->Instructions, original));
            }
        }
        if (targetScene != SCENE_24KEMONOMITI) {
            for (int selected : { SEASON_AUTUMN, SEASON_SPRING, SEASON_WINTER, SEASON_SPRING }) {
                season = selected;
                MMAutumnSceneFoliage_Update(&play);
                assert(Equal(resource->Instructions, original) == !foliage);
            }
        }
        season = SEASON_WINTER;
        play.sceneId = -1;
        MMAutumnSceneFoliage_Update(&play);
        assert(Equal(resource->Instructions, original));
        assert(rm->loads == (cold && foliage ? 1u : 0u));
    }
    ClearCache();
    season = previousSeason;
}

static void CheckRoadRandoGrass() {
    const std::vector<Gfx> triangles{ gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0), gsSPEndDisplayList() };
    for (const char* path : { "objects/gameplay_field_keep/gFieldBushRandomDL_tri_0",
                              "objects/gameplay_field_keep/gFieldBushRandomDL_tri_1",
                              "objects/gameplay_keep/gRandoCuttableGrassRandomDL_tri_0",
                              "objects/gameplay_keep/gRandoCuttableGrassRandomDL_tri_1" })
        for (bool cold : { false, true })
            CheckRoadRandoGrassTarget(path, triangles, 2, true, cold);
    for (const char* path : { "objects/gameplay_field_keep/gFieldBushRandomDL_tri_2",
                              "objects/gameplay_keep/gRandoCuttableGrassRandomDL_tri_2",
                              "objects/gameplay_keep/gRandoCuttableGrassRandomDL_tri_3",
                              "objects/gameplay_keep/gRandoCuttableGrassPrivateDL_tri_0" })
        for (bool cold : { false, true })
            CheckRoadRandoGrassTarget(path, triangles, 2, false, cold);
    for (int scene : { SCENE_00KEIKOKU, SCENE_BACKTOWN, SCENE_20SICHITAI2 })
        for (bool cold : { false, true }) {
            CheckRoadRandoGrassTarget("objects/gameplay_keep/gRandoCuttableGrassRandomDL_tri_0", triangles, 2, true,
                                      cold, scene);
            CheckRoadRandoGrassTarget("objects/gameplay_keep/gRandoCuttableGrassRandomDL_tri_2", triangles, 2, false,
                                      cold, scene);
        }
    std::puts(
        "PASS randomizer foliage-only children: three seasons across outdoor scenes, one tint and native flower/stem");
}

#include "autumn_road_vine_upload_fixture.h"
static void CheckActualRoadVineDraw(const std::shared_ptr<Fast::Texture>& source,
                                    const std::shared_ptr<Fast::Texture>& palette, const std::vector<Gfx>& original) {
    const std::string folder = "scenes/nonmq/Z2_24KEMONOMITI/";
    const auto path = folder + "Z2_24KEMONOMITITex_005730";
    const auto palettePath = folder + "Z2_24KEMONOMITITLUT_007950";
    const auto owner = folder + "Z2_24KEMONOMITI_room_00DL_003CE0";
    source->initData = std::make_shared<Ship::ResourceInitData>();
    source->initData->Path = path;
    const auto paletteBytes = *palette->mImageBuffer;
    for (bool alt : { false, true }) {
        ClearCache();
        auto rm = std::make_shared<Ship::ResourceManager>();
        Ship::Context::GetRawInstance()->rm = rm;
        rm->alt = alt;
        const std::string prefix = alt ? "alt/" : "";
        auto root = std::make_shared<Fast::DisplayList>();
        root->Instructions = original;
        rm->resources[prefix + owner] = root;
        rm->archive->files[prefix + owner] = root;
        rm->resources[prefix + path] = source;
        rm->resources[prefix + palettePath] = palette;
        ForestRendererFixture::manager.paths[CRC64(path.c_str())] = path;
        ForestRendererFixture::manager.paths[CRC64(palettePath.c_str())] = palettePath;
        PlayState play{};
        play.sceneId = SCENE_24KEMONOMITI;
        for (int selected : { SEASON_OFF, SEASON_AUTUMN, SEASON_WINTER, SEASON_SPRING, SEASON_SUMMER, -1 }) {
            season = selected;
            MMAutumnSceneFoliage_Update(&play);
            const auto stats = ForestRendererFixture::Draw(root->Instructions);
            assert(stats.triangles == 6 && stats.palettes == std::vector<uint32_t>(6, 0));
            assert(stats.textures.size() == 2 && stats.textures[0] == palette);
            if (selected == SEASON_AUTUMN || selected == SEASON_WINTER || selected == SEASON_SPRING)
                RoadVineUploadFixture::CheckUpload(stats.textures[1], original);
            else
                assert(stats.textures[1] == source);
            assert(*palette->mImageBuffer == paletteBytes);
            auto observed = root->Instructions;
            for (const auto& slot : RendererPointerSlots(original))
                observed[slot.index].words.w1 = original[slot.index].words.w1;
            assert(Equal(observed, original));
        }
    }
    ClearCache();
    std::puts("PASS exact six-triangle Road vine draw: production CI8 load/raw upload dimensions, source GBI and "
              "shared palette");
}

static void CheckRoadForestPair() {
    for (const auto* prefix : { "", "alt/" }) {
        const auto shared = std::string(prefix) + "misc/scene_texture_08/scene_texture_08_Tex_";
        const auto mask = shared + "005000";
        const auto composite = shared + "006000";
        std::vector<Gfx> original{ gsSPTexture(0xFFFF, 0xFFFF, 0, 0, G_ON) };
        const auto image = [&](const std::string& path, uintptr_t word) {
            const auto hash = CRC64(path.c_str());
            original.push_back(Command(word, 0));
            original.push_back(Command(hash >> 32, hash & 0xFFFFFFFF));
        };
        image(mask, 0x20700000);
        original.push_back(Command(0xF5700000, 0x07058060));
        original.push_back(Command(0xF3000000, 0x073FF200));
        original.push_back(Command(0xF5600800, 0x00F58060));
        image(composite, 0x20100000);
        original.push_back(Command(0xF5100100, 0x07054451));
        original.push_back(Command(0xF3000000, 0x073FF100));
        original.push_back(Command(0xF5101100, 0x01054451));
        original.push_back(Command(0xFC20AC04, 0xFF0F93FF));
        original.push_back(gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0));
        original.push_back(gsSPEndDisplayList());
        const char* owner = "scenes/nonmq/Z2_24KEMONOMITI/Z2_24KEMONOMITI_room_00DL_004F78";
        for (int selectedSeason : { SEASON_AUTUMN, SEASON_WINTER, SEASON_SPRING }) {
            CheckActualSceneMaterial(owner, SCENE_24KEMONOMITI, original, { 0, 0 }, selectedSeason);
            auto unknown = original;
            const auto hash = CRC64((shared + "unrelated").c_str());
            unknown[7] = Command(hash >> 32, hash & 0xFFFFFFFF);
            const auto variant = BuildVariant(unknown, selectedSeason, SCENE_24KEMONOMITI);
            const uint32_t color = selectedSeason == SEASON_AUTUMN   ? 0xEF484CFF
                                   : selectedSeason == SEASON_WINTER ? 0xE2EEE9FF
                                                                     : 0xF6C1D9E0;
            assert(Equal(RemoveTint(variant), unknown));
            assert(TrianglePalettes(variant) == std::vector<uint32_t>({ color, color }));
        }
        // The same IA identity continues to work for Field and other owners.
        assert(!BuildVariant(original, SEASON_AUTUMN, SCENE_00KEIKOKU).empty());
    }
    std::puts(
        "PASS Road exact paired surface avoids blanket tint; Field and unknown replacement pairs retain behavior");
}
static void CheckStumpGrassAtlas() {
    for (const auto* suffix : { "", "_scene" }) {
        for (const auto* prefix : { "", "alt/" }) {
            ClearCache();
            auto rm = std::make_shared<Ship::ResourceManager>();
            Ship::Context::GetRawInstance()->rm = rm;
            rm->alt = true;
            const auto path =
                std::string(prefix) + "scenes/nonmq/Z2_00KEIKOKU" + suffix + "/mm3d_field_poc4/textures/tl_ki_02";
            auto source = SurfaceTexture();
            const auto original = *source->mImageBuffer;
            rm->resources[path] = source;
            auto unrelated = SurfaceTexture();
            rm->resources[path + "_unrelated"] = unrelated;
            PlayState play{};
            play.sceneId = SCENE_00KEIKOKU;
            season = SEASON_AUTUMN;
            MMAutumnSceneFoliage_Update(&play);
            auto tinted = std::dynamic_pointer_cast<Fast::Texture>(rm->resources.at(path));
            if (tinted == source)
                std::fprintf(stderr,
                             "FAIL stump/arch grass: mixed atlas still resolves to its original green pixels\n");
            assert(tinted != source && tinted->ImageData != source->ImageData);
            const std::vector<uint8_t> expected = { 71, 44, 23, 128, 70, 50, 30, 255, 64, 64, 64, 0 };
            assert(std::equal(expected.begin(), expected.end(), tinted->ImageData));
            assert(*source->mImageBuffer == original);
            assert(tinted->Width == source->Width && tinted->Height == source->Height &&
                   tinted->ImageDataSize == source->ImageDataSize && tinted->Type == source->Type &&
                   tinted->Flags == source->Flags && tinted->HByteScale == source->HByteScale &&
                   tinted->VPixelScale == source->VPixelScale);
            ForestRendererFixture::manager.paths[CRC64(path.c_str())] = path;
            auto bind = MakeMaterial(path.c_str());
            bind.push_back(gsSPEndDisplayList());
            const auto stats = ForestRendererFixture::Draw(bind);
            assert(stats.textures.size() == 1 && stats.textures.front() == tinted);
            assert(bind[0].words.w1 == reinterpret_cast<uintptr_t>(tinted->ImageData));
            assert(rm->resources.at(path + "_unrelated") == unrelated && *unrelated->mImageBuffer == original);
            MMAutumnSceneFoliage_Update(&play);
            assert(rm->resources.at(path) == tinted && rm->loads == 0);
            for (unsigned phase = 0; phase < 3; ++phase) {
                if (phase == 0)
                    season = SEASON_OFF;
                if (phase == 1)
                    rm->alt = false;
                if (phase == 2)
                    play.sceneId = SCENE_BACKTOWN;
                MMAutumnSceneFoliage_Update(&play);
                assert(rm->resources.at(path) == source && *source->mImageBuffer == original);
                const auto restored = ForestRendererFixture::Draw(bind);
                assert(restored.textures.size() == 1 && restored.textures.front() == source);
                assert(bind[0].words.w1 == reinterpret_cast<uintptr_t>(source->ImageData));
                season = SEASON_AUTUMN;
                rm->alt = true;
                play.sceneId = SCENE_00KEIKOKU;
                MMAutumnSceneFoliage_Update(&play);
                assert(rm->resources.at(path) == tinted); // Reuse the immutable copy across transitions.
            }
            auto replacement = SurfaceTexture();
            rm->resources[path] = replacement;
            season = SEASON_OFF;
            MMAutumnSceneFoliage_Update(&play);
            assert(rm->resources.at(path) == replacement); // An outside cache edit must win.
        }
    }
    ClearCache();
    season = SEASON_AUTUMN;
    std::puts(
        "PASS private stump/arch grass-only atlas: immutable originals, exact bark/alpha/metadata and restoration");
}
static void CheckBankGrassSurface() {
    for (const bool rgba16 : { false, true }) {
        for (const auto* suffix : { "", "_scene" }) {
            for (const auto* prefix : { "", "alt/" }) {
                ClearCache();
                auto rm = std::make_shared<Ship::ResourceManager>();
                Ship::Context::GetRawInstance()->rm = rm;
                rm->alt = *prefix != '\0';
                const std::string path =
                    std::string(prefix) + "scenes/nonmq/Z2_00KEIKOKU" + suffix + "/Z2_00KEIKOKUTex_027050";
                auto source = SurfaceTexture();
                if (rgba16) {
                    source->Type = Fast::TextureType::RGBA16bpp;
                    source->Flags = 0;
                    source->ImageDataSize = 6;
                    source->mImageBuffer = std::make_shared<std::vector<char>>();
                    // Green grass (4,12,2), brown rock (9,6,4), neutral (8,8,8).
                    for (const uint8_t byte : { 0x23, 0x05, 0x49, 0x89, 0x42, 0x10 })
                        source->mImageBuffer->push_back(static_cast<char>(byte));
                    source->ImageData = reinterpret_cast<uint8_t*>(source->mImageBuffer->data());
                }
                const auto original = *source->mImageBuffer;
                rm->resources[path] = source;
                auto unrelated = SurfaceTexture();
                rm->resources[path + "_unrelated"] = unrelated;
                ForestRendererFixture::manager.paths[CRC64(path.c_str())] = path;
                auto bind = MakeMaterial(path.c_str());
                bind.push_back(gsSPEndDisplayList());
                PlayState play{};
                play.sceneId = SCENE_BACKTOWN; // Shared identity remains eligible outside Termina.
                season = SEASON_AUTUMN;
                MMAutumnSceneFoliage_Update(&play);
                auto autumn = std::dynamic_pointer_cast<Fast::Texture>(rm->resources.at(path));
                if (autumn == source)
                    std::fprintf(stderr, "FAIL shared bank: grass stays green outside Termina or with Alt off\n");
                assert(autumn != source && autumn->ImageData != source->ImageData);
                const std::vector<uint8_t> amber =
                    rgba16 ? std::vector<uint8_t>{ 0x49, 0x47, 0x49, 0x89, 0x42, 0x10 }
                           : std::vector<uint8_t>{ 71, 44, 23, 128, 70, 50, 30, 255, 64, 64, 64, 0 };
                assert(std::equal(amber.begin(), amber.end(), autumn->ImageData));
                assert(autumn->Type == source->Type && autumn->Flags == source->Flags &&
                       autumn->Width == source->Width && autumn->Height == source->Height &&
                       autumn->HByteScale == source->HByteScale && autumn->VPixelScale == source->VPixelScale);
                const auto draw = ForestRendererFixture::Draw(bind);
                assert(draw.textures.size() == 1 && draw.textures.front() == autumn);
                play.sceneId = SCENE_00KEIKOKU;
                season = SEASON_WINTER;
                MMAutumnSceneFoliage_Update(&play);
                auto winter = std::dynamic_pointer_cast<Fast::Texture>(rm->resources.at(path));
                const std::vector<uint8_t> snow =
                    rgba16 ? std::vector<uint8_t>{ 0x52, 0x97, 0x49, 0x89, 0x42, 0x10 }
                           : std::vector<uint8_t>{ 79, 84, 88, 128, 70, 50, 30, 255, 64, 64, 64, 0 };
                assert(winter != source && winter != autumn);
                assert(std::equal(snow.begin(), snow.end(), winter->ImageData));
                play.sceneId = SCENE_BACKTOWN;
                MMAutumnSceneFoliage_Update(&play); // The same bank keeps Termina's frost in adjacent outdoors.
                auto softWinter = std::dynamic_pointer_cast<Fast::Texture>(rm->resources.at(path));
                assert(softWinter == winter && std::equal(snow.begin(), snow.end(), softWinter->ImageData));
                assert(*source->mImageBuffer == original && rm->resources.at(path + "_unrelated") == unrelated);
                season = SEASON_SPRING;
                MMAutumnSceneFoliage_Update(&play);
                const auto blush = std::dynamic_pointer_cast<Fast::Texture>(rm->resources.at(path));
                CheckSurfacePixels(source, blush);
                const auto springPixels = TexturePixels(blush);
                assert(springPixels[0][0] > springPixels[0][2] && springPixels[0][2] > springPixels[0][1]);
                assert(ForestRendererFixture::Draw(bind).textures.front() == blush);
                for (const int inactive : { SEASON_OFF, SEASON_SUMMER, -1 }) {
                    season = inactive;
                    MMAutumnSceneFoliage_Update(&play);
                    assert(rm->resources.at(path) == source && *source->mImageBuffer == original);
                    const auto restored = ForestRendererFixture::Draw(bind);
                    assert(restored.textures.front() == source);
                    season = SEASON_AUTUMN;
                    MMAutumnSceneFoliage_Update(&play);
                    assert(rm->resources.at(path) == autumn);
                }
                if (*prefix != '\0') {
                    rm->alt = false;
                    MMAutumnSceneFoliage_Update(&play);
                    assert(rm->resources.at(path) == source);
                }
                auto replacement = SurfaceTexture();
                rm->resources[path] = replacement;
                MMAutumnSceneFoliage_Reset();
                assert(rm->resources.at(path) == replacement && rm->loads == 0);
            }
        }
    }
    ClearCache();
    season = SEASON_AUTUMN;
    std::puts("PASS shared bank grass: RGBA16/32, both namespaces, native/Alt, renderer restoration and consistent "
              "frost; rock/alpha exact");
}
static void CheckSpringSceneCoverage() {
    for (const auto* suffix : { "", "_scene" }) {
        for (const bool alt : { false, true }) {
            ClearCache();
            auto rm = std::make_shared<Ship::ResourceManager>();
            Ship::Context::GetRawInstance()->rm = rm;
            rm->alt = alt;
            const std::string prefix = (alt ? "alt/" : "") + std::string("scenes/nonmq/Z2_00KEIKOKU") + suffix + "/";
            std::vector<std::shared_ptr<Fast::DisplayList>> lists;
            for (const auto& [name, commands] :
                 { std::pair{ "003928", kActualMiddleForestCommands }, std::pair{ "0241B0", kActualForestCommands },
                   std::pair{ "027238", kActualBackwallGrassCommands } }) {
                auto list = std::make_shared<Fast::DisplayList>();
                list->Instructions = commands;
                rm->resources[prefix + "Z2_00KEIKOKU_room_00DL_" + name] = list;
                rm->archive->files[prefix + "Z2_00KEIKOKU_room_00DL_" + name] = list;
                lists.push_back(list);
            }
            for (const auto* id : { "005800", "005000", "007800", "006800" }) {
                const auto path = std::string("misc/scene_texture_08/scene_texture_08_Tex_") + id;
                ForestRendererFixture::manager.paths[CRC64(path.c_str())] = path;
            }
            PlayState play{};
            play.sceneId = SCENE_00KEIKOKU;
            for (const int selected : { SEASON_SPRING, SEASON_AUTUMN, SEASON_WINTER, SEASON_SPRING, SEASON_SUMMER,
                                        SEASON_OFF, SEASON_AUTUMN, SEASON_SPRING }) {
                season = selected;
                MMAutumnSceneFoliage_Update(&play);
                const std::vector<uint32_t> colors =
                    selected == SEASON_SPRING   ? std::vector<uint32_t>{ 0xF6C1D9CC, 0xF6C1D9E0, 0 }
                    : selected == SEASON_AUTUMN ? std::vector<uint32_t>{ 0xF26846FF, 0xEF484CFF, 0xF3974FFF }
                    : selected == SEASON_WINTER ? std::vector<uint32_t>{ 0xE2EEE9FF, 0xE2EEE9FF, 0xDCECF5FF }
                                                : std::vector<uint32_t>(3, 0);
                for (size_t i = 0; i < lists.size(); ++i) {
                    const auto stats = ForestRendererFixture::Draw(lists[i]->Instructions);
                    const unsigned count = i == 0 ? 32 : i == 1 ? 22 : 26;
                    if (selected == SEASON_SPRING && stats.palettes != std::vector<uint32_t>(count, colors[i]))
                        std::fprintf(
                            stderr,
                            "FAIL spring scene: selected tree foliage lacks blossom tint or ground turns pink\n");
                    assert(stats.palettes == std::vector<uint32_t>(count, colors[i]));
                    const unsigned mode = colors[i] == 0                                           ? 0
                                          : selected == SEASON_WINTER || selected == SEASON_SPRING ? 3
                                                                                                   : 2;
                    assert(stats.tintModes == std::vector<unsigned>(count, mode));
                }
            }
            MMAutumnSceneFoliage_Reset();
            const std::vector<std::vector<Gfx>> originals = { kActualMiddleForestCommands, kActualForestCommands,
                                                              kActualBackwallGrassCommands };
            for (size_t i = 0; i < lists.size(); ++i) {
                assert(lists[i]->Instructions.size() == originals[i].size());
                for (size_t word = 0; word < originals[i].size(); ++word) {
                    const auto opcode = originals[i][word].words.w0 >> 24;
                    // The production renderer writes resolved image/vertex
                    // pointers into w1. Every opcode, hash payload and other word stays exact.
                    assert(lists[i]->Instructions[word].words.w0 == originals[i][word].words.w0);
                    if (opcode != G_SETTIMG_OTR_HASH && opcode != G_VTX_OTR_HASH)
                        assert(lists[i]->Instructions[word].words.w1 == originals[i][word].words.w1);
                }
            }
        }
    }
    ClearCache();
    season = SEASON_AUTUMN;
    std::puts("PASS selective spring scene: middle/tree canopy blossom pink, pixel-scoped ground, native/Alt aliases "
              "and three-season renderer transitions");
}
static void CheckCrossSeasonVtxFirst() {
    unsigned failures = 0;
    for (const char* drawName : { "0241B0", "0241B1" }) {
        for (const int initial : { SEASON_AUTUMN, SEASON_WINTER }) {
            for (const bool renderOff : { false, true }) {
                ClearCache();
                auto rm = std::make_shared<Ship::ResourceManager>();
                Ship::Context::GetRawInstance()->rm = rm;
                const std::string path = "scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKU_room_00DL_" + std::string(drawName);
                auto forest = std::make_shared<Fast::DisplayList>();
                forest->Instructions = kActualForestCommands;
                forest->Instructions.erase(forest->Instructions.begin(), forest->Instructions.begin() + 2);
                assert((forest->Instructions[0].words.w0 >> 24) == G_VTX_OTR_HASH);
                rm->archive->files[path] = rm->resources[path] = forest;
                PlayState play{};
                play.sceneId = SCENE_00KEIKOKU;
                ForestRendererFixture::manager.refreshed = false;
                const int other = initial == SEASON_AUTUMN ? SEASON_WINTER : SEASON_AUTUMN;
                unsigned stage = 0;
                for (const int selected : { initial, SEASON_OFF, other, initial, other }) {
                    season = selected;
                    MMAutumnSceneFoliage_Update(&play);
                    if (selected != SEASON_OFF || renderOff) {
                        const auto stats = ForestRendererFixture::Draw(forest->Instructions);
                        const uint32_t color = selected == SEASON_AUTUMN   ? 0xEF484CFF
                                               : selected == SEASON_WINTER ? 0xE2EEE9FF
                                                                           : 0;
                        const auto expected = std::vector<uint32_t>(22, color);
                        const unsigned selectedCount = std::count_if(stats.palettes.begin(), stats.palettes.end(),
                                                                     [](uint32_t value) { return value != 0; });
                        if (stats.palettes != expected)
                            std::fprintf(stderr,
                                         "FAIL draw=%s initial=%d renderedOff=%d stage=%u season=%d tint=%u/%u\n",
                                         drawName, initial, int(renderOff), stage, selected, selectedCount,
                                         selected == SEASON_OFF ? 0u : 22u);
                        if (stats.palettes != expected)
                            ++failures;
                    }
                    ++stage;
                }
                ClearCache();
            }
        }
    }

    // Resolve and observe the first-command pointer before intentionally changing
    // it. The known restoration offset must not authorize unrelated later edits.
    alignas(16) Vtx alternateVertices[128]{};
    for (const char* drawName : { "0241B0", "0241B1" }) {
        for (const int initial : { SEASON_AUTUMN, SEASON_WINTER }) {
            for (const bool pointerEdit : { false, true }) {
                ClearCache();
                auto rm = std::make_shared<Ship::ResourceManager>();
                Ship::Context::GetRawInstance()->rm = rm;
                const std::string path = "scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKU_room_00DL_" + std::string(drawName);
                auto forest = std::make_shared<Fast::DisplayList>();
                forest->Instructions = kActualForestCommands;
                forest->Instructions.erase(forest->Instructions.begin(), forest->Instructions.begin() + 2);
                const auto originalFirst = forest->Instructions[0];
                rm->archive->files[path] = rm->resources[path] = forest;
                PlayState play{};
                play.sceneId = SCENE_00KEIKOKU;
                ForestRendererFixture::manager.refreshed = false;
                const int other = initial == SEASON_AUTUMN ? SEASON_WINTER : SEASON_AUTUMN;
                for (const int selected : { initial, SEASON_OFF, other, SEASON_OFF, initial }) {
                    season = selected;
                    MMAutumnSceneFoliage_Update(&play);
                    ForestRendererFixture::Draw(forest->Instructions);
                }
                auto intentional = originalFirst;
                intentional.words.w1 = pointerEdit ? reinterpret_cast<uintptr_t>(alternateVertices) : 16;
                forest->Instructions[0] = intentional;
                MMAutumnSceneFoliage_Update(&play);
                const bool preserved = SameCommand(forest->Instructions[0], intentional) && sActive.empty();
                const auto stats = ForestRendererFixture::Draw(forest->Instructions);
                const bool untreated = stats.palettes == std::vector<uint32_t>(22, 0);
                const auto expectedPointer =
                    pointerEdit
                        ? reinterpret_cast<uintptr_t>(alternateVertices)
                        : reinterpret_cast<uintptr_t>(ForestRendererFixture::manager.GetResourceRawPointer(0)) + 16;
                const bool followsEdit = !stats.vertexPointers.empty() && stats.vertexPointers[0] == expectedPointer;
                if (!preserved || !untreated || !followsEdit)
                    std::fprintf(
                        stderr,
                        "FAIL edit guard draw=%s initial=%d pointerEdit=%d preserved=%d untreated=%d followsEdit=%d\n",
                        drawName, initial, int(pointerEdit), int(preserved), int(untreated), int(followsEdit));
                if (!preserved || !untreated || !followsEdit)
                    ++failures;
                ClearCache();
            }
        }
    }
    ForestRendererFixture::manager.refreshed = false;
    season = SEASON_AUTUMN;
    if (failures)
        std::fprintf(stderr, "FAIL cross-season VTX-first failures=%u\n", failures);
    else
        std::puts("PASS both-season VTX-first transitions: rendered/no-draw Off, whole/material variants and outside "
                  "edit guards");
    assert(failures == 0);
}
static void CheckExistingSeasonCacheClear() {
    unsigned failures = 0;
    for (const char* drawName : { "0241B0", "0241B1" }) {
        for (const int initial : { SEASON_AUTUMN, SEASON_WINTER }) {
            for (const bool renderOff : { false, true }) {
                ClearCache();
                auto rm = std::make_shared<Ship::ResourceManager>();
                Ship::Context::GetRawInstance()->rm = rm;
                const std::string path = "scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKU_room_00DL_" + std::string(drawName);
                auto forest = std::make_shared<Fast::DisplayList>();
                forest->Instructions = kActualForestCommands;
                rm->archive->files[path] = rm->resources[path] = forest;
                PlayState play{};
                play.sceneId = SCENE_00KEIKOKU;
                ForestRendererFixture::manager.refreshed = false;
                const int other = initial == SEASON_AUTUMN ? SEASON_WINTER : SEASON_AUTUMN;
                // Create both seasonal caches and validate the source's resolved
                // non-first vertex pointers in each cache.
                for (const int selected : { initial, SEASON_OFF, initial, other }) {
                    season = selected;
                    MMAutumnSceneFoliage_Update(&play);
                    ForestRendererFixture::Draw(forest->Instructions);
                }
                assert(sVariants.contains(forest.get()) && sWinterVariants.contains(forest.get()));
                assert((forest->Instructions[28].words.w0 >> 24) == G_VTX_OTR_HASH);
                // The active season observes the exact known cache-clear offset;
                // the inactive season must inherit that validated observation.
                forest->Instructions[28].words.w1 = kActualForestCommands[28].words.w1;
                ForestRendererFixture::manager.refreshed = true;
                MMAutumnSceneFoliage_Update(&play);
                auto stats = ForestRendererFixture::Draw(forest->Instructions);
                const uint32_t otherColor = other == SEASON_AUTUMN ? 0xEF484CFF : 0xE2EEE9FF;
                assert(stats.palettes == std::vector<uint32_t>(22, otherColor));
                season = SEASON_OFF;
                MMAutumnSceneFoliage_Update(&play);
                if (renderOff)
                    ForestRendererFixture::Draw(forest->Instructions);
                season = initial;
                MMAutumnSceneFoliage_Update(&play);
                stats = ForestRendererFixture::Draw(forest->Instructions);
                const uint32_t initialColor = initial == SEASON_AUTUMN ? 0xEF484CFF : 0xE2EEE9FF;
                const unsigned selectedCount = std::count_if(stats.palettes.begin(), stats.palettes.end(),
                                                             [](uint32_t value) { return value != 0; });
                const uintptr_t expectedVertex =
                    reinterpret_cast<uintptr_t>(ForestRendererFixture::manager.GetResourceRawPointer(0)) +
                    kActualForestCommands[28].words.w1;
                const bool followsReload = stats.vertexPointers.size() > 1 && stats.vertexPointers[1] == expectedVertex;
                if (stats.palettes != std::vector<uint32_t>(22, initialColor) || !followsReload) {
                    std::fprintf(
                        stderr,
                        "FAIL existing cache clear draw=%s initial=%d renderedOff=%d tint=%u/22 followsReload=%d\n",
                        drawName, initial, int(renderOff), selectedCount, int(followsReload));
                    ++failures;
                }
                ClearCache();
            }
        }
    }
    ForestRendererFixture::manager.refreshed = false;
    season = SEASON_AUTUMN;
    if (failures)
        std::fprintf(stderr, "FAIL existing-season cache-clear failures=%u\n", failures);
    else
        std::puts(
            "PASS existing autumn/winter caches share validated vertex reloads across rendered/no-draw Off frames");
    assert(failures == 0);
}
static void CheckWinterTransitions() {
    for (const auto* suffix : { "", "_scene" }) {
        for (const bool alt : { false, true }) {
            ClearCache();
            auto rm = std::make_shared<Ship::ResourceManager>();
            Ship::Context::GetRawInstance()->rm = rm;
            rm->alt = alt;
            const auto prefix = (alt ? "alt/" : "") + std::string("scenes/nonmq/Z2_00KEIKOKU") + suffix + "/";
            std::vector<std::shared_ptr<Fast::DisplayList>> lists;
            for (const auto& [name, commands] :
                 { std::pair{ "003928", kActualMiddleForestCommands }, std::pair{ "0241B0", kActualForestCommands },
                   std::pair{ "027238", kActualBackwallGrassCommands } }) {
                auto list = std::make_shared<Fast::DisplayList>();
                list->Instructions = commands;
                const auto path = prefix + "Z2_00KEIKOKU_room_00DL_" + name;
                rm->resources[path] = rm->archive->files[path] = list;
                lists.push_back(list);
            }
            for (const auto* id : { "005800", "005000", "007800", "006800" }) {
                const auto path = std::string("misc/scene_texture_08/scene_texture_08_Tex_") + id;
                ForestRendererFixture::manager.paths[CRC64(path.c_str())] = path;
            }
            const auto atlasPath = prefix + "mm3d_field_poc4/textures/tl_ki_02";
            auto atlas = SurfaceTexture();
            const auto originalPixels = *atlas->mImageBuffer;
            rm->resources[atlasPath] = atlas;
            std::map<int, std::shared_ptr<Fast::Texture>> copies;
            std::map<int, uintptr_t> branches;
            PlayState play{};
            play.sceneId = SCENE_00KEIKOKU;
            for (const int selected : { SEASON_AUTUMN, SEASON_WINTER, SEASON_AUTUMN, SEASON_WINTER, SEASON_OFF,
                                        SEASON_WINTER, SEASON_SPRING, SEASON_SUMMER, -1, SEASON_AUTUMN }) {
                season = selected;
                MMAutumnSceneFoliage_Update(&play);
                const bool treated =
                    selected == SEASON_AUTUMN || selected == SEASON_WINTER || selected == SEASON_SPRING;
                const std::vector<uint32_t> colors =
                    selected == SEASON_AUTUMN   ? std::vector<uint32_t>{ 0xF26846FF, 0xEF484CFF, 0xF3974FFF }
                    : selected == SEASON_WINTER ? std::vector<uint32_t>{ 0xE2EEE9FF, 0xE2EEE9FF, 0xDCECF5FF }
                    : selected == SEASON_SPRING ? std::vector<uint32_t>{ 0xF6C1D9CC, 0xF6C1D9E0, 0 }
                                                : std::vector<uint32_t>(3, 0);
                for (size_t i = 0; i < lists.size(); ++i) {
                    const auto stats = ForestRendererFixture::Draw(lists[i]->Instructions);
                    const unsigned count = i == 0 ? 32 : i == 1 ? 22 : 26;
                    if (selected == SEASON_WINTER && i == 0 &&
                        stats.palettes != std::vector<uint32_t>(count, colors[i]))
                        std::fprintf(stderr,
                                     "FAIL winter middle trees: verified foliage draw still has no frosted tint\n");
                    assert(stats.palettes == std::vector<uint32_t>(count, colors[i]));
                    const unsigned mode = colors[i] == 0                                           ? 0
                                          : selected == SEASON_WINTER || selected == SEASON_SPRING ? 3
                                                                                                   : 2;
                    assert(stats.tintModes == std::vector<unsigned>(count, mode));
                }
                if (treated) {
                    const auto pointer = lists[0]->Instructions[0].words.w1;
                    if (branches.contains(selected))
                        assert(branches.at(selected) == pointer);
                    else
                        branches[selected] = pointer;
                    if (branches.size() == 2)
                        assert(branches.at(SEASON_AUTUMN) != branches.at(SEASON_WINTER));
                }
                auto cached = std::dynamic_pointer_cast<Fast::Texture>(rm->resources.at(atlasPath));
                if (treated && alt) {
                    assert(cached != atlas);
                    const std::vector<uint8_t> expected =
                        selected == SEASON_AUTUMN
                            ? std::vector<uint8_t>{ 71, 44, 23, 128, 70, 50, 30, 255, 64, 64, 64, 0 }
                        : selected == SEASON_WINTER
                            ? std::vector<uint8_t>{ 79, 84, 88, 128, 70, 50, 30, 255, 64, 64, 64, 0 }
                            : std::vector<uint8_t>{ 94, 74, 83, 128, 70, 50, 30, 255, 64, 64, 64, 0 };
                    assert(std::equal(expected.begin(), expected.end(), cached->ImageData));
                    if (copies.contains(selected))
                        assert(copies.at(selected) == cached);
                    else
                        copies[selected] = cached;
                    if (copies.contains(SEASON_AUTUMN) && copies.contains(SEASON_WINTER))
                        assert(copies.at(SEASON_AUTUMN)->ImageData != copies.at(SEASON_WINTER)->ImageData);
                } else {
                    assert(cached == atlas);
                }
                assert(*atlas->mImageBuffer == originalPixels && rm->loads == 0);
            }
            ClearCache();
            assert(rm->resources.at(atlasPath) == atlas);
            assert(sVariants.empty() && sWinterVariants.empty() && sSpringVariants.empty() && sSurfaceTextures.empty());
        }
    }
    season = SEASON_AUTUMN;
    std::puts("PASS spring/autumn/winter transitions: distinct tree layers, exact grass, immutable separate copies, "
              "native/Alt and Off/summer/ineligible cleanup");
}
static void CheckActualGroundResource(const char* path, const std::vector<Gfx>& original) {
    for (bool alt : { false, true }) {
        ClearCache();
        auto rm = std::make_shared<Ship::ResourceManager>();
        Ship::Context::GetRawInstance()->rm = rm;
        rm->alt = alt;
        const std::string resolved = std::string(alt ? "alt/" : "") + path;
        auto list = std::make_shared<Fast::DisplayList>();
        list->Instructions = original;
        rm->resources[resolved] = rm->archive->files[resolved] = list;
        PlayState play{};
        play.sceneId = SCENE_00KEIKOKU;
        for (int selected : { SEASON_AUTUMN, SEASON_WINTER, SEASON_SPRING, SEASON_OFF, SEASON_AUTUMN }) {
            season = selected;
            MMAutumnSceneFoliage_Update(&play);
            const auto branch = list->Instructions[0];
            const auto walks = sCoverageWalks;
            for (int frame = 0; frame < 120; ++frame)
                MMAutumnSceneFoliage_Update(&play);
            assert(sCoverageWalks == walks && SameCommand(branch, list->Instructions[0]));
        }
        ClearCache();
        assert(Equal(list->Instructions, original));
    }
    std::printf("PASS actual ground resource %s: native/Alt, three-season transitions,120 steady updates without "
                "coverage rescans\n",
                path);
}
#include "autumn_scene_foliage_fixtures.inc"
static void CheckVariant(const std::shared_ptr<Fast::DisplayList>& list, const std::vector<Gfx>& original) {
    assert((list->Instructions[0].words.w0 >> 24) == G_DL);
    auto* commands = reinterpret_cast<const Gfx*>(list->Instructions[0].words.w1);
    std::vector<Gfx> stripped;
    unsigned tintedTriangles = 0;
    bool tint = false;
    for (size_t i = 0; i < original.size() + 3; ++i) {
        auto command = commands[i];
        const auto op = command.words.w0 >> 24;
        if (op == G_SETGRAYSCALE) {
            assert(command.words.w1 == 0 || command.words.w1 == 2);
            tint = command.words.w1 != 0;
            continue;
        }
        if (op == G_SETINTENSITY) {
            assert(command.words.w1 == 0xF3974FFF || command.words.w1 == 0xEF484CFF || command.words.w1 == 0xDE496BFF ||
                   command.words.w1 == 0xF26846FF);
            continue;
        }
        if (op == G_TRI1 && tint)
            ++tintedTriangles;
        stripped.push_back(command);
    }
    assert(tintedTriangles == 1 && !tint);
    assert(Equal(stripped, original)); // UVs, textures, alpha/combine/render and geometry remain exact.
}

static void CheckPrimaryGrassTile() {
    const auto grass = MakeMaterial("scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_01C650");
    const auto detail = MakeMaterial("scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_01F650");
    for (const bool grassPrimary : { true, false }) {
        std::vector<Gfx> original = { gsSPTexture(0xFFFF, 0xFFFF, 0, 0, G_ON) };
        const auto& primary = grassPrimary ? grass : detail;
        const auto& secondary = grassPrimary ? detail : grass;
        original.insert(original.end(), primary.begin(), primary.begin() + 2);
        original.push_back(gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0, G_TX_LOADTILE, 0, 0, 0, 0, 0, 0, 0));
        original.push_back(gsDPLoadBlock(G_TX_LOADTILE, 0, 0, 1023, 0));
        original.push_back(gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0));
        original.insert(original.end(), secondary.begin(), secondary.begin() + 2);
        original.push_back(gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 256, G_TX_LOADTILE, 0, 0, 0, 0, 0, 0, 0));
        original.push_back(gsDPLoadBlock(G_TX_LOADTILE, 0, 0, 1023, 0));
        original.push_back(gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 8, 256, 1, 0, 0, 0, 0, 0, 0, 0));
        original.push_back(gsSP1Triangle(0, 1, 2, 0));
        original.push_back(gsSPEndDisplayList());
        const auto variant = BuildVariant(original);
        if (grassPrimary) {
            assert(!variant.empty()); // The detail image cannot hide primary ground grass.
            assert(Equal(RemoveTint(variant), original));
            assert(std::count_if(variant.begin(), variant.end(), [](const Gfx& c) {
                       return c.words.w0 >> 24 == G_SETINTENSITY && c.words.w1 == 0xF3974FFF;
                   }) == 1);
        } else {
            assert(variant.empty()); // Grass in an unused secondary tile cannot tint primary dirt.
        }
    }
    std::puts("PASS primary grass render tile survives an unrelated detail load; secondary grass does not tint dirt");
}

static void CheckOtherSceneGrass() {
    for (const auto& [sceneId, name] :
         { std::pair{ SCENE_BACKTOWN, "Z2_BACKTOWN" }, std::pair{ SCENE_ALLEY, "Z2_ALLEY" },
           std::pair{ SCENE_ROMANYMAE, "Z2_ROMANYMAE" }, std::pair{ SCENE_24KEMONOMITI, "Z2_24KEMONOMITI" },
           std::pair{ SCENE_20SICHITAI, "Z2_20SICHITAI" }, std::pair{ SCENE_20SICHITAI2, "Z2_20SICHITAI2" },
           std::pair{ SCENE_21MITURINMAE, "Z2_21MITURINMAE" } }) {
        for (const auto* prefix : { "", "alt/" }) {
            ClearCache();
            auto rm = std::make_shared<Ship::ResourceManager>();
            Ship::Context::GetRawInstance()->rm = rm;
            const auto path = std::string(prefix) + "scenes/nonmq/" + name + "/" + name + "_room_00DL_000100";
            // An edited scene may reuse an already-registered texture from another scene.
            auto resource = List("scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_01C650");
            const auto original = resource->Instructions;
            rm->archive->files[path] = resource;
            rm->resources[path] = resource;
            rm->alt = std::string(prefix) == "alt/";
            PlayState play{};
            play.sceneId = sceneId;
            season = SEASON_AUTUMN;
            MMAutumnSceneFoliage_Update(&play);
            assert(resource->Instructions[0].words.w0 >> 24 == G_DL);
            season = SEASON_WINTER;
            MMAutumnSceneFoliage_Update(&play);
            const auto& winter = sWinterVariants.at(resource.get()).commands;
            const auto colors = TrianglePalettes(winter);
            assert(std::count(colors.begin(), colors.end(), 0xDCECF5FF) > 0 &&
                   std::all_of(colors.begin(), colors.end(),
                               [](uint32_t color) { return color == 0 || color == 0xDCECF5FF; }));
            for (const auto& cmd : winter)
                if ((cmd.words.w0 >> 24) == G_SETGRAYSCALE && cmd.words.w1 != 0)
                    assert(cmd.words.w1 == 3);
            season = SEASON_OFF;
            MMAutumnSceneFoliage_Update(&play);
            assert(Equal(resource->Instructions, original));
            season = SEASON_AUTUMN;
            MMAutumnSceneFoliage_Update(&play);
            assert(resource->Instructions[0].words.w0 >> 24 == G_DL);
            for (const int invalid : { -1, int(SCENE_UNSET_01), 32767 }) {
                play.sceneId = invalid;
                MMAutumnSceneFoliage_Update(&play);
                assert(Equal(resource->Instructions, original));
            }
        }
    }
    ClearCache();
    std::puts("PASS shared grass across town, Milk Road, swamp approach, both swamps and Woodfall; Termina frost, "
              "native/Alt, Off and invalid scenes");
}

static void CheckRendererTextureSlots() {
    const auto grass = MakeMaterial("scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_01C650");
    const auto dirt = MakeMaterial("scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_01F650");
    std::vector<Gfx> original = { gsSPTexture(0xFFFF, 0xFFFF, 0, 3, G_ON) };
    original.insert(original.end(), grass.begin(), grass.begin() + 2);
    original.push_back(gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 64, G_TX_LOADTILE, 0, 0, 0, 0, 0, 0, 0));
    original.push_back(gsDPLoadBlock(G_TX_LOADTILE, 0, 0, 1023, 0));
    // The production renderer puts every nonzero TMEM address in texture slot 1.
    original.push_back(gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 8, 128, 3, 0, 0, 0, 0, 0, 0, 0));
    original.push_back(gsSP1Triangle(0, 1, 2, 0));
    original.insert(original.end(), dirt.begin(), dirt.begin() + 2);
    original.push_back(gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 256, G_TX_LOADTILE, 0, 0, 0, 0, 0, 0, 0));
    original.push_back(gsDPLoadBlock(G_TX_LOADTILE, 0, 0, 1023, 0));
    original.push_back(gsSP1Triangle(0, 1, 2, 0));
    original.push_back(gsSPEndDisplayList());
    const auto variant = BuildVariant(original);
    assert(!variant.empty() && Equal(RemoveTint(variant), original));
    assert(CountMaterialTriangles(variant, "scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_01C650", 0xF3974FFF) == 1);
    assert(CountMaterialTriangles(variant, "scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_01F650", 0xF3974FFF) == 0);
    std::puts("PASS renderer texture slots, nonzero render tile and detail-slot replacement");
}

static void CheckSharedListOutdoorTransition() {
    const char* texture = "scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_01C650";
    ForestRendererFixture::manager.paths[CRC64(texture)] = texture;
    const char* unrelated = "scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_029090";
    ForestRendererFixture::manager.paths[CRC64(unrelated)] = unrelated;
    for (const bool terminaFirst : { false, true }) {
        ClearCache();
        auto rm = std::make_shared<Ship::ResourceManager>();
        Ship::Context::GetRawInstance()->rm = rm;
        auto resource = List(texture);
        const auto original = resource->Instructions;
        const std::string field = "scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKU_room_00DL_000100";
        const std::string swamp = "scenes/nonmq/Z2_20SICHITAI2/Z2_20SICHITAI2_room_00DL_000100";
        rm->archive->files[field] = rm->archive->files[swamp] = resource;
        rm->resources[field] = rm->resources[swamp] = resource;
        PlayState play{};
        season = SEASON_WINTER;
        for (const int scene : { terminaFirst ? SCENE_00KEIKOKU : SCENE_20SICHITAI2,
                                 terminaFirst ? SCENE_20SICHITAI2 : SCENE_00KEIKOKU }) {
            play.sceneId = scene;
            MMAutumnSceneFoliage_Update(&play);
            const auto& commands = sWinterVariants.at(resource.get()).commands;
            const auto stripped = RemoveTint(commands);
            assert(stripped.size() == original.size());
            for (size_t i = 0; i < original.size(); ++i) {
                assert(stripped[i].words.w0 == original[i].words.w0);
                if ((original[i].words.w0 >> 24) == G_SETTIMG_OTR_HASH)
                    assert(
                        stripped[i].words.w1 == original[i].words.w1 ||
                        stripped[i].words.w1 == reinterpret_cast<uintptr_t>(ForestRendererFixture::manager.pixels[0]) ||
                        stripped[i].words.w1 == reinterpret_cast<uintptr_t>(ForestRendererFixture::manager.pixels[1]));
                else
                    assert(stripped[i].words.w1 == original[i].words.w1);
            }
            const auto stats = ForestRendererFixture::Draw(resource->Instructions);
            assert((stats.palettes == std::vector<uint32_t>{ 0xDCECF5FF, 0 }));
            assert((stats.tintModes == std::vector<unsigned>{ 3, 0 }));
        }
        MMAutumnSceneFoliage_Reset();
        assert(Equal(resource->Instructions, original));
    }
    ClearCache();
    std::puts("PASS shared display-list cache across Termina/swamp entrances in both directions; consistent frost "
              "and exact native restoration");
    season = SEASON_AUTUMN;
}

static void CheckSpringGroundGrass() {
    const std::pair<const char*, int> cases[] = {
        { "scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_01BE50", SCENE_00KEIKOKU },
        { "misc/scene_texture_08/scene_texture_08_Tex_001000", SCENE_24KEMONOMITI },
        { "scenes/nonmq/Z2_BACKTOWN/Z2_BACKTOWNTex_005A40", SCENE_BACKTOWN },
        { "scenes/nonmq/Z2_LOST_WOODS/Z2_LOST_WOODS_room_00Tex_006A18", SCENE_LOST_WOODS },
    };
    for (const auto& [logicalPath, scene] : cases)
        for (const char* suffix : { "", "_scene" })
            for (bool alt : { false, true }) {
                ClearCache();
                auto rm = std::make_shared<Ship::ResourceManager>();
                Ship::Context::GetRawInstance()->rm = rm;
                rm->alt = alt;
                std::string path = logicalPath;
                if (path.starts_with("scenes/nonmq/"))
                    path.insert(path.find('/', 13), suffix);
                path.insert(0, alt ? "alt/" : "");
                auto source = SurfaceTexture();
                const auto bytes = *source->mImageBuffer;
                rm->resources[path] = source;
                const std::string stonePath = path + "_unrelated";
                auto stone = SurfaceTexture();
                rm->resources[stonePath] = stone;
                ForestRendererFixture::manager.paths[CRC64(path.c_str())] = path;
                auto bind = MakeMaterial(path.c_str());
                bind.push_back(gsSPEndDisplayList());
                PlayState play{};
                play.sceneId = scene;
                for (int selected : { SEASON_SPRING, SEASON_SUMMER, SEASON_SPRING, SEASON_OFF }) {
                    season = selected;
                    MMAutumnSceneFoliage_Update(&play);
                    const auto shown = std::dynamic_pointer_cast<Fast::Texture>(rm->resources.at(path));
                    if (selected == SEASON_SPRING) {
                        if (shown == source)
                            std::fprintf(stderr, "FAIL spring ground grass still matches summer: %s\n", path.c_str());
                        CheckSurfacePixels(source, shown);
                        assert(shown->ImageData[0] > shown->ImageData[2] && shown->ImageData[2] > shown->ImageData[1]);
                    } else {
                        assert(shown == source);
                    }
                    const auto draw = ForestRendererFixture::Draw(bind);
                    assert(draw.textures.front() == shown && draw.palettes.front() == 0);
                    assert(*source->mImageBuffer == bytes && rm->resources.at(stonePath) == stone);
                }
                assert(rm->loads == 0);
            }
    ClearCache();
    season = SEASON_AUTUMN;
    std::puts("PASS spring ground grass: blush reaches renderer in native/Alt namespaces; stone/alpha/source and "
              "Summer exact");
}

static void CheckMm3dLostWoodsSurfaces() {
    ClearCache();
    auto rm = std::make_shared<Ship::ResourceManager>();
    Ship::Context::GetRawInstance()->rm = rm;
    rm->alt = true;
    const std::string prefix = "alt/custom/cor_mm3d/lost_woods_poc1/textures/";
    std::map<std::string, std::shared_ptr<Fast::Texture>> originals;
    for (const char* name : { "lw_enkei_01_0", "lw_kabe_01_5400", "lw_kusa_01_12600", "lw_wood_02_2CA00",
                              "lw_wood_03_31E00", "lw_wood_04_37200", "lw_yuka_02_39C00", "lw_yuka_03_44400",
                              "lw_kinoko_01_FC00", "lw_nenrin_01_27600", "lw_tutu_01_2A000", "portal_bark" }) {
        auto texture = SurfaceTexture();
        originals[prefix + name] = texture;
        rm->resources[prefix + name] = texture;
    }
    PlayState play{};
    play.sceneId = SCENE_LOST_WOODS;
    for (int selected : { SEASON_AUTUMN, SEASON_WINTER, SEASON_SPRING, SEASON_SUMMER, SEASON_AUTUMN, SEASON_OFF }) {
        season = selected;
        MMAutumnSceneFoliage_Update(&play);
        for (const auto& [path, original] : originals) {
            const bool protectedMaterial = path.ends_with("lw_kinoko_01_FC00") ||
                                           path.ends_with("lw_nenrin_01_27600") || path.ends_with("lw_tutu_01_2A000") ||
                                           path.ends_with("portal_bark");
            const bool tinted = !protectedMaterial && selected != SEASON_SUMMER && selected != SEASON_OFF;
            const auto shown = std::dynamic_pointer_cast<Fast::Texture>(rm->resources.at(path));
            assert((shown != original) == tinted);
            if (tinted)
                CheckSurfacePixels(original, shown);
        }
    }
    season = SEASON_AUTUMN;
    MMAutumnSceneFoliage_Update(&play);
    play.sceneId = SCENE_26SARUNOMORI;
    MMAutumnSceneFoliage_Update(&play);
    for (const auto& [path, original] : originals)
        assert(rm->resources.at(path) == original);
    play.sceneId = SCENE_LOST_WOODS;
    rm->alt = false;
    MMAutumnSceneFoliage_Update(&play);
    for (const auto& [path, original] : originals)
        assert(rm->resources.at(path) == original);
    ClearCache();
    std::puts("PASS MM3D opening woods: exact private foliage surfaces, protected bark/mushrooms, season/Alt/scene "
              "restoration");
}

static void CheckSoloForestSurfaces() {
    for (bool alt : { false, true }) {
        ClearCache();
        auto rm = std::make_shared<Ship::ResourceManager>();
        Ship::Context::GetRawInstance()->rm = rm;
        rm->alt = alt;
        const std::string prefix = "custom/skiejer_forest/textures/";
        std::map<std::string, std::shared_ptr<Fast::Texture>> originals;
        for (const char* assetPrefix : { "", "alt/" })
            for (const char* name : { "lw_enkei_01_0", "lw_kabe_01_5400", "lw_kusa_01_12600", "lw_wood_02_2CA00",
                                      "lw_wood_03_31E00", "lw_wood_04_37200", "lw_yuka_02_39C00", "lw_yuka_03_44400",
                                      "lw_kinoko_01_FC00", "lw_nenrin_01_27600", "lw_tutu_01_2A000", "portal_bark" }) {
                auto texture = SurfaceTexture();
                const auto path = std::string(assetPrefix) + prefix + name;
                originals[path] = texture;
                rm->resources[path] = texture;
            }
        PlayState play{};
        play.sceneId = COMBO_SOLO_FOREST_MM_SCENE;
        for (int selected : { SEASON_AUTUMN, SEASON_WINTER, SEASON_SPRING, SEASON_SUMMER, SEASON_AUTUMN, SEASON_OFF }) {
            season = selected;
            MMAutumnSceneFoliage_Update(&play);
            for (const auto& [path, original] : originals) {
                const bool protectedMaterial = path.ends_with("lw_kinoko_01_FC00") ||
                                               path.ends_with("lw_nenrin_01_27600") ||
                                               path.ends_with("lw_tutu_01_2A000") || path.ends_with("portal_bark");
                const bool tinted = (!path.starts_with("alt/") || alt) && !protectedMaterial &&
                                    selected != SEASON_SUMMER && selected != SEASON_OFF;
                const auto shown = std::dynamic_pointer_cast<Fast::Texture>(rm->resources.at(path));
                assert((shown != original) == tinted);
                if (tinted)
                    CheckSurfacePixels(original, shown);
            }
        }
        season = SEASON_AUTUMN;
        MMAutumnSceneFoliage_Update(&play);
        play.sceneId = SCENE_LOST_WOODS;
        MMAutumnSceneFoliage_Update(&play);
        for (const auto& [path, original] : originals)
            assert(rm->resources.at(path) == original);
        ClearCache();
    }
    std::puts(
        "PASS private solo woods foliage: base/Alt, eight targets, protected bark/mushrooms, seasons and cleanup");
}

#include "autumn_termina_floor_fixture.h"
static void CheckMergedTerminaFloor() {
    assert(MaterialCommandLength(G_RDPSETOTHERMODE) == 1);
    unsigned otherModes = 0;
    for (size_t i = 0; i < kMergedTerminaFloor.size();) {
        const auto opcode = uint8_t(kMergedTerminaFloor[i].words.w0 >> 24);
        otherModes += opcode == G_RDPSETOTHERMODE;
        const auto length = MaterialCommandLength(opcode);
        assert(length && i + length <= kMergedTerminaFloor.size());
        i += length;
    }
    assert(otherModes == 1);
    for (int selected : { SEASON_AUTUMN, SEASON_WINTER, SEASON_SPRING }) {
        const auto variant = BuildVariant(kMergedTerminaFloor, selected, SCENE_00KEIKOKU);
        if (selected == SEASON_SPRING) {
            assert(variant.empty());
            continue;
        }
        assert(!variant.empty() && Equal(RemoveTint(variant), kMergedTerminaFloor));
        std::set<uint64_t> coverage;
        const auto dry =
            BuildVariant(kMergedTerminaFloor, selected, SCENE_00KEIKOKU, nullptr, nullptr, &coverage, true);
        assert(dry.empty() && dry.capacity() == 0 && !coverage.empty());
        const uint32_t color = selected == SEASON_AUTUMN ? 0xF3974FFF : 0xDCECF5FF;
        assert(CountMaterialTriangles(variant, "scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_01DE50", color) == 57);
    }
    puts("PASS exact merged Termina floor: all57 grass triangles, native other-mode preserved, Spring pixel route");
}
static void CheckWholeListGroundCoverage() {
    for (bool unknownBefore : { false, true })
        for (bool alt : { false, true }) {
            ClearCache();
            auto rm = std::make_shared<Ship::ResourceManager>();
            Ship::Context::GetRawInstance()->rm = rm;
            rm->alt = alt;
            const char* texture = "scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_01BE50";
            const std::string path = std::string(alt ? "alt/" : "") + texture;
            const std::string root =
                std::string(alt ? "alt/" : "") + "scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKU_room_00DL_0153A8";
            auto image = SurfaceTexture();
            rm->resources[path] = image;
            ForestRendererFixture::manager.paths[CRC64(texture)] = texture;
            auto list = std::make_shared<Fast::DisplayList>();
            list->Instructions = MakeMaterial(texture);
            if (unknownBefore)
                list->Instructions.insert(list->Instructions.begin(), Command(0xE0000000, 0));
            else
                list->Instructions.push_back(Command(0xE0000000, 0)); // Accepted whole-root fallback.
            list->Instructions.push_back(gsSPEndDisplayList());
            rm->resources[root] = rm->archive->files[root] = list;
            PlayState play{};
            play.sceneId = SCENE_00KEIKOKU;
            for (int selected : { SEASON_AUTUMN, SEASON_WINTER, SEASON_AUTUMN }) {
                season = selected;
                for (int frame = 0; frame < 4; ++frame) {
                    MMAutumnSceneFoliage_Update(&play);
                    assert(rm->resources.at(path) == image);
                    assert(CurrentVariants().at(list.get()).unclassifiedWrapper);
                    if (!unknownBefore)
                        assert(CurrentVariants().at(list.get()).coveredImages.contains(CRC64(texture)));
                    const auto draw = ForestRendererFixture::Draw(list->Instructions);
                    assert(draw.textures.front() == image);
                    assert(draw.palettes.front() == (selected == SEASON_AUTUMN ? 0xDE496BFF : 0xDCECF5FF));
                }
            }
        }
    ClearCache();
    season = SEASON_AUTUMN;
    puts("PASS whole-root ground coverage: cold/warm cache agrees, no pixel copy beneath accepted wrapper");
}
static void CheckColdAltGroundOwnership() {
    for (bool sameHash : { true, false }) {
        ClearCache();
        auto rm = std::make_shared<Ship::ResourceManager>();
        Ship::Context::GetRawInstance()->rm = rm;
        const char* textureA = "scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_01BE50";
        const char* textureB = sameHash ? textureA : "scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_01C650";
        const std::string root = "scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKU_room_00DL_987650";
        auto imageA = SurfaceTexture();
        auto imageB = sameHash ? imageA : SurfaceTexture();
        rm->resources[textureA] = imageA;
        rm->resources[textureB] = imageB;
        const auto bytesA = *imageA->mImageBuffer, bytesB = *imageB->mImageBuffer;
        ForestRendererFixture::manager.paths[CRC64(textureA)] = textureA;
        ForestRendererFixture::manager.paths[CRC64(textureB)] = textureB;
        auto native = std::make_shared<Fast::DisplayList>();
        native->Instructions = MakeMaterial(textureA);
        if (sameHash)
            native->Instructions.push_back(Command(0xE0000000, 0));
        native->Instructions.push_back(gsSPEndDisplayList());
        auto alt = std::make_shared<Fast::DisplayList>();
        alt->Instructions = MakeMaterial(textureB);
        alt->Instructions.push_back(gsSPEndDisplayList());
        rm->resources[root] = rm->archive->files[root] = native;
        rm->archive->files["alt/" + root] = alt; // Real owning archive, deliberately cold.
        PlayState play{};
        play.sceneId = SCENE_00KEIKOKU;
        for (int selected : { SEASON_AUTUMN, SEASON_WINTER }) {
            season = selected;
            for (bool enabled : { true, false, true }) {
                rm->alt = enabled;
                for (int frame = 0; frame < 3; ++frame) {
                    MMAutumnSceneFoliage_Update(&play);
                    auto rendered = ForestRendererFixture::Draw(enabled ? alt->Instructions : native->Instructions);
                    if (enabled || !sameHash) {
                        assert(rendered.textures.front() == (enabled ? imageB : imageA) &&
                               "cold Alt must be classified before choosing pixel fallback");
                        assert(rendered.palettes.front() == SeasonPalette(0xFFD078FF, selected, play.sceneId));
                    } else {
                        assert(rendered.textures.front() != imageA && rendered.palettes.front() == 0);
                    }
                    assert(*imageA->mImageBuffer == bytesA && *imageB->mImageBuffer == bytesB);
                }
            }
        }
        assert(rm->loads == 1);
        season = SEASON_OFF;
        MMAutumnSceneFoliage_Update(&play);
        assert(rm->resources.at(textureA) == imageA && rm->resources.at(textureB) == imageB);
    }
    ClearCache();
    season = SEASON_AUTUMN;
    puts("PASS cold real Alt/native fallback: same/different texture hashes, same-update ownership, one treatment, "
         "toggles and no extra loads");
}
static void CheckBroadGroundCoverage() {
    const char* texture = "scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_01BE50";
    for (const char* suffix : { "", "_scene" })
        for (bool alt : { false, true })
            for (bool meta : { false, true }) {
                ClearCache();
                auto rm = std::make_shared<Ship::ResourceManager>();
                Ship::Context::GetRawInstance()->rm = rm;
                rm->alt = alt;
                std::string logical = texture;
                logical.insert(logical.find('/', 13), suffix);
                const std::string path = std::string(alt ? "alt/" : "") + logical;
                const std::string root = std::string(alt ? "alt/" : "") + "scenes/nonmq/Z2_00KEIKOKU" + suffix +
                                         "/Z2_00KEIKOKU_room_00DL_987654";
                auto source = SurfaceTexture();
                const auto sourceBytes = *source->mImageBuffer;
                rm->resources[path] = source;
                ForestRendererFixture::manager.paths[CRC64(logical.c_str())] = logical;
                auto draw = MakeMaterial(logical.c_str());
                draw.push_back(gsSPEndDisplayList());
                PlayState play{};
                play.sceneId = SCENE_00KEIKOKU;
                std::vector<std::pair<std::shared_ptr<Fast::Texture>, std::vector<char>>> queuedTextures;
                for (int selected : { SEASON_AUTUMN, SEASON_WINTER, SEASON_SPRING, SEASON_AUTUMN, SEASON_OFF,
                                      SEASON_WINTER, SEASON_SUMMER }) {
                    season = selected;
                    MMAutumnSceneFoliage_Update(&play);
                    auto shown = std::dynamic_pointer_cast<Fast::Texture>(rm->resources.at(path));
                    const bool active = selected != SEASON_OFF && selected != SEASON_SUMMER;
                    if (active) {
                        const auto color = SeasonPalette(0xFFD078FF, selected, play.sceneId, true);
                        auto expected = BuildSurfaceTexture(source, color, selected == SEASON_AUTUMN ? 1.f : 1.8f);
                        assert(shown != source && *shown->mImageBuffer == *expected->mImageBuffer);
                        CheckSurfacePixels(source, shown);
                        queuedTextures.emplace_back(shown, *shown->mImageBuffer);
                    } else
                        assert(shown == source);
                    const auto rendered = ForestRendererFixture::Draw(draw);
                    assert(rendered.textures.front() == shown && rendered.palettes.front() == 0);
                    assert(*source->mImageBuffer == sourceBytes);
                }
                // Once a supported room binding is loaded, preserve its accepted draw
                // treatment (including non-green samples) and never tint its pixels too.
                auto list = std::make_shared<Fast::DisplayList>();
                list->Instructions = draw;
                rm->archive->files[root + (meta ? ".meta" : "")] = list;
                // Exercise a cold canonical/.meta load, then its native cache alias.
                {
                    Ship::OwnRMScope scope("mm");
                    assert(rm->LoadResource(root, true) == list);
                }
                for (int selected : { SEASON_AUTUMN, SEASON_WINTER, SEASON_SPRING, SEASON_AUTUMN }) {
                    season = selected;
                    MMAutumnSceneFoliage_Update(&play);
                    const auto rendered = ForestRendererFixture::Draw(list->Instructions);
                    if (selected == SEASON_SPRING) {
                        assert(rendered.textures.front() != source && rendered.palettes.front() == 0);
                    } else {
                        assert(rendered.textures.front() == source);
                        assert(rendered.palettes.front() == SeasonPalette(0xFFD078FF, selected, play.sceneId));
                        const auto walks = sCoverageWalks;
                        const auto commands = CurrentVariants().at(list.get()).commands.data();
                        for (int frame = 0; frame < 120; ++frame)
                            MMAutumnSceneFoliage_Update(&play);
                        assert(sCoverageWalks == walks && CurrentVariants().at(list.get()).commands.data() == commands);
                    }
                }
                // Originals, queued seasonal pixels and old draw buffers stay alive
                // through hot texture replacement, Off and fresh resource identities.
                auto replacement = SurfaceTexture();
                replacement->ImageData[0] += 3;
                rm->resources[path] = replacement;
                season = SEASON_OFF;
                MMAutumnSceneFoliage_Update(&play);
                assert(rm->resources.at(path) == replacement);
                assert(*source->mImageBuffer == sourceBytes);
                for (const auto& [queued, bytes] : queuedTextures)
                    assert(queued && *queued->mImageBuffer == bytes);
                auto reloaded = std::make_shared<Fast::DisplayList>();
                reloaded->Instructions = draw;
                rm->resources[root] = reloaded;
                season = SEASON_WINTER;
                MMAutumnSceneFoliage_Update(&play);
                const auto rendered = ForestRendererFixture::Draw(reloaded->Instructions);
                assert(rendered.textures.front() == replacement && rendered.palettes.front() == 0xDCECF5FF);
                assert(rm->resources.at(path) == replacement);
            }
    ClearCache();
    season = SEASON_AUTUMN;
    puts("PASS broad ground copies: one pixel/draw treatment, preserved accepted draws, base/Alt/meta, transitions and "
         "queued pixels");
}
static void CheckConfirmedShrubRoles() {
    for (const auto* suffix : { "", "_scene" })
        for (const auto* prefix : { "", "alt/" })
            for (const auto* texture :
                 { "Z2_00KEIKOKUTex_021650", "Z2_00KEIKOKUTex_0216D0", "Z2_00KEIKOKUTex_0218D0",
                   "Z2_00KEIKOKUTex_034898", "foliage_poc3/leaves_rgba", "foliage_poc3/stems_rgba" }) {
                const auto path = std::string(prefix) + "scenes/nonmq/Z2_00KEIKOKU" + suffix + "/" + texture;
                auto original = MakeMaterial(path.c_str());
                original.push_back(gsSPEndDisplayList());
                const bool groundRole = std::string(texture).find("021") != std::string::npos;
                for (const auto selected : { SEASON_AUTUMN, SEASON_SPRING, SEASON_WINTER }) {
                    const uint32_t expected = selected == SEASON_AUTUMN   ? 0xDE496BFF
                                              : selected == SEASON_SPRING ? (groundRole ? 0u : 0xF6C1D9E0u)
                                              : groundRole                ? 0xDCECF5FF
                                                                          : 0xE2EEE9FF;
                    const auto changed = BuildVariant(original, selected, SCENE_00KEIKOKU);
                    if (expected == 0) {
                        assert(changed.empty());
                    } else {
                        assert(Equal(RemoveTint(changed), original));
                        assert(CountMaterialTriangles(changed, path.c_str(), expected) == 1);
                    }
                }
            }
    puts("PASS confirmed six Termina shrub roles: Crimson autumn, original spring/winter classification and aliases");
}
int main() {
    CheckConfirmedShrubRoles();
    CheckMergedTerminaFloor();
    CheckBroadGroundCoverage();
    CheckWholeListGroundCoverage();
    CheckColdAltGroundOwnership();
    CheckPreludeMoonTreeFoliage();
    CheckPreludeMoonTreeOwnerOverride();
    if (const char* selected = std::getenv("MM_ROAD_TARGET_TEST")) {
        if (std::string(selected) == "vines")
            CheckRoadVines();
        else if (std::string(selected) == "rando")
            CheckRoadRandoGrass();
        else
            CheckRoadTrim();
        return 0;
    }
    CheckSpringGroundGrass();
    CheckMm3dLostWoodsSurfaces();
    CheckSoloForestSurfaces();
    CheckRoadTrim();
    CheckRoadVines();
    CheckRoadRandoGrass();
    CheckWoodfallMixedLeaves();
    CheckRecoveredLeafMaterials();
    CheckWoodfallSurfaces();
    CheckRoadForestPair();
    CheckSpringSceneCoverage();
    CheckBankGrassSurface();
    CheckWinterTransitions();
    CheckCrossSeasonVtxFirst();
    CheckExistingSeasonCacheClear();
    CheckStumpGrassAtlas();
    CheckBackwallGrassMaterial();
    CheckMiddleForestMaterial();
    CheckPrimaryGrassTile();
    CheckOtherSceneGrass();
    CheckRendererTextureSlots();
    CheckSharedListOutdoorTransition();
    for (const auto* path :
         { "scenes/nonmq/Z2_BACKTOWN/Z2_BACKTOWNTex_005A40", "scenes/nonmq/Z2_ALLEY/Z2_ALLEYTex_0050E0",
           "scenes/nonmq/Z2_ROMANYMAE/Z2_ROMANYMAETex_001EB0", "scenes/nonmq/Z2_F01/Z2_F01Tex_01BA00",
           "misc/scene_texture_08/scene_texture_08_Tex_004800" }) {
        assert(MaterialColor(CRC64(path)) == 0xF3974FFF);
        assert(MaterialColor(CRC64((std::string("alt/") + path).c_str())) == 0xF3974FFF);
    }
    assert(MaterialColor(CRC64("scenes/nonmq/Z2_BACKTOWN/Z2_BACKTOWNTex_007A40")) == 0xEF484CFF);
    for (const auto* path :
         { "scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_02B050", "scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_02C850",
           "scenes/nonmq/Z2_BACKTOWN/Z2_BACKTOWNTex_00C5E0" })
        assert(MaterialColor(CRC64(path)) == 0); // Water and the accepted matching dirt stay unchanged.
    // These exact decoded duplicates are cracked masonry with moss, not grass.
    // Keep them unselected even when an eligible scene reuses the material.
    for (const auto* path :
         { "scenes/nonmq/Z2_DANPEI/Z2_DANPEI_room_00Tex_009708", "scenes/nonmq/Z2_DANPEI/Z2_DANPEI_room_03Tex_008AB8",
           "scenes/nonmq/Z2_DANPEI/Z2_DANPEI_room_04Tex_004CB0", "scenes/nonmq/Z2_DANPEI/Z2_DANPEI_room_05Tex_009670",
           "scenes/nonmq/Z2_DANPEI2TEST/Z2_DANPEI2TESTTex_005210" }) {
        std::string alias(path);
        alias.insert(alias.find('/', 13), "_scene");
        for (const auto& name : { std::string(path), "alt/" + std::string(path), alias, "alt/" + alias })
            assert(MaterialColor(CRC64(name.c_str())) == 0);
    }
    // POC3 beds call private material lists and no longer load the old native
    // texture in their root. Seasonal color must follow this established root.
    {
        auto rm = Ship::Context::GetRawInstance()->rm;
        const char* path = "scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKU_room_00DL_015DF0";
        auto bed = std::make_shared<Fast::DisplayList>();
        const auto hash = CRC64("scenes/nonmq/Z2_00KEIKOKU/foliage_poc3/bed_top_material");
        bed->Instructions = { Command(uintptr_t(G_DL_OTR_HASH) << 24, 0), Command(hash >> 32, hash & 0xFFFFFFFF),
                              gsSPEndDisplayList() };
        const auto original = bed->Instructions;
        rm->archive->files[std::string("alt/") + path] = bed;
        rm->resources[std::string("alt/") + path] = bed;
        rm->alt = true;
        PlayState play{};
        play.sceneId = SCENE_00KEIKOKU;
        MMAutumnSceneFoliage_Update(&play);
        assert((bed->Instructions[0].words.w0 >> 24) == G_DL);
        const auto* wrapped = (const Gfx*)bed->Instructions[0].words.w1;
        assert((wrapped[0].words.w0 >> 24) == G_SETGRAYSCALE && wrapped[0].words.w1 == 2);
        assert(wrapped[1].words.w1 == 0xDE496BFF);
        assert((wrapped[2].words.w0 >> 24) == G_DL);
        const auto* preserved = (const Gfx*)wrapped[2].words.w1;
        for (size_t i = 0; i < original.size(); ++i)
            assert(SameCommand(preserved[i], original[i]));
        assert((wrapped[3].words.w0 >> 24) == G_SETGRAYSCALE && wrapped[3].words.w1 == 0);
        assert((wrapped[4].words.w0 >> 24) == G_ENDDL);
        MMAutumnSceneFoliage_Reset();
        assert(Equal(bed->Instructions, original));
        rm->alt = false;
        std::puts("PASS edited Alt bed keeps private child materials and exact geometry under a scoped seasonal color");
    }
    for (const auto* name : { "Z2_00KEIKOKUTex_034098", "Z2_00KEIKOKUTex_037098" }) {
        auto forest = List((std::string("alt/scenes/nonmq/Z2_00KEIKOKU/") + name).c_str());
        const auto original = forest->Instructions;
        const auto changed = BuildVariant(original);
        assert(!changed.empty() && Equal(RemoveTint(changed), original));
    }
    CheckForestDrawOwners();
    CheckRenderedForestTransitions();
    CheckActualMaterials();
    CheckPrivateBrushMaterials();
    auto rm = Ship::Context::GetRawInstance()->rm;
    constexpr const char* path = "scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKU_room_00DL_0153A8";
    auto native = List("scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_021650");
    const auto original = native->Instructions;
    rm->archive->files[path] = native;
    rm->resources[path] = native; // Simulate a scene list already loaded for rendering.
    auto unused = List("scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_021650");
    const auto unusedOriginal = unused->Instructions;
    rm->archive->files["scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKU_room_00DL_Unused"] = unused;
    PlayState play{};
    play.sceneId = SCENE_00KEIKOKU;
    MMAutumnSceneFoliage_Update(&play);
    CheckVariant(native, original);
    assert(rm->loads == 0 && Equal(unused->Instructions, unusedOriginal));
    // An unrelated feature patch made after activation must not be overwritten
    // or replayed from a stale copy. Only our first-command redirect is undone.
    native->Instructions[3].words.w1 = 0x80909055;
    MMAutumnSceneFoliage_Update(&play);
    assert(SameCommand(native->Instructions[0], original[0]));
    assert(native->Instructions[3].words.w1 == 0x80909055);
    native->Instructions[3] = original[3];
    MMAutumnSceneFoliage_Update(&play);
    CheckVariant(native, original);
    const unsigned firstLoads = rm->loads;
    for (unsigned i = 0; i < 100; ++i)
        MMAutumnSceneFoliage_Update(&play);
    assert(rm->loads == firstLoads);
    season = SEASON_OFF;
    MMAutumnSceneFoliage_Update(&play);
    assert(Equal(native->Instructions, original));
    std::puts("PASS selected foliage triangles only; exact source/alpha/geometry restoration; cached steady state");

    auto alt = List("alt/scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_021650");
    const auto altOriginal = alt->Instructions;
    // An unloaded Alt .meta must win over an already cached vanilla display list.
    // A previous failed base lookup does not hide the real meta-only alias.
    rm->archive->files[std::string("alt/") + path] = nullptr;
    rm->archive->files[std::string("alt/") + path + ".meta"] = alt;
    rm->alt = true;
    season = SEASON_AUTUMN;
    MMAutumnSceneFoliage_Update(&play);
    CheckVariant(alt, altOriginal);
    assert(Equal(native->Instructions, original));
    rm->alt = false;
    MMAutumnSceneFoliage_Update(&play);
    assert(Equal(alt->Instructions, altOriginal));
    CheckVariant(native, original);
    // Replacing a cache entry must restore the retained old resource instance.
    auto replacement = List("scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_021650");
    const auto replacementOriginal = replacement->Instructions;
    rm->resources[path] = replacement;
    MMAutumnSceneFoliage_Update(&play);
    assert(Equal(native->Instructions, original));
    CheckVariant(replacement, replacementOriginal);
    play.sceneId = SCENE_TOWN;
    MMAutumnSceneFoliage_Update(&play);
    assert(Equal(replacement->Instructions, replacementOriginal));
    std::puts("PASS Alt cold load/toggle and resource-instance reload; excluded scene cleanup");

    auto wall = List("scenes/nonmq/Z2_21MITURINMAE/Z2_21MITURINMAETex_0055D0");
    const auto wallOriginal = wall->Instructions;
    rm->archive->files["scenes/nonmq/Z2_21MITURINMAE/Z2_21MITURINMAE_room_00DL_001C48"] = wall;
    rm->resources["scenes/nonmq/Z2_21MITURINMAE/Z2_21MITURINMAE_room_00DL_001C48"] = wall;
    play.sceneId = SCENE_21MITURINMAE;
    MMAutumnSceneFoliage_Update(&play);
    CheckVariant(wall, wallOriginal);
    MMAutumnSceneFoliage_Reset();
    assert(Equal(wall->Instructions, wallOriginal));
    season = -1; // Story, underwater and interior eligibility is provided by MMWeather.
    MMAutumnSceneFoliage_Update(&play);
    assert(Equal(wall->Instructions, wallOriginal));
    std::puts("PASS one Woodfall tree-wall material, reset and season-ineligible restoration");

    auto unsupported = wallOriginal;
    unsupported.insert(unsupported.begin(), Command(0x7F000000, 0));
    assert(BuildVariant(unsupported).empty());
    assert(BuildVariant({ Command(uintptr_t(G_SETTIMG_OTR_HASH) << 24, 0) }).empty());
    assert(BuildVariant({ gsSPEndDisplayList() }).empty());
    std::puts("PASS later material patches, unsupported extensions and truncated resources are preserved");
    season = SEASON_AUTUMN;
    MMAutumnSceneFoliage_Update(&play);
    assert(Ship::CrossRMRegistry::teardown != nullptr);
    Ship::CrossRMRegistry::teardown();
    assert(Equal(wall->Instructions, wallOriginal) && sVariants.empty() && sWinterVariants.empty() &&
           sSurfaceTextures.empty());
    std::puts("PASS loaded-list scope and safe teardown cache release");
}
