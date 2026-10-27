// Execute the complete shared menu builder. Rendering observes navigation and
// the original exported widgets; native widget callbacks remain game-owned.
#include <algorithm>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <imgui.h>
#include "gui/ComboMenuModel.h"

static std::string currentId;
static std::vector<std::string> navigation;
static std::vector<int> rendered;
static const ComboRando::GameMenu* renderedOwner;
namespace ImGui {
ImVec2 GetContentRegionAvail() { return {1000, 700}; }
ImGuiStyle& GetStyle() { static ImGuiStyle style; return style; }
ImVec2 CalcTextSize(const char*, const char*, bool, float) { return {100, 20}; }
bool BeginChild(const char*, const ImVec2&, ImGuiChildFlags, ImGuiWindowFlags) { return true; }
void EndChild() {}
void SetWindowFontScale(float) {}
void SeparatorText(const char*) {}
void TextUnformatted(const char*, const char*) {}
void TextDisabled(const char*, ...) {}
void TextWrapped(const char*, ...) {}
void PushID(const char* id) { currentId = id; }
void PopID() {}
void SameLine(float, float) {}
void Dummy(const ImVec2&) {}
ImVec2 GetCursorScreenPos() { return {}; }
ImDrawList* GetWindowDrawList() { static ImDrawList list(nullptr); return &list; }
void MemFree(void* pointer) { std::free(pointer); }
}
ImGuiStyle::ImGuiStyle() { ScrollbarSize = 0; ItemSpacing = {}; }
ImDrawList::ImDrawList(ImDrawListSharedData*) {}
ImDrawList::~ImDrawList() {}
void ImDrawListSplitter::ClearFreeMemory() {}
void ImDrawList::AddRectFilled(const ImVec2&, const ImVec2&, ImU32, float, ImDrawFlags) {}
namespace ComboEmotes { void DrawSettings() {} }
namespace ComboSync { void DrawSyncSharedPanel() {} }
namespace ComboRando {
ComboMenuModel& ComboMenuModel::Get() { static ComboMenuModel model; return model; }
void ComboMenuModel::EnsureLoaded() {}
bool ComboMenu_StyledSidebarEntry(const char*, bool, float) { navigation.push_back(currentId); return false; }
void RenderWidget(const CwWidget& widget, const GameMenu& owner, int = 90) {
    rendered.push_back(widget.index);
    renderedOwner = &owner;
}
void RenderSidebarWidgets(const CwSidebar& sidebar, const GameMenu& owner) {
    for (int i = 0; i < sidebar.widgetCount; ++i) RenderWidget(sidebar.widgets[i], owner);
}
void DrawComboPlandoPanel() {}
void DrawTrackerSharedPanel() {}
void DrawCheckTrackerSharedPanel() {}
void DrawHintTrackerSharedPanel() {}
void DrawTimersSharedPanel() {}
void DrawNetworkSharedPanel() {}
class ComboMenu {
  public:
    std::string mScope = "randomizer", mHubActive, mSearchQuery;
    void DrawSharedPanel();
    void DrawComboPanel() {}
    void DrawSearchResults(const std::string&) {}
};
/* MENU_HELPERS */
/* SHARED_PANEL */
}

static int failures;
static void Check(bool ok, const char* message) {
    if (!ok) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}
static bool Has(const char* key) {
    return std::find(navigation.begin(), navigation.end(), key) != navigation.end();
}
static void Draw(ComboRando::ComboMenu& menu, const char* active) {
    navigation.clear(); rendered.clear(); renderedOwner = nullptr;
    menu.mHubActive = active;
    menu.DrawSharedPanel();
}
int main() {
    using namespace ComboRando;
    CwWidget stats[2]{};
    stats[0].index = 101; stats[0].cvar = "gRandoSettings.QuarterHeart";
    stats[1].index = 102; stats[1].cvar = "gRandoSettings.SpeedUpgradeRequired";
    CwWidget hints[2]{};
    hints[0].index = 201; hints[0].cvar = "gRandoSettings.MapsCompassesGiveInformation";
    hints[1].index = 202; hints[1].cvar = "gRandoSettings.GossipStoneHints";
    CwSidebar ootSides[] = {{"Stat Upgrades", 2, stats, 2}, {"Hints/Traps", 2, hints, 2}};
    CwSection ootSection{"Randomizer", "", ootSides, 2};
    CwMenu ootMenu{1, &ootSection, 1};
    CwWidget mmWidget{}; mmWidget.index = 301;
    CwSidebar mmSide{"Hints", 1, &mmWidget, 1};
    CwSection mmSection{"Rando", "", &mmSide, 1};
    CwMenu mmMenu{1, &mmSection, 1};
    auto& model = ComboMenuModel::Get();
    auto& oot = const_cast<GameMenu&>(model.Oot());
    auto& mm = const_cast<GameMenu&>(model.Mm());
    oot.loaded = mm.loaded = true; oot.menu = &ootMenu; mm.menu = &mmMenu;
    ComboMenu menu;
    Draw(menu, "Shared Seed Settings/Stat Upgrades");
    Check(Has("Shared Seed Settings/Stat Upgrades"), "RPG controls belong to Shared Seed Settings");
    Check(!Has("OOT Randomizer/Stat Upgrades"), "RPG sidebar no longer claims OoT-only scope");
    Check(rendered == std::vector<int>({101, 102}) && renderedOwner == &oot,
          "shared RPG preserves both original native widgets and their owner");
    Draw(menu, "Shared Seed Settings/Map & Compass Information");
    Check(Has("Shared Seed Settings/Map & Compass Information"), "map/compass information has shared navigation");
    Check(rendered == std::vector<int>({201}) && renderedOwner == &oot,
          "shared information renders exactly the original native widget");
    Draw(menu, "OOT Randomizer/Hints/Traps");
    Check(rendered == std::vector<int>({202}) && renderedOwner == &oot,
          "OoT hints retain unrelated settings without a duplicate information control");
    Check(hints[0].index == 201 && ootSides[1].widgetCount == 2,
          "presentation filtering never mutates the exported native menu");
    Draw(menu, "MM Randomizer/Hints");
    Check(rendered == std::vector<int>({301}) && renderedOwner == &mm,
          "MM native hint sidebar keeps its original ownership");
    oot.loaded = false; oot.menu = nullptr;
    Draw(menu, "Shared Seed Settings/Map & Compass Information");
    Check(!Has("Shared Seed Settings/Map & Compass Information") && rendered == std::vector<int>({301}),
          "unavailable donor falls back to available navigation safely");
    if (!failures) std::cout << "PASS shared seed menu: native ownership, unique controls and unavailable-host recovery\n";
    return failures != 0;
}
