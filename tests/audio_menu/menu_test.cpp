#include "gui/WhatsNew.h"
#include <imgui.h>
#include <imgui_internal.h>

#include <array>
#include <cfloat>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#define CVAR_AUDIO(name) "gAudioEditor." name
#define CVAR_LINK_VOICE_FREQ_MULTIPLIER "gAudioEditor.LinkVoiceFreqMultiplier"
#define SOH_SEASON_WEATHER_CVAR(name) "gAudioEditor.SeasonWeather." name
#define MM_WEATHER_CVAR(name) "gAudioEditor.MMWeather." name
#define MM_SUMMER_CVAR(name) "gEnhancements.Graphics.SummerAtmosphere." name
constexpr int THEME_COLOR = 0;
constexpr int CONCURRENT_WEATHER_THUNDER_LOW = 0;
constexpr int CONCURRENT_WEATHER_THUNDER_LAYERED = 1;
constexpr int CONCURRENT_WEATHER_THUNDER_LIGHTNING = 2;
constexpr int SEASON_SPRING = 0, SEASON_SUMMER = 1, SEASON_AUTUMN = 2,
              SEASON_WINTER = 3;
constexpr int SEQ_BGM_WORLD = 1, SEQ_BGM_EVENT = 2, SEQ_BGM_BATTLE = 4;
constexpr int HYRULE_FIELD_NIGHT_MUSIC_ENTRY = 100;

static std::map<std::string, int> values;
static int writes, refreshes, saves;
static std::string lastTooltip;
int CVarGetInteger(const char *key, int fallback) {
  const auto value = values.find(key);
  return value == values.end() ? fallback : value->second;
}
void CVarSetInteger(const char *key, int value) {
  values[key] = value;
  ++writes;
}
void CVarSetFloat(const char *, float) { ++writes; }
void HyruleFieldNightMusic_RequestRefresh() { ++refreshes; }
namespace Ship {
struct Context {
  static Context *GetRawInstance() {
    static Context context;
    return &context;
  }
  Context *GetWindow() { return this; }
  Context *GetGui() { return this; }
  void SaveConsoleVariablesNextFrame() { ++saves; }
};
} // namespace Ship
struct SequenceInfo {
  std::string label;
  bool canBeUsedAsReplacement;
  int category;
};
struct AudioCollection {
  static AudioCollection *Instance;
  std::map<int, SequenceInfo> GetAllSequences() const {
    return {{1, {"Night A", true, SEQ_BGM_WORLD}},
            {2, {"Night B", true, SEQ_BGM_EVENT}},
            {3, {"Forbidden voice", true, 8}},
            {4, {"Unavailable", false, SEQ_BGM_WORLD}}};
  }
  int GetReplacementSequence(int) const {
    return CVarGetInteger(CVAR_AUDIO("HyruleFieldNightSequence"), 1);
  }
};
static AudioCollection collection;
AudioCollection *AudioCollection::Instance = &collection;

