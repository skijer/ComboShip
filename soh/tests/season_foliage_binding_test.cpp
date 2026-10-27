#include "soh/Enhancements/Graphics/SeasonFoliage.cpp"
#include "test_require.h"
#include <cstring>
#include <iostream>

Ship::Context* Ship::Context::current = nullptr;
GameInteractor* GameInteractor::Instance = nullptr;
// z_play.c owns this C symbol. A C++ declaration in the included production
// source must fail here instead of passing an ELF-only fixture.
extern "C" {
PlayState* gPlayState = nullptr;
}
static int season = -1;
static size_t evicted = 0;
extern "C" int32_t SeasonWeather_SeasonForPlay(const PlayState*) {
    return season;
}
extern "C" void gfx_texture_cache_clear() {
    ++evicted;
}

static std::shared_ptr<Fast::Texture> Make(const std::string& path) {
    auto init = std::make_shared<Ship::ResourceInitData>();
    init->Path = path;
    auto source = std::make_shared<Fast::Texture>(init);
    source->Type = Fast::TextureType::RGBA16bpp;
    source->Flags = TEX_FLAG_LOAD_AS_RAW;
    source->Width = source->Height = 1;
    source->ImageDataSize = 4;
    source->mImageBuffer = std::make_shared<std::vector<char>>(std::initializer_list<char>{ 60, 110, 70, 83 });
    source->ImageData = reinterpret_cast<uint8_t*>(source->mImageBuffer->data());
    return source;
}

static void CheckCliffBandRounding() {
    // Explicit row boundaries include heights smaller than either atlas band.
    struct Bounds {
        unsigned height, topEnd, bottomStart;
    };
    for (const Bounds item : { Bounds{ 1, 1, 1 },
                               { 2, 1, 2 },
                               { 3, 1, 3 },
                               { 7, 1, 7 },
                               { 8, 1, 8 },
                               { 15, 2, 15 },
                               { 16, 2, 15 },
                               { 17, 3, 16 },
                               { 31, 4, 30 },
                               { 32, 4, 30 },
                               { 33, 5, 31 } }) {
        for (int format : { 0, 1, 2 }) {
            const auto source = Make("fixture/cliff-bands");
            source->Width = 1;
            source->Height = item.height;
            source->Type = format == 2 ? Fast::TextureType::RGBA32bpp : Fast::TextureType::RGBA16bpp;
            source->Flags = format == 1 ? TEX_FLAG_LOAD_AS_RAW : 0;
            const size_t stride = format == 0 ? 2 : 4;
            source->ImageDataSize = item.height * stride;
            source->mImageBuffer = std::make_shared<std::vector<char>>(source->ImageDataSize);
            source->ImageData = reinterpret_cast<uint8_t*>(source->mImageBuffer->data());
            for (size_t at = 0; at < source->ImageDataSize; at += stride) {
                source->ImageData[at] = format == 0 ? 0x2B : 40;
                source->ImageData[at + 1] = format == 0 ? 0x87 : 110;
                if (format != 0) {
                    source->ImageData[at + 2] = 25;
                    source->ImageData[at + 3] = 83;
                }
            }
            const auto before = *source->mImageBuffer;
            for (int selected : { 0, 2, 3 }) {
                const auto copy =
                    BuildTexture(source, Palette(selected, Surface::Ground), {}, PixelMask::KakarikoCliffGrass);
                REQUIRE(copy);
                for (unsigned y = 0; y < item.height; ++y) {
                    const size_t at = y * stride;
                    REQUIRE((std::memcmp(copy->ImageData + at, source->ImageData + at, stride) != 0) ==
                            (y < item.topEnd || y >= item.bottomStart));
                    REQUIRE(format == 0 ? (copy->ImageData[at + 1] & 1) == (source->ImageData[at + 1] & 1)
                                        : copy->ImageData[at + 3] == source->ImageData[at + 3]);
                }
                REQUIRE(*source->mImageBuffer == before);
            }
        }
    }
}

