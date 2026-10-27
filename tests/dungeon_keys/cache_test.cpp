#include "ComboItemDrawABI.h"
#include "combo/menu/ComboElementalArrowGi.h"
#include "soh/soh/Enhancements/randomizer/NeiGiEffectPolicy.h"
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
extern "C" {
#include "fixture_api.h"
}
#define COMBO_EXPORT
namespace Ship {
static bool ownerActive=false;
struct CrossRMRegistry { static int Get(const char* game) { assert(!strcmp(game,"mm")); return 1; } };
struct ResourceManagerScope {
 bool previous=ownerActive;
 ResourceManagerScope(int) { ownerActive=true; }
 ~ResourceManagerScope() { ownerActive=previous; }
};
}
enum RandoItemId {
  RI_UNKNOWN = 0,
  RI_SHIELD_MIRROR,
  RI_ARROW_FIRE,
  RI_ARROW_ICE,
  RI_ARROW_LIGHT,
  RI_WOODFALL_SMALL_KEY,
  RI_WOODFALL_BOSS_KEY,
  RI_WOODFALL_MAP,
  RI_WOODFALL_COMPASS,
  RI_SNOWHEAD_SMALL_KEY,
  RI_SNOWHEAD_BOSS_KEY,
  RI_SNOWHEAD_MAP,
  RI_SNOWHEAD_COMPASS,
  RI_GREAT_BAY_SMALL_KEY,
  RI_GREAT_BAY_BOSS_KEY,
  RI_GREAT_BAY_MAP,
  RI_GREAT_BAY_COMPASS,
  RI_STONE_TOWER_SMALL_KEY,
  RI_STONE_TOWER_BOSS_KEY,
  RI_STONE_TOWER_MAP,
  RI_STONE_TOWER_COMPASS,
  RI_PROGRESSIVE_SWORD,
  RI_OOT_PROGRESSIVE_HAMMER,
  RI_OOT_PROGRESSIVE_MASTER_SWORD,
  RI_OOT_PROGRESSIVE_BGS,
  RI_SWORD_KOKIRI,
  RI_SWORD_RAZOR,
  RI_SWORD_GILDED,
  RI_GREAT_FAIRY_SWORD,
  RI_OOT_MASTER_SWORD,
  RI_OOT_TRUE_MASTER_SWORD,
  RI_OOT_BIGGORON_SWORD,
  RI_OOT_DINS_FIRE,
  RI_OOT_FARORES_WIND,
  RI_OOT_NAYRUS_LOVE,
  RI_PROGRESSIVE_BOW,
  RI_PROGRESSIVE_BOMB_BAG,
  RI_PROGRESSIVE_WALLET,
  RI_PROGRESSIVE_MAGIC,
  RI_PROGRESSIVE_LULLABY,
  RI_TIME_PROGRESSIVE,
  RI_JUNK,
  RI_TRAP,
  RI_TRIFORCE_PIECE,
  RI_TRIFORCE_PIECE_PREVIOUS,
  RI_TEST_SWORD,
  RI_TEST_COMPASS,
  RI_GREAT_SPIN_ATTACK,
  RI_OOT_NEI_ROD_OF_SEASONS,
  RI_OOT_NEI_SEASON_SPRING,
  RI_OOT_NEI_SEASON_SUMMER,
  RI_OOT_NEI_SEASON_AUTUMN,
  RI_OOT_NEI_SEASON_WINTER,
  RI_OOT_BOTTLE_BIG_POE,
  RI_OOT_BOTTLE_BLUE_FIRE,
  RI_OOT_BOTTLE_BLUE_POTION,
  RI_OOT_BOTTLE_BUGS,
  RI_OOT_BOTTLE_FAIRY,
  RI_OOT_BOTTLE_FISH,
  RI_OOT_BOTTLE_GREEN_POTION,
  RI_OOT_BOTTLE_MAGIC_MUSHROOM,
  RI_OOT_BOTTLE_POE,
  RI_OOT_RUTOS_LETTER,
  RI_OOT_NEI_LANTERN,
  RI_OOT_NEI_POKE_BALL,
  RI_OOT_NEI_MARIO_MASK,
  RI_OOT_NEI_MAGIC_MIRROR,
  RI_RED_POTION_REFILL,
  RI_GREEN_POTION_REFILL,
  RI_BLUE_POTION_REFILL,
  RI_BOTTLE_EMPTY
};
#include "ComboOotBottleShimmerMM.h"
/* PRODUCTION_OWNER */
RandoItemId progression = RI_TEST_SWORD;
namespace Rando {
RandoItemId ConvertItem(RandoItemId) { return progression; }
RandoItemId CurrentJunkItem() { return RI_UNKNOWN; }
RandoItemId CurrentTrapItem() { return RI_UNKNOWN; }
namespace StaticData {
struct Item {
  int drawId;
  const char *name;
};
std::map<RandoItemId, Item> Items;
RandoItemId GetItemIdFromDisplayName(const char *name) {
  for (const auto &[id, item] : Items)
    if (!strcmp(name, item.name))
      return id;
  return RI_UNKNOWN;
}
RandoItemId GetItemIdFromName(const char *name) {
  return GetItemIdFromDisplayName(name);
}
struct Location {
  std::string GetName() { return "fixture check"; }
};
Location *GetLocation(int) {
  static Location loc;
  return &loc;
}
} // namespace StaticData
} // namespace Rando
namespace ItemGrantAudit {
struct Scope {
  Scope(const char *, int, int, bool) {}
};
} // namespace ItemGrantAudit
extern "C" int ResourceMgr_GetIkanaShieldGiPoseForGame(const char*, const char*, float*, float*) { return 0; }
/* PRODUCTION_OPS */
/* PRODUCTION_CROSS */
int32_t MM_FillSongDrawInfo(RandoItemId, CwItemDrawInfo *) { return 0; }
int32_t MM_FillOpsDrawInfo(RandoItemId, CwItemDrawInfo *) { return 0; }
int32_t MM_FillSimpleDrawInfo(RandoItemId, CwItemDrawInfo *) { return 0; }
int32_t MM_FillEnemySoulDrawInfo(RandoItemId, CwItemDrawInfo *) { return 0; }
int32_t MM_FillGidAliasDrawInfo(RandoItemId, CwItemDrawInfo *) { return 0; }
bool neiAvailable = false;
bool MM_DescribeNeiGi(RandoItemId id, CwItemDrawInfo *out) {
  if (!neiAvailable || id != RI_TEST_SWORD)
    return false;
  out->drawKind = CW_DRAW_KIND_NEI_GI;
  out->dlistCount = 1;
  out->xluStartIndex = -1;
  out->scale = 1;
  out->dlists[0] = "__OTR__@oot:objects/nei_gi_redesign/master_sword/gi_dl";
  out->itemShimmer = 1;
  out->stateDependent = 2;
  return true;
}
static const char *gGreatSpinAttackDiskDL =
    "__OTR__objects/gameplay_keep/gGreatSpinAttackDiskDL";