struct Color_RGBA8 {
  int r, g, b, a;
};
struct Options {
  int defaultValue = 0;
  Options &DefaultValue(bool v) {
    defaultValue = v;
    return *this;
  }
  Options &DefaultValue(int v) {
    defaultValue = v;
    return *this;
  }
  Options &DefaultValue(Color_RGBA8) { return *this; }
  Options &DefaultIndex(int v) {
    defaultValue = v;
    return *this;
  }
  template <class T> Options &ComboMap(const T &) { return *this; }
  Options &Color(int) { return *this; }
  Options &Tooltip(const char *) { return *this; }
  Options &Min(int) { return *this; }
  Options &Max(int) { return *this; }
  Options &Size(ImVec2) { return *this; }
  Options &ShowReset() { return *this; }
};
using CheckboxOptions = Options;
using ComboboxOptions = Options;
using IntSliderOptions = Options;
using ColorPickerOptions = Options;
enum class WidgetType {
  WIDGET_CVAR_CHECKBOX,
  WIDGET_CVAR_COMBOBOX,
  WIDGET_CVAR_SLIDER_INT,
  WIDGET_CVAR_COLOR_PICKER,
  WIDGET_CUSTOM
};
struct WidgetInfo {
  std::string name;
  WidgetType type{};
  std::string key;
  int defaultValue = 0;
  std::function<void(WidgetInfo &)> custom;
  WidgetInfo &CVar(const char *value) {
    key = value;
    return *this;
  }
  WidgetInfo &Options(const ::Options &value) {
    defaultValue = value.defaultValue;
    return *this;
  }
  WidgetInfo &CustomFunction(std::function<void(WidgetInfo &)> f) {
    custom = f;
    return *this;
  }
};
struct SearchWidget {
  WidgetInfo widget;
  const char *section;
  const char *editor;
  const char *tab;
};
struct NativeMenu {
  std::vector<std::string> drawn;
  std::vector<SearchWidget> search;
  ImVec4 normalText;
  void MenuDrawItem(WidgetInfo &widget, int) {
    const auto text = ImGui::GetStyle().Colors[ImGuiCol_Text];
    if (text.x != normalText.x || text.y != normalText.y ||
        text.z != normalText.z || text.w != normalText.w)
      throw std::runtime_error(
          "tab accent must be restored before native widget contents");
    drawn.push_back(widget.key);
    if (widget.custom)
      widget.custom(widget);
    else
      ImGui::TextUnformatted(widget.name.c_str());
  }
  void MenuDrawItem(WidgetInfo &widget, float, int theme) {
    MenuDrawItem(widget, theme);
  }
  void AddSearchWidget(SearchWidget widget) {
    search.push_back(std::move(widget));
  }
};
static NativeMenu menu;
namespace SohGui {
NativeMenu *mSohMenu = &menu;
}
namespace BenGui {
NativeMenu *mBenMenu = &menu;
}
namespace UIWidgets {
void PushStyleTabs(int) {}
void PopStyleTabs() {}
void PushStyleCombobox(int) {}
void PopStyleCombobox() {}
void Tooltip(const char *text) { lastTooltip = text; }
void CVarColorPicker(const char *label, const char *, Color_RGBA8, bool, void *,
                     int) {
  ImGui::TextUnformatted(label);
}
struct ButtonOptions {
  ButtonOptions &Size(ImVec2) { return *this; }
  ButtonOptions &Padding(ImVec2) { return *this; }
};
bool Button(const char *label, ButtonOptions) { return ImGui::Button(label); }
} // namespace UIWidgets
#include "production_audio.inc"

static void Check(bool pass, const char *message) {
  if (!pass)
    throw std::runtime_error(message);
}
struct Tab {
  ImGuiID id;
  ImRect rect;
  bool accent;
};
static std::map<std::string, Tab> tabs;
static float windowWidth = 1500;

static void Frame(ImVec2 mouse = ImVec2(-100, -100), bool down = false) {
  auto &io = ImGui::GetIO();
  io.AddMousePosEvent(mouse.x, mouse.y);
  io.AddMouseButtonEvent(0, down);
  menu.drawn.clear();
  ImGui::NewFrame();
  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(ImVec2(windowWidth, 1000));
  ImGui::Begin("Audio editor probe", nullptr, ImGuiWindowFlags_NoDecoration);
  DrawAudioMenu();
  tabs.clear();
  for (auto &bar : GImGui->TabBars.Buf) {
    for (auto &item : bar.Tabs) {
      std::string name = ImGui::TabBarGetTabName(&bar, &item);
      if (const auto stable = name.rfind("###"); stable != std::string::npos)
        name = name.substr(stable + 3);
      const auto *entry = WhatsNew::Find(nullptr, name.c_str());
      const auto accent =
          entry ? ImGui::ColorConvertFloat4ToU32(WhatsNew::Accent(entry->kind))
                : 0;
      const ImRect rect(
          ImVec2(bar.BarRect.Min.x + item.Offset - bar.ScrollingAnim,
                 bar.BarRect.Min.y),
          ImVec2(bar.BarRect.Min.x + item.Offset - bar.ScrollingAnim +
                     item.Width,
                 bar.BarRect.Max.y));
      bool colored = false;
      for (const auto *window : GImGui->Windows) {
        for (const auto &vertex : window->DrawList->VtxBuffer)
          colored |= vertex.col == accent && rect.Contains(vertex.pos);
      }
      tabs[name] = {item.ID, rect, colored};
    }
  }
  ImGui::End();
  ImGui::Render();
}
static void ClickTab(const char *name) {
  Check(tabs.contains(name), "requested tab must exist");
  const auto pos = tabs.at(name).rect.GetCenter();
  Frame(pos);
  Frame(pos, true);
  Frame(pos);
  Frame();
}

