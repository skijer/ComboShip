// Uses real MM item IDs, the production static/FC catalogs and receipt builder.
// The font/message table and donor service are the runtime boundaries.
#include "2s2h/CustomItem/CustomItem.h"
#include "2s2h/FleetShipCombo/FleetComboItems.h"
#include "2s2h/FleetShipCombo/FleetComboItemsGlue.h"
#include "2s2h/Rando/ItemReceiptText.h"
#include <nlohmann/json.hpp>
#include "2s2h/Rando/Rando.h"
#include "2s2h/Rando/StaticData/StaticData.h"
#include "ComboExport.h"
#include "ComboItemReceiptText.h"
#include "ComboSongDrawMM.h"
#include "ComboItemIconOwnership.h"
#include "key_receipt_fixtures.h"
#include "boss_soul_fixtures.h"
#include "rando/CrossForeign.h"
#include "rando/CrossDungeonSeed.h"
extern "C" {
#include "message_data_static.h"
#include "mods/extended_inventory.h"
#include "soh/Enhancements/randomizer/randomizerTypes.h"
}
#include "2s2h_assets.h"
#include "assets/interface/parameter_static/parameter_static.h"
#include "assets/interface/icon_item_dungeon_static/icon_item_dungeon_static.h"
#include "interface/icon_item_field_static/icon_item_field_static.h"
#include "assets/archives/icon_item_static/icon_item_static_yar.h"
#include "assets/archives/icon_item_24_static/icon_item_24_static_yar.h"
#include "receipt_catalogs.inc"
#include <cassert>
#include <cstring>
#include <iostream>
#include <unordered_map>

namespace OotNative {
enum {
#define DEFINE_ENTRANCE(entrance, ...) entrance,
#include "soh/include/tables/entrance_table.h"
#undef DEFINE_ENTRANCE
};
}

extern "C" {
PlayState *gPlayState = nullptr;
int gComboGameMode = COMBO_GAME_BOTH;

const NeiItem *Nei_FindByItem(int32_t item) {
  for (const auto &nei : sReceiptNeiItems)
    if (nei.item != NEI_NO_ITEM && nei.item == item)
      return &nei;
  return nullptr;
}
const NeiItem *Nei_FindByRg(int16_t id) {
  for (const auto &nei : sReceiptNeiItems)
    if (nei.rg != NEI_NO_RG && nei.rg == id)
      return &nei;
  return nullptr;
}
static NeiSaveData neiSave{};
NeiSaveData *Nei_Save() { return &neiSave; }
SaveContext gSaveContext{};

static const char* stagedIconPath;
static s16 stagedIconWidth;
void Message_StageCustomItemIcon(void* path, s16 width) {
    stagedIconPath = static_cast<const char*>(path);
    stagedIconWidth = width;
}
static u8 stagedColor[3];
static s16 stagedWidth, stagedHeight;
static u8 stagedIA8;
void Message_StageCustomItemIconTint(void*, s16 width, s16 height, u8 ia8, u8 r, u8 g, u8 b) {
    stagedWidth = width;
    stagedHeight = height;
    stagedIA8 = ia8;
    stagedColor[0] = r;
    stagedColor[1] = g;
    stagedColor[2] = b;
}
u8 gItemSlots[77]{};
u32 gBitFlags[32] = {1, 2, 4};
u8 Nei_BulletBagLevel() { return std::min<int>(3, neiSave.ootUpgrades & 7); }
}
#include "receipt_fairy_grants.inc"

namespace Rando::StaticData {

const std::string& GetCheckDisplayName(RandoCheckId id) {
  static const std::map<RandoCheckId, std::string> names = {
    {RC_WOODFALL_TEMPLE_BOSS_WARP, "Woodfall Temple Boss Warp"},
    {RC_SNOWHEAD_TEMPLE_BOSS_WARP, "Snowhead Temple Boss Warp"},
    {RC_GREAT_BAY_TEMPLE_BOSS_WARP, "Great Bay Temple Boss Warp"},
    {RC_STONE_TOWER_TEMPLE_INVERTED_BOSS_WARP, "Stone Tower Temple Inverted Boss Warp"}
  };
  return names.at(id);
}
}

namespace CustomMessage {
static Entry shownSong;
static bool startedSong;
static Entry nativeRupeeLoaded;
static int nativeRupeeLoads = 0;
Entry LoadVanillaMessageTableEntry(u16) {
  return {.textboxType=3, .textboxYPos=2, .icon=0xA7, .nextMessageID=0x1234,
          .firstItemCost=0x5678, .secondItemCost=0x9ABC, .msg="Native rupee receipt"};
}
void LoadCustomMessageIntoFont(Entry entry) { nativeRupeeLoaded=std::move(entry);++nativeRupeeLoads; }
void SetActiveCustomMessage(std::string msg, Entry options) { options.msg=std::move(msg);shownSong=options;startedSong=false; }
void StartTextbox(std::string msg, Entry options) { options.msg=std::move(msg);shownSong=options;startedSong=true; }
}
static int songGrants=0;
static u8 grantedSong;
extern "C" u8 Item_Give(PlayState* play, u8 item) {
  assert(play==gPlayState);++songGrants;grantedSong=item;return item;
}
#include "receipt_song_grants.inc"

static std::string requested;
static bool donorReady = true;
static int donorReads = 0;
static uint64_t generation = 0;
static bool mapCompassInfo = false;
static int randomRupeeNames = 0;
static int wandRule = 0;
static bool neiEnabled = true;
#ifdef COMPASS_DONOR_INTEGRATION
extern "C" void FixtureConfigureWandReceipt(int rule, int customItems);
#endif
static void SyncWandFixtureSettings() {
#ifdef COMPASS_DONOR_INTEGRATION
  // Production ComboSettingsSync mirrors these two options between hosts.
  FixtureConfigureWandReceipt(wandRule, neiEnabled);
#endif
}
extern "C" uint8_t Wand_RandoMode() { return wandRule; }
extern "C" int32_t CVarGetInteger(const char* name, int32_t fallback) {
  if (std::strcmp(name, "gRando.Options.RO_ELEMENTAL_WAND_SHUFFLE") == 0)
    return wandRule;
  if (std::strcmp(name, "gRando.Options.RO_SHUFFLE_NEI_ITEMS") == 0)
    return neiEnabled;
  if (std::strcmp(name, "gRandoEnhancements.RandomizeRupeeNames") == 0)
    return randomRupeeNames < 0 ? fallback : randomRupeeNames;
  return fallback;
}
using NativeRupeeHook = void (*)(u16*, bool*);
static std::map<u16,NativeRupeeHook> nativeRupeeHooks;
#define COND_ID_HOOK(kind,id,condition,callback) do { if(condition) nativeRupeeHooks[id]=callback; } while(0)
#include "receipt_rupee_hooks.inc"
#undef COND_ID_HOOK
static RandoCheckId foreignRewardCheck = RC_UNKNOWN;
static ComboRando::ForeignItem foreignReward;
namespace Rando::MiscBehavior {
uint64_t ComboRandoGen() { return generation; }
const ComboRando::ForeignItem* MM_LookupForeign(RandoCheckId check) {
  return check == foreignRewardCheck ? &foreignReward : nullptr;
}
} // namespace Rando::MiscBehavior
#ifndef COMPASS_DONOR_INTEGRATION
static bool nativeHintEntranceSwap;
extern "C" COMBO_EXPORT int32_t OOT_GetDungeonEntranceSourceName(int32_t index,char* buffer,uint32_t capacity) {
  assert(index == OotNative::ENTR_DEKU_TREE_ENTRANCE);
  const std::string name = nativeHintEntranceSwap ? "Forest Temple" : "Deku Tree";
  if (!buffer || name.size() >= capacity) return 0;
  std::memcpy(buffer,name.c_str(),name.size()+1);return name.size();
}
extern "C" COMBO_EXPORT int32_t OOT_GetPhysicalBossRewardName(int32_t boss,char* buffer,uint32_t capacity) {
  assert(boss == 0);
  const std::string name = "Deku Leaf";
  if (!buffer || name.size() >= capacity) return 0;
  std::memcpy(buffer,name.c_str(),name.size()+1);return name.size();
}
extern "C" COMBO_EXPORT int32_t OOT_GetPhysicalBossRewardPresentation(int32_t boss,CwItemReceiptPresentation* out) {
  assert(boss == 0);*out={};out->singleBox=1;out->rewardLine=1;
  std::strcpy(out->iconPath,"__OTR__@oot:textures/icon_item_custom/gItemIconDekuLeafTex");
  out->iconWidth=out->iconHeight=32;return 1;
}
extern "C" COMBO_EXPORT int32_t OOT_MapCompassInfoEnabled() { return mapCompassInfo; }
extern "C" COMBO_EXPORT int32_t OOT_GetSeedItemIconInfo(const char* name, CwItemIconInfo* out) {
  assert(std::string(name) == "Progressive Hookshot");
  *out = { "__OTR__textures/icon_item_static/gItemIconHookshotTex", 32, 32, 0, 0, {} };
  return 1;
}
#endif
#include "receipt_map_pause.inc"

static std::string SmallKeyDonorReceipt(const std::string& name) {
  for (int id = RI_OOT_SMALL_KEY_BOTTOM_OF_THE_WELL; id <= RI_OOT_SMALL_KEY_WATER_TEMPLE; ++id) {
    const int fc = FcCombo_ItemForNative(id);
    if (name != gFcComboItems[fc].ootName)
      continue;
    // Dungeon keys use SoH's concise named randomizer receipt. Chest Game
    // owns a separate native tutorial (0xF3), as before.
    if (id != RI_OOT_SMALL_KEY_TREASURE_GAME)
      return ComboItemReceiptText::FromNeiMarkup("You found a %g" + name + "%w!");
    const std::string native = "You got a Key!\x01It opens the next door in the Treasure Chest Game.\x02";
    std::string body;
    assert(ComboItemReceiptText::FromOotMessage(native, body));
    return body;
  }
  return {};
}

#ifndef COMPASS_DONOR_INTEGRATION
extern "C" COMBO_EXPORT int32_t OOT_GetItemReceiptText(const char *name,
                                                       char *buffer,
                                                       uint32_t capacity) {
  requested = name;
  if (!donorReady || requested == "Progressive Hookshot")
    return 0;
  ++donorReads;
  const std::string key = SmallKeyDonorReceipt(requested);
  const std::string body = !key.empty() ? key :
      requested == "Deku Leaf"
          ? ComboItemReceiptText::FromNeiMarkup(kDekuLeafMessage)
          : "Full description of " + requested + " #" +
                std::to_string(donorReads);
  assert(body.size() <= capacity);
  std::memcpy(buffer, body.data(), body.size());
  return body.size();
}
#endif

static std::string FlattenReceiptLines(const std::string& body) {
  std::string flat;
  for (char c : body) {
    if (c == '\x11')
      c = ' ';
    if (c != ' ' || flat.empty() || flat.back() != ' ')
      flat += c;
  }
  return flat;
}

#ifdef COMPASS_DONOR_INTEGRATION
extern "C" void FixtureConfigureForestCompassReceipt(int information);
extern "C" void FixtureConfigureGeneratedCompassRoute(const char* fixturePath, const char* route);
extern "C" void FixtureConfigurePhysicalBossHints(int nativeSwap);
extern "C" int32_t OOT_GetItemReceiptText(const char*,char*,uint32_t);
extern "C" int32_t OOT_GetDungeonItemReceiptPresentation(const char*,CwItemReceiptPresentation*);

