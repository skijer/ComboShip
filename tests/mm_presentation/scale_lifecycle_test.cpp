// Diagnostic bridge is a no-op in this host-service fixture (included before
// COMBO_BUILD).
#include "combo/menu/ItemGrantAuditBridge.h"
// Production scale resolver + foreign queue + MM save writer/loader.
// Save schema, host engine services and target persistence are narrowed test
// boundaries.
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include "combo/rando/CrossDungeonRuntime.h"
#define COMBO_BUILD
#include "combo/ComboGameMode.h"
extern "C" { int gComboGameMode = COMBO_GAME_BOTH; }
#define SPDLOG_INFO(...) ((void)0)
#define SPDLOG_WARN(...) ((void)0)
#define SPDLOG_ERROR(...) ((void)0)
using s16 = int16_t;
enum RandomizerGet {
  RG_NONE,
  RG_PROGRESSIVE_SCALE,
  RG_BRONZE_SCALE,
  RG_SILVER_SCALE,
  RG_GOLDEN_SCALE
};
constexpr int RAND_INF_CAN_SWIM = 0, UPG_SCALE = 0, RI_COMBO_FOREIGN = 1;
using RandoCheckId = int;
constexpr RandoCheckId RC_UNKNOWN = -1;
RandoCheckId queuedCheck = RC_UNKNOWN;
constexpr RandoCheckId RC_CLOCK_TOWN_WEST_BANK_ADULTS_WALLET = 100,
                       RC_CLOCK_TOWN_WEST_BANK_INTEREST = 101,
                       RC_CLOCK_TOWN_WEST_BANK_PIECE_OF_HEART = 102;
struct Check {
  int randoItemId;
  bool obtained, cycleObtained, eligible;
};
struct Save {
  struct {
    int checksum;
    struct {
      int health;
    } playerData;
  } saveInfo;
  struct {
    int pauseSaveEntrance, respawn;
    struct {
      Check checks[2];
    } rando;
  } shipSaveInfo;
  bool isOwlSave;
};
struct SaveContext {
  Save save;
  s16 fileNum;
} gSaveContext{};
void to_json(nlohmann::json &j, const Save &s) {
  j = {{"saveInfo",
        {{"playerData", {{"health", s.saveInfo.playerData.health}}}}},
       {"isOwlSave", s.isOwlSave},
       {"shipSaveInfo",
        {{"pauseSaveEntrance", s.shipSaveInfo.pauseSaveEntrance},
         {"respawn", s.shipSaveInfo.respawn},
         {"rando", nlohmann::json::array()}}}};
  for (auto c : s.shipSaveInfo.rando.checks)
    j["shipSaveInfo"]["rando"].push_back(
        {c.randoItemId, c.obtained, c.cycleObtained, c.eligible});
}
void from_json(const nlohmann::json &j, Save &s) {
  s.saveInfo.playerData.health = j.at("saveInfo").at("playerData").at("health");
  s.isOwlSave = j.at("isOwlSave");
  for (int i = 0; i < 2; ++i) {
    auto c = j.at("shipSaveInfo").at("rando").at(i);
    s.shipSaveInfo.rando.checks[i] = {c.at(0), c.at(1), c.at(2), c.at(3)};
  }
}
void to_json(nlohmann::json &j, const SaveContext &s) {
  j = {{"save", s.save}};
}
void from_json(const nlohmann::json &j, SaveContext &s) {
  s.save = j.at("save").get<Save>();
}
constexpr int GAMEOVER_INACTIVE = 0, CURRENT_SAVE_VERSION = 4;
struct PlayState {
  struct {
    int state = 0;
  } gameOverCtx;
};
PlayState *gPlayState = nullptr;
int gComboOwlBlobSlot = -1, mmWrites = 0;
static int32_t restoredCrossDungeonCheckpoint = -1;
extern "C" void Combo_MM_LoadCrossDungeonCheckpoint(int32_t index) {
  restoredCrossDungeonCheckpoint = index;
}
std::map<std::string, nlohmann::json> mmDisk;
std::string SaveManager_GetFileName(int slot) { return std::to_string(slot); }
int SaveManager_ReadSaveFile(const std::string &path, nlohmann::json &j) {
  auto it = mmDisk.find(path);
  if (it == mmDisk.end())
    return -1;
  j = it->second;
  return 0;
}
int SaveManager_MigrateSave(nlohmann::json &) { return 0; }
void SaveManager_WriteSaveFile(const std::string &path,
                               const nlohmann::json &j) {
  ++mmWrites;
  mmDisk[path] = j;
}
int SaveManager_LoadFailedForCombo(int rc) { return rc; }
int Sram_CalcChecksum(const void *, size_t) { return 0; }
// Shop persistence has its own native fixture; it does not alter foreign-scale
// delivery or this narrowed save schema.
void ComboBlackMarket_LoadState(const nlohmann::json &, int) {}
/* MM_SAVE */