static const char *gGreatSpinAttackCylinderDL =
    "__OTR__objects/gameplay_keep/gGreatSpinAttackCylinderDL";
/* PRODUCTION_SPIN */
bool MM_HasAnimDraw(RandoItemId) { return false; }
const char *gGiMoonsTearItemDL = "__OTR__unrelated/tear";
const char *gGiMoonsTearTexAnim = "__OTR__unrelated/tearTex";
const char *gGiFairyBottleEmptyDL = "__OTR__unrelated/bottle";
const char *gGiFairyBottleTexAnim = "__OTR__unrelated/bottleTex";
void *Combo_ResolveSym(const char *, const char *);
#include "combo/menu/ComboSwordGiAssetSelection.h"
extern "C" const char *NeiResource_Route(const char *path) {
  static std::unordered_set<std::string> paths;
  return paths.insert(std::string("__OTR__@oot:") + (path + 7)).first->c_str();
}
enum RandomizerGet {
  RG_KOKIRI_SWORD,
  RG_RAZOR_SWORD,
  RG_GILDED_SWORD,
  RG_MASTER_SWORD,
  RG_TRUE_MASTER_SWORD,
  RG_BIGGORON_SWORD,
  RG_GIANTS_KNIFE,
  RG_GREAT_FAIRY_SWORD
};
static bool ownerAltEnabled, ownerDin, ownerNotReady, ownerModuleReady;
static const char* expectedDirectName = nullptr;
static std::unordered_set<std::string> ownerResources;
extern "C" int32_t OOT_NeiAltAssetsEnabled() { return ownerAltEnabled; }
extern "C" int32_t OOT_NeiResourceExists(const char *path) {
  return ownerResources.contains(path);
}
static int OwnerCVar(const char *, int) { return ownerDin; }
#define CVAR_ENHANCEMENT(x) x
#define CVarGetInteger OwnerCVar
/* PRODUCTION_OWNER_SWORD */
#undef CVarGetInteger
static const char *object_toki_objects_DL_001BD0 =
    "__OTR__objects/object_toki_objects/object_toki_objects_DL_001BD0";
