// Run the actual native registration callbacks and the common settings body.
#include "ComboGameMode.h"
#include "emotes/EmoteCatalog.h"
#include "gui/ComboEmoteSettings.h"
#include <array>
#include <cstdarg>
#include <cstdio>
#include <functional>
#include <imgui.h>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

static void Check(bool value, const char *why) {
  if (!value)
    throw std::runtime_error(why);
}
static std::map<std::string, int32_t> integers;
static std::map<std::string, float> floats;
static int saves = 0;
extern "C" {
int gComboGameMode = COMBO_GAME_BOTH;
int32_t CVarGetInteger(const char *key, int32_t fallback) {
  return integers.contains(key) ? integers.at(key) : fallback;
}
void CVarSetInteger(const char *key, int32_t value) { integers[key] = value; }
float CVarGetFloat(const char *key, float fallback) {
  return floats.contains(key) ? floats.at(key) : fallback;
}
void CVarSetFloat(const char *key, float value) { floats[key] = value; }
void CVarSave() { ++saves; }
}

struct UiInput {
  int slot = -1;
  std::array<int, 4> select{};
  std::array<std::set<std::string>, 4> options;
  bool shortcuts = false, disable = false, stop = false, volume = false;
  int playSlot = -1;
  std::string text;
} ui;

ImGuiStyle::ImGuiStyle() {
  Colors[ImGuiCol_Text] = ImVec4(1, 1, 1, 1);
  ItemInnerSpacing = ImVec2(4, 4);
}

namespace ImGui {
ImGuiStyle &GetStyle() {
  static ImGuiStyle style;
  return style;
}
static std::vector<std::pair<ImGuiCol, ImVec4>> colors;
void PushStyleColor(ImGuiCol index, const ImVec4 &color) {
  colors.emplace_back(index, GetStyle().Colors[index]);
  GetStyle().Colors[index] = color;
}
void PopStyleColor(int count) {
  while (count--) {
    Check(!colors.empty(), "balanced release highlight style");
    GetStyle().Colors[colors.back().first] = colors.back().second;
    colors.pop_back();
  }
}
void TextUnformatted(const char *, const char *) {}
ImVec2 CalcTextSize(const char *text, const char *, bool, float) {
  return ImVec2(std::string(text).size() * 7, 16);
}
ImVec2 GetContentRegionAvail() { return ImVec2(500, 500); }
float GetFrameHeight() { return 20; }
float GetTextLineHeight() { return 16; }
bool IsItemHovered(ImGuiHoveredFlags) { return false; }
void SetTooltip(const char *, ...) {}
void SeparatorText(const char *) {}
void Separator() {}
void SetNextItemWidth(float) {}
void SameLine(float, float) {}
void PushID(int slot) { ui.slot = slot; }
void PopID() { ui.slot = -1; }
void Text(const char *, ...) {}
void TextDisabled(const char *, ...) {}
void TextWrapped(const char *format, ...) {
  char text[1024];
  va_list args;
  va_start(args, format);
  vsnprintf(text, sizeof(text), format, args);
  va_end(args);
  ui.text += text;
  ui.text += '\n';
}
bool BeginCombo(const char *label, const char *, ImGuiComboFlags) {
  return std::string(label) == "Animation" || ui.shortcuts;
}
void EndCombo() {}
bool Selectable(const char *label, bool, ImGuiSelectableFlags, const ImVec2 &) {
  if (std::string(label) == "Clear shortcut")
    return false;
  ui.options.at(ui.slot).insert(label);
  const auto *selected = ComboEmotes::Find(ui.select.at(ui.slot));
  return selected && std::string(label) == selected->name;
}
bool Checkbox(const char *label, bool *value) {
  const bool clicked =
      (std::string(label) == "Enable emotes" && ui.disable) ||
      (std::string(label) == "M2" && ui.shortcuts && ui.slot == 0);
  if (clicked)
    *value = !*value;
  return clicked;
}
bool Button(const char *label, const ImVec2 &) {
  return (std::string(label) == "Play" && ui.slot == ui.playSlot) ||
         (std::string(label) == "Stop emote" && ui.stop);
}
bool SliderFloat(const char *, float *value, float, float, const char *,
                 ImGuiSliderFlags) {
  if (ui.volume)
    *value = 0.25f;
  return ui.volume;
}
bool CollapsingHeader(const char *, ImGuiTreeNodeFlags) { return true; }
} // namespace ImGui