static void CheckSceneTerrain(Ship::Context& context) {
    struct Case {
        const char* path;
        int16_t scene;
        SeasonFoliage::Surface surface;
        unsigned top = 0, bottom = 16;
        bool cliffBands = false;
    };
    const Case cases[] = {
        { "scenes/shared/spot01_scene/spot01_sceneTex_00AA50", SCENE_KAKARIKO_VILLAGE, Surface::Ground },
        { "scenes/shared/spot01_scene/spot01_sceneTex_018B50", SCENE_KAKARIKO_VILLAGE, Surface::Ground },
        { "scenes/shared/spot01_scene/spot01_sceneTex_019350", SCENE_KAKARIKO_VILLAGE, Surface::Ground },
        { "scenes/shared/spot03_scene/spot03_sceneTex_009D58", SCENE_ZORAS_RIVER, Surface::Ground },
        { "scenes/shared/spot03_scene/spot03_sceneTex_00A558", SCENE_ZORAS_RIVER, Surface::Ground },
        { "scenes/shared/spot20_scene/spot20_sceneTex_00D1E0", SCENE_LON_LON_RANCH, Surface::Ground },
        { "scenes/shared/spot20_scene/spot20_sceneTex_00D9E0", SCENE_LON_LON_RANCH, Surface::Ground },
        { "scenes/shared/spot01_scene/s01_kabe_03_2E680", SCENE_KAKARIKO_VILLAGE, Surface::Ground, 0, 2, true },
        { "scenes/shared/spot01_scene/s01_saku_01_5BB00", SCENE_KAKARIKO_VILLAGE, Surface::Understory, 12, 16 },
        { "custom/prelude/spot01_scene/recipe1_tex0", SCENE_KAKARIKO_VILLAGE, Surface::Understory },
        { "custom/prelude/spot01_scene/recipe8_tex0", SCENE_KAKARIKO_VILLAGE, Surface::Ground },
        { "custom/prelude/spot01_scene/recipe14_tex0", SCENE_KAKARIKO_VILLAGE, Surface::Canopy },
        // Previously supported scene aliases gain the same ownership boundary.
        { "scenes/shared/spot01_scene/s01_yuka_01_78900", SCENE_KAKARIKO_VILLAGE, Surface::Ground },
        { "scenes/shared/spot03_scene/s03_ki_01_13B00", SCENE_ZORAS_RIVER, Surface::Canopy },
        { "scenes/shared/spot20_scene/s20_kusa_01_3D300", SCENE_LON_LON_RANCH, Surface::Understory },
    };
    for (const auto& item : cases) {
        for (bool alt : { false, true }) {
            context.rm->alt = alt;
            for (const char* prefix : { "", "alt/" }) {
                for (int format : { 0, 1, 2 }) {
                    // A `.meta` may redirect a known binding to any replacement.
                    // Ownership comes from the known binding, never the target name.
                    const auto source = Make("custom/future/green-ground-replacement");
                    source->GetInitData()->IsCustom = true;
                    source->Width = source->Height = 16;
                    source->Type = format == 2 ? Fast::TextureType::RGBA32bpp : Fast::TextureType::RGBA16bpp;
                    source->Flags = format == 1 ? TEX_FLAG_LOAD_AS_RAW : 0;
                    const size_t stride = format == 0 ? 2 : 4;
                    source->ImageDataSize = 16 * 16 * stride;
                    source->HByteScale = 8;
                    source->VPixelScale = 4;
                    source->mImageBuffer = std::make_shared<std::vector<char>>(source->ImageDataSize);
                    source->ImageData = reinterpret_cast<uint8_t*>(source->mImageBuffer->data());
                    for (size_t at = 0; at < source->ImageDataSize; at += stride) {
                        const bool grass = (at / stride) % 2 == 0;
                        if (format == 0) {
                            // Green grass, then warm stone. Preserve both alpha bits.
                            source->ImageData[at] = grass ? 0x2B : 0x5A;
                            source->ImageData[at + 1] = grass ? 0x87 : 0x90;
                        } else {
                            source->ImageData[at] = grass ? 40 : 95;
                            source->ImageData[at + 1] = grass ? 110 : 85;
                            source->ImageData[at + 2] = grass ? 25 : 70;
                            source->ImageData[at + 3] = grass ? 83 : 0;
                        }
                    }
                    const auto bytes = *source->mImageBuffer;
                    const std::string path = std::string(prefix) + item.path;
                    context.rm->cache[path] = source;
                    // Unknown replacement names and unrelated green materials stay native.
                    const std::string unmapped = std::string(prefix) + "custom/future/new-unmapped-material";
                    const auto protectedImage = Make(unmapped);
                    context.rm->cache[unmapped] = protectedImage;
                    const auto metadata = source->GetInitData();
                    PlayState play{};
                    play.sceneNum = item.scene;
                    for (int selected : { 0, 2, 3, 1, 2, -1 }) {
                        season = selected;
                        Update(&play);
                        const bool tinted = (!*prefix || alt) && (selected == 0 || selected == 2 || selected == 3);
                        auto shown = std::dynamic_pointer_cast<Fast::Texture>(context.rm->cache.at(path));
                        REQUIRE((shown != source) == tinted && "scene-owned terrain/foliage binding was missed");
                        if (tinted && std::strcmp(item.path, "custom/prelude/spot01_scene/recipe14_tex0") == 0) {
                            const Tint expectedTint = selected == 0   ? Tint{ 0xF6C1D9E0u, 461 }
                                                      : selected == 2 ? Tint{ 0xEF484CFFu, 256 }
                                                                      : Tint{ 0xE2EEE9FFu, 461 };
                            const auto expected = BuildTexture(source, expectedTint);
                            REQUIRE(expected && *shown->mImageBuffer == *expected->mImageBuffer &&
                                    "Kakariko windmill backdrop must inherit its Canopy role hue");
                        }
                        for (unsigned y = 0; y < 16; ++y)
                            for (unsigned x = 0; x < 16; ++x) {
                                const size_t at = (y * 16 + x) * stride;
                                const bool expected =
                                    tinted && x % 2 == 0 &&
                                    ((y >= item.top && y < item.bottom) || (item.cliffBands && y >= 15));
                                REQUIRE((std::memcmp(shown->ImageData + at, source->ImageData + at, stride) != 0) ==
                                            expected &&
                                        "grass region or protected stone changed incorrectly");
                                REQUIRE(format == 0 ? (shown->ImageData[at + 1] & 1) == (source->ImageData[at + 1] & 1)
                                                    : shown->ImageData[at + 3] == source->ImageData[at + 3]);
                            }
                        REQUIRE(shown->GetInitData() == metadata && metadata->IsCustom &&
                                metadata->Path == "custom/future/green-ground-replacement");
                        REQUIRE(shown->Flags == source->Flags && shown->Type == source->Type &&
                                shown->Width == source->Width && shown->Height == source->Height &&
                                shown->HByteScale == source->HByteScale && shown->VPixelScale == source->VPixelScale);
                        REQUIRE(*source->mImageBuffer == bytes && context.rm->cache.at(unmapped) == protectedImage);
                    }
                    season = 2;
                    Update(&play);
                    const auto autumn = context.rm->cache.at(path);
                    play.sceneNum = SCENE_HYRULE_FIELD;
                    Update(&play);
                    REQUIRE(context.rm->cache.at(path) == source &&
                            "outgoing scene leaked its tint into another scene");
                    play.sceneNum = item.scene;
                    Update(&play);
                    REQUIRE(context.rm->cache.at(path) == autumn && "revisit did not reuse the original-source copy");
                    Update(nullptr);
                    REQUIRE(context.rm->cache.at(path) == source && "a missing scene must restore owned textures");
                    Update(&play);
                    context.rm->alt = false;
                    Update(&play);
                    if (*prefix)
                        REQUIRE(context.rm->cache.at(path) == source && "Alt-off did not restore an alias binding");
                    Retire();
                    context.rm->cache.clear();
                    context.rm->alt = alt;
                }
            }
        }
    }
    // Saturated green buildings/props/water must stay out of the scene registry.
    for (const char* path :
         { "scenes/shared/spot01_scene/s01_yane_01_68D00", "scenes/shared/spot01_scene/s01_iekabe_04_21480",
           "scenes/shared/spot01_scene/s01_kabe_01_23E80", "scenes/shared/spot01_scene/spot01_sceneTex_01B750",
           "objects/object_spot01_objects/gKakarikoWellWaterTex", "objects/object_link_child/green_tunic",
           "custom/prelude/spot01_scene/recipe6_tex0", "custom/prelude/spot01_scene/recipe19_tex0",
           "scenes/shared/spot03_scene/spot03_sceneTex_008D58", "scenes/shared/spot20_scene/spot20_sceneTex_005FE0" }) {
        for (const char* prefix : { "", "alt/" }) {
            const std::string key = std::string(prefix) + path;
            const auto source = Make(key);
            context.rm->cache[key] = source;
            for (int16_t scene : { SCENE_KAKARIKO_VILLAGE, SCENE_ZORAS_RIVER, SCENE_LON_LON_RANCH }) {
                PlayState play{};
                play.sceneNum = scene;
                for (int selected : { 0, 2, 3 }) {
                    season = selected;
                    Update(&play);
                    REQUIRE(context.rm->cache.at(key) == source && "an unrelated green resource received the tint");
                }
            }
            Retire();
            context.rm->cache.clear();
        }
    }
}

