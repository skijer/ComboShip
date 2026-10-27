// Production OoT export; only runtime item/message ownership is replaced.
#include "combo/menu/ComboItemReceiptText.h"
#include "combo/menu/ComboVanillaItemReceiptText.h"
#include "combo/menu/ComboKeyReceiptText.h"
#include "combo/menu/ComboDungeonKeyReceipt.h"
#include "combo/menu/ComboBossSoulReceiptText.h"
#include "combo/menu/ComboMagicItemReceiptText.h"
#include "combo/menu/ComboToolReceiptText.h"
#include "combo/menu/ComboMaskReceiptText.h"
#include "combo/menu/ComboItemReceiptPresentation.h"
#include "combo/menu/ComboItemReceiptText.h"
#include "soh/soh/Enhancements/custom-message/text.h"
#include "tests/item_receipts/key_receipt_fixtures.h"
#include "tests/item_receipts/boss_soul_fixtures.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <vector>
#ifdef COMPASS_DONOR_BRIDGE
#include <dlfcn.h>
#endif
using namespace std::literals::string_literals;
#define SPDLOG_DEBUG(...) ((void)0)
#define COMBO_BUILD
#include "soh/soh/FleetShipCombo/FleetComboIds.h"
#include "soh/soh/FleetShipCombo/FleetComboItems.h"
#define RANDO_ENUM_BEGIN(x) enum x {
#define RANDO_ENUM_ITEM(x) x,
#define RANDO_ENUM_END(x)                                                      \
  }                                                                            \
  ;
#include "soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerCheck.h"
#include "soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerGet.h"
#include "soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerSettingKey.h"
constexpr RandomizerCheck RC_RECEIPT_FIXTURE =
    static_cast<RandomizerCheck>(RC_MAX + 1);
constexpr int ITEM_CATEGORY_JUNK = 0, ITEM_CATEGORY_MAJOR = 1,
              TEXT_RANDOMIZER_CUSTOM_ITEM = 0x9000;
constexpr int TEXTBOX_TYPE_BLUE = 2, ITEM_COMPASS = 0x75,
              ITEM_DUNGEON_MAP = 0x76, ITEM_SKULL_TOKEN = 0x71,
              ITEM_CUSTOM = 0x9C, ITEM_MASK_KEATON = 0x24, ITEM_MASK_SKULL = 0x25,
              ITEM_MASK_SPOOKY = 0x26, ITEM_MASK_GERUDO = 0x2A, ITEM_MASK_TRUTH = 0x2B;
using ItemID = int;
using TextBoxType = int;
using TextBoxPosition = int;
constexpr int OBJECT_INVALID = -1, ITEM_NONE = 0xFF,
              ITEM_ELEMENTAL_WAND = 0xD0, EXT_ITEM_SHEIKAH_SLATE = 0x220,
              ITEM_ROCS_FEATHER_SKIJER = 0xA4, RAND_INF_CAN_OPEN_CHEST = 0,
              RO_OPEN_CHEST_PROGRESSIVE = 1;
bool Flags_GetRandomizerInf(int) { return false; }
static uint8_t wandRule = 0;
uint8_t Wand_RandoMode() { return wandRule; }
constexpr int WAND_RANDO_MEDALLIONS = 0;
enum {
#define DEFINE_SCENE(a, b, scene, ...) scene,
#include "soh/include/tables/scene_table.h"
#undef DEFINE_SCENE
};
enum {
#define DEFINE_ENTRANCE(entrance, ...) entrance,
#include "soh/include/tables/entrance_table.h"
#undef DEFINE_ENTRANCE
};
constexpr int RO_MQ_DUNGEONS_NONE = 0, RO_MQ_DUNGEONS_SET_NUMBER = 1,
              MAX_MQ_DUNGEON_COUNT = 12, RO_BOSS_ROOM_ENTRANCE_SHUFFLE_OFF = 0,
              RO_GENERIC_ON = 1;
struct ReceiptOption {
  int value;
  bool Is(int other) const { return value == other; }
};
struct ReceiptPlacement {
  RandomizerGet item = RG_NONE;
  std::string name;
  mutable Text placedName{};
  RandomizerGet GetPlacedRandomizerGet() const { return item; }
  const Text &GetPlacedItemName() const {
    placedName = Text(name);
    return placedName;
  }
};
struct ReceiptContext {
  int mqMode = 2, mqCount = 6, bossShuffle = 1, dungeonShuffle = 0,
      information = 1, customItems = 1;
  bool generated = true, spoiler = false;
  bool IsSeedGenerated() const { return generated; }
  bool IsSpoilerLoaded() const { return spoiler; }
  struct Dungeon {
    bool mq = false;
    bool IsMQ() const { return mq; }
  } dungeon;
  ReceiptContext *GetDungeons() { return this; }
  Dungeon *GetDungeonFromScene(int scene) {
    assert(scene >= SCENE_DEKU_TREE && scene <= SCENE_ICE_CAVERN);
    return &dungeon;
  }
  std::map<RandomizerCheck, ReceiptPlacement> placements;
  ReceiptOption GetOption(int key) const {
    return {key == RSK_MQ_DUNGEON_RANDOM           ? mqMode
            : key == RSK_MQ_DUNGEON_COUNT          ? mqCount
            : key == RSK_SHUFFLE_BOSS_ENTRANCES    ? bossShuffle
            : key == RSK_SHUFFLE_DUNGEON_ENTRANCES ? dungeonShuffle
            : key == RSK_SKIJER_CUSTOM_ITEMS       ? customItems
                                                   : information};
  }
  ReceiptPlacement *GetItemLocation(RandomizerCheck check) {
    return &placements[check];
  }
} receiptContext;
static bool masterQuest = false;
static bool randoActive = true;
bool ResourceMgr_IsSceneMasterQuest(int scene) {
  assert(scene >= SCENE_DEKU_TREE && scene <= SCENE_ICE_CAVERN);
  return randoActive && masterQuest;
}
struct {
  int fileNum = 0, mapIndex = 0, language = 0;
  struct {
    int gsTokens = 0;
    uint8_t dungeonItems[10]{};
  } inventory;
} gSaveContext;
#define IS_RANDO randoActive
constexpr int DUNGEON_MAP = 2, DUNGEON_COMPASS = 1;
#define CHECK_DUNGEON_ITEM(item, scene)                                        \
  (gSaveContext.inventory.dungeonItems[scene] & (1 << (item)))
constexpr int TEXT_DESC_DUNGEON_MAP_INFO = 0x9300,
              TEXT_DESC_DUNGEON_COMPASS_INFO = 0x9301;
constexpr int ENTRANCE_OVERRIDES_MAX_COUNT = 8;
struct EntranceOverride {
  int16_t index, destination, override, overrideDestination;
} entranceOverrides[8]{};
EntranceOverride *Randomizer_GetEntranceOverrides() {
  return entranceOverrides;
}
bool Entrance_EntranceIsNull(const EntranceOverride *value) {
  return !value->index && !value->destination && !value->override &&
         !value->overrideDestination;
}
namespace EntranceTracker {
struct EntranceData {
  std::string source;
};
std::map<int, EntranceData> data;
const EntranceData *GetEntranceData(int entrance) {
  auto it = data.find(entrance);
  return it == data.end() ? nullptr : &it->second;
}
} // namespace EntranceTracker
struct ReceiptNeiSave {
  uint8_t comboObtained[FC_COMBO_OBTAINED_SIZE]{};
  uint8_t comboObtainedFc[FC_COMBO_OBTAINED_FC_SIZE]{};
  uint8_t comboAppliedFc[FC_COMBO_OBTAINED_FC_SIZE]{};
} receiptNeiSave;
ReceiptNeiSave *Nei_Save() { return &receiptNeiSave; }
using NeiSaveData = ReceiptNeiSave;
static bool receiptApplyingFc = false;
static int receiptSharedCalls = 0;
int FleetSync_IsApplyingFc() { return receiptApplyingFc; }
void FleetShared_OnNativeObtained(int) { ++receiptSharedCalls; }
/* NATIVE_GRANT_RECORD */
struct GetItemEntry {
  uint16_t textId;
  uint16_t itemId = ITEM_NONE;
};
struct MessageTableEntry {
  uint16_t textId;
  uint8_t typePos;
  const char *segment;
  uint32_t msgSize;
};
struct CustomItemMessageEntry {
  int rgId;
  int itemId;
  const char *english;
  const char *german;
  const char *french;
};
static bool stateAdvanced = false, throwOnRead = false;
static int liveResolutionCalls = 0;
struct TraditionalReceiptFixture {
  RandomizerGet item;
  const char* name;
  uint16_t nativeText, expectedText;
};
/* TRADITIONAL_CATALOG */
struct EquipmentBottleFixture {
  RandomizerGet item;
  const char *name;
  uint16_t text;
};
const EquipmentBottleFixture equipmentBottleReceipts[] = {
    {RG_KOKIRI_SWORD, "Kokiri Sword", 0xA4},
    {RG_GIANTS_KNIFE, "Giant's Knife", 0x4B},
    {RG_BIGGORON_SWORD, "Biggoron's Sword", 0x0C},
    {RG_DEKU_SHIELD, "Deku Shield", 0x4C},
    {RG_HYLIAN_SHIELD, "Hylian Shield", 0x4D},
    {RG_MIRROR_SHIELD, "Mirror Shield", 0x4E},
    {RG_EMPTY_BOTTLE, "Empty Bottle", 0x42},
    {RG_BOTTLE_WITH_RED_POTION, "Bottle with Red Potion", 0x43},
    {RG_BOTTLE_WITH_GREEN_POTION, "Bottle with Green Potion", 0x44},
    {RG_BOTTLE_WITH_BLUE_POTION, "Bottle with Blue Potion", 0x45},
    {RG_BOTTLE_WITH_FAIRY, "Bottle with Fairy", 0x46},
    {RG_BOTTLE_WITH_MILK, "Bottle with Milk", 0x98},
    {RG_BOTTLE_WITH_BLUE_FIRE, "Bottle with Blue Fire", 0x5D},
    {RG_RUTOS_LETTER, "Bottle with Ruto's Letter", 0x99},
    {RG_BOTTLE_WITH_FISH, "Bottle with Fish", 0x47},
    {RG_BOTTLE_WITH_BUGS, "Bottle with Bugs", 0x7A},
    {RG_BOTTLE_WITH_POE, "Bottle with Poe", 0x97},
    {RG_BOTTLE_WITH_BIG_POE, "Bottle with Big Poe", 0xF9}};
