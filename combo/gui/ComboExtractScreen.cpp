// combo/gui/ComboExtractScreen.cpp
// ComboShip-owned ROM-extraction screen (see ComboExtractScreen.h).
//
// Why this lives in comboui.dll: the friendly progress UI needs a window + ImGui context, and the
// only window in the process is libultraship's, created by SOH_InitWindowOnly() (the OoT OTRGlobals
// ctor, from soh.o2r) before any ROM exists. comboui already drives ImGui against that shared context
// (ComboMenu.cpp), so it owns the frame loop here. The actual ROM validation + ZAPD work happens in
// soh.dll / 2ship.dll via the function pointers in ComboExtractCallbacks; this file only renders.
#include "ComboExtractScreen.h"
#include "ComboGameMode.h"
#include "ComboExport.h"
#include "ComboFilePicker.h"  // native Browse dialog (comdlg32 / pfd), shared with the settings-import screen
#include "ComboWidgetStyle.h" // ComboShip port style: ComboMenu_ThemeColor / PushButton / PopButton

#include <libultraship/libultraship.h>
#include <ship/window/FileDropMgr.h> // only forward-declared by the umbrella header
#include <fast/Fast3dWindow.h>
#include <imgui.h>

#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <cstdio>
#include <cctype>

using ComboRando::ComboMenu_PopButton;
using ComboRando::ComboMenu_PushButton;
using ComboRando::ComboMenu_ThemeColor; // port style helpers (combo/gui/ComboWidgetStyle.h)

namespace {

struct RomSlot {
    const char* label = "";
    bool needed = false; // archive missing -> must extract
    ComboFnValidateRom validate = nullptr;
    ComboFnValidateRom classify = nullptr; // header-only version check for the auto-scan
    ComboFnStartExtraction start = nullptr;
    ComboFnGetProgress progress = nullptr;
    std::string path;
    bool valid = false;
    int ootKind = COMBO_OOT_ROM_INVALID;
    int expectedOotKind = COMBO_OOT_ROM_INVALID; // zero accepts either OoT version
    // extraction runtime state
    bool started = false;
    bool done = false;
    bool success = false;
};

// Native open-file dialog filtered to N64 ROM extensions. Returns "" if cancelled.
std::string PickRomFile() {
    return ComboFilePicker::PickFile("Select ROM",
                                     "N64 ROMs (*.z64;*.n64;*.v64)\0*.z64;*.n64;*.v64\0All Files (*.*)\0*.*\0",
                                     { "N64 ROMs (*.z64 *.n64 *.v64)", "*.z64 *.n64 *.v64", "All Files", "*" });
}

// The UI disables Browse and points at drag & drop when no dialog helper exists (Linux only).
bool PickerAvailable() {
    return ComboFilePicker::Available();
}

// Dropped-file hand-off from the gfx backend's event pump (via FileDropMgr) to the PICK loop
// below. Same thread — the pump runs inside wnd->HandleEvents() — so a plain string suffices.
// Registered only while the extraction screen is up; returns true so FileDropMgr doesn't emit
// its "unsupported file" overlay for what is actually the expected input here.
std::string sDroppedRomPath;
bool HandleRomDrop(char* path) {
    if (path != nullptr) {
        sDroppedRomPath = path;
    }
    return true;
}

// Non-recursive scan of a directory for candidate ROM files.
void ScanForRoms(const std::filesystem::path& dir, std::vector<std::string>& out) {
    std::error_code ec;
    if (!std::filesystem::is_directory(dir, ec)) {
        return;
    }
    for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        if (ec) {
            break;
        }
        if (!entry.is_regular_file(ec)) {
            continue;
        }
        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return (char)std::tolower(c); });
        if (ext == ".z64" || ext == ".n64" || ext == ".v64") {
            out.push_back(entry.path().string());
        }
    }
}

// ImGui's GImGui is a per-module global; point comboui's at the shared libultraship context
// (same pattern as ComboMenu.cpp) before any ImGui call.
void UseSharedImGuiContext() {
    auto ctx = Ship::Context::GetRawInstance();
    if (ctx && ctx->GetWindow() && ctx->GetWindow()->GetGui()) {
        ImGui::SetCurrentContext(ctx->GetWindow()->GetGui()->GetImGuiContext());
    }
}

} // namespace

