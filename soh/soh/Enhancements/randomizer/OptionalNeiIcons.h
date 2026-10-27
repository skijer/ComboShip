#pragma once

#include <cstring>
#include "soh/ResourceManagerHelpers.h"

namespace NeiOptionalIcons {
inline bool UsesNativeMagicFallback(const char* path) {
    return path && std::strcmp(path, "__OTR__textures/icon_item_static/gStatMagicTex") == 0 &&
           !ResourceMgr_FileExists(path) && !(ResourceMgr_IsAltAssetsEnabled() && ResourceMgr_FileAltExists(path));
}

inline const char* Resolve(const char* path) {
    return UsesNativeMagicFallback(path) ? "__OTR__textures/icon_item_24_static/gQuestIconMagicJarSmallTex" : path;
}
} // namespace NeiOptionalIcons
