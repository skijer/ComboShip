#pragma once
#include <ship/resource/Resource.h>
#include <map>
#include <string>
namespace Ship {
class ResourceManager {
  public:
    bool alt = true;
    std::map<std::string, std::shared_ptr<IResource>> cache;
    bool IsAltAssetsEnabled() const {
        return alt;
    }
    std::shared_ptr<IResource> GetCachedResource(const std::string& path, bool) {
        const auto it = cache.find(path);
        return it == cache.end() ? nullptr : it->second;
    }
    void CacheExternalResource(const std::string& path, std::shared_ptr<IResource> resource) {
        cache[path] = resource;
    }
};
} // namespace Ship