struct KeyCatalogFixture {
  RandomizerGet item;
  const char *name;
};
/* KEY_CATALOG */
namespace Rando::StaticData {
std::map<std::string, RandomizerGet> itemNameToEnum = {
    {"Spirit Medallion", RG_SPIRIT_MEDALLION},
    {"Forest Medallion", RG_FOREST_MEDALLION},
    {"Water Medallion", RG_WATER_MEDALLION},
    {"Fire Medallion", RG_FIRE_MEDALLION},
    {"Light Medallion", RG_LIGHT_MEDALLION},
    {"Shadow Medallion", RG_SHADOW_MEDALLION},
    {"Elemental Wand", RG_ELEMENTAL_WAND},
    {"Sand Rod", RG_WAND_SAND_ROD},
    {"Tornado Rod", RG_WAND_TORNADO_ROD},
    {"Water Rod", RG_WAND_WATER_ROD},
    {"Meteor Rod", RG_WAND_METEOR_ROD},
    {"Storm Rod", RG_WAND_STORM_ROD},
    {"Shadow Scepter", RG_WAND_SHADOW_SCEPTER},
    {"Sheikah Slate", RG_SHEIKAH_SLATE},
    {"Phantom Hourglass", RG_PHANTOM_HOURGLASS},
    {"Shadow Crystal", RG_SHADOW_CRYSTAL},
    {"Skull Mask", RG_SKULL_MASK},
    {"Spooky Mask", RG_SPOOKY_MASK},
    {"Mask of Truth", RG_MASK_OF_TRUTH},
    {"Mask of Truth (MM)", RG_MM_MASK_TRUTH},
    {"Gerudo Mask", RG_GERUDO_MASK},
    {"Keaton Mask", RG_KEATON_MASK},
    {"Keaton Mask (MM)", RG_MM_MASK_KEATON},
    {"Rune: Remote Bomb", RG_SLATE_RUNE_BOMB},
    {"Rune: Stasis", RG_SLATE_RUNE_STASIS},
    {"Rune: Cryonis", RG_SLATE_RUNE_CRYONIS},
    {"Rune: Master Cycle", RG_SLATE_RUNE_MASTER_CYCLE},
    {"Rune: Sheikah Sensor", RG_DESIRE_SENSOR},
    {"Cane of Somaria", RG_CANE_OF_SOMARIA},
    {"Progressive Roc", RG_PROGRESSIVE_ROCS},
    {"Stone of Agony", RG_STONE_OF_AGONY},
    {"Magic Meter", RG_MAGIC_SINGLE},
    {"Enhanced Magic Meter", RG_MAGIC_DOUBLE},
    {"Deku Leaf", RG_DEKU_LEAF},
    {"Power Upgrade", RG_POWER_UPGRADE},
    {"Magic Stat Upgrade", RG_MAGIC_STAT_UPGRADE},
    {"Goht's Remains", RG_MM_REMAINS_GOHT},
    {"Goron Lullaby", RG_MM_SONG_LULLABY},
    {"Goron Lullaby Intro", RG_MM_SONG_LULLABY_INTRO},
    {"New Wave Bossa Nova", RG_MM_SONG_NOVA},
    {"Song of Healing", RG_MM_SONG_HEALING},
    {"Song of Storms (MM)", RG_MM_SONG_STORMS},
    {"Song of Soaring", RG_MM_SONG_SOARING},
    {"Great Deku Tree Compass", RG_DEKU_TREE_COMPASS},
    {"Great Deku Tree Map", RG_DEKU_TREE_MAP},
    {"Forest Temple Small Key", RG_FOREST_TEMPLE_SMALL_KEY},
    {"Forest Temple Boss Key", RG_FOREST_TEMPLE_BOSS_KEY},
    {"Ganon's Castle Boss Key", RG_GANONS_CASTLE_BOSS_KEY},
    {"Minuet of Forest", RG_MINUET_OF_FOREST},
    {"Snowhead Compass", RG_MM_COMPASS_SNOWHEAD},
    {"Woodfall Map", RG_MM_MAP_WOODFALL},
    {"Snowhead Map", RG_MM_MAP_SNOWHEAD},
    {"Great Bay Map", RG_MM_MAP_GREAT_BAY},
    {"Stone Tower Map", RG_MM_MAP_STONE_TOWER},
    {"Ice Cavern Compass", RG_ICE_CAVERN_COMPASS},
    {"Bottom of the Well Compass", RG_BOTTOM_OF_THE_WELL_COMPASS},
    {"Gold Skulltula Token", RG_GOLD_SKULLTULA_TOKEN},
    {"Swamp Gold Skulltula Token", RG_MM_GS_TOKEN_SWAMP},
    {"Ocean Gold Skulltula Token", RG_MM_GS_TOKEN_OCEAN},
    {"Clock Town Stray Fairy", RG_MM_STRAY_FAIRY},
    {"Woodfall Stray Fairy", RG_MM_STRAY_FAIRY_WOODFALL},
    {"Snowhead Stray Fairy", RG_MM_STRAY_FAIRY_SNOWHEAD},
    {"Great Bay Stray Fairy", RG_MM_STRAY_FAIRY_GREAT_BAY},
    {"Stone Tower Stray Fairy", RG_MM_STRAY_FAIRY_STONE_TOWER},
    {"Ice Trap", RG_ICE_TRAP},
    {"Green Rupee", RG_GREEN_RUPEE},
    {"None", RG_NONE}};
struct ReceiptItemName : std::string {
  ReceiptItemName(const std::string& value) : std::string(value) {}
  const std::string& GetEnglish() const { return *this; }
};
struct Item {
  RandomizerGet id;
  int GetCategory() {
    return id == RG_GREEN_RUPEE ? ITEM_CATEGORY_JUNK : ITEM_CATEGORY_MAJOR;
  }
  ReceiptItemName GetName() const {
    for (const auto &[name, rg] : itemNameToEnum)
      if (rg == id)
        return name;
    return std::string("Item");
  }
  std::string GetColor() const { return "%g"; }
  std::string GetArticle() const {
    for (const auto &fixture : KeyReceiptFixtures::entries)
      if (!fixture.mm && GetName() == fixture.name)
        return fixture.article;
    return {}; // Imported MM rows have no localized article in the OoT catalog.
  }
  bool HasCustomIcon() { return HasSoulIcon() || HasBottleIcon(); }
  bool HasSoulIcon() const {
    return (id >= RG_GOHMA_SOUL && id <= RG_GANON_SOUL) ||
           (id >= RG_MM_SOUL_GOHT && id <= RG_MM_SOUL_TWINMOLD);
  }
  int GetItemID() const {
    if (HasSoulIcon())
      return id <= RG_GANON_SOUL ? 0xE0 + id - RG_GOHMA_SOUL : 0x111 + id - RG_MM_SOUL_GOHT;
    return HasBottleIcon() ? ITEM_CUSTOM : ITEM_NONE;
  }
  bool HasBottleIcon() const { return id == RG_BOTTLE_WITH_FISH || id == RG_BOTTLE_WITH_BUGS || id == RG_BOTTLE_WITH_POE || id == RG_BOTTLE_WITH_BIG_POE; }
  std::shared_ptr<GetItemEntry> GetGIEntryUnresolved() const {
    if (throwOnRead)
      throw 1;
    uint16_t text = id == RG_STONE_OF_AGONY      ? 0x68
                    : id == RG_MAGIC_SINGLE      ? 0xE4
                    : id == RG_MAGIC_DOUBLE      ? 0xE8
                    : id == RG_DEKU_TREE_COMPASS ? 0x67
                    : id == RG_DEKU_TREE_MAP     ? 0x66
                    : id == RG_GREEN_RUPEE       ? 0x6F
                    : id == RG_MINUET_OF_FOREST  ? 0x73
                    : id == RG_FIRE_MEDALLION    ? 0x3C
                    : id == RG_WATER_MEDALLION   ? 0x3D
                    : id == RG_FOREST_MEDALLION  ? 0x3E
                    : id == RG_SPIRIT_MEDALLION  ? 0x3F
                    : id == RG_LIGHT_MEDALLION   ? 0x40
                    : id == RG_SHADOW_MEDALLION  ? 0x41
                                                 : TEXT_RANDOMIZER_CUSTOM_ITEM;
    for (const auto& fixture : traditionalReceipts)
      if (fixture.item == id)
        text = fixture.nativeText;
    for (const auto& fixture : equipmentBottleReceipts)
      if (fixture.item == id && id <= RG_MIRROR_SHIELD)
        text = fixture.text;
    uint16_t icon = ITEM_NONE;
    if (id >= RG_FOREST_TEMPLE_SMALL_KEY && id <= RG_TREASURE_GAME_SMALL_KEY)
      icon = 0x77;
    if (id >= RG_FOREST_TEMPLE_BOSS_KEY && id <= RG_GANONS_CASTLE_BOSS_KEY)
      icon = 0x74;
    return std::make_shared<GetItemEntry>(GetItemEntry{text, icon});
  }
  std::shared_ptr<GetItemEntry>
  GetGIEntry(RandomizerGet *actual = nullptr) const {
    ++liveResolutionCalls;
    if (actual && stateAdvanced && id == RG_CANE_OF_SOMARIA)
      *actual = RG_CANE_PACCI_FLIP;
    if (actual && stateAdvanced && id == RG_PROGRESSIVE_ROCS)
      *actual = RG_ROCS_CAPE;
    if (actual && stateAdvanced && id == RG_STONE_OF_AGONY)
      *actual = RG_QUARTZ_OF_MOTION;
    return GetGIEntryUnresolved();
  }
};
Item RetrieveItem(RandomizerGet id) { return {id}; }
} // namespace Rando::StaticData
struct OTRGlobals {
  static OTRGlobals *Instance;
  ReceiptContext *gRandoContext = &receiptContext;
  void *gRandomizer = reinterpret_cast<void *>(1);
};
OTRGlobals globals;
OTRGlobals *OTRGlobals::Instance = &globals;
/* DONOR_MESSAGES */
const CustomItemMessageEntry *GetCustomItemMessage(int rg) {
  static CustomItemMessageEntry localized{};
  for (const auto &msg : receiptMessages) {
    if (msg.rgId != rg)
      continue;
    localized = msg;
    // Production GetCustomItemMessage fills missing registry languages with
    // English before constructing a CustomMessage.
    if (!localized.german)
      localized.german = localized.english;
    if (!localized.french)
      localized.french = localized.english;
    return &localized;
  }
  return nullptr;
}
static const char stone[] =
    "You found the \x05\x44Stone of Agony\x05\x40!\x01It reacts to hidden "
    "secrets.\x02";
static const char magic[] =
    "You received the Magic Meter!\x04Use magic items with it.\x02";
static const char doubleMagic[] =
    "Your Magic Meter is enhanced!\x04You can use twice as much magic.\x02";
static const char compass[] =
    "You got the Compass!\x01Now you can see hidden things.\x02";
static const char dungeonMap[] =
    "You got the Dungeon Map!\x01" "Blue rooms are places you have visited.\x02";
static const char smallKey[] =
    "You got a Small Key!\x01This key will open a locked door in this dungeon.\x02";
static const char chestGameKey[] =
    "You got a Key!\x01It opens the next door in the Treasure Chest Game.\x02";
static const char bossKey[] =
    "You got the Boss Key!\x01Now you can get inside the chamber where the Boss lurks.\x02";
static const char minuet[] =
    "You have learned the Minuet of Forest!\x01" "A melody that will take you to the forest.\x02";
static const char greenRupee[] = "You got a Green Rupee!\x01It is worth one Rupee.\x02";
static const char medallionPrefix[] = "Native medallion reward text.\x02";
MessageTableEntry nativeTable[] = {
    {0x3C, 0, medallionPrefix, sizeof(medallionPrefix)-1},
    {0x3D, 0, medallionPrefix, sizeof(medallionPrefix)-1},
    {0x3E, 0, medallionPrefix, sizeof(medallionPrefix)-1},
    {0x3F, 0, medallionPrefix, sizeof(medallionPrefix)-1},
    {0x40, 0, medallionPrefix, sizeof(medallionPrefix)-1},
    {0x41, 0, medallionPrefix, sizeof(medallionPrefix)-1},
    {0x6F,0,greenRupee,sizeof(greenRupee)-1},
    {0x68, 0, stone, sizeof(stone) - 1},
    {0xE4, 0, magic, sizeof(magic) - 1},
    {0xE8, 0, doubleMagic, sizeof(doubleMagic) - 1},
    {0x67, 0, compass, sizeof(compass) - 1},
    {0x66, 0, dungeonMap, sizeof(dungeonMap) - 1},
    {0x60, 0, smallKey, sizeof(smallKey) - 1},
    {0xF3, 0, chestGameKey, sizeof(chestGameKey) - 1},
    {0xC7, 0, bossKey, sizeof(bossKey) - 1},
    {0x73, 0, minuet, sizeof(minuet) - 1},
    {0xFFFF, 0, nullptr, 0}};
