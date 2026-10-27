// The actual Generate/appearance expressions and callbacks are extracted into
// includes. Only config storage, pack availability and saving are fixtures.
#include "gui/WhatsNew.h"
#include "BlackMarketRoomMagic.h"
#include <imgui_internal.h>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace Fixture {
struct Row {
    std::string label;
    ImGuiID id;
    ImGuiID originalId;
    ImVec2 min, max;
    ImVec4 text;
    bool disabled;
    bool changed;
    int firstVertex, lastVertex;
};
struct FrameResult {
    std::vector<Row> rows;
    int markerVertices = 0;
};
std::map<std::string, int> config;
std::vector<std::pair<std::string, int>> writes;
std::vector<Row> rows;
bool grannyAvailable = true;
int saves = 0, failures = 0;
void Check(bool condition, const std::string& why) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << why << '\n';
    }
}
bool SameColor(const ImVec4& first, const ImVec4& second) {
    return first.x == second.x && first.y == second.y && first.z == second.z && first.w == second.w;
}
} // namespace Fixture

int CVarGetInteger(const char* key, int fallback) {
    const auto found = Fixture::config.find(key);
    return found == Fixture::config.end() ? fallback : found->second;
}
void CVarSetInteger(const char* key, int value) {
    Fixture::writes.emplace_back(key, value);
    Fixture::config[key] = value;
}
const char* CVarGetString(const char*, const char* fallback) { return fallback; }
namespace Ship {
struct Context {
    static Context* GetRawInstance() { static Context context; return &context; }
    Context* GetWindow() { return this; }
    Context* GetGui() { return this; }
    void SaveConsoleVariablesNextFrame() { ++Fixture::saves; }
};
} // namespace Ship
namespace BlackMarketGranny {
constexpr const char* kCvar = "gCombo.BlackMarket.RegularGrannyInterior";
constexpr const char* kErrorCvar = "gCombo.BlackMarket.GrannyLoadError";
bool DependenciesAvailable() { return Fixture::grannyAvailable; }
} // namespace BlackMarketGranny
namespace BlackMarketVisual {
constexpr const char* kSealsCvar = "gCombo.BlackMarket.AnimatedItemSeals";
}
namespace ComboRando {
void ComboMenu_PushCheckbox(int) {}
void ComboMenu_PopCheckbox() {}
void ComboMenu_PushCombobox(int) {}
void ComboMenu_PopCombobox() {}
} // namespace ComboRando

namespace ImGui {
template <typename Draw> bool Observe(const char* label, Draw draw) {
    Fixture::Row row{};
    row.label = label;
    row.originalId = GetID(label);
    row.text = GetStyleColorVec4(ImGuiCol_Text);
    row.disabled = (GImGui->CurrentItemFlags & ImGuiItemFlags_Disabled) != 0;
    row.firstVertex = GetWindowDrawList()->VtxBuffer.Size;
    row.changed = draw();
    row.id = GetItemID();
    row.min = GetItemRectMin();
    row.max = GetItemRectMax();
    row.lastVertex = GetWindowDrawList()->VtxBuffer.Size;
    Fixture::rows.push_back(row);
    return row.changed;
}
bool ObserveCheckbox(const char* label, bool* value) {
    return Observe(label, [&]() { return Checkbox(label, value); });
}
bool ObserveCombo(const char* label, int* current, const char* const items[], int count) {
    return Observe(label, [&]() { return Combo(label, current, items, count); });
}
} // namespace ImGui

// These macros affect only the extracted call sites; the observers above call
// Dear ImGui's real widgets and preserve their return values for real callbacks.
#define Checkbox ObserveCheckbox
#define Combo ObserveCombo
namespace BlackMarketUI {
#include "black_market_appearance.inc"
}
static void DrawGenerateControls() {
    [[maybe_unused]] const int goalTheme = 0;
#include "generate_controls.inc"
}
#undef Combo
#undef Checkbox