static void CheckForestCompassReceiveRoutes(const char* fixturePath) {
  gSaveContext.fileNum = 0;
  gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
  const auto saveBefore = gSaveContext;
  const auto neiBefore = neiSave;
  // The second Off is another loaded seed, so an On receipt cannot survive in
  // the foreign-check cache after its seed generation changes.
  for (int information : {0, 1, 0}) {
    FixtureConfigureForestCompassReceipt(information);
    ++generation;
    CustomMessage::Entry native, foreign;
    native.icon = foreign.icon = 0xF5;
    assert(Rando::ApplyItemReceiptText(RI_OOT_COMPASS_FOREST_TEMPLE, native));
    assert(Rando::ApplyForeignItemReceiptText("Forest Temple Compass", foreign, RC_CLOCK_TOWER_ROOF_OCARINA));
    assert(!native.autoFormat && !foreign.autoFormat);
    assert(native.icon == (information ? 0xFE : 0xF5) && foreign.icon == native.icon);
    assert(native.msg == foreign.msg);
    const auto text = FlattenReceiptLines(native.msg);
    assert((text.find("You got the Compass!") != std::string::npos) == !information);
    assert((text.find("Now you can see hidden things.") != std::string::npos) == !information);
    assert(text.find("Forest Temple Compass") != std::string::npos);
    assert((text.find("masterful") != std::string::npos) == !information);
    assert((text.find("Phantom Ganon") != std::string::npos) == bool(information));
    assert(text.find("Defeating the boss grants the") == std::string::npos);
    assert((text.find("You received a ") != std::string::npos) == bool(information));
    assert(native.receiptPresentation.singleBox == information);
    if (information) {
      assert(native.msg.find('\x10') == std::string::npos);
      assert(std::count(native.msg.begin(), native.msg.end(), '\x11') == 1);
      assert(native.receiptPresentation.rewardLine == 1);
      assert(ComboReceipt_HasIcon(&native.receiptPresentation));
      assert(std::string(native.receiptPresentation.iconPath) == "__OTR__@oot:textures/icon_item_custom/gItemIconDekuLeafTex");
      assert(!std::memcmp(&native.receiptPresentation, &foreign.receiptPresentation, sizeof(native.receiptPresentation)));
    }
    const auto body = native.msg;
    Rando::AppendReceiptSource(native, " (Bank reward)");
    assert(native.msg.compare(0, body.size(), body) == 0);
    assert(native.msg.substr(body.size()) == "\x10 (Bank reward)\xBF");
    assert(!std::memcmp(&saveBefore, &gSaveContext, sizeof(saveBefore)));
    assert(!std::memcmp(&neiBefore, &neiSave, sizeof(neiBefore)));
  }
  if (fixturePath) {
    for (const auto* route : {"direct", "nested", "cycle", "deadEnd"}) {
      FixtureConfigureGeneratedCompassRoute(fixturePath, route);
      ++generation;
      CustomMessage::Entry native, foreign;
      assert(Rando::ApplyItemReceiptText(RI_OOT_COMPASS_FOREST_TEMPLE, native));
      assert(Rando::ApplyForeignItemReceiptText("Forest Temple Compass", foreign, RC_CLOCK_TOWER_ROOF_OCARINA));
      assert(native.msg == foreign.msg && !native.autoFormat && native.icon == 0xFE);
      const bool assigned = std::string(route) == "direct" || std::string(route) == "nested";
      assert((native.msg.find("Volvagia") != std::string::npos) == assigned);
      assert(native.msg.find("Fire Medallion") == std::string::npos);
      assert(native.msg.find("Phantom Ganon") == std::string::npos);
      assert(ComboReceipt_HasIcon(&native.receiptPresentation) == assigned);
      if (assigned) {
        assert(native.receiptPresentation.rewardLine == 1);
        assert(std::string(native.receiptPresentation.iconPath) == "__OTR__@oot:textures/icon_item_24_static/gQuestIconMedallionFireTex");
      }
      if (!assigned) assert(native.msg.find("boss room") != std::string::npos);
      assert(!std::memcmp(&saveBefore, &gSaveContext, sizeof(saveBefore)));
      assert(!std::memcmp(&neiBefore, &neiSave, sizeof(neiBefore)));
    }
  }
  std::cout << "Real OoT donor -> MM compass native/foreign routes: two-line boss/reward sprites, saved Off tutorials, cache reset and source append passed\n";
}
#endif

static void CheckDungeonKeyReceipts() {
  const auto previousSave = gSaveContext;
  const auto previousNei = neiSave;
  gSaveContext.fileNum = 0;
  gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
  for (bool donorAvailable : {true, false}) {
    donorReady = donorAvailable;
    for (int enabled : {0, 1}) {
      mapCompassInfo = enabled;
      assert(Rando::MapCompassInfoEnabled() == bool(enabled));
      for (const auto &fixture : KeyReceiptFixtures::entries) {
        const auto found = std::find_if(
            Rando::StaticData::Items.begin(), Rando::StaticData::Items.end(),
            [&](const auto &row) {
              return row.second.name &&
                     std::string_view(row.second.name) == fixture.name;
            });
        assert(found != Rando::StaticData::Items.end());
        const auto key = found->first;
        const int fc = FcCombo_ItemForNative(key);
        assert(fc >= 0);
        const char *name = fixture.name;
        const std::string expected = KeyReceiptFixtures::Expected(fixture);
        for (int count : {0, int(gFcComboItems[fc].chainLen)}) {
          neiSave.comboObtainedFc[fc] = count;
          const auto saveBefore = gSaveContext;
          const auto neiBefore = neiSave;
          CustomMessage::Entry concrete;
          concrete.icon = 0xF5;
          concrete.msg = "generic name-only receipt";
          requested.clear();
          const bool accepted = Rando::ApplyItemReceiptText(key, concrete);
          if (!accepted)
            std::cerr << "Missing dungeon-named MM key receipt: " << name
                      << '\n';
          assert(accepted && "dungeon key skipped the receipt builder");
          assert(!concrete.autoFormat && concrete.icon == 0xF5);
          if (FlattenReceiptLines(concrete.msg) != expected)
            std::cerr << "Incorrect key name/color: " << name << '\n';
          assert(FlattenReceiptLines(concrete.msg) == expected);
          // Direct foreign-sentinel composition must agree with the local item.
          CustomMessage::Entry foreign;
          foreign.icon = 0xF5;
          assert(Rando::ApplyForeignItemReceiptText(name, foreign));
          assert(foreign.msg == concrete.msg && !foreign.autoFormat);
          assert(concrete.msg.find("The reward is") == std::string::npos);
          assert(concrete.msg.find("Its entrance is") == std::string::npos);
          const std::string body = concrete.msg;
          Rando::AppendReceiptSource(concrete, " (Bank reward)");
          assert(concrete.msg.compare(0, body.size(), body) == 0);
          assert(concrete.msg.substr(body.size()) == "\x10 (Bank reward)\xBF");
          assert(std::memcmp(&gSaveContext, &saveBefore, sizeof(saveBefore)) ==
                 0);
          assert(std::memcmp(&neiSave, &neiBefore, sizeof(neiBefore)) == 0);
        }
      }
    }
  }
  gSaveContext = previousSave;
  neiSave = previousNei;
  mapCompassInfo = false;
  donorReady = true;
  CustomMessage::Entry excluded;
  excluded.msg = "original skeleton-key receipt";
  assert(!Rando::ApplyItemReceiptText(RI_SKELETON_KEY, excluded));
  assert(excluded.msg == "original skeleton-key receipt");
  assert(!Rando::ApplyItemReceiptText(RI_TRAP, CustomMessage::shownSong));
  std::cout
      << "All 34 OoT/MM key receipts retain dungeon names/colors in native and "
         "foreign MM routes, with donor on/off, counts and source append\n";
}

static void CheckMapCompassInformation() {
  const auto saved = gSaveContext;
  gSaveContext.fileNum = 0;
  gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
  mapCompassInfo = true;
  const RandoCheckId checks[] = { RC_WOODFALL_TEMPLE_BOSS_WARP, RC_SNOWHEAD_TEMPLE_BOSS_WARP,
      RC_GREAT_BAY_TEMPLE_BOSS_WARP, RC_STONE_TOWER_TEMPLE_INVERTED_BOSS_WARP };
  const RandoItemId rewards[] = { RI_PROGRESSIVE_SWORD, RI_SONG_NOVA, RI_COMBO_FOREIGN, RI_PROGRESSIVE_MAGIC };
  const RandoItemId compasses[] = { RI_WOODFALL_COMPASS, RI_SNOWHEAD_COMPASS, RI_GREAT_BAY_COMPASS, RI_STONE_TOWER_COMPASS };
  const RandoItemId maps[] = { RI_WOODFALL_MAP, RI_SNOWHEAD_MAP, RI_GREAT_BAY_MAP, RI_STONE_TOWER_MAP };
  const char* bosses[] = { "Odolwa", "Goht", "Gyorg", "Twinmold" };
  const char* entrances[] = { "Woodfall", "Snowhead", "Zora Cape's turtle", "Stone Tower" };
  foreignRewardCheck = checks[2];
  foreignReward.itemGame = ComboRando::GAME_OOT;
  foreignReward.itemName = "Progressive Hookshot";
  foreignReward.fakeItemName = "Light Arrows";
  for (int d = 0; d < 4; ++d) {
    auto& check = gSaveContext.save.shipSaveInfo.rando.randoSaveChecks[checks[d]];
    check = {};
    check.randoItemId = rewards[d];
    gSaveContext.save.saveInfo.inventory.dungeonItems[d] = 0;
    assert(!PauseItemDesc_GetMapInfo(d, ITEM_COMPASS));
    assert(!PauseItemDesc_GetMapInfo(d, ITEM_DUNGEON_MAP));
    // Start With ownership: no obtain history, visited flags, or receipt latch.
    gSaveContext.save.saveInfo.inventory.dungeonItems[d] = (1 << DUNGEON_COMPASS) | (1 << DUNGEON_MAP);
    const std::string expected = d == 2 ? "Progressive Hookshot (OOT)" : Rando::StaticData::Items.at(rewards[d]).name;
    const auto before = gSaveContext;
    const int reads = donorReads;
    char name[128];
    assert(MM_GetDungeonRewardName(d, name, sizeof(name)) == static_cast<int>(expected.size()));
    assert(name == expected);
    const auto info = Rando::GetDungeonMapCompassInfo(d, true);
    assert(info.find(bosses[d]) != std::string::npos && info.find(expected) != std::string::npos);
    assert(info.find("Light Arrows") == std::string::npos);
    assert(PauseItemDesc_GetMapInfo(d, ITEM_COMPASS) == info);
    const auto map = Rando::GetDungeonMapCompassInfo(d, false);
    assert(map.find(entrances[d]) != std::string::npos && map.find("entrance") != std::string::npos);
    assert(PauseItemDesc_GetMapInfo(d, ITEM_DUNGEON_MAP) == map);
    CustomMessage::Entry receipt;
    assert(Rando::ApplyItemReceiptText(compasses[d], receipt));
    assert(receipt.msg.find(bosses[d]) != std::string::npos && !receipt.autoFormat);
    assert(receipt.msg.find('\x10') == std::string::npos && std::count(receipt.msg.begin(), receipt.msg.end(), '\x11') == 1);
    assert(receipt.msg.find("You received a ") != std::string::npos);
    assert(receipt.msg.find("Defeating the boss grants the") == std::string::npos);
    assert(receipt.receiptPresentation.rewardLine == 1);
    assert(receipt.msg.find("Now you can see") == std::string::npos);
    assert(receipt.icon == 0xFE && ComboReceipt_HasIcon(&receipt.receiptPresentation));
    assert(std::string(receipt.receiptPresentation.iconPath).starts_with(d == 2 ? "__OTR__@oot:" : "__OTR__@mm:"));
    if (d == 0) assert(std::strstr(receipt.receiptPresentation.iconPath, "KokiriSword"));
    if (d == 1) assert(receipt.receiptPresentation.iconWidth == 16 && receipt.receiptPresentation.iconHeight == 24 && receipt.receiptPresentation.iconIA8);
    if (d == 2) assert(std::string(receipt.receiptPresentation.iconPath) == "__OTR__@oot:textures/icon_item_static/gItemIconHookshotTex");
    assert(Rando::ApplyItemReceiptText(maps[d], receipt));
    assert(receipt.receiptPresentation.singleBox && !ComboReceipt_HasIcon(&receipt.receiptPresentation));
    assert(receipt.msg.find('\x10') == std::string::npos && std::count(receipt.msg.begin(), receipt.msg.end(), '\x11') == 1);
    assert(receipt.msg.find("entrance") != std::string::npos);
    assert(donorReads == reads && !std::memcmp(&before, &gSaveContext, sizeof(before)));
  }
  // Raw placed names remain stable across live equipment tiers and acquired checks.
  gSaveContext.save.saveInfo.equips.equipment = 0xFFFF;
  gSaveContext.save.shipSaveInfo.rando.randoSaveChecks[checks[0]].obtained = true;
  assert(Rando::GetDungeonMapCompassInfo(0, true).find("Progressive Sword") != std::string::npos);
  char shortName[2] = {'x', 'x'};
  assert(MM_GetDungeonRewardName(0, shortName, sizeof(shortName)) == 0 && !shortName[0]);
  assert(MM_GetDungeonRewardName(-1, shortName, sizeof(shortName)) == 0);
  assert(MM_GetDungeonRewardName(4, shortName, sizeof(shortName)) == 0);
  assert(MM_GetDungeonRewardName(0, nullptr, 0) == 0);
  assert(!PauseItemDesc_GetMapInfo(4, ITEM_COMPASS));
  assert(!PauseItemDesc_GetMapInfo(0, ITEM_KEY_BOSS));
  foreignRewardCheck = RC_UNKNOWN;
  assert(Rando::GetDungeonMapCompassInfo(2, true).empty()); // Missing foreign data never names the sentinel.
  CustomMessage::Entry missingReward;
  assert(Rando::ApplyItemReceiptText(RI_GREAT_BAY_COMPASS, missingReward));
  assert(missingReward.msg.find("Gyorg") != std::string::npos);
  assert(std::count(missingReward.msg.begin(), missingReward.msg.end(), '\x11') == 1);
  assert(missingReward.msg.find("Defeating the boss grants") == std::string::npos);
  assert(!ComboReceipt_HasIcon(&missingReward.receiptPresentation) &&
         missingReward.receiptPresentation.rewardLine == 1);
  mapCompassInfo = false;
  for (int d = 0; d < 4; ++d) {
    assert(Rando::GetDungeonMapCompassInfo(d, true).empty());
    assert(Rando::GetDungeonMapCompassInfo(d, false).empty());
    assert(!PauseItemDesc_GetMapInfo(d, ITEM_COMPASS));
  }
  mapCompassInfo = true;
  gSaveContext.fileNum = 0xFF;
  assert(!Rando::MapCompassInfoEnabled() && MM_GetDungeonRewardName(0, shortName, sizeof(shortName)) == 0);
  nlohmann::json seed = {
    {"mm", {{"placements", {{"Woodfall Temple Boss Warp", "Deku Leaf"}}}}},
    {"foreign", nlohmann::json::array({{{"checkGame", "mm"}, {"checkName", "Woodfall Temple Boss Warp"},
                                      {"itemGame", "oot"}, {"itemName", "Deku Leaf"}}})}
  };
  ComboRando::Combo_SetForeignJson(seed.dump().c_str());
  ++generation;
  char dormantName[128];
  assert(MM_GetDungeonRewardName(0, dormantName, sizeof(dormantName)) > 0);
  assert(!strcmp(dormantName, "Deku Leaf (OOT)"));
  seed["foreign"][0]["itemName"] = "Poké Ball";
  seed["foreign"][0]["shared"] = true;
  seed["foreign"][0]["fakeItemName"] = "Light Arrows";
  ComboRando::Combo_SetForeignJson(seed.dump().c_str());
  ++generation;
  assert(MM_GetDungeonRewardName(0, dormantName, sizeof(dormantName)) == std::strlen("Poké Ball"));
  assert(!strcmp(dormantName, "Poké Ball")); // UTF-8 bytes, actual name and shared ownership survive cache refresh.
  assert(MM_GetDungeonRewardName(1, dormantName, sizeof(dormantName)) == 0 && !dormantName[0]);
  ComboRando::Combo_SetForeignJson("{malformed");
  ++generation;
  assert(MM_GetDungeonRewardName(0, dormantName, sizeof(dormantName)) == 0 && !dormantName[0]);
  ComboRando::Combo_SetForeignJson(nullptr);
  ++generation;
  gSaveContext = saved;
  mapCompassInfo = false;
  std::cout << "MM map/compass information: four placed rewards, foreign owner names, native entrances, owned pause/Start With, gates and no save writes passed\n";
}