MessageTableEntry *sNesMessageEntryTablePtr = nativeTable;
MessageTableEntry *sGerMessageEntryTablePtr = nullptr, *sFraMessageEntryTablePtr = nullptr;
constexpr int MF_RAW = 0;
static std::string loadedMessage;
static std::string nativePrefixControls, nativeEnding = "\x02";
static CwItemReceiptPresentation loadedPresentation{};
struct CustomMessage {
  std::string english, german, french;
  int type = 0, position = 0;
  CwItemReceiptPresentation receiptPresentation{};
  void operator+=(const std::string& value) { english += value; }
  CustomMessage() = default;
  CustomMessage(std::string en, int boxType = 0, int boxPosition = 0)
      : english(std::move(en)), type(boxType), position(boxPosition) {}
  CustomMessage(const Text& text, int = 0) : english(text.GetEnglish()) {}
  CustomMessage(std::string en, std::string ger, std::string fre, int boxType = 0, int boxPosition = 0)
      : english(std::move(en)), german(std::move(ger)), french(std::move(fre)), type(boxType), position(boxPosition) {}
  static CustomMessage LoadVanillaMessageTableEntry(uint16_t textId) {
    assert(textId >= 0x3C && textId <= 0x41);
    // Engine table boundary: the production loader returns the selected
    // locale's native prefix in its English slot.
    return CustomMessage(nativePrefixControls + std::string("\x13\x69") + (gSaveContext.language == 1 ? "Native German reward." :
                                    gSaveContext.language == 2 ? "Native French reward." :
                                                               "Native English reward.") + nativeEnding, 2, 3);
  }
  int GetTextBoxType() const { return type; }
  int GetTextBoxPosition() const { return position; }
  void Replace(const char *key, const CustomMessage &value) {
    const auto replace = [&](std::string& body, const std::string& replacement) {
      size_t pos = 0;
      while ((pos = body.find(key, pos)) != std::string::npos) {
        body.replace(pos, std::strlen(key), replacement);
        pos += replacement.size();
      }
    };
    replace(english, value.english);
    replace(german, value.german.empty() ? value.english : value.german);
    replace(french, value.french.empty() ? value.english : value.french);
  }
  void Replace(const char *key, const std::string &value) {
    Replace(key, CustomMessage(value));
  }
  void Replace(const char *key, const char *value) {
    Replace(key, std::string(value));
  }
  void Replace(std::string key, std::string value) {
    Replace(key.c_str(), value);
  }
  static std::string ITEM_OBTAINED(uint8_t icon) {
    return std::string("\x13") + static_cast<char>(icon);
  }
  static std::string NEWLINE() { return "\x01"; }
  static std::string WAIT_FOR_INPUT() { return "\x04"; }
  static std::string PLAYER_NAME() { return "\x0F"; }
  static std::string MESSAGE_END() { return "\x02"; }
  static std::string COLOR(std::string c) { return "\x05" + c; }
  std::vector<std::string> colors;
  void FormatString(std::string& str) const;
  void AutoFormatString(std::string& str) const;
  void ReplaceSpecialCharacters(std::string& str) const;
  void ReplaceColors(std::string& str) const;
  void ReplaceAltarIcons(std::string& str) const;
  void EncodeColors(std::string& str) const;
  size_t FindNEWLINE(std::string& str, size_t start) const;
  bool AddBreakString(std::string& str, size_t pos, std::string br) const;
  void Format() {
    for (auto* value : {&english, &german, &french})
      if (!value->empty()) FormatString(*value);
  }
  void AutoFormat() {
    for (auto* value : {&english, &german, &french})
      if (!value->empty()) AutoFormatString(*value);
  }
  void AutoFormat(int icon) {
    for (auto* value : {&english, &german, &french})
      if (!value->empty()) value->insert(0, ITEM_OBTAINED(icon));
    AutoFormat();
    Replace(WAIT_FOR_INPUT(), WAIT_FOR_INPUT() + ITEM_OBTAINED(icon));
  }
  std::string GetEnglish(int) const { return english; }
  std::string GetGerman(int) const { return german.empty() ? english : german; }
  std::string GetFrench(int) const { return french.empty() ? english : french; }
  void LoadIntoFont() const {
    loadedMessage = gSaveContext.language == 1 && !german.empty() ? german :
                    gSaveContext.language == 2 && !french.empty() ? french : english;
    loadedPresentation = receiptPresentation;
  }
};
/* ACTUAL_FORMATTER */
constexpr int RHT_DUNGEON_ORDINARY = 0, RHT_DUNGEON_MASTERFUL = 1;
namespace Rando::StaticData {
struct ReceiptHint {
  CustomMessage message;
  CustomMessage GetHintMessage() const { return message; }
};
ReceiptHint hintTextTable[] = {{CustomMessage("&It's %gordinary%w.")},
                               {CustomMessage("&It's %rmasterful%w!")}};
} // namespace Rando::StaticData
void BuildQuarterHeartMessage(CustomMessage &msg) {
  msg.english = "A Quarter Heart!\x01You gained quarter of a heart.\x02";
}
void BuildDefenseUpgradeMessage(CustomMessage &msg) {
  msg.english = "Defense Upgrade!\x01Max defense reached!\x02";
}
void BuildSpeedUpgradeMessage(CustomMessage &msg) {
  msg.english = "Speed Upgrade!\x01Max speed reached!\x02";
}
void BuildPowerUpgradeMessage(CustomMessage &msg) {
  msg.english = "Power Upgrade!\x01Raises your double damage chance!\x02";
}
void BuildMagicStatUpgradeMessage(CustomMessage &msg) {
  msg.english =
      std::string("Magic Meter!\x05\x00", 14) + "more to reach max stat.\x02";
}
void BuildCrawlSpeedUpgradeMessage(CustomMessage &msg) {
  msg.english = "Crawl Speed Upgrade!\x02";
}
void BuildClimbSpeedUpgradeMessage(CustomMessage &msg) {
  msg.english = "Climb Speed Upgrade!\x02";
}
void BuildPushSpeedUpgradeMessage(CustomMessage &msg) {
  msg.english = "Push Speed Upgrade!\x02";
}
constexpr int MOD_RANDOMIZER = 1;
struct Player {
  struct {
    int comboForeignCheck = RC_RECEIPT_FIXTURE, getItemId = RG_NONE,
        modIndex = MOD_RANDOMIZER, itemId = 0, objectId = 0;
  } getItemEntry;
  int getItemId = RG_NONE;
} receiptPlayer;
struct PlayState {
  int sceneNum = SCENE_DEKU_TREE;
} play;
PlayState *gPlayState = &play;
#define GET_PLAYER(play) (&receiptPlayer)
namespace ComboRando {
constexpr int GAME_MM = 1, GAME_OOT = 0;
std::string StripGameSuffix(const std::string& name) {
  if (name.ends_with(" (MM)")) return name.substr(0, name.size() - 5);
  if (name.ends_with(" (OOT)")) return name.substr(0, name.size() - 6);
  return name;
}
struct ForeignItem {
  int itemGame = GAME_MM;
  bool trap = false;
  std::string itemName = "Goht's Remains", displayName = "Goht's Remains (MM)",
              fakeTrickName;
  bool HasDisguise() const { return !fakeTrickName.empty(); }
};
std::string ShownForeignName(const ForeignItem &, const char *name) {
  return std::string(name) + " (MM)";
}
} // namespace ComboRando
static ComboRando::ForeignItem foreign;
static ComboRando::ForeignItem rewardForeign;
static const char *resolvedForeign = nullptr;
static int foreignLatches = 0;
const ComboRando::ForeignItem *OOT_LookupForeign(int, const std::string &) {
  return &foreign;
}
const ComboRando::ForeignItem *OOT_LookupForeignByCheck(RandomizerCheck check) {
  assert(check == RC_PHANTOM_GANON);
  return &rewardForeign;
}
static bool mmRewardAvailable = false;
int32_t FixtureMmReward(int32_t dungeon, char *buffer, uint32_t capacity) {
  assert(dungeon == 1);
  const std::string reward = "Progressive Hookshot (OOT)";
  assert(capacity > reward.size());
  std::memcpy(buffer, reward.c_str(), reward.size() + 1);
  return reward.size();
}
static bool seedIconAvailable = true;
extern "C" int32_t OOT_GetSeedItemIconInfo(const char* name, CwItemIconInfo* out) {
  *out = {};
  if (!seedIconAvailable) return 0;
  out->path = std::strcmp(name, "Fire Medallion") == 0
      ? "__OTR__textures/icon_item_24_static/gQuestIconMedallionFireTex"
      : std::strcmp(name, "Kokiri Emerald") == 0
      ? "__OTR__textures/icon_item_24_static/gQuestIconKokiriEmeraldTex"
      : "__OTR__textures/icon_item_custom/gItemIconDekuLeafTex";
  out->width = out->height = std::strcmp(name, "Fire Medallion") == 0 || std::strcmp(name, "Kokiri Emerald") == 0 ? 24 : 32;
  return 1;
}
static int32_t FixtureSeedIcon(const char* name, CwItemIconInfo* out) {
  assert(std::string(name) == "Progressive Goron Lullaby");
  *out = {};
  if (!seedIconAvailable) return 0;
  out->path = "__OTR__icon_item_static_yar/gItemIconSongNoteTex";
  out->width = 16; out->height = 24; out->isIA8 = 1;
  return 1;
}
#ifndef COMPASS_DONOR_BRIDGE
static int32_t FixtureMmInformationName(int32_t dungeon,char* buffer,uint32_t capacity,bool compass) {
  if(dungeon<0 || dungeon>=4 || !buffer || !capacity)return 0;
  const char* entrances[]={"Woodfall","Snowhead","Zora Cape's turtle","Stone Tower"};
  const char* bosses[]={"Odolwa","Goht","Gyorg","Twinmold"};
  const std::string text=compass?bosses[dungeon]:entrances[dungeon];
  if(text.size()>=capacity)return 0;
  std::memcpy(buffer,text.c_str(),text.size()+1);return text.size();
}
static int32_t FixtureMmEntranceName(int32_t d,char* b,uint32_t c){return FixtureMmInformationName(d,b,c,false);}
static int32_t FixtureMmBossName(int32_t d,char* b,uint32_t c){return FixtureMmInformationName(d,b,c,true);}
#endif
void *Combo_ResolveSym(const char *module, const char *symbol) {
  assert(std::string(module) == "2ship");
#ifdef COMPASS_DONOR_BRIDGE
  if(std::string(symbol)=="MM_GetDungeonRewardName" || std::string(symbol)=="MM_GetDungeonRewardIconInfo" ||
     std::string(symbol)=="MM_GetSeedItemIconInfo")return dlsym(RTLD_DEFAULT,symbol);
#endif
  if(std::string(symbol)=="MM_GetDungeonEntranceName" || std::string(symbol)=="MM_GetDungeonBossName" ||
     std::string(symbol)=="MM_GetCrossDungeonEntranceSourceName" || std::string(symbol)=="MM_GetInformationBossDestination" ||
     std::string(symbol)=="MM_GetPhysicalBossRewardName" || std::string(symbol)=="MM_GetPhysicalBossRewardIconInfo") {
#ifdef COMPASS_DONOR_BRIDGE
    return dlsym(RTLD_DEFAULT,symbol);
#else
    if(std::string(symbol)=="MM_GetDungeonEntranceName")return reinterpret_cast<void*>(FixtureMmEntranceName);
    if(std::string(symbol)=="MM_GetDungeonBossName")return reinterpret_cast<void*>(FixtureMmBossName);
    return nullptr;
#endif
  }
  if (std::string(symbol) == "MM_GetDungeonRewardName")
    return mmRewardAvailable ? reinterpret_cast<void *>(FixtureMmReward) : nullptr;
  if (std::string(symbol) == "MM_GetSeedItemIconInfo")
    return reinterpret_cast<void *>(FixtureSeedIcon);
  assert(std::string(symbol) == "MM_GetDungeonRewardIconInfo");
  return nullptr;
}
void Randomizer_LatchComboForeign(RandomizerCheck) { ++foreignLatches; }
const char *Randomizer_ComboForeignLatchedName(int32_t) {
  return resolvedForeign;
}
namespace Rando::StaticData {
struct Location {
  std::string GetName() const { return "fixture"; }
} receiptLocation;
Location *GetLocation(RandomizerCheck) { return &receiptLocation; }
} // namespace Rando::StaticData
namespace Rando::Traps {
void BuildIceTrapMessageNamed(CustomMessage &msg, const std::string &) {
  msg.english = "Trap receipt";
}
} // namespace Rando::Traps
#ifndef COMBO_EXPORT
#define COMBO_EXPORT
#endif
/* CONTEXT_BUILDERS */
/* NATIVE_ITEM_BUILDER */
/* FOREIGN_BUILDER */
/* DONOR_EXPORT */

