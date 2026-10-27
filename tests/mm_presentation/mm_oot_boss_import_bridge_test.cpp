#include "combo/menu/ComboItemDrawABI.h"
#include "combo/menu/ComboCollectibleShimmer.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>

enum RandoItemId {
  RI_NONE,
  RI_SOUL_OOT_BOSS_GOHMA,
  RI_SOUL_OOT_BOSS_KING_DODONGO,
  RI_SOUL_OOT_BOSS_BARINADE,
  RI_SOUL_OOT_BOSS_PHANTOM_GANON,
  RI_SOUL_OOT_BOSS_VOLVAGIA,
  RI_SOUL_OOT_BOSS_MORPHA,
  RI_SOUL_OOT_BOSS_BONGO_BONGO,
  RI_SOUL_OOT_BOSS_TWINROVA,
  RI_SOUL_OOT_BOSS_GANON,
};
using RandoCheckId = int;
constexpr int RC_UNKNOWN = -1;
/* DONOR_NAMES */
namespace ComboRando {
constexpr int GAME_OOT = 0;
struct ForeignItem {
  int itemGame = GAME_OOT;
  std::string itemName, fakeItemName;
  bool HasDisguise() const { return !fakeItemName.empty(); }
};
} // namespace ComboRando
int lookups = 0;
namespace Rando::MiscBehavior {
const ComboRando::ForeignItem *MM_LookupForeign(int) {
  ++lookups;
  return nullptr;
}
} // namespace Rando::MiscBehavior
namespace Ship {
struct ResourceManager {};
bool ownerReady = true;
namespace CrossRMRegistry {
std::shared_ptr<ResourceManager> Get(const char *game) {
  assert(!strcmp(game, "oot"));
  static auto owner = std::make_shared<ResourceManager>();
  return ownerReady ? owner : nullptr;
}
} // namespace CrossRMRegistry
struct ResourceManagerScope {
  explicit ResourceManagerScope(std::shared_ptr<ResourceManager>) {}
};
} // namespace Ship
struct GraphicsContext {
} gfx;
struct PlayState {
  struct {
    GraphicsContext *gfxCtx = &gfx;
  } state;
  int gameplayFrames = 37;
} play;
PlayState *gPlayState = &play;
std::vector<std::string> queried;
int selected = -1, donorResult = 1;
bool simpler = false, animationReady = true, symbolsReady = true;
int32_t Describe(const char *name, CwItemDrawInfo *out) {
  queried.emplace_back(name);
  selected = -1;
  for (int i = 0; i < 9; ++i)
    if (!strcmp(name, names[i]))
      selected = i;
  if (selected < 0 || donorResult != 1)
    return donorResult == 1 ? 0 : donorResult;
  if (!simpler && selected != 5)
    return 0;
  out->drawKind =
      simpler ? CW_DRAW_KIND_BOSS_SOUL : CW_DRAW_KIND_OOT_MORPHA_SOUL;
  out->dlistCount = simpler ? 2 : 3;
  out->xluStartIndex = 0;
  out->dlists[0] = "__OTR__flame";
  out->dlists[1] = "__OTR__membrane";
  out->dlists[2] = "__OTR__nucleus";
  return 1;
}
int32_t DescribeAnim(const char *name, CwItemAnimDrawInfo *out) {
  assert(selected >= 0 && queried.back() == name && selected != 5);
  if (!animationReady)
    return 0;
  out->skelPath = "__OTR__selected_owner_skeleton";
  out->animPath = "__OTR__selected_owner_animation";
  out->limbCount = 27;
  return 1;
}
void *Combo_ResolveSym(const char *module, const char *name) {
  assert(!strcmp(module, "soh"));
  if (!symbolsReady)
    return nullptr;
  if (!strcmp(name, "OOT_GetItemDrawInfo"))
    return (void *)Describe;
  if (!strcmp(name, "OOT_GetItemAnimDrawInfo"))
    return (void *)DescribeAnim;
  return nullptr;
}
const char *ComboInternRoutedPathOOT(const std::string &path) {
  static std::set<std::string> paths;
  return paths.insert(path).first->c_str();
}
enum class ComboForeignResolveOOT { Ok, Unknown, NotReady };
/* HOST_INFO */
/* HOST_RESOLVER */
int pose = 123;
std::vector<int> poses;
void Matrix_Push() { poses.push_back(pose); }
void Matrix_Pop() { assert(!poses.empty()); pose = poses.back(); poses.pop_back(); }
int drawnKind = -1, animationDraws = 0;
bool animationDrawSucceeds = true;
bool ComboForeignAnim_Draw(const CwItemAnimDrawInfo *info, const char *owner,
                           PlayState *state) {
  assert(info->limbCount == 27 && !strcmp(owner, "oot") && state == &play);
  ++animationDraws;
  pose += 100;
  return animationDrawSucceeds;
}
void MM_DrawForeignBossSoul(const ComboForeignDrawInfoOOT *info) {
  drawnKind = info->drawKind;
  pose += 25;
  assert(info->count == 2 && !strcmp(info->dls[0], "__OTR__@oot:flame"));
}
void MM_DrawForeignMorphaSoul(const ComboForeignDrawInfoOOT *info) {
  drawnKind = info->drawKind;
  pose += 25;
  assert(info->count == 3 && !strcmp(info->dls[1], "__OTR__@oot:membrane") &&
         !strcmp(info->dls[2], "__OTR__@oot:nucleus"));
}
/* HOST_IMPORT_HELPER */
using u8 = uint8_t;
struct Gfx {};
Gfx commands[100];
Gfx *cursor = commands;
int legacyFlames = 0;
Gfx *ResourceMgr_LoadGfxByName(const char *) { return commands; }
void DrawOotSoulFlame(PlayState *, const uint8_t *, const float *,
                      const float *) {
  ++legacyFlames;
}
void Gfx_SetupDL25_Xlu(GraphicsContext *) {}
#define OPEN_DISPS(g) ((void)(g))
#define CLOSE_DISPS(g) ((void)(g))
#define POLY_XLU_DISP cursor
#define MATRIX_FINALIZE_AND_LOAD(p, g) ((void)(p))
#define gDPSetEnvColor(p, ...) ((void)(p))
#define gSPDisplayList(p, dl) ((void)(p), (void)(dl))
/* NATIVE_IMPORT_DRAW */
int main() {
  symbolsReady = false;
  assert(!MM_TryDrawOotBossSoul(RI_SOUL_OOT_BOSS_MORPHA));
  assert(queried.empty());
  symbolsReady = true;
  for (int i = 0; i < 9; ++i) {
    drawnKind = -1;
    animationDraws = 0;
    DrawOotBossSoul(static_cast<RandoItemId>(i + 1));
    assert(pose == 123 && poses.empty());
    assert(legacyFlames == 0 && queried.size() == static_cast<size_t>(i + 1));
    assert(queried.back() == names[i] && lookups == 0);
    if (i == 5)
      assert(drawnKind == CW_DRAW_KIND_OOT_MORPHA_SOUL && drawnKind == 32 &&
             animationDraws == 0);
    else
      assert(animationDraws == 1 && drawnKind == -1);
  }
  simpler = true;
  for (int i = 0; i < 9; ++i) {
    drawnKind = -1;
    animationDraws = 0;
    assert(MM_TryDrawOotBossSoul(static_cast<RandoItemId>(i + 1)));
    assert(drawnKind == CW_DRAW_KIND_BOSS_SOUL && animationDraws == 0 &&
           queried.back() == names[i]);
  }
  const auto beforeUnknown = queried.size();
  assert(!MM_TryDrawOotBossSoul(RI_NONE) && queried.size() == beforeUnknown);
  gPlayState = nullptr;
  assert(!MM_TryDrawOotBossSoul(RI_SOUL_OOT_BOSS_MORPHA));
  gPlayState = &play;
  Ship::ownerReady = false;
  assert(!MM_TryDrawOotBossSoul(RI_SOUL_OOT_BOSS_MORPHA));
  DrawOotBossSoul(RI_SOUL_OOT_BOSS_MORPHA);
  assert(legacyFlames == 1);
  Ship::ownerReady = true;
  donorResult = CW_DRAW_NOT_READY;
  assert(!MM_TryDrawOotBossSoul(RI_SOUL_OOT_BOSS_MORPHA));
  donorResult = 1;
  assert(MM_TryDrawOotBossSoul(RI_SOUL_OOT_BOSS_MORPHA));
  simpler = false;
  animationReady = false;
  assert(!MM_TryDrawOotBossSoul(RI_SOUL_OOT_BOSS_TWINROVA));
  animationReady = true;
  animationDrawSucceeds = false;
  assert(!MM_TryDrawOotBossSoul(RI_SOUL_OOT_BOSS_BARINADE));
  assert(pose == 123 && poses.empty());
  animationDrawSucceeds = true;
  assert(MM_TryDrawOotBossSoul(RI_SOUL_OOT_BOSS_BARINADE));
  assert(lookups == 0);
  std::cout
      << "PASS production imported OoT boss souls: nine names, no check "
         "lookup, Morpha32, animated sisters/Barinade, simpler and fallback\n";
}
