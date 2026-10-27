#pragma once

#include <memory>

namespace Ship {
class ResourceManager;

// Only the active-manager service is replaced; ResourceLoader remains production code.
class Context {
  public:
    static Context* GetRawInstance();
    std::shared_ptr<ResourceManager> GetResourceManager() const;
};
} // namespace Ship
