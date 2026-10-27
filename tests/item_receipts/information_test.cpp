// Execute production altar routing and saved-setting restore. Requirement
// builders and final font formatting are boundaries; seed flags/JSON are real.
#include <array>
#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <vector>
#define COMBO_BUILD
#define COMBO_EXPORT
#define RANDO_ENUM_BEGIN(x) enum x {
#define RANDO_ENUM_ITEM(x, ...) x,
#define RANDO_ENUM_END(x)                                                      \
  }                                                                            \
  ;
#include "soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerHintTextKey.h"
#include "soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerMiscEnums.h"
#include "soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerOptions.h"
#include "soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerSettingKey.h"
#include "combo/NeiGracePolicy.h"
#include "combo/NeiSeasonsPolicy.h"
enum MessageFormat { MF_RAW, MF_FORMATTED, MF_AUTO_FORMAT, MF_CLEAN };
constexpr int TEXTBOX_TYPE_BLUE = 2;
struct CustomMessage {
  std::string value;
  CustomMessage(std::string text = "") : value(std::move(text)) {}
  CustomMessage &operator+=(const CustomMessage &other) {
    value += other.value;
    return *this;
  }
  CustomMessage operator+(const CustomMessage &other) const {
    return CustomMessage(value + other.value);
  }
  void SetTextBoxType(int) {}
  void InsertNames(const std::vector<CustomMessage> &names) {
    for (size_t i = 0; i < names.size(); ++i) {
      const auto key = "[[" + std::to_string(i + 1) + "]]";
      const auto pos = value.find(key);
      if (pos != std::string::npos)
        value.replace(pos, key.size(), names[i].value);
    }
  }
  void ReplaceUnfilledNames(const char *) {}
  void SetSingularPlural() {}
  void InsertNumber(int) {}
  void Format() {}
  void AutoFormat() {}
  void Clean() {}
  std::string GetEnglish(MessageFormat) const { return value; }
  std::string GetGerman(MessageFormat) const { return value; }
  std::string GetFrench(MessageFormat) const { return value; }
};
struct HintText {
  CustomMessage value;
  CustomMessage GetHintMessage(size_t = 0) const { return value; }
};
namespace Rando {
struct Option {
  int value = 0;
  std::string cvarName;
  const std::string &GetCVarName() const;
  int GetValueFromText(const nlohmann::json &text) { return text == "On"; }
  bool Is(int target) const { return value == target; }
  operator bool() const { return value != 0; }
  void Set(int target) { value = target; }
};
struct Trial {
  CustomMessage GetName() const { return CustomMessage("Trial"); }
};
struct Context {
  std::array<Option, RSK_MAX> options{};
  static Context *GetInstance() {
    static Context context;
    return &context;
  }
  Option &GetOption(RandomizerSettingKey key) { return options.at(key); }
  Trial *GetTrial(int) {
    static Trial trial;
    return &trial;
  }
  void SetSeedString(const std::string &) {}
  void SetSeed(uint32_t) {}
};
namespace StaticData {
std::map<RandomizerHintTextKey, HintText> hintTextTable;
std::map<std::string, RandomizerSettingKey> optionNameToEnum{
    {"Maps and Compasses Give Information",
     RSK_MAPS_COMPASSES_GIVE_INFORMATION}};
} // namespace StaticData
struct Hint {
  Hint() = default;
  Hint(RandomizerHint key, std::vector<CustomMessage> text)
      : ownKey(key), messages(std::move(text)) {
    hintType = HINT_TYPE_MESSAGE;
  }
  HintType hintType = HINT_TYPE_ALTAR_CHILD;
  RandomizerHint ownKey = RH_ALTAR_CHILD;
  std::vector<size_t> hintTextsChosen, locations, trials, items, areas{0};
  std::vector<CustomMessage> messages;
  int num = 0;
  HintText GetHintText(size_t) const { return {CustomMessage("Other hint")}; }
  CustomMessage GetItemName(size_t) const { return CustomMessage("Item"); }
  CustomMessage GetAreaName(size_t) const {
    return CustomMessage("Test Reward Location");
  }
  static CustomMessage GetBridgeReqsText() {
    return CustomMessage("Bridge requirement;");
  }
  static CustomMessage GetGanonBossKeyText() {
    return CustomMessage("Boss key requirement;");
  }
  static CustomMessage GetGanonsSoulText() {
    return CustomMessage("Ganon soul requirement;");
  }
  static CustomMessage GetWinconText() {
    return CustomMessage("Victory requirement;");
  }
  const CustomMessage GetHintMessage(MessageFormat format, size_t id = 0) const;
};
/* ALTAR_BUILDER */
struct Settings {
  Context *mContext = Context::GetInstance();
  static std::shared_ptr<Settings> mInstance;
  std::array<Option, RSK_MAX> mOptions{};
  static std::shared_ptr<Settings> GetInstance();
  const std::array<Option, RSK_MAX>& GetAllOptions() const;
  void LoadSpoilerSettings(const nlohmann::json &spoilerFileJson) {
    /* SPOILER_SETTINGS */
  }
};
std::shared_ptr<Settings> Settings::mInstance;
/* OPTION_METADATA */
} // namespace Rando

