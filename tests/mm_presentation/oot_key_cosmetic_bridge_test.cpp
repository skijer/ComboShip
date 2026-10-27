#include "combo/menu/ComboItemDrawABI.h"
#include "combo/menu/ComboCollectibleShimmer.h"
#include "combo/BlackMarketShelves.h"
#include "objects/object_gi_bosskey/object_gi_bosskey.h"
#include "objects/object_gi_key/object_gi_key.h"
#include "soh/assets/soh_assets.h"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#define RANDO_ENUM_BEGIN(x) enum x {
#define RANDO_ENUM_ITEM(x) x,
#define RANDO_ENUM_END(x)                                                      \
  }                                                                            \
  ;
#include "soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerGet.h"
struct Color_RGB8 {
  uint8_t r, g, b;
};
struct Color_RGBA8 {
  uint8_t r, g, b, a;
};
struct ImVec4 {
  float x, y, z, w;
};
constexpr Color_RGBA8 ColorRGBA8(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  return {r, g, b, a};
}
#define CVAR_COSMETIC(x) "gCosmetics." x
#define CVAR_RANDOMIZER_ENHANCEMENT(x) "gRandoEnhancements." x
std::map<std::string, int> integers;
std::map<std::string, Color_RGBA8> colors;
float rainbowSpeed = .6f;
int CVarGetInteger(const char *key, int fallback) {
  auto it = integers.find(key);
  return it == integers.end() ? fallback : it->second;
}
float CVarGetFloat(const char *, float) { return rainbowSpeed; }
Color_RGB8 CVarGetColor24(const char *key, Color_RGB8 fallback) {
  auto it = colors.find(key);
  return it == colors.end()
             ? fallback
             : Color_RGB8{it->second.r, it->second.g, it->second.b};
}
Color_RGBA8 CVarGetColor(const char *key, Color_RGBA8 fallback) {
  auto it = colors.find(key);
  return it == colors.end() ? fallback : it->second;
}
void CVarSetColor(const char *key, Color_RGBA8 color) { colors[key] = color; }
/* EDITOR_DECLARATIONS */
/* EDITOR_OPTIONS */
int hue = 0, nativePatches = 0;
namespace FrameTiming {
struct Scope {
  Scope(int) {}
};
} // namespace FrameTiming
#define FRAME_TIMING_COSMETICS 0
#define COMBO_EXPORT
void UpdateCustomCosmeticsRainbow(int, float, int &) {}
void ApplyOrResetCustomGfxPatches(bool) { ++nativePatches; }
void ApplyCustomCosmetics() { ++nativePatches; }
/* EDITOR_TICK */
/* EDITOR_SAMPLER */
/* LIVE_WRAPPER */
/* KEY_PALETTE_TABLES */
namespace Rando {
enum DungeonKey {
  FOREST_TEMPLE = 1,
  FIRE_TEMPLE,
  WATER_TEMPLE,
  SPIRIT_TEMPLE,
  SHADOW_TEMPLE,
  BOTTOM_OF_THE_WELL,
  GERUDO_TRAINING_GROUND,
  GANONS_CASTLE
};
struct Dungeon {
  bool IsMQ() { return false; }
};
struct Context {
  static Context *GetInstance() {
    static Context context;
    return &context;
  }
  Dungeon *GetDungeon(DungeonKey) {
    static Dungeon dungeon;
    return &dungeon;
  }
};
} // namespace Rando
/* OWNER_KEY_RECIPES */
/* HOST_INFO */
using RandoCheckId = int;
constexpr int RC_UNKNOWN = -1;
struct {
  int fileNum = 0;
} gSaveContext;
namespace ComboRando {
constexpr int GAME_OOT = 0;
struct ForeignItem {
  int itemGame = GAME_OOT;
  std::string itemName = "key", fakeItemName;
  bool HasDisguise() const { return false; }
};
} // namespace ComboRando
namespace Rando::MiscBehavior {
const ComboRando::ForeignItem *MM_LookupForeign(int) {
  static ComboRando::ForeignItem item;
  return &item;
}
uint64_t ComboRandoGen() { return 1; }
} // namespace Rando::MiscBehavior
RandomizerGet selected = RG_BOTTOM_OF_THE_WELL_SMALL_KEY;
int32_t Describe(const char *, CwItemDrawInfo *out) {
  return DescribeKeys(selected, out);
}
void *Combo_ResolveSym(const char *, const char *name) {
  if (!strcmp(name, "OOT_GetItemDrawInfo"))
    return (void *)Describe;
  if (!strcmp(name, "OOT_SetGiCosmeticFrame"))
    return (void *)OOT_SetGiCosmeticFrame;
  return nullptr;
}
const char *ComboInternRoutedPathOOT(const std::string &path) {
  static std::set<std::string> pool;
  return pool.insert(path).first->c_str();
}
enum class ComboForeignResolveOOT { Ok, Unknown, NotReady };
struct Gfx {
  int stream;
};
Gfx opa[100], xlu[100];
struct GraphicsContext {
  Gfx *o = opa;
  Gfx *x = xlu;
} gfx;
struct PlayState {
  struct {
    GraphicsContext *gfxCtx = &gfx;
  } state;
  int gameplayFrames = 100;
} play;
PlayState *gPlayState = &play;
/* HOST_RESOLVER */
/* HOST_CACHE */
std::vector<std::pair<int, std::string>> submitted;
std::vector<std::pair<int, std::vector<int>>> envColors, grayColors;
std::vector<std::pair<int, bool>> grayEnabled;
std::vector<bool> submittedGray;
bool grayActive = false;
#define OPEN_DISPS(g) ((void)(g))
#define CLOSE_DISPS(g) ((void)(g))
#define POLY_OPA_DISP gfx.o
#define POLY_XLU_DISP gfx.x
#define MM_FOREIGN_PIN_OPA() ((void)0)
#define MM_FOREIGN_PIN_XLU() ((void)0)
#define MATRIX_FINALIZE_AND_LOAD(p, g) ((void)(p))
#define MTXMODE_APPLY 1
#define gSPDisplayList(p, dl)                                                  \
  do { submitted.emplace_back((p)->stream, (const char *)(dl)); submittedGray.push_back(grayActive); } while (0)
