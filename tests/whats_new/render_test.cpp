// Exercise the production release helper and both unchanged native checkbox
// renderers against the project's real, patched Dear ImGui. No GPU is required.
#include "gui/WhatsNew.h"
#include <imgui_internal.h>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>

static void Check(bool condition, const char *why) {
  if (!condition)
    throw std::runtime_error(why);
}
static bool Ship_IsCStringEmpty(const char *text) { return !text || !text[0]; }
// Only native option/storage dependencies are fixtures; the checkbox drawing,
// input behavior, labels and release scope below are the exact production code.
// MM's existing WrappedText compares its signed line counter with an unsigned
// width. Keep that unrelated function unchanged in this extraction probe.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#include "native_checkboxes.inc"
#pragma GCC diagnostic pop

struct Result {
  ImGuiID id;
  ImVec2 min, max;
  float right;
  bool changed;
  bool tooltip;
  int markerVertices;
};
template <typename Draw>
Result Frame(Draw draw, bool enabled, bool disabled, bool &value,
             ImVec2 mouse = ImVec2(-100, -100), bool mouseDown = false,
             float width = 400) {
  auto &io = ImGui::GetIO();
  io.AddMousePosEvent(mouse.x, mouse.y);
  io.AddMouseButtonEvent(0, mouseDown);
  ImGui::NewFrame();
  ImGui::SetNextWindowPos(ImVec2(20, 20));
  ImGui::SetNextWindowSize(ImVec2(width, 300));
  ImGui::Begin("Release test", nullptr,
               ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar);
  const char *label = "Always Thawed Zora's Domain";
  const ImGuiID original = ImGui::GetID(label);
  const ImVec4 normal = ImGui::GetStyle().Colors[ImGuiCol_Text];
  const int colors = GImGui->ColorStack.Size;
  Result result{};
  result.right = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
  {
    const int beforeMarker = ImGui::GetWindowDrawList()->VtxBuffer.Size;
    WhatsNew::Scope highlight("gEnhancements.AlwaysThawedZorasDomain", label,
                              enabled, disabled);
    result.markerVertices =
        ImGui::GetWindowDrawList()->VtxBuffer.Size - beforeMarker;
    Check((GImGui->ColorStack.Size == colors + 1) == (enabled && !disabled),
          "only active controls get an accent");
    result.changed = draw(label, &value, disabled);
    result.id = ImGui::GetItemID();
    result.min = ImGui::GetItemRectMin();
    result.max = ImGui::GetItemRectMax();
    Check(result.id == original,
          "release markers must preserve the original control ID");
  }
  Check(GImGui->ColorStack.Size == colors,
        "release scope must balance the real style stack");
  const ImVec4 restored = ImGui::GetStyle().Colors[ImGuiCol_Text];
  Check(restored.x == normal.x && restored.y == normal.y &&
            restored.z == normal.z && restored.w == normal.w,
        "later unrelated controls must retain their original text color");
  ImGui::End();
  ImGui::Render();
  for (auto *window : GImGui->Windows)
    result.tooltip |=
        window->Active && (window->Flags & ImGuiWindowFlags_Tooltip);
  return result;
}