namespace Fixture {
constexpr const char* labels[] = {
    "Cross-Game Dungeon Entrances", "Expanded Dungeon Pool", "Cross-Game Boss Entrances",
    "Cross-Game Interiors", "Cross-Game Shops", "Cross-Game Door Interiors", "Cross-Game Grottos and Graves",
    "Pricing##BlackMarket", "Dungeon Purchase Unlocks", "Regular Granny Interior", "Magical Room Materials",
    "Animated Item Seals",
};
constexpr const char* bossKey = "gCombo.Rando.CrossBossEntrances";
const Row& Find(const FrameResult& frame, const char* label) {
    const auto row = std::find_if(frame.rows.begin(), frame.rows.end(), [label](const Row& current) {
        return current.label == label;
    });
    if (row == frame.rows.end())
        throw std::runtime_error(std::string("production control missing: ") + label);
    return *row;
}

FrameResult Frame(ImVec2 mouse = ImVec2(-100, -100), bool down = false) {
    auto& io = ImGui::GetIO();
    io.AddMousePosEvent(mouse.x, mouse.y);
    io.AddMouseButtonEvent(0, down);
    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(20, 20));
    ImGui::SetNextWindowSize(ImVec2(1000, 860));
    ImGui::Begin("Generate release controls", nullptr,
                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar);
    rows.clear();
    const ImVec4 normal = ImGui::GetStyleColorVec4(ImGuiCol_Text);
    const int colors = GImGui->ColorStack.Size;
    DrawGenerateControls();
    Check(GImGui->ColorStack.Size == colors && SameColor(ImGui::GetStyleColorVec4(ImGuiCol_Text), normal),
          "production controls restore the real text/style stack");
    FrameResult result{rows};
    const auto* list = ImGui::GetWindowDrawList();
    const ImU32 goldRgb = ImGui::ColorConvertFloat4ToU32(WhatsNew::Accent(WhatsNew::Kind::New)) & 0x00FFFFFFu;
    // Native widget vertices are excluded, leaving only release marker glyphs.
    // RGB ignores native disabled alpha while still requiring the gold accent.
    for (int vertex = 0; vertex < list->VtxBuffer.Size; ++vertex) {
        const bool native = std::any_of(rows.begin(), rows.end(), [vertex](const Row& row) {
            return vertex >= row.firstVertex && vertex < row.lastVertex;
        });
        if (!native && (list->VtxBuffer[vertex].col & 0x00FFFFFFu) == goldRgb)
            ++result.markerVertices;
    }
    ImGui::End();
    ImGui::Render();
    return result;
}

void Seed(bool highlights, bool gated = false) {
    config = {
        {WhatsNew::kEnabledCvar, highlights},
        {"gCombo.Rando.CrossDungeonEntrances", !gated},
        {"gCombo.Rando.CrossInteriorEntrances", gated},
        {"unrelated-sentinel", 73},
    };
    writes.clear();
    saves = 0;
    grannyAvailable = !gated;
}

void CheckFrame(const FrameResult& frame, bool highlighted, const FrameResult& ordinary,
                std::initializer_list<const char*> disabledLabels = {}) {
    const ImVec4 normal = ImGui::GetStyleColorVec4(ImGuiCol_Text);
    Check(frame.rows.size() == std::size(labels), "all production controls render exactly once");
    for (const auto* label : labels) {
        const auto& row = Find(frame, label);
        const auto count = std::count_if(frame.rows.begin(), frame.rows.end(), [label](const Row& current) {
            return current.label == label;
        });
        Check(count == 1, std::string(label) + " renders once");
        const bool disabled = std::any_of(disabledLabels.begin(), disabledLabels.end(), [label](const char* gated) {
            return std::string(label) == gated;
        });
        Check(row.disabled == disabled, std::string(label) + " has its expected native input gate");
        const auto expected = highlighted && !disabled ? WhatsNew::Accent(WhatsNew::Kind::New) : normal;
        Check(SameColor(row.text, expected), std::string(label) + " has its expected native text accent");
        Check(row.id == row.originalId && row.id == Find(ordinary, label).id,
              std::string(label) + " retains its original native ID");
        Check(row.min.x < row.max.x && row.min.y < row.max.y, std::string(label) + " has a usable native input rect");
        const auto& baseline = Find(ordinary, label);
        Check(row.min.x == baseline.min.x && row.min.y == baseline.min.y &&
                  row.max.x == baseline.max.x && row.max.y == baseline.max.y,
              std::string(label) + " keeps its complete native rectangle with highlights on");
    }
    Check(frame.markerVertices == 0, "color-only highlights never submit extra gold marker glyphs");
}