static void CheckShuffledDungeonInformation() {
  using namespace ComboRando::CrossDungeon;
  const auto saved = gSaveContext;
  const auto seedBefore = ComboRando::g_comboForeignJson;
  gSaveContext.fileNum = 0;
  gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
  mapCompassInfo = true;
#ifdef COMPASS_DONOR_INTEGRATION
  FixtureConfigurePhysicalBossHints(0);
#endif
  std::vector<Dungeon> dungeons = {
    {"mm:woodfall", {1, ENTRANCE(WOODFALL_TEMPLE,0)}, {1, ENTRANCE(WOODFALL,1)}, {1, ENTRANCE(WOODFALL,1)}},
    {"mm:snowhead", {1, ENTRANCE(SNOWHEAD_TEMPLE,0)}, {1, ENTRANCE(SNOWHEAD,1)}, {1, ENTRANCE(SNOWHEAD,1)}}
  };
  nlohmann::json seed = {{"crossDungeonEntrances", Encode(dungeons,{1,0})}};
  ComboRando::Combo_SetForeignJson(seed.dump().c_str());
  ++generation;
  for (int dungeon : {0,1}) {
    const std::string source = dungeon == 0 ? "Snowhead" : "Woodfall";
    const auto before = gSaveContext;
    const auto text = Rando::GetDungeonMapCompassInfo(dungeon,false);
    assert(text.find("at " + source) != std::string::npos && "MM map must name saved shuffled entrance");
    assert(!std::memcmp(&before,&gSaveContext,sizeof(before)));
    gSaveContext.save.saveInfo.inventory.dungeonItems[dungeon] |= 1 << DUNGEON_MAP;
    const auto owned = gSaveContext;
    assert(PauseItemDesc_GetMapInfo(dungeon,ITEM_DUNGEON_MAP) == text);
    CustomMessage::Entry receipt;
    assert(Rando::ApplyItemReceiptText(dungeon == 0 ? RI_WOODFALL_MAP : RI_SNOWHEAD_MAP,receipt));
    assert(FlattenReceiptLines(receipt.msg).find(source) != std::string::npos);
    assert(!std::memcmp(&owned,&gSaveContext,sizeof(owned)));
    gSaveContext = before;
  }
  dungeons.push_back({"oot:" + std::to_string(OotNative::ENTR_DEKU_TREE_ENTRANCE),
                     {0,OotNative::ENTR_DEKU_TREE_ENTRANCE}, {0,0x209}, {0,0x209}});
  seed["crossDungeonEntrances"] = Encode(dungeons,{1,2,0});
  ComboRando::Combo_SetForeignJson(seed.dump().c_str());++generation;
  assert(Rando::GetDungeonMapCompassInfo(0,false).find("at Deku Tree") != std::string::npos);
#ifdef COMPASS_DONOR_INTEGRATION
  FixtureConfigurePhysicalBossHints(1);
#else
  nativeHintEntranceSwap = true;
#endif
  assert(Rando::GetDungeonMapCompassInfo(0,false).find("at Forest Temple") != std::string::npos &&
         "physical source composes native OoT shuffle before the saved cross route");
#ifdef COMPASS_DONOR_INTEGRATION
  char importedMap[2048];
  const int importedMapSize=OOT_GetItemReceiptText("Woodfall Map",importedMap,sizeof(importedMap));
  assert(importedMapSize>0 && std::string(importedMap,importedMapSize).find("Forest Temple")!=std::string::npos);
  CustomMessage::Entry foreignMap;
  assert(Rando::ApplyForeignItemReceiptText("Great Deku Tree Map",foreignMap));
  assert(FlattenReceiptLines(foreignMap.msg).find("Snowhead")!=std::string::npos &&
         "OoT map received in MM must invert the recorded cross route even with native shuffle Off");
#endif
#ifdef COMPASS_DONOR_INTEGRATION
  FixtureConfigurePhysicalBossHints(0);
#else
  nativeHintEntranceSwap = false;
#endif

  const int entries[] = {ENTRANCE(WOODFALL_TEMPLE,0),ENTRANCE(SNOWHEAD_TEMPLE,0),
                         ENTRANCE(GREAT_BAY_TEMPLE,0),ENTRANCE(STONE_TOWER_TEMPLE_INVERTED,0)};
  const int arenas[] = {ENTRANCE(ODOLWAS_LAIR,0),ENTRANCE(GOHTS_LAIR,0),
                        ENTRANCE(GYORGS_LAIR,0),ENTRANCE(TWINMOLDS_LAIR,0)};
  const char* owners[] = {"mm:woodfall","mm:snowhead","mm:greatbay","mm:inverted"};
  const char* checks[] = {"Woodfall Temple Boss Warp","Snowhead Temple Boss Warp",
                         "Great Bay Temple Boss Warp","Stone Tower Temple Inverted Boss Warp"};
  std::vector<Dungeon> bosses;
  for(int i=0;i<4;++i) {
    Dungeon b{"mm:boss:"+std::to_string(i+8),{1,arenas[i]},{1,entries[i]},{1,entries[i]}};
    b.pool=Pool::Boss;b.boss=i+8;b.owner=owners[i];b.hasReverse=false;b.warpReturn=Endpoint{1,entries[i]};
    bosses.push_back(b);
    seed["mm"]["placements"][checks[i]]=Rando::StaticData::Items.at(RI_PROGRESSIVE_MAGIC).name;
  }
  Dungeon gohma{"oot:boss:0",{0,OotNative::ENTR_DEKU_TREE_BOSS_ENTRANCE},
                 {0,OotNative::ENTR_DEKU_TREE_BOSS_DOOR},{0,OotNative::ENTR_DEKU_TREE_ENTRANCE}};
  gohma.pool=Pool::Boss;gohma.boss=0;gohma.owner="oot:"+std::to_string(OotNative::ENTR_DEKU_TREE_ENTRANCE);
  gohma.warpReturn=Endpoint{0,OotNative::ENTR_KOKIRI_FOREST_DEKU_TREE_BLUE_WARP};bosses.push_back(gohma);
  seed["crossDungeonEntrances"]=Encode(bosses,{3,4,2,0,1});
  ComboRando::Combo_SetForeignJson(seed.dump().c_str());++generation;
  const auto compass = Rando::GetDungeonMapCompassInfo(0,true);
  assert(compass.find("Twinmold") != std::string::npos && compass.find("Odolwa") == std::string::npos);
  assert(compass.find(Rando::StaticData::Items.at(RI_PROGRESSIVE_MAGIC).name) != std::string::npos);
  gSaveContext.save.saveInfo.inventory.dungeonItems[0] |= 1 << DUNGEON_COMPASS;
  assert(PauseItemDesc_GetMapInfo(0,ITEM_COMPASS) == compass);
  const auto other = Rando::GetDungeonMapCompassInfo(1,true);
  assert(other.find("Queen Gohma") != std::string::npos && other.find("Deku Leaf") != std::string::npos);
  CustomMessage::Entry receipt;
  assert(Rando::ApplyItemReceiptText(RI_SNOWHEAD_COMPASS,receipt));
  assert(FlattenReceiptLines(receipt.msg).find("Queen Gohma") != std::string::npos);
  assert(std::string(receipt.receiptPresentation.iconPath)=="__OTR__@oot:textures/icon_item_custom/gItemIconDekuLeafTex");
  assert(receipt.receiptPresentation.rewardLine==1);
#ifdef COMPASS_DONOR_INTEGRATION
  char importedCompass[2048];
  const int importedCompassSize=OOT_GetItemReceiptText("Snowhead Compass",importedCompass,sizeof(importedCompass));
  assert(importedCompassSize>0 && std::string(importedCompass,importedCompassSize).find("Queen Gohma")!=std::string::npos);
  CwItemReceiptPresentation importedPresentation{};
  assert(OOT_GetDungeonItemReceiptPresentation("Snowhead Compass",&importedPresentation)==1);
  assert(!std::strcmp(importedPresentation.iconPath,receipt.receiptPresentation.iconPath));
  CustomMessage::Entry foreignCompass;
  assert(Rando::ApplyForeignItemReceiptText("Great Deku Tree Compass",foreignCompass));
  assert(FlattenReceiptLines(foreignCompass.msg).find("Goht")!=std::string::npos &&
         "OoT compass received in MM must name the assigned cross-game physical boss");
  assert(std::strstr(foreignCompass.receiptPresentation.iconPath,"gQuestIconSmallMagicJarTex"));
  FixtureConfigurePhysicalBossHints(2);
  ++generation;
  assert(Rando::ApplyForeignItemReceiptText("Forest Temple Compass",foreignCompass));
  assert(FlattenReceiptLines(foreignCompass.msg).find("Goht")!=std::string::npos &&
         "native boss override must compose before the cross-game boss permutation");
  gComboGameMode=COMBO_GAME_OOT;
  const int soloSize=OOT_GetItemReceiptText("Forest Temple Compass",importedCompass,sizeof(importedCompass));
  assert(soloSize>0 && std::string(importedCompass,soloSize).find("Queen Gohma")!=std::string::npos &&
         "solo OoT keeps its native boss override while ignoring dormant combined seed data");
  gComboGameMode=COMBO_GAME_BOTH;
  auto mixed=dungeons;
  mixed.insert(mixed.end(),bosses.begin(),bosses.end());
  seed["crossDungeonEntrances"]=Encode(mixed,{1,2,0,6,7,5,3,4});
  ComboRando::Combo_SetForeignJson(seed.dump().c_str());++generation;
  FixtureConfigurePhysicalBossHints(3);
  assert(Rando::ApplyForeignItemReceiptText("Forest Temple Compass",foreignCompass));
  assert(FlattenReceiptLines(foreignCompass.msg).find("Twinmold")!=std::string::npos &&
         "a native mixed route into a dungeon must cross to its assigned temple before following that boss door");
  FixtureConfigurePhysicalBossHints(0);
#endif
  std::vector<Dungeon> expanded=dungeons;
  expanded.push_back({"mm:stonetower",{1,ENTRANCE(STONE_TOWER_TEMPLE,0)},
                       {1,ENTRANCE(STONE_TOWER,2)},{1,ENTRANCE(STONE_TOWER,2)}});
  expanded.push_back({"mm:inverted",{1,ENTRANCE(STONE_TOWER_TEMPLE_INVERTED,0)},
                       {1,ENTRANCE(STONE_TOWER_INVERTED,1)},{1,ENTRANCE(STONE_TOWER_INVERTED,1)}});
  expanded.push_back({"mm:swamp-spider-house",{1,ENTRANCE(SWAMP_SPIDER_HOUSE,0)},
                       {1,ENTRANCE(SOUTHERN_SWAMP_POISONED,8)},{1,ENTRANCE(SOUTHERN_SWAMP_POISONED,8)}});
  expanded.back().expanded=true;
  seed["crossDungeonEntrances"]=Encode(expanded,{5,3,2,0,1,4});
  ComboRando::Combo_SetForeignJson(seed.dump().c_str());++generation;
  const auto tower=Rando::GetDungeonMapCompassInfo(3,false);
  assert(tower.find("normal: Snowhead; inverted: Southern Swamp's Spider House")!=std::string::npos);
  CustomMessage::Entry towerReceipt;
  assert(Rando::ApplyItemReceiptText(RI_STONE_TOWER_MAP,towerReceipt));
  const auto flatTower=FlattenReceiptLines(towerReceipt.msg);
  assert(flatTower.find("normal: Snowhead")!=std::string::npos && flatTower.find("inverted: Southern Swamp's Spider House")!=std::string::npos);
  char exportedSource[512], exportedBoss[64];
  assert(MM_GetDungeonEntranceName(3,exportedSource,sizeof(exportedSource))>0);
  assert(tower.find(exportedSource)!=std::string::npos);
  assert(MM_GetDungeonBossName(3,exportedBoss,sizeof(exportedBoss))==8 && !std::strcmp(exportedBoss,"Twinmold"));
  char tooShort[2]={'x','x'};
  assert(MM_GetDungeonEntranceName(3,tooShort,sizeof(tooShort))==0 && !tooShort[0]);
  assert(MM_GetDungeonBossName(-1,tooShort,sizeof(tooShort))==0 && !tooShort[0]);
  assert(MM_GetDungeonEntranceName(0,nullptr,0)==0);
  seed.erase("crossDungeonEntrances");
  ComboRando::Combo_SetForeignJson(seed.dump().c_str());++generation;
  assert(Rando::GetDungeonMapCompassInfo(3,false).find("at Stone Tower")!=std::string::npos);
  // A new seed resets descriptors even if no scene, slot, or inventory changes.
  seed["crossDungeonEntrances"]=Encode(dungeons,{0,1,2});
  ComboRando::Combo_SetForeignJson(seed.dump().c_str());++generation;
  assert(Rando::GetDungeonMapCompassInfo(0,false).find("at Woodfall") != std::string::npos);
  assert(Rando::GetDungeonMapCompassInfo(0,true).find("Odolwa") != std::string::npos);
  seed["crossDungeonEntrances"]["permutation"]={0,0,0};
  ComboRando::Combo_SetForeignJson(seed.dump().c_str());++generation;
  assert(Rando::GetDungeonMapCompassInfo(0,false).empty());
  assert(Rando::GetDungeonMapCompassInfo(0,true).empty());
  gComboGameMode=COMBO_GAME_MM;
  gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[RO_MAPS_COMPASSES_GIVE_INFORMATION]=1;
  gSaveContext.save.shipSaveInfo.rando.randoSaveChecks[RC_WOODFALL_TEMPLE_BOSS_WARP].randoItemId=RI_PROGRESSIVE_SWORD;
  assert(Rando::GetDungeonMapCompassInfo(0,false).find("at Woodfall") != std::string::npos);
  assert(Rando::GetDungeonMapCompassInfo(0,true).find(Rando::StaticData::Items.at(RI_PROGRESSIVE_SWORD).name) != std::string::npos);
  const RandoItemId nativeMaps[]={RI_WOODFALL_MAP,RI_SNOWHEAD_MAP,RI_GREAT_BAY_MAP,RI_STONE_TOWER_MAP};
  const RandoItemId nativeCompasses[]={RI_WOODFALL_COMPASS,RI_SNOWHEAD_COMPASS,RI_GREAT_BAY_COMPASS,RI_STONE_TOWER_COMPASS};
  const RandoCheckId nativeRewards[]={RC_WOODFALL_TEMPLE_BOSS_WARP,RC_SNOWHEAD_TEMPLE_BOSS_WARP,
      RC_GREAT_BAY_TEMPLE_BOSS_WARP,RC_STONE_TOWER_TEMPLE_INVERTED_BOSS_WARP};
  for(int enabled:{0,1,0}) {
    gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[RO_MAPS_COMPASSES_GIVE_INFORMATION]=enabled;
    for(int d=0;d<4;++d) {
      gSaveContext.save.shipSaveInfo.rando.randoSaveChecks[nativeRewards[d]].randoItemId=RI_PROGRESSIVE_SWORD;
      gSaveContext.save.saveInfo.inventory.dungeonItems[d]=(1<<DUNGEON_MAP)|(1<<DUNGEON_COMPASS);
      const auto before=gSaveContext;
      const auto map=Rando::GetDungeonMapCompassInfo(d,false);
      const auto compass=Rando::GetDungeonMapCompassInfo(d,true);
      assert(map.empty()==!enabled && compass.empty()==!enabled);
      assert(bool(PauseItemDesc_GetMapInfo(d,ITEM_DUNGEON_MAP))==bool(enabled));
      assert(bool(PauseItemDesc_GetMapInfo(d,ITEM_COMPASS))==bool(enabled));
      CustomMessage::Entry mapReceipt,compassReceipt;
      const bool mapHandled=Rando::ApplyItemReceiptText(nativeMaps[d],mapReceipt);
      const bool compassHandled=Rando::ApplyItemReceiptText(nativeCompasses[d],compassReceipt);
      if(enabled) {
        assert(mapHandled && compassHandled);
        assert(mapReceipt.msg.find("entrance")!=std::string::npos);
        assert(compassReceipt.msg.find("It points to")!=std::string::npos);
        assert(std::strstr(compassReceipt.receiptPresentation.iconPath,"KokiriSword"));
        assert(compass.find("Progressive Sword")!=std::string::npos);
      } else {
        assert(!mapReceipt.receiptPresentation.singleBox && !compassReceipt.receiptPresentation.singleBox);
        assert(mapReceipt.msg.find("It seems the entrance") == std::string::npos);
        assert(compassReceipt.msg.find("It points to") == std::string::npos);
      }
      assert(!std::memcmp(&before,&gSaveContext,sizeof(before)));
    }
  }
  std::cout<<"PASS solo 2Ship: all four native map/compass collection and owned-pause paths, saved Off/On/Off, physical rewards, stale combined seed ignored and save isolation\n";
  gComboGameMode=COMBO_GAME_BOTH;
  ComboRando::Combo_SetForeignJson(seedBefore.c_str());
  ++generation;
  gSaveContext = saved;
  mapCompassInfo = false;
  std::cout << "PASS MM information: saved map/boss permutations, native composition, physical rewards/icons, collection/C-Up, reload, malformed and solo ownership\n";
}

