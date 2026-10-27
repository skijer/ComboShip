#pragma once

// Test adapters for real, immutable serialized resources. This is a controller
// executable, not the application's ResourceManager, scene loader or renderer.
#include "BlackMarketEntry.h"
#include "BlackMarketNativeResources.h"
#include "entrances/NativeForest.h"
#include "../../mm/2s2h/Enhancements/Gameplay/BlackMarketResources.h"
#include <fstream>
#include <map>
#include <set>
#include <unordered_map>
#include <vector>

inline bool destination = true; // Retained only for the original fixture's Reset.
inline std::vector<nlohmann::json> diagnostics;
inline nlohmann::json inputResources;
inline std::map<std::string, std::string> activeFiles, baseFiles;
inline std::set<std::string> hiddenFiles;
inline bool altEnabled;
inline std::vector<TransitionActorEntry> realDoors;
inline std::vector<Vec3s> realFloorVertices;
inline std::vector<CollisionPoly> realFloorPolys;
inline int graphPatches, graphVertices, graphTextures;

inline std::string StripResource(const char* path) {
    return std::strncmp(path, "__OTR__", 7) == 0 ? path + 7 : path;
}
inline std::vector<uint8_t> ReadResourceBytes(const std::string& path, bool base = false) {
    const auto& files = base ? baseFiles : activeFiles;
    if (hiddenFiles.count(path) || !files.count(path))
        return {};
    std::ifstream stream(files.at(path), std::ios::binary);
    assert(stream && "resource inventory must name a real readable file");
    return {std::istreambuf_iterator<char>(stream), {}};
}
extern "C" inline u8 ResourceMgr_FileExists(const char* path) {
    const auto key = StripResource(path);
    return !hiddenFiles.count(key) && activeFiles.count(key);
}
extern "C" inline u8 MmAssets_BaseResourceExists(const char* path) {
    const auto key = StripResource(path);
    return !hiddenFiles.count(key) && baseFiles.count(key);
}
extern "C" inline void MmAssets_Init() {}
inline void CaptureEntryLog(const char*, const std::string& payload) {
    diagnostics.push_back(nlohmann::json::parse(payload));
}
#define SPDLOG_INFO(pattern, payload) CaptureEntryLog(pattern, payload)
bool DestinationAvailable();
void LogDoorAttempt(PlayState*, Actor*, int, const char*);
void LogEntryRecovery();

inline void ReadEntryInput(const char* path) {
    std::ifstream stream(path);
    stream >> inputResources;
    assert(inputResources.at("boundary") == "production-controller executable; no game loop");
}

inline void ConfigureResources(const char* scenario, const char* forest) {
    hiddenFiles.clear();
    activeFiles = inputResources.at("scenarios").at(scenario).at("files").get<decltype(activeFiles)>();
    baseFiles = inputResources.at("base_files").get<decltype(baseFiles)>();
    altEnabled = inputResources.at("scenarios").at(scenario).at("alt");
    const auto& data = inputResources.at("forests").at(forest);
    realDoors.clear();
    for (const auto& row : data.at("transitions")) {
        TransitionActorEntry entry{};
        entry.sides[0].room = row[0]; entry.sides[1].room = row[2];
#ifdef OOT_RESOURCE_ENTRY_TEST
        entry.sides[0].effects = row[1]; entry.sides[1].effects = row[3];
#else
        entry.sides[0].bgCamIndex = row[1]; entry.sides[1].bgCamIndex = row[3];
#endif
        entry.id = row[4]; entry.pos = {row[5], row[6], row[7]};
        entry.rotY = row[8]; entry.params = row[9];
        realDoors.push_back(entry);
    }
    realFloorVertices.clear(); realFloorPolys.clear();
    for (const auto& row : data.at("vertices"))
        realFloorVertices.push_back({row[0], row[1], row[2]});
    for (const auto& row : data.at("polygons")) {
        CollisionPoly poly{};
        poly.type = row[0]; poly.flags_vIA = row[1]; poly.flags_vIB = row[2]; poly.vIC = row[3];
        poly.normal = {row[4], row[5], row[6]}; poly.dist = row[7];
        realFloorPolys.push_back(poly);
    }
}

inline bool RealForestAvailable(const char* mode) {
    const char* scene = std::strcmp(mode, "oot") == 0 ? NativeForest::kOotSoloScene :
                        std::strcmp(mode, "mm") == 0 ? NativeForest::kMmSoloScene : NativeForest::kCombinedScene;
    return NativeForest::SceneAvailable(scene, ResourceMgr_FileExists, MmAssets_BaseResourceExists,
        [](const char* path) {
            const auto bytes = ReadResourceBytes(path);
            return bytes.size() > 68 ? NativeForestDependencies::Parse(bytes.data() + 68, bytes.size() - 68)
                                     : NativeForestDependencies::Graph{};
        });
}

