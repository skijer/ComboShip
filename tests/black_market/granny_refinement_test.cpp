#include "combo/BlackMarketGranny.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
using namespace BlackMarketGranny;
static std::ifstream input;
static uint32_t U32() {
  uint32_t n;
  input.read(reinterpret_cast<char *>(&n), 4);
  assert(input);
  return n;
}
static uint64_t U64() {
  uint64_t n;
  input.read(reinterpret_cast<char *>(&n), 8);
  assert(input);
  return n;
}
static std::string String() {
  std::string s(U32(), '\0');
  input.read(s.data(), s.size());
  assert(input);
  return s;
}
int main(int argc, char **argv) {
  assert(argc == 4);
  input.open(argv[1], std::ios::binary);
  assert(input);
  std::map<std::string, std::shared_ptr<std::vector<Gfx>>> displays;
  std::map<std::string, std::vector<Gfx>> expected;
  std::map<std::string, std::shared_ptr<std::vector<Vtx>>> vertices;
  std::set<std::string> textures;
  std::deque<std::string> paths;
  const unsigned resources = U32();
  for (unsigned r = 0; r < resources; ++r) {
    auto path = String();
    const unsigned kind = U32(), count = U32();
    if (kind == 0) {
      auto list = std::make_shared<std::vector<Gfx>>();
      for (unsigned i = 0; i < count; ++i) {
        Gfx g{};
        g.words.w0 = U32();
        g.words.w1 = U64();
        auto reference = String();
        if (!reference.empty()) {
          paths.push_back(reference);
          g.words.w1 = (uintptr_t)paths.back().c_str();
        }
        list->push_back(g);
        Gfx want{};
        want.words.w0 = U32();
        want.words.w1 = U64();
        expected[path].push_back(want);
      }
      displays[path] = list;
    } else if (kind == 1) {
      auto data = std::make_shared<std::vector<Vtx>>(count);
      input.read(reinterpret_cast<char *>(data->data()), count * sizeof(Vtx));
      assert(input);
      vertices[path] = data;
    } else {
      assert(kind == 2 && count == 0);
      textures.insert(path);
    }
  }
  auto originalDisplays = std::map<std::string, std::vector<Gfx>>{};
  auto originalVertices = std::map<std::string, std::vector<Vtx>>{};
  for (auto &[name, list] : displays)
    originalDisplays[name] = *list;
  for (auto &[name, list] : vertices)
    originalVertices[name] = *list;
  Provider provider;
  provider.game = "oot";
  provider.display = [&](const std::string &path) -> Display {
    auto at = displays.find(path);
    return at == displays.end()
               ? Display{}
               : Display{at->second->data(), at->second->size(), at->second};
  };
  provider.vertices = [&](const std::string &path) -> Vertices {
    auto at = vertices.find(path);
    return at == vertices.end()
               ? Vertices{}
               : Vertices{at->second->data(), at->second->size(), at->second};
  };
  provider.texture = [&](const std::string &path) -> std::shared_ptr<void> {
    return textures.contains(path) ? std::make_shared<int>(1) : nullptr;
  };
  const auto loadRefined = [](auto &room, const Provider &provider) {
    if constexpr (requires { room.Load(provider, true); })
      return room.Load(provider, true);
    else
      return room.Load(provider);
  };
  Room room;
  if (!loadRefined(room, provider)) {
    std::cerr << room.Error() << '\n';
    return 1;
  }
  const Room *checkingRoom = &room;
  size_t unchanged = 0, loads = 0, textureReferences = 0;
  std::set<std::string> visited, usedTextures;
  std::function<void(const std::string &, Gfx *)> compare =
      [&](const std::string &path, Gfx *copy) {
        if (!visited.insert(path).second)
          return;
        const auto &source = *displays.at(path);
        for (size_t i = 0; i < source.size(); ++i) {
          const auto &g = source[i];
          const auto op = g.words.w0 >> 24;
          if (op == 0x27) {
            assert(copy[i].words.w0 ==
                   ((g.words.w0 & 0xffffff) | (G_DL << 24)));
            compare(Exact(reinterpret_cast<const char *>(g.words.w1)),
                    reinterpret_cast<Gfx *>(copy[i].words.w1));
          } else if (op == 0x24) {
            const auto &args = source[i + 1].words;
            const auto name = Exact(reinterpret_cast<const char *>(g.words.w1));
            const Vtx *data = name == Exact(kCounterVertices)
                                  ? checkingRoom->Counter().data()
                                  : vertices.at(name)->data();
            Gfx expected = {
                gsSPVertex(data + (args.w1 & 0xffff), args.w0, args.w1 >> 16)};
            assert(copy[i].words.w0 == expected.words.w0 &&
                   copy[i].words.w1 == expected.words.w1);
            ++i;
            assert((copy[i].words.w0 >> 24) == G_SPNOOP);
            ++loads;
          } else if (op == 0x25) {
            const auto name = Exact(reinterpret_cast<const char *>(g.words.w1));
            usedTextures.insert(name);
            assert(copy[i].words.w0 == g.words.w0);
            assert(
                std::string(reinterpret_cast<const char *>(copy[i].words.w1)) ==
                "__OTR__@" + provider.game + ":" + name);
            ++textureReferences;
          } else if (op == 5 || op == 6 || op == G_TRI1_OTR) {
            const auto &want = expected.at(path)[i];
            if (copy[i].words.w0 != want.words.w0 ||
                copy[i].words.w1 != want.words.w1)
              std::fprintf(stderr,
                           "Mismatch %s command%zu: %lx/%lx expected%lx/%lx\n",
                           path.c_str(), i, copy[i].words.w0, copy[i].words.w1,
                           want.words.w0, want.words.w1);
            assert(copy[i].words.w0 == want.words.w0 &&
                   copy[i].words.w1 == want.words.w1 &&
                   "only exact complete rear pot components may disappear from "
                   "the private donor draw");
          } else if (op == G_ENDDL && path == Exact(kCounterDraw)) {
            assert((copy[i].words.w0 >> 24) == G_VTX &&
                   (copy[i + 6].words.w0 >> 24) == G_ENDDL);
          } else {
            assert(std::memcmp(&copy[i], &g, sizeof(Gfx)) == 0);
            ++unchanged;
          }
        }
      };
  if (std::string(argv[3]) != "vapor")
    compare(Exact(kOpaque), room.Opaque());
  if (std::string(argv[3]) != "vapor") {
    provider.game = "mm";
    Room mmOwned;
    assert(loadRefined(mmOwned, provider));
    checkingRoom = &mmOwned;
    visited.clear();
    compare(Exact(kOpaque), mmOwned.Opaque());
    auto source = vertices.at(Exact(std::string(kOpaque) + "_vtx_0"));
    const auto saved = (*source)[0];
    (*source)[0].v.ob[0] = 30000;
    Room incompatible;
    assert(!loadRefined(incompatible, provider) && !incompatible.Opaque());
    (*source)[0] = saved;
  }
  for (auto &[name, list] : displays)
    assert(std::memcmp(list->data(), originalDisplays.at(name).data(),
                       list->size() * sizeof(Gfx)) == 0);
  for (auto &[name, list] : vertices)
    assert(std::memcmp(list->data(), originalVertices.at(name).data(),
                       list->size() * sizeof(Vtx)) == 0);
  if (std::string(argv[3]) != "opa") {
    input.close();
    input.open(argv[2], std::ios::binary);
    const auto rootCount = U32(), leafCount = U32(), vertexCount = U32();
    auto readList = [&](unsigned count) {
      auto data = std::make_shared<std::vector<Gfx>>(count);
      for (auto &g : *data) {
        g.words.w0 = U32();
        g.words.w1 = U32();
      }
      return data;
    };
    auto root = readList(rootCount), leaf = readList(leafCount);
    auto verts = std::make_shared<std::vector<Vtx>>(vertexCount);
    input.read(reinterpret_cast<char *>(verts->data()),
               vertexCount * sizeof(Vtx));
    assert(input);
    const auto originalRoot = *root;
    const auto originalLeaf = *leaf;
    const auto originalVerts = *verts;
#if __has_include("combo/BlackMarketGrannyRefinement.h")
    BlackMarketGranny::Vapor vapor;
    assert(vapor.Load({root->data(), root->size(), root},
                      {leaf->data(), leaf->size(), leaf},
                      {verts->data(), verts->size(), verts}));
    const Gfx *copy = reinterpret_cast<const Gfx *>(vapor.Root()[2].words.w1);
#else
    const Gfx *copy = leaf->data();
#endif
    unsigned triangles = 0, deleted = 0;
    for (size_t i = 0; i < leaf->size(); ++i) {
      const auto &g = (*leaf)[i];
      const bool rear = i == 36 || i == 37 || (i >= 40 && i <= 43);
      if (rear) {
        assert(copy[i].words.w0 == 0xe0000000 && copy[i].words.w1 == 0 &&
               "rear native cauldron steam must be absent");
        ++deleted;
      } else
        assert(!std::memcmp(copy + i, &g, sizeof(Gfx)) &&
               "unrelated native translucent commands stay exact");
      triangles += (copy[i].words.w0 >> 24) == 6   ? 2
                   : (copy[i].words.w0 >> 24) == 5 ? 1
                                                   : 0;
    }
    assert(triangles == 12 && deleted == 6);
#if __has_include("combo/BlackMarketGrannyRefinement.h")
    (*verts)[24].v.ob[0] = 30000;
    assert(!vapor.Load({root->data(), root->size(), root},
                       {leaf->data(), leaf->size(), leaf},
                       {verts->data(), verts->size(), verts}) &&
           !vapor.Root() &&
           "changed vapor layout fails closed without a stale graph");
    (*verts)[24] = originalVerts[24];
    BlackMarketGranny::Vapor changed;
    (*leaf)[36].words.w0 ^= 2;
    assert(!changed.Load({root->data(), root->size(), root},
                         {leaf->data(), leaf->size(), leaf},
                         {verts->data(), verts->size(), verts}) &&
           !changed.Root());
    *leaf = originalLeaf;
#endif
    assert(!std::memcmp(root->data(), originalRoot.data(),
                        root->size() * sizeof(Gfx)));
    assert(!std::memcmp(leaf->data(), originalLeaf.data(),
                        leaf->size() * sizeof(Gfx)));
    assert(!std::memcmp(verts->data(), originalVerts.data(),
                        verts->size() * sizeof(Vtx)));
  }
  if (!usedTextures.empty()) {
    auto missing = *usedTextures.begin();
    textures.erase(missing);
    Room partial;
    assert(!loadRefined(partial, provider) && !partial.Opaque());
  }
  std::cout << "{\"lists\":" << visited.size()
            << ",\"unchanged_commands\":" << unchanged
            << ",\"vertex_loads\":" << loads
            << ",\"texture_references\":" << textureReferences
            << ",\"textures\":" << usedTextures.size()
            << ",\"missing_texture_rejected\":true,\"original_buffers_"
               "unchanged\":true}\n";
}