// Test-only entry point for the receiver integration. OoT is dormant while MM
// owns the active save; its generated seed still supplies this saved option.
extern "C" COMBO_EXPORT void FixtureConfigureWandReceipt(int rule, int customItems) {
  wandRule = rule;
  receiptContext.customItems = customItems;
}

extern "C" COMBO_EXPORT void FixtureConfigureForestCompassReceipt(int information) {
  for (const auto &fixture : traditionalReceipts)
    Rando::StaticData::itemNameToEnum[fixture.name] = fixture.item;
  receiptContext.information = information;
  receiptContext.generated = true;
  receiptContext.spoiler = false;
  receiptContext.bossShuffle = RO_BOSS_ROOM_ENTRANCE_SHUFFLE_OFF;
  receiptContext.mqMode = 2;
  receiptContext.mqCount = 6;
  receiptContext.dungeon.mq = true;
  receiptContext.placements[RC_PHANTOM_GANON] = {RG_DEKU_LEAF, "Deku Leaf"};
  randoActive = false;
  gPlayState = nullptr;
}

extern "C" COMBO_EXPORT void FixtureConfigurePhysicalBossHints(int nativeSwap) {
  FixtureConfigureForestCompassReceipt(1);
  receiptContext.placements[RC_QUEEN_GOHMA]={RG_DEKU_LEAF,"Deku Leaf"};
  std::fill(std::begin(entranceOverrides),std::end(entranceOverrides),EntranceOverride{});
  receiptContext.dungeonShuffle=0;
  if(nativeSwap==1) {
    entranceOverrides[0]={ENTR_FOREST_TEMPLE_ENTRANCE,1,ENTR_DEKU_TREE_ENTRANCE,1};
    entranceOverrides[1]={ENTR_DEKU_TREE_ENTRANCE,1,ENTR_FOREST_TEMPLE_ENTRANCE,1};
  } else if(nativeSwap==2 || nativeSwap==3) {
    receiptContext.bossShuffle=1;
    entranceOverrides[0]={ENTR_FOREST_TEMPLE_BOSS_ENTRANCE,1,
        nativeSwap==2 ? ENTR_DEKU_TREE_BOSS_ENTRANCE : ENTR_DEKU_TREE_ENTRANCE,1};
  }
}

static void CheckForestCompassReceiptInformation() {
  const auto contextBefore = receiptContext;
  auto *playBefore = gPlayState;
  const bool activeBefore = randoActive;
  const auto saveBefore = gSaveContext;
  const int resolutionsBefore = liveResolutionCalls;
  const int latchesBefore = foreignLatches;
  for (int information : {0, 1}) {
    FixtureConfigureForestCompassReceipt(information);
    char buffer[1269];
    const int32_t size = OOT_GetItemReceiptText("Forest Temple Compass", buffer, sizeof(buffer));
    assert(size > 0);
    const std::string text(buffer, size);
    CwItemReceiptPresentation presentation{};
    assert(OOT_GetDungeonItemReceiptPresentation("Forest Temple Compass", &presentation) == information);
    if (information) {
      assert(ComboReceipt_HasIcon(&presentation));
      assert(std::string(presentation.iconPath) == "__OTR__@oot:textures/icon_item_custom/gItemIconDekuLeafTex");
      assert(presentation.rewardLine == 1 && presentation.iconWidth == 32 && presentation.iconHeight == 32);
    }
    assert(text.find("Forest Temple Compass") != std::string::npos);
    if (information) {
      assert(text.find("You got the Compass!") == std::string::npos && "On must replace the native tutorial");
      assert(text.find("Now you can see hidden things.") == std::string::npos);
      assert(text.find("masterful") == std::string::npos && "Compass receipt has only its title and boss with reward icon");
      assert(text.find("It points to") != std::string::npos);
      assert(text.find("Phantom Ganon") != std::string::npos);
      assert(text.find("Defeating the boss grants") == std::string::npos);
      assert(text.find("You received a ") != std::string::npos);
      assert(text.find('\x10') == std::string::npos && "Compass information is one textbox");
      assert(std::count(text.begin(), text.end(), '\x11') == 1);
    } else {
      assert(text.find("You got the Compass!") != std::string::npos);
      assert(text.find("Now you can see hidden things.") != std::string::npos);
      assert(text.find("masterful") != std::string::npos);
      assert(text.find("Phantom Ganon") == std::string::npos);
      assert(text.find("Defeating the boss grants") == std::string::npos);
    }
  }
  assert(!std::memcmp(&saveBefore, &gSaveContext, sizeof(saveBefore)));
  assert(liveResolutionCalls == resolutionsBefore && foreignLatches == latchesBefore);
  receiptContext = contextBefore;
  gPlayState = playBefore;
  randoActive = activeBefore;
  std::cout << "Forest Temple Compass: saved Off tutorial; saved On two-line boss/reward sprite, dormant seed preserved passed\n";
}

static void CheckApprovedCompassExample() {
  const auto contextBefore = receiptContext;
  const auto overridesBefore = std::to_array(entranceOverrides);
  receiptContext.information = 1;
  receiptContext.bossShuffle = 1;
  receiptContext.placements[RC_QUEEN_GOHMA] = {RG_KOKIRI_EMERALD, "Kokiri Emerald"};
  receiptContext.placements[RC_VOLVAGIA] = {RG_FIRE_MEDALLION, "Fire Medallion"};
  for (bool shuffled : {false, true}) {
    entranceOverrides[0] = {ENTR_DEKU_TREE_BOSS_ENTRANCE, 1,
        shuffled ? ENTR_FIRE_TEMPLE_BOSS_ENTRANCE : ENTR_DEKU_TREE_BOSS_ENTRANCE, 1};
    char buffer[1269];
    const int size = OOT_GetItemReceiptText("Great Deku Tree Compass", buffer, sizeof(buffer));
    assert(size > 0);
    std::string plain;
    for (int i = 0; i < size; ++i) {
      const uint8_t c = buffer[i];
      if (c == 0x11) plain += '\n';
      else if (c >= 0x20) plain += c;
    }
    assert(plain == std::string("You received a Deku Tree Compass!\nIt points to ") +
        (shuffled ? "Volvagia" : "Queen Gohma"));
    CwItemReceiptPresentation presentation{};
    assert(OOT_GetDungeonItemReceiptPresentation("Great Deku Tree Compass", &presentation) == 1);
    assert(std::string(presentation.iconPath) == std::string("__OTR__@oot:textures/icon_item_24_static/") +
        (shuffled ? "gQuestIconMedallionFireTex" : "gQuestIconKokiriEmeraldTex"));
    assert(presentation.rewardLine == 1 && presentation.iconWidth == 24 && presentation.iconHeight == 24);
  }
  assert(liveResolutionCalls == 0);
  receiptContext = contextBefore;
  std::copy(overridesBefore.begin(), overridesBefore.end(), entranceOverrides);
}

extern "C" COMBO_EXPORT void FixtureConfigureGeneratedCompassRoute(const char* fixturePath, const char* route) {
  FixtureConfigureForestCompassReceipt(1);
  receiptContext.bossShuffle = 1;
  receiptContext.placements[RC_VOLVAGIA] = {RG_FIRE_MEDALLION, "Fire Medallion"};
  std::ifstream input(fixturePath);
  const auto fixtures = nlohmann::json::parse(input);
  std::fill(std::begin(entranceOverrides), std::end(entranceOverrides), EntranceOverride{});
  size_t i = 0;
  for (const auto& row : fixtures.at(route)) {
    assert(i < std::size(entranceOverrides));
    entranceOverrides[i++] = {row.at("index"), row.at("destination"), row.at("override"),
                              row.at("overrideDestination")};
  }
}

static void CheckGeneratedCompassRoutes(const char* fixturePath) {
  if (!fixturePath) return;
  const auto contextBefore = receiptContext;
  const auto overridesBefore = std::to_array(entranceOverrides);
  auto* playBefore = gPlayState;
  const bool activeBefore = randoActive;
  for (const auto* route : {"direct", "nested", "cycle", "deadEnd"}) {
    FixtureConfigureGeneratedCompassRoute(fixturePath, route);
    char buffer[1269];
    const int size = OOT_GetItemReceiptText("Forest Temple Compass", buffer, sizeof(buffer));
    assert(size > 0);
    const std::string text(buffer, size);
    const bool bossAssigned = std::string(route) == "direct" || std::string(route) == "nested";
    assert((text.find("Volvagia") != std::string::npos) == bossAssigned &&
           "native generated nested entrance must reveal the actual reachable boss");
    assert(text.find("Fire Medallion") == std::string::npos);
    assert(text.find("Phantom Ganon") == std::string::npos && "never invent the vanilla boss for a mixed route");
    CwItemReceiptPresentation p{};
    assert(OOT_GetDungeonItemReceiptPresentation("Forest Temple Compass", &p) == 1);
    assert(ComboReceipt_HasIcon(&p) == bossAssigned);
    if (bossAssigned) assert(p.rewardLine == 1 && std::strstr(p.iconPath, "MedallionFire"));
    if (!bossAssigned)
      assert(text.find("boss room") != std::string::npos && "unknown routes need an explanation, not a title alone");
  }
  receiptContext = contextBefore;
  std::copy(overridesBefore.begin(), overridesBefore.end(), entranceOverrides);
  gPlayState = playBefore;
  randoActive = activeBefore;
  std::cout << "Native generated/dumped forward routes: direct and nested boss/reward identity, cycles/dead ends, and no vanilla invention passed\n";
}