template <typename Draw> void Probe(Draw draw) {
  ImGui::CreateContext();
  auto &io = ImGui::GetIO();
  io.IniFilename = nullptr;
  io.DisplaySize = ImVec2(900, 600);
  io.DeltaTime = 1.0f / 60;
  unsigned char *pixels;
  int width, height;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
  bool value = false;
  Frame(draw, false, false, value);
  const auto original = Frame(draw, false, false, value);
  const auto highlighted = Frame(draw, true, false, value);
  Check(original.id == highlighted.id && !value,
        "styling alone must not toggle a setting");
  Check(highlighted.markerVertices == 0,
        "color-only highlights must not submit marker text");
  Check(original.min.x == highlighted.min.x &&
            original.min.y == highlighted.min.y &&
            original.max.x == highlighted.max.x &&
            original.max.y == highlighted.max.y,
        "highlights must preserve the complete native control rectangle");
  ImVec2 hit(highlighted.min.x + 8,
             (highlighted.min.y + highlighted.max.y) * 0.5f);
  Frame(draw, true, false, value, hit);
  Frame(draw, true, false, value, hit, true);
  const auto clicked = Frame(draw, true, false, value, hit, false);
  Check(clicked.changed && value,
        "highlighted native controls must still accept clicks");
  const auto disabled = Frame(draw, true, true, value);
  Check(disabled.markerVertices == 0,
        "disabled controls must not gain marker text");
  hit = ImVec2(disabled.min.x + 8, (disabled.min.y + disabled.max.y) * 0.5f);
  Frame(draw, true, true, value, hit, true);
  const auto locked = Frame(draw, true, true, value, hit, false);
  Check(!locked.changed && value,
        "release markers must not bypass native disabled controls");
  const auto off = Frame(draw, false, false, value);
  Check(off.min.x == original.min.x && off.min.y == original.min.y,
        "off restores the original layout");
  // Highlighting must preserve geometry even at narrow-column wrap boundaries.
  for (int windowWidth = 250; windowWidth <= 340; ++windowWidth) {
    const auto native = Frame(draw, false, false, value, ImVec2(-100, -100),
                              false, windowWidth);
    const auto narrow =
        Frame(draw, true, false, value, ImVec2(-100, -100), false, windowWidth);
    Check(narrow.min.x == native.min.x && narrow.min.y == native.min.y &&
              narrow.max.x == native.max.x && narrow.max.y == native.max.y,
          "highlighting must not move or resize a narrow native control");
    Check(narrow.max.x <= narrow.right + 1,
          "a release marker must not push native controls outside a narrow "
          "column");
  }
  const auto help = Frame(draw, true, false, value);
  hit = ImVec2(help.min.x + 8, (help.min.y + help.max.y) * 0.5f);
  Frame(draw, true, false, value, hit);
  const auto hover = Frame(draw, true, false, value, hit);
  Check(hover.tooltip, "highlighted controls keep their native tooltip");
  ImGui::DestroyContext();
}

