// Launcher-owned config merge, kept independent of the window and game DLLs.
#pragma once

#include <nlohmann/json.hpp>

namespace ComboSettingsImport {

// Shared settings retain the existing policy: objects recurse, SoH wins leaves.
inline void DeepMerge(nlohmann::json& base, const nlohmann::json& overlay) {
    if (!base.is_object() || !overlay.is_object()) {
        base = overlay;
        return;
    }
    for (auto it = overlay.begin(); it != overlay.end(); ++it) {
        auto found = base.find(it.key());
        if (found != base.end() && found->is_object() && it->is_object()) {
            DeepMerge(*found, *it);
        } else {
            base[it.key()] = it.value();
        }
    }
}

inline const nlohmann::json* FindModOrder(const nlohmann::json& config, const nlohmann::json::json_pointer& key) {
    if (config.contains(key) && config.at(key).is_string()) {
        return &config.at(key);
    }
    return nullptr;
}

// A missing/invalid file is null. Both standalone games store a pipe-separated
// order in EnabledMods; ComboShip's MM loader instead reads EnabledModsMM.
inline nlohmann::json Merge(const nlohmann::json& soh, const nlohmann::json& mm) {
    const nlohmann::json::json_pointer nativeOrderKey("/CVars/gSettings/EnabledMods");
    const nlohmann::json::json_pointer mmOrderKey("/CVars/gSettings/EnabledModsMM");
    const nlohmann::json* mmOrder = FindModOrder(mm, mmOrderKey);
    if (!mmOrder) {
        mmOrder = FindModOrder(mm, nativeOrderKey);
    }

    nlohmann::json merged = mm.is_object() ? mm : nlohmann::json::object();
    if (merged.contains(nativeOrderKey)) {
        // Never let a standalone MM list become OoT's list, even without a SoH
        // input or when SoH has never saved its own mod order.
        merged["CVars"]["gSettings"].erase("EnabledMods");
    }
    if (soh.is_object()) {
        DeepMerge(merged, soh);
    }
    if (mmOrder) {
        // MM owns its order even if the SoH input also has a separated MM copy.
        // Copy verbatim: sorting, reversing, or filtering would change priority.
        merged[mmOrderKey] = *mmOrder;
    }

    merged.erase("Window"); // machine-specific
    // Only a SoH version can drive SoH's config migrations.
    if (!soh.is_object() || !soh.contains("ConfigVersion")) {
        merged.erase("ConfigVersion");
    }
    return merged;
}

} // namespace ComboSettingsImport
