#include "combo/BlackMarketRoomMagic.h"
#include <cassert>
#include <cstdio>

using namespace BlackMarketRoomMagic;
struct Graph {
  std::map<std::string, std::shared_ptr<std::vector<Gfx>>> lists;
  std::shared_ptr<std::vector<Vtx>> vertices =
      std::make_shared<std::vector<Vtx>>(4);
  std::shared_ptr<int> texture = std::make_shared<int>(1);
  std::deque<std::string> paths;
  Graph() {
    auto &root = *(lists[kRoom] = std::make_shared<std::vector<Gfx>>());
    for (size_t i = 0; i < 28; ++i) {
      paths.push_back(MaterialPath(i));
      const auto color = kDefaultPalette[i % 4];
      lists[paths.back()] =
          std::make_shared<std::vector<Gfx>>(std::initializer_list<Gfx>{
              gsDPSetPrimColor(0, 0, color[0], color[1], color[2], 255),
              gsSPScrollTexture(0, int(i % 3) - 1, int(i % 2) * 2 - 1),
              gsDPSetTextureImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, kTexture),
              gsSPEndDisplayList()});
      lists[paths.back()]->at(2).words.w0 = G_SETTIMG_OTR_FILEPATH << 24;
      root.push_back({gsSPDisplayListOTRFilePath(paths.back().c_str())});
    }
    paths.push_back("custom/skiejer_market/mm_shop_presentation_v1/geometry/"
                    "accepted_vertices");
    root.push_back({gsSPNoOp()});
    root.back().words = {G_VTX_OTR_FILEPATH << 24,
                         reinterpret_cast<uintptr_t>(paths.back().c_str())};
    root.push_back({gsSPNoOp()});
    root.back().words = {3, (1 << 16) | 1};
    root.push_back({gsSPEndDisplayList()});
  }
  Provider Get(const char *host) {
    return {host,
            [this](const std::string &path) -> Display {
              auto it = lists.find(path);
              return it == lists.end()
                         ? Display{}
                         : Display{it->second->data(), it->second->size(),
                                   it->second};
            },
            [this](const std::string &) {
              return Vertices{vertices->data(), vertices->size(), vertices};
            },
            [this](const std::string &path) -> std::shared_ptr<void> {
              return path == kTexture ? texture : nullptr;
            }};
  }
};
int main() {
  for (const char *host : {"oot", "mm"}) {
    for (const char *prefix : {"__OTR__", "__OTR__alt/"}) {
      const auto source =
          std::string(prefix) + SourceRoom(std::strcmp(host, "oot") == 0);
      assert(MatchesSource(reinterpret_cast<const Gfx *>(source.c_str()),
                           std::strcmp(host, "oot") == 0));
      assert(!MatchesSource(reinterpret_cast<const Gfx *>(source.c_str()),
                            std::strcmp(host, "oot") != 0));
    }
    Graph graph;
    Room room;
    assert(room.Load(graph.Get(host)));
    auto *root = room.Opaque();
    assert(root && root != graph.lists.at(kRoom)->data());
    Palette palette = kDefaultPalette;
    for (size_t slot = 0; slot < 4; ++slot) {
      palette[slot] = {static_cast<uint8_t>(slot + 1), 40, 80, 12};
      room.SetPalette(palette);
      for (size_t i = 0; i < 28; ++i) {
        assert(root[i].words.w0 >> 24 == G_DL);
        auto *material = reinterpret_cast<Gfx *>(root[i].words.w1);
        const auto expected = palette[i % 4];
        assert(material[0].words.w1 ==
               ((expected[0] << 24) | (expected[1] << 16) | (expected[2] << 8) |
                255u));
        assert(material[1].words.w0 ==
               graph.lists.at(MaterialPath(i))->at(1).words.w0);
        assert(material[1].words.w1 ==
               graph.lists.at(MaterialPath(i))->at(1).words.w1);
        const auto original = kDefaultPalette[i % 4];
        assert(graph.lists.at(MaterialPath(i))->at(0).words.w1 ==
               ((original[0] << 24) | (original[1] << 16) | (original[2] << 8) |
                255u));
        assert(
            std::string(reinterpret_cast<const char *>(material[2].words.w1)) ==
            std::string("__OTR__@") + host + ":" + kTexture);
      }
    }
    room.SetPalette(kDefaultPalette);
    assert(root[28].words.w0 >> 24 == G_VTX);
    assert(root[28].words.w1 ==
           reinterpret_cast<uintptr_t>(graph.vertices->data() + 1));
    std::weak_ptr<void> owner = graph.texture;
    graph.texture.reset();
    assert(!owner.expired());
    room.Clear();
    assert(!room.Opaque() && owner.expired());
    Graph missing;
    missing.lists.erase(MaterialPath(17));
    assert(!room.Load(missing.Get(host)) && !room.Opaque());
    Graph malformed;
    malformed.lists.at(MaterialPath(0))->at(0) = {gsSPNoOp()};
    assert(!room.Load(malformed.Get(host)) && !room.Opaque());
    Graph recursive;
    recursive.lists[kRoom]->insert(recursive.lists[kRoom]->begin(),
                                   {gsSPDisplayListOTRFilePath(kRoom)});
    assert(!room.Load(recursive.Get(host)) && !room.Opaque());
    Graph badVertex;
    badVertex.lists[kRoom]->at(29).words.w0 = 30;
    assert(!room.Load(badVertex.Get(host)) && !room.Opaque());
    Graph valid;
    assert(room.Load(valid.Get(host)) && room.Opaque());
  }
  puts("PASS: owned room graph; four independent live colors; immutable "
       "source; forced alpha; original scroll; vertex identity; host texture "
       "routing; missing/malformed fallback; teardown");
  return 0;
}
