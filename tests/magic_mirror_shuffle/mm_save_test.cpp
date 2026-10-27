#include "2s2h/BenJsonConversions.hpp"
#include "tests/test_require.h"
#include <memory>
int main() {
  auto original = std::make_unique<RandoSaveInfo>();
  original->finalSeed = 123456;
  original->randoSaveOptions[RO_SHUFFLE_MAGIC_MIRROR] = RO_GENERIC_ON;
  auto &stump = original->randoSaveChecks[RC_LOST_WOODS_MAGIC_MIRROR_STUMP];
  stump = {RI_BOW, true, false, true, true, false, 0};
  original->randoSaveChecks[0] = {RI_RUPEE_PURPLE, true, false, true, true,
                                  false,           40};
  json current = *original;
  auto restored = std::make_unique<RandoSaveInfo>();
  current.get_to(*restored);
  REQUIRE(restored->randoSaveOptions[RO_SHUFFLE_MAGIC_MIRROR] == RO_GENERIC_ON);
  REQUIRE(
      restored->randoSaveChecks[RC_LOST_WOODS_MAGIC_MIRROR_STUMP].randoItemId ==
      RI_BOW);
  REQUIRE(restored->randoSaveChecks[RC_LOST_WOODS_MAGIC_MIRROR_STUMP].obtained);
  json legacy = current;
  legacy["randoSaveChecks"].erase(legacy["randoSaveChecks"].end() - 1);
  legacy["randoSaveOptions"].erase(legacy["randoSaveOptions"].end() - 1);
  legacy.get_to(*restored); // Reuse a nonzero live destination.
  REQUIRE(restored->randoSaveOptions[RO_SHUFFLE_MAGIC_MIRROR] ==
          RO_GENERIC_OFF);
  REQUIRE(
      !restored->randoSaveChecks[RC_LOST_WOODS_MAGIC_MIRROR_STUMP].obtained);
  REQUIRE(
      !restored->randoSaveChecks[RC_LOST_WOODS_MAGIC_MIRROR_STUMP].eligible);
  REQUIRE(
      !restored->randoSaveChecks[RC_LOST_WOODS_MAGIC_MIRROR_STUMP].shuffled);
  REQUIRE(
      restored->randoSaveChecks[RC_LOST_WOODS_MAGIC_MIRROR_STUMP].randoItemId ==
      RI_UNKNOWN);
  REQUIRE(restored->randoSaveChecks[0].randoItemId == RI_RUPEE_PURPLE);
  REQUIRE(restored->randoSaveChecks[0].obtained &&
          restored->randoSaveChecks[0].price == 40);
  REQUIRE(restored->finalSeed == 123456);
  current["randoSaveChecks"].push_back(current["randoSaveChecks"][0]);
  current["randoSaveOptions"].push_back(99);
  current.get_to(*restored);
  REQUIRE(restored->randoSaveOptions[RO_SHUFFLE_MAGIC_MIRROR] == RO_GENERIC_ON);
  REQUIRE(
      restored->randoSaveChecks[RC_LOST_WOODS_MAGIC_MIRROR_STUMP].randoItemId ==
      RI_BOW);
  REQUIRE(restored->randoSaveChecks[RC_LOST_WOODS_MAGIC_MIRROR_STUMP].obtained);
  puts("PASS native MM save JSON: current assigned/collected stump roundtrip; "
       "old shorter arrays default OFF/uncollected; future tails ignored");
}