int targetScale = 0, targetDiskScale = 0, targetGrants = 0, latches = 0,
    broadcasts = 0;
bool canSwim = true;
struct Logic {
  bool CheckRandoInf(int) { return canSwim; }
  int CurrentUpgrade(int) { return targetScale; }
} logicState;
RandomizerGet ResolveScale() {
  auto *logic = &logicState;
  RandomizerGet actual = RG_NONE;
  switch (static_cast<int>(RG_PROGRESSIVE_SCALE)) { /* SCALE_CASE */
  }
  return actual;
}
void GrantScale() {
  auto item = ResolveScale();
  ++targetGrants;
  if (item == RG_BRONZE_SCALE)
    canSwim = true;
  else
    targetScale = item == RG_SILVER_SCALE ? 1 : 2;
  targetDiskScale =
      targetScale; // SOH cross-grant's documented eager SaveFile boundary.
}
constexpr int RI_ABILITY_SWIM = 2, RI_SINGLE_MAGIC = 3, RI_DOUBLE_MAGIC = 4,
              RI_PROGRESSIVE_MAGIC = 5, FCI_SCALE = 9, FCI_NO_ITEM = 0,
              FC_COMBO_ITEM_COUNT = 10, COMBO_RPG_MAGIC = 0;
int sReceiveDepth = 0;
bool ComboRpg_IsEnabled(int) { return false; }
int ChainAliasFor(int) { return 0; }
int FcCombo_ItemForNative(int native) {
  return native == RI_ABILITY_SWIM ? FCI_SCALE : FCI_NO_ITEM;
}
struct FcItem {
  const char *ootName;
};
FcItem gFcComboItems[FC_COMBO_ITEM_COUNT]{};
using FnGrantSharedItem = void (*)(const char *);
FnGrantSharedItem ResolvePeerGrant() {
  return [](const char *) { GrantScale(); };
}
int nativeMagicFloor = 0;
void ApplyNativeMagicFloor(int tier) {
  if (tier > nativeMagicFloor)
    nativeMagicFloor = tier;
}
void* Combo_ResolveSym(const char* module, const char* symbol) {
  assert(std::strcmp(module, "soh") == 0);
  assert(std::strcmp(symbol, "SOH_ApplySharedMagicFloor") == 0);
  return reinterpret_cast<void*>(ApplyNativeMagicFloor);
}
/* SHARE_SWIM */
namespace ComboRando {
constexpr int GAME_MM = 1, GAME_OOT = 0;
struct ForeignItem {
  int itemGame = 0;
  bool trap = false;
  std::string itemName = "Progressive Scale";
};
std::string ShownForeignName(const ForeignItem &item, const char *name) {
  return name ? name : item.itemName;
}
std::map<std::string, ForeignItem> LoadForeignForGame(int, int) {
  return {{"chest", {}}};
}
} // namespace ComboRando
std::map<std::string, ComboRando::ForeignItem> g_mmForeignMap = {{"chest", {}}};
uint64_t g_mmForeignGen = 0, g_comboRandoGen = 0;
void (*gMMComboCrossDeliver)(int, const char *, const char *) =
    [](int, const char *, const char *) { GrantScale(); };