std::map<std::string, int> intCvars;
struct OTRGlobals {
  static OTRGlobals *Instance;
  Rando::Context *gRandoContext = Rando::Context::GetInstance();
} globals;
OTRGlobals *OTRGlobals::Instance = &globals;
/* ALTAR_EXPORT */
std::map<std::string, std::string> stringCvars;
#define CVAR_RANDOMIZER_SETTING(name) "gRando." name
void CVarSetInteger(const char *name, int value) { intCvars[name] = value; }
void CVarSetString(const char *name, const char *value) {
  stringCvars[name] = value;
}
void CVarClear(const char *name) {
  intCvars.erase(name);
  stringCvars.erase(name);
}
const char *CVarGetString(const char *name, const char *fallback) {
  const auto value = stringCvars.find(name);
  return value == stringCvars.end() ? fallback : value->second.c_str();
}
int exclusionWarnings = 0;
#define SPDLOG_WARN(...) (++exclusionWarnings)
/* CLEAR_NUMERIC_EXCLUSIONS */
/* SETTINGS_RESTORE */

struct SaveManager {
  static SaveManager *Instance;
  nlohmann::json root;
  nlohmann::json *currentJsonContext = &root;
  nlohmann::json::iterator currentJsonArrayContext;
  using LoadArrayFunc = std::function<void(size_t)>;
  void LoadArray(const std::string &name, const size_t size,
                 LoadArrayFunc func);
  template <typename T>
  void LoadData(const std::string &name, T &data, const T &defaultValue = T{})
  /* LOAD_DATA */
};
SaveManager saveManager;
SaveManager *SaveManager::Instance = &saveManager;
/* LOAD_ARRAY */
void LoadSettings() {
  auto randoContext = Rando::Context::GetInstance();
  /* LOAD_SETTINGS */
}

