// Execute the Ikana descriptor, real OoT export, and MM cache/latch lifecycle.
#include "combo/menu/ComboIkanaShieldResources.h"
#include "combo/menu/ComboItemDrawABI.h"
#include "combo/menu/ComboCollectibleShimmer.h"
extern "C" int ResourceMgr_IsIkanaShieldGiAvailable() { return 1; }
#include "soh/soh/Enhancements/randomizer/NeiGiFrameFit.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#define COMBO_EXPORT
#define RANDO_ENUM_BEGIN(x) enum x {
#define RANDO_ENUM_ITEM(x) x,
#define RANDO_ENUM_END(x)                                                      \
  }                                                                            \
  ;
#include "soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerGet.h"
namespace ItemGrantAudit {
struct Scope {
  Scope(const char *, int, int, bool) {}
};
} // namespace ItemGrantAudit
namespace Rando::StaticData {
std::unordered_map<std::string, RandomizerGet> itemNameToEnum{
    {"Shield of Ikana", RG_EXT_SHIELD_OF_IKANA}};
}
struct OTRGlobals {
  static inline OTRGlobals *Instance = nullptr;
  void *gRandomizer;
  void *gRandoContext;
};
static bool OOT_BossSoulUsesSkeleton(RandomizerGet) { return false; }
bool donorAlt = false;
int poseQueries = 0;
int ResourceMgr_IsModAssetForGame(const char *, const char *) { return 0; }
int ResourceMgr_GetIkanaShieldGiPoseForGame(const char *owner, const char *,
                                            float *tilt, float *roll) {
  assert(!strcmp(owner, "mm"));
  ++poseQueries;
  *tilt = donorAlt ? 0.f : 1.5707963267948966f;
  *roll = donorAlt ? -1.5707963267948966f : 0.f;
  return 1;
}
/* OWNER_DESCRIPTOR */
/* OWNER_EXPORT */
namespace ComboRando {
constexpr int GAME_OOT = 0;
struct ForeignItem {
  int itemGame = GAME_OOT;
  std::string itemName = "Shield of Ikana", fakeItemName;
  bool HasDisguise() const { return false; }
} item;
} // namespace ComboRando
namespace Rando::MiscBehavior {
const ComboRando::ForeignItem *MM_LookupForeign(int) {
  return &ComboRando::item;
}
uint64_t ComboRandoGen() { return 1; }
} // namespace Rando::MiscBehavior
using RandoCheckId = int;
constexpr int RC_UNKNOWN = -1;
struct {
  int fileNum = 0;
} gSaveContext;
struct {
  uint32_t gameplayFrames = 42;
} fakePlay;
auto *gPlayState = &fakePlay;
bool ResourceMgr_IsAltAssetsEnabled() { return false; }
int ComboNativeMmImport(const char *) { return -1; }
void *Combo_ResolveSym(const char *, const char *name) {
  return !strcmp(name, "OOT_GetItemDrawInfo")
             ? reinterpret_cast<void *>(OOT_GetItemDrawInfo)
             : nullptr;
}
const char *ComboInternRoutedPathOOT(const std::string &path) {
  static std::unordered_set<std::string> pool;
  return pool.insert(path).first->c_str();
}
enum class ComboForeignResolveOOT { Ok, Unknown, NotReady };
#include "combo/menu/ComboSwordGiAssetSelection.h"
/* FOREIGN_RESOLVER */
/* FOREIGN_CACHE */
void CheckPose(const ComboForeignDrawInfoOOT *info) {
  assert(info && info->appearanceDependent && info->stateDependent &&
         info->opCount == 1);
  assert(info->drawKind == CW_DRAW_KIND_SIMPLE && info->count == 2 &&
         info->xluStart == 1);
  assert(info->ops[0].op == (donorAlt ? CW_OP_ROTATE_Z : CW_OP_ROTATE_X));
  assert(info->ops[0].a == (donorAlt ? -16384.f : 16384.f));
  assert(!strcmp(info->dls[0], COMBO_IKANA_GI_OPA_MM));
  assert(!strcmp(info->dls[1], COMBO_IKANA_GI_XLU_MM));
  assert(info->scale == 0.f);
}

int main() {
  OTRGlobals globals{&globals, &globals};
  OTRGlobals::Instance = &globals;
  CwItemDrawInfo raw{};
  assert(OOT_GetItemDrawInfo("Shield of Ikana", &raw) == 1 &&
         raw.stateDependent == 2);
  for (bool alt : {false, true, false}) {
    donorAlt = alt;
    CheckPose(ComboResolveForeignDrawInfoOOT(17));
  }
  ComboLatchForeignDrawOOT(17);
  const int queriesAtLatch = poseQueries;
  for (bool alt : {true, false, true}) {
    donorAlt = alt;
    CheckPose(ComboResolveForeignDrawInfoOOT(17));
  }
  assert(poseQueries == queriesAtLatch + 3 &&
         "latched appearance must refresh the donor pose every frame");
  std::cout << "PASS actual OoT Ikana export/MM cache: live owner Alt pose "
               "before and after acquisition latch\n";
}
