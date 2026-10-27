#include "combo/BlackMarketGranny.h"
#include <cassert>
#include <cstdio>

namespace Ship {
struct Archive {
  std::set<std::string> paths;
  bool HasFile(const std::string &path) const { return paths.contains(path); }
};
struct ArchiveManager {
  std::shared_ptr<std::vector<std::shared_ptr<Archive>>> archives =
      std::make_shared<std::vector<std::shared_ptr<Archive>>>();
  auto GetArchives() { return archives; }
  std::shared_ptr<Archive> GetArchiveFromFile(const std::string &path) {
    for (auto it = archives->rbegin(); it != archives->rend(); ++it)
      if ((*it)->HasFile(path))
        return *it;
    return nullptr;
  }
  bool HasFile(const std::string &path) {
    return bool(GetArchiveFromFile(path));
  }
};
struct ResourceManager {
  std::shared_ptr<ArchiveManager> archives = std::make_shared<ArchiveManager>();
  auto GetArchiveManager() { return archives; }
};
struct CrossRMRegistry {
  static inline std::map<std::string, std::shared_ptr<ResourceManager>> owners;
  static std::shared_ptr<ResourceManager> Get(const std::string &host) {
    auto found = owners.find(host);
    return found == owners.end() ? nullptr : found->second;
  }
};
struct Context {
  static inline Context *current = nullptr;
  std::shared_ptr<ResourceManager> active;
  static Context *GetRawInstance() { return current; }
  auto GetResourceManager() { return active; }
};
} // namespace Ship
namespace BlackMarketGranny {
/* PRODUCTION AVAILABILITY */
}
using namespace BlackMarketGranny;
static auto MakeArchive(std::initializer_list<const char *> paths) {
  auto result = std::make_shared<Ship::Archive>();
  for (auto path : paths)
    result->paths.insert(path);
  return result;
}
static auto Maps(bool complete) {
  auto result =
      MakeArchive({"alt/scenes/shared/mahouya_scene/mahouya_room_0DL_006548"});
  if (complete) {
    result->paths.insert(
        "alt/scenes/shared/mahouya_scene/mahouya_room_0DL_006548_vtx_2");
    result->paths.insert(
        "alt/scenes/shared/mahouya_scene/sh_maho_book_01__UDIM_");
  }
  return result;
}
static auto Native() {
  return MakeArchive({"scenes/shared/mahouya_scene/mahouya_scene",
                      "scenes/shared/mahouya_scene/mahouya_room_0",
                      "scenes/shared/mahouya_scene/mahouya_room_0DL_006A78"});
}
int main(int argc, char **argv) {
  assert(argc == 2);
  const std::string scenario = argv[1];
  auto oot = std::make_shared<Ship::ResourceManager>();
  auto mm = std::make_shared<Ship::ResourceManager>();
  auto native = Native();
  oot->archives->archives->push_back(native);
  oot->archives->archives->push_back(Maps(true));
  Ship::CrossRMRegistry::owners = {{"oot", oot}, {"mm", mm}};
  Ship::Context context;
  context.active = oot;
  Ship::Context::current = &context;
  assert(DependenciesAvailable());
  if (scenario == "native-shadow") {
    auto partial = MakeArchive({"scenes/shared/mahouya_scene/mahouya_scene"});
    oot->archives->archives->push_back(partial);
    assert(NativeOwner().second == native &&
           "scene-only override must not hide the complete native archive");
    assert(DependenciesAvailable());
    auto complete = Native();
    oot->archives->archives->push_back(complete);
    assert(NativeOwner().second == complete &&
           "complete higher-priority native donor keeps precedence");
  } else if (scenario == "partial-maps-owner") {
    mm->archives->archives->push_back(Maps(false));
    context.active = mm;
    assert(MapsOwner().second == oot && "partial active-host maps must not "
                                        "hide complete peer-host dependencies");
    assert(DependenciesAvailable());
    mm->archives->archives->push_back(Maps(true));
    assert(MapsOwner().second == mm &&
           "complete active-host maps keep precedence");
  } else {
    assert(scenario == "controls");
    Ship::CrossRMRegistry::owners.erase("oot");
    mm->archives->archives->push_back(Native());
    mm->archives->archives->push_back(Maps(true));
    context.active = mm;
    assert(DependenciesAvailable() && MapsOwner().second == mm &&
           NativeOwner().first == mm);
    for (const char *missing :
         {"alt/scenes/shared/mahouya_scene/mahouya_room_0DL_006548",
          "alt/scenes/shared/mahouya_scene/mahouya_room_0DL_006548_vtx_2",
          "alt/scenes/shared/mahouya_scene/sh_maho_book_01__UDIM_"}) {
      auto maps = mm->archives->archives->back();
      maps->paths.erase(missing);
      assert(!DependenciesAvailable());
      maps->paths.insert(missing);
    }
    mm->archives->archives->erase(mm->archives->archives->begin());
    assert(!DependenciesAvailable());
    Ship::Context::current = nullptr;
    assert(!DependenciesAvailable());
  }
  std::printf("PASS Granny dependency selection: %s\n", argv[1]);
}
