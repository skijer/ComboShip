#pragma once

// One renderer is compiled privately into the shared menu and both native menus.
// No game DLL needs to import a C++ UI function from comboui.
#include "EmoteCatalog.h"
#include "../gui/WhatsNew.h"
#include <imgui.h>
#include <libultraship/bridge/consolevariablebridge.h>
#include <string>

namespace ComboEmotes {
namespace SettingsDetail {
struct Button {
    const char* name;
    uint32_t bit;
};
inline constexpr Button kButtons[]{ { "M1", 0x40 },      { "M2", 0x80 },       { "D-Up", 0x800 }, { "D-Down", 0x400 },
                                    { "D-Left", 0x200 }, { "D-Right", 0x100 }, { "A", 0x8000 },   { "B", 0x4000 },
                                    { "Z", 0x2000 },     { "L", 0x20 },        { "R", 0x10 },     { "C-Up", 8 },
                                    { "C-Down", 4 },     { "C-Left", 2 },      { "C-Right", 1 } };
inline std::string BindingName(uint32_t mask) {
    std::string label;
    for (const auto& button : kButtons) {
        if (mask & button.bit) {
            if (!label.empty())
                label += " + ";
            label += button.name;
        }
    }
    return label.empty() ? "Unbound" : label;
}
} // namespace SettingsDetail

inline void DrawSettings() {
    using namespace SettingsDetail;
    bool changed = false;
    {
        WhatsNew::Scope releaseHighlight("gCombo.Emotes.Enabled", "Emotes",
                                         CVarGetInteger(WhatsNew::kEnabledCvar, 1) != 0);
        ImGui::SeparatorText("Emotes");
    }
    bool enabled = CVarGetInteger("gCombo.Emotes.Enabled", 1) != 0;
    if (ImGui::Checkbox("Enable emotes", &enabled)) {
        CVarSetInteger("gCombo.Emotes.Enabled", enabled);
        if (!enabled)
            CVarSetInteger("gCombo.Emotes.Request", -1);
        changed = true;
    }
    ImGui::TextWrapped(
        "Choose up to four shortcuts. Map M1/M2 and the game buttons to your keyboard or controller in Controls.");
    ImGui::TextWrapped(
        "Emotes play while standing on the ground as human Link. Move or press an action button to stop.");
    for (size_t i = 0; i < kSlots; ++i) {
        ImGui::PushID(static_cast<int>(i));
        ImGui::Separator();
        ImGui::Text("Slot %d", static_cast<int>(i + 1));
        int id = CVarGetInteger(kSlotKeys[i], kDefaultEmotes[i]);
        const auto* selected = Find(id);
        ImGui::SetNextItemWidth(260);
        if (ImGui::BeginCombo("Animation", selected ? selected->name : "None")) {
            if (ImGui::Selectable("None", !selected)) {
                id = 0;
                changed = true;
            }
            for (const auto& entry : kCatalog) {
                if (ImGui::Selectable(entry.name, id == entry.id)) {
                    id = entry.id;
                    changed = true;
                }
            }
            ImGui::EndCombo();
            CVarSetInteger(kSlotKeys[i], id);
        }
        uint32_t mask = static_cast<uint32_t>(CVarGetInteger(kChordKeys[i], kDefaultChords[i]));
        ImGui::SetNextItemWidth(260);
        const std::string binding = BindingName(mask);
        if (ImGui::BeginCombo("Shortcut", binding.c_str(), ImGuiComboFlags_HeightLarge)) {
            bool bindingChanged = false;
            if (ImGui::Selectable("Clear shortcut", mask == 0)) {
                mask = 0;
                bindingChanged = true;
            }
            for (const auto& button : kButtons) {
                bool checked = (mask & button.bit) != 0;
                if (ImGui::Checkbox(button.name, &checked)) {
                    mask = checked ? mask | button.bit : mask & ~button.bit;
                    bindingChanged = true;
                }
            }
            if (bindingChanged) {
                CVarSetInteger(kChordKeys[i], static_cast<int32_t>(mask));
                changed = true;
            }
            ImGui::EndCombo();
        }
        for (size_t earlier = 0; mask && earlier < i; ++earlier) {
            if (mask == static_cast<uint32_t>(CVarGetInteger(kChordKeys[earlier], kDefaultChords[earlier])))
                ImGui::TextWrapped("This shortcut is also assigned to slot %d. Choose a different shortcut.",
                                   static_cast<int>(earlier + 1));
        }
        if (selected) {
            if (ImGui::Button("Play"))
                CVarSetInteger("gCombo.Emotes.Request", id);
            ImGui::SameLine();
            ImGui::TextDisabled("%.1f seconds", selected->frames / 20.0);
        }
        ImGui::PopID();
    }
    ImGui::Separator();
    if (ImGui::Button("Stop emote"))
        CVarSetInteger("gCombo.Emotes.Request", -1);
    ImGui::TextWrapped("If gameplay is paused, close the menu to play a queued emote.");
    float volume = CVarGetFloat("gCombo.Emotes.Volume", 0.8f);
    ImGui::SetNextItemWidth(260);
    if (ImGui::SliderFloat("Emote sound volume", &volume, 0.0f, 1.0f, "%.2f")) {
        CVarSetFloat("gCombo.Emotes.Volume", volume);
        changed = true;
    }
    if (ImGui::CollapsingHeader("Animation credits")) {
        for (const auto& entry : kCatalog)
            ImGui::TextWrapped("%s — %s", entry.name, entry.author);
    }
    if (changed)
        CVarSave();
}
} // namespace ComboEmotes
