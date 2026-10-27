// Native option registration, save option migration and production information
// gate. Config/SoH service calls are boundary observers; native save types are real.
#include <nlohmann/json.hpp>
#include "Rando/Rando.h"
#include "Rando/ItemReceiptText.h"
#include "combo/ComboGameMode.h"
#include "combo/gui/WhatsNew.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <map>
#include <vector>
#include <imgui.h>

extern "C" {
SaveContext gSaveContext{};
#ifdef COMBO_BUILD
int gComboGameMode = COMBO_GAME_BOTH;
#endif
}
static bool donorEnabled;
static int donorCalls;
static std::map<std::string, int> config;
static std::vector<std::string> controls;
namespace ImGui {
// This fixture observes native hint routing; styling/input uses real Dear ImGui
// in tests/whats_new.
void PushStyleColor(ImGuiCol, const ImVec4&) {}
void PopStyleColor(int) {}
ImVec2 GetContentRegionAvail() { return {1000, 700}; }
ImGuiStyle& GetStyle() { static ImGuiStyle style; return style; }
bool BeginChild(const char*, const ImVec2&, ImGuiChildFlags, ImGuiWindowFlags) { return true; }
void EndChild() {}
}
ImGuiStyle::ImGuiStyle() { ItemSpacing = {}; }
struct CheckboxOptions {
    struct Data {
        const char* tooltip = nullptr;
        bool disabled = false;
        const char* disabledTooltip = nullptr;
    } data;
    CheckboxOptions& DefaultValue(bool) { return *this; }
};
struct IntSliderOptions {
    IntSliderOptions& Min(int) { return *this; }
    IntSliderOptions& Max(int) { return *this; }
    IntSliderOptions& DefaultValue(int) { return *this; }
    IntSliderOptions& Tooltip(const char*) { return *this; }
};
static bool CVarCheckbox(const char*, const char* key, const CheckboxOptions&) {
    if (key) controls.emplace_back(key);
    return false;
}
static void CVarSliderInt(const char*, const char*, const IntSliderOptions&) {}
static int DonorGate() { ++donorCalls; return donorEnabled; }
extern "C" void* Combo_ResolveSym(const char*, const char*) { return reinterpret_cast<void*>(&DonorGate); }
extern "C" int CVarGetInteger(const char* key, int fallback) {
    auto value = config.find(key); return value == config.end() ? fallback : value->second;
}
/* INFORMATION_GATE */
/* NATIVE_HINT_MENU */
static void LoadOptions(const nlohmann::json& j, RandoSaveInfo& rando) {
    /* SAVE_OPTIONS_LOAD */
}
static int failures;
static void Check(bool ok, const char* message) {
    if (!ok) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}
int main() {
    int option = RO_MAX;
    for (const auto& [id, descriptor] : Rando::StaticData::Options)
        if (!std::strcmp(descriptor.name, "RO_MAPS_COMPASSES_GIVE_INFORMATION")) {
            option = id;
            Check(!std::strcmp(descriptor.cvar, "gRando.Options.RO_MAPS_COMPASSES_GIVE_INFORMATION") &&
                      descriptor.defaultValue == 0, "native information option has its own stable key and default Off");
        }
    Check(option != RO_MAX, "solo-MM information is a registered seed option");
    controls.clear();
#ifdef COMBO_BUILD
    gComboGameMode = COMBO_GAME_MM;
#endif
    DrawHintsTab();
    Check(std::count(controls.begin(), controls.end(), "gRando.Options.RO_MAPS_COMPASSES_GIVE_INFORMATION") == 1,
          "native solo-MM hints menu exposes the registered option once");
#ifdef COMBO_BUILD
    controls.clear(); gComboGameMode = COMBO_GAME_BOTH;
    DrawHintsTab();
    Check(std::count(controls.begin(), controls.end(), "gRando.Options.RO_MAPS_COMPASSES_GIVE_INFORMATION") == 0,
          "combined MM menu defers to the shared information widget");
#endif
    gSaveContext.fileNum = 0;
    gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
#ifdef COMBO_BUILD
    gComboGameMode = COMBO_GAME_MM;
#endif
    donorEnabled = true;
    Check(!Rando::MapCompassInfoEnabled(), "solo-MM defaults Off despite an enabled dormant SoH context");
    if (option != RO_MAX) {
        Check(option == RO_STARTING_ROD_OF_SEASONS + 1, "append new option without renumbering existing saved IDs");
        RANDO_SAVE_OPTIONS[option] = 1;
    }
    donorEnabled = false;
    const auto calls = donorCalls;
    Check(Rando::MapCompassInfoEnabled(), "solo-MM uses its enabled saved seed without SoH");
    Check(donorCalls == calls, "solo-MM never probes the dormant donor gate");
    config["gRando.Options.RO_MAPS_COMPASSES_GIVE_INFORMATION"] = 0;
    Check(Rando::MapCompassInfoEnabled(), "editing next-seed config cannot change the loaded information rule");
    if (option != RO_MAX) {
        auto saved = std::vector<u32>(RANDO_SAVE_OPTIONS, RANDO_SAVE_OPTIONS + RO_MAX);
        RANDO_SAVE_OPTIONS[option] = 0;
        LoadOptions({{"randoSaveOptions", saved}}, gSaveContext.save.shipSaveInfo.rando);
        Check(Rando::MapCompassInfoEnabled(), "native saved information rule survives actual option-array reload");
        saved.resize(option);
        LoadOptions({{"randoSaveOptions", saved}}, gSaveContext.save.shipSaveInfo.rando);
        Check(!Rando::MapCompassInfoEnabled(), "loading an older short option array clears the newer rule");
        RANDO_SAVE_OPTIONS[option] = 1;
    }
#ifdef COMBO_BUILD
    gComboGameMode = COMBO_GAME_BOTH;
    donorEnabled = false;
    Check(!Rando::MapCompassInfoEnabled(), "combined seed retains authoritative SoH Off despite native MM On");
    donorEnabled = true;
    Check(Rando::MapCompassInfoEnabled(), "combined seed retains authoritative SoH On");
#endif
    gSaveContext.fileNum = 0xFF;
    Check(!Rando::MapCompassInfoEnabled(), "invalid slot suppresses information in every mode");
    gSaveContext.fileNum = 0;
    gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_VANILLA;
    Check(!Rando::MapCompassInfoEnabled(), "vanilla saves do not inherit randomizer information");
    if (!failures) std::cout << "PASS native MM information rule: solo/combined authority, saved reload and legacy Off\n";
    return failures != 0;
}
