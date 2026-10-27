// Complete production menu control and OoT park/query bodies are injected by
// the runner. Rendering is a fixture: only Generate/Save is pressed,
// and disabled buttons correctly return false. No game loop or fill runs.
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <vector>
#include <imgui.h>
#include "z64.h"
#include "ComboExport.h"
#include "gui/ComboGenProgress.h"
#include "gui/ComboForeground.h"
#include "rando/SharedItems.h"
#include "rando/ComboPlaythrough.h"

static std::vector<bool> disabledStack;
static int foregroundGame = 0;
static int generationCalls = 0;
static int reloadCalls = 0;
static const char* pressedButton = "Generate";
namespace ImGui {
void TextWrapped(const char*, ...) {}
void TextDisabled(const char*, ...) {}
void Text(const char*, ...) {}
void TextColored(const ImVec4&, const char*, ...) {}
void TextUnformatted(const char*, const char*) {}
void Separator() {}
void SeparatorText(const char*) {}
void SetNextItemWidth(float) {}
void SameLine(float, float) {}
// ImGui pushes an item-flags scope even for BeginDisabled(false); nested false
// scopes also retain a disabled parent's state. Mirror that observer contract.
void BeginDisabled(bool disabled) {
    disabledStack.push_back(disabled || (!disabledStack.empty() && disabledStack.back()));
}
void EndDisabled() {
    assert(!disabledStack.empty());
    disabledStack.pop_back();
}
bool Button(const char* label, const ImVec2&) {
    return (disabledStack.empty() || !disabledStack.back()) && std::strcmp(label, pressedButton) == 0;
}
bool SmallButton(const char*) { return false; }
bool Checkbox(const char*, bool*) { return false; }
bool InputInt(const char*, int*, int, int, ImGuiInputTextFlags) { return false; }
bool InputTextWithHint(const char*, const char*, char*, size_t, ImGuiInputTextFlags,
                       ImGuiInputTextCallback, void*) { return false; }
bool BeginCombo(const char*, const char*, ImGuiComboFlags) { return false; }
bool Combo(const char*, int*, const char* const*, int, int) { return false; }
void EndCombo() {}
bool Selectable(const char*, bool, ImGuiSelectableFlags, const ImVec2&) { return false; }
bool BeginTable(const char*, int, ImGuiTableFlags, const ImVec2&, float) { return false; }
void EndTable() {}
void TableNextRow(ImGuiTableRowFlags, float) {}
bool TableSetColumnIndex(int) { return false; }
void SetItemTooltip(const char*, ...) {}
void ProgressBar(float, const ImVec2&, const char*) {}
void SetClipboardText(const char*) {}
bool BeginChild(const char*, const ImVec2&, ImGuiChildFlags, ImGuiWindowFlags) { return true; }
void EndChild() {}
void TableSetupColumn(const char*, ImGuiTableColumnFlags, float, ImGuiID) {}
void TableSetupScrollFreeze(int, int) {}
void TableHeadersRow() {}
void PushID(int) {}
void PopID() {}
void PushStyleColor(ImGuiCol, const ImVec4&) {}
void PopStyleColor(int) {}
void OpenPopup(const char*, ImGuiPopupFlags) {}
bool BeginPopup(const char*, ImGuiWindowFlags) { return false; }
void CloseCurrentPopup() {}
void EndPopup() {}
}
namespace ComboUI {
int GetForegroundGame() { return foregroundGame; }
}
// The appearance panel is rendering only; generation/reload gating is the
// production behavior under test here.
namespace BlackMarketUI {
void DrawAppearance() {}
}
extern "C" int ComboUI_GetForegroundGame() { return foregroundGame; }
int CVarGetInteger(const char*, int fallback) { return fallback; }
void CVarSetInteger(const char*, int) {}
void CVarSetString(const char*, const char*) {}
extern "C" {
PlayState* gPlayState = nullptr;
GameState* gGameState = nullptr;
void FileChoose_Main(GameState*) {}
void Other_Main(GameState*) {}
}
/* OOT_PARK_AND_QUERY */

