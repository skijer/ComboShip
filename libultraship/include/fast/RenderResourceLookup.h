#pragma once

#include <cstdint>
#include <cstring>

namespace Fast {
struct RenderResourceLookupStats {
    uint64_t hits = 0;
    uint64_t fallbacks = 0;
};

// Reuse ResourceManager's own Alt/owner/archive/dirty checks. This introduces no
// cache and retains the normal loader (including failures) on every cache miss.
// Templated so the production cache decisions can also run in the ROM-free fixture.
template <class Manager>
auto LoadRenderResource(Manager& manager, const char* path, bool enabled, RenderResourceLookupStats* stats) {
    if (enabled) {
        const char* normalized = path;
        while (std::strncmp(normalized, "__OTR__", 7) == 0) {
            normalized += 7;
        }
        if (auto resource = manager.GetCachedResource(normalized)) {
            if (stats != nullptr) {
                ++stats->hits;
            }
            return resource;
        }
        if (stats != nullptr) {
            ++stats->fallbacks;
        }
    }
    return manager.LoadResource(path);
}
} // namespace Fast
