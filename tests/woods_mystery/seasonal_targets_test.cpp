// Reuse the established archive/GPU seams; execute the production update and
// the native FILEPATH texture handler used by generated Woods XML materials.
#define main ExistingSeasonalSceneTests
#include "autumn_scene_foliage_test.cpp"
#undef main
#include "ComboGameMode.h"

SaveContext gSaveContext{};
PlayState* gPlayState = nullptr;
int gComboGameMode = COMBO_GAME_BOTH;
extern "C" int ComboSoloForestMm_PassageAvailable() { return false; }

namespace NativeBridgeFixture {
struct Patch {
    std::shared_ptr<Fast::DisplayList> source;
    int index;
    Gfx original;
};
static std::map<std::pair<std::string, std::string>, Patch> patches;
static std::map<std::string, size_t> sizes;
static std::string Path(const char* path) {
    std::string result = path;
    if (result.starts_with("__OTR__"))
        result.erase(0, 7);
    return result;
}
static std::shared_ptr<Ship::IResource> Find(const char* path) {
    const auto name = Path(path);
    auto rm = Ship::Context::GetRawInstance()->rm;
    if (rm->alt && rm->resources.contains("alt/" + name))
        return rm->resources.at("alt/" + name);
    return rm->resources.contains(name) ? rm->resources.at(name) : nullptr;
}
static auto List(const char* path) {
    return std::dynamic_pointer_cast<Fast::DisplayList>(Find(path));
}
} // namespace NativeBridgeFixture
extern "C" bool ResourceMgr_IsAltAssetsEnabled() {
    return Ship::Context::GetRawInstance()->rm->alt;
}
extern "C" uint8_t ResourceMgr_FileExists(const char* path) {
    return NativeBridgeFixture::Find(path) != nullptr || NativeBridgeFixture::sizes.contains(NativeBridgeFixture::Path(path));
}
extern "C" uint8_t ResourceMgr_FileAltExists(const char* path) {
    return Ship::Context::GetRawInstance()->rm->resources.contains("alt/" + NativeBridgeFixture::Path(path));
}
extern "C" Gfx* ResourceMgr_LoadGfxByName(const char* path) {
    auto list = NativeBridgeFixture::List(path);
    return list ? list->Instructions.data() : nullptr;
}
extern "C" size_t ResourceGetSizeByName(const char* path) {
    const auto found = NativeBridgeFixture::sizes.find(NativeBridgeFixture::Path(path));
    if (found != NativeBridgeFixture::sizes.end())
        return found->second;
    const auto list = NativeBridgeFixture::List(path);
    return list ? list->Instructions.size() * sizeof(Gfx) : 0;
}
extern "C" uint8_t ResourceGetIsCustomByName(const char*) { return false; }
extern "C" void ResourceMgr_PatchGfxByName(const char* path, const char* name, int index, Gfx word) {
    auto list = NativeBridgeFixture::List(path);
    assert(list && index >= 0 && index < list->Instructions.size());
    auto [found, inserted] = NativeBridgeFixture::patches.try_emplace(
        std::make_pair(path, name), NativeBridgeFixture::Patch{ list, index, list->Instructions[index] });
    if (!inserted && found->second.source != list) {
        auto& old = found->second;
        old.source->Instructions[old.index] = old.original;
        old = { list, index, list->Instructions[index] };
    }
    list->Instructions[index] = word;
}
extern "C" void ResourceMgr_UnpatchGfxByName(const char* path, const char* name) {
    const auto found = NativeBridgeFixture::patches.find(std::make_pair(path, name));
    if (found == NativeBridgeFixture::patches.end())
        return;
    auto& old = found->second;
    old.source->Instructions[old.index] = old.original;
    NativeBridgeFixture::patches.erase(found);
}

