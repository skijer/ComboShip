#pragma once
#include "NeiAssetPriority.h"
#include "NeiGiModelBounds.h"
#include "DinSwordGiResources.h"
#include <ship/Context.h>
#include <ship/resource/CrossRMRegistry.h>
#include <ship/resource/ResourceManager.h>
#include <ship/resource/ResourceManagerScope.h>
#include <ship/resource/archive/Archive.h>
#include <ship/resource/archive/ArchiveManager.h>

namespace NeiAssetPriority {
inline std::string FindCompanion(const char* game) {
    // Mirror the stock companion search at the archive mount, selecting only
    // the first existing candidate. Other similarly named archives stay mods.
    if (std::string(game) == "mm") {
#ifdef COMBO_BUILD
        constexpr const char* neiCompanion = "nei/2ship/oot.o2r";
#else
        constexpr const char* neiCompanion = "nei/oot.o2r";
#endif
        for (const auto& candidate : { "oot.o2r", neiCompanion, "mods/oot.o2r", "../oot.o2r" }) {
            auto path = Ship::Context::LocateFileAcrossAppDirs(candidate, "2ship");
            if (path.empty() || !std::filesystem::exists(path))
                path = candidate;
            if (std::filesystem::exists(path))
                return path;
        }
    } else {
        auto path = Ship::Context::LocateFileAcrossAppDirs("mm.o2r", "soh");
        if (!path.empty() && std::filesystem::exists(path))
            return path;
        for (const auto& candidate :
             { "mm.o2r", "x64/Debug/mm.o2r", "x64/Release/mm.o2r", "build/x64/mm.o2r", "../mm.o2r" })
            if (std::filesystem::exists(candidate))
                return candidate;
    }
    return {};
}
inline bool IsModAsset(const char* nativeGame, const char* game, const char* path) {
    if (!game || !path)
        return false;
    std::string resource = path;
    if (resource.compare(0, 7, "__OTR__") == 0)
        resource.erase(0, 7);
    std::string ownerGame = game;
    if (!resource.empty() && resource[0] == '@') {
        const auto colon = resource.find(':');
        if (colon == std::string::npos)
            return false;
        ownerGame = resource.substr(1, colon - 1);
        resource.erase(0, colon + 1);
    }
    if (ownerGame != "oot" && ownerGame != "mm")
        return false;
    auto owner = Ship::CrossRMRegistry::Get(ownerGame);
#ifndef COMBO_BUILD
    if (!owner && ownerGame == nativeGame) {
        const auto context = Ship::Context::GetRawInstance();
        if (context)
            owner = context->GetResourceManager();
    }
#else
    // A not-yet-initialized donor must never fall back to the active host.
    (void)nativeGame;
#endif
    if (!owner || !owner->GetArchiveManager())
        return false;
    // Exact resolved stock locations: a mod named soh.o2r is still a mod.
    // Resolve per game, independent of the active game's resource manager.
    static const std::vector<std::string> ootBuiltins{ Ship::Context::LocateFileAcrossAppDirs("soh.o2r", "soh"),
                                                       Ship::Context::LocateFileAcrossAppDirs("soh.o2r"),
                                                       Ship::Context::LocateFileAcrossAppDirs("oot.o2r", "soh"),
                                                       Ship::Context::LocateFileAcrossAppDirs("oot-mq.o2r", "soh"),
                                                       FindCompanion("oot") };
    static const std::vector<std::string> mmBuiltins{ Ship::Context::LocateFileAcrossAppDirs("2ship.o2r", "2ship"),
                                                      Ship::Context::LocateFileAcrossAppDirs("2ship.o2r"),
                                                      Ship::Context::LocateFileAcrossAppDirs("mm.o2r", "2ship"),
                                                      FindCompanion("mm") };
    const auto archives = owner->GetArchiveManager();
    return UsesModAsset(resource.c_str(), owner->IsAltAssetsEnabled(), ownerGame == "oot" ? ootBuiltins : mmBuiltins,
                        [&](const std::string& key) {
                            if (!archives->HasFile(key))
                                return std::string{};
                            const auto archive = archives->GetArchiveFromFile(key);
                            return archive ? archive->GetPath() : std::string{};
                        });
}
// XML model replacements remain custom after users consolidate them into a
// companion/base O2R. Archive provenance alone cannot identify that geometry.
inline bool IsCustomAsset(const char* nativeGame, const char* game, const char* path) {
    if (!game || !path)
        return false;
    std::string resource = path;
    if (resource.compare(0, 7, "__OTR__") == 0)
        resource.erase(0, 7);
    std::string ownerGame = game;
    if (!resource.empty() && resource[0] == '@') {
        const auto colon = resource.find(':');
        if (colon == std::string::npos)
            return false;
        ownerGame = resource.substr(1, colon - 1);
        resource.erase(0, colon + 1);
    }
    auto owner = Ship::CrossRMRegistry::Get(ownerGame);
#ifndef COMBO_BUILD
    if (!owner && ownerGame == nativeGame) {
        const auto context = Ship::Context::GetRawInstance();
        if (context)
            owner = context->GetResourceManager();
    }
#else
    (void)nativeGame;
#endif
    if (!owner)
        return false;
    Ship::ResourceManagerScope scope(owner);
    const auto model = owner->LoadResource(resource);
    return model && model->GetInitData() && model->GetInitData()->IsCustom;
}
inline bool GetGiModelFit(const char* nativeGame, const char* game, const char* path, float scale, float tilt,
                          int presentation, float fit[2], int dinProfile = 0) {
    if (!game || !path || !fit || !std::isfinite(scale) || scale <= 0.f || !std::isfinite(tilt))
        return false;
    std::string resource = path;
    if (resource.compare(0, 7, "__OTR__") == 0)
        resource.erase(0, 7);
    std::string ownerGame = game;
    if (!resource.empty() && resource[0] == '@') {
        const auto colon = resource.find(':');
        if (colon == std::string::npos)
            return false;
        ownerGame = resource.substr(1, colon - 1);
        resource.erase(0, colon + 1);
    }
    auto owner = Ship::CrossRMRegistry::Get(ownerGame);
#ifndef COMBO_BUILD
    if (!owner && ownerGame == nativeGame) {
        const auto context = Ship::Context::GetRawInstance();
        if (context)
            owner = context->GetResourceManager();
    }
#else
    (void)nativeGame;
#endif
    if (!owner)
        return false;
    Ship::ResourceManagerScope scope(owner);
    // Hash and filepath dependencies follow the same owner's live Alt selection.
    const auto load = [&](auto key) { return owner->LoadResource(key); };
    NeiGi::ShopFit correction{};
    if (!NeiGi::SelectedModelFit(load, resource.c_str(), scale, tilt, presentation, dinProfile, correction))
        return false;
    fit[0] = correction.scale;
    fit[1] = correction.lift;
    return true;
}
inline bool GetIkanaShieldGiPose(const char* nativeGame, const char* game, const char* path, float* tilt, float* roll) {
    if (!game || !path || !tilt || !roll || (std::strcmp(game, "oot") && std::strcmp(game, "mm")))
        return false;
    auto owner = Ship::CrossRMRegistry::Get(game);
#ifndef COMBO_BUILD
    if (!owner && std::strcmp(game, nativeGame) == 0)
        if (const auto context = Ship::Context::GetRawInstance())
            owner = context->GetResourceManager();
#else
    (void)nativeGame;
#endif
    if (!owner)
        return false;
    Ship::ResourceManagerScope scope(owner);
    const auto load = [&](auto key) { return owner->LoadResource(key); };
    NeiGi::ModelBoundsReader<decltype(load), true> reader(load, 0.f);
    NeiGi::FrameBounds bounds{};
    if (!reader.Read(path, bounds))
        return false;
    *tilt = reader.ShieldTiltX();
    *roll = reader.ShieldRollZ();
    return true;
}
inline bool GetIkanaShieldGiTiltX(const char* nativeGame, const char* game, const char* path, float* tilt) {
    float roll = 0.f;
    return GetIkanaShieldGiPose(nativeGame, game, path, tilt, &roll);
}

inline int GetDinSwordGiProfile(const char* nativeGame, const char* game, const char* path, bool enabled) {
    if (!game || !path || !enabled)
        return 0;
    std::string resource = path;
    if (resource.compare(0, 7, "__OTR__") == 0)
        resource.erase(0, 7);
    std::string ownerGame = game;
    if (!resource.empty() && resource[0] == '@') {
        const auto colon = resource.find(':');
        if (colon == std::string::npos)
            return 0;
        ownerGame = resource.substr(1, colon - 1);
        resource.erase(0, colon + 1);
    }
    auto owner = Ship::CrossRMRegistry::Get(ownerGame);
#ifndef COMBO_BUILD
    if (!owner && ownerGame == nativeGame) {
        const auto context = Ship::Context::GetRawInstance();
        if (context)
            owner = context->GetResourceManager();
    }
#else
    (void)nativeGame;
#endif
    if (!owner)
        return 0;
    Ship::ResourceManagerScope scope(owner);
    return DinSwordGi::SelectedProfile(resource.c_str(), enabled, owner->IsAltAssetsEnabled(),
                                       [&](const char* dependency) { return bool(owner->LoadResource(dependency)); });
}

// Each independent pass retains its actual selected owner and live Alt lookup.
// A secondary native sword pass may be a material-only/empty display list.
struct GiFitResourceLoader {
    std::shared_ptr<Ship::ResourceManager> owner;
    bool materialSegment8;
    std::shared_ptr<Ship::IResource> operator()(const char* path) {
        return owner->LoadResource(path);
    }
    std::shared_ptr<Ship::IResource> operator()(uint64_t hash) {
        return owner->LoadResource(hash);
    }
    bool IsMaterialDisplayList(uintptr_t address) const {
        return materialSegment8 && address == 0x08000001;
    }
};

// Observe selected geometry without changing its fit or retaining a frame cache.
inline bool GetGiModelBounds(const char* nativeGame, const char* game, const char* const* paths, int count,
                             float minimum[3], float maximum[3]) {
    if (!game || !paths || count < 1 || count > 16 || !minimum || !maximum)
        return false;
    NeiGi::Point low{}, high{};
    bool geometry = false;
    for (int i = 0; i < count; ++i) {
        if (!paths[i])
            return false;
        std::string resource = paths[i], ownerGame = game;
        if (resource.starts_with("__OTR__"))
            resource.erase(0, 7);
        if (resource.starts_with('@')) {
            const auto colon = resource.find(':');
            if (colon == std::string::npos)
                return false;
            ownerGame = resource.substr(1, colon - 1);
            resource.erase(0, colon + 1);
        }
        auto owner = Ship::CrossRMRegistry::Get(ownerGame);
#ifndef COMBO_BUILD
        if (!owner && ownerGame == nativeGame)
            if (const auto context = Ship::Context::GetRawInstance())
                owner = context->GetResourceManager();
#else
        (void)nativeGame;
#endif
        if (!owner)
            return false;
        Ship::ResourceManagerScope scope(owner);
        struct Loader : GiFitResourceLoader {
            bool spinMaterial = false;
            bool IsMaterialDisplayList(uintptr_t address) const {
                return GiFitResourceLoader::IsMaterialDisplayList(address) ||
                       (spinMaterial && (address & ~uintptr_t(1)) == 0x08000000u);
            }
        } load;
        load.owner = owner;
        load.materialSegment8 = resource == "objects/object_gi_longsword/gGiBiggoronSwordDL" ||
                                resource == "objects/object_toki_objects/object_toki_objects_DL_001BD0" ||
                                // Both legacy trident drawers bind this hook to an empty list.
                                resource == "objects/object_gnd/gPhantomGanonSkelLimbsLimb_00C610DL_009298" ||
                                // Native spell setup supplies only texture scrolling here.
                                resource == "objects/object_gi_goddess/gGiMagicSpellOrbDL";
        load.spinMaterial = resource == "objects/gameplay_keep/gGreatSpinAttackDiskDL" ||
                            resource == "objects/gameplay_keep/gGreatSpinAttackCylinderDL";
        NeiGi::ModelBoundsReader<Loader, true> reader(load, 0.f);
        NeiGi::FrameBounds fitted{};
        if (!reader.Read(resource.c_str(), fitted, true) || (count > 1 && !reader.RestoresModelView()))
            return false;
        NeiGi::Point partLow{}, partHigh{};
        if (!reader.ExactBounds(partLow, partHigh))
            continue; // Geometry-free native color/material pass.
        if (!geometry) {
            low = partLow;
            high = partHigh;
            geometry = true;
        } else {
            low = { std::min(low.x, partLow.x), std::min(low.y, partLow.y), std::min(low.z, partLow.z) };
            high = { std::max(high.x, partHigh.x), std::max(high.y, partHigh.y), std::max(high.z, partHigh.z) };
        }
    }
    if (!geometry)
        return false;
    minimum[0] = low.x;
    minimum[1] = low.y;
    minimum[2] = low.z;
    maximum[0] = high.x;
    maximum[1] = high.y;
    maximum[2] = high.z;
    return true;
}

inline bool GetGiModelsFit(const char* nativeGame, const char* game, const char* const* paths, int count, float scale,
                           float tilt, int presentation, float fit[2], bool dinEnabled) {
    if (!game || !paths || count < 1 || count > 16 || !fit || !std::isfinite(scale) || scale <= 0.f ||
        !std::isfinite(tilt))
        return false;
    NeiGi::FrameBounds bounds{};
    for (int i = 0; i < count; ++i) {
        if (!paths[i])
            return false;
        std::string resource = paths[i];
        if (resource.compare(0, 7, "__OTR__") == 0)
            resource.erase(0, 7);
        std::string ownerGame = game;
        if (!resource.empty() && resource[0] == '@') {
            const auto colon = resource.find(':');
            if (colon == std::string::npos)
                return false;
            ownerGame = resource.substr(1, colon - 1);
            resource.erase(0, colon + 1);
        }
        auto owner = Ship::CrossRMRegistry::Get(ownerGame);
#ifndef COMBO_BUILD
        if (!owner && ownerGame == nativeGame) {
            const auto context = Ship::Context::GetRawInstance();
            if (context)
                owner = context->GetResourceManager();
        }
#else
        (void)nativeGame;
#endif
        if (!owner)
            return false;
        const int din = i == 0 ? GetDinSwordGiProfile(nativeGame, game, paths[i], dinEnabled) : 0;
        Ship::ResourceManagerScope scope(owner);
        GiFitResourceLoader load{ owner, resource == "objects/object_gi_longsword/gGiBiggoronSwordDL" ||
                                             resource == "objects/object_toki_objects/object_toki_objects_DL_001BD0" };
        NeiGi::FrameBounds part{};
        if (!NeiGi::SelectedModelBounds(load, resource.c_str(), tilt, din, part, true, count > 1))
            return false;
        NeiGi::MergeModelBounds(bounds, part);
    }
    if (!bounds.slug)
        return false;
    const auto correction = NeiGi::FrameFit(bounds, scale, presentation == 1, presentation >= 2 ? presentation - 1 : 0);
    fit[0] = correction.scale;
    fit[1] = correction.lift;
    return true;
}
} // namespace NeiAssetPriority