static void CheckPavingSeams(Ship::Context& context) {
    const char* roads[] = { "scenes/shared/spot00_scene/s00_road_01_41180",
                            "scenes/shared/spot00_scene/s00_road_02_46580" };
    const char* ground = "scenes/shared/spot00_scene/s00_yuka_01_78B00___Copie";
    for (bool packed : { false, true }) {
        for (bool alt : { false, true }) {
            context.rm->alt = alt;
            for (const char* prefix : { "", "alt/" }) {
                const auto source = Make("fixture/shared-paving");
                source->Width = 3;
                source->Flags = packed ? 0 : TEX_FLAG_LOAD_AS_RAW;
                // Grass, slightly green stone, and warm stone. Sharing this
                // allocation with ordinary grass also checks copy-cache isolation.
                const std::vector<uint8_t> pixels =
                    packed ? std::vector<uint8_t>{ 0x2B, 0x87, 0x7C, 0x1C, 0x5A, 0x91 }
                           : std::vector<uint8_t>{ 40, 110, 25, 83, 120, 130, 118, 255, 95, 85, 70, 0 };
                source->mImageBuffer = std::make_shared<std::vector<char>>(pixels.begin(), pixels.end());
                source->ImageData = reinterpret_cast<uint8_t*>(source->mImageBuffer->data());
                source->ImageDataSize = pixels.size();
                const auto original = *source->mImageBuffer;
                const size_t stride = packed ? 2 : 4;
                for (const char* road : roads)
                    context.rm->cache[std::string(prefix) + road] = source;
                context.rm->cache[std::string(prefix) + ground] = source;
                for (int selected : { 0, 2, 3, 1, 2, -1 }) {
                    season = selected;
                    Update(nullptr);
                    const bool tinted = (!*prefix || alt) && (selected == 0 || selected == 2 || selected == 3);
                    for (const char* road : roads) {
                        const auto shown =
                            std::dynamic_pointer_cast<Fast::Texture>(context.rm->cache.at(std::string(prefix) + road));
                        REQUIRE((shown != source) == tinted && "paving seam grass missed the seasonal binding");
                        REQUIRE((std::memcmp(shown->ImageData, source->ImageData, stride) != 0) == tinted);
                        if (tinted) {
                            const auto ordinary = std::dynamic_pointer_cast<Fast::Texture>(
                                context.rm->cache.at(std::string(prefix) + ground));
                            REQUIRE(std::memcmp(shown->ImageData, ordinary->ImageData, stride) == 0 &&
                                    "grass and paving grass trim must share the identical Ground treatment");
                            const Tint expectedTint = selected == 0   ? Tint{ 0xF6C1D9FFu, 461, 24 }
                                                      : selected == 2 ? Tint{ 0xF3974FFFu, 320, 24 }
                                                                      : Tint{ 0xDCECF5FFu, 461, 128 };
                            const auto expected = BuildTexture(source, expectedTint, {}, PixelMask::GrassTrim);
                            REQUIRE(expected && *shown->mImageBuffer == *expected->mImageBuffer);
                        }
                        REQUIRE(std::memcmp(shown->ImageData + stride, source->ImageData + stride, stride * 2) == 0 &&
                                "paving stones must retain their exact source pixels");
                        REQUIRE(shown->Type == source->Type && shown->Flags == source->Flags &&
                                shown->Width == source->Width && shown->Height == source->Height &&
                                shown->HByteScale == source->HByteScale && shown->VPixelScale == source->VPixelScale);
                        for (size_t i = 0; i < pixels.size(); i += stride)
                            REQUIRE(packed ? (shown->ImageData[i + 1] & 1) == (source->ImageData[i + 1] & 1)
                                           : shown->ImageData[i + 3] == source->ImageData[i + 3]);
                    }
                    if (tinted) {
                        const auto ordinary = std::dynamic_pointer_cast<Fast::Texture>(
                            context.rm->cache.at(std::string(prefix) + ground));
                        REQUIRE(std::memcmp(ordinary->ImageData + stride, source->ImageData + stride, stride) != 0 &&
                                "grass trim must not narrow ordinary terrain's existing selection");
                    }
                    REQUIRE(*source->mImageBuffer == original);
                }
                Retire();
                context.rm->cache.clear();
            }
        }
    }
}

