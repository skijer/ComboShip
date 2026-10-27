#include "AutumnSceneFoliage.h"
#include "2s2h/Enhancements/Audio/MMWeather.h"
#include <fast/resource/type/DisplayList.h>
#include <fast/resource/type/Texture.h>
#include <ship/Context.h>
#include <ship/resource/File.h>
#include <ship/resource/ResourceManager.h>
#include <ship/resource/archive/Archive.h>
#include <ship/utils/StrHash64.h>
#include <spdlog/spdlog.h>
#ifdef COMBO_BUILD
#include <ship/resource/CrossRMRegistry.h>
#include "entrances/SoloForest.h"
#endif
#include <algorithm>
#include <map>
#include <limits>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <vector>
#include <nlohmann/json.hpp>

extern "C" {
#include "global.h"
}
#ifdef COMBO_BUILD
#include "WoodsMysteryNativeBridge.h"
#endif

namespace {
const char* SceneResourceName(int sceneId) {
#ifdef COMBO_BUILD
    if (sceneId == COMBO_SOLO_FOREST_MM_SCENE)
        return "skiejer_forest";
#endif
    switch (sceneId) {
#define DEFINE_SCENE(name, enumId, ...) \
    case enumId:                        \
        return #name;
#define DEFINE_SCENE_UNSET(...)
#include "tables/scene_table.h"
#undef DEFINE_SCENE
#undef DEFINE_SCENE_UNSET
        default:
            return nullptr;
    }
}

// Texture identity, rather than command offsets or an old scene export, owns
// the palette. Both generated scene namespaces and explicit Alt hashes occur
// in edited packs. No source texture, UV, vertex or alpha command is changed.
struct Material {
    const char* path;
    uint32_t color; // Legacy role identifier; surface selectors depend on these original values.
    bool springGrass = false;
    int scene = -1;
    const char* palette = nullptr;
    bool autumnCrimson = false;
};
constexpr Material kMaterials[] = {
#include "AutumnSceneMaterials.inc"
};
constexpr const char* kWoodfallTempleDL = "objects/object_mtoride/gWoodfallSceneryTempleDL";

uint32_t SeasonPalette(uint32_t autumnColor, int selectedSeason, int /*sceneId*/ = -1, bool springGrass = false,
                       bool autumnCrimson = false) {
    if (selectedSeason == SEASON_SPRING) {
        if (springGrass)
            return 0xF6C1D9FF; // Full existing blush on explicit grass; keep tree/ground palettes separate.
        switch (autumnColor) {
            case 0xB96848FF:
            case 0xF4B783FF:
                return 0xF6C1D9E0; // Blossom canopy; retain the source texture's shading.
            case 0xFF9F91FF:
                return 0xF6C1D9CC; // Keep the middle-tree layer's existing blend alpha.
            default:
                return 0; // Ground uses green-only image copies, never a whole-triangle spring tint.
        }
    }
    if (selectedSeason != SEASON_WINTER) {
        if (selectedSeason == SEASON_AUTUMN && autumnCrimson)
            return 0xDE496BFF;
        switch (autumnColor) {
            case 0xD99C45FF:
            case 0xFFD078FF:
                return 0xF3974FFF;
            case 0xB96848FF:
            case 0xF4B783FF:
                return 0xEF484CFF;
            case 0xFF9F91FF:
                return 0xF26846FF;
            default:
                return autumnColor;
        }
    }
    // Use Termina's accepted full frost palette wherever this foliage is drawn.
    // Shared materials must not lose their white lift across outdoor entrances.
    switch (autumnColor) {
        case 0xD99C45FF:
        case 0xFFD078FF:
            return 0xDCECF5FF;
        case 0xB96848FF:
        case 0xF4B783FF:
            return 0xE2EEE9FF;
        case 0xFF9F91FF:
            return 0xE2EEE9FF;
        default:
            return autumnColor;
    }
}

unsigned SeasonTintMode(int selectedSeason, int /*sceneId*/) {
    return selectedSeason == SEASON_WINTER || selectedSeason == SEASON_SPRING ? 3 : 2;
}

uint32_t ForestListColor(std::string path, int selectedSeason = SEASON_AUTUMN) {
    if (path.starts_with("alt/"))
        path.erase(0, 4);
    // This exact draw owns the distant forest wallpaper and upper canopy.
    // The middle trees use shared textures in DL_003928, selected by MaterialColor.
    // Scope both together even when a replacement exposes private or mixed
    // texture identities. Do not recolor adjacent terrain or architecture.
    for (const auto* suffix : { "", "_scene" }) {
        const std::string prefix = "scenes/nonmq/Z2_00KEIKOKU" + std::string(suffix) + "/Z2_00KEIKOKU_room_00";
        for (const auto* setup : { "", "Set_001200", "Set_001340", "Set_0014C0", "Set_0015D0", "Set_001750",
                                   "Set_0019F0", "Set_001B30", "Set_001C90", "Set_001E60" }) {
            if (path == prefix + setup + "DL_0241B0")
                return SeasonPalette(0xF4B783FF, selectedSeason, SCENE_00KEIKOKU);
        }
    }
    return 0;
}

const std::vector<std::string>& RoadRandoGrassPaths() {
    static const auto paths = [] {
        std::vector<std::string> result;
        // Bundled model materials explicitly split grass/leaves (0/1) from
        // reward flowers and brown stems (2/3). OPA and XLU roots share them.
        for (const auto* family :
             { "objects/gameplay_field_keep/gFieldBush", "objects/gameplay_keep/gRandoCuttableGrass" }) {
            for (const auto* kind :
                 { "Random", "Minor", "Major", "Mask", "Token", "SmallKey", "Fairy", "BossKey", "Heart", "Junk" }) {
                result.push_back(std::string(family) + kind + "DL_tri_0");
                if (std::string(kind) != "Junk")
                    result.push_back(std::string(family) + kind + "DL_tri_1");
            }
        }
        return result;
    }();
    return paths;
}

uint32_t RoadRandoGrassColor(std::string path, int selectedSeason, int sceneId) {
    // Eligibility is owned by MMWeather_FoliageSeasonForPlay. All three
    // seasonal palettes reach only these reviewed foliage children outdoors.
    if (selectedSeason != SEASON_SPRING && selectedSeason != SEASON_AUTUMN && selectedSeason != SEASON_WINTER)
        return 0;
    if (path.starts_with("alt/"))
        path.erase(0, 4);
    const auto& paths = RoadRandoGrassPaths();
    return std::find(paths.begin(), paths.end(), path) == paths.end() ? 0
           : selectedSeason == SEASON_AUTUMN                          ? 0xF26846FF
                                             : SeasonPalette(0xD99C45FF, selectedSeason, sceneId, true);
}

uint32_t WholeListColor(std::string path, int selectedSeason = SEASON_AUTUMN, int sceneId = -1) {
    if (const auto color = RoadRandoGrassColor(path, selectedSeason, sceneId))
        return color;
    if (path.starts_with("alt/"))
        path.erase(0, 4);
    // The accepted POC3 foliage roots call private child materials. Follow
    // these exact foliage-only roots without guessing their private texture names.
    for (const auto* suffix : { "", "_scene" }) {
        const std::string prefix = "scenes/nonmq/Z2_00KEIKOKU" + std::string(suffix) + "/Z2_00KEIKOKU_room_00DL_";
        for (const auto* top : { "0153A8", "015DF0", "016598" }) {
            if (path == prefix + top)
                return SeasonPalette(0xFFD078FF, selectedSeason, SCENE_00KEIKOKU, false, true);
        }
        for (const auto* side : { "0158E8", "016180", "016A38" }) {
            if (path == prefix + side)
                return SeasonPalette(0xF4B783FF, selectedSeason, SCENE_00KEIKOKU, false, true);
        }
    }
    return 0;
}

using MoonTreeMaterials = std::map<std::string, std::set<uint64_t>>;

bool NumberedPath(const std::string& path, const std::string& prefix) {
    return path.starts_with(prefix) && path.size() > prefix.size() &&
           std::all_of(path.begin() + prefix.size(), path.end(), [](char c) { return c >= '0' && c <= '9'; });
}

MoonTreeMaterials FindMoonTreeMaterials(const std::shared_ptr<Ship::ArchiveManager>& archives,
                                        std::set<std::string>& paths) {
    const std::string prefix = "custom/prelude/Z2_00KEIKOKU/recipe";
    std::map<std::shared_ptr<Ship::Archive>, std::set<std::string>> owners;
    for (const auto* assetPrefix : { "", "alt/" }) {
        const auto files = archives->ListFiles(assetPrefix + prefix + "*");
        if (files == nullptr)
            continue;
        for (const auto& path : *files) {
            if (path.ends_with(".meta"))
                continue;
            const auto root = path.starts_with("alt/") ? path.substr(4) : path;
            if (!NumberedPath(root, prefix))
                continue;
            // An alias can resolve into another pack. Its sidecar's owner does
            // not prove the loaded material's identity. Only direct Prelude
            // recipe draws without an alias are eligible for this extension.
            if (archives->HasFile(path + ".meta") && archives->GetArchiveFromFile(path + ".meta") != nullptr)
                continue;
            const auto owner = archives->GetArchiveFromFile(path);
            if (owner != nullptr)
                owners[owner].insert(path);
        }
    }
    MoonTreeMaterials result;
    const nlohmann::json::json_pointer editsPath("/edits/Z2_00KEIKOKU");
    const nlohmann::json::json_pointer shapesPath("/data/shapes");
    const nlohmann::json::json_pointer texturesPath("/stored/textures");
    const nlohmann::json::json_pointer texturePathsPath("/stored/texPaths");
    for (const auto& [owner, roots] : owners) {
        // Project paths are shared by all Prelude exports. Read the draw's
        // actual owner, not whichever pack shadows the global project entry.
        // Only metadata is read: missing bark and large image blobs are irrelevant.
        const auto file = owner->LoadFile("prelude/project/edits.json");
        if (file == nullptr || file->Buffer == nullptr || file->BufferOffset >= file->Buffer->size())
            continue;
        const auto project =
            nlohmann::json::parse(file->Buffer->begin() + file->BufferOffset, file->Buffer->end(), nullptr, false);
        if (!project.is_object() || project.value("codec", nlohmann::json{}) != 2 || !project.contains(editsPath) ||
            !project.at(editsPath).is_array())
            continue;
        for (const auto& edit : project.at(editsPath)) {
            if (!edit.contains(shapesPath) || !edit.at(shapesPath).is_array())
                continue;
            for (const auto& shape : edit.at(shapesPath)) {
                if (!shape.is_object() || !shape.contains("newDlPath") || !shape["newDlPath"].is_string() ||
                    !shape.contains(texturesPath) || !shape.at(texturesPath).is_array() ||
                    !shape.contains(texturePathsPath) || !shape.at(texturePathsPath).is_array())
                    continue;
                std::string root = shape["newDlPath"];
                if (root.starts_with("alt/"))
                    root.erase(0, 4);
                if (!NumberedPath(root, prefix))
                    continue;
                const auto& textures = shape.at(texturesPath);
                const auto& texturePaths = shape.at(texturePathsPath);
                if (textures.size() != texturePaths.size())
                    continue;
                std::set<uint64_t> foliage;
                for (size_t i = 0; i < textures.size(); ++i) {
                    if (!textures[i].is_object() || !texturePaths[i].is_string())
                        continue;
                    const auto label = textures[i].value("label", nlohmann::json{});
                    if (label != "sou_ha_01" && label != "sou_ha_02" && label != "sou_ha_03" && label != "sou_sida_01")
                        continue; // Explicit Moon leaves/ferns; bark, grass and buildings stay native.
                    std::string texture = texturePaths[i];
                    if (texture.starts_with("alt/"))
                        texture.erase(0, 4);
                    if (!NumberedPath(texture, root + "_tex"))
                        continue;
                    foliage.insert(CRC64(texture.c_str()));
                    foliage.insert(CRC64(("alt/" + texture).c_str()));
                }
                if (foliage.empty())
                    continue;
                for (const auto* assetPrefix : { "", "alt/" }) {
                    const auto path = assetPrefix + root;
                    if (roots.contains(path)) {
                        result[path].insert(foliage.begin(), foliage.end());
                        paths.insert(root);
                    }
                }
            }
        }
    }
    return result;
}

uint32_t MaterialColor(uint64_t hash, int selectedSeason = SEASON_AUTUMN, int sceneId = -1,
                       const std::set<uint64_t>* moonTree = nullptr) {
    if (sceneId == SCENE_00KEIKOKU && moonTree != nullptr && moonTree->contains(hash))
        return SeasonPalette(0xF4B783FF, selectedSeason, sceneId, false, true);
    static const auto colors = [] {
        std::map<uint64_t, const Material*> result;
        for (const auto& material : kMaterials) {
            for (const auto* suffix : { "", "_scene" }) {
                std::string path = material.path;
                if (path.starts_with("scenes/nonmq/")) {
                    path.insert(path.find('/', 13), suffix);
                } else if (*suffix != '\0') {
                    continue; // Shared misc texture paths have no generated scene namespace.
                }
                result[CRC64(path.c_str())] = &material;
                result[CRC64(("alt/" + path).c_str())] = &material;
            }
        }
        return result;
    }();
    const auto found = colors.find(hash);
    return found == colors.end() || (found->second->scene != -1 && found->second->scene != sceneId)
               ? 0
               : SeasonPalette(found->second->color, selectedSeason, sceneId, found->second->springGrass,
                               found->second->autumnCrimson);
}

size_t MaterialCommandLength(uint8_t opcode) {
    switch (opcode) {
        case G_SETTIMG_OTR_HASH:
        case G_VTX_OTR_HASH:
        case G_VTX_OTR_FILEPATH: // Native Woods canopy clearance keeps the two-word vertex command.
        case G_MARKER:
        case G_DL_OTR_HASH:
        case G_MTX_OTR:
            return 2;
        case G_VTX:
        case G_MTX:
        case G_POPMTX:
        case G_CULLDL:
        case G_TRI1:
        case G_TRI2:
        case G_TEXTURE:
        case G_GEOMETRYMODE:
        case G_ENDDL:
        case G_DL:
        case G_RDPSETOTHERMODE: // Native combined other-mode packet (one word).
        case G_SETOTHERMODE_L:
        case G_SETOTHERMODE_H:
        case G_RDPLOADSYNC:
        case G_RDPPIPESYNC:
        case G_RDPTILESYNC:
        case G_LOADTLUT:
        case G_SETTILESIZE:
        case G_LOADBLOCK:
        case G_LOADTILE:
        case G_SETTILE:
        case G_SETPRIMCOLOR:
        case G_SETENVCOLOR:
        case G_SETCOMBINE:
        case G_SETTIMG:
            return 1;
        default:
            // Unknown/custom extensions and existing grayscale treatments
            // retain their mod's original list. Never guess their word count.
            return 0;
    }
}

struct RendererPointerSlot {
    size_t index;
    uintptr_t observed;
    size_t copyIndex;
};

std::vector<RendererPointerSlot> RendererPointerSlots(const std::vector<Gfx>& source, bool seasonalCopy = false) {
    std::vector<RendererPointerSlot> slots;
    for (size_t i = 0; i < source.size();) {
        const auto opcode = static_cast<uint8_t>(source[i].words.w0 >> 24);
        const auto length =
            seasonalCopy && (opcode == G_SETGRAYSCALE || opcode == G_SETINTENSITY) ? 1 : MaterialCommandLength(opcode);
        if (length == 0 || i + length > source.size()) {
            // Whole-list wrappers may preserve unknown extensions. Leave their
            // words strictly compared rather than treating payloads as opcodes.
            return {};
        }
        if (opcode == G_SETTIMG_OTR_HASH || opcode == G_VTX_OTR_HASH) {
            slots.push_back({ i, source[i].words.w1, i });
        }
        i += length;
    }
    return slots;
}

bool IsRoadForestPair(uint64_t primary, uint64_t secondary) {
    static const uint64_t mask = CRC64("misc/scene_texture_08/scene_texture_08_Tex_005000");
    static const uint64_t altMask = CRC64("alt/misc/scene_texture_08/scene_texture_08_Tex_005000");
    static const uint64_t color = CRC64("misc/scene_texture_08/scene_texture_08_Tex_006000");
    static const uint64_t altColor = CRC64("alt/misc/scene_texture_08/scene_texture_08_Tex_006000");
    return (primary == mask || primary == altMask) && (secondary == color || secondary == altColor);
}

std::vector<Gfx> BuildVariant(const std::vector<Gfx>& source, int selectedSeason = SEASON_AUTUMN, int sceneId = -1,
                              bool* onlyRoadForest = nullptr, const std::set<uint64_t>* moonTree = nullptr,
                              std::set<uint64_t>* tintedImages = nullptr, bool coverageOnly = false) {
    std::vector<Gfx> result;
    std::set<uint64_t> covered;
    if (tintedImages != nullptr)
        tintedImages->clear();
    uint32_t imageColor = 0;
    uint64_t imageHash = 0;
    bool tileSlot[8]{};
    uint32_t loadedColors[2]{};
    uint64_t loadedImages[2]{};
    unsigned renderTile = G_TX_RENDERTILE;
    bool explicitLoads = false;
    bool changed = false;
    bool roadTriangles = false, otherTriangles = false;
    bool roadCombine = false;
    if (onlyRoadForest != nullptr)
        *onlyRoadForest = false;
    for (size_t i = 0; i < source.size();) {
        const auto opcode = static_cast<uint8_t>(source[i].words.w0 >> 24);
        const auto length = MaterialCommandLength(opcode);
        if (length == 0 || i + length > source.size()) {
            return {};
        }
        if (opcode == G_SETTIMG_OTR_HASH) {
            imageHash = (uint64_t(source[i + 1].words.w0) << 32) | source[i + 1].words.w1;
            imageColor = MaterialColor(imageHash, selectedSeason, sceneId, moonTree);
        } else if (opcode == G_SETTIMG) {
            imageColor = 0;
            imageHash = 0;
        } else if (opcode == G_SETTILE) {
            // Match Interpreter::GfxDpSetTile: address 0 owns texture slot 0;
            // every nonzero TMEM address owns texture slot 1.
            tileSlot[(source[i].words.w1 >> 24) & 7] = (source[i].words.w0 & 0x1FF) != 0;
        } else if (opcode == G_LOADBLOCK || opcode == G_LOADTILE) {
            // A second image may populate a detail tile without replacing the
            // grass in the primary render tile. Follow TMEM ownership, not the
            // last image command (which may also be a palette load).
            loadedColors[tileSlot[(source[i].words.w1 >> 24) & 7]] = imageColor;
            loadedImages[tileSlot[(source[i].words.w1 >> 24) & 7]] = imageHash;
            explicitLoads = true;
        } else if (opcode == G_TEXTURE) {
            renderTile = (source[i].words.w0 >> 8) & 7;
        } else if (opcode == G_SETCOMBINE) {
            roadCombine = source[i].words.w0 == 0xFC20AC04 && source[i].words.w1 == 0xFF0F93FF;
        } else if (opcode == G_DL || opcode == G_DL_OTR_HASH) {
            // A child list can change texture state. Require a subsequent
            // explicit selected texture load before recoloring more triangles.
            imageColor = 0;
            imageHash = 0;
            loadedColors[0] = loadedColors[1] = 0;
            loadedImages[0] = loadedImages[1] = 0;
            explicitLoads = false;
        }
        if (opcode == G_TRI1 || opcode == G_TRI2) {
            const bool roadPair = sceneId == SCENE_24KEMONOMITI && explicitLoads && roadCombine &&
                                  !tileSlot[renderTile] && IsRoadForestPair(loadedImages[0], loadedImages[1]);
            roadTriangles |= roadPair;
            otherTriangles |= !roadPair;
            // The primary IA mask also serves other forests. Only this exact
            // Road binding uses a selective color copy, preserving its trunks.
            const uint32_t color = roadPair ? 0 : explicitLoads ? loadedColors[tileSlot[renderTile]] : imageColor;
            size_t end = i + 1;
            while (end < source.size()) {
                const auto next = source[end].words.w0 >> 24;
                if (next != G_TRI1 && next != G_TRI2) {
                    break;
                }
                ++end;
            }
            if (color != 0) {
                changed = true;
                if (tintedImages != nullptr)
                    covered.insert(explicitLoads ? loadedImages[tileSlot[renderTile]] : imageHash);
            }
            if (color != 0 && !coverageOnly) {
                result.push_back(gsSPGrayscale(SeasonTintMode(selectedSeason, sceneId)));
                result.push_back(
                    gsDPSetGrayscaleColor(color >> 24, (color >> 16) & 255, (color >> 8) & 255, color & 255));
            }
            if (!coverageOnly)
                result.insert(result.end(), source.begin() + i, source.begin() + end);
            if (color != 0 && !coverageOnly) {
                result.push_back(gsSPGrayscale(false));
            }
            i = end;
            continue;
        }
        if (!coverageOnly)
            result.insert(result.end(), source.begin() + i, source.begin() + i + length);
        if (opcode == G_ENDDL) {
            if (onlyRoadForest != nullptr && i + 1 == source.size())
                *onlyRoadForest = roadTriangles && !otherTriangles;
            if (changed && i + 1 == source.size()) {
                if (tintedImages != nullptr)
                    *tintedImages = std::move(covered);
                return result;
            }
            return {};
        }
        i += length;
    }
    return {};
}

bool SameCommand(const Gfx& first, const Gfx& second) {
    return first.words.w0 == second.words.w0 && first.words.w1 == second.words.w1;
}

struct DrawCoverage {
    std::set<uint64_t> images;
    bool unclassifiedWrapper = false;
};

struct Variant {
    // As with the actor material caches, retain commands and their source for
    // queued raw display-list pointers, including after a season/Alt transition.
    std::shared_ptr<Fast::DisplayList> source;
    std::vector<Gfx> original;
    std::vector<Gfx> commands;
    // The interpreter writes resolved texture pointers into executed lists.
    // A scoped child must not mutate the immutable source comparison above.
    std::vector<Gfx> draw;
    std::vector<RendererPointerSlot> pointerSlots = RendererPointerSlots(original);
    // Observations belong to the source, not its season's immutable draw copy.
    std::shared_ptr<std::map<size_t, uintptr_t>> observedPointers = std::make_shared<std::map<size_t, uintptr_t>>();
    std::string forestPath;
    std::set<uint64_t> coveredImages;
    bool unclassifiedWrapper = false;
    bool reportedPointerWrites = false;
    bool reportedConflict = false;