static int32_t OwnerSword(const char *name, CwItemDrawInfo *out) {
  if (ownerNotReady)
    return CW_DRAW_NOT_READY;
  if (expectedDirectName) {
    assert(!strcmp(name, expectedDirectName));
    out->stateDependent = 2;
    return CwCustomGi(out, "__OTR__direct_legacy_recipe", 1.f);
  }
  const RandomizerGet rg = !strcmp(name, "Master Sword") ? RG_MASTER_SWORD
                           : !strcmp(name, "True Master Sword")
                               ? RG_TRUE_MASTER_SWORD
                               : RG_BIGGORON_SWORD;
  assert(!strcmp(name, "Master Sword") || !strcmp(name, "True Master Sword") ||
         !strcmp(name, "Biggoron's Sword"));
  out->itemShimmer = 1;
  const uint8_t color[4] = {220, 225, 240, 255};
  std::memcpy(out->itemShimmerColor, color, 4);
  if (CwAltSwordGi(rg, out))
    return 1;
  switch (rg) {
    /* PRODUCTION_OWNER_MASTER_CASE */
  default:
    break;
  }
  out->dlistCount = 1;
  out->xluStartIndex = -1;
  out->drawKind = CW_DRAW_KIND_GORON_SWORD;
  out->dlists[0] = "__OTR__objects/object_gi_longsword/gGiBiggoronSwordDL";
  return 1;
}
/* PRODUCTION_PRODUCER */
using RandomizerCheck = int;
constexpr int RC_UNKNOWN_CHECK = -1;
namespace ComboRando {
constexpr int GAME_MM = 1;
struct ForeignItem {
  int itemGame = GAME_MM;
  std::string itemName, fakeItemName;
  bool HasDisguise() const { return !fakeItemName.empty(); }
};
} // namespace ComboRando
ComboRando::ForeignItem foreign;
bool ready = true;
const ComboRando::ForeignItem *OOT_LookupForeign(int, const std::string &) {
  return ready ? &foreign : nullptr;
}
uint64_t generation = 1;
uint64_t OOT_ForeignMapGen() { return generation; }
struct {
  int fileNum = 0;
} gSaveContext;
int malformedSeason = 0;
int32_t DescribeForeign(const char* name, CwItemDrawInfo* out) {
  int32_t result = MM_GetItemDrawInfo(name, out);
  if (malformedSeason == 1) out->neiEffect = 0;
  if (malformedSeason == 2) out->neiEffect = 7;
  if (malformedSeason == 3) {
    out->dlistCount = 1;
    out->dlists[0] = "__OTR__@oot:objects/object_nei_rod_of_seasons/gNeiRodOfSeasonsDL";
  }
  if (malformedSeason == 4) out->dlistCount = 0;
  if (malformedSeason == 5) { out->opCount = 1; out->ops[0].op = CW_OP_SCALE; }
  return result;
}
void *Combo_ResolveSym(const char *, const char *symbol) {
  if (!strcmp(symbol, "OOT_GetItemDrawInfo"))
    return ownerModuleReady ? (void *)OwnerSword : nullptr;
  return !strcmp(symbol, "MM_GetItemDrawInfo") ? (void *)DescribeForeign
                                               : nullptr;
}
/* PRODUCTION_CACHE */
/* PRODUCTION_HOST */

void CheckRendered(const ComboForeignDrawInfo *info, int owner, int kind,
                   bool custom) {
  assert(info && info->ok && info->appearanceDependent && info->stateDependent);
  assert(info->drawKind == CW_DRAW_KIND_OPS);
  PlayState play{};
  TestReset();
  OOT_DrawForeignOps(&play, info);
  assert(testDepth == 0 && !testGray[0] && !testGray[1]);
  if (custom) {
    assert(testCount == 2 && testScales == 1 &&
           strstr(testPaths[0], "@mm:objects/combo_dungeon_keys/"));
    assert(testColorAtDraw[0].r == (testColors && testChanged ? 10 + owner
                                    : kind == 1               ? 233
                                                              : 213));
    const int defaults[] = {236, 129, 99, 201};
    assert(testColorAtDraw[1].r ==
           (testEmblemChanged ? 210 + owner : defaults[owner]));
  } else {
    assert(testCount == (kind == 1 || kind == 3 ? 2 : 1) && testScales == 0);
    assert(strstr(testPaths[0], "@mm:objects/") &&
           !strstr(testPaths[0], "combo_dungeon_keys"));
    assert(testTint[0] == (bool)testColors);
    if (testColors)
      assert(testColorAtDraw[0].r == 10 + owner &&
             testColorAtDraw[0].a == (kind == 2   ? 96
                                      : kind == 3 ? 176
                                                  : 192));
    if (kind == 1 || kind == 3)
      assert(testTint[1] == (kind == 1 && testEmblemChanged));
    if (kind == 1 && testEmblemChanged)
      assert(testColorAtDraw[1].r == 210 + owner);
    if (kind == 3)
      assert(testSetup5 == 1 && !testTint[1]);
  }
}