static void CheckActualAliasSelections(Ship::Context& context) {
    // One .meta target can back both requested cache identifiers. Prefix,
    // rather than metadata/custom status or global Alt mode, owns the mask.
    const char* river = "scenes/shared/spot03_scene/s03_kabe_01_3F00";
    const auto source = Make("custom/shared/meta-target");
    source->GetInitData()->IsCustom = true;
    source->Width = source->Height = 16;
    source->ImageDataSize = 16 * 16 * 4;
    source->mImageBuffer = std::make_shared<std::vector<char>>(source->ImageDataSize);
    source->ImageData = reinterpret_cast<uint8_t*>(source->mImageBuffer->data());
    for (size_t at = 0; at < source->ImageDataSize; at += 4) {
        source->ImageData[at] = 40;
        source->ImageData[at + 1] = 110;
        source->ImageData[at + 2] = 25;
        source->ImageData[at + 3] = 83;
    }
    const auto original = *source->mImageBuffer;
    const std::string native = river, alt = "alt/" + native;
    context.rm->alt = true;
    context.rm->cache[native] = source;
    PlayState play{};
    play.sceneNum = SCENE_ZORAS_RIVER;
    season = 2;
    Update(&play);
    const auto fallback = context.rm->cache.at(native);
    REQUIRE(fallback != source && !context.rm->cache.contains(alt) && "Alt-on native fallback must use native mask");
    context.rm->cache[alt] = source;
    Update(&play);
    const auto alternate = context.rm->cache.at(alt);
    REQUIRE(alternate != source && alternate != fallback && context.rm->cache.at(native) == fallback);
    for (unsigned y = 0; y < 16; ++y) {
        const auto n = std::dynamic_pointer_cast<Fast::Texture>(fallback);
        const auto a = std::dynamic_pointer_cast<Fast::Texture>(alternate);
        REQUIRE((std::memcmp(n->ImageData + y * 64, source->ImageData + y * 64, 4) != 0) == (y < 3 || y >= 14));
        REQUIRE((std::memcmp(a->ImageData + y * 64, source->ImageData + y * 64, 4) != 0) == (y < 3 || y >= 15));
        REQUIRE(n->GetInitData() == source->GetInitData() && a->GetInitData() == source->GetInitData());
    }
    context.rm->alt = false;
    Update(&play);
    REQUIRE(context.rm->cache.at(native) == fallback && context.rm->cache.at(alt) == source);
    context.rm->alt = true;
    Update(&play);
    REQUIRE(context.rm->cache.at(alt) == alternate);
    play.sceneNum = SCENE_LON_LON_RANCH;
    Update(&play);
    REQUIRE(context.rm->cache.at(native) == source && context.rm->cache.at(alt) == source);
    play.sceneNum = SCENE_ZORAS_RIVER;
    Update(&play);
    REQUIRE(context.rm->cache.at(native) == fallback && context.rm->cache.at(alt) == alternate);
    const auto external = Make("custom/external-reload");
    context.rm->cache[alt] = external;
    Reset();
    REQUIRE(context.rm->cache.at(native) == source && context.rm->cache.at(alt) == external);
    REQUIRE(*source->mImageBuffer == original);
    Retire();
    context.rm->cache.clear();
    const std::string masonry = "scenes/shared/spot20_scene/s20_kabe_03_1B100";
    context.rm->cache[masonry] = context.rm->cache["alt/" + masonry] = source;
    play.sceneNum = SCENE_LON_LON_RANCH;
    Update(&play);
    REQUIRE(context.rm->cache.at(masonry) != source && context.rm->cache.at("alt/" + masonry) == source &&
            "Native fallback grass and Alt masonry require distinct binding selections");
    Retire();
    REQUIRE(context.rm->cache.at(masonry) == source && context.rm->cache.at("alt/" + masonry) == source);
    context.rm->cache.clear();
}
static void CheckActualArchives(Ship::Context& context) {
    CheckActualAliasSelections(context);
    // The expected atlas rectangles come from decoded actual-export audits.
    // All-green protected interiors make an accidental whole-image tint fail.
    struct Case {
        const char* path;
        int16_t scene;
        std::vector<Region> native, alt;
        std::vector<uint64_t> contour{};
    };
    const Case cases[] = {
        { "scenes/shared/spot05_scene/sp05_kabe_05_500",
          SCENE_SACRED_FOREST_MEADOW,
          { { 0, 0, 16, 4 }, { 0, 14, 16, 16 } },
          { { 0, 0, 16, 4 }, { 0, 14, 16, 16 } } },
        { "scenes/shared/spot05_scene/sp5_kabe_01_26600",
          SCENE_SACRED_FOREST_MEADOW,
          { { 0, 0, 16, 4 }, { 0, 14, 16, 16 } },
          { { 0, 0, 16, 4 }, { 0, 14, 16, 16 } } },
        { "scenes/shared/spot05_scene/sp5_kabe_02_30E00",
          SCENE_SACRED_FOREST_MEADOW,
          { { 0, 0, 16, 4 }, { 0, 14, 16, 16 } },
          { { 0, 0, 16, 4 }, { 0, 14, 16, 16 } } },
        { "scenes/shared/spot05_scene/sp5_kabe_04_3B600",
          SCENE_SACRED_FOREST_MEADOW,
          { { 0, 0, 16, 4 }, { 0, 14, 16, 16 } },
          { { 0, 0, 16, 4 }, { 0, 14, 16, 16 } } },
        { "scenes/shared/spot05_scene/sp5_yuka_01_62180",
          SCENE_SACRED_FOREST_MEADOW,
          { { 0, 0, 16, 16 } },
          { { 0, 0, 16, 16 } } },
        { "scenes/shared/spot05_scene/sp5_ha_01_5900",
          SCENE_SACRED_FOREST_MEADOW,
          { { 0, 0, 16, 16 } },
          { { 0, 0, 16, 16 } } },
        { "scenes/shared/spot05_scene/sp05_kusa_02_1A00",
          SCENE_SACRED_FOREST_MEADOW,
          { { 0, 0, 16, 16 } },
          { { 0, 0, 16, 16 } } },
        { "scenes/shared/spot05_scene/sp05_kusa_03_4400",
          SCENE_SACRED_FOREST_MEADOW,
          { { 0, 0, 16, 16 } },
          { { 0, 0, 16, 16 } } },
        { "scenes/shared/spot05_scene/sp5_leaf_01_50600",
          SCENE_SACRED_FOREST_MEADOW,
          { { 0, 0, 16, 16 } },
          { { 0, 0, 16, 16 } } },
        { "scenes/shared/spot05_scene/sp5_kabe_03_36200",
          SCENE_SACRED_FOREST_MEADOW,
          { { 0, 0, 16, 1 } },
          { { 0, 0, 16, 1 } } },
        { "scenes/shared/spot05_scene/sp5_kabe_03_36205",
          SCENE_SACRED_FOREST_MEADOW,
          { { 0, 0, 16, 1 } },
          { { 0, 0, 16, 1 } } },
        { "scenes/shared/spot05_scene/sp5_mon_01_5AE00",
          SCENE_SACRED_FOREST_MEADOW,
          { { 0, 15, 16, 16 } },
          { { 0, 15, 16, 16 } } },
        { "scenes/shared/spot05_scene/sp5_iseki_01_1BE00",
          SCENE_SACRED_FOREST_MEADOW,
          { { 0, 0, 8, 4 }, { 0, 15, 8, 16 } },
          { { 0, 0, 8, 4 }, { 0, 15, 8, 16 } } },
        { "scenes/shared/spot03_scene/s03_tuta_01_1A300",
          SCENE_ZORAS_RIVER,
          { { 0, 0, 16, 16 } },
          { { 0, 0, 16, 16 } } },
        { "scenes/shared/spot03_scene/s03_saku_01_19880",
          SCENE_ZORAS_RIVER,
          { { 0, 13, 16, 16 } },
          { { 0, 13, 16, 16 } } },
        { "scenes/shared/spot03_scene/s03_mon_01_18380",
          SCENE_ZORAS_RIVER,
          { { 0, 14, 16, 16 } },
          { { 0, 15, 16, 16 } } },
        { "scenes/shared/spot03_scene/s03_kabe_01_3F00",
          SCENE_ZORAS_RIVER,
          { { 0, 0, 16, 3 }, { 0, 14, 16, 16 } },
          { { 0, 0, 16, 3 }, { 0, 15, 16, 16 } } },
        { "scenes/shared/spot20_scene/s00_saku_01_2C00",
          SCENE_LON_LON_RANCH,
          { { 0, 14, 16, 16 } },
          { { 0, 14, 16, 16 } } },
        { "scenes/shared/spot20_scene/s20_gate_01_3680",
          SCENE_LON_LON_RANCH,
          { { 0, 15, 16, 16 } },
          { { 0, 15, 16, 16 } } },
        { "scenes/shared/spot20_scene/s20_kabe_00_6100",
          SCENE_LON_LON_RANCH,
          { { 0, 0, 16, 3 }, { 0, 14, 16, 16 } },
          { { 0, 0, 16, 3 }, { 0, 14, 16, 16 } } },
        { "scenes/shared/spot20_scene/s20_kabe_01_10900",
          SCENE_LON_LON_RANCH,
          { { 0, 0, 16, 3 }, { 0, 14, 16, 16 } },
          { { 0, 0, 16, 3 }, { 0, 14, 16, 16 } } },
        { "scenes/shared/spot20_scene/s20_kabe_03_1B100",
          SCENE_LON_LON_RANCH,
          { { 0, 0, 8, 1 }, { 7, 15, 16, 16 } },
          {} },
        { "scenes/shared/spot20_scene/s20_kikabe_01_3A900",
          SCENE_LON_LON_RANCH,
          { { 0, 14, 16, 16 } },
          { { 0, 14, 16, 16 } } },
        { "scenes/shared/spot20_scene/s20_saku_01_5A100",
          SCENE_LON_LON_RANCH,
          { { 0, 15, 16, 16 } },
          { { 0, 15, 16, 16 } } },
        { "scenes/shared/spot20_scene/s20_ki_01_30100",
          SCENE_LON_LON_RANCH,
          { { 0, 7, 2, 8 },
            { 4, 7, 6, 8 },
            { 0, 15, 2, 16 },
            { 8, 15, 11, 16 },
            { 12, 0, 14, 16 },
            { 14, 15, 16, 16 } },
          { { 0, 7, 2, 8 },
            { 4, 7, 6, 8 },
            { 0, 15, 2, 16 },
            { 8, 15, 11, 16 },
            { 12, 0, 14, 16 },
            { 14, 15, 16, 16 } } },
        { "scenes/shared/spot20_scene/s20_road_01_45100",
          SCENE_LON_LON_RANCH,
          { { 0, 0, 16, 16 } },
          { { 0, 0, 16, 16 } } },
        { "scenes/shared/spot20_scene/s20_road_02_4F900",
          SCENE_LON_LON_RANCH,
          { { 0, 0, 16, 16 } },
          { { 0, 0, 16, 16 } } },
        { "scenes/shared/spot20_scene/s20_yuka_01_67300",
          SCENE_LON_LON_RANCH,
          { { 0, 0, 16, 16 } },
          { { 0, 0, 16, 16 } } },
        { "scenes/shared/spot20_scene/s20_yuka_02_71B00",
          SCENE_LON_LON_RANCH,
          { { 0, 0, 16, 16 } },
          { { 0, 0, 16, 16 } } },
        { "scenes/shared/spot20_scene/s20_kusa_01_3D300",
          SCENE_LON_LON_RANCH,
          { { 0, 0, 16, 16 } },
          { { 0, 0, 16, 16 } } },
        { "scenes/shared/spot03_scene/s03_yuka_01_24A80",
          SCENE_ZORAS_RIVER,
          { { 0, 0, 16, 16 } },
          { { 0, 0, 16, 16 } } },
        { "scenes/shared/spot03_scene/s03_yuka_02_2BD80",
          SCENE_ZORAS_RIVER,
          {},
          {},
          { 0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL,
            0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL,
            0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL,
            0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL,
            0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL,
            0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL,
            0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL, 0xfc00ffffffffffffULL,
            0xfc00ffffffffffffULL, 0xfc00c01ff8006067ULL, 0xfc00e017f410e03fULL, 0xfc00c001e400000fULL,
            0x000000006000001fULL, 0x000000004000001cULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
            0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
            0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
            0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
            0x00000000ffffffffULL, 0x000000007ffffffeULL, 0x000000003ffffffcULL, 0x000000001ffffffcULL,
            0x0000000007ffffb0ULL, 0x0000000007ffefa0ULL, 0x0000000000ffe484ULL, 0x0000000004ffd800ULL,
            0x000000000478b800ULL, 0x0000000001206010ULL, 0x000000001f00051eULL, 0x000000001e00079eULL,
            0x000000003fc81f9eULL, 0x000000002ffa7f5eULL, 0x000000007ffe7ffeULL, 0x00000000ffffffffULL } },
        { "scenes/shared/spot03_scene/s03_yuka_03_2F280",
          SCENE_ZORAS_RIVER,
          {},
          {},
          { 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL,
            0xffffffffffffffffULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL,
            0xffffffffffffffffULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL,
            0xffffff8fffffffffULL, 0xffffe60001ffffffULL, 0xffff00000000ffffULL, 0xffff000000001fffULL,
            0xfffc000000001fffULL, 0xfff8000000001fffULL, 0xfff0000000001fffULL, 0xfff0000000000fffULL,
            0xfff0000000000fffULL, 0xfff0000000000fffULL, 0xfff0000000000fffULL, 0xffe0000000001fffULL,
            0xffe0000000000fffULL, 0xffe0000000000fffULL, 0xffe0000000000fffULL, 0xffe0000000000fffULL,
            0xffe0000000000fffULL, 0xfff0000000000fffULL, 0xfff0000000000fffULL, 0xfff0000000000fffULL,
            0xfff0000000000fffULL, 0xfff0000000000fffULL, 0xfff0000000000fffULL, 0xffe0000000000fffULL,
            0xfff0000000000fffULL, 0xfff0000000000fffULL, 0xfff0000000000fffULL, 0xfff0000000000fffULL,
            0xfff0000000000fffULL, 0xfff8000000001fffULL, 0xfff0000000001fffULL, 0xfff0000000001fffULL,
            0xfff0000000000fffULL, 0xfff0000000000fffULL, 0xfff0000000000fffULL, 0xffe0000000001fffULL,
            0xffe0000000001fffULL, 0xfff0000000001fffULL, 0xfff0000000001fffULL, 0xfff8000000007fffULL,
            0xfffe00000000ffffULL, 0xffffe0000001ffffULL, 0xfffffffe081fffffULL, 0xffffffffffffffffULL,
            0xffffffffffffffffULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL,
            0xffffffffffffffffULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL } },
        { "scenes/shared/spot03_scene/s03_kabekusa_01_E700",
          SCENE_ZORAS_RIVER,
          { { 0, 0, 16, 16 } },
          { { 0, 0, 16, 16 } } },
        { "scenes/shared/spot03_scene/s03_ki_01_13B00", SCENE_ZORAS_RIVER, { { 0, 0, 16, 16 } }, { { 0, 0, 16, 16 } } },
        { "scenes/shared/spot03_scene/s03_kui_01_18F00",
          SCENE_ZORAS_RIVER,
          { { 0, 15, 8, 16 } },
          { { 0, 15, 8, 16 } } },
        { "scenes/shared/spot03_scene/s03_kusa_03_19980",
          SCENE_ZORAS_RIVER,
          { { 0, 0, 16, 16 } },
          { { 0, 0, 16, 16 } } },
        { "scenes/shared/spot03_scene/s03_tw_01_1CD00", SCENE_ZORAS_RIVER, { { 0, 0, 16, 16 } }, { { 0, 0, 16, 16 } } },
        { "scenes/shared/spot03_scene/spot03_room_1Tex_005ED8",
          SCENE_ZORAS_RIVER,
          { { 0, 15, 16, 16 } },
          { { 0, 15, 16, 16 } } },
        { "scenes/shared/spot03_scene/s03_yuka_04_30780", SCENE_ZORAS_RIVER, {}, {} },
        { "scenes/shared/spot05_scene/spot05_sceneTex_006D60",
          SCENE_SACRED_FOREST_MEADOW,
          { { 0, 0, 16, 16 } },
          { { 0, 0, 16, 16 } } },
        { "scenes/shared/spot05_scene/spot05_sceneTex_007D60",
          SCENE_SACRED_FOREST_MEADOW,
          { { 0, 0, 16, 16 } },
          { { 0, 0, 16, 16 } } },
        { "scenes/shared/spot05_scene/spot05_sceneTex_00DB60",
          SCENE_SACRED_FOREST_MEADOW,
          { { 0, 0, 16, 16 } },
          { { 0, 0, 16, 16 } } },
        { "scenes/shared/spot05_scene/spot05_sceneTex_011360",
          SCENE_SACRED_FOREST_MEADOW,
          { { 0, 0, 16, 4 }, { 0, 13, 16, 16 } },
          { { 0, 0, 16, 4 }, { 0, 13, 16, 16 } } },
        { "scenes/shared/spot05_scene/spot05_sceneTex_012360",
          SCENE_SACRED_FOREST_MEADOW,
          { { 0, 0, 16, 16 } },
          { { 0, 0, 16, 16 } } },
        { "scenes/shared/spot05_scene/spot05_sceneTex_013360",
          SCENE_SACRED_FOREST_MEADOW,
          { { 0, 0, 16, 16 } },
          { { 0, 0, 16, 16 } } },
        { "scenes/shared/spot05_scene/spot05_sceneTex_013B60",
          SCENE_SACRED_FOREST_MEADOW,
          { { 0, 0, 16, 16 } },
          { { 0, 0, 16, 16 } } },
        { "scenes/shared/spot05_scene/spot05_sceneTex_014360",
          SCENE_SACRED_FOREST_MEADOW,
          { { 0, 0, 16, 16 } },
          { { 0, 0, 16, 16 } } },
    };
    for (const auto& item : cases) {
        for (bool altEnabled : { false, true }) {
            context.rm->alt = altEnabled;
            for (const char* prefix : { "", "alt/" }) {
                const auto& regions = *prefix ? item.alt : item.native;
                for (int format : { 0, 1, 2 }) {
                    const auto source = Make("custom/future/actual-archive-meta-target");
                    source->GetInitData()->IsCustom = true;
                    const unsigned size = item.contour.empty() ? 16 : 64;
                    source->Width = source->Height = size;
                    source->Type = format == 2 ? Fast::TextureType::RGBA32bpp : Fast::TextureType::RGBA16bpp;
                    source->Flags = format == 1 ? TEX_FLAG_LOAD_AS_RAW : 0;
                    source->HByteScale = 8;
                    source->VPixelScale = 4;
                    const size_t stride = format == 0 ? 2 : 4;
                    source->ImageDataSize = size * size * stride;
                    source->mImageBuffer = std::make_shared<std::vector<char>>(source->ImageDataSize);
                    source->ImageData = reinterpret_cast<uint8_t*>(source->mImageBuffer->data());
                    for (size_t at = 0; at < source->ImageDataSize; at += stride) {
                        const bool grass = at / stride != size * size - 1;
                        source->ImageData[at] = format == 0 ? (grass ? 0x2B : 0x5A) : (grass ? 40 : 95);
                        source->ImageData[at + 1] = format == 0 ? (grass ? 0x87 : 0x90) : (grass ? 110 : 85);
                        if (format != 0) {
                            source->ImageData[at + 2] = grass ? 25 : 70;
                            source->ImageData[at + 3] = grass ? 83 : 0;
                        }
                    }
                    const auto bytes = *source->mImageBuffer;
                    const auto metadata = source->GetInitData();
                    const std::string path = std::string(prefix) + item.path;
                    context.rm->cache[path] = source;
                    const auto unmapped = Make("custom/unmapped/green-material");
                    context.rm->cache["custom/unmapped/green-material"] = unmapped;
                    PlayState play{};
                    play.sceneNum = item.scene;
                    for (int selected : { 0, 2, 3, 1, 2, -1 }) {
                        season = selected;
                        Update(&play);
                        const bool tinted = (!*prefix || altEnabled) && (!regions.empty() || !item.contour.empty()) &&
                                            (selected == 0 || selected == 2 || selected == 3);
                        const auto shown = std::dynamic_pointer_cast<Fast::Texture>(context.rm->cache.at(path));
                        REQUIRE((shown != source) == tinted && "actual archive binding selection was missed");
                        for (unsigned y = 0; y < size; ++y)
                            for (unsigned x = 0; x < size; ++x) {
                                bool inRegion = false;
                                for (const auto& r : regions)
                                    inRegion |= x * 16 >= size * r.left && x * 16 < size * r.right &&
                                                y * 16 >= size * r.top && y * 16 < size * r.bottom;
                                if (!item.contour.empty())
                                    inRegion = (item.contour[y] & (uint64_t{ 1 } << x)) != 0;
                                const size_t at = (y * size + x) * stride;
                                REQUIRE((std::memcmp(shown->ImageData + at, source->ImageData + at, stride) != 0) ==
                                            (tinted && !(x == size - 1 && y == size - 1) && inRegion) &&
                                        "top/bottom/brick trim or protected masonry changed incorrectly");
                                REQUIRE(format == 0 ? (shown->ImageData[at + 1] & 1) == (source->ImageData[at + 1] & 1)
                                                    : shown->ImageData[at + 3] == source->ImageData[at + 3]);
                            }
                        REQUIRE(*source->mImageBuffer == bytes && shown->GetInitData() == metadata &&
                                metadata->IsCustom && metadata->Path == "custom/future/actual-archive-meta-target");
                        REQUIRE(shown->Flags == source->Flags && shown->Type == source->Type && shown->Width == size &&
                                shown->Height == size && shown->HByteScale == 8 && shown->VPixelScale == 4);
                        REQUIRE(context.rm->cache.at("custom/unmapped/green-material") == unmapped);
                    }
                    // Every other valid scene must restore this exact source.
                    // Revisit uses a copy made from the original, never another season.
                    season = 2;
                    Update(&play);
                    const auto autumn = context.rm->cache.at(path);
                    for (int16_t scene = 0; scene <= 0x6F; ++scene) {
                        if (scene == item.scene)
                            continue;
                        play.sceneNum = scene;
                        Update(&play);
                        REQUIRE(context.rm->cache.at(path) == source && "scene ownership leaked");
                    }
                    play.sceneNum = item.scene;
                    Update(&play);
                    REQUIRE(context.rm->cache.at(path) == autumn && "revisit lost original-source copy reuse");
                    Update(nullptr);
                    REQUIRE(context.rm->cache.at(path) == source);
                    Retire();
                    context.rm->cache.clear();
                }
            }
        }
    }
}