static void ProbeWeather() {
  RegisterWeatherWidgets();
  Check(writes == 0, "registering weather widgets must not change preferences");
  const char *seasonKey =
      TEST_MM ? MM_WEATHER_CVAR("Season") : SOH_SEASON_WEATHER_CVAR("Season");
  values[seasonKey] = -2;
  values[WhatsNew::kEnabledCvar] = 1;
  menu.normalText = ImGui::GetStyle().Colors[ImGuiCol_Text];
  Frame();
  Frame();
  Frame();
  const auto weatherId = tabs.at("Weather & Seasons").id;
  Check(tabs.at("Weather & Seasons").accent,
        "Weather & Seasons tab must receive the current release highlight");
  Check(std::find(menu.drawn.begin(), menu.drawn.end(), seasonKey) ==
            menu.drawn.end(),
        "audio options must not contain weather controls");
  ClickTab("Weather & Seasons");
  for (const auto *season : {"Spring", "Summer", "Autumn", "Winter"})
    Check(tabs.contains(season) && tabs.at(season).accent,
          "all four seasonal tabs must be highlighted");
  const std::array<size_t, 4> starts{1, 3, 7, 9}, ends{3, 7, 9, 13};
  size_t index = 0;
  for (const auto *season : {"Spring", "Summer", "Autumn", "Winter"}) {
    ClickTab(season);
    const std::set<std::string> drawn(menu.drawn.begin(), menu.drawn.end());
    Check(drawn.size() == menu.drawn.size(),
          "moved weather controls must be drawn once");
    for (size_t i = 1; i < seasonWeatherWidgets.size(); ++i)
      Check(drawn.contains(seasonWeatherWidgets[i].key) ==
                (i >= starts[index] && i < ends[index]),
            "seasonal navigation must display only that season's native "
            "controls");
    Check(CVarGetInteger(seasonKey, 0) == -2 && writes == 0,
          "opening a season tab must not change the selected gameplay/weather "
          "season");
    ++index;
  }
  for (const auto &entry : menu.search) {
    if (entry.widget.key.find("Weather.") == std::string::npos &&
        entry.widget.key.find("SummerAtmosphere.") == std::string::npos &&
        entry.widget.key.find("OutdoorRain") == std::string::npos &&
        entry.widget.key.find("Weather") == std::string::npos &&
        entry.widget.name != "Outdoor Rain Color")
      continue;
    Check(std::string(entry.tab) == "Weather & Seasons",
          "weather search results must route to their actual tab");
    Check(WhatsNew::Find(entry.widget.key.c_str()),
          "all registered weather widgets need exact release keys");
  }
  const auto highlightedTabs = tabs;
  values[WhatsNew::kEnabledCvar] = 0;
  Frame();
  Check(tabs.at("Weather & Seasons").id == weatherId,
        "toggle must preserve the original weather tab ID");
  for (const auto *name :
       {"Weather & Seasons", "Spring", "Summer", "Autumn", "Winter"}) {
    Check(!tabs.at(name).accent,
          "What's New off must restore normal tab presentation");
    Check(tabs.at(name).id == highlightedTabs.at(name).id,
          "release toggle must preserve every season tab ID");
  }
  values[WhatsNew::kEnabledCvar] = 1;
  for (const float width : {320.f, 420.f, 600.f}) {
    windowWidth = width;
    Frame();
    Frame();
    for (const auto *season : {"Spring", "Summer", "Autumn", "Winter"}) {
      Check(tabs.at(season).rect.Min.x >= 0 &&
                tabs.at(season).rect.Max.x <= width,
            "season tabs must fit the narrow native tab bar");
      ClickTab(season);
      Check(writes == 0 && CVarGetInteger(seasonKey, 0) == -2,
            "narrow highlighted tabs must retain native navigation without "
            "changing the season");
    }
  }
  windowWidth = 1500;
#if TEST_MM
  Check(weatherWidgets[2].key == MM_WEATHER_CVAR("RainColor"),
        "custom MM rain picker must expose its existing key to the native "
        "highlight owner");
#endif
}