int main() {
  const RandoItemId itemIds[4][4] = {
      {RI_WOODFALL_SMALL_KEY, RI_WOODFALL_BOSS_KEY, RI_WOODFALL_MAP,
       RI_WOODFALL_COMPASS},
      {RI_SNOWHEAD_SMALL_KEY, RI_SNOWHEAD_BOSS_KEY, RI_SNOWHEAD_MAP,
       RI_SNOWHEAD_COMPASS},
      {RI_GREAT_BAY_SMALL_KEY, RI_GREAT_BAY_BOSS_KEY, RI_GREAT_BAY_MAP,
       RI_GREAT_BAY_COMPASS},
      {RI_STONE_TOWER_SMALL_KEY, RI_STONE_TOWER_BOSS_KEY, RI_STONE_TOWER_MAP,
       RI_STONE_TOWER_COMPASS}};
  const char *names[4][4] = {{"WF key", "WF boss", "WF map", "WF compass"},
                             {"SH key", "SH boss", "SH map", "SH compass"},
                             {"GB key", "GB boss", "GB map", "GB compass"},
                             {"ST key", "ST boss", "ST map", "ST compass"}};
  for (int d = 0; d < 4; d++)
    for (int k = 0; k < 4; k++)
      Rando::StaticData::Items[itemIds[d][k]] = {k, names[d][k]};
  for (int d = 0; d < 4; d++)
    for (int k = 0; k < 4; k++) {
      const int rc = d * 4 + k;
      foreign.itemName = names[d][k];
      generation++;
      testAlt = false;
      testMissing = 0;
      testColors = 1;
      testChanged = 1;
      testEmblemChanged = 1;
      CheckRendered(ComboResolveForeignDrawInfo(rc), d, k, k < 2);
      ComboLatchForeignDraw(rc);
      assert(ComboForeignDrawCacheGet().map.at(rc).appearanceDependent);
      // Editing or Tab switching after the GI latch must immediately
      // re-resolve.
      testAlt = true;
      CheckRendered(ComboResolveForeignDrawInfo(rc), d, k, k < 2);
      if (k < 2) {
        testChanged = 0;
        testEmblemChanged = 0;
        CheckRendered(ComboResolveForeignDrawInfo(rc), d, k, true);
        testChanged = 1;
        testEmblemChanged = 1;
      }
      testAlt = false;
      testColors = 0;
      testEmblemChanged = 0;
      const auto *plain = ComboResolveForeignDrawInfo(rc);
      assert(plain && plain->appearanceDependent);
      if (k < 2)
        CheckRendered(plain, d, k, true);
      else
        assert(plain->drawKind == CW_DRAW_KIND_SIMPLE &&
               !strstr(plain->dls[0], "combo_dungeon_keys"));
      ComboLatchForeignDraw(rc);
      testColors = 1;
      testEmblemChanged = 1;
      CheckRendered(ComboResolveForeignDrawInfo(rc), d, k, k < 2);
      testAlt = true;
      for (int missing : {1, 2}) {
        testMissing = missing;
        CheckRendered(ComboResolveForeignDrawInfo(rc), d, k, false);
      }
      testMissing = 0;
      CheckRendered(ComboResolveForeignDrawInfo(rc), d, k, k < 2);
      if (k == 1) {
        testAlt = false;
        testMissing = 1;
        testColors = 0;
        testEmblemChanged = 1;
        CheckRendered(ComboResolveForeignDrawInfo(rc), d, k, false);
      }
    }
  Rando::StaticData::Items[RI_GREAT_SPIN_ATTACK] = {0, "Great Spin Attack"};
  foreign.itemName = "Great Spin Attack";
  generation++;
  CwItemDrawInfo spin{};
  assert(MM_GetItemDrawInfo(foreign.itemName.c_str(), &spin) == 1);
  assert(spin.stateDependent == 2 &&
         spin.drawKind == CW_DRAW_KIND_MM_SPIN_ATTACK);
  const auto *burst = ComboResolveForeignDrawInfo(98);
  assert(burst && burst->appearanceDependent && burst->stateDependent);
  assert(strstr(burst->dls[0],
                "__OTR__@mm:objects/gameplay_keep/gGreatSpinAttackDiskDL"));
  assert(burst->primColorXlu[0] == 17);
  ComboLatchForeignDraw(98);
  testSpinRed = 211;
  assert(ComboResolveForeignDrawInfo(98)->primColorXlu[0] == 211);
  assert(ComboForeignLatchedName(98) == nullptr);

  const char* seasonNames[] = {"Season: Spring", "Season: Summer", "Season: Autumn", "Season: Winter", "Rod of Seasons"};
  for(int i=0;i<5;++i) {
    auto id=i==4 ? RI_OOT_NEI_ROD_OF_SEASONS : static_cast<RandoItemId>(RI_OOT_NEI_SEASON_SPRING+i);
    Rando::StaticData::Items[id]={-1,seasonNames[i]}; foreign.itemName=seasonNames[i]; ++generation;
    const auto* season=ComboResolveForeignDrawInfo(170+i);
    assert(season && season->drawKind==CW_DRAW_KIND_SEASON_GI && season->neiEffect==i+1);
    if(i==4) {
      assert(season->count==1 && season->scale==.35f);
      assert(!strcmp(season->dls[0],"__OTR__@oot:objects/object_nei_rod_of_seasons/gNeiRodOfSeasonsDL"));
    } else assert(season->count==0 && season->dls[0]==nullptr);
  }
  for (malformedSeason = 1; malformedSeason <= 5; ++malformedSeason) {
    foreign.itemName = seasonNames[malformedSeason == 4 ? 4 : 0]; ++generation;
    assert(ComboResolveForeignDrawInfo(180) == nullptr);
  }
  malformedSeason = 0;
  Rando::StaticData::Items[RI_OOT_BOTTLE_FAIRY] = {GID_FAIRY_CONTAINER, "Bottle with Fairy"};
  foreign.itemName = "Bottle with Fairy";
  ++generation;
  const bool fairyPreviousAlt = testAlt;
  testAlt = testFairyEmptyMod = false;
  const auto* fairy = ComboResolveForeignDrawInfo(185);
  assert(fairy && fairy->drawKind == CW_DRAW_KIND_MM_FAIRY_CONTAINER &&
         fairy->appearanceDependent && fairy->stateDependent);
  assert(strstr(fairy->dls[0], "@mm:objects/combo_bottle_gi/EmptyOpaque"));
  ComboLatchForeignDraw(185);
  for (int selection : {0, 1, 0, 2, 0}) {
    testAlt = selection == 1;
    testFairyEmptyMod = selection == 2;
    fairy = ComboResolveForeignDrawInfo(185);
    assert(fairy && fairy->appearanceDependent && fairy->stateDependent && fairy->count == 4);
    assert(strstr(fairy->dls[0], selection ? "@mm:objects/object_gi_bottle/gGiEmptyBottleCorkDL"
                                         : "@mm:objects/combo_bottle_gi/EmptyOpaque") &&
           "latched fairy casing must refresh after owner Alt/editor selection changes");
    assert(strstr(fairy->dls[1], selection ? "@mm:objects/object_gi_bottle/gGiEmptyBottleGlassDL"
                                         : "@mm:objects/combo_bottle_gi/FairyXlu"));
    assert(strstr(fairy->dls[2], "gGiFairyBottleContentsDL") &&
           strstr(fairy->dls[3], "gGiFairyBottleBillboardRotMtx"));
  }
  testAlt = testFairyEmptyMod = false;
  for (bool missing : {false, true})
    for (int selected : {0, 1, 2, 4, 8}) {
      testFairyGlassMissing = missing;
      testFairyBundledMod = selected;
      fairy = ComboResolveForeignDrawInfo(185);
      assert(fairy && fairy->count == 4 && fairy->appearanceDependent && fairy->stateDependent);
      assert(strstr(fairy->dls[0], "@mm:objects/combo_bottle_gi/EmptyOpaque"));
      assert(strstr(fairy->dls[1], !missing && !selected
                                     ? "@mm:objects/combo_bottle_gi/FairyXlu"
                                     : "@mm:objects/combo_bottle_gi/EmptyXlu"));
      assert(strstr(fairy->dls[2], "gGiFairyBottleContentsDL") &&
             strstr(fairy->dls[3], "gGiFairyBottleBillboardRotMtx"));
    }
  testFairyGlassMissing = false;
  testFairyBundledMod = 0;
  testAlt = fairyPreviousAlt;
  testFairyEmptyMod = false;
  puts("PASS fairy readable-shell selector -> MM export -> OoT cache/latch with Alt/editor changes, missing glass and selected casing/materials");
  const RandoItemId bottles[] = {
      RI_OOT_BOTTLE_BIG_POE,      RI_OOT_BOTTLE_BLUE_FIRE,
      RI_OOT_BOTTLE_BLUE_POTION,  RI_OOT_BOTTLE_BUGS,
      RI_OOT_BOTTLE_FAIRY,        RI_OOT_BOTTLE_FISH,
      RI_OOT_BOTTLE_GREEN_POTION, RI_OOT_BOTTLE_MAGIC_MUSHROOM,
      RI_OOT_BOTTLE_POE,          RI_OOT_RUTOS_LETTER};
  const uint8_t bottleColors[][3] = {
      {150, 200, 0},   {100, 160, 255}, {100, 160, 255}, {220, 225, 240},
      {255, 160, 235}, {220, 225, 240}, {0, 200, 0},     {222, 100, 245},
      {100, 0, 200},   {220, 225, 240}};
  for (int i = 0; i < 10; ++i) {
    Rando::StaticData::Items[bottles[i]] = {(i == 1 || i == 9) ? -1 : 0,
                                            "Imported OoT bottle"};
    CwItemDrawInfo bottle{};
    assert(MM_FillItemDrawInfo(bottles[i], &bottle));
    assert(bottle.itemShimmer &&
           !memcmp(bottle.itemShimmerColor, bottleColors[i], 3));
    if (i == 1 || i == 9) {
      assert(bottle.dlistCount == 2 && bottle.xluStartIndex == 1);
      assert(!strncmp(bottle.dlists[0], "__OTR__@oot:", 11));
      assert(bottle.drawKind ==
             (i == 1 ? CW_DRAW_KIND_BLUE_FIRE : CW_DRAW_KIND_SIMPLE));
    }
  }
  // New authored descriptors must preserve the resolved progressive tier and
  // freeze that tier when granted, while carrying the explicit OoT asset owner.
  Rando::StaticData::Items[RI_PROGRESSIVE_SWORD] = {0, "NEI progressive sword"};
  Rando::StaticData::Items[RI_OOT_PROGRESSIVE_MASTER_SWORD] = {
      0, "NEI progressive Master"};
  Rando::StaticData::Items[RI_OOT_PROGRESSIVE_BGS] = {
      0, "NEI progressive Biggoron"};
  Rando::StaticData::Items[RI_OOT_PROGRESSIVE_HAMMER] = {
      0, "NEI progressive hammer"};
  Rando::StaticData::Items[RI_TEST_SWORD] = {0, "NEI concrete sword"};
  Rando::StaticData::Items[RI_TEST_COMPASS] = {3, "Next tier"};
  neiAvailable = true;
  for (auto id : {RI_PROGRESSIVE_SWORD, RI_OOT_PROGRESSIVE_MASTER_SWORD,
                  RI_OOT_PROGRESSIVE_BGS, RI_OOT_PROGRESSIVE_HAMMER}) {
    progression = RI_TEST_SWORD;
    foreign.itemName = Rando::StaticData::Items[id].name;
    ++generation;
    CwItemDrawInfo descriptor{};
    assert(MM_GetItemDrawInfo(foreign.itemName.c_str(), &descriptor));
    assert(descriptor.drawKind == CW_DRAW_KIND_NEI_GI &&
           descriptor.itemShimmer);
    assert(descriptor.stateDependent == 1 &&
           std::string(descriptor.resolvedName) == "NEI concrete sword");
    const auto *current = ComboResolveForeignDrawInfo(201);
    assert(current && current->drawKind == CW_DRAW_KIND_NEI_GI &&
           !current->appearanceDependent);
    assert(!strcmp(current->dls[0], descriptor.dlists[0]));
    ComboLatchForeignDraw(201);
    progression = RI_TEST_COMPASS;
    const auto *frozen = ComboResolveForeignDrawInfo(201);
    assert(frozen && frozen->drawKind == CW_DRAW_KIND_NEI_GI &&
           !frozen->stateDependent);
    assert(frozen->resolvedName == "NEI concrete sword");
  }
  neiAvailable = false;
  // Imported concrete sword aliases really have GID_NONE in MM. Absence of
  // the authored mesh must describe the owner fallback instead of a sentinel.
  for (auto [id, name] :
       {std::pair{RI_OOT_MASTER_SWORD, "Master Sword"},
        std::pair{RI_OOT_TRUE_MASTER_SWORD, "Real Master Sword"},
        std::pair{RI_OOT_BIGGORON_SWORD, "Biggoron's Sword"}}) {
    Rando::StaticData::Items[id] = {-1, name};
    if (!ownerModuleReady) {
      CwItemDrawInfo unavailable{};
      assert(MM_GetItemDrawInfo(name, &unavailable) == CW_DRAW_NOT_READY &&
             !unavailable.dlistCount);
      foreign.itemName = name;
      ++generation;
      assert(!ComboResolveForeignDrawInfo(202));
      ownerModuleReady = true;
    }
    for (int mode = 0; mode < 4; ++mode) {
      ownerAltEnabled = mode != 0;
      ownerDin = mode == 2;
      ownerResources.clear();
      const bool bgs = id == RI_OOT_BIGGORON_SWORD;
      const char *selected =
          bgs ? "__OTR__alt/objects/object_custom_equip/gCustomLongswordDL"
              : "__OTR__alt/objects/object_custom_equip/gCustomMasterSwordDL";
      const char *fire =
          bgs ? "__OTR__objects/din_fire_sword/progressive/bgs/SwordDL"
              : "__OTR__objects/din_fire_sword/progressive/adult/SwordDL";
      if (mode != 3) {
        ownerResources.insert(selected);
        ownerResources.insert(fire);
      }
      CwItemDrawInfo descriptor{};
      assert(MM_GetItemDrawInfo(name, &descriptor) == 1);
      assert(descriptor.dlistCount == 1 && descriptor.stateDependent == 2 &&
             descriptor.itemShimmer);
      assert(!strncmp(descriptor.dlists[0], "__OTR__@oot:", 12));
      const bool custom = mode == 1 || mode == 2;
      assert(descriptor.drawKind == (custom
                                         ? CW_DRAW_KIND_CUSTOM_GI
                                         : (bgs ? CW_DRAW_KIND_GORON_SWORD
                                                : CW_DRAW_KIND_MASTER_SWORD)));
      if (custom) {
        assert(strstr(descriptor.dlists[0],
                      mode == 2 ? "din_fire_sword" : "object_custom_equip"));
        // Export the pickup's +X blade with one quarter-turn into upright +Y.
        assert(descriptor.opCount == 1 && descriptor.ops[0].op == CW_OP_ROTATE_Z && descriptor.ops[0].a == 16384.f &&
               descriptor.scale == .04f);
      } else if (id == RI_OOT_TRUE_MASTER_SWORD) {
        const uint8_t gold[4] = {255, 215, 110, 255};
        assert(!memcmp(descriptor.primColorOpa, gold, 4));
      }
      if (id == RI_OOT_TRUE_MASTER_SWORD)
        assert(descriptor.primColorXlu[3] && descriptor.primColorXlu[0] == 120);
      foreign.itemName = name;
      ++generation;
      const auto *first = ComboResolveForeignDrawInfo(202);
      assert(first && first->appearanceDependent && first->itemShimmer);
      assert(!memcmp(first->primColorOpa, descriptor.primColorOpa, 4));
      ComboLatchForeignDraw(202);
      ownerAltEnabled = !ownerAltEnabled;
      const auto *refreshed = ComboResolveForeignDrawInfo(202);
      assert(refreshed && refreshed->appearanceDependent &&
             refreshed->stateDependent);
      if (mode < 3)
        assert(refreshed->drawKind ==
               (mode == 0 ? CW_DRAW_KIND_CUSTOM_GI
                          : (bgs ? CW_DRAW_KIND_GORON_SWORD
                                 : CW_DRAW_KIND_MASTER_SWORD)));
    }
  }
  ownerNotReady = true;
  foreign.itemName = "Master Sword";
  ++generation;
  CwItemDrawInfo unavailable{};
  assert(MM_GetItemDrawInfo("Master Sword", &unavailable) ==
             CW_DRAW_NOT_READY &&
         !unavailable.dlistCount);
  assert(!ComboResolveForeignDrawInfo(202));
  ownerNotReady = false;
  assert(ComboResolveForeignDrawInfo(202));
  ownerAltEnabled = false;
  ownerDin = false;
  for (auto [id, name] :
       {std::pair{RI_SWORD_KOKIRI, "Concrete MM Kokiri"},
        std::pair{RI_SWORD_RAZOR, "Concrete MM Razor"},
        std::pair{RI_SWORD_GILDED, "Concrete MM Gilded"},
        std::pair{RI_GREAT_FAIRY_SWORD, "Concrete MM Great Fairy"}}) {
    Rando::StaticData::Items[id] = {0, name};
    CwItemDrawInfo descriptor{};
    assert(MM_GetItemDrawInfo(name, &descriptor) == 1 &&
           descriptor.stateDependent == 2);
  }
  for (auto [progressive, concrete] :
       {std::pair{RI_OOT_PROGRESSIVE_MASTER_SWORD, RI_OOT_MASTER_SWORD},
        std::pair{RI_OOT_PROGRESSIVE_BGS, RI_OOT_BIGGORON_SWORD}}) {
    ownerAltEnabled = true;
    ownerResources = {
        "__OTR__alt/objects/object_custom_equip/gCustomMasterSwordDL",
        "__OTR__alt/objects/object_custom_equip/gCustomLongswordDL"};
    progression = concrete;
    const char *name = Rando::StaticData::Items[progressive].name;
    CwItemDrawInfo descriptor{};
    assert(MM_GetItemDrawInfo(name, &descriptor) == 1 &&
           descriptor.stateDependent == 1);
    assert(descriptor.drawKind == CW_DRAW_KIND_CUSTOM_GI &&
           descriptor.resolvedName == Rando::StaticData::Items[concrete].name);
    foreign.itemName = name;
    ++generation;
    ComboLatchForeignDraw(203);
    progression = RI_TEST_COMPASS;
    ownerAltEnabled = false;
    const auto *frozen = ComboResolveForeignDrawInfo(203);
    assert(frozen && !frozen->stateDependent &&
           frozen->drawKind == CW_DRAW_KIND_CUSTOM_GI);
    assert(frozen->resolvedName == Rando::StaticData::Items[concrete].name);
  }
  puts("PASS imported sword export/cache: real GID_NONE aliases, native fallback, "
       "selected standalone/Din recipes, owner/module retry, concrete appearance "
       "refresh and progressive grant freeze.");
  for (auto [id, name] : { std::pair{ RI_OOT_NEI_LANTERN, "Lantern" },
                          std::pair{ RI_OOT_NEI_POKE_BALL, "Poké Ball" },
                          std::pair{ RI_OOT_NEI_MARIO_MASK, "Mario Mask" } }) {
    Rando::StaticData::Items[id] = { -1, name };
    ownerResources.insert("__OTR__direct_legacy_recipe");
    expectedDirectName = name;
    CwItemDrawInfo descriptor{};
    assert(MM_GetItemDrawInfo(name, &descriptor) == 1);
    assert(descriptor.drawKind == CW_DRAW_KIND_CUSTOM_GI && descriptor.stateDependent == 2);
    assert(!strcmp(descriptor.dlists[0], "__OTR__@oot:direct_legacy_recipe"));
  }
  expectedDirectName = nullptr;
  // The original progressive latch must remain frozen across grant-state
  // changes.
  Rando::StaticData::Items[RI_PROGRESSIVE_SWORD] = {0, "Progressive sword"};
  Rando::StaticData::Items[RI_TEST_SWORD] = {0, "First sword"};
  Rando::StaticData::Items[RI_TEST_COMPASS] = {3, "Next tier"};
  foreign.itemName = "Progressive sword";
  generation++;
  progression = RI_TEST_SWORD;
  const int rc = 99;
  const auto *before = ComboResolveForeignDrawInfo(rc);
  assert(before && before->stateDependent && !before->appearanceDependent &&
         before->resolvedName == "First sword");
  ComboLatchForeignDraw(rc);
  progression = RI_TEST_COMPASS;
  const auto *frozen = ComboResolveForeignDrawInfo(rc);
  assert(frozen && frozen->count == 1 && !frozen->stateDependent &&
         frozen->resolvedName == "First sword");
  assert(!strcmp(ComboForeignLatchedName(rc), "First sword"));
  const char *retained = frozen->dls[0];
  for (int i = 0; i < 1000; i++)
    ComboInternRoutedPath(std::string("__OTR__@mm:") + std::to_string(i));
  assert(strstr(retained, "@mm:objects/object_gi_key/"));
  gSaveContext.fileNum = 1;
  assert(ComboResolveForeignDrawInfo(rc)->resolvedName == "Next tier");
  generation++;
  progression = RI_TEST_SWORD;
  assert(ComboResolveForeignDrawInfo(rc)->resolvedName == "First sword");
  puts("PASS actual MM export -> OoT resolver/cache/latch -> OPS replay: all "
       "16 dungeon GIs, independent palettes without assets, live edits/Alt "
       "after grant, retained progressive tiers and cache sweeps.");
}