    Gfx Branch() const {
        return gsSPBranchList(commands.data());
    }
    void Restore() {
        if (!commands.empty() && !source->Instructions.empty() && SameCommand(source->Instructions[0], Branch())) {
            source->Instructions[0] = original[0];
            if (!pointerSlots.empty() && pointerSlots[0].index == 0) {
                // Our branch covered this cache field. Restoring its original
                // offset means the next renderer resolution is a fresh write.
                pointerSlots[0].observed = original[0].words.w1;
                (*observedPointers)[0] = original[0].words.w1;
            }
        }
    }
    bool MatchesSource(unsigned& pointerWrites, bool canopyLayoutChange = false) {
        if (commands.empty() || original.empty() || source->Instructions.size() != original.size()) {
            return false;
        }
        // Off/scene-ineligible draws execute the original list. Its hash
        // handlers cache image and vertex addresses in w1; these are not mod
        // edits. Compare every opcode and hash payload, and tolerate only the
        // renderer's pointer fields. Vertex offsets/pointer replacements and
        // all other material/geometry patches still invalidate the variant.
        size_t nextSlot = 0;
        for (auto& slot : pointerSlots) {
            const auto observed = observedPointers->find(slot.index);
            if (observed != observedPointers->end()) {
                slot.observed = observed->second;
            }
        }
        for (size_t i = 0; i < original.size(); ++i) {
            auto* pointerSlot =
                nextSlot < pointerSlots.size() && pointerSlots[nextSlot].index == i ? &pointerSlots[nextSlot] : nullptr;
            if (pointerSlot != nullptr) {
                ++nextSlot;
            }
            // Only SelectNativeCanopyLayout calls this after checking both
            // exact owned signatures. Every other word keeps the usual guard.
            if (canopyLayoutChange && (i == 28 || i == 29 || i == 49 || i == 50)) {
                continue;
            }
            if (i == 0 && SameCommand(source->Instructions[i], Branch())) {
                continue;
            }
            if (!SameCommand(source->Instructions[i], original[i])) {
                const auto& current = source->Instructions[i];
                const auto opcode = static_cast<uint8_t>(original[i].words.w0 >> 24);
                if (pointerSlot != nullptr && current.words.w0 == original[i].words.w0 && current.words.w1 > 0xFFFFF &&
                    (opcode == G_SETTIMG_OTR_HASH || pointerSlot->observed <= 0xFFFFF ||
                     current.words.w1 == pointerSlot->observed)) {
                    // Vertex hash resolution happens once. Unlike image hash
                    // reloads, a later change to that resolved address is an
                    // outside edit and must not be hidden by our copied draw.
                    ++pointerWrites;
                    continue;
                }
                if (!forestPath.empty() && !reportedConflict) {
                    SPDLOG_WARN("[AutumnForest] refusing changed draw path={} word={} old=({:X},{:X}) new=({:X},{:X})",
                                forestPath, i, original[i].words.w0, original[i].words.w1, current.words.w0,
                                current.words.w1);
                    reportedConflict = true;
                }
                Restore();
                return false;
            }
        }
        return true;
    }
    bool Apply() {
        unsigned pointerWrites = 0;
        if (!MatchesSource(pointerWrites)) {
            return false;
        }
        const bool redirected = SameCommand(source->Instructions[0], Branch());
        auto& copied = draw.empty() ? commands : draw;
        for (auto& slot : pointerSlots) {
            if (slot.index == 0 && redirected) {
                continue;
            }
            const auto current = source->Instructions[slot.index].words.w1;
            if ((original[slot.index].words.w0 >> 24) == G_VTX_OTR_HASH && (!redirected || current != slot.observed)) {
                // The source may resolve a newer resource while Off, or still
                // hold an offset if no Off frame rendered. Keep that exact
                // address/offset in the copy instead of replaying an old cache.
                copied[slot.copyIndex].words.w1 = current;
            }
            slot.observed = current;
            (*observedPointers)[slot.index] = current;
        }
        if (!forestPath.empty() && pointerWrites != 0 && !reportedPointerWrites) {
            SPDLOG_INFO("[AutumnForest] accepted {} renderer pointer writes; tint remains active path={}",
                        pointerWrites, forestPath);
            reportedPointerWrites = true;
        }
        source->Instructions[0] = Branch();
        return true;
    }
};

std::map<const Fast::DisplayList*, Variant> sVariants;
std::map<const Fast::DisplayList*, Variant> sWinterVariants;
std::map<const Fast::DisplayList*, Variant> sSpringVariants;
// Only the native Woods curtain has two layouts owned by our bridge helper.
// Keep the inactive layout's buffers alive for display lists already queued.
std::map<std::pair<const Fast::DisplayList*, int>, Variant> sCanopyLayouts;
std::map<const Fast::DisplayList*, std::weak_ptr<Fast::DisplayList>> sIgnored;
std::set<const Fast::DisplayList*> sActive;
std::vector<std::string> sPaths;
MoonTreeMaterials sMoonTreeMaterials;
const Ship::ArchiveManager* sArchives = nullptr;
int sScene = -1;
int sFoliageSeason = -1;

auto& CurrentVariants() {
    if (sFoliageSeason == SEASON_SPRING)
        return sSpringVariants;
    return sFoliageSeason == SEASON_WINTER ? sWinterVariants : sVariants;
}

bool SelectNativeCanopyLayout(const std::string& path, const std::shared_ptr<Fast::DisplayList>& resource,
                              int selectedSeason, std::map<const Fast::DisplayList*, Variant>& variants) {
#ifdef COMBO_BUILD
    if (path != WoodsMysteryNativeBridge::kNativeCanopy + 7 || resource->Instructions.size() <= 50)
        return false;
    const auto layout = [](const std::vector<Gfx>& words) {
        if (words.size() <= 50)
            return 0;
        if (WoodsMysteryNativeBridge::CanopyPatched(words.data()))
            return 2;
        const auto hash = [](const Gfx& word) { return word.words.w0 == 0x6D7AC1D5u && word.words.w1 == 0xC35928A3u; };
        return words[28].words.w0 == 0x3200C018u && (words[28].words.w1 == 0 || words[28].words.w1 > 0xFFFFFu) &&
                       words[49].words.w0 == 0x32012024u &&
                       (words[49].words.w1 == 0xC0 || words[49].words.w1 > 0xFFFFFu) && hash(words[29]) &&
                       hash(words[50])
                   ? 1
                   : 0;
    };
    const int desired = layout(resource->Instructions);
    auto current = variants.find(resource.get());
    Variant* prior = current == variants.end() ? nullptr : &current->second;
    if (prior == nullptr) {
        for (auto* cache : { &sVariants, &sWinterVariants, &sSpringVariants }) {
            const auto found = cache->find(resource.get());
            if (found != cache->end()) {
                prior = &found->second;
                break;
            }
        }
    }
    if (prior == nullptr || desired == 0 || layout(prior->original) == 0 || desired == layout(prior->original))
        return false;
    if (desired == 1) {
        for (const size_t index : { 28, 49 }) {
            const auto observed = prior->observedPointers->find(index);
            const auto pointer = resource->Instructions[index].words.w1;
            if (observed != prior->observedPointers->end() && observed->second > 0xFFFFFu && pointer > 0xFFFFFu &&
                pointer != observed->second) {
                prior->Restore();
                return false;
            }
        }
    }
    unsigned pointerWrites = 0;
    if (!prior->MatchesSource(pointerWrites, true))
        return false;
    prior->Restore();
    if (current != variants.end()) {
        const auto key = std::make_pair(resource.get(), selectedSeason);
        const auto saved = sCanopyLayouts.find(key);
        if (saved != sCanopyLayouts.end()) {
            if (layout(saved->second.original) != desired)
                return false;
            std::swap(current->second, saved->second);
        } else {
            sCanopyLayouts.emplace(key, std::move(current->second));
            variants.erase(current);
            sActive.erase(resource.get());
        }
    }
    return true;
#else
    return false;
#endif
}

struct SurfaceTextureVariant {
    std::shared_ptr<Fast::Texture> source;
    std::shared_ptr<Fast::Texture> copy;
    std::shared_ptr<Fast::Texture> palette; // Retain an indexed image's exact palette through queued draws.
};
struct SurfaceTextureBinding {
    std::weak_ptr<Ship::ResourceManager> owner;
    std::string path;
    SurfaceTextureVariant texture;
};
struct SurfaceMaterial {
    const char* path;
    uint32_t color;
    int scene = -1;
    bool altOnly = false;
    bool roadPairOnly = false;
    bool blossomLift = false;
    bool springGrass = false;
    const char* palette = nullptr;
    bool drawFallback = false;
    bool autumnCrimson = false;
};
constexpr SurfaceMaterial kSurfaceMaterials[] = {
#ifdef COMBO_BUILD
    // Native exploration copies keep the donor palettes in both asset modes.
    // These eight images contain the leaf/ground islands; plain bark and
    // mushrooms retain their original pixels and alpha.
    { "custom/skiejer_forest/textures/lw_enkei_01_0", 0xB96848FF, COMBO_SOLO_FOREST_MM_SCENE },
    { "custom/skiejer_forest/textures/lw_kabe_01_5400", 0xD99C45FF, COMBO_SOLO_FOREST_MM_SCENE },
    { "custom/skiejer_forest/textures/lw_kusa_01_12600", 0xB96848FF, COMBO_SOLO_FOREST_MM_SCENE, false, false, true },
    { "custom/skiejer_forest/textures/lw_wood_02_2CA00", 0xB96848FF, COMBO_SOLO_FOREST_MM_SCENE },
    { "custom/skiejer_forest/textures/lw_wood_03_31E00", 0xB96848FF, COMBO_SOLO_FOREST_MM_SCENE },
    { "custom/skiejer_forest/textures/lw_wood_04_37200", 0xB96848FF, COMBO_SOLO_FOREST_MM_SCENE },
    { "custom/skiejer_forest/textures/lw_yuka_02_39C00", 0xD99C45FF, COMBO_SOLO_FOREST_MM_SCENE },
    { "custom/skiejer_forest/textures/lw_yuka_03_44400", 0xD99C45FF, COMBO_SOLO_FOREST_MM_SCENE },
#endif
    // MM3D opening woods POC1: XML draws use private FILEPATH images, not
    // native texture hashes. Recolor green texels in their exact cached images;
    // keep the mixed bark/stone pixels, UVs, alpha and pink return light intact.
    { "custom/cor_mm3d/lost_woods_poc1/textures/lw_enkei_01_0", 0xB96848FF, SCENE_LOST_WOODS, true },
    { "custom/cor_mm3d/lost_woods_poc1/textures/lw_kabe_01_5400", 0xD99C45FF, SCENE_LOST_WOODS, true },
    { "custom/cor_mm3d/lost_woods_poc1/textures/lw_kusa_01_12600", 0xB96848FF, SCENE_LOST_WOODS, true, false, true },
    { "custom/cor_mm3d/lost_woods_poc1/textures/lw_wood_02_2CA00", 0xB96848FF, SCENE_LOST_WOODS, true },
    { "custom/cor_mm3d/lost_woods_poc1/textures/lw_wood_03_31E00", 0xB96848FF, SCENE_LOST_WOODS, true },
    { "custom/cor_mm3d/lost_woods_poc1/textures/lw_wood_04_37200", 0xB96848FF, SCENE_LOST_WOODS, true },
    { "custom/cor_mm3d/lost_woods_poc1/textures/lw_yuka_02_39C00", 0xD99C45FF, SCENE_LOST_WOODS, true },
    { "custom/cor_mm3d/lost_woods_poc1/textures/lw_yuka_03_44400", 0xD99C45FF, SCENE_LOST_WOODS, true },
    // Woods makeover XML resolves these images by FILEPATH on every draw.
    // Use immutable green-only cache copies instead of recoloring the mixed
    // wall triangles; stone, bark, leaf gaps and the black portal stay exact.
    // Canonical copies from Prelude exports and Alt images share this scope.
    { "custom/cor_woods_mystery/poc1/textures/grass", 0xD99C45FF, SCENE_26SARUNOMORI, true },
    { "custom/cor_woods_mystery/poc1/textures/rock", 0xD99C45FF, SCENE_26SARUNOMORI, true },
    { "custom/cor_woods_mystery/poc1/textures/vines", 0xD99C45FF, SCENE_26SARUNOMORI, true },
    { "custom/cor_woods_mystery/poc1/textures/ruins", 0xD99C45FF, SCENE_26SARUNOMORI, true },
    { "custom/cor_woods_mystery/poc1/textures/henriko_canopy", 0xB96848FF, SCENE_26SARUNOMORI, true },
    { "custom/cor_woods_mystery/poc1/textures/henriko_forest_edge", 0xB96848FF, SCENE_26SARUNOMORI, true },
    { "scenes/nonmq/Z2_00KEIKOKU/mm3d_field_poc4/textures/tl_ki_02", 0xFFD078FF, SCENE_00KEIKOKU, true },
    { "scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_027050", 0xFFD078FF },
    // Keep the wall's secondary gradient intact: tint only green primary
    // texels so brown/stone samples retain both original combine inputs.
    { "objects/object_mtoride/gWoodfallSceneryPurifiedWalls2Tex", 0xB96848FF, SCENE_21MITURINMAE },
    // Native I4 and supplied HD grayscale detail preserve the gradient's
    // original green/non-green boundary without a whole-triangle treatment.
    { "scenes/nonmq/Z2_21MITURINMAE/Z2_21MITURINMAETex_005190", 0xB96848FF, SCENE_21MITURINMAE },
    { "misc/scene_texture_08/scene_texture_08_Tex_006000", 0xB96848FF, SCENE_24KEMONOMITI, false, true },
    // The climbing vines are a scene image, not the nearby tree actors. Their
    // CI8 palette is shared with shop/bark materials and must never be recolored.
    { "scenes/nonmq/Z2_24KEMONOMITI/Z2_24KEMONOMITITex_005730", 0xD99C45FF, SCENE_24KEMONOMITI, false, false, true,
      true, "scenes/nonmq/Z2_24KEMONOMITI/Z2_24KEMONOMITITLUT_007950" },
    // The CI4 image includes a brown stem. Recolor only green palette entries;
    // RGBA replacements use the same green-only copy, without a triangle tint.
    { "objects/object_mtoride/gWoodfallSceneryBranchAndLeavesTLUT", 0xB96848FF, SCENE_21MITURINMAE, false, false,
      true },
    { "objects/object_mtoride/gWoodfallSceneryBranchAndLeavesTex", 0xB96848FF, SCENE_21MITURINMAE, false, false, true },
};

const std::vector<SurfaceMaterial>& SurfaceMaterials(int /*selectedSeason*/) {
    static const std::vector<SurfaceMaterial> original(std::begin(kSurfaceMaterials), std::end(kSurfaceMaterials));
    static const auto broader = [] {
        auto result = original;
        const auto grass = [](uint32_t color) { return color == 0xD99C45FF || color == 0xFFD078FF; };
        // Private ground/grass atlases already have selective image ownership.
        for (auto& material : result) {
            if (grass(material.color)) {
                material.springGrass = true;
                material.blossomLift = true;
            }
        }
        // Query the same reviewed ground identities in all three seasons.
        // Autumn/Winter retain an existing accepted draw treatment wherever
        // available; these copies add coverage only when that route is absent.
        for (const auto& material : kMaterials) {
            if (!grass(material.color) || material.springGrass ||
                std::any_of(result.begin(), result.end(),
                            [&](const auto& existing) { return std::string(existing.path) == material.path; }))
                continue;
            result.push_back({ material.path, material.color, material.scene, false, false, true, true,
                               material.palette, true, material.autumnCrimson });
        }
        return result;
    }();
    return broader;
}

// Retain immutable originals and copies until renderer teardown, as with the
// display-list variants. A new pixel address also gives the GPU a new cache key.
std::map<std::tuple<const Fast::Texture*, uint32_t, bool, const Fast::Texture*>, SurfaceTextureVariant>
    sSurfaceTextures;
std::vector<SurfaceTextureBinding> sSurfaceBindings;

std::shared_ptr<Fast::Texture> ExpandIndexedSurface(const std::shared_ptr<Fast::Texture>& source,
                                                    const std::shared_ptr<Fast::Texture>& palette) {
    const size_t pixels = size_t(source->Width) * source->Height;
    const bool ci4 = source->Type == Fast::TextureType::Palette4bpp;
    const size_t paletteEntries = ci4 ? 16 : 256;
    if ((!ci4 && source->Type != Fast::TextureType::Palette8bpp) || source->Flags != 0 ||
        source->ImageData == nullptr || pixels == 0 || (ci4 ? (pixels + 1) / 2 : pixels) != source->ImageDataSize ||
        pixels > std::numeric_limits<uint32_t>::max() / 4 || palette == nullptr ||
        palette->Type != Fast::TextureType::RGBA16bpp || palette->Flags != 0 || palette->ImageData == nullptr ||
        palette->ImageDataSize != paletteEntries * 2 || size_t(palette->Width) * palette->Height != paletteEntries) {
        return nullptr;
    }
    auto copy = std::make_shared<Fast::Texture>(source->GetInitData());
    copy->Type = Fast::TextureType::RGBA32bpp;
    copy->Width = source->Width;
    copy->Height = source->Height;
    copy->Flags = TEX_FLAG_LOAD_AS_RAW;
    // The intact CI4/CI8 commands still address half/one byte per texel.
    // Raw copies carry four channels; preserve their authored UV dimensions.
    copy->HByteScale = source->HByteScale * (ci4 ? 8 : 4);
    copy->VPixelScale = source->VPixelScale;
    copy->ImageDataSize = pixels * 4;
    copy->mImageBuffer = std::make_shared<std::vector<char>>(copy->ImageDataSize);
    copy->ImageData = reinterpret_cast<uint8_t*>(copy->mImageBuffer->data());
    for (size_t i = 0; i < pixels; ++i) {
        const unsigned byte = source->ImageData[ci4 ? i / 2 : i];
        const size_t index = size_t(ci4 ? (i % 2 == 0 ? byte >> 4 : byte & 15) : byte) * 2;
        const uint16_t color = (unsigned(palette->ImageData[index]) << 8) | palette->ImageData[index + 1];
        copy->ImageData[i * 4] = (color >> 11) * 255 / 31;
        copy->ImageData[i * 4 + 1] = ((color >> 6) & 31) * 255 / 31;
        copy->ImageData[i * 4 + 2] = ((color >> 1) & 31) * 255 / 31;
        copy->ImageData[i * 4 + 3] = (color & 1) * 255;
    }
    return copy;
}

std::shared_ptr<Fast::Texture> BuildSurfaceTexture(const std::shared_ptr<Fast::Texture>& source,
                                                   uint32_t color = 0xF3974FFF, float intensityScale = 1.0f) {
    const unsigned red = color >> 24, green = (color >> 16) & 255, blue = (color >> 8) & 255;
    const unsigned colorSum = red + green + blue;
    const unsigned blend = color & 255;
    const bool packed16 = source->Type == Fast::TextureType::RGBA16bpp && source->Flags == 0;
    const bool raw = (source->Flags & TEX_FLAG_LOAD_AS_RAW) != 0;
    const unsigned pixelBytes = packed16 ? 2 : 4;
    if ((!packed16 && !raw && source->Type != Fast::TextureType::RGBA32bpp) || source->ImageData == nullptr ||
        source->Width == 0 || source->Height == 0 ||
        size_t(source->Width) * source->Height * pixelBytes != source->ImageDataSize || colorSum == 0) {
        return nullptr;
    }
    auto copy = std::make_shared<Fast::Texture>(source->GetInitData());
    copy->Type = source->Type;
    copy->Width = source->Width;
    copy->Height = source->Height;
    copy->Flags = source->Flags;
    copy->HByteScale = source->HByteScale;
    copy->VPixelScale = source->VPixelScale;
    copy->ImageDataSize = source->ImageDataSize;
    const auto* first = reinterpret_cast<const char*>(source->ImageData);
    copy->mImageBuffer = std::make_shared<std::vector<char>>(first, first + source->ImageDataSize);
    copy->ImageData = reinterpret_cast<uint8_t*>(copy->mImageBuffer->data());
    bool changed = false;
    for (size_t i = 0; i < copy->ImageDataSize; i += pixelBytes) {
        auto* pixel = copy->ImageData + i;
        const uint16_t packed = packed16 ? (unsigned(pixel[0]) << 8) | pixel[1] : 0;
        unsigned r = packed16 ? packed >> 11 : pixel[0];
        unsigned g = packed16 ? (packed >> 6) & 31 : pixel[1];
        unsigned b = packed16 ? (packed >> 1) & 31 : pixel[2];
        // Mixed grass/rock and grass/bark surfaces retain their non-green
        // pixels and exact alpha, including the native RGBA5551 alpha bit.
        if (g > r && g > b) {
            // Match the brightness-preserving grass treatment used by the
            // display-list variants, with only byte rounding/highlight clipping.
            const unsigned sum = r + g + b;
            const auto channel = [&](unsigned value) {
                const unsigned normalized = intensityScale == 1.0f
                                                ? sum * value / colorSum
                                                : unsigned(float(sum * value) * intensityScale / colorSum);
                return std::min(packed16 ? 31u : 255u, normalized);
            };
            r = (r * (255 - blend) + channel(red) * blend) / 255;
            g = (g * (255 - blend) + channel(green) * blend) / 255;
            b = (b * (255 - blend) + channel(blue) * blend) / 255;
            if (packed16) {
                const uint16_t tinted = (r << 11) | (g << 6) | (b << 1) | (packed & 1);
                pixel[0] = tinted >> 8;
                pixel[1] = tinted & 255;
            } else {
                pixel[0] = r;
                pixel[1] = g;
                pixel[2] = b;
            }
            changed = true;
        }
    }
    return changed ? copy : nullptr;
}

void RestoreSurfaceBinding(const SurfaceTextureBinding& binding) {
#ifdef COMBO_BUILD
    Ship::OwnRMScope rmScope("mm");
#endif
    if (auto rm = binding.owner.lock(); rm && rm->GetCachedResource(binding.path, true) == binding.texture.copy) {
        rm->CacheExternalResource(binding.path, binding.texture.source);
    }
}

void RestoreSurfaceTextures() {
    for (const auto& binding : sSurfaceBindings) {
        RestoreSurfaceBinding(binding);
    }
    sSurfaceBindings.clear();
}

void UpdateSurfaceTextures(const PlayState* play, const std::shared_ptr<Ship::ResourceManager>& rm, int selectedSeason,
                           const DrawCoverage& drawCoverage) {
    const auto& drawImages = drawCoverage.images;
    std::vector<SurfaceTextureBinding> active;
    bool roadPair = false;
    if (play->sceneId == SCENE_24KEMONOMITI) {
        bool found = false, allPaired = true;
        for (const auto* suffix : { "", "_scene" }) {
            std::string path =
                "scenes/nonmq/Z2_24KEMONOMITI" + std::string(suffix) + "/Z2_24KEMONOMITI_room_00DL_004F78";
            if (rm->IsAltAssetsEnabled()) {
                const auto archives = rm->GetArchiveManager();
                const std::string altPath = "alt/" + path;
                if ((archives->HasFile(altPath) && archives->GetArchiveFromFile(altPath) != nullptr) ||
                    (archives->HasFile(altPath + ".meta") &&
                     archives->GetArchiveFromFile(altPath + ".meta") != nullptr))
                    path = altPath;
            }
            if (auto root = std::dynamic_pointer_cast<Fast::DisplayList>(rm->GetCachedResource(path, true))) {
                bool paired = false;
                BuildVariant(root->Instructions, selectedSeason, play->sceneId, &paired);
                found = true;
                allPaired &= paired;
            }
        }
        roadPair = found && allPaired;
    }
    for (const auto& material : SurfaceMaterials(selectedSeason)) {
        if ((material.scene != -1 && play->sceneId != material.scene) ||
            (material.altOnly && !rm->IsAltAssetsEnabled()) || (material.roadPairOnly && !roadPair))
            continue;
        if (material.drawFallback && selectedSeason != SEASON_SPRING) {
            if (drawCoverage.unclassifiedWrapper)
                continue; // Preserve accepted wrappers without guessing unknown command lengths.
            bool drawn = false;
            for (const auto* suffix : { "", "_scene" }) {
                std::string path = material.path;
                if (path.starts_with("scenes/nonmq/"))
                    path.insert(path.find('/', 13), suffix);
                else if (*suffix != '\0')
                    continue;
                drawn |=
                    drawImages.contains(CRC64(path.c_str())) || drawImages.contains(CRC64(("alt/" + path).c_str()));
            }
            if (drawn)
                continue; // Preserve the accepted draw formula, including non-green samples.
        }
        for (const auto* suffix : { "", "_scene" }) {
            for (const auto* prefix : { "", "alt/" }) {
                if (*prefix != '\0' && !rm->IsAltAssetsEnabled()) {
                    continue;
                }
                std::string path = material.path;
                if (path.starts_with("scenes/nonmq/"))
                    path.insert(path.find('/', 13), suffix);
                else if (*suffix != '\0')
                    continue; // Object textures have no generated scene namespace.
                path.insert(0, prefix);
                auto source = std::dynamic_pointer_cast<Fast::Texture>(rm->GetCachedResource(path, true));
                if (source == nullptr) {
                    continue; // Leave cold assets to the normal renderer load path.
                }
                auto current = std::find_if(sSurfaceBindings.begin(), sSurfaceBindings.end(), [&](const auto& binding) {
                    return binding.owner.lock() == rm && binding.path == path && binding.texture.copy == source;
                });
                if (current != sSurfaceBindings.end()) {
                    source = current->texture.source; // Always recolor the intact source.
                }
                std::shared_ptr<Fast::Texture> palette;
                if ((source->Type == Fast::TextureType::Palette8bpp ||
                     source->Type == Fast::TextureType::Palette4bpp) &&
                    source->Flags == 0 && material.palette != nullptr) {
                    std::string palettePath = material.palette;
                    palettePath.insert(palettePath.find('/', 13), suffix);
                    if (rm->IsAltAssetsEnabled()) {
                        const auto altPalette = "alt/" + palettePath;
                        const auto archives = rm->GetArchiveManager();
                        const auto hasArchive = [&](const std::string& name) {
                            return archives->HasFile(name) && archives->GetArchiveFromFile(name) != nullptr;
                        };
                        if (rm->GetCachedResource(altPalette, true) != nullptr || hasArchive(altPalette) ||
                            hasArchive(altPalette + ".meta"))
                            palettePath = altPalette;
                    }
                    palette = std::dynamic_pointer_cast<Fast::Texture>(rm->GetCachedResource(palettePath, true));
                    if (palette == nullptr)
                        continue; // Load the active palette normally; never borrow a different cold Alt palette.
                }
                const auto color = SeasonPalette(material.color, selectedSeason, play->sceneId, material.springGrass,
                                                 material.autumnCrimson);
                const bool lift =
                    selectedSeason == SEASON_WINTER || (selectedSeason == SEASON_SPRING && material.blossomLift);
                const auto key = std::tuple{ source.get(), color, lift, palette.get() };
                auto it = sSurfaceTextures.find(key);
                if (it == sSurfaceTextures.end()) {
                    auto decoded = palette != nullptr ? ExpandIndexedSurface(source, palette) : source;
                    auto copy = decoded != nullptr ? BuildSurfaceTexture(decoded, color, lift ? 1.8f : 1.0f) : nullptr;
                    if (copy == nullptr) {
                        continue;
                    }
                    it = sSurfaceTextures.emplace(key, SurfaceTextureVariant{ source, std::move(copy), palette }).first;
                }
                if (current == sSurfaceBindings.end() || current->texture.copy != it->second.copy) {
                    rm->CacheExternalResource(path, it->second.copy);
                }
                active.push_back({ rm, path, it->second });
            }
        }
    }
    for (const auto& binding : sSurfaceBindings) {
        if (std::none_of(active.begin(), active.end(), [&](const auto& next) {
                return next.owner.lock() == binding.owner.lock() && next.path == binding.path &&
                       next.texture.copy == binding.texture.copy;
            })) {
            RestoreSurfaceBinding(binding); // Respect outside cache edits and reloads.
        }
    }
    sSurfaceBindings = std::move(active);
}

struct LoadedDraw {
    std::string path, resolved;
    std::shared_ptr<Fast::DisplayList> resource;
};

// Materialize only the draws the existing update would load. Coverage and
// installation must observe the same resource on the first cold Alt frame.
std::vector<LoadedDraw> PrepareDraws(const std::shared_ptr<Ship::ResourceManager>& rm, int sceneId,
                                     int selectedSeason) {
    const auto archives = rm->GetArchiveManager();
    std::vector<LoadedDraw> draws;
    draws.reserve(sPaths.size());
    for (const auto& path : sPaths) {
        std::string resolved = path;
        if (rm->IsAltAssetsEnabled()) {
            const std::string altPath = "alt/" + path;
            // A failed renderer Alt lookup leaves a null ArchiveManager entry.
            // HasFile alone sees that key and would abandon the cached native
            // draw. Only a real archive can supply a replacement or .meta alias.
            const auto hasArchive = [&](const std::string& candidate) {
                return archives->HasFile(candidate) && archives->GetArchiveFromFile(candidate) != nullptr;
            };
            if (hasArchive(altPath) || hasArchive(altPath + ".meta")) {
                resolved = altPath;
            }
        }
        auto loaded = rm->GetCachedResource(resolved, true);
        if (loaded == nullptr && RoadRandoGrassColor(path, selectedSeason, sceneId) != 0) {
            // Prepare only the explicit foliage children before the first actor
            // draw; an outer grouped-grass tint must not reach a cold flower.
            loaded = rm->LoadResource(resolved, true);
        }
        if (loaded == nullptr && resolved != path && rm->GetCachedResource(path, true) != nullptr) {
            // Preload a cold Alt only when replacing an already-loaded scene
            // list. Leave unrelated/unloaded rooms to the normal render path.
            loaded = rm->LoadResource(resolved, true);
        }
        auto resource = std::dynamic_pointer_cast<Fast::DisplayList>(loaded);
        if (resource == nullptr || resource->Instructions.empty()) {
            continue;
        }
        draws.push_back({ path, resolved, std::move(resource) });
    }
    return draws;
}

// Keep existing draw tint authoritative. A shared image may appear in several
// room roots: choose its original draw treatment once, rather than recoloring
// cached pixels underneath an already accepted tinted draw. Reevaluate cold,
// reloaded, Alt and aliased resources every update without retaining new draws.
DrawCoverage CoveredDrawImages(const std::vector<LoadedDraw>& draws, int selectedSeason, int sceneId) {
    DrawCoverage result;
    if (selectedSeason == SEASON_SPRING)
        return result;
    for (const auto& draw : draws) {
        const auto& path = draw.path;
        const auto& resolved = draw.resolved;
        const auto& resource = draw.resource;
        const auto cached = CurrentVariants().find(resource.get());
        if (cached != CurrentVariants().end()) {
            result.images.insert(cached->second.coveredImages.begin(), cached->second.coveredImages.end());
            result.unclassifiedWrapper |= cached->second.unclassifiedWrapper;
            continue; // Immutable per-season variant already classified this draw.
        }
        const auto ignored = sIgnored.find(resource.get());
        if (ignored != sIgnored.end() && ignored->second.lock() == resource)
            continue;
        const std::vector<Gfx>* source = &resource->Instructions;
        for (const auto* cache : { &sVariants, &sWinterVariants, &sSpringVariants }) {
            const auto found = cache->find(resource.get());
            if (found != cache->end()) {
                source = &found->second.original;
                break;
            }
        }
        std::set<uint64_t> images;
        const auto moon = sMoonTreeMaterials.find(resolved);
        BuildVariant(*source, selectedSeason, sceneId, nullptr,
                     moon == sMoonTreeMaterials.end() ? nullptr : &moon->second, &images, true);
        if (images.empty() &&
            (ForestListColor(path, selectedSeason) || WholeListColor(path, selectedSeason, sceneId))) {
            // Whole-list wrappers already own their child materials. Preserve
            // every safely parsed image they enclose, including mixed roots.
            for (size_t i = 0; i < source->size();) {
                const auto op = uint8_t((*source)[i].words.w0 >> 24);
                const auto length = MaterialCommandLength(op);
                if (!length || i + length > source->size()) {
                    result.unclassifiedWrapper = true;
                    break;
                }
                if (op == G_SETTIMG_OTR_HASH)
                    images.insert((uint64_t((*source)[i + 1].words.w0) << 32) | (*source)[i + 1].words.w1);
                i += length;
            }
        }
        result.images.insert(images.begin(), images.end());
    }
    return result;
}

void RestoreActive() {
    for (const auto* source : sActive) {
        CurrentVariants().at(source).Restore();
    }
    sActive.clear();
}

void ClearCache() {
    // Registered teardown runs after rendering stops and before game DLLs are
    // unloaded, so queued copies can be released at this boundary.
    MMAutumnSceneFoliage_Reset();
    sVariants.clear();
    sWinterVariants.clear();
    sSpringVariants.clear();
    sCanopyLayouts.clear();
    sSurfaceTextures.clear();
}
} // namespace

