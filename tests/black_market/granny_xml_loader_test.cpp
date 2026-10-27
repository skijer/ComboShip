#include "combo/BlackMarketGranny.h"
#include <cassert>
#include <cstdio>
#include <fast/resource/factory/DisplayListFactory.h>
#include <fast/resource/factory/VertexFactory.h>
#include <fast/resource/type/DisplayList.h>
#include <fast/resource/type/Vertex.h>
#include <filesystem>
#include <fstream>
#include <tinyxml2.h>

using namespace BlackMarketGranny;
static std::filesystem::path assets;
static std::set<std::string> textures;
static std::map<std::string, std::shared_ptr<Ship::IResource>> loaded;
static std::map<std::string, std::vector<Gfx>> originalLists;
static std::map<std::string, std::vector<Vtx>> originalVertices;
static size_t sourceSingleTriangles;
static std::shared_ptr<Ship::IResource> Load(const std::string &path) {
  if (auto it = loaded.find(path); it != loaded.end())
    return it->second;
  auto document = std::make_shared<tinyxml2::XMLDocument>();
  assert(document->LoadFile((assets / path).string().c_str()) ==
         tinyxml2::XML_SUCCESS);
  auto file = std::make_shared<Ship::File>();
  file->Reader = document;
  auto init = std::make_shared<Ship::ResourceInitData>();
  init->Format = RESOURCE_FORMAT_XML;
  init->Path = path;
  std::shared_ptr<Ship::IResource> resource;
  if (std::string(document->FirstChildElement()->Name()) == "DisplayList") {
    Fast::ResourceFactoryXMLDisplayListV0 factory;
    resource = factory.ReadResource(file, init);
    originalLists[path] =
        std::dynamic_pointer_cast<Fast::DisplayList>(resource)->Instructions;
  } else {
    Fast::ResourceFactoryXMLVertexV0 factory;
    resource = factory.ReadResource(file, init);
    originalVertices[path] =
        std::dynamic_pointer_cast<Fast::Vertex>(resource)->VertexList;
  }
  assert(resource);
  loaded[path] = resource;
  return resource;
}
static size_t SourceTriangles(const std::string &path) {
  size_t count = 0;
  auto list = std::dynamic_pointer_cast<Fast::DisplayList>(Load(path));
  for (const auto &command : list->Instructions) {
    const auto op = static_cast<uint8_t>(command.words.w0 >> 24);
    if (op == G_DL_OTR_FILEPATH)
      count += SourceTriangles(
          Exact(reinterpret_cast<const char *>(command.words.w1)));
    else if (op == G_TRI1_OTR) {
      ++sourceSingleTriangles;
      ++count;
    } else if (op == G_TRI1)
      ++count;
    else if (op == G_TRI2)
      count += 2;
  }
  return count;
}
static size_t CopiedTriangles(const Gfx *list) {
  size_t count = 0;
  for (size_t i = 0; i < 100000; ++i) {
    const auto &command = list[i];
    const auto op = static_cast<uint8_t>(command.words.w0 >> 24);
    if (op == G_DL)
      count += CopiedTriangles(reinterpret_cast<const Gfx *>(command.words.w1));
    else if (op == G_TRI1_OTR || op == G_TRI1)
      ++count;
    else if (op == G_TRI2)
      count += 2;
    else if (op == G_ENDDL)
      return count;
  }
  assert(false);
  return 0;
}
int main(int argc, char **argv) {
  assert(argc == 3);
  assets = argv[1];
  std::ifstream names(assets / "textures.txt");
  std::string line;
  while (std::getline(names, line))
    textures.insert(line);
  const auto before = SourceTriangles(Exact(kOpaque));
  assert(sourceSingleTriangles > 0 && "real native XML factory must exercise "
                                      "its extended single-triangle encoding");
  Provider provider{
      argv[2],
      [](const std::string &path) {
        auto r = std::dynamic_pointer_cast<Fast::DisplayList>(Load(path));
        return Display{r->Instructions.data(), r->Instructions.size(), r};
      },
      [](const std::string &path) {
        auto r = std::dynamic_pointer_cast<Fast::Vertex>(Load(path));
        return Vertices{r->VertexList.data(), r->VertexList.size(), r};
      },
      [](const std::string &path) -> std::shared_ptr<void> {
        assert(textures.contains(path));
        return std::make_shared<int>(1);
      }};
  Room room;
  const bool ready = room.Load(provider, true);
  if (!ready)
    std::fprintf(stderr, "Granny real XML loader: %s\n", room.Error().c_str());
  assert(ready && "unchanged donor must pass the rear-pot validator after "
                  "actual native XML decoding");
  const auto after = CopiedTriangles(room.Opaque());
  if (after != before - 342 + 4 || room.Counter().size() != 1197)
    std::fprintf(stderr,
                 "Granny XML counts: before=%zu after=%zu counter=%zu\n",
                 before, after, room.Counter().size());
  assert(after == before - 342 + 4 && room.Counter().size() == 1197);
  for (const auto &[path, original] : originalLists) {
    auto actual = std::dynamic_pointer_cast<Fast::DisplayList>(loaded.at(path));
    assert(actual->Instructions.size() == original.size());
    assert(std::memcmp(actual->Instructions.data(), original.data(),
                       original.size() * sizeof(Gfx)) == 0);
  }
  for (const auto &[path, original] : originalVertices) {
    auto actual = std::dynamic_pointer_cast<Fast::Vertex>(loaded.at(path));
    assert(actual->VertexList.size() == original.size());
    assert(std::memcmp(actual->VertexList.data(), original.data(),
                       original.size() * sizeof(Vtx)) == 0);
  }
  std::printf("PASS %s actual XML Granny: %zu source triangles, %zu extended "
              "singles; exactly 342 rear faces removed, four counter seam "
              "faces added; immutable source\n",
              argv[2], before, sourceSingleTriangles);
}