static void CheckRandomRupeeReceipts() {
  const auto saved = gSaveContext;
  gSaveContext.fileNum = 0;
  gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
  const struct { RandoItemId id; const char* name; const char* amount; unsigned color; } cases[] = {
    {RI_RUPEE_GREEN, "Green Rupee", "1", 2}, {RI_RUPEE_BLUE, "Blue Rupee", "5", 3},
    {RI_RUPEE_RED, "Red Rupee", "20", 1}, {RI_RUPEE_PURPLE, "Purple Rupee", "50", 6},
    {RI_RUPEE_SILVER, "Silver Rupee", "100", 0}, {RI_RUPEE_HUGE, "Huge Rupee", "200", 4}
  };
  for (int enabled : {-1, 1}) {
    randomRupeeNames = enabled;
    for (auto [id, name, amount, color] : cases) {
      for (auto lang : {LANGUAGE_ENG, LANGUAGE_GER, LANGUAGE_FRE}) {
        gSaveContext.options.language = lang;
        for (bool foreign : {false, true}) {
          const auto before = gSaveContext;
          CustomMessage::Entry receipt;
          receipt.icon = 0xF5;
          const bool applied = foreign ? Rando::ApplyForeignItemReceiptText(name, receipt)
                                       : Rando::ApplyItemReceiptText(id, receipt);
          assert(applied && "rupee enhancement must work without the donor/message table");
          const std::string value = std::string(1, static_cast<char>(color)) + amount;
          assert(!receipt.autoFormat && receipt.msg.find(value) != std::string::npos);
          assert(receipt.msg.find(name) == std::string::npos);
          assert(receipt.msg.find("%") == std::string::npos);
          assert(receipt.msg.find("[P]") == std::string::npos);
          assert(receipt.icon == 0xF5 && !receipt.receiptPresentation.singleBox);
          assert(!std::memcmp(&gSaveContext, &before, sizeof(before)));
        }
      }
    }
  }
  // Disabled and non-randomizer modes retain the ordinary native/donor path.
  donorReady = false;
  randomRupeeNames = 0;
  CustomMessage::Entry receipt;
  assert(!Rando::ApplyItemReceiptText(RI_RUPEE_BLUE, receipt));
  assert(!Rando::ApplyForeignItemReceiptText("Blue Rupee", receipt));
  randomRupeeNames = 1;
  gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_VANILLA;
  assert(!Rando::ApplyItemReceiptText(RI_RUPEE_BLUE, receipt));
  assert(!Rando::ApplyForeignItemReceiptText("Blue Rupee", receipt));
  gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
  assert(!Rando::ApplyForeignItemReceiptText("Silver Rupee (Forest Temple)", receipt));
  donorReady = true;
  randomRupeeNames = 0;
  gSaveContext = saved;
  std::cout << "Localized random rupee receipts: native/foreign values, live/default setting, modes and non-currency exclusion passed\n";
}

