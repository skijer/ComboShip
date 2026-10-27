#pragma once
#include <ship/resource/ResourceManager.h>
namespace Ship {
class Context {
  public:
    static Context* current;
    std::shared_ptr<ResourceManager> rm;
    static Context* GetRawInstance() {
        return current;
    }
    std::shared_ptr<ResourceManager> GetResourceManager() {
        return rm;
    }
};
} // namespace Ship