// Production Math3D triangle predicates are inserted by the runner. Traversal
// here is deliberately a linear scan of the actual scene's static floor polys.
/* PRODUCTION_TRIANGLE_MATH */
inline float RealFloor(CollisionPoly** result, s32* bg, const Vec3f* probe) {
    float best = BGCHECK_Y_MIN;
    *result = nullptr; *bg = BGCHECK_SCENE;
    for (auto& poly : realFloorPolys) {
        if (poly.normal.y < 23170)
            continue;
        Vec3f points[3];
        const unsigned indices[] = {unsigned(poly.flags_vIA & 0x1FFF), unsigned(poly.flags_vIB & 0x1FFF), unsigned(poly.vIC)};
        for (int i = 0; i < 3; ++i) {
            assert(indices[i] < realFloorVertices.size());
            const auto& v = realFloorVertices[indices[i]];
            points[i] = {float(v.x), float(v.y), float(v.z)};
        }
        float y;
        if (Math3D_TriChkPointParaYIntersectInsideTri(&points[0], &points[1], &points[2],
                poly.normal.x / 32767.0f, poly.normal.y / 32767.0f, poly.normal.z / 32767.0f,
                poly.dist, probe->z, probe->x, &y, 1.0f) && y <= probe->y && y > best) {
            best = y; *result = &poly;
        }
    }
    return best;
}

#ifdef OOT_RESOURCE_ENTRY_TEST
#include "BlackMarketKeeperAssets.h"
#include "../../soh/mods/transformation_masks/assets/mm_display_list_patch.h"