enum { SECTION_COLUMN_1, WIDGET_CUSTOM };
struct WidgetPath {
  std::string sectionName, sidebarName;
  int column;
};
struct WidgetInfo {
  std::function<void(WidgetInfo &)> customFunction;
  WidgetInfo &RaceDisable(bool) { return *this; }
  WidgetInfo &CustomFunction(std::function<void(WidgetInfo &)> callback) {
    customFunction = callback;
    return *this;
  }
};
struct MenuFixture {
  bool sidebar = false;
  WidgetInfo widget;
  void AddSidebarEntry(const std::string &section, const std::string &name,
                       unsigned columns) {
    Check(section == "Settings" && name == "Emotes" && columns == 1,
          "native Emotes sidebar path");
    sidebar = true;
  }
  WidgetInfo &AddWidget(const WidgetPath &path, const std::string &, int type) {
    Check(sidebar && path.sectionName == "Settings" &&
              path.sidebarName == "Emotes" && path.column == SECTION_COLUMN_1 &&
              type == WIDGET_CUSTOM,
          "native Emotes widget route");
    return widget;
  }
};
#include "native_emote_registration.inc"

template <typename NativeMenu> static void ExerciseNativeMenu(int soloMode) {
  gComboGameMode = COMBO_GAME_BOTH;
  NativeMenu combined;
  combined.Register();
  Check(!combined.sidebar,
        "combined mode retains its existing shared Emotes panel");

  gComboGameMode = soloMode;
  NativeMenu native;
  native.Register();
  Check(native.sidebar && bool(native.widget.customFunction),
        "solo native Settings must expose the Emotes panel");
  auto draw = [&]() { native.widget.customFunction(native.widget); };
  integers.clear();
  floats.clear();
  saves = 0;
  ui = {};
  ui.select = {3, 6, 7,
               8}; // The four clips unavailable through the default chords.
  ui.volume = true;
  draw();
  for (size_t slot = 0; slot < ComboEmotes::kSlots; ++slot) {
    Check(integers.at(ComboEmotes::kSlotKeys[slot]) == ui.select[slot],
          "native choices persist the shared slot CVars");
    for (const auto &entry : ComboEmotes::kCatalog)
      Check(ui.options[slot].contains(entry.name),
            "all eight clips must be selectable in each native slot");
  }
  Check(floats.at("gCombo.Emotes.Volume") == 0.25f && saves == 1,
        "native sound volume persists");
  for (const auto &entry : ComboEmotes::kCatalog)
    Check(ui.text.find(entry.author) != std::string::npos,
          "native panel retains every creator credit");
  ui = {};
  ui.shortcuts = true;
  draw();
  Check(integers.at(ComboEmotes::kChordKeys[0]) ==
            int32_t(ComboEmotes::kDefaultChords[0] | 0x80),
        "native shortcut editing uses the shared chord CVar");
  for (const int slot : {1, 2}) {
    ui = {};
    ui.playSlot = slot;
    draw();
    Check(integers.at("gCombo.Emotes.Request") == (slot == 1 ? 6 : 7),
          "both voiced clips can be queued natively");
  }
  ui = {};
  ui.stop = true;
  draw();
  Check(integers.at("gCombo.Emotes.Request") == -1,
        "native Stop reaches the shared player request");
  ui = {};
  ui.disable = true;
  draw();
  Check(integers.at("gCombo.Emotes.Enabled") == 0 &&
            integers.at("gCombo.Emotes.Request") == -1,
        "native disable stops playback");
}

int main() {
  ExerciseNativeMenu<OotMenu>(COMBO_GAME_OOT);
  ExerciseNativeMenu<MmMenu>(COMBO_GAME_MM);
  std::puts("PASS both native emote callbacks: eight choices, voiced playback, "
            "chords, volume, stop and credits");
}