static void ProbeRequestedInventory() {
  // These are the user-facing integrations requested for the cumulative pass.
  // Native controls, the Generate page and direct editors use the same lookup.
  const char *requested[]{
      "gRandoSettings.HyliasGrace",
      "gRandoSettings.HyliasGraceRewards",
      "gRando.Options.RO_HYLIAS_GRACE",
      "gRando.Options.RO_HYLIAS_GRACE_REWARDS",
      "gRandoSettings.RodOfSeasons",
      "gRando.Options.RO_ROD_OF_SEASONS",
      "gRando.Options.RO_STARTING_ROD_OF_SEASONS",
      "gEnhancements.Graphics.OotLocationTitleCards",
      "gEnhancements.MidnaCompanionMM",
      "gEnhancements.MidnaCompanion",
      "gEnhancements.ChestStyleMatchesContentsMM",
      "gEnhancements.ChestSizeMatchesContentsMM",
      "gCheats.DropsDontDie",
      "gEnhancements.DinFireSword",
      "gRandoSettings.SkijerCustomItems",
      "gRandoSettings.ShuffleBombArrows",
      "gRandoSettings.ElementalWandShuffle",
      "gRando.Options.RO_SHUFFLE_NEI_ITEMS",
      "gRando.Options.RO_SHUFFLE_OOT_GEAR",
      "gRando.Options.RO_SHUFFLE_OOT_EQUIPMENT",
      "gRando.Options.RO_SHUFFLE_OOT_QUEST",
      "gRando.Options.RO_SHUFFLE_OOT_MASKS",
      "gRando.Options.RO_SHUFFLE_BOMB_ARROWS",
      "gRando.Options.RO_ELEMENTAL_WAND_SHUFFLE",
      "gRando.Options.RO_CROSSOVER_POKEBALL",
      "gRando.Options.RO_CROSSOVER_MARIO_MASK",
      "gRando.Options.RO_MAPS_COMPASSES_GIVE_INFORMATION",
      "gMods.TransformMasks.ChildDungeonGates",
      "gMods.TransformMasks.OotGoronZora",
      "gMods.WolfLink.UseHDModel",
      "gEnhancements.Fixes.MMStreamedPositionalAudio",
      "gRandoSettings.MQDungeons",
      "gRandoSettings.MQDungeonCount",
      "gRandoSettings.MQDungeonsSelection",
      "gCombo.Rando.CrossDungeonEntrances",
      "gCombo.Rando.ExpandedDungeonPool",
      "gCombo.Rando.CrossBossEntrances",
      "gCombo.Rando.CrossInteriorEntrances",
      "gCombo.Rando.CrossShopEntrances",
      "gCombo.Rando.CrossDoorInteriorEntrances",
      "gCombo.Rando.CrossGrottoEntrances",
      "gCombo.Rando.BlackMarketPricing",
      "gCombo.Rando.BlackMarketDungeonUnlocks",
      "gCombo.BlackMarket.RegularGrannyInterior",
      "gCombo.BlackMarket.AnimatedItemSeals",
      "gCheats.UnrestrictedItems",
      "gEnhancements.Graphics.SpinStyle",
      "gEnhancements.Graphics.ElementalSpinEffects",
      "gEnhancements.Graphics.UseOotSkyTextures",
      "gEnhancements.HideBackEquipment",
      "gEnhancements.BottleShimmer",
      "gEnhancements.DungeonItemColors",
      "gEnhancements.SkijerNEI.UseVanillaHeldSwords",
      "gAudioEditor.HyruleFieldNightMusic",
      "gAudioEditor.HyruleFieldNightSequence",
      "gAudioEditor.SeasonWeather.Season",
      "gAudioEditor.SeasonWeather.Spring.RainMode",
      "gAudioEditor.SeasonWeather.Spring.Blossoms",
      "gAudioEditor.SeasonWeather.Summer.Enabled",
      "gAudioEditor.SeasonWeather.Summer.Fluff",
      "gAudioEditor.SeasonWeather.Summer.Fireflies",
      "gAudioEditor.SeasonWeather.Summer.Sunbeams",
      "gAudioEditor.SeasonWeather.Autumn.RainMode",
      "gAudioEditor.SeasonWeather.Autumn.Leaves",
      "gAudioEditor.SeasonWeather.Winter.Snow",
      "gAudioEditor.SeasonWeather.Winter.Flurries",
      "gAudioEditor.SeasonWeather.Winter.Mist",
      "gAudioEditor.SeasonWeather.Winter.Aurora",
      "gAudioEditor.GlobalOutdoorRain",
      "gAudioEditor.GlobalOutdoorRainMode",
      "gAudioEditor.GlobalOutdoorRainColor",
      "gAudioEditor.GlobalOutdoorRainOvercast",
      "gAudioEditor.WeatherAudioDiagnostics",
      "gAudioEditor.ProximityWeatherThunder",
      "gAudioEditor.ProximityWeatherThunderStyle",
      "gAudioEditor.ProximityWeatherRainVolume",
      "gAudioEditor.ProximityWeatherThunderVolume",
      "gAudioEditor.ProximityWeatherThunderFrequency",
      "gAudioEditor.MMWeather.Enabled",
      "gAudioEditor.MMWeather.Mode",
      "gAudioEditor.MMWeather.RainColor",
      "gAudioEditor.MMWeather.Overcast",
      "gAudioEditor.MMWeather.Thunder",
      "gAudioEditor.MMWeather.RainVolume",
      "gAudioEditor.MMWeather.ThunderVolume",
      "gAudioEditor.MMWeather.ThunderFrequency",
      "gAudioEditor.MMWeather.Season",
      "gAudioEditor.MMWeather.Spring.RainMode",
      "gAudioEditor.MMWeather.Spring.Blossoms",
      "gAudioEditor.MMWeather.Autumn.RainMode",
      "gAudioEditor.MMWeather.Autumn.Leaves",
      "gAudioEditor.MMWeather.Winter.Snow",
      "gAudioEditor.MMWeather.Winter.Flurries",
      "gAudioEditor.MMWeather.Winter.Mist",
      "gAudioEditor.MMWeather.Winter.Aurora",
      "gEnhancements.Graphics.SummerAtmosphere.Enabled",
      "gEnhancements.Graphics.SummerAtmosphere.Fluff",
      "gEnhancements.Graphics.SummerAtmosphere.Fireflies",
      "gEnhancements.Graphics.SummerAtmosphere.Sunbeams",
  };
  for (const auto *key : requested)
    Check(WhatsNew::Find(key), key);
  Check(!WhatsNew::Find("gMods.TransformMasks.InstantTransform"),
        "the existing Instant Transform switch is no longer a release change");
}

