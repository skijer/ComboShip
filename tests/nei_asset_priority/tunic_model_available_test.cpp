// Execute the production owner query and scope; replace only registry/archive
// lookup, retaining real display-list and texture resource types.
#include "fast/resource/type/DisplayList.h"
#include "fast/resource/type/Texture.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <string>

namespace Ship {
class ResourceManager;
class Context {
  public:
    static Context* GetRawInstance();
    std::shared_ptr<ResourceManager> GetResourceManager() { return active; }
    void SetResourceManager(std::shared_ptr<ResourceManager> value) { active = value; }
    std::shared_ptr<ResourceManager> active;
};
static Context context;
Context* Context::GetRawInstance() { return &context; }
class ResourceManager {
  public:
    std::shared_ptr<IResource> LoadResource(std::string path) {
        assert(context.active.get() == this && "root factory and its nested dependencies must use the owner");
        if (path.starts_with("__OTR__")) path.erase(0, 7);
        if (alt && files.contains("alt/" + path)) path = "alt/" + path;
        return files.contains(path) ? files.at(path) : nullptr;
    }
    std::map<std::string, std::shared_ptr<IResource>> files;
    bool alt = false;
};
struct CrossRMRegistry {
    static std::shared_ptr<ResourceManager> Get(const std::string& game) {
        return owners.contains(game) ? owners.at(game) : nullptr;
    }
    static inline std::map<std::string, std::shared_ptr<ResourceManager>> owners;
};
} // namespace Ship

/* PRODUCTION_SCOPE */
/* PRODUCTION_QUERY */

int main() {
    auto oot = std::make_shared<Ship::ResourceManager>();
    auto mm = std::make_shared<Ship::ResourceManager>();
    Ship::CrossRMRegistry::owners = {{"oot", oot}, {"mm", mm}};
    Ship::context.active = mm;
    auto model = std::make_shared<Fast::DisplayList>();
    model->Instructions = {gsDPPipeSync(), gsSPEndDisplayList()};
    auto empty = std::make_shared<Fast::DisplayList>();
    auto texture = std::make_shared<Fast::Texture>();
    constexpr char collar[] = "objects/object_gi_clothes/gGiTunicCollarDL";
    constexpr char body[] = "objects/object_gi_clothes/gGiTunicDL";
    oot->files[collar] = model;
    oot->files[body] = model;
    mm->files[collar] = texture;
    mm->files[body] = empty;
    for (bool hostAlt : {false, true}) {
        mm->alt = hostAlt;
        for (bool donorAlt : {false, true}) {
            oot->alt = donorAlt;
            assert(ResourceMgr_IsGiModelAvailableForGame("oot", collar));
            assert(ResourceMgr_IsGiModelAvailableForGame("oot", body));
            assert(!ResourceMgr_IsGiModelAvailableForGame("mm", collar));
            assert(!ResourceMgr_IsGiModelAvailableForGame("mm", body));
            assert(!ResourceMgr_IsGiModelAvailableForGame("oot", "missing"));
            assert(Ship::context.active == mm && oot->alt == donorAlt && mm->alt == hostAlt);
        }
    }
    // Selected Alt type changes are live, without borrowing the host's model.
    oot->files[std::string("alt/") + body] = texture;
    oot->alt = true;
    assert(!ResourceMgr_IsGiModelAvailableForGame("oot", body));
    oot->files[std::string("alt/") + body] = empty;
    assert(!ResourceMgr_IsGiModelAvailableForGame("oot", body));
    oot->files[std::string("alt/") + body] = model;
    assert(ResourceMgr_IsGiModelAvailableForGame("oot", body));
    oot->alt = false;
    assert(ResourceMgr_IsGiModelAvailableForGame("oot", "__OTR__objects/object_gi_clothes/gGiTunicDL"));
    Ship::CrossRMRegistry::owners.erase("oot");
#ifdef COMBO_BUILD
    assert(!ResourceMgr_IsGiModelAvailableForGame("oot", body));
#else
    Ship::context.active = oot;
    assert(ResourceMgr_IsGiModelAvailableForGame("oot", body));
    Ship::context.active = mm;
#endif
    assert(!ResourceMgr_IsGiModelAvailableForGame(nullptr, body));
    assert(!ResourceMgr_IsGiModelAvailableForGame("oot", nullptr));
    assert(!ResourceMgr_IsGiModelAvailableForGame("unknown", body));
    assert(Ship::context.active == mm);
    std::cout << "PASS actual tunic owner query: missing/texture/empty roots, independent Alt selection, "
                 "native fallback and scope restoration\n";
}
