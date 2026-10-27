#include "combo/BlackMarketRoomMagic.h"
#include "global.h"
#include <cassert>
#include <cstdio>
#include <libultraship/color.h>

static std::map<std::string, int> integers;
static std::map<std::string, Color_RGBA8> colors;
extern "C" int32_t CVarGetInteger(const char *key, int32_t fallback) {
  auto it = integers.find(key);
  return it == integers.end() ? fallback : it->second;
}
extern "C" Color_RGBA8 CVarGetColor(const char *key, Color_RGBA8 fallback) {
  auto it = colors.find(key);
  return it == colors.end() ? fallback : it->second;
}
namespace BlackMarketRoomMagic {
/* PALETTE */
struct Resources {
  bool available = true;
  Room room;
  Gfx opaque[1] = {gsSPEndDisplayList()};
  Palette palette{};
  int prepares = 0, clears = 0;
  std::string host;
  const Gfx *source = nullptr;
  bool Prepare(const char *value, const Gfx *opaque = nullptr) {
    ++prepares;
    host = value;
    source = opaque;
    return available;
  }
  Gfx *Opaque() { return opaque; }
  void SetPalette(const Palette &value) { palette = value; }
  void Clear() { ++clears; }
};
} // namespace BlackMarketRoomMagic
PlayState *gPlayState;
static bool interior = true, sGrannyActive = false;
bool Interior() { return interior && gPlayState; }
#ifndef OOT_ROOM_MAGIC_TEST
void TraceRoomMagic(const char *, RoomShape * = nullptr, bool = false) {}
#endif
/* PRODUCTION */
#ifdef OOT_ROOM_MAGIC_TEST
using Shape = MeshHeader;
using Entry = PolygonDlist;
static Shape *&ShapePointer(PlayState &play) {
  return play.roomCtx.curRoom.meshHeader;
}
static void SetShape(Shape &shape, Entry &entry) {
  shape.polygon0 = {{0}, 1, &entry, &entry + 1};
}
static Entry &GetEntry(Shape &shape) {
  return *static_cast<Entry *>(shape.polygon0.start);
}
static constexpr bool oot = true;
#else
using Shape = RoomShape;
using Entry = RoomShapeDListsEntry;
static Shape *&ShapePointer(PlayState &play) {
  return play.roomCtx.curRoom.roomShape;
}
static void SetShape(Shape &shape, Entry &entry) {
  shape.normal = {{ROOM_SHAPE_TYPE_NORMAL}, 1, &entry, &entry + 1};
}
static Entry &GetEntry(Shape &shape) { return *shape.normal.entries; }
static constexpr bool oot = false;
#endif
int main() {
  PlayState play{}, other{};
  gPlayState = &play;
  const std::string path =
      std::string("__OTR__") + BlackMarketRoomMagic::SourceRoom(oot);
  Gfx translucent[] = {gsSPEndDisplayList()};
  Entry entry{reinterpret_cast<Gfx *>(const_cast<char *>(path.c_str())),
              translucent};
  const Entry originalEntry = entry;
  Shape original{};
  SetShape(original, entry);
  const Shape before = original;
  ShapePointer(play) = &original;
  ShapePointer(other) = &original;
  const auto unchanged = [&] {
    assert(ShapePointer(play) == &original);
    assert(std::memcmp(&before, &original, sizeof(original)) == 0);
    assert(std::memcmp(&entry, &originalEntry, sizeof(entry)) == 0);
  };
  BeginRoomMagic();
  unchanged();
  assert(sRoomMagicResources.prepares == 0);
  integers[BlackMarketRoomMagic::kCvar] = 1;
  BeginRoomMagic();
  assert(ShapePointer(play) == &sRoomMagicShape &&
         sRoomMagicResources.prepares == 1);
  assert(GetEntry(*ShapePointer(play)).opa == sRoomMagicResources.Opaque());
  assert(GetEntry(*ShapePointer(play)).xlu == translucent);
  assert(sRoomMagicResources.host == (oot ? "oot" : "mm"));
  assert(sRoomMagicResources.source == originalEntry.opa);
  BeginRoomMagic(); // Duplicate hooks cannot save our own replacement as the
                    // original.
  RestoreRoomMagic();
  unchanged();
  const std::string altPath =
      std::string("__OTR__alt/") + BlackMarketRoomMagic::SourceRoom(oot);
  entry.opa = reinterpret_cast<Gfx *>(const_cast<char *>(altPath.c_str()));
  BeginRoomMagic();
  assert(ShapePointer(play) == &sRoomMagicShape &&
         sRoomMagicResources.source == entry.opa);
  RestoreRoomMagic();
  assert(ShapePointer(play) == &original &&
         entry.opa == reinterpret_cast<const Gfx *>(altPath.c_str()));
  entry = originalEntry;
  unchanged();
  assert(sRoomMagicResources.palette == BlackMarketRoomMagic::kDefaultPalette);
  for (size_t i = 0; i < 4; ++i) {
    const auto key = std::string(oot ? "gCosmetics." : "gCosmetic.") +
                     BlackMarketRoomMagic::kCosmeticKeys[i];
    colors[key + (oot ? ".Value" : ".Color")] = {
        21, static_cast<uint8_t>(31 + i), 41, 3};
    BeginRoomMagic();
    assert(sRoomMagicResources.palette[i] ==
           BlackMarketRoomMagic::kDefaultPalette[i]);
    RestoreRoomMagic();
    integers[key + ".Changed"] = 1;
    BeginRoomMagic();
    assert((sRoomMagicResources.palette[i] ==
            BlackMarketRoomMagic::Color{21, static_cast<uint8_t>(31 + i), 41,
                                        255}));
    RestoreRoomMagic();
    // Native rainbow updates this same host value between frames.
    colors[key + (oot ? ".Value" : ".Color")].r = 42;
    BeginRoomMagic();
    assert(sRoomMagicResources.palette[i][0] == 42);
    RestoreRoomMagic();
    integers.erase(key + ".Changed");
    BeginRoomMagic();
    assert(sRoomMagicResources.palette[i] ==
           BlackMarketRoomMagic::kDefaultPalette[i]);
    RestoreRoomMagic();
  }
  colors[oot ? "gCosmetics.World.BlackMarketShelves.Value"
             : "gCosmetic.World.BlackMarketShelves.Color"] = {1, 2, 3, 4};
  BeginRoomMagic();
  assert(sRoomMagicResources.palette == BlackMarketRoomMagic::kDefaultPalette);
  RestoreRoomMagic();
  integers[BlackMarketRoomMagic::kCvar] = 0;
  BeginRoomMagic();
  unchanged();
  integers[BlackMarketRoomMagic::kCvar] = 1;
  sGrannyActive = true;
  BeginRoomMagic();
  unchanged();
  sGrannyActive = false;
  sRoomMagicResources.available = false;
  BeginRoomMagic();
  unchanged();
  sRoomMagicResources.available = true;
  interior = false;
  BeginRoomMagic();
  unchanged();
  interior = true;
  play.roomCtx.curRoom.num = 1;
  BeginRoomMagic();
  unchanged();
  play.roomCtx.curRoom.num = 0;
  entry.opa = translucent;
  BeginRoomMagic();
  assert(ShapePointer(play) == &original);
  entry = originalEntry;
  BeginRoomMagic();
  ClearRoomMagic();
  unchanged();
  assert(sRoomMagicResources.clears > 0 && !sRoomMagicPlay &&
         !sRoomMagicOriginal);
  BeginRoomMagic();
  gPlayState = &other;
  RestoreRoomMagic(); // Never restore the old play's pointer through a new
                      // play.
  assert(ShapePointer(other) == &original && !sRoomMagicPlay &&
         !sRoomMagicOriginal);
  ShapePointer(play) = &original;
  gPlayState = &play;
  BeginRoomMagic();
  Shape newer{};
  ShapePointer(play) = &newer;
  RestoreRoomMagic();
  assert(ShapePointer(play) == &newer);
  gPlayState = nullptr;
  ClearRoomMagic();
  assert(!sRoomMagicPlay && !sRoomMagicOriginal);
  puts("PASS: native host single-room attachment; exact restoration; live "
       "independent color/reset/rainbow; shelf isolation; Granny precedence; "
       "fallback; scene/room transitions; teardown");
}
