// Characterize the existing shared OoT selection: Autumn/Winter must reach
// every eligible reviewed texture reached by Spring, without changing math.
#include "soh/Enhancements/Graphics/SeasonFoliage.cpp"
#include "test_require.h"
#include <cstring>
#include <iostream>
#include <set>

Ship::Context* Ship::Context::current = nullptr;
GameInteractor* GameInteractor::Instance = nullptr;
extern "C" {
PlayState* gPlayState = nullptr;
}
static int selectedSeason = -1;
extern "C" int32_t SeasonWeather_SeasonForPlay(const PlayState*) {
    return selectedSeason;
}
extern "C" void gfx_texture_cache_clear() {
}

static std::shared_ptr<Fast::Texture> Texture() {
    auto init = std::make_shared<Ship::ResourceInitData>();
    init->Path = "custom/repacked/meta-target";
    init->IsCustom = true;
    auto image = std::make_shared<Fast::Texture>(init);
    image->Type = Fast::TextureType::RGBA16bpp;
    image->Flags = TEX_FLAG_LOAD_AS_RAW;
    image->Width = image->Height = 16;
    image->HByteScale = 8;
    image->VPixelScale = 4;
    image->ImageDataSize = 16 * 16 * 4;
    image->mImageBuffer = std::make_shared<std::vector<char>>(image->ImageDataSize);
    image->ImageData = reinterpret_cast<uint8_t*>(image->mImageBuffer->data());
    for (size_t at = 0; at < image->ImageDataSize; at += 4) {
        image->ImageData[at] = 40;
        image->ImageData[at + 1] = 110;
        image->ImageData[at + 2] = 25;
        image->ImageData[at + 3] = 83;
    }
    return image;
}

int main() {
    Ship::Context context;
    context.rm = std::make_shared<Ship::ResourceManager>();
    Ship::Context::current = &context;
    size_t cases = 0;
    std::set<std::tuple<std::string, int, bool>> visited;
    for (bool alt : { false, true }) {
        context.rm->alt = alt;
        for (const auto& material : kMaterials) {
            for (const char* prefix : { "", "alt/" }) {
                if ((*prefix && !alt) || (material.assets == Assets::Native && *prefix) ||
                    (material.assets == Assets::Alt && !*prefix))
                    continue;
                const int scene = material.scene >= 0 ? material.scene : SCENE_HYRULE_FIELD;
                const std::string path = std::string(prefix) + material.path;
                if (!visited.emplace(path, scene, alt).second)
                    continue;
                Retire();
                context.rm->cache.clear();
                const auto original = Texture();
                const auto pixels = *original->mImageBuffer;
                context.rm->cache[path] = original;
                const auto protectedImage = Texture();
                context.rm->cache["objects/object_link_child/green_tunic"] = protectedImage;
                PlayState play{};
                play.sceneNum = scene;
                std::map<int, std::vector<char>> firstPixels;
                for (int season : { 0, 2, 3, 0, 2, 3, 1, -1 }) {
                    selectedSeason = season;
                    Update(&play);
                    auto shown = std::dynamic_pointer_cast<Fast::Texture>(context.rm->cache.at(path));
                    const bool foliageSeason = season == 0 || season == 2 || season == 3;
                    REQUIRE((shown != original) == foliageSeason &&
                            "an eligible Spring material was missed in Autumn/Winter or leaked into Off/Summer");
                    if (foliageSeason) {
                        auto [it, fresh] = firstPixels.emplace(season, *shown->mImageBuffer);
                        REQUIRE(fresh || it->second == *shown->mImageBuffer);
                    }
                    REQUIRE(shown->GetInitData() == original->GetInitData());
                    REQUIRE(shown->Flags == original->Flags && shown->Type == original->Type &&
                            shown->HByteScale == 8 && shown->VPixelScale == 4);
                    for (size_t at = 3; at < shown->ImageDataSize; at += 4)
                        REQUIRE(shown->ImageData[at] == 83);
                    REQUIRE(*original->mImageBuffer == pixels);
                    REQUIRE(context.rm->cache.at("objects/object_link_child/green_tunic") == protectedImage);
                }
                ++cases;
            }
        }
    }
    Retire();
    Ship::Context::current = nullptr;
    std::cout << "PASS " << cases
              << " existing OoT query cohorts: Spring/Autumn/Winter parity, exact repeat "
                 "output, original/alpha/metadata preservation and Off/Summer restoration\n";
}
