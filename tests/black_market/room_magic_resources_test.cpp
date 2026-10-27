// The resource adapter is included unchanged below; archive/RM lookups are the
// boundary fixture. Real Room copying still executes for every successful load.
#define main RoomGraphFixtureMain
#include "room_magic_test.cpp"
#undef main
namespace Fast {
struct Resource {
  virtual ~Resource() = default;
};
struct DisplayList : Resource {
  std::vector<Gfx> Instructions;
};
struct Vertex : Resource {
  std::vector<Vtx> VertexList;
};
struct Texture : Resource {};
} // namespace Fast
namespace Ship {
struct Archive {
  std::set<std::string> paths;
  bool HasFile(const std::string &path) { return paths.contains(path); }
};
class ResourceManager {
public:
  Graph graph;
  bool alt = false;
  std::shared_ptr<Archive> archive = std::make_shared<Archive>();
  std::map<std::string, std::shared_ptr<Fast::Resource>> resources;
  std::vector<std::string> requested;
  ResourceManager() {
    for (const auto &[path, list] : graph.lists) {
      auto resource = std::make_shared<Fast::DisplayList>();
      resource->Instructions = *list;
      resources[path] = resource;
      archive->paths.insert(path);
    }
    auto vertices = std::make_shared<Fast::Vertex>();
    vertices->VertexList = *graph.vertices;
    resources[graph.paths.back()] = vertices;
    resources[kTexture] = std::make_shared<Fast::Texture>();
    archive->paths.insert(kTexture);
  }
  bool IsAltAssetsEnabled() const { return alt; }
  auto GetArchiveManager() { return archive; }
  std::shared_ptr<Fast::Resource> LoadResource(const std::string &path,
                                               bool exact = false) {
    assert(!exact && "Private graph must use the host's normal/Alt selection");
    requested.push_back(path);
    if (alt && !path.starts_with("alt/") && resources.contains("alt/" + path))
      return resources.at("alt/" + path);
    auto it = resources.find(path);
    return it == resources.end() ? nullptr : it->second;
  }
};
struct CrossRMRegistry {
  static inline std::map<std::string, std::shared_ptr<ResourceManager>> owners;
  static inline std::shared_ptr<ResourceManager> active;
  static std::shared_ptr<ResourceManager> Get(const std::string &host) {
    const auto found = owners.find(host);
    return found == owners.end() ? nullptr : found->second;
  }
  static auto GetOrActive(const std::string &host) {
    const auto found = owners.find(host);
    return found == owners.end() ? active : found->second;
  }
};
} // namespace Ship
namespace BlackMarketRoomMagic {
/* RESOURCES */
}
int main() {
  for (const char *host : {"oot", "mm"}) {
    auto own = std::make_shared<Ship::ResourceManager>();
    auto foreign = std::make_shared<Ship::ResourceManager>();
    Ship::CrossRMRegistry::owners[host] = own;
    Ship::CrossRMRegistry::active = foreign;
    Resources resources;
    assert(resources.Prepare(host) && resources.Opaque());
    assert(foreign->requested.empty() && !own->requested.empty());
    auto *root = resources.Opaque();
    assert(resources.Prepare(host) && resources.Opaque() == root);
    own->alt = true; // Base fallback remains available with Alt enabled.
    assert(resources.Prepare(host));
    const auto source =
        std::string("alt/") + SourceRoom(std::string(host) == "oot");
    own->archive->paths.insert(source);
    assert(!resources.Prepare(
        host));                  // An unreadable Alt source still fails closed.
    assert(!resources.Opaque()); // A failed replacement cannot retain a
                                 // previous owned graph.
    // A stock Alt room has no private magical graph. Apply its materials
    // while retaining the active room's own geometry and vertex overrides.
    std::deque<std::string> sourceMaterials;
    auto sourceRoot = std::make_shared<Fast::DisplayList>();
    sourceRoot->Instructions =
        std::dynamic_pointer_cast<Fast::DisplayList>(own->resources.at(kRoom))
            ->Instructions;
    for (size_t i = 0; i < 28; ++i) {
      sourceMaterials.push_back("custom/skiejer_market/" +
                                MaterialPath(i).substr(std::strlen(kPrefix)));
      sourceRoot->Instructions[i].words.w1 =
          reinterpret_cast<uintptr_t>(sourceMaterials.back().c_str());
    }
    sourceRoot->Instructions.insert(sourceRoot->Instructions.end() - 1,
                                    {gsSP1Triangle(0, 1, 2, 0)});
    own->resources[source] = sourceRoot;
    auto sourceVertices = std::make_shared<Fast::Vertex>();
    sourceVertices->VertexList.resize(4);
    sourceVertices->VertexList[1].v.ob[0] = 456;
    own->resources["alt/" + own->graph.paths.back()] = sourceVertices;
    const auto sourceSnapshot = sourceRoot->Instructions;
    assert(resources.Prepare(host) && resources.Opaque());
    assert(resources.Opaque()[28].words.w1 ==
           reinterpret_cast<uintptr_t>(sourceVertices->VertexList.data() + 1));
    assert(std::memcmp(&resources.Opaque()[30], &sourceSnapshot[30],
                       2 * sizeof(Gfx)) == 0);
    Palette palette = kDefaultPalette;
    palette[0] = {11, 22, 33, 44};
    resources.SetPalette(palette);
    auto *material = reinterpret_cast<Gfx *>(resources.Opaque()[0].words.w1);
    assert(material[0].words.w1 == 0x0b1621ff);
    assert(std::memcmp(sourceRoot->Instructions.data(), sourceSnapshot.data(),
                       sourceSnapshot.size() * sizeof(Gfx)) == 0);
    root = resources.Opaque();
    assert(resources.Prepare(host) && resources.Opaque() == root);
    own->alt = false;
    assert(resources.Prepare(host));
    const auto baseVertices = std::dynamic_pointer_cast<Fast::Vertex>(
        own->resources.at(own->graph.paths.back()));
    assert(resources.Opaque()[28].words.w1 ==
           reinterpret_cast<uintptr_t>(baseVertices->VertexList.data() + 1));
    own->alt = true;
    assert(resources.Prepare(host));
    assert(resources.Opaque()[28].words.w1 ==
           reinterpret_cast<uintptr_t>(sourceVertices->VertexList.data() + 1));
    const auto explicitSource = std::string("__OTR__") + source;
    for (bool altEnabled : {false, true}) {
      own->alt = altEnabled;
      assert(resources.Prepare(
          host, reinterpret_cast<const Gfx *>(explicitSource.c_str())));
      assert(std::memcmp(&resources.Opaque()[30], &sourceSnapshot[30],
                         2 * sizeof(Gfx)) == 0);
      const auto expectedVertices = altEnabled ? sourceVertices : baseVertices;
      assert(
          resources.Opaque()[28].words.w1 ==
          reinterpret_cast<uintptr_t>(expectedVertices->VertexList.data() + 1));
      root = resources.Opaque();
      assert(resources.Prepare(
                 host, reinterpret_cast<const Gfx *>(explicitSource.c_str())) &&
             resources.Opaque() == root);
    }
    // An explicitly paired optional Alt graph retains precedence.
    auto altRoot = std::make_shared<Fast::DisplayList>();
    altRoot->Instructions =
        std::dynamic_pointer_cast<Fast::DisplayList>(own->resources.at(kRoom))
            ->Instructions;
    own->archive->paths.insert(std::string("alt/") + kRoom);
    own->resources[std::string("alt/") + kRoom] = altRoot;
    auto altVertices = std::make_shared<Fast::Vertex>();
    altVertices->VertexList.resize(4);
    altVertices->VertexList[1].v.ob[0] = 123;
    own->resources["alt/" + own->graph.paths.back()] = altVertices;
    assert(resources.Prepare(host));
    assert(resources.Opaque()[28].words.w1 ==
           reinterpret_cast<uintptr_t>(altVertices->VertexList.data() + 1));
    auto replacement = std::make_shared<Ship::ResourceManager>();
    Ship::CrossRMRegistry::owners[host] = replacement;
    assert(resources.Prepare(host));
    assert(resources.Opaque()[28].words.w1 !=
           reinterpret_cast<uintptr_t>(altVertices->VertexList.data() + 1));
    std::weak_ptr<Ship::ResourceManager> retained = replacement;
    Ship::CrossRMRegistry::owners.clear();
    replacement.reset();
    assert(!retained.expired());
    assert(!resources.Prepare(host) && !resources.Opaque() &&
           retained.expired());
    assert(foreign->requested.empty()); // A named texture route must never
                                        // borrow an unregistered active owner.
    // Supported solo mode registers its one host and needs no foreign owner.
    Ship::CrossRMRegistry::owners[host] = own;
    assert(Ship::CrossRMRegistry::owners.size() == 1 &&
           resources.Prepare(host));
    resources.Clear();
    Ship::CrossRMRegistry::owners.clear();
    Ship::CrossRMRegistry::active.reset();
    assert(!resources.Prepare(host) && !resources.Opaque());
  }
  puts("PASS: production resource adapter respects native host ownership, "
       "registered solo mode, absent-route rejection, active Alt "
       "geometry/materials, normal/Alt transitions, paired overrides, owner "
       "replacement and teardown");
}