#define gDPSetEnvColor(p, r, g, b, a)                                          \
  envColors.push_back({(p)->stream, {r, g, b, a}})
#define gDPSetPrimColor(p, ...) ((void)(p))
#define gDPSetGrayscaleColor(p, r, g, b, a)                                    \
  grayColors.push_back({(p)->stream, {r, g, b, a}})
#define gSPGrayscale(p, enabled) do { grayActive = enabled; grayEnabled.push_back({(p)->stream, enabled}); } while (0)
void Matrix_Scale(float, float, float, int) {}
void Gfx_SetupDL25_Opa(GraphicsContext *) {}
void Gfx_SetupDL25_Xlu(GraphicsContext *) {}
/* HOST_GRAY_DRAW */
/* HOST_COLOR_DRAW */
std::vector<int> Rgb(const uint8_t *color) {
  return {color[0], color[1], color[2]};
}
void ResetDraw() {
  gfx.o = opa;
  gfx.x = xlu;
  submitted.clear();
  submittedGray.clear();
  grayActive = false;
  envColors.clear();
  grayColors.clear();
  grayEnabled.clear();
}
int main() {
  for (auto &cmd : opa)
    cmd.stream = 0;
  for (auto &cmd : xlu)
    cmd.stream = 1;
  colors["gCosmetics.Key.WellSmallBody.Value"] = {10, 20, 30, 255};
  colors["gCosmetics.Key.WellSmallEmblem.Value"] = {40, 50, 60, 255};
  colors["gCosmetics.Key.ShadowSmallEmblem.Value"] = {90, 80, 70, 255};
  CwItemDrawInfo key{};
  assert(DescribeKeys(RG_BOTTOM_OF_THE_WELL_SMALL_KEY, &key) == 1 &&
         key.stateDependent == 2);
  assert(Rgb(key.layerEnvColor[0]) == std::vector<int>({10, 20, 30}));
  assert(Rgb(key.layerEnvColor[1]) == std::vector<int>({40, 50, 60}));
  ComboLatchForeignDrawOOT(1);
  colors["gCosmetics.Key.WellSmallEmblem.Value"] = {200, 10, 20, 255};
  auto live = ComboResolveForeignDrawInfoOOT(1);
  assert(live &&
         Rgb(live->layerEnvColor[1]) == std::vector<int>({200, 10, 20}));
  selected = RG_SHADOW_TEMPLE_SMALL_KEY;
  ComboForeignDrawInfoOOT shadow;
  assert(ComboFillForeignDrawInfoOOT(1, shadow) == ComboForeignResolveOOT::Ok);
  assert(Rgb(shadow.layerEnvColor[1]) == std::vector<int>({90, 80, 70}));
  integers["gCosmetics.Key.WellSmallEmblem.Rainbow"] = 1;
  hue = 7;
  CosmeticsUpdateTick();
  const auto expected = colors["gCosmetics.Key.WellSmallEmblem.Value"];
  hue = 7;
  nativePatches = 0;
  OOT_SetGiCosmeticFrame(300);
  selected = RG_BOTTOM_OF_THE_WELL_SMALL_KEY;
  key = {};
  DescribeKeys(selected, &key);
  assert(Rgb(key.layerEnvColor[1]) ==
         std::vector<int>({expected.r, expected.g, expected.b}));
  const auto untouchedColors = colors;
  const auto before = Rgb(key.layerEnvColor[1]);
  OOT_SetGiCosmeticFrame(301);
  key = {};
  DescribeKeys(selected, &key);
  assert(Rgb(key.layerEnvColor[1]) != before && nativePatches == 0);
  const auto after = Rgb(key.layerEnvColor[1]);
  OOT_SetGiCosmeticFrame(301);
  key = {};
  DescribeKeys(selected, &key);
  assert(Rgb(key.layerEnvColor[1]) == after);
  assert(colors["gCosmetics.Key.WellSmallEmblem.Value"].r ==
         untouchedColors.at("gCosmetics.Key.WellSmallEmblem.Value").r);
  selected = RG_BOTTOM_OF_THE_WELL_KEY_RING;
  key = {};
  assert(DescribeKeys(selected, &key) == 1 && key.dlistCount == 3 &&
         key.layerEnvMask == 7 && key.stateDependent == 2);
  assert(Rgb(key.layerEnvColor[2]) == after);
  integers["gRandoEnhancements.CustomKeyModels"] = 0;
  key = {};
  assert(DescribeKeys(selected, &key) == 1 && key.layerPrimMask == 1);
  ComboForeignDrawInfoOOT vanilla;
  assert(ComboFillForeignDrawInfoOOT(1, vanilla) == ComboForeignResolveOOT::Ok);
  ResetDraw();
  MM_DrawForeignGrayscaleLayers(&vanilla);
  assert(submitted.size() == 1 && grayColors.size() == 1 &&
         grayEnabled.back().second == false);
  selected = RG_BOTTOM_OF_THE_WELL_KEY_RING;
  key = {};
  assert(DescribeKeys(selected, &key) == 1 && key.dlistCount == 1 &&
         key.layerPrimMask == 1);
  selected = RG_SHADOW_TEMPLE_BOSS_KEY;
  key = {};
  assert(DescribeKeys(selected, &key) == 1 && key.layerPrimMask == 0);
  integers["gCosmetics.Key.ShadowBossBody.Changed"] = 1;
  colors["gCosmetics.Key.ShadowBossBody.Value"] = {1, 2, 3, 255};
  key = {};
  assert(DescribeKeys(selected, &key) == 1 && key.layerPrimMask == 1);
  integers["gCosmetics.Key.ShadowBossGem.Changed"] = 1;
  key = {};
  assert(DescribeKeys(selected, &key) == 1 && key.layerPrimMask == 3);
  ComboForeignDrawInfoOOT boss;
  assert(ComboFillForeignDrawInfoOOT(1, boss) == ComboForeignResolveOOT::Ok);
  ResetDraw();
  MM_DrawForeignGrayscaleLayers(&boss);
  assert(submitted.size() == 2 && grayColors.size() == 2 &&
         grayColors[0].first == 0 && grayColors[1].first == 1);
  assert((grayEnabled == std::vector<std::pair<int, bool>>(
                             {{0, true}, {0, false}, {1, true}, {1, false}})));
  integers["gRandoEnhancements.CustomKeyModels"] = 1;
  key = {};
  assert(DescribeKeys(selected, &key) == 1 && key.layerEnvMask == 3 &&
         key.layerPrimMask == 0);
  // Custom heart border/body are both XLU; tint only the body and reset.
  ComboForeignDrawInfoOOT heart{};
  heart.count = 2;
  heart.xluStart = 0;
  heart.dls[0] = "@oot/__OTR__border";
  heart.dls[1] = "@oot/__OTR__body";
  heart.layerPrimMask = 2;
  heart.layerPrimColor[1][0] = 53;
  heart.layerPrimColor[1][1] = 167;
  heart.layerPrimColor[1][2] = 225;
  heart.layerPrimColor[1][3] = 255;
  ResetDraw();
  MM_DrawForeignGrayscaleLayers(&heart);
  assert((submitted == std::vector<std::pair<int, std::string>>(
      {{1, "@oot/__OTR__border"}, {1, "@oot/__OTR__body"}})));
  assert(submittedGray == std::vector<bool>({false, true}));
  assert((grayColors == std::vector<std::pair<int, std::vector<int>>>(
      {{1, {53, 167, 225, 255}}})));
  assert((grayEnabled == std::vector<std::pair<int, bool>>(
      {{1, false}, {1, false}, {1, true}, {1, false}})) && !grayActive);
  // Compare wrap and synchronized phases against the actual native editor tick.
  rainbowSpeed = .1f;
  hue = 35;
  CosmeticsUpdateTick();
  const auto expectedLast = colors["gCosmetics.Key.WellSmallEmblem.Value"];
  hue = 35;
  nativePatches = 0;
  OOT_SetGiCosmeticFrame(400);
  auto sampled =
      CwLiveCosmeticColor("gCosmetics.Key.WellSmallEmblem.Value", {});
  assert(sampled.r == expectedLast.r && sampled.g == expectedLast.g &&
         sampled.b == expectedLast.b);
  OOT_SetGiCosmeticFrame(401);
  sampled = CwLiveCosmeticColor("gCosmetics.Key.WellSmallEmblem.Value", {});
  assert(nativePatches == 0);
  hue = 0;
  CosmeticsUpdateTick();
  const auto expectedWrapped = colors["gCosmetics.Key.WellSmallEmblem.Value"];
  assert(sampled.r == expectedWrapped.r && sampled.g == expectedWrapped.g &&
         sampled.b == expectedWrapped.b);
  integers["gCosmetics.RainbowSync"] = 1;
  integers["gCosmetics.Key.ShadowSmallEmblem.Rainbow"] = 1;
  hue = 9;
  CosmeticsUpdateTick();
  const auto expectedSynced = colors["gCosmetics.Key.WellSmallEmblem.Value"];
  hue = 9;
  nativePatches = 0;
  OOT_SetGiCosmeticFrame(450);
  const auto well =
      CwLiveCosmeticColor("gCosmetics.Key.WellSmallEmblem.Value", {});
  const auto shadowSync =
      CwLiveCosmeticColor("gCosmetics.Key.ShadowSmallEmblem.Value", {});
  assert(well.r == shadowSync.r && well.g == shadowSync.g &&
         well.b == shadowSync.b);
  assert(well.r == expectedSynced.r && well.g == expectedSynced.g &&
         well.b == expectedSynced.b && nativePatches == 0);
  std::cout << "PASS production OoT key cosmetics: Well/Shadow hex, live "
               "latch, editor rainbow phase and native grayscale\n";
}