static void ProbeTabs() {
  ImGui::CreateContext();
  auto &io = ImGui::GetIO();
  io.IniFilename = nullptr;
  io.DisplaySize = ImVec2(900, 600);
  unsigned char *pixels;
  int width, height;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
  const auto frame = [&](bool enabled) {
    ImGui::NewFrame();
    ImGui::SetNextWindowSize(ImVec2(850, 300));
    ImGui::Begin("Tab probe", nullptr, ImGuiWindowFlags_NoSavedSettings);
    ImGui::BeginTabBar("AudioTabs");
    const int colors = GImGui->ColorStack.Size;
    const ImVec4 normal = ImGui::GetStyleColorVec4(ImGuiCol_Text);
    if (WhatsNew::BeginTabItem("Weather & Seasons", enabled)) {
      Check(GImGui->ColorStack.Size == colors,
            "a tab accent must be restored before drawing unrelated contents");
      const auto after = ImGui::GetStyleColorVec4(ImGuiCol_Text);
      Check(after.x == normal.x && after.y == normal.y && after.z == normal.z &&
                after.w == normal.w,
            "tab contents keep their native text color");
      ImGui::EndTabItem();
    }
    auto *bar = GImGui->CurrentTabBar;
    Check(bar->LastTabItemIdx >= 0, "the native tab is registered");
    const ImGuiID id = bar->Tabs[bar->LastTabItemIdx].ID;
    Check(bar->SelectedTabId == 0 || bar->SelectedTabId == id,
          "the weather tab stays selected after native first-frame selection");
    Check(GImGui->ColorStack.Size == colors,
          "tab styling must balance the stack");
    ImGui::EndTabBar();
    ImGui::End();
    ImGui::Render();
    return id;
  };
  frame(false);
  const auto original = frame(false);
  Check(
      frame(true) == original && frame(false) == original,
      "What's New must preserve tab identity and selection when toggled live");
  for (const auto *label :
       {"Weather & Seasons", "Spring", "Summer", "Autumn", "Winter"})
    Check(WhatsNew::Find(nullptr, label),
          "every weather and season tab has an explicit release identity");
  ImGui::DestroyContext();
}

int main() {
  try {
    ProbeRequestedInventory();
    ProbeTabs();
    std::set<std::string> keys;
    for (const auto &entry : WhatsNew::kControls) {
      Check(keys.insert(entry.key).second && entry.note[0],
            "release keys must be unique and explain their change");
      Check(WhatsNew::Find(entry.key) == &entry,
            "every release key must resolve exactly");
    }
    Check(!WhatsNew::Find(nullptr) && !WhatsNew::Find("") &&
              !WhatsNew::Find("gSettings.Menu.Theme"),
          "unrelated and missing keys keep normal styling");
    Check(!WhatsNew::Find("gEnhancements.AlwaysThawedZorasDomain.Unrelated"),
          "release lookup is not a prefix match");
    Check(!WhatsNew::Find("gSettings.Menu.Theme", "Black Market Appearance"),
          "an unrelated CVar owns its identity");
    Check(!WhatsNew::Find(nullptr, "Black Market Appearance"),
          "the custom appearance container delegates release markers to its "
          "individual controls");
    Probe([](const char *label, bool *value, bool disabled) {
      OotWidgets::CheckboxOptions options;
      options.disabled = disabled;
      options.tooltip = "Native setting help";
      return OotWidgets::Checkbox(label, value, options);
    });
    Probe([](const char *label, bool *value, bool disabled) {
      MmWidgets::CheckboxOptions options;
      options.disabled = disabled;
      options.tooltip = "Native setting help";
      return MmWidgets::Checkbox(label, value, options);
    });
    Probe([](const char *label, bool *value, bool disabled) {
      ImGui::BeginDisabled(disabled);
      const bool changed = ImGui::Checkbox(label, value);
      ImGui::EndDisabled();
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Native setting help");
      return changed;
    });
    std::cout
        << "PASS: release inventory, native OoT/MM and combo fallback IDs, "
           "input, disabled state, layout, hover and style restoration\n";
  } catch (const std::exception &error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
}