extern "C" COMBO_EXPORT int ComboUI_RunExtraction(const ComboExtractCallbacks* cb) {
    if (!cb) {
        return 0;
    }
    auto ctx = Ship::Context::GetRawInstance();
    if (!ctx || !ctx->GetWindow() || !ctx->GetWindow()->GetGui()) {
        return 0;
    }
    auto wnd = std::dynamic_pointer_cast<Fast::Fast3dWindow>(ctx->GetWindow());
    if (!wnd) {
        return 0;
    }
    auto gui = wnd->GetGui();
    UseSharedImGuiContext();

    constexpr int kSlotCount = 3;
    constexpr int kMmSlot = 2;
    RomSlot slots[kSlotCount];
    slots[0].label = "Ocarina of Time";
    const bool ootEnabled = cb->gameMode != COMBO_GAME_MM;
    slots[0].needed = ootEnabled && cb->sohNeeded != 0;
    slots[0].validate = cb->sohValidate;
    slots[0].classify = cb->sohClassify;
    slots[0].start = cb->sohStart;
    slots[0].progress = cb->sohProgress;
    slots[1].label = "Additional Ocarina of Time ROM";
    slots[1].validate = cb->sohValidate;
    slots[1].classify = cb->sohClassify;
    slots[1].start = cb->sohStart;
    slots[1].progress = cb->sohProgress;
    slots[kMmSlot].label = "Majora's Mask";
    slots[kMmSlot].needed = cb->gameMode != COMBO_GAME_OOT && cb->mmNeeded != 0;
    slots[kMmSlot].validate = cb->mmValidate;
    slots[kMmSlot].classify = cb->mmClassify;
    slots[kMmSlot].start = cb->mmStart;
    slots[kMmSlot].progress = cb->mmProgress;

    const bool bothOotPresent = cb->sohVanillaPresent && cb->sohMqPresent;
    auto updateComplement = [&]() {
        if (!ootEnabled || bothOotPresent) {
            return;
        }
        if (cb->sohVanillaPresent || slots[0].ootKind == COMBO_OOT_ROM_VANILLA) {
            slots[1].label = "Ocarina of Time: Master Quest";
            slots[1].expectedOotKind = COMBO_OOT_ROM_MQ;
        } else if (cb->sohMqPresent || slots[0].ootKind == COMBO_OOT_ROM_MQ) {
            slots[1].label = "Ocarina of Time: Vanilla";
            slots[1].expectedOotKind = COMBO_OOT_ROM_VANILLA;
        }
        if (slots[1].ootKind != slots[1].expectedOotKind) {
            slots[1].path.clear();
            slots[1].valid = false;
            slots[1].needed = false;
            slots[1].ootKind = COMBO_OOT_ROM_INVALID;
        }
    };
    updateComplement();

    // Browse performs full CRC validation; scans and drops stay header-only until
    // extraction validates the ROM again. A second copy of the same quest cannot
    // accidentally replace the complementary archive.
    auto selectRom = [&](int index, const std::string& path, bool fullValidation) {
        RomSlot& s = slots[index];
        s.path = path;
        ComboFnValidateRom check = fullValidation ? s.validate : (s.classify ? s.classify : s.validate);
        if (index != kMmSlot) {
            s.ootKind = cb->sohClassifyKind ? cb->sohClassifyKind(path.c_str()) : COMBO_OOT_ROM_INVALID;
            s.valid = (s.ootKind == COMBO_OOT_ROM_VANILLA || s.ootKind == COMBO_OOT_ROM_MQ) &&
                      (!s.expectedOotKind || s.ootKind == s.expectedOotKind) && check && check(path.c_str());
        } else {
            s.valid = check && check(path.c_str());
        }
        if (index == 0 && s.valid) {
            updateComplement();
        }
    };

    // Auto-scan the working directory and classify any ROMs found into the needed slots.
    {
        std::vector<std::string> candidates;
        ScanForRoms(std::filesystem::current_path(), candidates);
        std::sort(candidates.begin(), candidates.end());
        for (const auto& c : candidates) {
            for (int i = 0; i < kSlotCount; ++i) {
                auto& s = slots[i];
                const bool selectable = i == 1 ? s.expectedOotKind != COMBO_OOT_ROM_INVALID : s.needed;
                if (!selectable || s.valid) {
                    continue;
                }
                // Route by the header-only classify (fast); fall back to full validate if unavailable.
                ComboFnValidateRom check = s.classify ? s.classify : s.validate;
                if (check && check(c.c_str())) {
                    selectRom(i, c, false);
                    if (s.valid) {
                        break;
                    }
                }
            }
        }
        // An unmatched scan candidate is not a player selection or an error.
        for (auto& s : slots) {
            if (!s.valid) {
                s.path.clear();
            }
        }
    }

    // Selection is deliberately ordered: first OoT, optional other quest, then
    // MM. Keep installed archives out of the extraction queue.
    int pickSlot = !ootEnabled ? kMmSlot : (slots[0].needed ? 0 : (bothOotPresent ? kMmSlot : 1));
    auto advancePickSlot = [&]() {
        if (pickSlot == 1 && (bothOotPresent || !slots[1].expectedOotKind)) {
            ++pickSlot;
        }
        if (pickSlot == kMmSlot && !slots[kMmSlot].needed) {
            ++pickSlot;
        }
    };
    advancePickSlot();

    // Accept ROMs dragged onto the window while this screen is up. The gfx backends hand drops
    // to FileDropMgr (created by the window-only boot precisely so it exists here); the handler
    // stashes the path and the PICK phase below routes it into a slot.
    auto fileDropMgr = ctx->GetFileDropMgr();
    sDroppedRomPath.clear();
    if (fileDropMgr != nullptr) {
        fileDropMgr->RegisterDropHandler(HandleRomDrop);
    }

    enum Phase { PICK, EXTRACTING, FAILED };
    Phase phase = PICK;
    int activeSlot = -1; // slot currently extracting, -1 = pick next
    int result = 0;
    bool finished = false;

    while (!finished) {
        if (!ctx->GetWindow()->IsRunning()) {
            result = 0;
            break;
        }
        wnd->HandleEvents();
        if (!wnd->IsFrameReady()) {
            continue;
        }

        UseSharedImGuiContext();
        gui->StartDraw();
        wnd->StartFrame();
        wnd->RunGuiOnly();

        const ImVec4 theme = ComboMenu_ThemeColor();
        const ImVec4 kGreen(0.5f, 1.0f, 0.5f, 1.0f);
        const ImVec4 kRed(1.0f, 0.4f, 0.4f, 1.0f);

        ImGuiViewport* vp = ImGui::GetMainViewport();
        // Dim the game behind, mirroring the menu's translucent fullscreen overlay (ComboMenu.cpp).
        ImGui::GetBackgroundDrawList()->AddRectFilled(vp->Pos, ImVec2(vp->Pos.x + vp->Size.x, vp->Pos.y + vp->Size.y),
                                                      IM_COL32(0, 0, 0, 210));

        // Preferred 680px, but clamp to the main window's work area so we never bleed outside it
        // (a new player's window can launch small/square before any resolution is persisted).
        const float kMargin = 20.0f;
        float panelW = vp->WorkSize.x - kMargin * 2.0f;
        if (panelW > 680.0f) {
            panelW = 680.0f;
        }
        float panelMaxH = vp->WorkSize.y - kMargin * 2.0f;
        ImGui::SetNextWindowPos(vp->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSizeConstraints(ImVec2(panelW, 0.0f), ImVec2(panelW, panelMaxH));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24.0f, 20.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.09f, 0.09f, 0.11f, 0.98f));
        bool open = ImGui::Begin("ComboShip - ROM Setup", nullptr,
                                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
                                     ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);
        if (open) {
            ImGui::SetWindowFontScale(1.4f);
            ImGui::TextColored(theme, "ComboShip ROM Setup");
            ImGui::SetWindowFontScale(1.0f);
            ImGui::Separator();

            if (phase == PICK) {
                if (pickSlot < kSlotCount) {
                    RomSlot& s = slots[pickSlot];
                    if (!sDroppedRomPath.empty()) {
                        std::string dropped = std::move(sDroppedRomPath);
                        sDroppedRomPath.clear();
                        if (fileDropMgr != nullptr) {
                            fileDropMgr->ClearDroppedFile();
                        }
                        selectRom(pickSlot, dropped, false);
                    }

                    ImGui::TextColored(theme, "%s", s.label);
                    if (pickSlot == 0) {
                        ImGui::TextWrapped("Select your Ocarina of Time ROM. Vanilla or Master Quest can be "
                                           "selected first. You can add the other version next.");
                    } else if (pickSlot == 1) {
                        ImGui::TextWrapped("Add the other Ocarina of Time ROM to enable mixed vanilla/Master Quest "
                                           "dungeons. This is optional; you can add it later by launching "
                                           "ComboShip with --rom-setup.");
                    } else {
                        ImGui::TextWrapped("Select your Majora's Mask ROM to finish ComboShip ROM setup.");
                    }
                    if (!PickerAvailable()) {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
                        ImGui::TextWrapped("No file-picker helper found (install zenity or kdialog); "
                                           "drag and drop your ROM onto this window instead.");
                        ImGui::PopStyleColor();
                    }
                    ImGui::Spacing();
                    ImGui::TextWrapped("%s", s.path.empty() ? "(no ROM selected)" : s.path.c_str());
                    ComboMenu_PushButton(theme);
                    ImGui::BeginDisabled(!PickerAvailable());
                    if (ImGui::Button(s.path.empty() ? "Browse..." : "Change")) {
                        std::string picked = PickRomFile();
                        if (!picked.empty()) {
                            selectRom(pickSlot, picked, true);
                        }
                    }
                    ImGui::EndDisabled();
                    ComboMenu_PopButton();
                    ImGui::SameLine();
                    if (s.path.empty()) {
                        ImGui::TextDisabled(pickSlot == 1 ? "optional" : "required");
                    } else if (s.valid) {
                        ImGui::TextColored(kGreen, "Valid %s ROM", s.label);
                    } else {
                        ImGui::TextColored(kRed, "Not a valid %s ROM", s.label);
                    }
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    ImGui::BeginDisabled(!s.valid);
                    ComboMenu_PushButton(theme);
                    if (ImGui::Button(pickSlot == 1 ? "Add ROM" : "Continue", ImVec2(150.0f, 0.0f))) {
                        s.needed = true;
                        ++pickSlot;
                        advancePickSlot();
                    }
                    ComboMenu_PopButton();
                    ImGui::EndDisabled();
                    // Use the slot being rendered: Continue can already have advanced pickSlot.
                    if (&s == &slots[1]) {
                        ImGui::SameLine();
                        ComboMenu_PushButton(theme);
                        if (ImGui::Button("Skip", ImVec2(150.0f, 0.0f))) {
                            s.needed = false;
                            pickSlot = kMmSlot;
                            advancePickSlot();
                        }
                        ComboMenu_PopButton();
                    }
                } else {
                    ImGui::TextWrapped("ROM selection complete. Extract the selected ROMs to continue.");
                    ImGui::Spacing();
                    for (int i = 0; i < kSlotCount; ++i) {
                        const auto& s = slots[i];
                        if (s.needed) {
                            ImGui::TextColored(theme, "%s", s.label);
                            ImGui::TextWrapped("%s", s.path.c_str());
                        } else if (i != 1) {
                            ImGui::TextColored(kGreen, "%s (already extracted)", s.label);
                        }
                    }
                    ImGui::Spacing();
                    ComboMenu_PushButton(theme);
                    if (ImGui::Button("Extract", ImVec2(150.0f, 0.0f))) {
                        phase = EXTRACTING;
                        activeSlot = -1;
                    }
                    ComboMenu_PopButton();
                }
                ImGui::SameLine();
                ComboMenu_PushButton(theme);
                if (ImGui::Button("Quit", ImVec2(150.0f, 0.0f))) {
                    result = 0;
                    finished = true;
                }
                ComboMenu_PopButton();
            } else if (phase == EXTRACTING) {
                if (activeSlot < 0) {
                    // Pick the next needed-but-unfinished slot, or finish.
                    int next = -1;
                    for (int i = 0; i < kSlotCount; i++) {
                        if (slots[i].needed && !slots[i].done) {
                            next = i;
                            break;
                        }
                    }
                    if (next < 0) {
                        bool allOk = true;
                        for (const auto& s : slots) {
                            if (s.needed && !s.success) {
                                allOk = false;
                            }
                        }
                        if (allOk) {
                            result = 1;
                            finished = true;
                        } else {
                            phase = FAILED;
                        }
                    } else {
                        activeSlot = next;
                        slots[next].started = slots[next].start && slots[next].start(slots[next].path.c_str());
                        if (!slots[next].started) {
                            slots[next].done = true;
                            slots[next].success = false;
                            activeSlot = -1;
                        }
                    }
                } else {
                    RomSlot& s = slots[activeSlot];
                    unsigned long long count = 0, total = 0;
                    int done = 0, success = 0;
                    if (s.progress) {
                        s.progress(&count, &total, &done, &success);
                    }
                    float pct = total > 0 ? (float)count / (float)total : 0.0f;
                    ImGui::Text("Extracting %s...", s.label);
                    ImGui::Spacing();
                    char overlay[32];
                    if (count > 0) {
                        snprintf(overlay, sizeof(overlay), "%.0f%%", pct * 100.0f);
                    } else {
                        snprintf(overlay, sizeof(overlay), "Starting Up");
                    }
                    // Themed progress bar (matches OTRGlobals::RunExtract's "ROM Extraction" modal).
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(theme.x, theme.y, theme.z, 0.5f));
                    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(theme.x, theme.y, theme.z, 1.0f));
                    ImGui::ProgressBar(pct, ImVec2(ImGui::GetContentRegionAvail().x, 38.0f), overlay);
                    ImGui::PopStyleColor(2);
                    ImGui::PopStyleVar(1);
                    if (done) {
                        s.done = true;
                        s.success = (success != 0);
                        activeSlot = -1;
                    }
                }
            } else { // FAILED
                ImGui::TextColored(kRed, "Extraction failed. Check that your ROM is a supported, uncompressed "
                                         "copy, then relaunch.");
                ImGui::Spacing();
                ComboMenu_PushButton(theme);
                if (ImGui::Button("Quit", ImVec2(150.0f, 0.0f))) {
                    result = 0;
                    finished = true;
                }
                ComboMenu_PopButton();
            }
        }
        ImGui::End();

        gui->EndDraw();
        wnd->EndFrame();
    }

    if (fileDropMgr != nullptr) {
        fileDropMgr->UnregisterDropHandler(HandleRomDrop);
        fileDropMgr->ClearDroppedFile();
    }
    sDroppedRomPath.clear();

    return result;
}

