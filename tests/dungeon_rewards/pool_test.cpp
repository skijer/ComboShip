// Execute the complete native pool generator with real game/rando data types.
// Save/config inputs and RNG are test boundaries; no item-placement code is
// mocked.
#include "FleetShipCombo/FleetShipCombo.h"
#include "Rando/Logic/Logic.h"
#include <cstdio>
#include <memory>

extern "C" int32_t CVarGetInteger(const char *, int32_t fallback) {
  return fallback;
}

extern "C" {
#ifdef COMBO_BUILD
int gComboGameMode = COMBO_GAME_BOTH;
#endif
int gMMComboGoalHunt = 0;
int gMMComboGoalRequired = 0;
int gMMComboGoalPieces = -1;
s32 Ship_Random(s32 min, s32 max) { return min; }
int FleetShipCombo_GetActiveGame(void) { return -1; }
}

// Extracted unchanged from FleetShipCombo.cpp by the runner. In ComboShip this
// intentionally returns false: the launcher, not NEI's old fill, owns the pool.
#include "unified_pool.inc"

static std::vector<RandoItemId> startingItems;
namespace Rando {
std::vector<RandoItemId> GetStartingItemsFromSave(RandoSaveInfo &) {
  return startingItems;
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
} // namespace Logic
} // namespace Rando

static const RandoItemId ootRewards[] = {
    RI_OOT_MEDALLION_FOREST,     RI_OOT_MEDALLION_FIRE,
    RI_OOT_MEDALLION_WATER,      RI_OOT_MEDALLION_SPIRIT,
    RI_OOT_MEDALLION_SHADOW,     RI_OOT_MEDALLION_LIGHT,
    RI_OOT_STONE_KOKIRI_EMERALD, RI_OOT_STONE_GORON_RUBY,
    RI_OOT_STONE_ZORA_SAPPHIRE,
};
static const RandoItemId otherQuestItems[] = {
    RI_OOT_STONE_OF_AGONY,          RI_OOT_SONG_ZELDAS_LULLABY,
    RI_OOT_SONG_MINUET_OF_FOREST,   RI_OOT_SONG_BOLERO_OF_FIRE,
    RI_OOT_SONG_SERENADE_OF_WATER,  RI_OOT_SONG_REQUIEM_OF_SPIRIT,
    RI_OOT_SONG_NOCTURNE_OF_SHADOW, RI_OOT_SONG_PRELUDE_OF_LIGHT,
    RI_OOT_SONG_FUGUE_OF_HOME,      RI_OOT_SONG_COMMAND_MELODY,
    RI_OOT_SONG_BALLAD_OF_THE_HERO,
};
static int failures = 0;
static void expect(bool ok, const char *what) {
  if (!ok) {
    std::fprintf(stderr, "FAIL: %s\n", what);
    ++failures;
  }
}

int main() {
  using namespace Rando;
  // Hand-authored native checks: a normal chest plus the four MM boss rewards.
  const RandoCheckId remainsChecks[] = {
      RC_WOODFALL_TEMPLE_BOSS_WARP, RC_SNOWHEAD_TEMPLE_BOSS_WARP,
      RC_GREAT_BAY_TEMPLE_BOSS_WARP, RC_STONE_TOWER_TEMPLE_INVERTED_BOSS_WARP};
  const RandoItemId remainsItems[] = {RI_REMAINS_ODOLWA, RI_REMAINS_GOHT,
                                      RI_REMAINS_GYORG, RI_REMAINS_TWINMOLD};
  auto &region = Logic::Regions[(RandoRegionId)0];
  auto addCheck = [&](RandoCheckId id, RandoCheckType type, RandoItemId item) {
    StaticData::Checks[id] = {id,        "fixture", type, SCENE_20SICHITAI,
                              FLAG_NONE, 0,         item};
    region.checks[id] = {[] { return true; }, "true"};
  };
  addCheck(RC_CLOCK_TOWN_SOUTH_CHEST_LOWER, RCTYPE_CHEST, RI_JUNK);
  for (size_t i = 0; i < 4; ++i)
    addCheck(remainsChecks[i], RCTYPE_REMAINS, remainsItems[i]);
  // Category fixtures needed by the production plentiful phase.
  for (auto item : ootRewards)
    StaticData::Items[item].randoItemType = RITYPE_MAJOR;
  for (auto item : otherQuestItems)
    StaticData::Items[item].randoItemType = RITYPE_MAJOR;
  for (auto item : remainsItems)
    StaticData::Items[item].randoItemType = RITYPE_MAJOR;
  StaticData::Items[RI_JUNK].randoItemType = RITYPE_JUNK;

  for (bool quest : {false, true}) {
    for (bool plentiful : {false, true}) {
      for (bool shuffleRemains : {false, true}) {
        for (bool startWith : {false, true}) {
          auto save = std::make_unique<RandoSaveInfo>();
          save->randoSaveOptions[RO_STARTING_HEALTH] = 3;
          save->randoSaveOptions[RO_SHUFFLE_OOT_QUEST] = quest;
          save->randoSaveOptions[RO_PLENTIFUL_ITEMS] = plentiful;
          save->randoSaveOptions[RO_SHUFFLE_BOSS_REMAINS] = shuffleRemains;
          save->randoSaveOptions[RO_SHUFFLE_NEI_ITEMS] = 1;
          startingItems =
              startWith ? std::vector<RandoItemId>{RI_OOT_STONE_ZORA_SAPPHIRE,
                                                   RI_OOT_STONE_OF_AGONY}
                        : std::vector<RandoItemId>{};
          std::vector<RandoCheckId> checks;
          std::vector<RandoItemId> items;
          Logic::GeneratePools(*save, checks, items);
          auto count = [&](RandoItemId id) {
            return std::count(items.begin(), items.end(), id);
          };
          for (auto reward : ootRewards) {
#ifdef COMBO_BUILD
            const int want = 0;
#else
            const int want =
                quest && !(startWith && reward == RI_OOT_STONE_ZORA_SAPPHIRE)
                    ? (plentiful ? 2 : 1)
                    : 0;
#endif
            expect(count(reward) == want,
                   "OoT reward pool ownership (stones/medallions)");
          }
          for (auto item : otherQuestItems) {
            const int want =
                quest && !(startWith && item == RI_OOT_STONE_OF_AGONY)
                    ? (plentiful ? 2 : 1)
                    : 0;
            expect(count(item) == want,
                   "other quest items and starting-item removal preserved");
          }
          for (auto item : remainsItems) {
            expect(count(item) == (shuffleRemains ? (plentiful ? 2 : 1) : 0),
                   "MM boss remains follow MM's own shuffle setting");
          }
          expect(checks.size() == (shuffleRemains ? 5u : 1u),
                 "native check pool preserved");
          expect(count(RI_OOT_NEI_TIME_GATE) == 1 &&
                     count(RI_OOT_NEI_LIGHT_ROD) == 1,
                 "NEI item supply preserved");
        }
      }
    }
  }
#ifdef COMBO_BUILD
  const char *mode = "ComboShip";
#else
  const char *mode = "standalone MM";
#endif
  std::printf("%s: %s native GeneratePools, 16 configurations, %d failures\n",
              failures ? "FAIL" : "PASS", mode, failures);
  return failures ? 1 : 0;
}