void Click(const char* label) {
    const auto frame = Frame();
    const auto& row = Find(frame, label);
    const ImVec2 hit(row.min.x + 7, (row.min.y + row.max.y) * 0.5f);
    Frame(hit);
    Frame(hit, true);
    Frame(hit, false);
}

void Probe() {
    Seed(false);
    Frame();
    const auto ordinary = Frame();
    CheckFrame(ordinary, false, ordinary);
    const auto initial = config;
    config[WhatsNew::kEnabledCvar] = 1;
    const auto highlighted = Frame();
    CheckFrame(highlighted, true, ordinary);
    config[WhatsNew::kEnabledCvar] = 0;
    const auto restored = Frame();
    CheckFrame(restored, false, ordinary);
    Check(config == initial && writes.empty() && saves == 0,
          "drawing/toggling highlights preserves config and performs no CVar writes or saves");
    for (const auto* label : labels) {
        const auto& before = Find(ordinary, label);
        const auto& after = Find(restored, label);
        Check(before.min.x == after.min.x && before.min.y == after.min.y && before.max.x == after.max.x,
              std::string(label) + " restores ordinary geometry when highlights turn off");
    }
    // A missing production scope must fail before input checks can obscure it.
    if (failures)
        return;

    Seed(true, true);
    config[WhatsNew::kEnabledCvar] = 0;
    const auto ordinaryGated = Frame();
    config[WhatsNew::kEnabledCvar] = 1;
    const auto gated = Frame();
    CheckFrame(gated, true, ordinaryGated,
               {"Expanded Dungeon Pool", "Cross-Game Door Interiors", "Regular Granny Interior"});
    Check(Find(gated, "Expanded Dungeon Pool").disabled, "expanded pool is disabled while dungeon shuffle is off");
    Check(Find(gated, "Cross-Game Door Interiors").disabled, "door interiors are disabled by broad interiors");
    Check(Find(gated, "Regular Granny Interior").disabled, "unavailable unsaved Granny interior stays disabled");
    const auto untouched = config;
    for (const auto* label : {"Expanded Dungeon Pool", "Cross-Game Door Interiors", "Regular Granny Interior"})
        Click(label);
    Check(config == untouched && writes.empty() && saves == 0,
          "clicking gated controls cannot write CVars or schedule saves");

    Seed(true);
    Click("Cross-Game Boss Entrances");
    Check(writes == std::vector<std::pair<std::string, int>>{{bossKey, 1}} && config[bossKey] == 1 && saves == 0,
          "a real highlighted boss-entrance click performs its one original CVar write");
    writes.clear();
    Click("Animated Item Seals");
    Check(writes == std::vector<std::pair<std::string, int>>{{BlackMarketVisual::kSealsCvar, 0}} && saves == 1,
          "a real animated-seals click toggles the default-on value and schedules exactly one save");
    Check(config["unrelated-sentinel"] == 73, "native callbacks preserve unrelated settings");

    Seed(true, true);
    config[BlackMarketGranny::kCvar] = 1;
    config[WhatsNew::kEnabledCvar] = 0;
    const auto ordinarySaved = Frame();
    config[WhatsNew::kEnabledCvar] = 1;
    const auto saved = Frame();
    CheckFrame(saved, true, ordinarySaved, {"Expanded Dungeon Pool", "Cross-Game Door Interiors"});
    Check(!Find(saved, "Regular Granny Interior").disabled, "saved Granny remains editable after its pack is removed");
    Click("Regular Granny Interior");
    Check(writes == std::vector<std::pair<std::string, int>>{{BlackMarketGranny::kCvar, 0}} && saves == 1,
          "a saved unavailable Granny preference can be turned off with one write/save");
    Check(Find(Frame(), "Regular Granny Interior").disabled, "Granny becomes disabled after clearing its saved preference");
    Click("Regular Granny Interior");
    Check(writes.size() == 1 && saves == 1, "disabled Granny cannot be switched back on without its resources");
}
} // namespace Fixture

int main() {
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = ImVec2(1100, 900);
    io.DeltaTime = 1.0f / 60;
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    try {
        Fixture::Probe();
    } catch (const std::exception& error) {
        Fixture::Check(false, error.what());
    }
    ImGui::DestroyContext();
    if (Fixture::failures)
        return 1;
    std::cout << "PASS real ImGui Generate/Black Market: 12 color-only accents, stable IDs/on-off geometry, no draw writes, gated input, boss toggle, seals save and unavailable Granny reset\n";
}