namespace WoodsFilepathFixture {
using namespace ForestRendererFixture;
static auto ActiveResMgr() {
    return Ship::Context::GetRawInstance()->rm;
}
static std::shared_ptr<Fast::Texture> ComboLoadTextureResource(const char* name) {
    std::string path = name;
    if (path.starts_with("__OTR__"))
        path.erase(0, 7);
    auto rm = Ship::Context::GetRawInstance()->rm;
    Ship::OwnRMScope scope("mm");
    if (rm->alt)
        if (auto resource = rm->GetCachedResource("alt/" + path, true))
            return std::dynamic_pointer_cast<Fast::Texture>(resource);
    return std::dynamic_pointer_cast<Fast::Texture>(rm->GetCachedResource(path, true));
}
#define C0(pos, width) ((cmd->words.w0 >> (pos)) & ((1U << width) - 1))
#include "woods_filepath_production.inc"
#undef C0
static std::shared_ptr<Fast::Texture> Draw(const std::string& path) {
    Interpreter renderer;
    auto command = Command((G_SETTIMG_OTR_FILEPATH << 24) | (G_IM_SIZ_16b << 19),
                           reinterpret_cast<uintptr_t>(path.c_str()));
    Gfx* pointer = &command;
    gfx_set_timg_otr_filepath_handler_custom(&renderer, &pointer);
    assert(renderer.stats.textures.size() == 1 && pointer == &command);
    return renderer.stats.textures.front();
}
} // namespace WoodsFilepathFixture

#include "woods_seasonal_pixels.inc"
#include "woods_native_bridge_world_hook.inc"
#include "woods_native_canopy_case.inc"

static void CheckWoodsRawUpload(const std::shared_ptr<Fast::Texture>& image) {
    assert(image->Type == Fast::TextureType::RGBA16bpp && image->Flags == TEX_FLAG_LOAD_AS_RAW);
    assert(image->Width == 1024 && image->HByteScale == 2 * 1024 / 32 && image->VPixelScale == 32);
    RoadVineUploadFixture::Interpreter renderer;
    const unsigned logicalHeight = image->Height / image->VPixelScale;
    renderer.state.texture_to_load = {
        image->ImageData, G_IM_SIZ_16b, 1, image->Flags,
        { image->Width, image->Height, image->HByteScale, image->VPixelScale, image->Type, image }
    };
    renderer.state.texture_tile[0].line_size_bytes = 32 * 2;
    renderer.state.texture_tile[0].siz = G_IM_SIZ_16b;
    renderer.GfxDpLoadBlock(7, 0, 0, 32 * logicalHeight - 1, 0);
    renderer.ImportTextureRaw(0, false);
    assert(renderer.uploadedWidth == image->Width && renderer.uploadedHeight == image->Height);
    assert(renderer.uploaded.size() == image->ImageDataSize &&
           std::equal(renderer.uploaded.begin(), renderer.uploaded.end(), image->ImageData));
}

static void CheckWoodsSurfaces() {
    const auto textures = ActualWoodsTextures();
    const std::string prefix = "custom/cor_woods_mystery/poc1/textures/";
    const std::set<std::string> grass = { "grass", "rock", "vines", "ruins" };
    const std::set<std::string> leaves = { "henriko_canopy", "henriko_forest_edge" };
    for (const char* placement : { "", "alt/" }) {
        ClearCache();
        auto rm = std::make_shared<Ship::ResourceManager>();
        Ship::Context::GetRawInstance()->rm = rm;
        rm->alt = true;
        for (const auto& [path, texture] : textures)
            rm->resources[placement + path] = texture;
        PlayState play{};
        play.sceneId = SCENE_26SARUNOMORI;
        for (int selected : { SEASON_AUTUMN, SEASON_WINTER, SEASON_SPRING, SEASON_SUMMER, SEASON_OFF }) {
            season = selected;
            MMAutumnSceneFoliage_Update(&play);
            for (const auto& [path, source] : textures) {
                const auto name = path.substr(path.rfind('/') + 1);
                const bool target = path.starts_with(prefix) && (grass.contains(name) || leaves.contains(name));
                const bool tinted = target && selected != SEASON_SUMMER && selected != SEASON_OFF;
                auto shown = WoodsFilepathFixture::Draw(path);
                if (tinted) {
                    if (shown == source)
                        std::fprintf(stderr, "FAIL Woods FILEPATH still uses untinted pixels: %s\n", path.c_str());
                    CheckSurfacePixels(source, shown); // Exact non-green, alpha and raw metadata.
                    CheckWoodsRawUpload(shown); // Native logical RGBA16 tile -> full raw RGBA32 image.
                    auto material = std::find_if(SurfaceMaterials(selected).begin(), SurfaceMaterials(selected).end(),
                                                 [&](const auto& item) { return path == item.path && item.scene == play.sceneId; });
                    assert(material != SurfaceMaterials(selected).end());
                    const uint32_t expected = grass.contains(name)
                        ? selected == SEASON_AUTUMN ? 0xD99C45FF : selected == SEASON_WINTER ? 0xE5EDF3FF : 0xF6C1D9FF
                        : selected == SEASON_AUTUMN ? 0xB96848FF : selected == SEASON_WINTER ? 0xCCDCEEFF : 0xF6C1D9E0;
                    assert(SeasonPalette(material->color, selected, play.sceneId, material->springGrass) == expected);
                } else {
                    assert(shown == source); // Black mouth and donor bark remain owned by their source.
                }
            }
            for (int room = 0; room < 9; ++room) {
                play.roomCtx.curRoom.num = room;
                MMAutumnSceneFoliage_Update(&play);
                assert(WoodsFilepathFixture::Draw(prefix + "grass") == rm->resources.at(placement + prefix + "grass"));
            }
        }
        season = SEASON_AUTUMN;
        MMAutumnSceneFoliage_Update(&play);
        auto retained = WoodsFilepathFixture::Draw(prefix + "grass");
        rm->alt = false;
        MMAutumnSceneFoliage_Update(&play);
        for (const auto& [path, source] : textures)
            assert(rm->resources.at(placement + path) == source);
        assert(retained->ImageData != textures.at(prefix + "grass")->ImageData);
        rm->alt = true;
        MMAutumnSceneFoliage_Update(&play);
        play.sceneId = SCENE_21MITURINMAE;
        MMAutumnSceneFoliage_Update(&play);
        for (const auto& [path, source] : textures)
            assert(rm->resources.at(placement + path) == source);
    }
    ClearCache();
    std::puts("PASS Woods actual XML FILEPATH: six selective images, nine rooms, three palettes; stone/bark/alpha/black and reset exact");
}