int main() {
  static_assert(RSK_MAPS_COMPASSES_GIVE_INFORMATION ==
                RSK_PUSH_SPEED_UPGRADE_REQUIRED + 1);
  static_assert(RSK_HYLIAS_GRACE == RSK_MAPS_COMPASSES_GIVE_INFORMATION + 1);
  static_assert(RSK_HYLIAS_GRACE_REWARDS + 1 == RSK_ROD_OF_SEASONS);
  static_assert(RSK_STARTING_ROD_OF_SEASONS + 1 == RSK_SHUFFLE_MAGIC_MIRROR);
  static_assert(RSK_SHUFFLE_MAGIC_MIRROR + 1 == RSK_MAX);
  auto ctx = Rando::Context::GetInstance();
  using namespace Rando;
  StaticData::hintTextTable[RHT_CHILD_ALTAR_STONES] = {
      CustomMessage("Stone at [[1]];")};
  StaticData::hintTextTable[RHT_ADULT_ALTAR_MEDALLIONS] = {
      CustomMessage("Medallion at [[1]];")};
  StaticData::hintTextTable[RHT_CHILD_ALTAR_TEXT_END_DOTOPEN] = {
      CustomMessage("Door open;")};
  StaticData::hintTextTable[RHT_CHILD_ALTAR_TEXT_END_DOTSONGONLY] = {
      CustomMessage("Door needs song;")};
  StaticData::hintTextTable[RHT_CHILD_ALTAR_TEXT_END_DOTCLOSED] = {
      CustomMessage("Door needs stones;")};
  StaticData::hintTextTable[RHT_ADULT_ALTAR_TEXT_END] = {
      CustomMessage("Altar ending;")};
  ctx->GetOption(RSK_TOT_ALTAR_HINT).Set(1);
  for (int information : {0, 1}) {
    ctx->GetOption(RSK_MAPS_COMPASSES_GIVE_INFORMATION).Set(information);
    for (int door :
         {RO_DOOROFTIME_OPEN, RO_DOOROFTIME_SONGONLY, RO_DOOROFTIME_CLOSED}) {
      ctx->GetOption(RSK_DOOR_OF_TIME).Set(door);
      Hint child;
      const auto text = child.GetHintMessage(MF_RAW).value;
      assert((text.find("Test Reward Location") != std::string::npos) ==
             !information);
      assert(text.find(door == RO_DOOROFTIME_OPEN ? "Door open;"
                       : door == RO_DOOROFTIME_SONGONLY
                           ? "Door needs song;"
                           : "Door needs stones;") != std::string::npos);
    }
    Hint adult;
    adult.hintType = HINT_TYPE_ALTAR_ADULT;
    adult.ownKey = RH_ALTAR_ADULT;
    const auto text = adult.GetHintMessage(MF_RAW).value;
    assert((text.find("Test Reward Location") != std::string::npos) ==
           !information);
    for (const auto *phrase :
         {"Bridge requirement;", "Boss key requirement;",
          "Ganon soul requirement;", "Victory requirement;"})
      assert(text.find(phrase) != std::string::npos);
    // Consolidated Combo hints are stored as precomposed MESSAGE text.
    // On replaces reward paragraphs; off returns the exact existing text.
    adult.hintType = HINT_TYPE_MESSAGE;
    adult.messages = {
        CustomMessage("Legacy reward location;Bridge requirement;")};
    const auto composed = adult.GetHintMessage(MF_RAW).value;
    if (information) {
      assert(composed.find("Legacy reward location") == std::string::npos);
      assert(composed.find("Victory requirement") != std::string::npos);
    } else
      assert(composed == adult.messages[0].value);
  }

  const auto beforeExport = ctx->options;
  const auto exported = nlohmann::json::parse(SOH_DumpAltarHintMessages());
  assert(exported.at("__ALTAR_CHILD__")[0]["en"] == "Door needs stones;");
  assert(exported.at("__ALTAR_ADULT__")[0]["de"] ==
         "Bridge requirement;Boss key requirement;Ganon soul "
         "requirement;Victory requirement;Altar ending;");
  for (size_t i = 0; i < beforeExport.size(); ++i)
    assert(beforeExport[i].value == ctx->options[i].value);
  ctx->GetOption(RSK_MAPS_COMPASSES_GIVE_INFORMATION).Set(0);
  assert(nlohmann::json::parse(SOH_DumpAltarHintMessages()).empty());
  ctx->GetOption(RSK_MAPS_COMPASSES_GIVE_INFORMATION).Set(1);
  globals.gRandoContext = nullptr;
  assert(nlohmann::json::parse(SOH_DumpAltarHintMessages()).empty());
  globals.gRandoContext = ctx;

  // Replay an old consolidated snapshot after a locally enabled menu option.
  const auto *key = CVAR_RANDOMIZER_SETTING("MapsCompassesGiveInformation");
  Settings::GetInstance()->mOptions[RSK_MAPS_COMPASSES_GIVE_INFORMATION].cvarName = key;
  Settings::GetInstance()->mOptions[RSK_NO_LOGIC].cvarName = "gRandoSettings.NoLogic";
  intCvars[key] = 1;
  intCvars["gRandoSettings.NoLogic"] = 7;
  intCvars["unrelated.setting"] = 9;
  SOH_RestoreRandoSettings("{}");
  assert(intCvars.at(key) == 0);
  assert(!intCvars.contains("gRandoSettings.NoLogic"));
  assert(intCvars.at("unrelated.setting") == 9);
  SOH_RestoreRandoSettings(nlohmann::json{{key, 1}}.dump().c_str());
  assert(intCvars.at(key) == 1);
  SOH_RestoreRandoSettings(nlohmann::json{{"gRandoSettings.LogicRules", 2},
      {CVAR_RANDOMIZER_SETTING("ExcludedLocations"), "42"}}.dump().c_str());
  assert(intCvars.at("gRandoSettings.NoLogic") == 2);
  assert(stringCvars.at(CVAR_RANDOMIZER_SETTING("ExcludedLocations")).empty());
  assert(exclusionWarnings == 1);
  SOH_RestoreRandoSettings(nlohmann::json{{CVAR_RANDOMIZER_SETTING("ExcludedLocations"),
      "Kokiri Sword Chest"}}.dump().c_str());
  assert(stringCvars.at(CVAR_RANDOMIZER_SETTING("ExcludedLocations")) == "Kokiri Sword Chest");
  assert(exclusionWarnings == 1);
  Settings settings;
  nlohmann::json spoiler{{"seed", "fixture"}, {"finalSeed", 123}};
  ctx->GetOption(RSK_MAPS_COMPASSES_GIVE_INFORMATION).Set(1);
  ctx->GetOption(RSK_HYLIAS_GRACE).Set(NEI_GRACE_GATED);
  ctx->GetOption(RSK_HYLIAS_GRACE_REWARDS).Set(7);
  settings.LoadSpoilerSettings(spoiler);
  assert(ctx->GetOption(RSK_MAPS_COMPASSES_GIVE_INFORMATION).Is(0));
  assert(ctx->GetOption(RSK_HYLIAS_GRACE).Is(NEI_GRACE_ON));
  assert(ctx->GetOption(RSK_HYLIAS_GRACE_REWARDS).Is(4));
  spoiler["settings"] = {{"Maps and Compasses Give Information", "On"}};
  settings.LoadSpoilerSettings(spoiler);
  assert(ctx->GetOption(RSK_MAPS_COMPASSES_GIVE_INFORMATION).Is(1));
  // LoadArray/LoadData and the actual randoSettings callback must default an
  // appended missing value to off, preserving each preceding numeric slot.
  std::vector<int> old(RSK_MAPS_COMPASSES_GIVE_INFORMATION, 7);
  saveManager.root["randoSettings"] = old;
  LoadSettings();
  assert(ctx->GetOption(RSK_MAPS_COMPASSES_GIVE_INFORMATION).Is(0));
  assert(ctx->GetOption(RSK_PUSH_SPEED_UPGRADE_REQUIRED).Is(7));
  old.push_back(1);
  saveManager.root["randoSettings"] = old;
  LoadSettings();
  assert(ctx->GetOption(RSK_MAPS_COMPASSES_GIVE_INFORMATION).Is(1));
  std::cout
      << "Dungeon information altar replacement preserves progression "
         "requirements; old/new save and seed settings round-trip passed\n";
}
