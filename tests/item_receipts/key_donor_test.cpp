// The entire donor export is copied unchanged by the runner. Only the engine
// catalog/message/font services are seams, with key rows read from item_list.cpp.
#include "ComboItemReceiptText.h"
#include "ComboDungeonKeyReceipt.h"
#include "ComboMagicItemReceiptText.h"
#if __has_include("ComboVanillaItemReceiptText.h")
#include "ComboVanillaItemReceiptText.h"
#endif
#if __has_include("ComboKeyReceiptText.h")
#include "ComboKeyReceiptText.h"
#endif
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <vector>
#define COMBO_BUILD
#define COMBO_EXPORT __attribute__((visibility("default")))
#define RANDO_ENUM_BEGIN(x) enum x {
#define RANDO_ENUM_ITEM(x, ...) x,
#define RANDO_ENUM_END(x) };
#include "soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerGet.h"
constexpr int ITEM_CATEGORY_JUNK = 0, ITEM_CATEGORY_MAJOR = 1, MF_RAW = 0;
constexpr int ITEM_KEY_SMALL = 0xAF, ITEM_KEY_BOSS = 0x95, TEXTBOX_TYPE_BLUE = 2;
constexpr int TEXT_RANDOMIZER_CUSTOM_ITEM = 0x9000;
using TextBoxType = int;
using TextBoxPosition = int;
using ItemID = int;
constexpr int ITEM_NONE = 0xFF, ITEM_ROCS_FEATHER_SKIJER = 0x9E;
struct KeyRow { RandomizerGet id; const char* name; const char* article; const char* color; bool boss; };
/* KEY_CATALOG */
static int liveResolutions = 0;
struct GetItemEntry { uint16_t textId = TEXT_RANDOMIZER_CUSTOM_ITEM; int itemId = ITEM_NONE; };
struct CustomMessage {
    std::string english;
    CustomMessage() = default;
    CustomMessage(std::string en, int = 0) : english(std::move(en)) {}
    CustomMessage(std::string en, std::string, std::string, int = 0, int = 0) : english(std::move(en)) {}
    void Replace(const char* key, const CustomMessage& value) {
        size_t position = 0;
        while ((position = english.find(key, position)) != std::string::npos) {
            english.replace(position, std::strlen(key), value.english);
            position += value.english.size();
        }
    }
    void Replace(const char* key, const std::string& value) { Replace(key, CustomMessage(value)); }
    void Format() {
        // CustomMessage's native encoder is an engine boundary. It retains the
        // authoring text and emits the OoT controls consumed by the real export.
        std::string result;
        for (size_t i = 0; i < english.size(); ++i) {
            if (english[i] == '%' && i + 1 < english.size()) {
                const char code = english[++i];
                const char* codes = "wrgbcpyB";
                const char* color = std::strchr(codes, code);
                assert(color);
                result += '\x05';
                result += static_cast<char>(0x40 + (color - codes));
            } else if (english[i] == '&') result += '\x01';
            else if (english[i] == '^') result += '\x04';
            else result += english[i];
        }
        english = std::move(result);
    }
    void AutoFormat(int = 0) { Format(); }
    std::string GetEnglish(int) const { return english; }
};
namespace Rando::StaticData {
std::map<std::string, RandomizerGet> itemNameToEnum;
struct ItemName : std::string {
    using std::string::string;
    std::string GetEnglish() const { return *this; }
};
struct Item {
    RandomizerGet id;
    const KeyRow& Row() const {
        for (const auto& row : keyRows) if (row.id == id) return row;
        assert(false); return keyRows[0];
    }
    int GetCategory() { return ITEM_CATEGORY_MAJOR; }
    ItemName GetName() const { return Row().name; }
    bool HasCustomIcon() const { return false; }
    int GetItemID() const { return ITEM_NONE; }
    std::string GetArticle() const { return Row().article; }
    std::string GetColor() const { return Row().color; }
    std::shared_ptr<GetItemEntry> GetGIEntryUnresolved() const { return std::make_shared<GetItemEntry>(); }
    std::shared_ptr<GetItemEntry> GetGIEntry() const { ++liveResolutions; return GetGIEntryUnresolved(); }
};
Item RetrieveItem(RandomizerGet id) { return {id}; }
}
struct OTRGlobals { static OTRGlobals* Instance; void* gRandoContext = reinterpret_cast<void*>(1); void* gRandomizer = reinterpret_cast<void*>(1); } globals;
OTRGlobals* OTRGlobals::Instance = &globals;
struct MessageTableEntry { uint16_t textId; uint8_t typePos; const char* segment; uint32_t msgSize; };
static const char smallTutorial[] = "Generic key tutorial\x02";
static const char bossTutorial[] = "Generic boss tutorial\x02";
static const char chestTutorial[] = "You got a Key!\x01It opens the next door in the Treasure Chest Game.\x02";
static MessageTableEntry nativeMessages[] = {{0x60, 0, smallTutorial, sizeof(smallTutorial) - 1},
    {0xC7, 0, bossTutorial, sizeof(bossTutorial) - 1}, {0xF3, 0, chestTutorial, sizeof(chestTutorial) - 1}, {0xFFFF, 0, nullptr, 0}};
MessageTableEntry* sNesMessageEntryTablePtr = nativeMessages;
MessageTableEntry* sGerMessageEntryTablePtr = nullptr;
MessageTableEntry* sFraMessageEntryTablePtr = nullptr;
struct CustomItemMessageEntry { int rgId, itemId; const char* english; const char* german; const char* french; };
const CustomItemMessageEntry* GetCustomItemMessage(RandomizerGet) { return nullptr; }
bool DungeonInformationEnabled() { return false; }
bool BuildDungeonItemReceiptMessage(RandomizerGet, CustomMessage&, bool = true) { return false; }
bool BuildTokenReceiptMessage(RandomizerGet, CustomMessage&) { return false; }
void BuildQuarterHeartMessage(CustomMessage&) {}
void BuildDefenseUpgradeMessage(CustomMessage&) {}
void BuildSpeedUpgradeMessage(CustomMessage&) {}
void BuildPowerUpgradeMessage(CustomMessage&) {}
void BuildMagicStatUpgradeMessage(CustomMessage&) {}
void BuildCrawlSpeedUpgradeMessage(CustomMessage&) {}
void BuildClimbSpeedUpgradeMessage(CustomMessage&) {}
void BuildPushSpeedUpgradeMessage(CustomMessage&) {}
extern "C" uint8_t Wand_RandoMode(void) { return 0; }
// This key-only fixture has custom-item tutorials disabled, like its dungeon
// information service above. The wand tutorial has its own production fixture.
bool WandMedallionDescriptionsEnabled() { return false; }
/* DONOR_EXPORT */
extern "C" COMBO_EXPORT int32_t OOT_MapCompassInfoEnabled() { return 0; }
extern "C" COMBO_EXPORT int32_t OOT_GetDungeonItemReceiptPresentation(const char*, void*) { return 0; }
extern "C" COMBO_EXPORT const char* KeyFixture_Name(int id) {
    for (const auto& row : keyRows) if (row.id == id) return row.name;
    return nullptr;
}
extern "C" COMBO_EXPORT const char* KeyFixture_Color(const char* name) {
    for (const auto& row : keyRows) if (!std::strcmp(row.name, name)) return row.color;
    return nullptr;
}
extern "C" COMBO_EXPORT int KeyFixture_LiveResolutions() { return liveResolutions; }
struct InitializeCatalog { InitializeCatalog() { for (const auto& row : keyRows) Rando::StaticData::itemNameToEnum[row.name] = row.id; } } initialized;