void MMAutumnSceneFoliage_Update(const PlayState* play) {
    const char* scene = play ? SceneResourceName(play->sceneId) : nullptr;
    const int selectedSeason = MMWeather_FoliageSeasonForPlay(play);
    if (scene == nullptr ||
        (selectedSeason != SEASON_AUTUMN && selectedSeason != SEASON_WINTER && selectedSeason != SEASON_SPRING)) {
        MMAutumnSceneFoliage_Reset();
        return;
    }
#ifdef COMBO_BUILD
    static const bool teardownRegistered = [] {
        Ship::CrossRMRegistry::RegisterTeardownListener(ClearCache);
        return true;
    }();
    (void)teardownRegistered;
    Ship::OwnRMScope rmScope("mm");
#endif
    auto* context = Ship::Context::GetRawInstance();
    auto rm = context ? context->GetResourceManager() : nullptr;
    auto archives = rm ? rm->GetArchiveManager() : nullptr;
    if (archives == nullptr) {
        MMAutumnSceneFoliage_Reset();
        return;
    }
    if (sFoliageSeason != selectedSeason) {
        // Restore before selecting another immutable cache. All seasons retain
        // their queued command/pixel buffers, without tinting a tinted copy.
        RestoreActive();
        RestoreSurfaceTextures();
        sIgnored.clear(); // Seasons have different draw-scoped versus pixel-copy targets.
        sScene = -1;      // Rebuild Spring's broader replacement-grass path selection.
        sFoliageSeason = selectedSeason;
    }
    if (sScene != play->sceneId || sArchives != archives.get()) {
        RestoreActive();
        sScene = play->sceneId;
        sArchives = archives.get();
        std::set<std::string> paths;
        sMoonTreeMaterials.clear();
        if (play->sceneId == SCENE_00KEIKOKU)
            sMoonTreeMaterials = FindMoonTreeMaterials(archives, paths);
        for (const auto* suffix : { "", "_scene" }) {
            const std::string prefix = "scenes/nonmq/" + std::string(scene) + suffix + "/";
            for (const auto* assetPrefix : { "", "alt/" }) {
                auto files = archives->ListFiles(assetPrefix + prefix + "*DL*");
                if (files == nullptr) {
                    continue;
                }
                for (auto path : *files) {
                    if (path.find("room_") == std::string::npos || path.find("DL_") == std::string::npos) {
                        continue;
                    }
                    if (path.starts_with("alt/")) {
                        path.erase(0, 4);
                    }
                    if (path.ends_with(".meta")) {
                        path.resize(path.size() - 5);
                    }
                    paths.insert(path);
                }
            }
        }
        if (play->sceneId == SCENE_21MITURINMAE) {
            // Dm_Char01 owns the upper leaves in this loaded object root,
            // independently of the room display lists discovered above.
            for (const auto* assetPrefix : { "", "alt/" }) {
                const std::string path = assetPrefix + std::string(kWoodfallTempleDL);
                if (archives->HasFile(path) || archives->HasFile(path + ".meta"))
                    paths.insert(kWoodfallTempleDL);
            }
        }
        { // Reviewed foliage-only model children in every eligible outdoor scene.
            for (const auto& path : RoadRandoGrassPaths())
                for (const auto* assetPrefix : { "", "alt/" }) {
                    const auto resolved = assetPrefix + path;
                    if (archives->HasFile(resolved) || archives->HasFile(resolved + ".meta"))
                        paths.insert(path);
                }
        }
        sPaths.assign(paths.begin(), paths.end());
    }
    const auto draws = PrepareDraws(rm, play->sceneId, selectedSeason);
    UpdateSurfaceTextures(play, rm, selectedSeason, CoveredDrawImages(draws, selectedSeason, play->sceneId));
    std::set<const Fast::DisplayList*> active;
    auto& variants = CurrentVariants();
    for (const auto& draw : draws) {
        const auto& path = draw.path;
        const auto& resolved = draw.resolved;
        const auto& resource = draw.resource;
        const bool freshCanopyLayout = SelectNativeCanopyLayout(path, resource, selectedSeason, variants);
        auto it = variants.find(resource.get());
        if (it == variants.end()) {
            const auto ignored = sIgnored.find(resource.get());
            if (ignored != sIgnored.end() && ignored->second.lock() == resource) {
                continue;
            }
            // All seasons compare against the same intact source snapshot.
            // An Off draw may have resolved a first-command vertex offset since
            // the first season was cached; that season still restores its known
            // offset when removing the branch. Preserve that identity and the
            // observed pointer guards rather than snapshotting a different base.
            const Variant* other = nullptr;
            for (const auto* prior : { &sVariants, &sWinterVariants, &sSpringVariants }) {
                if (prior == &variants)
                    continue;
                const auto found = prior->find(resource.get());
                if (found != prior->end()) {
                    other = &found->second;
                    break;
                }
            }
            if (other == nullptr && freshCanopyLayout) {
                const auto saved = sCanopyLayouts.find(std::make_pair(resource.get(), selectedSeason));
                if (saved != sCanopyLayouts.end())
                    other = &saved->second;
            }
            const auto& original = other == nullptr || freshCanopyLayout ? resource->Instructions : other->original;
            const auto forestColor = ForestListColor(path, selectedSeason);
            const auto moonTree = sMoonTreeMaterials.find(resolved);
            std::set<uint64_t> imageCoverage;
            auto commands =
                forestColor != 0
                    ? std::vector<Gfx>{}
                    : BuildVariant(original, selectedSeason, play->sceneId, nullptr,
                                   moonTree == sMoonTreeMaterials.end() ? nullptr : &moonTree->second, &imageCoverage);
            const auto wholeColor =
                forestColor != 0 ? forestColor : WholeListColor(path, selectedSeason, play->sceneId);
            if (commands.empty() && wholeColor == 0) {
                // Ordinary scene materials stay owned by the resource manager;
                // don't retain or duplicate every non-foliage list in the scene.
                sIgnored[resource.get()] = resource;
                continue;
            }
            Variant variant{ resource, original, std::move(commands), {} };
            variant.coveredImages = std::move(imageCoverage);
            if (other != nullptr) {
                variant.observedPointers = other->observedPointers;
            }
            if (forestColor != 0) {
                variant.forestPath = resolved;
                const auto initData = resource->GetInitData();
                const std::string loadedPath = initData != nullptr ? initData->Path : "";
                const auto lookupPath = loadedPath.empty() ? resolved : loadedPath;
                const auto ownerPath = archives->HasFile(lookupPath) ? lookupPath : lookupPath + ".meta";
                const auto archive = archives->HasFile(ownerPath) ? archives->GetArchiveFromFile(ownerPath) : nullptr;
                SPDLOG_INFO("[AutumnForest] bound path={} resource_path={} path_owner_archive={} words={} source={}",
                            resolved, loadedPath.empty() ? "<unknown>" : loadedPath,
                            archive ? archive->GetPath() : "<unknown>", resource->Instructions.size(),
                            static_cast<const void*>(resource.get()));
            }
            if (variant.commands.empty()) {
                for (size_t i = 0; i < original.size();) {
                    const auto op = uint8_t(original[i].words.w0 >> 24);
                    const auto length = MaterialCommandLength(op);
                    if (!length || i + length > original.size()) {
                        variant.unclassifiedWrapper = true;
                        break;
                    }
                    if (op == G_SETTIMG_OTR_HASH)
                        variant.coveredImages.insert((uint64_t(original[i + 1].words.w0) << 32) |
                                                     original[i + 1].words.w1);
                    i += length;
                }
                // Execute the intact root as a pushed child, then restore the
                // color even when that root ends with a tail branch. Copies
                // retain all dynamic child calls, geometry and alpha commands.
                variant.draw = variant.original;
                const unsigned mode = RoadRandoGrassColor(path, selectedSeason, play->sceneId) != 0
                                          ? (selectedSeason == SEASON_AUTUMN ? 2 : 3)
                                          : SeasonTintMode(selectedSeason, play->sceneId);
                variant.commands = { gsSPGrayscale(mode),
                                     gsDPSetGrayscaleColor(wholeColor >> 24, (wholeColor >> 16) & 255,
                                                           (wholeColor >> 8) & 255, wholeColor & 255),
                                     gsSPDisplayList(variant.draw.data()), gsSPGrayscale(false), gsSPEndDisplayList() };
            }
            if (!variant.pointerSlots.empty()) {
                const auto& copied = variant.draw.empty() ? variant.commands : variant.draw;
                const auto copySlots = RendererPointerSlots(copied, true);
                if (copySlots.size() != variant.pointerSlots.size()) {
                    variant.pointerSlots.clear();
                } else {
                    for (size_t i = 0; i < copySlots.size(); ++i) {
                        variant.pointerSlots[i].copyIndex = copySlots[i].index;
                    }
                }
            }
            it = variants.emplace(resource.get(), std::move(variant)).first;
        }
        if (it->second.Apply()) {
            active.insert(resource.get());
        }
    }
    for (const auto* source : sActive) {
        if (!active.contains(source)) {
            variants.at(source).Restore();
        }
    }
    sActive = std::move(active);
}

void MMAutumnSceneFoliage_Reset() {
    RestoreActive();
    RestoreSurfaceTextures();
    sPaths.clear();
    sMoonTreeMaterials.clear();
    sIgnored.clear();
    sArchives = nullptr;
    sScene = -1;
    sFoliageSeason = -1;
}