namespace ComboRando {
class ComboMenu {
    char mSeedBuf[128]{};
  public:
    void DrawComboPanel();
};
ImVec4 ComboMenu_ThemeColor() { return {}; }
void ComboMenu_PushCheckbox(const ImVec4&) {}
void ComboMenu_PopCheckbox() {}
void ComboMenu_PushInput(const ImVec4&) {}
void ComboMenu_PopInput() {}
void ComboMenu_PushCombobox(const ImVec4&) {}
void ComboMenu_PopCombobox() {}
void ComboMenu_PushButton(const ImVec4&) {}
void ComboMenu_PopButton() {}
void ResolveComboGenSyms() {}
void (*sTrigger)() = []() { ++generationCalls; };
unsigned char (*sIsOnFileSelect)() = SOH_IsOnFileSelect;
const ComboGenProgress* (*sGetProgress)() = nullptr;
void (*sRefreshStartingGameUI)() = nullptr;
int (*sRequestReload)(const char*) = [](const char*) { ++reloadCalls; return 1; };
void ResolvePlandoSyms() {}
void PlandoRefreshSpoilerList() {}
void PlandoLoad() {}
void PlandoSavePlay() { sRequestReload("fixture-seed.json"); }
/* ACTUAL_PLANDO_STATE */
/* ACTUAL_FILE_SELECT_GATE */
/* ACTUAL_PLANDO_PANEL */
/* ACTUAL_COMBO_PANEL */
}

static int failures = 0;
static void Check(bool condition, const char* message) {
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); ++failures; }
}

// Execute both complete native panels; check whether the mutation callback was
// invoked. This does not substitute a copied Boolean expression for the caller.
static void CheckActions(ComboRando::ComboMenu& menu, bool generate, bool reload, const char* scenario) {
    const int beforeGenerate = generationCalls, beforeReload = reloadCalls;
    pressedButton = "Generate";
    menu.DrawComboPanel();
    Check(generationCalls == beforeGenerate + generate, scenario);
    pressedButton = "Save";
    ComboRando::DrawComboPlandoPanel();
    Check(reloadCalls == beforeReload + reload, scenario);
    Check(disabledStack.empty(), "each production panel balances its disabled scope");
}

int main() {
    GameState fileSelect{};
    fileSelect.main = FileChoose_Main;
    fileSelect.running = true;
    gGameState = &fileSelect;
    ComboRando::sPlando.loaded = true;
    ComboRando::sPlando.spoilerPaths.push_back("fixture-seed.json");
    ComboRando::ComboMenu menu;
    CheckActions(menu, true, true, "real OoT foreground file select permits generation/reload");
    // Actual MM-first handoff ordering: the launcher parks the OoT file-select
    // and changes foreground to MM. The dormant donor still has that main ptr.
    SOH_ParkForComboMMResume();
    foregroundGame = 1;
    Check(!gGameState->running && !gPlayState && gGameState->main == FileChoose_Main,
          "native park reproduces the MM-first dormant state");
    CheckActions(menu, false, false, "parked MM-first donor blocks generation/reload during MM gameplay");
    foregroundGame = 0;
    fileSelect.running = true;
    CheckActions(menu, true, true, "return to true OoT foreground file select permits both actions");

    PlayState livePlay{};
    gPlayState = &livePlay;
    CheckActions(menu, false, false, "OoT gameplay blocks both actions");
    gPlayState = nullptr;
    gGameState = nullptr;
    CheckActions(menu, false, false, "absent game state blocks both actions");
    gGameState = &fileSelect;
    fileSelect.main = Other_Main;
    CheckActions(menu, false, false, "other OoT screen blocks both actions");
    fileSelect.main = FileChoose_Main;
    ComboRando::sIsOnFileSelect = nullptr;
    CheckActions(menu, false, false, "missing donor query blocks both actions");
    ComboRando::sIsOnFileSelect = SOH_IsOnFileSelect;

    auto trigger = ComboRando::sTrigger;
    ComboRando::sTrigger = nullptr;
    CheckActions(menu, false, true, "missing generator export disables only generation");
    ComboRando::sTrigger = trigger;
    auto reload = ComboRando::sRequestReload;
    ComboRando::sRequestReload = nullptr;
    CheckActions(menu, true, false, "missing reload export disables only reload");
    ComboRando::sRequestReload = reload;

    static ComboRando::ComboGenProgress progress;
    ComboRando::sGetProgress = []() -> const ComboRando::ComboGenProgress* { return &progress; };
    progress.running = true;
    CheckActions(menu, false, true, "busy generator remains disabled at true OoT file select");
    progress.running = false;
    CheckActions(menu, true, true, "idle generator re-enables generation");
    foregroundGame = 1;
    CheckActions(menu, false, false, "MM foreground blocks both actions even with an unparked donor file select");
    if (failures) return 1;
    std::puts("PASS actual combo generation/reload panels: MM-first park, foreground return, gameplay, screen, exports and busy controls");
}
