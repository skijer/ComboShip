#pragma once
#include "NeiWolfAsset.h"
#include <ship/Context.h>
#include <ship/resource/CrossRMRegistry.h>
#include <ship/resource/ResourceManager.h>
#include <ship/resource/ResourceManagerScope.h>
#include <ship/resource/type/Blob.h>

namespace NeiWolfAsset {
// Preserve the owner's archive and live Alt priorities. In ComboShip the other
// initialized game may supply the same model, but never Context's active owner.
inline int CopyResource(const char* nativeGame, uint8_t* destination, size_t capacity, size_t* size,
                        const char** selectedOwner, bool useHD = false, const char** selectedKey = nullptr) {
    if (!size)
        return -1;
    *size = 0;
    if (selectedOwner)
        *selectedOwner = nullptr;
    if (selectedKey)
        *selectedKey = nullptr;
    const char* owners[] = { nativeGame, std::strcmp(nativeGame, "mm") == 0 ? "oot" : "mm" };
    for (const char* game : owners) {
        auto manager = Ship::CrossRMRegistry::Get(game);
#ifndef COMBO_BUILD
        if (!manager && std::strcmp(game, nativeGame) == 0) {
            const auto context = Ship::Context::GetRawInstance();
            if (context)
                manager = context->GetResourceManager();
        }
#endif
        if (!manager || !manager->GetArchiveManager())
            continue;
        const auto archives = manager->GetArchiveManager();
        const auto has = [&](const std::string& key) {
            return archives->HasFile(key) || archives->HasFile(key + ".meta");
        };
        const bool alt = manager->IsAltAssetsEnabled();
        const char* selectedPath = alt && has(kAltResourcePath) ? kAltResourcePath : kResourcePath;
        // Keep owner priority first. Missing HD assets on an older installation
        // fall back to that owner's regular Wolf; present broken HD never does.
        const char* hdPath = alt && has(kAltHDResourcePath) ? kAltHDResourcePath : kHDResourcePath;
        if (useHD && has(hdPath))
            selectedPath = hdPath;
        if (!has(selectedPath))
            continue;
        if (selectedOwner)
            *selectedOwner = game;
        if (selectedKey)
            *selectedKey = selectedPath;
        Ship::ResourceManagerScope scope(manager);
        const auto resource = std::dynamic_pointer_cast<Ship::Blob>(manager->LoadResource(selectedPath, true));
        if (!resource)
            return -1;
        const size_t payload = ResourcePayloadSize(resource->Data.data(), resource->Data.size());
        if (!payload)
            return -1;
        *size = payload;
        if (destination) {
            if (capacity < payload)
                return -1;
            std::memcpy(destination, resource->Data.data(), payload);
        }
        return 1;
    }
    return 0;
}
} // namespace NeiWolfAsset