int main() {
    Ship::Context context;
    context.rm = std::make_shared<Ship::ResourceManager>();
    Ship::Context::current = &context;
    CheckActualArchives(context);
    CheckCliffBandRounding();
    CheckSceneTerrain(context);
    CheckPavingSeams(context);
    // The private forest is shared scenery: its eight greenery identities must
    // use the confirmed copper ground/scarlet foliage while preserving their exposure.
    const char* forestLeaves[] = { "lw_enkei_01_0", "lw_kusa_01_12600", "lw_wood_02_2CA00", "lw_wood_03_31E00",
                                   "lw_wood_04_37200" };
    const char* forestGround[] = { "lw_kabe_01_5400", "lw_yuka_02_39C00", "lw_yuka_03_44400" };
    for (bool alt : { false, true }) {
        context.rm->alt = alt;
        for (const char* prefix : { "", "alt/" }) {
            for (bool ground : { false, true }) {
                const auto check = [&](const char* name) {
                    const std::string path = std::string(prefix) + "custom/skiejer_forest/textures/" + name;
                    const auto original = Make(path);
                    const auto bytes = *original->mImageBuffer;
                    context.rm->cache[path] = original;
                    for (int selected : { 2, 3, 0, 1, 2, -1 }) {
                        season = selected;
                        Update(nullptr);
                        const bool tinted = (!*prefix || alt) && (selected == 2 || selected == 3 || selected == 0);
                        const auto shown = std::dynamic_pointer_cast<Fast::Texture>(context.rm->cache.at(path));
                        REQUIRE((shown != original) == tinted);
                        if (tinted) {
                            const uint32_t color = selected == 2   ? (ground ? 0xF3974FFFu : 0xEF484CFFu)
                                                   : selected == 3 ? (ground ? 0xDCECF5FFu : 0xE2EEE9FFu)
                                                   : ground        ? 0xF6C1D9FFu
                                                                   : 0xF6C1D9E0u;
                            const auto expected = BuildTexture(original, { color, selected == 2 ? 256u : 461u });
                            REQUIRE(expected && *shown->mImageBuffer == *expected->mImageBuffer);
                            REQUIRE(shown->ImageData[3] == original->ImageData[3]);
                        }
                        REQUIRE(*original->mImageBuffer == bytes);
                    }
                    Retire();
                    context.rm->cache.clear();
                };
                if (ground)
                    for (const char* name : forestGround)
                        check(name);
                else
                    for (const char* name : forestLeaves)
                        check(name);
            }
            for (const char* name : { "lw_kinoko_01_FC00", "lw_nenrin_01_27600", "lw_tutu_01_2A000", "portal_bark" }) {
                const std::string path = std::string(prefix) + "custom/skiejer_forest/textures/" + name;
                const auto original = Make(path);
                context.rm->cache[path] = original;
                for (int selected : { 0, 2, 3 }) {
                    season = selected;
                    Update(nullptr);
                    REQUIRE(context.rm->cache.at(path) == original);
                }
                Retire();
                context.rm->cache.clear();
            }
        }
    }
    context.rm->alt = true;
    // The same cached image can be reached by different authored atlas paths.
    // Recolor only each path's foliage region, even when stone/bark is greenish.
    const auto atlas = Make("fixture/shared-atlas");
    atlas->Width = atlas->Height = 16;
    atlas->ImageDataSize = 16 * 16 * 4;
    atlas->mImageBuffer = std::make_shared<std::vector<char>>(atlas->ImageDataSize);
    for (size_t at = 0; at < atlas->ImageDataSize; at += 4) {
        (*atlas->mImageBuffer)[at] = 60;
        (*atlas->mImageBuffer)[at + 1] = 110;
        (*atlas->mImageBuffer)[at + 2] = 70;
        (*atlas->mImageBuffer)[at + 3] = 83;
    }
    atlas->ImageData = reinterpret_cast<uint8_t*>(atlas->mImageBuffer->data());
    const auto atlasOriginal = *atlas->mImageBuffer;
    struct AtlasCase {
        const char* path;
        unsigned left, top, right, bottom;
    };
    const AtlasCase atlasCases[] = {
        { "scenes/shared/spot00_scene/s00_yama_01_76B00", 0, 8, 16, 16 },
        { "custom/prelude/spot00_scene/recipe252_tex0", 0, 8, 8, 16 },
        { "scenes/shared/spot00_scene/s00_ki_02_22000", 0, 0, 16, 12 },
        { "scenes/shared/spot00_scene/s00_ki_03_2C800", 0, 0, 16, 15 },
    };
    for (const char* prefix : { "", "alt/" }) {
        for (const auto& item : atlasCases)
            context.rm->cache[std::string(prefix) + item.path] = atlas;
        season = 3;
        Update(nullptr);
        for (const auto& item : atlasCases) {
            const auto copy =
                std::dynamic_pointer_cast<Fast::Texture>(context.rm->cache.at(std::string(prefix) + item.path));
            REQUIRE(copy && copy != atlas && "authored foliage region was not tinted");
            for (unsigned y = 0; y < 16; ++y)
                for (unsigned x = 0; x < 16; ++x) {
                    const size_t at = (y * 16 + x) * 4;
                    const bool foliage = x >= item.left && x < item.right && y >= item.top && y < item.bottom;
                    REQUIRE((std::memcmp(atlas->ImageData + at, copy->ImageData + at, 3) != 0) == foliage);
                    REQUIRE(copy->ImageData[at + 3] == 83);
                }
        }
        REQUIRE(*atlas->mImageBuffer == atlasOriginal);
        season = 1;
        Update(nullptr);
        for (const auto& item : atlasCases)
            REQUIRE(context.rm->cache.at(std::string(prefix) + item.path) == atlas);
        Retire();
        context.rm->cache.clear();
    }
    // Authored field terrain and shuffled actor greenery must resolve through
    // both resource namespaces without affecting colored reward markers.
    for (const char* target : { "scenes/shared/spot00_scene/s00_yuka_01_78B00___Copie",
                                "custom/prelude/spot00_scene/s00_yuka_04_85D00",
                                "scenes/shared/spot00_scene/s00_kabe_01_11300",
                                "scenes/shared/spot00_scene/s00_kabe_05_1C700",
                                "custom/prelude/spot00_scene/recipe222_tex0",
                                "custom/prelude/spot00_scene/recipe232_tex0",
                                "custom/prelude/spot00_scene/recipe242_tex0",
                                "custom/prelude/spot00_scene/recipe282_tex0",
                                "custom/prelude/spot00_scene/recipe292_tex0",
                                "custom/prelude/spot00_scene/recipe302_tex0",
                                "custom/prelude/spot00_scene/recipe312_tex0",
                                "custom/prelude/spot00_scene/recipe322_tex0",
                                "scenes/shared/spot00_scene/s00_yuka_04_85D00",
                                "custom/prelude/spot00_scene/recipe223_tex0",
                                "custom/prelude/spot00_scene/recipe233_tex0",
                                "custom/prelude/spot00_scene/recipe243_tex0",
                                "custom/prelude/spot00_scene/recipe283_tex0",
                                "custom/prelude/spot00_scene/recipe293_tex0",
                                "custom/prelude/spot00_scene/recipe303_tex0",
                                "custom/prelude/spot00_scene/recipe313_tex0",
                                "custom/prelude/spot00_scene/recipe323_tex0",
                                "custom/prelude/spot00_scene/recipe229_tex0",
                                "custom/prelude/spot00_scene/recipe239_tex0",
                                "custom/prelude/spot00_scene/recipe249_tex0",
                                "custom/prelude/spot00_scene/recipe281_tex0",
                                "custom/prelude/spot00_scene/recipe289_tex0",
                                "custom/prelude/spot00_scene/recipe299_tex0",
                                "custom/prelude/spot00_scene/recipe309_tex0",
                                "custom/prelude/spot00_scene/recipe319_tex0",
                                "custom/prelude/spot00_scene/recipe329_tex0",
                                "custom/prelude/spot00_scene/recipe221_tex0",
                                "custom/prelude/spot00_scene/recipe225_tex0",
                                "custom/prelude/spot00_scene/recipe235_tex0",
                                "custom/prelude/spot00_scene/recipe245_tex0",
                                "custom/prelude/spot00_scene/recipe285_tex0",
                                "custom/prelude/spot00_scene/recipe295_tex0",
                                "custom/prelude/spot00_scene/recipe305_tex0",
                                "custom/prelude/spot00_scene/recipe315_tex0",
                                "custom/prelude/spot00_scene/recipe325_tex0",
                                "custom/prelude/spot00_scene/recipe226_tex0",
                                "custom/prelude/spot00_scene/recipe236_tex0",
                                "custom/prelude/spot00_scene/recipe246_tex0",
                                "custom/prelude/spot00_scene/recipe286_tex0",
                                "custom/prelude/spot00_scene/recipe296_tex0",
                                "custom/prelude/spot00_scene/recipe306_tex0",
                                "custom/prelude/spot00_scene/recipe316_tex0",
                                "custom/prelude/spot00_scene/recipe326_tex0",
                                "scenes/shared/spot00_scene/s00_hana_02_5500",
                                "scenes/shared/spot04_scene/s04_02_en_01_0",
                                "scenes/shared/spot04_scene/s04_ie_01r_D100",
                                "scenes/shared/spot04_scene/s04_ie_01x_17900",
                                "scenes/shared/spot04_scene/s04_ie_01x_17900___Copie",
                                "scenes/shared/spot04_scene/s04_tuta_02_41800",
                                "scenes/shared/spot04_scene/s04_kusa_05_44900",
                                "scenes/shared/spot04_scene/s04_yuka_01r_46800___Copie",
                                "scenes/shared/spot04_scene/spot04_room_0Tex_011F08_rgba16_png",
                                "scenes/shared/spot04_scene/spot04_room_0Tex_01B090_rgba16_png",
                                "scenes/shared/spot04_scene/spot04_room_1Tex_0056A8_rgba16_png",
                                "scenes/shared/spot04_scene/spot04_sceneTex_00FE18_rgba16_png",
                                "scenes/shared/spot04_scene/spot04_sceneTex_010618_rgba16_png",
                                "scenes/shared/spot10_scene/s04_kusa_05_2A00",
                                "scenes/shared/spot10_scene/sp10_ki_02_2CF00",
                                "scenes/shared/spot10_scene/sp10_ki_02c_4E100",
                                "scenes/shared/spot10_scene/sp10_mon_01_3CB00",
                                "scenes/shared/spot10_scene/sp10_mon_02_3F500",
                                "objects/gameplay_field_keep/BushTex.rgba16",
                                "objects/gameplay_field_keep/LeafTex.rgba16",
                                "objects/gameplay_keep/gGrassBunchTex.rgba16",
                                "objects/gameplay_keep/LeafTex.rgba16" }) {
        for (const char* prefix : { "", "alt/" }) {
            const std::string path = std::string(prefix) + target;
            const auto source = Make(path);
            const auto original = *source->mImageBuffer;
            const std::string marker = "objects/gameplay_keep/reward_marker";
            const auto reward = Make(marker);
            context.rm->cache[path] = source;
            context.rm->cache[marker] = reward;
            const std::string pavers = "alt/scenes/shared/spot00_scene/s00_road_01_41180";
            const auto stone = Make(pavers);
            stone->ImageData[0] = 120;
            stone->ImageData[1] = 130;
            stone->ImageData[2] = 118;
            context.rm->cache[pavers] = stone;
            for (int selected : { 0, 2, 3 }) {
                season = selected;
                Update(nullptr);
                REQUIRE(context.rm->cache[path] != source && "target greenery did not receive the selected tint");
                // Existing shuffled bush/grass copies share the non-canopy actor
                // role, including native and Alt namespaces; reward markers stay native.
                if (std::strncmp(target, "objects/", 8) == 0) {
                    const Tint expectedTint = selected == 0   ? Tint{ 0xF6C1D9FFu, 461 }
                                              : selected == 2 ? Tint{ 0xF26846FFu, 256 }
                                                              : Tint{ 0xDCECF5FFu, 461 };
                    const auto expected = BuildTexture(source, expectedTint);
                    const auto shown = std::dynamic_pointer_cast<Fast::Texture>(context.rm->cache.at(path));
                    REQUIRE(expected && shown && *shown->mImageBuffer == *expected->mImageBuffer &&
                            "bush/grass texture copies must match the Maple/Glacial non-canopy actor role");
                }
                REQUIRE(context.rm->cache[marker] == reward);
                REQUIRE(context.rm->cache[pavers] == stone && "greenish stone paving must stay native");
                REQUIRE(*source->mImageBuffer == original);
            }
            season = 1;
            Update(nullptr);
            REQUIRE(context.rm->cache[path] == source);
            Retire();
            context.rm->cache.clear();
        }
    }
    const std::string canopy = "alt/scenes/shared/spot04_scene/s04_ha_03_38800";
    const std::string grass = "alt/scenes/shared/spot04_scene/s04_yuka_01r_46800";
    const auto tree = Make(canopy), ground = Make(grass);
    context.rm->cache[canopy] = tree;
    context.rm->cache[grass] = ground;
    const auto originalTree = *tree->mImageBuffer, originalGround = *ground->mImageBuffer;
    season = 0;
    Update(nullptr);
    const auto pink = context.rm->cache.at(canopy);
    REQUIRE(pink != tree);
    REQUIRE(context.rm->cache.at(grass) != ground && "spring terrain must no longer match summer");
    const auto populated = context.rm->cache.size();
    season = 2;
    Update(nullptr);
    const auto amber = context.rm->cache.at(canopy);
    REQUIRE(amber != tree && amber != pink && context.rm->cache.at(grass) != ground);
    season = 3;
    Update(nullptr);
    REQUIRE(context.rm->cache.at(canopy) != amber);
    season = 2;
    Update(nullptr);
    REQUIRE(context.rm->cache.at(canopy) == amber && "re-entering autumn must reuse original-source copy");
    REQUIRE(context.rm->cache.size() == populated && "cold assets must not be loaded by tint");
    season = 1;
    Update(nullptr);
    REQUIRE(context.rm->cache.at(canopy) == tree && context.rm->cache.at(grass) == ground);
    season = 0;
    Update(nullptr);
    REQUIRE(context.rm->cache.at(canopy) == pink);
    context.rm->alt = false;
    Update(nullptr);
    REQUIRE(context.rm->cache.at(canopy) == tree && "Alt-off must restore its bindings");
    context.rm->alt = true;
    Update(nullptr);
    const auto replacement = Make(canopy);
    context.rm->cache[canopy] = replacement;
    Reset();
    REQUIRE(context.rm->cache.at(canopy) == replacement && "cleanup must respect external reload");
    Update(nullptr);
    REQUIRE(context.rm->cache.at(canopy) != replacement);
    season = -1;
    Update(nullptr);
    REQUIRE(context.rm->cache.at(canopy) == replacement);
    REQUIRE(*tree->mImageBuffer == originalTree && *ground->mImageBuffer == originalGround);
    // Scene travel unloads Alt allocations. Retirement must release the old
    // source/copy cohort after draw/texture-cache teardown, before the next scene.
    Retire();
    REQUIRE(sCopies.empty() && evicted > 0);
    for (int visit = 0; visit < 50; ++visit) {
        context.rm->cache.clear();
        context.rm->cache[canopy] = Make(canopy);
        season = 2;
        Update(nullptr);
        REQUIRE(sCopies.size() == 1);
        Retire();
        REQUIRE(sCopies.empty());
    }
    Ship::Context::current = nullptr;
    Update(nullptr);
    std::cout << "PASS seasonal foliage bindings: spring/ground, native/Alt, season transitions, cold assets, reload "
                 "guards, cleanup\n";
}