extern "C" COMBO_EXPORT int ComboUI_SelectGame(int currentMode) {
    auto ctx = Ship::Context::GetRawInstance();
    if (!ctx || !ctx->GetWindow() || !ctx->GetWindow()->GetGui()) {
        return -1;
    }
    auto wnd = std::dynamic_pointer_cast<Fast::Fast3dWindow>(ctx->GetWindow());
    if (!wnd) {
        return -1;
    }
    const char* labels[] = { "Combo (OoT + MM)", "Ocarina of Time", "Majora's Mask" };
    if (currentMode < COMBO_GAME_BOTH || currentMode > COMBO_GAME_MM) {
        currentMode = COMBO_GAME_BOTH;
    }
    auto gui = wnd->GetGui();
    while (wnd->IsRunning()) {
        wnd->HandleEvents();
        if (!wnd->IsFrameReady()) {
            continue;
        }
        UseSharedImGuiContext();
        gui->StartDraw();
        wnd->StartFrame();
        wnd->RunGuiOnly();
        const auto theme = ComboMenu_ThemeColor();
        auto vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        const float width = std::max(220.0f, std::min(520.0f, vp->WorkSize.x - 32.0f));
        ImGui::SetNextWindowSizeConstraints(ImVec2(width, 0.0f), ImVec2(width, vp->WorkSize.y - 32.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24.0f, 20.0f));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.09f, 0.09f, 0.11f, 0.98f));
        const bool open = ImGui::Begin("ComboShip - Choose Game", nullptr,
                                       ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                           ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                           ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        int result = -2;
        if (open) {
            ImGui::SetWindowFontScale(1.4f);
            ImGui::TextColored(theme, "Choose your game");
            ImGui::SetWindowFontScale(1.0f);
            ImGui::TextDisabled("Last played: %s", labels[currentMode]);
            ImGui::Separator();
            ImGui::TextWrapped("Play both games together, or play either game on its own. "
                               "Solo games offer vanilla and randomizer files.");
            ImGui::Spacing();
            ComboMenu_PushButton(theme);
            for (int mode = COMBO_GAME_BOTH; mode <= COMBO_GAME_MM; ++mode) {
                if (ImGui::Button(labels[mode], ImVec2(-1.0f, 48.0f))) {
                    result = mode;
                }
            }
            ImGui::Spacing();
            if (ImGui::Button("Quit", ImVec2(-1.0f, 0.0f))) {
                result = -1;
            }
            ComboMenu_PopButton();
        }
        ImGui::End();
        gui->EndDraw();
        wnd->EndFrame();
        if (result != -2) {
            return result;
        }
    }
    return -1;
}