static void CheckNativeRupeeHooks() {
  RegisterNativeRandomRupeeNames();
  assert(nativeRupeeHooks.size()==7 && "native repeat/minigame rupee boxes need the enhancement too");
  const struct {u16 text; const char* amount; unsigned color;} cases[] = {
    {0xC4,"1",2},{0x2,"5",3},{0x3,"10",3},{0x4,"20",1},
    {0x5,"50",6},{0x6,"100",0},{0x7,"200",4}
  };
  const auto saved=gSaveContext;
  const auto* savedTable=gPlayState->msgCtx.messageTableNES;
  MessageTableEntry table[]={{0xFFFF}};
  gPlayState->msgCtx.messageTableNES=table;
  gSaveContext.fileNum=0;
  gSaveContext.save.shipSaveInfo.saveType=SAVETYPE_RANDO;
  gSaveContext.options.language=LANGUAGE_ENG;
  randomRupeeNames=1;
  for(auto [id,amount,color]:cases) {
    assert(nativeRupeeHooks.count(id));
    auto text=id;bool load=true;
    const auto before=gSaveContext;
    nativeRupeeHooks[id](&text,&load);
    const auto& receipt=CustomMessage::nativeRupeeLoaded;
    assert(!load && text==id && !receipt.autoFormat);
    assert(receipt.msg.find(std::string(1,char(color))+amount)!=std::string::npos && receipt.msg.back()==char(0xBF));
    assert(receipt.textboxType==3 && receipt.textboxYPos==2 && receipt.icon==0xA7 &&
           receipt.nextMessageID==0x1234 && receipt.firstItemCost==0x5678 && receipt.secondItemCost==0x9ABC);
    assert(!std::memcmp(&gSaveContext,&before,sizeof(before)));
  }
  // Unrelated dialogue never registers, and live guards leave native loading intact.
  assert(!nativeRupeeHooks.count(0x1C14) && !nativeRupeeHooks.count(CUSTOM_MESSAGE_ID));
  const int before=CustomMessage::nativeRupeeLoads;
  for(int guard=0;guard<4;++guard) {
    auto id=u16(0x4);bool load=guard!=0;
    randomRupeeNames=guard==1?0:1;
    gSaveContext.fileNum=guard==2?0xFF:0;
    gSaveContext.save.shipSaveInfo.saveType=guard==3?SAVETYPE_VANILLA:SAVETYPE_RANDO;
    nativeRupeeHooks.at(id)(&id,&load);
    assert(load==(guard!=0) && CustomMessage::nativeRupeeLoads==before);
  }
  randomRupeeNames=0;
  gSaveContext=saved;
  gPlayState->msgCtx.messageTableNES=const_cast<MessageTableEntry*>(savedTable);
  std::cout<<"Native MM rupee award hooks: seven IDs including10, live guards, header/icon/END and save isolation passed\n";
}

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  PlayState play{};
  gPlayState = &play;
  const struct {
    RandoItemId id;
    const char *name;
    int color;
  } souls[] = {
#define SOUL_ROW(mm, oot, name, color) {mm, name, color},
      BOSS_SOUL_RECEIPT_FIXTURES(SOUL_ROW)
#undef SOUL_ROW
  };
  const auto soulSaveBefore = gSaveContext;
  const auto soulNeiBefore = neiSave;
  for (int mode : {COMBO_GAME_MM, COMBO_GAME_BOTH}) {
    gComboGameMode = mode;
    for (bool ready : {false, true}) {
      donorReady = ready;
      for (int language : {LANGUAGE_ENG, LANGUAGE_GER, LANGUAGE_FRE}) {
        gSaveContext.options.language = language;
        for (const auto &soul : souls) {
          const auto before = gSaveContext;
          const int reads = donorReads;
          CustomMessage::Entry native, imported;
          native.icon = imported.icon = 0xF5;
          assert(Rando::ApplyItemReceiptText(soul.id, native) &&
                 "boss soul fell through to white queue text");
          assert(Rando::ApplyForeignItemReceiptText(soul.name, imported));
          for (const auto *entry : {&native, &imported}) {
            const auto body = FlattenReceiptLines(entry->msg);
            const auto color = body.find(static_cast<char>(soul.color));
            const auto reset = body.find('\0', color);
            assert(!entry->autoFormat && entry->icon == 0xF5);
            assert(color != std::string::npos && reset > color + 3 &&
                   reset != std::string::npos);
            if (language == LANGUAGE_ENG)
              assert(body.find(std::string(1, static_cast<char>(soul.color)) +
                               soul.name + std::string(1, '\0')) !=
                     std::string::npos);
            else
              assert(body.find(language == LANGUAGE_GER
                                   ? "Du hast "
                                   : "Vous avez") != std::string::npos);
          }
          assert(native.msg == imported.msg && donorReads == reads &&
                 "boss soul text depends on the donor module");
          assert(std::memcmp(&before, &gSaveContext, sizeof(before)) == 0);
        }
      }
    }
  }
  assert(std::memcmp(&soulNeiBefore, &neiSave, sizeof(neiSave)) == 0);
  gSaveContext = soulSaveBefore;
  gComboGameMode = COMBO_GAME_BOTH;
  donorReady = true;
  std::cout << "Boss soul native/foreign receipts: 14 identities, three "
               "locales, cold/warm donor, solo/combined passed\n";
  // Cosmetic masks retain their OoT descriptions; Gerudo/Keaton teach their forms.
  // MM-first pickups must not depend on a hydrated OoT module or mutate saves.
  const struct { RandoItemId id; const char* name; const char* detail; } masks[] = {
    {RI_OOT_MASK_SKULL, "Skull Mask", "monster"},
    {RI_OOT_MASK_SPOOKY, "Spooky Mask", "scare"},
    {RI_MASK_TRUTH, "Mask of Truth", "Show it"},
    {RI_OOT_MASK_GERUDO, "Gerudo Mask", "Urbosa"},
    {RI_MASK_KEATON, "Keaton Mask", "reflect"},
  };
  for (bool ready : {false, true}) {
    donorReady = ready;
    for (int language : {LANGUAGE_ENG, LANGUAGE_GER, LANGUAGE_FRE}) {
      gSaveContext.options.language = language;
      for (const auto& mask : masks) {
        CustomMessage::Entry native, foreign;
        native.icon = foreign.icon = 0xF5;
        const auto saveBefore = gSaveContext;
        const auto neiBefore = neiSave;
        const int readsBefore = donorReads;
        assert(Rando::ApplyItemReceiptText(mask.id, native) && "mask still uses generic receipt");
        assert(Rando::ApplyForeignItemReceiptText(mask.name, foreign));
        assert(native.msg == foreign.msg && !native.autoFormat && !foreign.autoFormat);
        assert(native.icon == 0xF5 && foreign.icon == 0xF5);
        assert(native.msg.find('\xB2') != std::string::npos && "missing MM C glyph");
        assert(native.msg.find('%') == std::string::npos && native.msg.size() <= 1269);
        if (language == LANGUAGE_ENG) assert(native.msg.find(mask.detail) != std::string::npos);
        if (mask.id == RI_MASK_KEATON) {
          CustomMessage::Entry imported;
          imported.icon = native.icon; // Compare the same header-icon wrapping width.
          assert(Rando::ApplyForeignItemReceiptText("Keaton Mask (MM)", imported));
          assert(imported.msg == native.msg && "Keaton copies have different tutorials");
          for (char glyph : {'\xB0', '\xB1', '\xB4'})
            assert(native.msg.find(glyph) != std::string::npos);
          if (language == LANGUAGE_ENG) {
            for (const char* move : {"punch", "charge", "jump", "kick", "climb", "magic"})
              assert(native.msg.find(move) != std::string::npos && "missing Keaton instruction");
          }
        }
        unsigned lines = 1;
        for (char c : native.msg) {
          if (c == '\x10') lines = 1;
          if (c == '\x11') assert(++lines <= 3 && "mask receipt exceeds MM line offsets");
        }
        assert(!std::memcmp(&saveBefore, &gSaveContext, sizeof(saveBefore)));
        assert(!std::memcmp(&neiBefore, &neiSave, sizeof(neiBefore)));
        assert(donorReads == readsBefore && "mask receipt queried dormant OoT");
      }
    }
  }
  gSaveContext.options.language = LANGUAGE_ENG;
  std::cout << "PASS mask receipts: native/foreign, locales, cold/warm donor, glyphs and save isolation\n";
  // Utility tutorials must work in an MM-first seed without querying OoT.
  const struct { RandoItemId id; const char* name; char color; char extraButton; unsigned pages; } tools[] = {
    {RI_OOT_NEI_PHANTOM_HOURGLASS, "Phantom Hourglass", '\x04', '\xB4', 4},
    {RI_OOT_NEI_SHADOW_CRYSTAL, "Shadow Crystal", '\x06', '\xB0', 3},
  };
  for (bool ready : {false, true}) {
    donorReady = ready;
    for (int language : {LANGUAGE_ENG, LANGUAGE_GER, LANGUAGE_FRE}) {
      gSaveContext.options.language = language;
      for (const auto& tool : tools) {
        CustomMessage::Entry native, foreign;
        native.icon = foreign.icon = 0xF5;
        const auto saveBefore = gSaveContext;
        const auto neiBefore = neiSave;
        const int readsBefore = donorReads;
        assert(Rando::ApplyItemReceiptText(tool.id, native) && "utility pickup still uses generic text");
        assert(Rando::ApplyForeignItemReceiptText(tool.name, foreign));
        assert(native.msg == foreign.msg && !native.autoFormat && !foreign.autoFormat);
        assert(native.icon == 0xF5 && foreign.icon == 0xF5);
        assert(native.msg.find('\xB2') != std::string::npos && "C-button glyph was not encoded for MM");
        assert(native.msg.find('\xB1') != std::string::npos && native.msg.find(tool.extraButton) != std::string::npos);
        assert(native.msg.substr(0, native.msg.find('\x10')).find(tool.color) != std::string::npos);
        assert(native.msg.find('%') == std::string::npos && native.msg.size() <= 1269);
        unsigned lines = 1, pages = 1;
        for (char c : native.msg) {
          if (c == '\x10') { lines = 1; ++pages; }
          if (c == '\x11') assert(++lines <= 3 && "tutorial exceeds MM's line-offset table");
        }
        assert(pages == tool.pages && "tutorial instruction spilled onto an extra page");
        assert(!std::memcmp(&saveBefore, &gSaveContext, sizeof(saveBefore)));
        assert(!std::memcmp(&neiBefore, &neiSave, sizeof(neiBefore)));
        assert(donorReads == readsBefore && "shared tool tutorial queried the OoT donor");
      }
    }
  }
  gSaveContext.options.language = LANGUAGE_ENG;
  std::cout << "PASS Hourglass/Crystal MM tutorials: native/foreign, all locales, cold/warm donor, glyphs, colors, pages and save isolation\n";
  const struct {
    RandoItemId id;
    const char* name;
    const char* effect;
    uint8_t color;
  } magicItems[] = {
    {RI_OOT_NEI_ELEMENTAL_WAND, "Elemental Wand", "medallion", 5},
    {RI_OOT_NEI_WAND_SAND_ROD, "Sand Rod", "platform", 4},
    {RI_OOT_NEI_WAND_TORNADO_ROD, "Tornado Rod", "jump", 2},
    {RI_OOT_NEI_WAND_WATER_ROD, "Water Rod", "water", 3},
    {RI_OOT_NEI_WAND_METEOR_ROD, "Meteor Rod", "explosive", 1},
    {RI_OOT_NEI_WAND_STORM_ROD, "Storm Rod", "lightning", 5},
    {RI_OOT_NEI_WAND_SHADOW_SCEPTER, "Shadow Scepter", "stun", 6},
    {RI_OOT_NEI_SHEIKAH_SLATE, "Sheikah Slate", "rune", 5},
    {RI_OOT_NEI_SLATE_RUNE_BOMB, "Rune: Remote Bomb", "detonate", 5},
    {RI_OOT_NEI_SLATE_RUNE_STASIS, "Rune: Stasis", "Freeze", 4},
    {RI_OOT_NEI_SLATE_RUNE_CRYONIS, "Rune: Cryonis", "pillar", 3},
    {RI_OOT_NEI_SLATE_RUNE_MASTER_CYCLE, "Rune: Master Cycle", "motorcycle", 2},
    {RI_OOT_NEI_DESIRE_SENSOR, "Rune: Sheikah Sensor", "Heart Container", 6},
  };
  const struct { RandoItemId id; const char* name; const char* power; } medallions[] = {
    {RI_OOT_MEDALLION_SPIRIT, "Spirit Medallion", "Sand Rod"},
    {RI_OOT_MEDALLION_FOREST, "Forest Medallion", "Tornado Rod"},
    {RI_OOT_MEDALLION_WATER, "Water Medallion", "Water Rod"},
    {RI_OOT_MEDALLION_FIRE, "Fire Medallion", "Meteor Rod"},
    {RI_OOT_MEDALLION_LIGHT, "Light Medallion", "Storm Rod"},
    {RI_OOT_MEDALLION_SHADOW, "Shadow Medallion", "Shadow Scepter"},
  };
  donorReady = false;
  SyncWandFixtureSettings();
  for (const auto& medallion : medallions) {
    CustomMessage::Entry native, foreign;
    const auto saveBefore = gSaveContext;
    const auto neiBefore = neiSave;
    assert(Rando::ApplyItemReceiptText(medallion.id, native));
    assert(Rando::ApplyForeignItemReceiptText(medallion.name, foreign) && "medallion unlock needs its rod tutorial without a donor");
    assert(native.msg == foreign.msg);
    assert(native.msg.find(medallion.name) != std::string::npos);
    assert(native.msg.find(medallion.power) != std::string::npos);
    assert(native.msg.find("awakens") != std::string::npos);
    assert(native.msg.size() <= 1269 && "magic description exceeds MM's textbox body capacity");
    assert(!std::memcmp(&saveBefore, &gSaveContext, sizeof(saveBefore)));
    assert(!std::memcmp(&neiBefore, &neiSave, sizeof(neiBefore)));
  }
  for (int language : {LANGUAGE_GER, LANGUAGE_FRE}) {
    gSaveContext.options.language = language;
    for (const auto& medallion : medallions) {
      CustomMessage::Entry native, foreign;
      assert(Rando::ApplyItemReceiptText(medallion.id, native));
      assert(Rando::ApplyForeignItemReceiptText(medallion.name, foreign));
      assert(native.msg == foreign.msg);
      assert(native.msg.find(language == LANGUAGE_GER ? "erweckt" : "pouvoir") != std::string::npos);
      assert(native.msg.find('\xB2') != std::string::npos && native.msg.size() <= 1269);
    }
  }
  gSaveContext.options.language = LANGUAGE_ENG;
  for (int rule : {1, 2}) {
    wandRule = rule;
    SyncWandFixtureSettings();
    for (const auto& medallion : medallions) {
      CustomMessage::Entry receipt;
      Rando::ApplyItemReceiptText(medallion.id, receipt);
      assert(receipt.msg.find("awakens") == std::string::npos && "medallions unlock rods only in medallion mode");
    }
  }
  wandRule = 0;
  neiEnabled = false;
  SyncWandFixtureSettings();
  for (const auto& medallion : medallions) {
    CustomMessage::Entry receipt;
    Rando::ApplyItemReceiptText(medallion.id, receipt);
    assert(receipt.msg.find("awakens") == std::string::npos && "disabled wand pool must retain ordinary medallion text");
  }
  neiEnabled = true;
  SyncWandFixtureSettings();
  for (bool ready : {false, true}) {
    donorReady = ready;
    gSaveContext.options.language = LANGUAGE_ENG;
    for (const auto& item : magicItems) {
      CustomMessage::Entry native, foreign;
      native.msg = foreign.msg = "generic grant";
      const auto saveBefore = gSaveContext;
      const auto neiBefore = neiSave;
      assert(Rando::ApplyItemReceiptText(item.id, native) && "native magic item has no description");
      assert(Rando::ApplyForeignItemReceiptText(item.name, foreign) && "foreign magic item has no description");
      assert(native.msg == foreign.msg && native.msg.find(item.effect) != std::string::npos);
      assert(native.msg.find('\xB2') != std::string::npos && "missing equipped C-button glyph");
      assert(native.msg.find('\xB3') != std::string::npos && native.msg.find('\xB0') != std::string::npos);
      assert(native.msg.substr(0, native.msg.find('\x10')).find(char(item.color)) != std::string::npos);
      assert(native.msg.find('%') == std::string::npos && "raw color markup leaked into a receipt");
      assert(native.msg.size() <= 1269 && "magic description exceeds MM's textbox body capacity");
      assert(!native.autoFormat && !native.capeVisibilityChoice && native.receiptPresentation.singleBox == 0);
      assert(!std::memcmp(&saveBefore, &gSaveContext, sizeof(saveBefore)));
      assert(!std::memcmp(&neiBefore, &neiSave, sizeof(neiBefore)));
    }
    for (int language : {LANGUAGE_GER, LANGUAGE_FRE}) {
      gSaveContext.options.language = language;
      for (const auto& item : magicItems) {
        CustomMessage::Entry native, foreign;
        assert(Rando::ApplyItemReceiptText(item.id, native));
        assert(Rando::ApplyForeignItemReceiptText(item.name, foreign));
        assert(native.msg == foreign.msg);
        assert(native.msg.find(language == LANGUAGE_GER ? "ziehen" : "sortir") != std::string::npos);
        assert(native.msg.find('\xB2') != std::string::npos);
        assert(native.msg.size() <= 1269);
      }
    }
  }
  gSaveContext.options.language = LANGUAGE_ENG;
  for (int rule : {0, 1, 2}) {
    wandRule = rule;
    SyncWandFixtureSettings();
    CustomMessage::Entry receipt;
    assert(Rando::ApplyItemReceiptText(RI_OOT_NEI_ELEMENTAL_WAND, receipt));
    assert(receipt.msg.find(rule == 0 ? "medallion" : rule == 1 ? "All six" : "separately") != std::string::npos);
    if (rule != 2) {
      for (const char* effect : {"platform", "wind", "water", "explosive", "lightning", "stun"})
        assert(receipt.msg.find(effect) != std::string::npos && "shared wand pickup must teach every power");
      if (rule == 0) {
        for (const char* medallion : {"Spirit:", "Forest:", "Water:", "Fire:", "Light:", "Shadow:"})
          assert(receipt.msg.find(medallion) != std::string::npos && "missing medallion-to-rod explanation");
      }
      const auto footer = receipt.msg.find("Equip to");
      assert(footer != std::string::npos && receipt.msg.find("Equip to", footer+1) == std::string::npos);
    }
  }
  wandRule = 0;
  SyncWandFixtureSettings();
  donorReady = true;
  std::cout << "PASS 13 magic pickups and six medallion tutorials: effects, colors, glyphs, all locales, cold/warm donor, three wand rules and complete universal guide\n";
  CheckRandomRupeeReceipts();
  CheckNativeRupeeHooks();
  // A real cape identity carries the prompt marker through the native and
  // cross-game builders without changing ownership or visibility at preview
  // time.
  const auto capeSaveBefore = gSaveContext;
  const auto capeNeiBefore = neiSave;
  CustomMessage::Entry cape;
  Rando::ApplyItemReceiptText(RI_OOT_EXT_MAGIC_CAPE, cape);
  assert(cape.capeVisibilityChoice);
  Rando::ApplyForeignItemReceiptText("Magic Cape", cape);
  assert(cape.capeVisibilityChoice);
  Rando::ApplyItemReceiptText(RI_OOT_NEI_ROCS_CAPE, cape);
  assert(!cape.capeVisibilityChoice &&
         "Roc's Cape must not open the Magic Cape choice");
  Rando::ApplyForeignItemReceiptText("Roc's Cape", cape);
  assert(!cape.capeVisibilityChoice);
  assert(!std::memcmp(&capeSaveBefore, &gSaveContext, sizeof(capeSaveBefore)));
  assert(!std::memcmp(&capeNeiBefore, &neiSave, sizeof(capeNeiBefore)));
