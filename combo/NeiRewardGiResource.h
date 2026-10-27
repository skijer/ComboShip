#pragma once
#include "NeiRewardGiSurface.h"
#include <ship/Context.h>
#include <ship/resource/CrossRMRegistry.h>
#include <ship/resource/ResourceManager.h>
#include <ship/resource/ResourceManagerScope.h>

namespace NeiGi {
inline bool GetRewardSurface(const char* nativeGame, const char* game, const char* path, Mesh* out) {
    if (!out)
        return false;
    *out = {};
    if (!game || !path)
        return false;
    std::string resource(path), ownerGame(game);
    if (resource.starts_with("__OTR__"))
        resource.erase(0, 7);
    if (resource.starts_with('@')) {
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
    (void)nativeGame;
#endif
    if (!owner)
        return false;
    Ship::ResourceManagerScope scope(owner);
    auto load = [&](auto key) { return owner->LoadResource(key); };
    return ReadRewardSurface(load, resource.c_str(), *out);
}
} // namespace NeiGi