int main(int argc, char** argv) {
  const struct {
    RandomizerGet id;
    const char *name;
    int color;
  } souls[] = {
#define SOUL_ROW(mm, oot, name, color) {oot, name, color},
      BOSS_SOUL_RECEIPT_FIXTURES(SOUL_ROW)
#undef SOUL_ROW
  };
  const auto soulPlayer = receiptPlayer;
  const auto soulForeign = foreign;
  const auto soulResolved = resolvedForeign;
  const auto soulLatches = foreignLatches;
  const auto soulResolutions = liveResolutionCalls;
  for (const auto &soul : souls) {
    Rando::StaticData::itemNameToEnum[soul.name] = soul.id;
    receiptPlayer.getItemEntry.getItemId = soul.id;
    const auto before = gSaveContext;
    CustomMessage native;
    BuildCustomItemMessage(&receiptPlayer, native);
    for (const auto &text :
         {native.GetEnglish(MF_RAW), native.GetGerman(MF_RAW),
          native.GetFrench(MF_RAW)}) {
      std::string body;
      const auto item = Rando::StaticData::RetrieveItem(soul.id);
      const auto icon = CustomMessage::ITEM_OBTAINED(static_cast<uint8_t>(item.GetItemID()));
      assert(text.find(icon) != std::string::npos && "native soul lost its catalog icon");
      assert(ComboItemReceiptText::FromOotMessage(text, body));
      const auto color = body.find(static_cast<char>(soul.color));
      const auto reset = body.find('\0', color);
      assert(color != std::string::npos && reset > color + 3 &&
             reset != std::string::npos);
    }
    assert(native.GetGerman(MF_RAW).find("Du hast ") != std::string::npos);
    assert(native.GetFrench(MF_RAW).find("Vous avez") != std::string::npos);
    assert(std::memcmp(&before, &gSaveContext, sizeof(before)) == 0);
    if (soul.id >= RG_MM_SOUL_GOHT && soul.id <= RG_MM_SOUL_TWINMOLD) {
      foreign.itemGame = ComboRando::GAME_MM;
      foreign.itemName = soul.name;
      foreign.displayName = std::string(soul.name) + " (MM)";
      foreign.trap = false;
      foreign.fakeTrickName.clear();
      resolvedForeign = soul.name;
      CustomMessage imported;
      BuildComboForeignMessage(&receiptPlayer, imported);
      for (const auto& texts : {std::pair{native.GetEnglish(MF_RAW), imported.GetEnglish(MF_RAW)},
                                std::pair{native.GetGerman(MF_RAW), imported.GetGerman(MF_RAW)},
                                std::pair{native.GetFrench(MF_RAW), imported.GetFrench(MF_RAW)}}) {
        std::string nativeBody;
        assert(ComboItemReceiptText::WithoutOotItemIcons(texts.first, nativeBody));
        assert(texts.second.find('\x13') == std::string::npos && "foreign sentinel must remain iconless");
        auto flatten = [](std::string body) {
          // An icon reserves line width, so wrapping (including a trailing
          // newline after punctuation) can differ without changing the body.
          body.erase(std::remove_if(body.begin(), body.end(), [](char c) { return c == ' ' || c == '\x01'; }), body.end());
          return body;
        };
        assert(flatten(nativeBody) == flatten(texts.second));
      }
    }
  }
  receiptPlayer = soulPlayer;
  foreign = soulForeign;
  resolvedForeign = soulResolved;
  foreignLatches = soulLatches;
  liveResolutionCalls = soulResolutions;
  std::cout << "OoT native/imported boss soul receipts: localized themed names "
               "and white body reset passed\n";
  // Item names and unresolved native text IDs come from the real OoT catalog.
  // Message-table ownership is the seam; unique bodies reveal wrong IDs.
  std::vector<std::string> songBodies;
  songBodies.reserve(std::size(traditionalReceipts));
  std::vector<MessageTableEntry> nativeMessages(nativeTable, nativeTable + std::size(nativeTable) - 1);
  for (const auto& fixture : traditionalReceipts) {
    Rando::StaticData::itemNameToEnum[fixture.name] = fixture.item;
    if (fixture.item < RG_ZELDAS_LULLABY || fixture.item > RG_PRELUDE_OF_LIGHT || fixture.expectedText == 0x73)
      continue;
    songBodies.emplace_back(std::string("Native song tutorial: ") + fixture.name + "\x02");
    const auto& body = songBodies.back();
    nativeMessages.push_back({fixture.expectedText, 0, body.c_str(), static_cast<uint32_t>(body.size())});
  }
  nativeMessages.push_back({0xFFFF, 0, nullptr, 0});
  sNesMessageEntryTablePtr = nativeMessages.data();
  std::vector<std::string> equipmentBodies;
  equipmentBodies.reserve(std::size(equipmentBottleReceipts) * 3);
  std::vector<MessageTableEntry> germanEquipmentMessages,
      frenchEquipmentMessages;
  nativeMessages.pop_back();
  for (const auto &fixture : equipmentBottleReceipts) {
    Rando::StaticData::itemNameToEnum[fixture.name] = fixture.item;
    for (const char *language :
         {"Native complete", "German complete", "French complete"}) {
      equipmentBodies.emplace_back(
          std::string("\x08\x13\x3B") + language + " tutorial for " +
          fixture.name + std::string("\x05\0", 2) +
          ".\x04"
          "Equip and press \xA0 or \xA1 to use it.\x09\x02");
      const auto &body = equipmentBodies.back();
      auto &bank = language == std::string_view("Native complete")
                       ? nativeMessages
                   : language == std::string_view("German complete")
                       ? germanEquipmentMessages
                       : frenchEquipmentMessages;
      bank.push_back({fixture.text, 0x23, body.c_str(),
                      static_cast<uint32_t>(body.size())});
    }
  }
  nativeMessages.push_back({0xFFFF, 0, nullptr, 0});
  sNesMessageEntryTablePtr = nativeMessages.data();
  germanEquipmentMessages.push_back({0xFFFF, 0, nullptr, 0});
  frenchEquipmentMessages.push_back({0xFFFF, 0, nullptr, 0});
  sGerMessageEntryTablePtr = germanEquipmentMessages.data();
  sFraMessageEntryTablePtr = frenchEquipmentMessages.data();
  Rando::StaticData::itemNameToEnum["Master Sword"] = RG_MASTER_SWORD;
  CheckForestCompassReceiptInformation();
  CheckApprovedCompassExample();
  CheckGeneratedCompassRoutes(argc > 1 ? argv[1] : nullptr);
  entranceOverrides[0] = {ENTR_DEKU_TREE_BOSS_ENTRANCE, 1,
                          ENTR_FOREST_TEMPLE_BOSS_ENTRANCE, 1};
  char buffer[1269];
  auto read = [&](const char *name) {
    const int32_t size = OOT_GetItemReceiptText(name, buffer, sizeof(buffer));
    if (size <= 0)
      std::cerr << "Missing donor receipt: " << name << '\n';
    assert(size > 0);
    return std::string(buffer, size);
  };
  const auto playerBeforeTutorials = receiptPlayer;
  const auto foreignBeforeTutorials = foreign;
  const auto resolvedBeforeTutorials = resolvedForeign;
  const auto latchesBeforeTutorials = foreignLatches;
  for (const auto &fixture : equipmentBottleReceipts) {
    receiptPlayer.getItemEntry.getItemId = fixture.item;
    const auto saveBefore = gSaveContext;
    const auto neiBefore = receiptNeiSave;
    CustomMessage native;
    BuildCustomItemMessage(&receiptPlayer, native);
    assert(native.GetEnglish(MF_RAW).find("Native complete tutorial for " +
                                          std::string(fixture.name)) !=
               std::string::npos &&
           "equipment/bottle native receipt must retain the complete tutorial");
    assert(native.GetGerman(MF_RAW).find("German complete tutorial") !=
           std::string::npos);
    assert(native.GetFrench(MF_RAW).find("French complete tutorial") !=
           std::string::npos);
    assert(native.GetEnglish(MF_RAW).find(std::string("\x05\0", 2)) !=
           std::string::npos);
    assert(native.GetTextBoxType() == 2 && native.GetTextBoxPosition() == 3);
    foreign.itemGame = ComboRando::GAME_MM;
    foreign.itemName = fixture.name;
    foreign.displayName = std::string(fixture.name) + " (MM)";
    resolvedForeign = fixture.name;
    CustomMessage imported;
    BuildComboForeignMessage(&receiptPlayer, imported);
    assert(imported.GetEnglish(MF_RAW).find("Native complete tutorial") !=
           std::string::npos);
    assert(imported.GetEnglish(MF_RAW).find("(MM)") != std::string::npos);
    assert(imported.GetEnglish(MF_RAW).find('\x13') == std::string::npos);
    const auto body = read(fixture.name);
    assert(body.find("Native complete tutorial for " +
                     std::string(fixture.name)) != std::string::npos);
    assert(body.find('\xB1') != std::string::npos &&
           body.find('\xB2') != std::string::npos);
    assert(body.find('\x10') != std::string::npos &&
           body.find('\x13') == std::string::npos);
    assert(!std::memcmp(&saveBefore, &gSaveContext, sizeof(gSaveContext)));
    assert(!std::memcmp(&neiBefore, &receiptNeiSave, sizeof(receiptNeiSave)));
  }
  const auto master = read("Master Sword");
  assert(master.find("Equipment") != std::string::npos &&
         master.find('\xB1') != std::string::npos &&
         "Master Sword requires its full equipment/B tutorial");
  for (const char* name : {"Bottle of Milk", "Milk (Half)", "Bottled Hot Spring Water", "Deku Princess",
      "Bottled Seahorse", "Bottled Spring Water", "Zora Egg", "Hylian Loach", "Obaba's Special Drink",
      "Bottle of Chateau Romani", "Bottle With Gold Dust", "Bottle with Magic Mushroom",
      "Red Potion Refill", "Green Potion Refill", "Blue Potion Refill", "Milk Refill", "Fairy",
      "Chateau Romani Refill", "Gold Dust Refill"}) {
    const auto body = read(name);
    assert(body.find('\xB2') != std::string::npos && body.find('\x10') != std::string::npos);
    foreign.itemGame = ComboRando::GAME_MM;
    foreign.itemName = name;
    foreign.displayName = std::string(name) + " (MM)";
    resolvedForeign = name;
    CustomMessage imported;
    BuildComboForeignMessage(&receiptPlayer, imported);
    assert(imported.GetEnglish(MF_RAW).find("You found") == std::string::npos);
    assert(imported.GetEnglish(MF_RAW).find('\xA1') != std::string::npos);
    assert(imported.GetEnglish(MF_RAW).find("(MM)") != std::string::npos);
  }
  // Additional aliases produce foreign latches but never alter grants or saves.
  foreignLatches = latchesBeforeTutorials + static_cast<int>(std::size(equipmentBottleReceipts));
  const auto bankBeforeTutorials = sNesMessageEntryTablePtr;
  sNesMessageEntryTablePtr = nullptr;
  for (const auto &fixture : equipmentBottleReceipts) {
    const auto body = read(fixture.name);
    assert(
        body.find(fixture.item <= RG_MIRROR_SHIELD ? "Equipment" : "bottle") !=
        std::string::npos);
    assert(body.find(fixture.item <= RG_MIRROR_SHIELD ? '\xB0' : '\xB2') !=
           std::string::npos);
    assert(body.size() < 1269 && body.find('%') == std::string::npos);
  }
  sNesMessageEntryTablePtr = bankBeforeTutorials;
  receiptPlayer = playerBeforeTutorials;
  foreign = foreignBeforeTutorials;
  resolvedForeign = resolvedBeforeTutorials;
  assert(foreignLatches == latchesBeforeTutorials + static_cast<int>(std::size(equipmentBottleReceipts)));
  foreignLatches = latchesBeforeTutorials;
  std::cout << "PASS native sword/shield and bottle-content tutorials, donor "
               "codec, pages, buttons and save isolation\n";
  const struct {
    uint16_t text;
    const char *name;
    const char *power;
  } medallions[] = {{0x3F, "Spirit Medallion", "Sand Rod"},
                    {0x3E, "Forest Medallion", "Tornado Rod"},
                    {0x3D, "Water Medallion", "Water Rod"},
                    {0x3C, "Fire Medallion", "Meteor Rod"},
                    {0x40, "Light Medallion", "Storm Rod"},
                    {0x41, "Shadow Medallion", "Shadow Scepter"}};
  for (const auto &medallion : medallions) {
    const auto body = read(medallion.name);
    assert(body.find("Native medallion reward text.") != std::string::npos);
    assert(body.find(medallion.power) != std::string::npos &&
           body.find("awakens") != std::string::npos);
  }
  for (int language : {0, 1, 2}) {
    gSaveContext.language = language;
    for (const auto &medallion : medallions) {
      auto text = medallion.text;
      bool native = true;
      loadedMessage = "sentinel";
      BuildWandMedallionMessage(&text, &native);
      assert(!native && text == medallion.text);
      assert(loadedMessage.find(language == 1 ? "Native German reward." :
                                language == 2 ? "Native French reward." : "Native English reward.") != std::string::npos);
      assert(loadedMessage.find(language == 1 ? "erweckt" : language == 2 ? "pouvoir" : "awakens") != std::string::npos);
      assert(loadedMessage.find('\x13') != std::string::npos && loadedMessage.back() == '\x02');
      assert(std::count(loadedMessage.begin(), loadedMessage.end(), '\x02') == 1);
    }
  }
  gSaveContext.language = 0;
  // Native prefix bytes are already encoded. COLOR's 0x40 is not an '@'
  // markup token, SHIFT's 0x0B and SFX's 0x02 are arguments, and the original
  // ending command must execute after the added tutorial.
  nativePrefixControls = std::string("\x05\x40\x06\x0B\x12\x00\x02", 7);
  for (const auto& ending : {std::string("\x02"), std::string("\x0B\x02", 2),
                             std::string("\x07\x00\x26\x02", 4), std::string("\x0E\x26\x02", 3),
                             std::string("\x11\x00\x26\x02", 4)}) {
    nativeEnding = ending;
    auto text = medallions[0].text;
    bool native = true;
    BuildWandMedallionMessage(&text, &native);
    assert(!native);
    assert(loadedMessage.starts_with(nativePrefixControls) && "native prefix control arguments were reformatted");
    assert(loadedMessage.ends_with(ending) && "native ending must follow the added lesson");
    assert(loadedMessage.find("awakens") < loadedMessage.size()-ending.size());
  }
  nativePrefixControls.clear();
  nativeEnding = "\x07";
  auto malformedText = medallions[0].text;
  bool malformedNative = true;
  loadedMessage = "sentinel";
  BuildWandMedallionMessage(&malformedText, &malformedNative);
  assert(malformedNative && loadedMessage == "sentinel" && "truncated native control must retain the table path");
  nativeEnding = "\x02";
  for (uint8_t rule : {1, 2}) {
    wandRule = rule;
    for (const auto& medallion : medallions) {
      assert(read(medallion.name).find("awakens") == std::string::npos);
      auto text = medallion.text;
      bool native = true;
      loadedMessage = "sentinel";
      BuildWandMedallionMessage(&text, &native);
      assert(native && loadedMessage == "sentinel");
    }
  }
  wandRule = 0;
  for (bool customItems : {false, true}) {
    receiptContext.customItems = customItems;
    randoActive = !customItems;
    auto text = medallions[0].text;
    bool native = true;
    loadedMessage = "sentinel";
    BuildWandMedallionMessage(&text, &native);
    assert(native && loadedMessage == "sentinel");
  }
  receiptContext.customItems = 1;
  randoActive = true;
  const struct { const char* name; const char* effect; char color; char extraButton; } tools[] = {
      {"Phantom Hourglass", "rewind", '\x04', '\xB4'},
      {"Shadow Crystal", "Wolf Link", '\x06', '\xB0'}};
  for (const auto& tool : tools) {
    const auto body = read(tool.name);
    assert(body.find(tool.effect) != std::string::npos && body.find(tool.color) != std::string::npos);
    Player player{};
    player.getItemId = Rando::StaticData::itemNameToEnum.at(tool.name);
    player.getItemEntry.objectId = OBJECT_INVALID;
    CustomMessage native;
    BuildCustomItemMessage(&player, native);
    for (const auto& text : {native.GetEnglish(MF_RAW), native.GetGerman(MF_RAW), native.GetFrench(MF_RAW)}) {
      assert(text.starts_with(CustomMessage::ITEM_OBTAINED(ITEM_CUSTOM)) &&
             "extended item ID truncated instead of using its custom icon");
      std::string converted;
      assert(ComboItemReceiptText::FromOotMessage(text, converted));
      assert(converted.find('\xB2') != std::string::npos && converted.find('\xB1') != std::string::npos);
      assert(converted.find(tool.extraButton) != std::string::npos && converted.find(tool.color) != std::string::npos);
      assert(converted.find('%') == std::string::npos && converted.size() <= sizeof(buffer));
    }
  }
  std::cout << "PASS Hourglass/Crystal OoT tutorials: custom icons, all locales, encoded glyphs/colors and donor export\n";
  const struct { const char* name; const char* detail; int icon; } masks[] = {
      {"Skull Mask", "monster", ITEM_MASK_SKULL},
      {"Spooky Mask", "scare", ITEM_MASK_SPOOKY},
      {"Mask of Truth", "Show it", ITEM_MASK_TRUTH},
      {"Mask of Truth (MM)", "Show it", ITEM_CUSTOM},
      {"Gerudo Mask", "Urbosa", ITEM_MASK_GERUDO},
      {"Keaton Mask", "reflect", ITEM_MASK_KEATON},
      {"Keaton Mask (MM)", "reflect", ITEM_CUSTOM}};
  for (const auto& mask : masks) {
    const auto body = read(mask.name);
    assert(body.find(mask.detail) != std::string::npos);
    Player player{};
    player.getItemId = Rando::StaticData::itemNameToEnum.at(mask.name);
    player.getItemEntry.objectId = OBJECT_INVALID;
    CustomMessage native;
    BuildCustomItemMessage(&player, native);
    for (const auto& text : {native.GetEnglish(MF_RAW), native.GetGerman(MF_RAW), native.GetFrench(MF_RAW)}) {
      assert(text.starts_with(CustomMessage::ITEM_OBTAINED(mask.icon)));
      std::string converted;
      assert(ComboItemReceiptText::FromOotMessage(text, converted));
      assert(converted.find('\xB2') != std::string::npos && converted.find('%') == std::string::npos);
      assert(converted.size() <= sizeof(buffer));
      if (std::string_view(mask.name).starts_with("Keaton Mask")) {
        for (char glyph : {'\xB0', '\xB1', '\xB4'})
          assert(converted.find(glyph) != std::string::npos);
      }
    }
    if (mask.icon == ITEM_MASK_GERUDO || std::string_view(mask.name).starts_with("Keaton Mask")) {
      for (const auto glyph : {'\xB0', '\xB1', '\xB4'})
        assert(body.find(glyph) != std::string::npos);
    }
  }
  std::cout << "PASS OoT mask descriptions and Gerudo/Keaton tutorials: native icons, locales and donor export\n";
  const struct { const char* name; const char* effect; } magicItems[] = {
      {"Elemental Wand", "medallion"}, {"Sand Rod", "platform"},
      {"Tornado Rod", "jump"}, {"Water Rod", "water"},
      {"Meteor Rod", "explosive"}, {"Storm Rod", "lightning"},
      {"Shadow Scepter", "stun"}, {"Sheikah Slate", "rune"},
      {"Rune: Remote Bomb", "detonate"}, {"Rune: Stasis", "Freeze"},
      {"Rune: Cryonis", "pillar"}, {"Rune: Master Cycle", "motorcycle"},
      {"Rune: Sheikah Sensor", "Heart Container"}};
  for (const auto& magic : magicItems) {
    const auto body = read(magic.name);
    assert(body.find(magic.effect) != std::string::npos);
    assert(body.find('\xB2') != std::string::npos && body.find('\xB3') != std::string::npos);
    Player player{};
    player.getItemId = Rando::StaticData::itemNameToEnum.at(magic.name);
    player.getItemEntry.objectId = OBJECT_INVALID;
    CustomMessage native;
    BuildCustomItemMessage(&player, native);
    std::string converted;
    assert(ComboItemReceiptText::FromOotMessage(native.GetEnglish(MF_RAW), converted));
    assert(converted.find(magic.effect) != std::string::npos);
  }
  for (uint8_t rule : {0, 1, 2}) {
    wandRule = rule;
    const auto body = read("Elemental Wand");
    assert(body.find(rule == 0 ? "medallion" : rule == 1 ? "All six" : "separately") != std::string::npos);
    if (rule != 2) {
      assert(body.size() <= 1269 && "shared wand export exceeds MM's body capacity");
      for (const char* effect : {"platform", "wind", "water", "explosive", "lightning", "stun"})
        assert(body.find(effect) != std::string::npos && "shared wand export must teach every power");
      Player player{};
      player.getItemId = RG_ELEMENTAL_WAND;
      player.getItemEntry.objectId = OBJECT_INVALID;
      CustomMessage native;
      BuildCustomItemMessage(&player, native);
      std::string converted;
      assert(ComboItemReceiptText::FromOotMessage(native.GetEnglish(MF_RAW), converted));
      assert(converted.size() <= 1269);
      for (const char* effect : {"platform", "wind", "water", "explosive", "lightning", "stun"})
        assert(converted.find(effect) != std::string::npos && "shared OoT wand pickup must teach every power");
    }
  }
  wandRule = 0;
  const auto cane = read("Cane of Somaria"), roc = read("Progressive Roc"),
             agony = read("Stone of Agony");
  assert(cane.find("Statue") != std::string::npos &&
         roc.find("high jump") != std::string::npos);
  stateAdvanced =
      true; // grant happened; recollection still refers to the frozen identity
  assert(read("Cane of Somaria") == cane &&
         "repeat receipt advanced to the next Cane skill");
  assert(read("Progressive Roc") == roc &&
         "repeat receipt advanced to Roc's Cape");
  assert(read("Stone of Agony") == agony &&
         "repeat receipt advanced to Quartz");
  assert(liveResolutionCalls == 0);
  assert(read("Magic Meter").find("Use magic") != std::string::npos);
  assert(read("Enhanced Magic Meter").find("twice") != std::string::npos);
  const auto leaf = read("Deku Leaf");
  assert(leaf.find("glide") != std::string::npos &&
         leaf.find('\0') != std::string::npos);
  assert(read("Power Upgrade").find("double damage") != std::string::npos);
  assert(read("Magic Stat Upgrade").find("reach max stat") !=
         std::string::npos);
  assert(read("Goht's Remains").find("\x01Goht") != std::string::npos);
  assert(read("Goron Lullaby").find("\x01Goron") != std::string::npos);
  assert(read("New Wave Bossa Nova").find("\x03New Wave") != std::string::npos);
  assert(read("Song of Healing").find("\x06Song") != std::string::npos);
  assert(read("Song of Storms (MM)").find("rain") != std::string::npos);
  for (const auto &fixture : keyCatalog)
    Rando::StaticData::itemNameToEnum.emplace(fixture.name, fixture.item);
  // Exercise the real OoT export with the exact native catalog identities.
  // The encoded receipt must retain its owner's name and palette in MM.
  for (const auto &fixture : KeyReceiptFixtures::entries) {
    const auto body = read(fixture.name);
    if (KeyReceiptFixtures::Flatten(body) !=
        KeyReceiptFixtures::Expected(fixture)) {
      std::cerr << "Incorrect exported key name/color: " << fixture.name
                << '\n';
      for (unsigned char byte : body)
        std::cerr << unsigned(byte) << ' ';
      std::cerr << '\n';
    }
    assert(KeyReceiptFixtures::Flatten(body) ==
           KeyReceiptFixtures::Expected(fixture));
  }
  for (const auto &fixture : KeyReceiptFixtures::entries) {
    receiptPlayer.getItemEntry.getItemId =
        Rando::StaticData::itemNameToEnum.at(fixture.name);
    CustomMessage native;
    BuildCustomItemMessage(&receiptPlayer, native);
    std::string body;
    assert(
        ComboItemReceiptText::FromOotMessage(native.GetEnglish(MF_RAW), body));
    assert(KeyReceiptFixtures::Flatten(body) ==
           KeyReceiptFixtures::Expected(fixture));
  }
  receiptPlayer.getItemEntry.getItemId = RG_NONE;
  // Native icon resolution above retains the established path; donor export
  // itself must never resolve a progressive item against the live inventory.
  liveResolutionCalls = 0;
  for (const auto *excluded : {"Skeleton Key", "Room Key", "Woodfall Key Ring",
                               "Ice Cavern Small Key"}) {
    assert(ComboDungeonKeyReceipt::Markup(excluded).empty());
  }
  assert(read("Minuet of Forest").find("take you to the forest") != std::string::npos);
  receiptContext.information = 0;
  for (const auto& fixture : traditionalReceipts) {
    if ((fixture.item >= RG_FOREST_TEMPLE_SMALL_KEY &&
         fixture.item <= RG_TREASURE_GAME_SMALL_KEY) ||
        (fixture.item >= RG_FOREST_TEMPLE_BOSS_KEY &&
         fixture.item <= RG_GANONS_CASTLE_BOSS_KEY))
      continue; // Keys now have the requested dungeon-named randomizer receipt.
    const auto found = std::find_if(nativeMessages.begin(), nativeMessages.end(), [&](const auto& message) {
      return message.textId == fixture.expectedText;
    });
    assert(found != nativeMessages.end());
    std::string expected;
    assert(ComboItemReceiptText::FromOotMessage(std::string_view(found->segment, found->msgSize), expected));
    const auto receipt = read(fixture.name);
    if (receipt.compare(0, expected.size(), expected) != 0)
      std::cerr << "Replaced traditional receipt: " << fixture.name << '\n';
    assert(receipt.compare(0, expected.size(), expected) == 0);
  }
  assert(liveResolutionCalls == 0);
  for (int enabled : {0, 1}) {
    receiptContext.information = enabled;
    assert((read("Great Deku Tree Compass").find("see hidden things") != std::string::npos) == !enabled);
    assert((read("Great Deku Tree Map").find("Blue rooms") != std::string::npos) == !enabled);
  }
  const std::array<std::pair<const char *, const char *>, 4> mmMaps{
      {{"Woodfall Map", "Woodfall"},
       {"Snowhead Map", "Snowhead"},
       {"Great Bay Map", "Zora Cape's turtle"},
       {"Stone Tower Map", "Stone Tower"}}};
  for (int enabled : {0, 1}) {
    receiptContext.information = enabled;
    for (const auto &[map, entrance] : mmMaps) {
      const auto text = read(map);
      assert((text.find("It seems the entrance is at") != std::string::npos) ==
             bool(enabled));
      if (enabled)
        assert(text.find(entrance) != std::string::npos);
    }
  }
  const auto shuffledCompass = read("Great Deku Tree Compass");
  assert(shuffledCompass.find("Phantom Ganon") != std::string::npos);
  assert(shuffledCompass.find("ordinary") == std::string::npos);
  receiptContext.placements[RC_QUEEN_GOHMA] = {RG_KOKIRI_EMERALD,
                                               "Kokiri Emerald"};
  receiptContext.placements[RC_PHANTOM_GANON] = {RG_DEKU_LEAF, "Deku Leaf"};
  auto rewardCompass = read("Great Deku Tree Compass");
  CwItemReceiptPresentation rewardPresentation{};
  assert(OOT_GetDungeonItemReceiptPresentation("Great Deku Tree Compass", &rewardPresentation) == 1);
  assert(std::strstr(rewardPresentation.iconPath, "DekuLeaf") &&
         rewardCompass.find("Kokiri Emerald") == std::string::npos);
  receiptContext.placements[RC_PHANTOM_GANON] = {RG_COMBO_FOREIGN,
                                                 "Combo Foreign Item"};
  rewardForeign.itemName = "Progressive Goron Lullaby";
  rewardForeign.displayName = "Progressive Goron Lullaby (MM)";
  assert(read("Great Deku Tree Compass").find("Progressive Goron Lullaby (MM)") == std::string::npos);
  assert(OOT_GetDungeonItemReceiptPresentation("Great Deku Tree Compass", &rewardPresentation) == 1);
  assert(std::string(rewardPresentation.iconPath) == "__OTR__@mm:icon_item_static_yar/gItemIconSongNoteTex");
  assert(liveResolutionCalls == 0 && foreignLatches == 0);
  receiptContext.dungeonShuffle = 1;
  entranceOverrides[1] = {ENTR_DODONGOS_CAVERN_ENTRANCE, 1,
                          ENTR_DEKU_TREE_ENTRANCE, 1};
  entranceOverrides[2] = {ENTR_DEKU_TREE_ENTRANCE, 1,
                          ENTR_WATER_TEMPLE_ENTRANCE, 1};
  EntranceTracker::data[ENTR_DODONGOS_CAVERN_ENTRANCE] = {
      "Dodongo's Cavern"};
  EntranceTracker::data[ENTR_DEKU_TREE_ENTRANCE] = {"Kokiri Forest"};
  assert(read("Great Deku Tree Map").find("Dodongo's Cavern") !=
         std::string::npos);
  assert(read("Great Deku Tree Map").find("Kokiri Forest") ==
         std::string::npos);
  entranceOverrides[2].override = ENTR_FOREST_TEMPLE_ENTRANCE;
  const auto forestMap = read("Forest Temple Map");
  assert(forestMap.find("Deku Tree") != std::string::npos && forestMap.find("Kokiri Forest") == std::string::npos);
  assert(forestMap.find('\x10') == std::string::npos && forestMap.find("ordinary") != std::string::npos);
  assert(forestMap.find("It's \x02ordinary\x00.") != std::string::npos);
  assert(forestMap.find("It seems the entrance is at \x05" "Deku Tree\x00.") != std::string::npos);
  entranceOverrides[2].override = ENTR_WATER_TEMPLE_ENTRANCE;
  // Ownership alone makes the pause view available, as for Start With. The
  // acquisition latch and item grant history have never been set in this save.
  assert(Randomizer_GetDungeonItemInfoTextId(ITEM_COMPASS) == 0);
  assert(Randomizer_GetDungeonItemInfoTextId(ITEM_DUNGEON_MAP) == 0);
  gSaveContext.inventory.dungeonItems[0] =
      (1 << DUNGEON_MAP) | (1 << DUNGEON_COMPASS);
  const auto saveBeforeInfo = gSaveContext;
  for (const auto &[item, id] : std::array<std::pair<int, uint16_t>, 2>{
           {{ITEM_COMPASS, TEXT_DESC_DUNGEON_COMPASS_INFO},
            {ITEM_DUNGEON_MAP, TEXT_DESC_DUNGEON_MAP_INFO}}}) {
    assert(Randomizer_GetDungeonItemInfoTextId(item) == id);
    uint16_t text = id;
    bool fromTable = true;
    BuildDungeonPauseInfoMessage(&text, &fromTable);
    assert(!fromTable && loadedMessage.find("You found") == std::string::npos);
    assert(loadedMessage.find(item == ITEM_COMPASS
                                  ? "Progressive Goron Lullaby (MM)"
                                  : "Dodongo's Cavern") !=
           std::string::npos);
  }
  assert(std::memcmp(&saveBeforeInfo, &gSaveContext, sizeof(gSaveContext)) ==
         0);
  play.sceneNum = SCENE_HYRULE_FIELD;
  assert(Randomizer_GetDungeonItemInfoTextId(ITEM_COMPASS) ==
         0); // overworld map has no dungeon selection
  play.sceneNum = SCENE_DEKU_TREE;
  // A dormant OoT donor still has a generated/restored seed in MM-first runs.
  randoActive = false;
  gPlayState = nullptr;
  receiptContext.dungeon.mq = true;
  masterQuest = true; // runtime helper deliberately ignores this while dormant
  assert(OOT_MapCompassInfoEnabled() == 1);
  assert(read("Great Deku Tree Map").find("masterful") != std::string::npos);
  assert(read("Great Deku Tree Compass").find("masterful") ==
         std::string::npos);
  assert(read("Great Deku Tree Compass").find("Progressive Goron Lullaby (MM)") == std::string::npos);
  assert(OOT_GetDungeonItemReceiptPresentation("Great Deku Tree Compass", &rewardPresentation) == 1);
  assert(rewardPresentation.rewardLine == 1 && std::strstr(rewardPresentation.iconPath, "@mm:"));
  // Receipt layout stays on two lines even if this plando has no reward sprite.
  seedIconAvailable = false;
  const auto missingSprite = read("Great Deku Tree Compass");
  assert(missingSprite.find("It points to") != std::string::npos);
  assert(missingSprite.find("Progressive Goron Lullaby (MM)") == std::string::npos);
  assert(missingSprite.find("Defeating the boss grants") == std::string::npos);
  assert(OOT_GetDungeonItemReceiptPresentation("Great Deku Tree Compass", &rewardPresentation) == 1);
  assert(!ComboReceipt_HasIcon(&rewardPresentation) && rewardPresentation.rewardLine == 1);
  seedIconAvailable = true;
  assert(read("Great Deku Tree Map").find("Dodongo's Cavern") !=
         std::string::npos);
  receiptContext.generated = false;
  assert(OOT_MapCompassInfoEnabled() == 0);
  receiptContext.spoiler = true;
  assert(OOT_MapCompassInfoEnabled() == 1);
  receiptContext.information = 0;
  assert(OOT_MapCompassInfoEnabled() == 0);
  receiptContext.generated = true;
  receiptContext.spoiler = false;
  randoActive = true;
  masterQuest = false;
  gPlayState = &play;
  receiptContext.information = 0;
  assert(Randomizer_GetDungeonItemInfoTextId(ITEM_COMPASS) == 0);
  assert(read("Great Deku Tree Compass").find("Phantom Ganon") ==
         std::string::npos);
  assert(read("Great Deku Tree Compass").find("Lullaby") == std::string::npos);
  assert(read("Great Deku Tree Map").find("Dodongo's Cavern") ==
         std::string::npos);
  assert(read("Great Deku Tree Map").find("ordinary") != std::string::npos);
  receiptContext.information = 1;
  // Unshuffled/native pickup entries still identify their current dungeon.
  receiptPlayer.getItemEntry.modIndex = 0;
  receiptPlayer.getItemEntry.itemId = ITEM_DUNGEON_MAP;
  uint16_t nativeText = 0x66;
  bool nativeTable = true;
  loadedMessage.clear();
  BuildMapMessage(&nativeText, &nativeTable);
  assert(!nativeTable &&
         loadedMessage.find("Dodongo's Cavern") != std::string::npos);
  receiptPlayer.getItemEntry.itemId = ITEM_COMPASS;
  nativeTable = true;
  BuildMapMessage(&nativeText, &nativeTable);
  assert(!nativeTable && loadedMessage.find("Progressive Goron Lullaby (MM)") == std::string::npos);
  assert(loadedMessage.find("Phantom Ganon") != std::string::npos && loadedPresentation.rewardLine == 1);
  assert(std::string(loadedPresentation.iconPath) == "__OTR__@mm:icon_item_static_yar/gItemIconSongNoteTex");
  receiptPlayer.getItemEntry.modIndex = MOD_RANDOMIZER;
  masterQuest = true;
  assert(read("Great Deku Tree Compass").find("masterful") ==
         std::string::npos);
  assert(read("Great Deku Tree Map").find("masterful") != std::string::npos);
  assert(read("Snowhead Compass").find("Goht") != std::string::npos);
  mmRewardAvailable = true;
  assert(read("Snowhead Compass").find("Progressive Hookshot (OOT)") ==
         std::string::npos);
  assert(read("Snowhead Compass").find("Defeating the boss grants") == std::string::npos);
  assert(read("Ice Cavern Compass").find("Phantom Ganon") == std::string::npos);
  assert(read("Ice Cavern Compass").find("reward") == std::string::npos);
  receiptContext.mqMode = RO_MQ_DUNGEONS_NONE;
  assert(read("Bottom of the Well Compass").find("reward") ==
         std::string::npos);
  receiptContext.mqMode = RO_MQ_DUNGEONS_NONE;
  assert(read("Great Deku Tree Compass").find("masterful") ==
         std::string::npos);
  receiptContext.bossShuffle = RO_BOSS_ROOM_ENTRANCE_SHUFFLE_OFF;
  assert(read("Great Deku Tree Compass").find("Gohma") != std::string::npos);
  receiptContext.bossShuffle = 1;
  entranceOverrides[0].override =
      ENTR_DEKU_TREE_ENTRANCE; // mixed pool: this door no longer reaches a boss
  assert(read("Great Deku Tree Compass").find("Gohma") == std::string::npos);
  assert(read("Great Deku Tree Compass").find("reward") == std::string::npos);
  gSaveContext.inventory.gsTokens = 42;
  assert(read("Gold Skulltula Token").find("43") != std::string::npos);
  receiptNeiSave.comboObtained[FC_MM_SKULLS_SWAMP] = 7;
  receiptNeiSave.comboObtained[FC_MM_SKULLS_OCEAN] = 19;
  assert(read("Swamp Gold Skulltula Token").find("8") != std::string::npos);
  assert(read("Ocean Gold Skulltula Token").find("20") != std::string::npos);
  assert(gSaveContext.inventory.gsTokens == 42);
  assert(receiptNeiSave.comboObtained[FC_MM_SKULLS_SWAMP] == 7);
  // Native MM-port receipts, donor export, and foreign-MM pickups in OoT
  // all include this one pending fairy without mutating either count store.
  const RandomizerGet fairyIds[] = {
      RG_MM_STRAY_FAIRY_WOODFALL, RG_MM_STRAY_FAIRY_SNOWHEAD,
      RG_MM_STRAY_FAIRY_GREAT_BAY, RG_MM_STRAY_FAIRY_STONE_TOWER};
  const char *fairyNames[] = {"Woodfall Stray Fairy", "Snowhead Stray Fairy",
                              "Great Bay Stray Fairy",
                              "Stone Tower Stray Fairy"};
  const char *fairyTemples[] = {"Woodfall Temple", "Snowhead Temple",
                                "Great Bay Temple", "Stone Tower Temple"};
  const char fairyColors[] = {
      0x06, 0x02, 0x03, 0x04}; // converted native pink, green, blue, yellow
  const int fairyFcIds[] = {
      FCI_MM_STRAY_FAIRY_WOODFALL, FCI_MM_STRAY_FAIRY_SNOWHEAD,
      FCI_MM_STRAY_FAIRY_GREAT_BAY, FCI_MM_STRAY_FAIRY_STONE_TOWER};
  const auto foreignBeforeFairies = foreign;
  const auto resolvedBeforeFairies = resolvedForeign;
  const auto countsBeforeFairies = receiptNeiSave;
  const auto playerBeforeFairies = receiptPlayer;
  const auto flattenFairyReceipt = [](std::string text) {
    std::replace(text.begin(), text.end(), '\x11', ' ');
    std::replace(text.begin(), text.end(), '\x10', ' ');
    return text;
  };
  for (int d = 0; d < 4; ++d) {
    for (auto counts : {std::pair{0, 0}, std::pair{7, 3}, std::pair{3, 7},
                        std::pair{14, 14}}) {
      receiptNeiSave = {};
      receiptNeiSave.comboObtained[FC_MM_FAIRIES_WOODFALL + d] = counts.first;
      receiptNeiSave.comboObtainedFc[fairyFcIds[d]] = counts.second;
      const auto before = receiptNeiSave;
      const auto expected =
          std::to_string(std::max(counts.first, counts.second) + 1);
      const auto body = flattenFairyReceipt(read(fairyNames[d]));
      assert(body.find(fairyTemples[d]) != std::string::npos &&
             "OoT fairy receipt omitted its dungeon");
      assert(body.find(fairyNames[d]) != std::string::npos);
      assert(body.find(expected) != std::string::npos);
      assert(std::count(body.begin(), body.end(), fairyColors[d]) == 3 &&
             "OoT fairy has wrong dungeon color codes");
      receiptPlayer.getItemEntry.getItemId = fairyIds[d];
      CustomMessage native;
      BuildCustomItemMessage(&receiptPlayer, native);
      std::string nativeBody;
      assert(ComboItemReceiptText::FromOotMessage(native.GetEnglish(MF_RAW),
                                                  nativeBody));
      nativeBody = flattenFairyReceipt(std::move(nativeBody));
      assert(nativeBody.find(expected) != std::string::npos &&
             nativeBody.find(fairyTemples[d]) != std::string::npos);
      assert(std::count(nativeBody.begin(), nativeBody.end(), fairyColors[d]) ==
             3);
      for (const auto &localized :
           {native.GetGerman(MF_RAW), native.GetFrench(MF_RAW)}) {
        std::string translated;
        assert(ComboItemReceiptText::FromOotMessage(localized, translated));
        assert(translated.find(expected) != std::string::npos &&
               translated.find("[[") == std::string::npos);
        assert(std::count(translated.begin(), translated.end(),
                          fairyColors[d]) == 3);
      }
      foreign.itemName = fairyNames[d];
      foreign.displayName = std::string(fairyNames[d]) + " (MM)";
      resolvedForeign = fairyNames[d];
      CustomMessage imported;
      BuildComboForeignMessage(&receiptPlayer, imported);
      std::string importedBody;
      assert(ComboItemReceiptText::FromOotMessage(imported.GetEnglish(MF_RAW),
                                                  importedBody));
      importedBody = flattenFairyReceipt(std::move(importedBody));
      assert(importedBody.find(expected) != std::string::npos &&
             importedBody.find(fairyTemples[d]) != std::string::npos);
      assert(std::count(importedBody.begin(), importedBody.end(),
                        fairyColors[d]) == 3);
      assert(imported.english.find(CustomMessage::ITEM_OBTAINED(ITEM_CUSTOM)) ==
             std::string::npos);
      assert(!std::memcmp(&before, &receiptNeiSave, sizeof(before)));
    }
  }
  CustomMessage clockTown("Clock Town pickup");
  assert(!BuildTokenReceiptMessage(RG_MM_STRAY_FAIRY, clockTown));
  assert(clockTown.english == "Clock Town pickup");
  // Exercise the actual grant recorder between receipts. Advancing only the
  // legacy mirror keeps the FC deficit equal to the unmaterialized copies.
  for (int d = 0; d < 4; ++d) {
    const int fc = fairyFcIds[d];
    const int legacy = FC_MM_FAIRIES_WOODFALL + d;
    for (auto counts : {std::pair{7, 0}, std::pair{7, 3}, std::pair{3, 7},
                        std::pair{14, 14}}) {
      receiptNeiSave = {};
      receiptNeiSave.comboObtained[legacy] = counts.first;
      receiptNeiSave.comboObtainedFc[fc] = counts.second;
      receiptNeiSave.comboAppliedFc[fc] = counts.second;
      const int start = std::max(counts.first, counts.second);
      for (int copy = 1; copy <= 2; ++copy) {
        const auto before = receiptNeiSave;
        const auto body = flattenFairyReceipt(read(fairyNames[d]));
        assert(body.find(std::to_string(start + copy)) != std::string::npos);
        assert(!std::memcmp(&before, &receiptNeiSave, sizeof(before)));
        RecordReceiptGrant(fairyIds[d]);
        assert(receiptNeiSave.comboObtained[legacy] == start + copy &&
               "successive fairy grants must advance the receipt baseline");
        assert(receiptNeiSave.comboObtainedFc[fc] == counts.second + copy);
        assert(receiptNeiSave.comboAppliedFc[fc] == counts.second + copy);
        // Copying this FC count into MM with its old applied count owes only
        // the actual new copies, even when MM's legacy baseline is seven.
        assert(receiptNeiSave.comboObtainedFc[fc] - counts.second == copy);
        for (int other = 0; other < 4; ++other) {
          if (other != d)
            assert(
                receiptNeiSave.comboObtained[FC_MM_FAIRIES_WOODFALL + other] ==
                0);
        }
      }
    }
    receiptNeiSave = {};
    receiptNeiSave.comboObtained[legacy] = 7;
    receiptNeiSave.comboObtainedFc[fc] = 7;
    receiptNeiSave.comboAppliedFc[fc] = 3;
    RecordReceiptGrant(fairyIds[d]);
    assert(receiptNeiSave.comboObtainedFc[fc] -
               receiptNeiSave.comboAppliedFc[fc] ==
           4);
    const auto beforeSuppression = receiptNeiSave;
    const auto callsBeforeSuppression = receiptSharedCalls;
    receiptApplyingFc = true;
    RecordReceiptGrant(fairyIds[d]);
    receiptApplyingFc = false;
    assert(!std::memcmp(&beforeSuppression, &receiptNeiSave,
                        sizeof(receiptNeiSave)));
    assert(receiptSharedCalls == callsBeforeSuppression);
  }
  receiptNeiSave = {};
  receiptNeiSave.comboObtained[FC_MM_SKULLS_SWAMP] = 7;
  RecordReceiptGrant(RG_MM_GS_TOKEN_SWAMP);
  assert(receiptNeiSave.comboObtained[FC_MM_SKULLS_SWAMP] == 7);
  assert(receiptNeiSave.comboObtainedFc[FCI_MM_GS_TOKEN_SWAMP] == 1);
  assert(receiptNeiSave.comboAppliedFc[FCI_MM_GS_TOKEN_SWAMP] == 1);
  RecordReceiptGrant(RG_MM_STRAY_FAIRY);
  for (int d = 0; d < 4; ++d)
    assert(receiptNeiSave.comboObtained[FC_MM_FAIRIES_WOODFALL + d] == 0);
  foreign = foreignBeforeFairies;
  resolvedForeign = resolvedBeforeFairies;
  receiptNeiSave = countsBeforeFairies;
  receiptPlayer = playerBeforeFairies;
  std::cout << "OoT stray fairy receipts: four native colors, pending/mirrored "
               "counts, three locales, native/export/foreign paths and save "
               "isolation passed\n";
  std::cout
      << "OoT fairy grant recorder: consecutive legacy/FC totals, preserved "
         "deficits, suppression and unrelated token/Clock Town grants passed\n";
  CustomMessage foreignMessage;
  BuildComboForeignMessage(&receiptPlayer, foreignMessage);
  std::string foreignBody;
  assert(ComboItemReceiptText::FromOotMessage(foreignMessage.GetEnglish(MF_RAW),
                                              foreignBody));
  assert(foreignBody.find("\x01Goht") != std::string::npos &&
         foreignBody.find("mechanical") != std::string::npos);
  foreign.itemName = "Progressive Goron Lullaby";
  resolvedForeign = "Goron Lullaby Intro";
  BuildComboForeignMessage(&receiptPlayer, foreignMessage);
  assert(foreignMessage.english.find("opening bars") != std::string::npos);
  const int latchesBeforeTrap = foreignLatches;
  foreign.trap = true;
  BuildComboForeignMessage(&receiptPlayer, foreignMessage);
  assert(foreignMessage.english == "Trap receipt" &&
         foreignLatches == latchesBeforeTrap);
  foreign.trap = false;
  foreign.itemName = "Unknown MM Item";
  foreign.displayName = "Unknown MM Item (MM)";
  resolvedForeign = nullptr;
  BuildComboForeignMessage(&receiptPlayer, foreignMessage);
  assert(foreignMessage.english.find("Unknown MM Item") != std::string::npos);
  const auto beforeKeys = foreign;
  const auto beforeResolvedKey = resolvedForeign;
  for (const auto &fixture : KeyReceiptFixtures::entries) {
    if (!fixture.mm)
      continue;
    foreign.itemName = fixture.name;
    foreign.displayName = std::string(fixture.name) + " (MM)";
    resolvedForeign = fixture.name;
    BuildComboForeignMessage(&receiptPlayer, foreignMessage);
    std::string body;
    assert(ComboItemReceiptText::FromOotMessage(
        foreignMessage.GetEnglish(MF_RAW), body));
    assert(KeyReceiptFixtures::Flatten(body) ==
           KeyReceiptFixtures::Expected(fixture));
    assert(foreignMessage.english.find(char(0x13)) == std::string::npos);
  }
  foreign = beforeKeys;
  resolvedForeign = beforeResolvedKey;
  std::cout << "All 34 key names/colors passed in OoT native/export receipts; "
               "all 8 MM keys passed the foreign receipt path\n";
  // The actual receive route for MM Maps found in OoT uses this same info,
  // independently of the donor export and the MM-native catalog fixture.
  for (int enabled : {0, 1}) {
    receiptContext.information = enabled;
    for (const auto &[map, entrance] : mmMaps) {
      foreign.itemName = map;
      foreign.displayName = std::string(map) + " (MM)";
      BuildComboForeignMessage(&receiptPlayer, foreignMessage);
      assert((foreignMessage.english.find("It seems the entrance is at") !=
              std::string::npos) == bool(enabled));
      if (enabled)
        assert(foreignMessage.english.find(entrance) != std::string::npos);
    }
  }
  assert(OOT_GetItemReceiptText("Deku Leaf", buffer, 5) == 0);
  assert(OOT_GetItemReceiptText("unknown", buffer, sizeof(buffer)) == 0);
  assert(OOT_GetItemReceiptText("Ice Trap", buffer, sizeof(buffer)) == 0);
  assert(read("Green Rupee").find("one Rupee") != std::string::npos);
  assert(read("Song of Soaring").find("statue") != std::string::npos);
  assert(OOT_GetItemReceiptText(nullptr, buffer, sizeof(buffer)) == 0);
  assert(OOT_GetItemReceiptText("Deku Leaf", nullptr, sizeof(buffer)) == 0);
  globals.gRandoContext = nullptr;
  assert(OOT_MapCompassInfoEnabled() == 0);
  assert(OOT_GetItemReceiptText("Deku Leaf", buffer, sizeof(buffer)) == 0);
  globals.gRandoContext = &receiptContext;
  assert(read("Deku Leaf") == leaf);
  throwOnRead = true;
  assert(OOT_GetItemReceiptText("Magic Meter", buffer, sizeof(buffer)) == 0);
  std::cout << "actual OoT export: full text, fixed tiers, binary lengths, "
               "guards and exceptions passed\n";
  return 0;
}