static void CheckWoodsRandomizerSelection() {
    const auto& paths = RoadRandoGrassPaths();
    for (const auto& path : paths) {
        assert(RoadRandoGrassColor(path, SEASON_AUTUMN, SCENE_26SARUNOMORI) == 0xD99C45FF);
        assert(RoadRandoGrassColor(path, SEASON_WINTER, SCENE_26SARUNOMORI) == 0xE5EDF3FF);
        assert(RoadRandoGrassColor(path, SEASON_SPRING, SCENE_26SARUNOMORI) == 0xF6C1D9FF);
        assert(RoadRandoGrassColor(path, SEASON_AUTUMN, SCENE_21MITURINMAE) == 0);
    }
    assert(WholeListColor("objects/gameplay_field_keep/gFieldBushMajorDL_tri_2", SEASON_AUTUMN,
                          SCENE_26SARUNOMORI) == 0); // Flower and stem stay unwrapped.
    for (int selected : { SEASON_AUTUMN, SEASON_WINTER, SEASON_SPRING }) {
        ClearCache();
        auto rm = std::make_shared<Ship::ResourceManager>();
        Ship::Context::GetRawInstance()->rm = rm;
        rm->alt = true;
        const auto leaf = paths.front();
        const std::string flower = "objects/gameplay_field_keep/gFieldBushRandomDL_tri_2";
        for (const auto& path : { leaf, flower }) {
            auto list = std::make_shared<Fast::DisplayList>();
            list->Instructions = { gsSP1Triangle(0, 1, 2, 0), gsSPEndDisplayList() };
            rm->archive->files["alt/" + path] = list;
        }
        PlayState play{};
        play.sceneId = SCENE_26SARUNOMORI;
        season = selected;
        MMAutumnSceneFoliage_Update(&play); // Must prepare a cold replacement leaf before first actor draw.
        const auto found = rm->resources.find("alt/" + leaf);
        assert(found != rm->resources.end());
        auto list = std::dynamic_pointer_cast<Fast::DisplayList>(found->second);
        const auto draw = ForestRendererFixture::Draw(list->Instructions);
        assert(draw.triangles == 1 && draw.tintModes.front() == (selected == SEASON_AUTUMN ? 1u : 3u));
        assert(!rm->resources.contains("alt/" + flower)); // Flower stays cold and unmodified.
        season = SEASON_OFF;
        MMAutumnSceneFoliage_Update(&play);
        assert(ForestRendererFixture::Draw(list->Instructions).tintModes.front() == 0);
    }
    ClearCache();
    std::puts("PASS Woods randomizer foliage children: cold Alt selection, submitted seasonal wrapper and protected flowers");
}

int main() {
    CheckWoodsSurfaces();
    CheckWoodsRandomizerSelection();
    CheckPatchedNativeCanopy();
    return 0;
}
