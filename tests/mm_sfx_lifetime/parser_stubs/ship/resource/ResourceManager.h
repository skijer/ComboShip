#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace Ship {
struct File;
class IResource;
class ResourceLoader;

// Controlled global archive I/O and priorities, used by the real alias resolver.
class ArchiveManager {
  public:
    int32_t GetFilePriority(const std::string& path);
};

class ResourceManager {
  public:
    std::shared_ptr<File> LoadFileProcess(const std::string& path);
    std::shared_ptr<IResource> LoadResourceProcess(const std::string& path);
    std::shared_ptr<ArchiveManager> GetArchiveManager();
    std::shared_ptr<ResourceLoader> GetResourceLoader();
};
} // namespace Ship
