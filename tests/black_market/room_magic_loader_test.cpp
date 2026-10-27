// Load both generated variants with the engine's real XML display-list and
// vertex factories, then compare every copied instruction with its source.
#include "combo/BlackMarketRoomMagic.h"
#include "fast/resource/factory/DisplayListFactory.h"
#include "fast/resource/factory/VertexFactory.h"
#include "fast/resource/type/DisplayList.h"
#include "fast/resource/type/Vertex.h"
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <tinyxml2.h>

using namespace BlackMarketRoomMagic;
static std::filesystem::path assets;
static std::string host;
static bool useSource;
static std::map<std::string, std::shared_ptr<Ship::IResource>> loaded;
static std::map<std::string, std::vector<Gfx>> originals;
static size_t triangles, scrolls, vertexLoads;
static std::set<std::string> acceptedVertices, usedVertices;

static std::shared_ptr<Ship::IResource> Load(const std::string &path) {
  if (auto found = loaded.find(path); found != loaded.end())
    return found->second;
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
    originals[path] =
        std::dynamic_pointer_cast<Fast::DisplayList>(resource)->Instructions;
  } else {
    Fast::ResourceFactoryXMLVertexV0 factory;
    resource = factory.ReadResource(file, init);
  }
  assert(resource);
  loaded[path] = resource;
  return resource;
}
static void CollectSourceVertices(const std::string &path) {
  const auto source = std::dynamic_pointer_cast<Fast::DisplayList>(Load(path));
  for (size_t i = 0; i < source->Instructions.size(); ++i) {
    const auto command = source->Instructions[i];
    if (command.words.w0 >> 24 == G_DL_OTR_FILEPATH)
      CollectSourceVertices(reinterpret_cast<const char *>(command.words.w1));
    else if (command.words.w0 >> 24 == G_VTX_OTR_FILEPATH) {
      acceptedVertices.insert(reinterpret_cast<const char *>(command.words.w1));
      ++i;
    }
  }
}
static void Compare(const std::string &path, const Gfx *copy,
                    const Palette &palette) {
  // Expected material substitution is derived from the generated resource
  // namespaces, independently of the production material-path classifier.
  const std::string sourceMaterial =
      "custom/skiejer_market/geometry/mat_mahouya_room_0DL_006548_GranmaShop";
  if (useSource && path.starts_with(sourceMaterial)) {
    Compare("custom/skiejer_market/room_magic_v1/geometry/"
            "mat_mahouya_room_0DL_006548_GranmaShop" +
                path.substr(sourceMaterial.size()),
            copy, palette);
    return;
  }
  const auto source = std::dynamic_pointer_cast<Fast::DisplayList>(Load(path));
  const auto &list = source->Instructions;
  for (size_t i = 0; i < list.size(); ++i) {
    const auto &command = list[i];
    const auto opcode = command.words.w0 >> 24;
    if (opcode == G_DL_OTR_FILEPATH) {
      assert(copy[i].words.w0 ==
             ((command.words.w0 & 0xffffff) | (uint32_t(G_DL) << 24)));
      Compare(reinterpret_cast<const char *>(command.words.w1),
              reinterpret_cast<Gfx *>(copy[i].words.w1), palette);
    } else if (opcode == G_VTX_OTR_FILEPATH) {
      auto vertices = std::dynamic_pointer_cast<Fast::Vertex>(
          Load(reinterpret_cast<const char *>(command.words.w1)));
      const auto args = list[i + 1].words;
      Gfx expected = {
          gsSPVertex(vertices->VertexList.data() + (args.w1 & 0xffff), args.w0,
                     args.w1 >> 16)};
      assert(std::memcmp(&copy[i], &expected, sizeof(Gfx)) == 0);
      const Gfx noOp = {gsSPNoOp()};
      ++i;
      assert(std::memcmp(&copy[i], &noOp, sizeof(Gfx)) == 0);
      ++vertexLoads;
    } else if (opcode == G_SETTIMG_OTR_FILEPATH) {
      assert(copy[i].words.w0 == command.words.w0);
      assert(std::string(reinterpret_cast<const char *>(copy[i].words.w1)) ==
             "__OTR__@" + host + ":" + kTexture);
    } else if (opcode == G_SETPRIMCOLOR &&
               path.find("/mat_") != std::string::npos) {
      const auto index = std::stoul(path.substr(path.size() - 2));
      const auto color = palette[index % 4];
      assert(copy[i].words.w0 == command.words.w0);
      assert(copy[i].words.w1 ==
             ((uint32_t(color[0]) << 24) | (uint32_t(color[1]) << 16) |
              (uint32_t(color[2]) << 8) | 255));
    } else {
      assert(std::memcmp(&copy[i], &command, sizeof(Gfx)) == 0);
      triangles += opcode == G_TRI2
                       ? 2
                       : (opcode == G_TRI1 || opcode == G_TRI1_OTR ? 1 : 0);
      scrolls += opcode == G_SCROLL_TEXTURE;
    }
  }
}
int main(int argc, char **argv) {
  assert(argc == 4);
  assets = argv[1];
  host = argv[2];
  useSource = std::strcmp(argv[3], "source") == 0;
  CollectSourceVertices(SourceRoom(host == "oot"));
  Provider provider{
      host,
      [](const std::string &path) {
        const auto resource =
            std::dynamic_pointer_cast<Fast::DisplayList>(Load(path));
        return Display{resource->Instructions.data(),
                       resource->Instructions.size(), resource};
      },
      [](const std::string &path) {
        assert(acceptedVertices.contains(path));
        usedVertices.insert(path);
        const auto resource =
            std::dynamic_pointer_cast<Fast::Vertex>(Load(path));
        return Vertices{resource->VertexList.data(),
                        resource->VertexList.size(), resource};
      },
      [](const std::string &path) -> std::shared_ptr<void> {
        assert(path == kTexture &&
               std::filesystem::file_size(assets / path) > 64);
        return std::make_shared<int>(1); // Texture decoding/pixel hashes have a
                                         // separate source-asset test.
      }};
  Room room;
  assert(room.Load(provider, useSource));
  Palette palette = {Color{1, 2, 3, 0}, Color{4, 5, 6, 7}, Color{8, 9, 10, 11},
                     Color{12, 13, 14, 15}};
  room.SetPalette(palette);
  const auto root = useSource ? SourceRoom(host == "oot") : kRoom;
  Compare(root, room.Opaque(), palette);
  const int expectedTriangles = host == "mm" ? 7577 : 7919;
  assert(triangles == expectedTriangles && scrolls == 30 && vertexLoads == 342);
  assert(usedVertices == acceptedVertices);
  for (const auto &[path, snapshot] : originals) {
    const auto actual =
        std::dynamic_pointer_cast<Fast::DisplayList>(loaded.at(path));
    assert(actual->Instructions.size() == snapshot.size());
    assert(std::memcmp(actual->Instructions.data(), snapshot.data(),
                       snapshot.size() * sizeof(Gfx)) == 0);
  }
  room.SetPalette(kDefaultPalette);
  triangles = scrolls = vertexLoads = 0;
  Compare(root, room.Opaque(), kDefaultPalette);
  assert(triangles == expectedTriangles && scrolls == 30);
  room.Clear();
  assert(!room.Opaque());
  std::printf("PASS %s %s real XML graph: %d retained triangles, 30 original "
              "scroll calls, exact host vertex pointers/UVs, live/reset "
              "palette, immutable source commands\n",
              host.c_str(), argv[3], expectedTriangles);
}
