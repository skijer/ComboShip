#pragma once

#include "NativeForestResources.h"
#include "NativeForestDependencies.h"
#include <cstring>
#include <string>

// These resources belong to the normal game asset build. Texture overrides
// neither enable the scene nor supply its geometry/collision framing.
namespace NativeForest {
inline constexpr const char* kMarker = "custom/comboship/native_forest/materials_v1";
inline constexpr const char* kCombinedScene = NativeForestResources::kCombinedScene;
inline constexpr const char* kMmSoloScene = NativeForestResources::kMmSoloScene;
inline constexpr const char* kOotSoloScene = NativeForestResources::kOotSoloScene;
inline constexpr const char* kPortalRoot = "__OTR__custom/comboship/native_forest/portal_root";
inline constexpr const char* kAuthoredPortalMarker = NativeForestResources::kAuthoredPortalMarker;

template <typename Entry> bool PortalEntries(const Entry* entries, unsigned count) {
    if (!entries)
        return false;
    for (unsigned i = 0; i < count; ++i)
        if (entries[i].opa && std::strcmp(reinterpret_cast<const char*>(entries[i].opa), kPortalRoot) == 0)
            return true;
    return false;
}

inline const char* DependencyPath(const char* scene) {
    if (!scene)
        return nullptr;
    if (std::strcmp(scene, kCombinedScene) == 0)
        return NativeForestResources::kCombinedDependencies;
    if (std::strcmp(scene, kMmSoloScene) == 0)
        return NativeForestResources::kMmSoloDependencies;
    return std::strcmp(scene, kOotSoloScene) == 0 ? NativeForestResources::kOotSoloDependencies : nullptr;
}

template <typename Exists> bool CoreAvailable(const char* scene, Exists exists) {
    const char* dependencies = DependencyPath(scene);
    if (!dependencies || !exists(dependencies))
        return false;
    if (!scene || !exists(scene) || !exists((std::string(scene) + "_collision").c_str()))
        return false;
    for (const char* core : NativeForestResources::kCommon)
        if (!exists(core))
            return false;
    // The native scene retains its three-room framing. A half-deployed base
    // must not become playable merely because a texture pack is present.
    for (int room = 0; room < 3; ++room)
        if (!exists((std::string(scene) + "_room_" + std::to_string(room)).c_str()))
            return false;
    return true;
}

template <typename Exists> bool StockAvailable(Exists exists) {
    if (!exists("scenes/nonmq/Z2_LOST_WOODS/Z2_LOST_WOODS_room_00"))
        return false;
    for (const char* stock : NativeForestResources::kStock)
        if (!exists(stock))
            return false;
    return true;
}

template <typename Exists, typename StockExists, typename Load>
bool SceneAvailable(const char* scene, Exists exists, StockExists stockExists, Load load) {
    if (!CoreAvailable(scene, exists) || !StockAvailable(stockExists))
        return false;
    const auto graph = load(DependencyPath(scene));
    if (graph.core.empty() || graph.stock.empty())
        return false;
    for (const auto& path : graph.core)
        if (!exists(path.c_str()))
            return false;
    for (const auto& path : graph.stock)
        if (!stockExists(path.c_str()))
            return false;
    return true;
}

template <typename Exists, typename Load> const char* CombinedScene(bool exploration, Exists exists, Load load) {
    return exploration && SceneAvailable(kCombinedScene, exists, exists, load) ? kCombinedScene : nullptr;
}
} // namespace NativeForest
