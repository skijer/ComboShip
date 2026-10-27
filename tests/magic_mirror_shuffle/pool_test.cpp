#include "FleetShipCombo/FleetShipCombo.h"
#include "Rando/Logic/Logic.h"
#include "tests/test_require.h"
#include <cstring>
#include <memory>
extern "C" {
int gComboGameMode = COMBO_GAME_MM;
int gMMComboGoalHunt = 0, gMMComboGoalRequired = 0, gMMComboGoalPieces = -1;
int requestMirror = 0;
s32 Ship_Random(s32 min, s32) { return min; }
int FleetCombo_UnifiedPoolActive() { return 0; }
int32_t CVarGetInteger(const char *key, int32_t fallback) {
  return strcmp(key, "gRando.Generation.ShuffleMagicMirror") == 0
             ? requestMirror
             : fallback;
}
}
namespace Rando {
std::vector<RandoItemId> GetStartingItemsFromSave(RandoSaveInfo &) {
  return {};
}
std::vector<RandoItemId> GetComputedStartingItems(RandoSaveInfo &) {
  return {};
}
std::vector<RandoCheckId> GetExcludedChecksFromConfig() { return {}; }
namespace StaticData {
std::map<RandoCheckId, RandoStaticCheck> Checks;
std::map<RandoItemId, RandoStaticItem> Items;
} // namespace StaticData
namespace Logic {
std::map<RandoRegionId, RandoRegion> Regions;
}
} // namespace Rando
int main() {
  using namespace Rando;
  StaticData::Checks[RC_LOST_WOODS_MAGIC_MIRROR_STUMP] = {
      RC_LOST_WOODS_MAGIC_MIRROR_STUMP,
      "RC_LOST_WOODS_MAGIC_MIRROR_STUMP",
      RCTYPE_NPC,
      SCENE_LOST_WOODS,
      FLAG_NONE,
      0,
      RI_OOT_NEI_MAGIC_MIRROR};
  StaticData::Items[RI_OOT_NEI_MAGIC_MIRROR].randoItemType = RITYPE_MAJOR;
  // Native room entrance; combined also relays the actual OoT Lost Woods route.
  Logic::Regions[RR_WOODS_OF_MYSTERY]
      .checks[RC_LOST_WOODS_MAGIC_MIRROR_STUMP] = {[] { return true; }, "true"};
  for (int mode : {COMBO_GAME_MM, COMBO_GAME_BOTH}) {
    gComboGameMode = mode;
    for (int nei : {0, 1})
      for (int plentiful : {0, 1}) {
        requestMirror = 0;
        auto control = std::make_unique<RandoSaveInfo>();
        control->randoSaveOptions[RO_STARTING_HEALTH] = 3;
        control->randoSaveOptions[RO_SHUFFLE_NEI_ITEMS] = nei;
        control->randoSaveOptions[RO_PLENTIFUL_ITEMS] = plentiful;
        std::vector<RandoCheckId> offChecks;
        std::vector<RandoItemId> offItems;
        Logic::GeneratePools(*control, offChecks, offItems);
        REQUIRE(std::count(offChecks.begin(), offChecks.end(),
                           RC_LOST_WOODS_MAGIC_MIRROR_STUMP) == 0);
        REQUIRE(std::count(offItems.begin(), offItems.end(),
                           RI_OOT_NEI_MAGIC_MIRROR) == 0);
        for (int option : {0, 1})
          for (int request : {0, 1}) {
            requestMirror = request;
            auto save = std::make_unique<RandoSaveInfo>(*control);
            save->randoSaveOptions[RO_SHUFFLE_MAGIC_MIRROR] = option;
            std::vector<RandoCheckId> checks;
            std::vector<RandoItemId> items;
            Logic::GeneratePools(*save, checks, items);
            const bool enabled = option || (mode == COMBO_GAME_BOTH && request);
            REQUIRE(save->randoSaveOptions[RO_SHUFFLE_MAGIC_MIRROR] == enabled);
            REQUIRE(std::count(checks.begin(), checks.end(),
                               RC_LOST_WOODS_MAGIC_MIRROR_STUMP) == enabled);
            REQUIRE(std::count(items.begin(), items.end(),
                               RI_OOT_NEI_MAGIC_MIRROR) == enabled);
            REQUIRE(checks.size() == offChecks.size() + enabled);
            REQUIRE(items.size() == offItems.size() + enabled);
          }
      }
  }
  puts("PASS native MM generator: OFF/ON, NEI independent, Plentiful unique, "
       "solo/combined +1/+1, OR request serialized");
}