#ifdef COMPASS_DONOR_INTEGRATION
  CheckForestCompassReceiveRoutes(argc > 1 ? argv[1] : nullptr);
  CheckShuffledDungeonInformation();
  return 0;
#else
  // Header is deliberately a story entry with a different icon and next ID.
  const char bow[] = "\x00\x00\x20\x00\x03\xFF\xFF\xFF\xFF\xFF\xFF"
                     "You got the Hero's Bow!\x10Press \xB2 to aim.\x19\xBF";
  const uint16_t bowText = Player_GetItemReceiptTextId(GI_QUIVER_30, ITEM_BOW);
  assert(bowText != 0);
  const char hook[] =
      "\x00\x00\x20\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
      "You got the Hookshot!\x11Latch onto distant targets.\x19\xBF";
  const uint16_t hookText =
      Player_GetItemReceiptTextId(GI_HOOKSHOT, ITEM_HOOKSHOT);
  assert(hookText != 0);
  const char rupee[] = "\x00\x00\x20\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
                       "You got a Green Rupee!\x11It's worth one Rupee.\x19\xBF";
  const char arrows[] = "\x00\x00\x20\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
                        "You got 10 Arrows!\x11Use them with your bow.\x19\xBF";
  const auto rupeeText=Player_GetItemReceiptTextId(GI_RUPEE_GREEN,ITEM_RUPEE_GREEN);
  const auto arrowsText=Player_GetItemReceiptTextId(GI_ARROWS_10,ITEM_ARROWS_10);
  assert(rupeeText && arrowsText);
  MessageTableEntry table[] = {{rupeeText,0,rupee,sizeof(rupee)-1},
                               {arrowsText,0,arrows,sizeof(arrows)-1},
                               {bowText, 0, bow, sizeof(bow) - 1},
                               {hookText, 0, hook, sizeof(hook) - 1},
                               {0xFFFF, 0, nullptr, 0}};
  play.msgCtx.messageTableNES = table;
  CustomMessage::Entry entry;
  entry.icon = 0xF5;
  CheckDungeonKeyReceipts();
  CheckMapCompassInformation();
  CheckShuffledDungeonInformation();
  // Native MM receipt and exported icon both retain MM's own shield artwork.
  stagedIconPath = nullptr;
  assert(Rando::StaticData::GetIconForZMessage(RI_SHIELD_MIRROR) == 0xF5);
  assert(stagedIconPath && !std::strcmp(stagedIconPath, COMBO_IKANA_SHIELD_ICON));
  assert(stagedIconWidth == 32);
  assert(!std::strcmp(Rando::StaticData::GetIconTexturePath(RI_SHIELD_MIRROR), COMBO_IKANA_SHIELD_ICON));
  // GI_NONE song rows must still take the engine's 16x24 IA8 note branch.
  const std::pair<RandoItemId, int> songIcons[] = {
      { RI_SONG_SONATA, ITEM_SONG_SONATA }, { RI_SONG_LULLABY, ITEM_SONG_LULLABY },
      { RI_SONG_LULLABY_INTRO, ITEM_SONG_LULLABY }, { RI_SONG_NOVA, ITEM_SONG_NOVA },
      { RI_SONG_ELEGY, ITEM_SONG_ELEGY }, { RI_SONG_OATH, ITEM_SONG_OATH },
      { RI_SONG_SARIA, ITEM_SONG_SARIA }, { RI_SONG_TIME, ITEM_SONG_TIME },
      { RI_SONG_DOUBLE_TIME, ITEM_SONG_TIME }, { RI_SONG_INVERTED_TIME, ITEM_SONG_TIME },
      { RI_SONG_HEALING, ITEM_SONG_HEALING }, { RI_SONG_EPONA, ITEM_SONG_EPONA },
      { RI_SONG_SOARING, ITEM_SONG_SOARING }, { RI_SONG_STORMS, ITEM_SONG_STORMS },
      { RI_SONG_SUN, ITEM_SONG_SUN },
  };
  for (auto [song, icon] : songIcons) {
    auto byte = Rando::StaticData::GetIconForZMessage(song);
    (void)icon;
    assert(byte == 0xF5 && stagedWidth == 16 && stagedHeight == 24 && stagedIA8);
  }
  Rando::StaticData::GetIconForZMessage(RI_SONG_DOUBLE_TIME);
  assert(stagedColor[0] == 128 && stagedColor[1] == 216 && stagedColor[2] == 240);
  Rando::StaticData::GetIconForZMessage(RI_SONG_INVERTED_TIME);
  assert(stagedColor[0] == 74 && stagedColor[1] == 112 && stagedColor[2] == 202);
  Rando::StaticData::GetIconForZMessage(RI_SONG_LULLABY_INTRO);
  assert(stagedColor[0] == 255 && stagedColor[1] == 100 && stagedColor[2] == 100);
  const std::pair<RandoItemId, uint32_t> receiptColors[] = {
      {RI_SONG_DOUBLE_TIME, 0x80D8F0}, {RI_SONG_ELEGY, 0xFF6200},         {RI_SONG_EPONA, 0xD96E30},
      {RI_SONG_HEALING, 0xFF96E6},     {RI_SONG_INVERTED_TIME, 0x4A70CA}, {RI_SONG_LULLABY_INTRO, 0xFF6464},
      {RI_SONG_LULLABY, 0xFF1414},     {RI_SONG_NOVA, 0x1414FF},          {RI_SONG_OATH, 0x620062},
      {RI_SONG_SARIA, 0x6ACB62},       {RI_SONG_SOARING, 0xC8A0FF},       {RI_SONG_SONATA, 0x62FF62},
      {RI_SONG_STORMS, 0x929292},      {RI_SONG_SUN, 0xEDE73E},           {RI_SONG_TIME, 0x62B1D3}};
  for (auto [song, rgb] : receiptColors) {
      assert(Rando::StaticData::GetIconForZMessage(song) == 0xF5);
      assert((uint32_t(stagedColor[0]) << 16 | uint32_t(stagedColor[1]) << 8 | stagedColor[2]) == rgb);
  }
  const int healing = ITEM_SONG_HEALING - ITEM_SONG_SONATA;
  assert(D_801CFE04[healing] == 255 && D_801CFE1C[healing] == 150 && D_801CFE34[healing] == 230);
  const int time = ITEM_SONG_TIME - ITEM_SONG_SONATA;
  assert(D_801CFE04[time] == 98 && D_801CFE1C[time] == 177 && D_801CFE34[time] == 211);
  const auto tableBeforeEquipment = play.msgCtx.messageTableNES;
  const RandoItemId nativeEquipment[] = {RI_SWORD_KOKIRI, RI_SWORD_RAZOR,
                                         RI_SWORD_GILDED, RI_GREAT_FAIRY_SWORD,
                                         RI_SHIELD_HERO,  RI_SHIELD_MIRROR,
                                         RI_BOTTLE_EMPTY, RI_BOTTLE_RED_POTION,
                                         RI_BOTTLE_MILK, RI_BOTTLE_GOLD_DUST,
                                         RI_BOTTLE_CHATEAU_ROMANI};
  std::vector<std::string> equipmentBodies;
  std::vector<MessageTableEntry> equipmentMessages;
  equipmentBodies.reserve(std::size(nativeEquipment));
  for (const auto id : nativeEquipment) {
    const auto &item = Rando::StaticData::Items.at(id);
    // Native receipt IDs from MM's real player get-item table. Red's catalog
    // GI is a model alias; Chateau's full bottle differs from its refill GI.
    const auto text = id == RI_BOTTLE_RED_POTION ? 0x5B
                     : id == RI_BOTTLE_CHATEAU_ROMANI ? 0x6F
                     : Player_GetItemReceiptTextId(item.getItemId, item.itemId);
    assert(text);
    equipmentBodies.emplace_back(
        std::string(MESSAGE_HEADER_SIZE, '\0') + "Native MM tutorial: " +
        (id == RI_SHIELD_MIRROR ? "Mirror Shield" : item.name) +
        "!\x10"
        "Equip and press \xB1 or \xB4.\xBF");
    const auto &body = equipmentBodies.back();
    equipmentMessages.push_back(
        {text, 0, body.data(), static_cast<uint32_t>(body.size())});
  }
  equipmentMessages.push_back({0xFFFF, 0, nullptr, 0});
  play.msgCtx.messageTableNES = equipmentMessages.data();
  for (const auto id : nativeEquipment) {
    entry = {};
    entry.icon = 0xE7;
    const auto before = gSaveContext;
    assert(Rando::ApplyItemReceiptText(id, entry));
    assert(entry.msg.find("Native MM tutorial") != std::string::npos &&
           "MM equipment must keep its own full native tutorial");
    auto equipmentBody = entry.msg;
    std::replace(equipmentBody.begin(), equipmentBody.end(), '\x11', ' ');
    assert(equipmentBody.find(Rando::StaticData::Items.at(id).name) !=
           std::string::npos);
    assert(entry.icon == 0xE7 && entry.msg.find('\xB1') != std::string::npos);
    assert(!entry.autoFormat &&
           !std::memcmp(&before, &gSaveContext, sizeof(before)));
  }
  play.msgCtx.messageTableNES = tableBeforeEquipment;
  const RandoItemId importedEquipment[] = {
      RI_OOT_MASTER_SWORD,  RI_OOT_BIGGORON_SWORD, RI_OOT_DEKU_SHIELD,
      RI_OOT_MIRROR_SHIELD, RI_OOT_BOTTLE_FISH,    RI_OOT_BOTTLE_BUGS,
      RI_OOT_BOTTLE_POE,    RI_OOT_BOTTLE_BIG_POE,
      RI_OOT_BOTTLE_BLUE_FIRE, RI_OOT_BOTTLE_BLUE_POTION, RI_OOT_BOTTLE_GREEN_POTION,
      RI_OOT_BOTTLE_FAIRY, RI_OOT_BOTTLE_MAGIC_MUSHROOM, RI_OOT_RUTOS_LETTER};
  const auto donorBeforeEquipment = donorReady;
  donorReady = false;
  for (int language : {LANGUAGE_ENG, LANGUAGE_GER, LANGUAGE_FRE}) {
    gSaveContext.options.language = language;
    for (const auto id : importedEquipment) {
      entry = {};
      entry.icon = 0xE7;
      const auto before = gSaveContext;
      assert(Rando::ApplyItemReceiptText(id, entry));
      assert(entry.msg.find('\x10') != std::string::npos &&
             entry.msg.size() < 1269);
      assert(
          entry.msg.find((id >= RI_OOT_BOTTLE_BIG_POE && id <= RI_OOT_BOTTLE_POE) || id == RI_OOT_RUTOS_LETTER
                             ? '\xB2'
                             : '\xB0') != std::string::npos);
      assert(entry.icon == 0xE7 && !entry.capeVisibilityChoice &&
             entry.receiptPresentation.singleBox == 0);
      assert(!std::memcmp(&before, &gSaveContext, sizeof(before)));
    }
  }
  const auto playBeforeBottles = gPlayState;
  gPlayState = nullptr;
  const RandoItemId nativeBottlesAndRefills[] = { RI_BOTTLE_EMPTY, RI_BOTTLE_RED_POTION, RI_BOTTLE_MILK,
      RI_BOTTLE_GOLD_DUST, RI_BOTTLE_CHATEAU_ROMANI, RI_RED_POTION_REFILL, RI_GREEN_POTION_REFILL,
      RI_BLUE_POTION_REFILL, RI_MILK_REFILL, RI_CHATEAU_ROMANI_REFILL, RI_GOLD_DUST_REFILL,
      RI_FAIRY_REFILL, RI_MUSHROOM };
  for (int language : {LANGUAGE_ENG, LANGUAGE_GER, LANGUAGE_FRE}) {
    gSaveContext.options.language = language;
    for (auto id : nativeBottlesAndRefills) {
      entry = {}; entry.icon = 0xE7;
      const auto before = gSaveContext;
      assert(Rando::ApplyItemReceiptText(id, entry));
      assert(entry.msg.find('\xB2') != std::string::npos && entry.msg.find('\x10') != std::string::npos);
      assert(entry.icon == 0xE7 && entry.msg.size() < 1269);
      assert(!std::memcmp(&before, &gSaveContext, sizeof(before)));
    }
    for (const char* name : {"Bottle with Hot Spring Water", "Deku Princess", "Seahorse", "Spring Water",
        "Zora Egg", "Hylian Loach", "Obaba's Drink", "Milk (Half)"}) {
      entry = {}; entry.icon = 0xE7;
      assert(Rando::ApplyForeignItemReceiptText(name, entry));
      assert(entry.msg.find('\xB2') != std::string::npos && entry.msg.find('\x10') != std::string::npos);
      assert(entry.icon == 0xE7);
    }
  }
  // A live donor exports English bytes even when its native banks are absent.
  // MM must still own the language of its local bottle fallback lessons.
  donorReady = true;
  for (int language : {LANGUAGE_GER, LANGUAGE_FRE}) {
    gSaveContext.options.language = language;
    const char* opening = language == LANGUAGE_GER ? "Du hast" : "Vous obtenez";
    for (auto id : nativeBottlesAndRefills) {
      entry = {}; entry.icon = 0xE7;
      assert(Rando::ApplyItemReceiptText(id, entry));
      assert(entry.msg.find(opening) != std::string::npos && "live English donor bypassed localized bottle lesson");
      assert(entry.icon == 0xE7);
    }
    for (const char* name : {"Bottle with Fairy", "Bottle with Blue Fire", "Bottle with Red Potion",
        "Bottle with Green Potion", "Bottle with Blue Potion", "Bottle with Fish", "Bottle with Bugs",
        "Bottle with Poe", "Bottle with Big Poe", "Bottle with Ruto's Letter", "Bottle with Hot Spring Water",
        "Deku Princess", "Seahorse", "Spring Water", "Zora Egg", "Hylian Loach", "Obaba's Drink", "Milk (Half)"}) {
      entry = {}; entry.icon = 0xE7;
      assert(Rando::ApplyForeignItemReceiptText(name, entry));
      assert(entry.msg.find(opening) != std::string::npos && "foreign bottle lesson lost MM locale");
      assert(entry.icon == 0xE7);
    }
  }
  gPlayState = playBeforeBottles;
  gSaveContext.options.language = LANGUAGE_ENG;
  donorReady = donorBeforeEquipment;
  std::cout
      << "PASS MM native equipment tutorials and imported sword/shield/bottle "
         "cold fallback, locales, icons and save isolation\n";
  // Shuffled fairy receipts use their own dungeon, with the pending grant
  // included once. The current scene and shared scene index are unrelated.
  const RandoItemId fairies[] = {
      RI_WOODFALL_STRAY_FAIRY, RI_SNOWHEAD_STRAY_FAIRY,
      RI_GREAT_BAY_STRAY_FAIRY, RI_STONE_TOWER_STRAY_FAIRY};
  const char *fairyTemples[] = {"Woodfall Temple", "Snowhead Temple",
                                "Great Bay Temple", "Stone Tower Temple"};
  const char fairyColors[] = {0x06, 0x02, 0x03,
                              0x04}; // native pink, green, blue, yellow
  const auto beforeFairyReceipts = gSaveContext;
  play.sceneId = SCENE_CLOCKTOWER;
  gSaveContext.dungeonSceneSharedIndex = DUNGEON_SCENE_INDEX_SNOWHEAD_TEMPLE;
  for (int d = 0; d < 4; ++d) {
    for (int current : {0, 7, 14}) {
      for (int other = 0; other < 4; ++other)
        gSaveContext.save.saveInfo.inventory.strayFairies[other] =
            other == d ? current : 3;
      const auto before = gSaveContext;
      entry = {};
      entry.icon = Rando::StaticData::GetIconForZMessage(fairies[d]);
      assert(entry.icon ==
             0xF1 + d); // Existing per-dungeon fairy icon sentinel.
      assert(Rando::ApplyItemReceiptText(fairies[d], entry) &&
             "MM fairy lost its pickup dialogue");
      std::string flat = entry.msg;
      std::replace(flat.begin(), flat.end(), '\x11', ' ');
      assert(flat.find(Rando::StaticData::Items.at(fairies[d]).name) !=
             std::string::npos);
      assert(flat.find(fairyTemples[d]) != std::string::npos);
      assert(flat.find(std::to_string(current + 1)) != std::string::npos);
      assert(flat.find(current == 0 ? "stray fairy" : "stray fairies") !=
             std::string::npos);
      assert(std::count(entry.msg.begin(), entry.msg.end(), fairyColors[d]) ==
                 3 &&
             "fairy name, count and temple need their dungeon color code");
      assert(flat.find(std::string(1, fairyColors[d]) + fairyTemples[d]) !=
             std::string::npos);
      assert(entry.icon == 0xF1 + d && !entry.autoFormat &&
             entry.nextMessageID == 0xFFFF);
      assert(!std::memcmp(&before, &gSaveContext, sizeof(before)) &&
             "receipt granted or changed a fairy");
    }
  }
  entry.msg = "Clock Town pickup";
  assert(!Rando::ApplyItemReceiptText(RI_CLOCK_TOWN_STRAY_FAIRY, entry));
  assert(entry.msg ==
         "Clock Town pickup"); // Its single-fairy dialogue stays separate.
  const auto beforeFairyGrants = neiSave;
  for (int d = 0; d < 4; ++d) {
    const int fc = FcCombo_ItemForNative(fairies[d]);
    for (auto counts : {std::pair{7, 0}, std::pair{7, 3}, std::pair{3, 7},
                        std::pair{14, 14}}) {
      neiSave = {};
      for (int other = 0; other < 4; ++other)
        gSaveContext.save.saveInfo.inventory.strayFairies[other] =
            other == d ? counts.first : 3;
      neiSave.comboObtainedFc[fc] = counts.second + 2;
      neiSave.comboAppliedFc[fc] = counts.second;
      ApplyReceiptFcRegistry();
      assert(
          gSaveContext.save.saveInfo.inventory.strayFairies[d] ==
              counts.first + 2 &&
          "an older native fairy baseline must not become phantom FC grants");
      assert(neiSave.comboObtainedFc[fc] == counts.second + 2);
      assert(neiSave.comboAppliedFc[fc] == counts.second + 2);
      assert(gFcCombo_SuppressRecord == 0);
      for (int other = 0; other < 4; ++other)
        if (other != d)
          assert(gSaveContext.save.saveInfo.inventory.strayFairies[other] == 3);
      const auto saveAfterGrant = gSaveContext;
      const auto fcAfterGrant = neiSave;
      ApplyReceiptFcRegistry();
      assert(
          !std::memcmp(&saveAfterGrant, &gSaveContext, sizeof(gSaveContext)));
      assert(!std::memcmp(&fcAfterGrant, &neiSave, sizeof(neiSave)));
    }
  }
  neiSave = beforeFairyGrants;
  gSaveContext = beforeFairyReceipts;
  std::cout
      << "MM stray fairy receipts: four native colors/icons, first/8th/15th "
         "counts, shuffled dungeon identity and save isolation passed\n";
  std::cout
      << "MM fairy FC materialization: preserved native baseline, exact copy "
         "deficits, suppression and repeat reconciliation passed\n";
  entry = {};
  entry.icon = 0xF5;
  // Receipt is composed before GiveItem. The token's identity, never the
  // current scene, chooses the spider-house counter.
  play.sceneId = SCENE_CLOCKTOWER;
  gSaveContext.save.saveInfo.skullTokenCount = (7u << 16) | 19u;
  assert(Rando::ApplyItemReceiptText(RI_GS_TOKEN_SWAMP, entry));
  assert(entry.msg.find("Swamp") != std::string::npos &&
         entry.msg.find("8") != std::string::npos &&
         entry.msg.find("20") == std::string::npos);
  assert(Rando::ApplyItemReceiptText(RI_GS_TOKEN_OCEAN, entry));
  assert(entry.msg.find("Ocean") != std::string::npos &&
         entry.msg.find("20") != std::string::npos);
  neiSave.ootGsCount = 42;
  assert(Rando::ApplyItemReceiptText(RI_OOT_GS_TOKEN, entry));
  assert(entry.msg.find("43") != std::string::npos);
  assert(gSaveContext.save.saveInfo.skullTokenCount == ((7u << 16) | 19u));
  assert(neiSave.ootGsCount == 42); // descriptions cannot grant items
  assert(Rando::ApplyItemReceiptText(RI_BOW, entry));
  assert(entry.msg.find("aim") != std::string::npos);
  assert(entry.msg.find('\x19') == std::string::npos &&
         entry.msg.find('\xBF') == std::string::npos);
  assert(entry.icon == 0xF5 && entry.nextMessageID == 0xFFFF &&
         !entry.autoFormat);
  Rando::AppendReceiptSource(entry, " (Bank reward)");
  assert(entry.msg.back() == '\xBF' &&
         entry.msg.find("Bank reward") != std::string::npos);

  // Canonical GI/Item namespace validation protects frog/model aliases.
  assert(Player_GetItemReceiptTextId(GI_MASK_DON_GERO, ITEM_NONE) == 0);
  assert(Player_GetItemReceiptTextId(GI_MASK_DON_GERO, ITEM_BOW) == 0);
  assert(Player_GetItemReceiptTextId(GI_NONE, ITEM_BOW) == 0);
  assert(Player_GetItemReceiptTextId(GI_MAX, ITEM_BOW) == 0);
  assert(Player_GetItemReceiptTextId(-1, ITEM_BOW) == 0);
  assert(Player_GetItemReceiptTextId(GI_SHIP, ITEM_SHIP) == 0);

  entry = {};
  entry.icon = 0xF5;
  assert(Rando::ApplyItemReceiptText(RI_OOT_NEI_DEKU_LEAF, entry));
  assert(entry.msg.find("glide") != std::string::npos &&
         entry.msg.find("gust") != std::string::npos);
  assert(entry.msg.find('%') == std::string::npos &&
         entry.msg.find('^') == std::string::npos);
  assert(entry.msg.find('\0') !=
         std::string::npos); // default color is binary NUL
  Rando::AppendReceiptSource(entry, "");
  assert(entry.msg.back() == '\xBF');
  donorReady = false;
  // Missing donor access must not send songs to story/teaching text, or leave
  // a one-line identity receipt. Both warm and cold runs use the MM meanings.
  const std::pair<RandoItemId, const char *> descriptions[] = {
    {RI_SONG_SONATA, "slumber"}, {RI_SONG_LULLABY, "sleep"},
    {RI_SONG_LULLABY_INTRO, "opening bars"}, {RI_SONG_NOVA, "new life"},
    {RI_SONG_ELEGY, "hollow"}, {RI_SONG_OATH, "giants"},
    {RI_SONG_HEALING, "masks"}, {RI_SONG_SOARING, "statue"},
    {RI_SONG_TIME, "Termina"}, {RI_SONG_STORMS, "thunder"},
    {RI_SONG_SUN, "night"}, {RI_SONG_EPONA, "horse"},
    {RI_SONG_SARIA, "forest"}, {RI_SONG_DOUBLE_TIME, "dusk"},
    {RI_SONG_INVERTED_TIME, "slows"}
  };
  for (bool ready : {false, true}) {
    donorReady = ready;
    gSaveContext.options.language = LANGUAGE_ENG;
    for (auto [id, description] : descriptions) {
      entry = {};
      assert(Rando::ApplyItemReceiptText(id, entry) && "MM song has no descriptive receipt");
      assert(entry.msg.find(description) != std::string::npos && !entry.autoFormat);
    }
    gSaveContext.options.language = LANGUAGE_GER;
    assert(Rando::ApplyItemReceiptText(RI_SONG_HEALING, entry));
    assert(entry.msg.find("Masken") != std::string::npos);
    gSaveContext.options.language = LANGUAGE_FRE;
    assert(Rando::ApplyItemReceiptText(RI_SONG_HEALING, entry));
    assert(entry.msg.find("masques") != std::string::npos);
  }
  gSaveContext.options.language = LANGUAGE_ENG;
  donorReady = false;
  entry.msg = "fallback";
  assert(Rando::ApplyItemReceiptText(
      RI_OOT_NEI_DEKU_LEAF, entry)); // local registry works without donor
  assert(entry.msg.find("glide") != std::string::npos);
  assert(!Rando::ApplyItemReceiptText(RI_SINGLE_MAGIC, entry));
  entry = {};
  entry.msg = "generic Clawshot";
  assert(!Rando::ApplyItemReceiptText(RI_CLAWSHOT, entry) &&
         entry.msg == "generic Clawshot");
  neiSave.ootHookshotLevel = 1;
  assert(!Rando::ApplyItemReceiptText(
      RI_HOOKSHOT, entry)); // no incorrect vanilla Hookshot for Longshot
  neiSave.ootHookshotLevel = 0;
  assert(Rando::ApplyItemReceiptText(RI_HOOKSHOT, entry) &&
         entry.msg.find("Latch") != std::string::npos);
  // Actual MM decoder stores a line offset for every newline and at page end.
  // A four-line Roc's Cape page previously indexed beyond its three entries.
  assert(Rando::ApplyItemReceiptText(RI_OOT_NEI_ROCS_CAPE, entry));
  const auto lineCapacity =
      sizeof(play.msgCtx.unk11F1A) / sizeof(play.msgCtx.unk11F1A[0]);
  size_t lineIndex = 0;
  for (unsigned char c : entry.msg) {
    if (c == 0x10 || c == 0x12)
      lineIndex = 0;
    else if (c == 0x11)
      ++lineIndex;
    assert(lineIndex < lineCapacity &&
           "receipt exceeds the MM decoder line array");
  }
  donorReady = true;
  assert(Rando::ApplyItemReceiptText(RI_OOT_NEI_DEKU_LEAF,
                                     entry)); // no negative cache
  const std::pair<RandoItemId, const char *> concrete[] = {
      {RI_SINGLE_MAGIC, "Magic Meter"},
      {RI_SONG_SONATA, "Sonata of Awakening"},
      {RI_SONG_LULLABY, "Goron Lullaby"},
      {RI_SONG_LULLABY_INTRO, "Goron Lullaby Intro"},
      {RI_SONG_NOVA, "New Wave Bossa Nova"},
      {RI_SONG_ELEGY, "Elegy of Emptiness"},
      {RI_SONG_OATH, "Oath to Order"},
      {RI_SONG_HEALING, "Song of Healing"},
      {RI_SONG_SOARING, "Song of Soaring"},
      {RI_SONG_TIME, "Song of Time (MM)"},
      {RI_SONG_STORMS, "Song of Storms (MM)"},
      {RI_SONG_SUN, "Sun's Song (MM)"},
      {RI_SONG_EPONA, "Epona's Song (MM)"},
      {RI_SONG_SARIA, "Saria's Song (MM)"},
      {RI_SONG_DOUBLE_TIME, "Song of Double Time"},
      {RI_SONG_INVERTED_TIME, "Inverted Song of Time"},
      {RI_OOT_COMPASS_DEKU_TREE, "Great Deku Tree Compass"},
      {RI_OOT_MAP_DEKU_TREE, "Great Deku Tree Map"},
      {RI_SNOWHEAD_COMPASS, "Snowhead Compass"},
      {RI_DOUBLE_MAGIC, "Enhanced Magic Meter"},
      {RI_CLAWSHOT, "Clawshot"},
      {RI_OOT_MASTER_SWORD, "Master Sword"},
      {RI_OOT_TRUE_MASTER_SWORD, "True Master Sword"},
      {RI_OOT_HAMMER, "Megaton Hammer"},
      {RI_OOT_IRON_KNUCKLE_AXE, "Iron Knuckle's Axe"},
      {RI_OOT_BIGGORON_SWORD, "Biggoron's Sword"},
      {RI_OOT_GORONS_BRACELET, "Goron's Bracelet"},
      {RI_OOT_SILVER_GAUNTLETS, "Silver Gauntlets"},
      {RI_OOT_GOLDEN_GAUNTLETS, "Golden Gauntlets"},
      {RI_OOT_NEI_ROCS_FEATHER, "Progressive Roc"},
      {RI_OOT_NEI_ROCS_CAPE, "Roc's Cape"},
      {RI_OOT_STONE_OF_AGONY, "Stone of Agony"},
      {RI_OOT_QUARTZ_OF_MOTION, "Quartz of Motion"},
      {RI_OOT_NEI_CANE_PACCI_FLIP, "Cane of Pacci"},
      {RI_OOT_NEI_CANE_PACCI_STONE, "Pacci Stone Skill"},
      {RI_OOT_NEI_CANE_PACCI_ULTRAHAND, "Ultrahand"},
      {RI_OOT_NEI_CANE_SOMARIA_BLOCK, "Somaria Block Skill"},
      {RI_OOT_NEI_CANE_SOMARIA_PLATFORM, "Somaria Platform Skill"}};
  for (const auto &[id, name] : concrete) {
    requested.clear();
    assert(Rando::ApplyItemReceiptText(id, entry));
    assert((id >= RI_SONG_DOUBLE_TIME && id <= RI_SONG_TIME) || requested == name &&
           "concrete receipt went back through progressive donor state");
  }
  for (int owned = 0; owned <= 3; ++owned) {
    neiSave.ootHookshotLevel = owned;
    assert(Rando::ApplyItemReceiptText(RI_HOOKSHOT, entry));
    assert(requested == (owned == 0   ? "Hookshot"
                         : owned == 1 ? "Longshot"
                                      : "Ultrashot"));
  }
  neiSave.ootHookshotLevel = 0;
  INV_CONTENT(ITEM_HOOKSHOT) = ITEM_HOOKSHOT;
  assert(Rando::ApplyItemReceiptText(RI_HOOKSHOT, entry) &&
         requested == "Longshot");
  neiSave.ootQuestItems = 1u << OOT_QUEST_STONE_OF_AGONY;
  assert(Rando::ApplyItemReceiptText(RI_OOT_STONE_OF_AGONY, entry) &&
         requested == "Quartz of Motion");
  neiSave.ootQuestItems = 0;
  neiSave.slingshotOwned = 0;
  assert(Rando::ApplyItemReceiptText(RI_FAIRY_SLINGSHOT, entry) &&
         requested == "Fairy Slingshot");
  neiSave.slingshotOwned = 1;
  for (int level = 0; level <= 3; ++level) {
    neiSave.ootUpgrades = level;
    assert(Rando::ApplyItemReceiptText(RI_FAIRY_SLINGSHOT, entry));
    assert(requested == (level == 0   ? "Fairy Slingshot"
                         : level == 1 ? "Big Deku Seed Bullet Bag"
                                      : "Biggest Deku Seed Bullet Bag"));
  }
  const char *sticks[] = {"Deku Stick Bag", "Deku Stick Capacity (20)",
                          "Deku Stick Capacity (30)"};
  const char *nuts[] = {"Deku Nut Bag", "Deku Nut Capacity (30)",
                        "Deku Nut Capacity (40)"};
  for (int level = 0; level <= 3; ++level) {
    gSaveContext.save.saveInfo.inventory.upgrades =
        (level << gUpgradeShifts[UPG_DEKU_STICKS]) |
        (level << gUpgradeShifts[UPG_DEKU_NUTS]);
    assert(
        Rando::ApplyItemReceiptText(RI_OOT_PROGRESSIVE_STICK_CAPACITY, entry));
    assert(requested == sticks[std::min(level, 2)]);
    assert(Rando::ApplyItemReceiptText(RI_OOT_PROGRESSIVE_NUT_CAPACITY, entry));
    assert(requested == nuts[std::min(level, 2)]);
  }
  assert(Rando::ApplyForeignItemReceiptText("Magic Meter", entry,
                                            RC_CLOCK_TOWER_ROOF_OCARINA));
  const auto firstReceipt = entry.msg;
  const int readsBeforeRepeat = donorReads;
  assert(Rando::ApplyForeignItemReceiptText("Magic Meter", entry,
                                            RC_CLOCK_TOWER_ROOF_OCARINA));
  assert(entry.msg == firstReceipt &&
         donorReads ==
             readsBeforeRepeat); // fixed body, including stat counters
  ++generation;
  assert(Rando::ApplyForeignItemReceiptText("Magic Meter", entry,
                                            RC_CLOCK_TOWER_ROOF_OCARINA));
  assert(entry.msg != firstReceipt);
  const int readsBeforeSlot = donorReads;
  ++gSaveContext.fileNum;
  assert(Rando::ApplyForeignItemReceiptText("Magic Meter", entry,
                                            RC_CLOCK_TOWER_ROOF_OCARINA));
  assert(donorReads == readsBeforeSlot + 1);
  entry.msg = "brief";
  for(const auto [id, detail] : {std::pair{RI_RUPEE_GREEN,"one Rupee"},std::pair{RI_ARROWS_10,"your bow"}}) {
    entry={};entry.icon=Rando::StaticData::GetIconForZMessage(id);
    const auto icon=entry.icon;
    const int reads=donorReads;
    assert(Rando::ApplyItemReceiptText(id,entry) && "junk receipts must use their brief native body instead of generic queue dialogue");
    assert(FlattenReceiptLines(entry.msg).find(detail)!=std::string::npos && !entry.autoFormat && entry.icon==icon);
    assert(donorReads==reads && entry.msg.find('\x19')==std::string::npos);
    Rando::AppendReceiptSource(entry,"");assert(entry.msg.back()=='\xBF');
  }
  CheckDirectSongGrants();
  std::cout << "Native junk receipts and five actual story-song grants in ENG/FRE/GER, cutscene on/off passed\n";
  entry.msg="brief";
  assert(!Rando::ApplyItemReceiptText(RI_TRAP, entry));
  assert(!Rando::ApplyItemReceiptText(RI_MAX, entry));
  assert(entry.msg == "brief");
  std::cout << "real native/FC item catalogs, donor descriptions, binary "
               "length, safe aliases and cold fallback passed\n";
#endif
}