void MMAnchor_BroadcastCrossItem(int, const char *, const char *) {
  ++broadcasts;
}
struct Toast {
  std::string message, suffix;
};
namespace Notification {
void Emit(Toast) {}
} // namespace Notification
int CUSTOM_ITEM_PARAM = 0, CUSTOM_ITEM_PARAM2 = 0, CUSTOM_ITEM_FLAGS = 1;
namespace CustomItem {
constexpr int GIVE_ITEM_CUTSCENE = 1;
}
namespace CustomMessage {
struct Entry {
  int textboxType;
  uint8_t icon;
  std::string msg;
  bool autoFormat = true;
  bool capeVisibilityChoice = false;
};
std::string shown;
void SetActiveCustomMessage(std::string msg, Entry) { shown = msg; }
void StartTextbox(std::string msg, Entry) { shown = msg; }
} // namespace CustomMessage
std::string frozen;
namespace Rando {
// This lifecycle probe exercises the generic fallback; detailed donor receipts
// and their binary encoding are verified in tests/item_receipts.
bool ApplyForeignItemReceiptText(const char *, CustomMessage::Entry &, int) { return false; }
void AppendReceiptSource(CustomMessage::Entry &, const std::string &);
namespace StaticData {
std::string GetCheckDisplayName(int) { return "chest"; }
std::string GetItemName(int, bool, int) { return "Progressive Scale"; }
} // namespace StaticData
namespace MiscBehavior {
/* BANK_SOURCE */
const ComboRando::ForeignItem *MM_LookupForeign(int) {
  return &g_mmForeignMap.begin()->second;
}
bool ShouldShowForeignCutscene(int) { return true; }
void OfferTrapItem() {}
void SendForeignCheck(int);
} // namespace MiscBehavior
void LatchComboForeign(int) {
  ++latches;
  frozen = ResolveScale() == RG_SILVER_SCALE ? "Silver Scale" : "Golden Scale";
}
const char *ComboForeignLatchedName(int) {
  return frozen.empty() ? nullptr : frozen.c_str();
}
uint8_t ComboForeignMessageIcon(int) { return 0xF5; }
} // namespace Rando
/* RECEIPT_SOURCE */
/* SEND_FOREIGN */
bool queued = false;
std::string GetTrapMessage() { return "trap"; }
#define RANDO_SAVE_CHECKS gSaveContext.save.shipSaveInfo.rando.checks
void CollectForeign(int check) {
  using Rando::MiscBehavior::BankRewardSourceSuffix;
  CUSTOM_ITEM_PARAM = check;
  auto &randoSaveCheck = RANDO_SAVE_CHECKS[check];
  /* FOREIGN_QUEUE */
}
void ReloadWithoutManualSave() {
  gSaveContext = {};
  frozen.clear();
  targetScale = targetDiskScale;
  assert(SaveManager_LoadSaveFile(1) == 0);
  // Chest actors/cycle flags may reset on re-entry; permanent obtained must
  // survive.
  RANDO_SAVE_CHECKS[0].cycleObtained = false;
  RANDO_SAVE_CHECKS[0].eligible = true;
}
int main() {
  for (auto &check : RANDO_SAVE_CHECKS)
    check = {RI_COMBO_FOREIGN, false, false, true};
  gSaveContext.save.saveInfo.playerData.health = 0x30;
  assert(ResolveScale() == RG_SILVER_SCALE);
  CollectForeign(0);
  assert(targetScale == 1 && targetDiskScale == 1 && targetGrants == 1);
  assert(CustomMessage::shown == "You found Silver Scale!");
  assert(RANDO_SAVE_CHECKS[0].obtained && mmWrites == 1);
  ReloadWithoutManualSave();
  assert(RANDO_SAVE_CHECKS[0].obtained && targetScale == 1 && restoredCrossDungeonCheckpoint == -1);
  CollectForeign(0);
  assert(targetGrants == 1 && latches == 1 && broadcasts == 1 && mmWrites == 1);
  assert(
      CustomMessage::shown ==
      "You found Progressive Scale!"); // Existing replay presentation contract.
  CollectForeign(1);
  assert(targetScale == 2 && targetGrants == 2 &&
         CustomMessage::shown == "You found Golden Scale!");
  ReloadWithoutManualSave();
  CollectForeign(0);
  assert(targetScale == 2 && targetGrants == 2 && broadcasts == 2);
  // Existing native MM swim sharing is a prior progressive acquisition, not
  // this chest's grant.
  targetScale = targetDiskScale = 0;
  gFcComboItems[FCI_SCALE].ootName = "Progressive Scale";
  FleetShared_OnNativeObtained(RI_ABILITY_SWIM);
  assert(targetScale == 1 && ResolveScale() == RG_GOLDEN_SCALE);
  int priorGrants = targetGrants;
  FleetShared_OnNativeObtained(999);
  sReceiveDepth = 1;
  FleetShared_OnNativeObtained(RI_ABILITY_SWIM);
  assert(targetGrants == priorGrants);
  sReceiveDepth = 0;
  nativeMagicFloor = 0;
  int priorMagicGrants = targetGrants;
  FleetShared_OnNativeObtained(RI_SINGLE_MAGIC);
  FleetShared_OnNativeObtained(RI_DOUBLE_MAGIC);
  assert(nativeMagicFloor == 2 && targetGrants == priorMagicGrants);
  targetScale = 0;
  canSwim = false;
  assert(ResolveScale() == RG_BRONZE_SCALE);
  // Owl continue also retains source obtained flags instead of an old page
  // shadowing them.
  canSwim = true;
  gSaveContext = {};
  mmDisk.clear();
  gComboOwlBlobSlot = 1;
  nlohmann::json owl = gSaveContext;
  mmDisk["1"] = {{"owlSave", owl}};
  RANDO_SAVE_CHECKS[0].obtained = true;
  SaveManager_SaveCurrentForCombo();
  gSaveContext = {};
  assert(SaveManager_LoadSaveFile(1) == 0 && RANDO_SAVE_CHECKS[0].obtained);
  std::cout << "PASS production scale selection and foreign queue/save "
               "lifecycle: Silver->Gold, replay/reload idempotency, prior swim "
               "grant and owl-page refresh\n";
}