#if !TEST_MM
static ImGuiID nightId;
static ImRect nightRect;
static bool NightFrame(ImVec2 mouse = ImVec2(-100, -100), bool down = false) {
  auto &io = ImGui::GetIO();
  io.AddMousePosEvent(mouse.x, mouse.y);
  io.AddMouseButtonEvent(0, down);
  ImGui::NewFrame();
  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(ImVec2(900, 500));
  ImGui::Begin("Night selector probe", nullptr, ImGuiWindowFlags_NoDecoration);
  const int colors = GImGui->ColorStack.Size;
  DrawHyruleFieldNightTrack();
  Check(GImGui->ColorStack.Size == colors,
        "raw night scope must restore native styles");
  if (!ImGui::IsPopupOpen("##HyruleFieldNightTrack")) {
    nightId = GImGui->LastItemData.ID;
    nightRect = GImGui->LastItemData.Rect;
  }
  const auto *entry = WhatsNew::Find(CVAR_AUDIO("HyruleFieldNightSequence"));
  const auto accent =
      entry ? ImGui::ColorConvertFloat4ToU32(WhatsNew::Accent(entry->kind)) : 0;
  bool highlighted = false;
  for (const auto &vertex : ImGui::GetWindowDrawList()->VtxBuffer)
    highlighted |= vertex.col == accent;
  ImGui::End();
  ImGui::Render();
  return highlighted;
}
static void ProbeNight() {
  values[CVAR_AUDIO("HyruleFieldNightMusic")] = 1;
  values[WhatsNew::kEnabledCvar] = 0;
  NightFrame();
  const auto original = nightId;
  values[WhatsNew::kEnabledCvar] = 1;
  Check(NightFrame(),
        "night track selector must receive its exact release highlight");
  Check(original == nightId && writes == 0 && refreshes == 0 && saves == 0,
        "night highlight must preserve the hidden widget ID without writing "
        "settings");
  Check(lastTooltip.find("supports shuffle, reset and locking") !=
            std::string::npos,
        "the night selector must retain its existing tooltip");
  values[WhatsNew::kEnabledCvar] = 0;
  Check(!NightFrame(),
        "What's New off must restore normal night selector colors");
  values[WhatsNew::kEnabledCvar] = 1;
  NightFrame();
  const auto combo = nightRect.GetCenter();
  NightFrame(combo);
  NightFrame(combo, true);
  NightFrame(combo);
  NightFrame();
  auto *popup = ImGui::FindWindowByName("##Combo_00");
  Check(popup && popup->Active,
        "the highlighted selector must retain native popup input");
  const auto &style = ImGui::GetStyle();
  const auto second =
      ImVec2(popup->DC.CursorStartPos.x + 20, popup->DC.CursorStartPos.y +
                                                  ImGui::GetFontSize() * 1.5f +
                                                  style.ItemSpacing.y);
  NightFrame(second);
  NightFrame(second, true);
  NightFrame(second);
  Check(CVarGetInteger(CVAR_AUDIO("HyruleFieldNightSequence"), 0) == 2 &&
            writes == 1 && refreshes == 1 && saves == 1,
        "native track selection must update its original key, refresh audio "
        "and schedule exactly one save");
  values[CVAR_AUDIO("HyruleFieldNightMusic")] = 0;
  values[WhatsNew::kEnabledCvar] = 1;
  Check(!NightFrame() && writes == 1 && refreshes == 1 && saves == 1,
        "the existing night music gate must hide its selector without changing "
        "audio or preferences");
}
#endif

int main() {
  try {
    ImGui::CreateContext();
    auto &io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = ImVec2(1600, 1100);
    io.DeltaTime = 1.0f / 60;
    unsigned char *pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    ProbeWeather();
#if !TEST_MM
    ProbeNight();
#endif
    ImGui::DestroyContext();
    std::cout << "PASS " << (TEST_MM ? "MM" : "OoT")
              << " audio menu: real seasonal tab input, unchanged "
                 "season/preferences, search routing, release presentation"
              << (TEST_MM ? " and custom weather metadata\n"
                          : " and raw night selector identity/tooltips\n");
  } catch (const std::exception &error) {
    std::cerr << "FAIL " << (TEST_MM ? "MM" : "OoT")
              << " audio menu: " << error.what() << '\n';
    return 1;
  }
}