// Minimal binary import adapters for stock OSKL/OSLB/OANM and F3DEX2. These
// consume donor bytes; only NativeModel::Load and the graph command patcher are
// production implementations. No application ResourceManager is substituted by
// implication in the evidence report.
struct ResourceReader {
    std::vector<uint8_t> bytes;
    size_t offset = 64;
    uint32_t Take(unsigned size) {
        assert(size <= 4 && offset + size <= bytes.size());
        uint32_t value = 0;
        for (unsigned i = 0; i < size; ++i) value |= uint32_t(bytes[offset++]) << (i * 8);
        return value;
    }
    std::string Text() {
        const auto count = Take(4);
        assert(offset + count <= bytes.size());
        std::string value(reinterpret_cast<const char*>(bytes.data() + offset), count);
        offset += count; return value;
    }
};
inline BlackMarketKeeperAssets::NativeModel sNativeKeeper;
inline std::map<std::string, StandardLimb> importedLimbs;
inline std::map<std::string, std::string> limbGraphPaths;
inline std::map<std::string, std::vector<MmDisplayListCommand>> importedGraphs;
inline std::map<std::string, std::vector<uint8_t>> importedPayloads;
inline std::map<std::string, std::string> importedTextureAliases;
inline std::set<std::string> graphInProgress;
inline std::unordered_map<uint64_t, std::string> resourceHashes;
inline FlexSkeletonHeader importedSkeleton;
inline AnimationHeader importedAnimation;
inline MmDisplayListCommand opaqueRenderMode[4] = {{0xDF000000, 0}, {0xDF000000, 0}, {0xDF000000, 0}, {0xDF000000, 0}};
inline void ClearImports() {
    importedLimbs.clear(); limbGraphPaths.clear(); importedGraphs.clear(); importedPayloads.clear();
    importedTextureAliases.clear(); graphInProgress.clear(); resourceHashes.clear();
    for (const auto& item : inputResources.at("base_hashes"))
        resourceHashes.emplace(item.at("hash").get<uint64_t>(), item.at("path").get<std::string>());
    graphPatches = graphVertices = graphTextures = 0;
}
extern "C" inline void* MmAssets_LoadSkeleton(const char* path) {
    ResourceReader r{ReadResourceBytes(StripResource(path), true)};
    if (r.bytes.empty()) return nullptr;
    assert(std::memcmp(r.bytes.data() + 4, "LKSO", 4) == 0);
    assert(r.Take(1) == 1 && r.Take(1) == 1);
    importedSkeleton = {};
    importedSkeleton.sh.limbCount = r.Take(4); importedSkeleton.dListCount = r.Take(4);
    assert(r.Take(1) == 1);
    const auto count = r.Take(4); assert(count == 23);
    for (unsigned i = 0; i < count; ++i) assert(r.Text() == BlackMarketKeeperAssets::kNativeLimbs[i]);
    assert(r.offset == r.bytes.size());
    return &importedSkeleton;
}
extern "C" inline void* MmAssets_LoadAnimation(const char* path) {
    ResourceReader r{ReadResourceBytes(StripResource(path), true)};
    if (r.bytes.empty()) return nullptr;
    assert(std::memcmp(r.bytes.data() + 4, "MNAO", 4) == 0 && r.Take(4) == 0);
    importedAnimation = {};
    importedAnimation.common.frameCount = r.Take(2);
    const auto rotations = r.Take(4); for (unsigned i = 0; i < rotations; ++i) r.Take(2);
    const auto joints = r.Take(4); for (unsigned i = 0; i < joints * 3; ++i) r.Take(2);
    const auto staticMax = r.Take(2);
    assert(rotations > staticMax && joints == 24 && r.offset == r.bytes.size());
    return &importedAnimation;
}
extern "C" inline void* MmAssets_LoadResourceStrict(const char* path) {
    const auto name = StripResource(path);
    auto bytes = ReadResourceBytes(name, true);
    if (bytes.empty()) return nullptr;
    if (name.ends_with("Limb")) {
        ResourceReader r{std::move(bytes)};
        assert(std::memcmp(r.bytes.data() + 4, "BLSO", 4) == 0);
        assert(r.Take(1) == 1); r.Take(1); assert(r.Text().empty());
        assert(r.Take(2) == 0 && r.Take(4) == 0 && r.Text().empty());
        r.Take(4); r.Take(4); r.Take(4); r.Take(2); r.Take(2); r.Take(2);
        assert(r.Text().empty() && r.Text().empty());
        const auto dl = r.Text(); assert(r.Text().empty());
        StandardLimb limb{};
        limb.jointPos = {s16(r.Take(2)), s16(r.Take(2)), s16(r.Take(2))};
        limb.child = r.Take(1); limb.sibling = r.Take(1);
        assert(r.offset == r.bytes.size());
        if (!dl.empty()) {
            limbGraphPaths[name] = "__OTR__" + dl;
            limb.dList = reinterpret_cast<Gfx*>(limbGraphPaths.at(name).data());
        }
        importedLimbs[name] = limb; return &importedLimbs.at(name);
    }
    assert(std::memcmp(bytes.data() + 4, "XETO", 4) == 0 && bytes.size() > 92);
    importedPayloads[name] = std::move(bytes); return importedPayloads.at(name).data() + 92;
}
inline MmDisplayListCommand* ImportGraph(const std::string& name);
inline uintptr_t ResolveGraphResource(void*, MmDisplayListReferenceKind kind, uint64_t hash, size_t* size) {
    *size = 0;
    if (kind == MM_DISPLAY_LIST_REFERENCE_RENDER_MODE)
        return (hash == 0 || hash == 2) ? reinterpret_cast<uintptr_t>(&opaqueRenderMode[hash]) : 0;
    if (!resourceHashes.count(hash)) return 0;
    const auto& path = resourceHashes.at(hash);
    if (kind == MM_DISPLAY_LIST_REFERENCE_NESTED)
        return reinterpret_cast<uintptr_t>(ImportGraph(path));
    auto bytes = ReadResourceBytes(path, true);
    if (bytes.empty()) return 0;
    if (kind == MM_DISPLAY_LIST_REFERENCE_TEXTURE) {
        if (bytes.size() < 92 || std::memcmp(bytes.data() + 4, "XETO", 4) != 0) return 0;
        importedPayloads[path] = std::move(bytes);
        importedTextureAliases[path] = "__OTR__" + path;
        return reinterpret_cast<uintptr_t>(importedTextureAliases.at(path).c_str());
    }
    if (bytes.size() < 72 || std::memcmp(bytes.data() + 4, "RRAO", 4) != 0) return 0;
    ResourceReader reader{bytes};
    if (reader.Take(4) != 25) return 0;
    const auto count = reader.Take(4);
    if (bytes.size() != 72 + count * 16) return 0;
    importedPayloads[path] = std::move(bytes); *size = count * 16;
    return reinterpret_cast<uintptr_t>(importedPayloads.at(path).data() + 72);
}
inline MmDisplayListCommand* ImportGraph(const std::string& name) {
    if (hiddenFiles.count(name)) return nullptr;
    if (importedGraphs.count(name)) return importedGraphs.at(name).data();
    if (graphInProgress.count(name) || graphInProgress.size() > 8) return nullptr;
    ResourceReader reader{ReadResourceBytes(name, true)};
    if (reader.bytes.size() < 80 || std::memcmp(reader.bytes.data() + 4, "TLDO", 4) != 0) return nullptr;
    assert(reader.Take(1) == 4); reader.offset = 72;
    std::vector<MmDisplayListCommand> commands;
    while (reader.offset < reader.bytes.size()) {
        const auto a = reader.Take(4), b = reader.Take(4); commands.push_back({a, b});
    }
    graphInProgress.insert(name);
    MmDisplayListPatchStats stats{};
    const bool valid = MmDisplayList_PatchCommands(commands.data(), commands.size(), ResolveGraphResource, nullptr, &stats);
    graphInProgress.erase(name);
    if (!valid || stats.unresolved) return nullptr;
    graphPatches += stats.nestedPatched; graphVertices += stats.verticesPatched; graphTextures += stats.texturesPatched;
    importedGraphs[name] = std::move(commands); return importedGraphs.at(name).data();
}
extern "C" inline Gfx* MmAssets_LoadDisplayListGraphStrict(const char* path) {
    return reinterpret_cast<Gfx*>(ImportGraph(StripResource(path)));
}
#endif
