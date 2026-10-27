#!/usr/bin/env python3
"""Execute both cosmetic tab bindings and their native CVarCheckbox functions.

The menu blocks, option types and CVar read/write/save logic are production code.
Only visual input/output, the shared config store and save/apply services are
fixtures. --source-rev runs older production sources as a negative control.
"""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def block(source, signature):
    """Extract a complete production block, ignoring braces in text/comments."""
    masked = re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"',
                    lambda match: re.sub(r'[^\n]', ' ', match[0]), source, flags=re.S)
    start = source.index(signature)
    end = masked.index("{", start) + 1
    depth = 1
    while depth:
        depth += (masked[end] == "{") - (masked[end] == "}")
        end += 1
    return source[start:end]


FIXTURE = r'''
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

// Visual value type only; no ImGui context or renderer is needed for input.
struct ImVec2 {
    float x, y;
    ImVec2(float x, float y) : x(x), y(y) {}
};

namespace Fixture {
constexpr const char* key = "gEnhancements.SkijerNEI.UseVanillaHeldSwords";
constexpr const char* label = "Use Vanilla Held Swords";
std::map<std::string, int32_t> config;
std::vector<std::pair<std::string, int32_t>> writes;
std::vector<std::string> applies;
int saves = 0, failures = 0;
bool click = false, tabOpen = true;
std::string lastRead;
struct Row {
    std::string label, key;
    bool checked, defaultValue, disabled;
};
std::vector<Row> rows;

void Check(bool condition, const std::string& why) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << why << '\n';
    }
}

bool Checkbox(const char* text, bool* value, bool defaultValue, bool disabled) {
    rows.push_back({text, lastRead, *value, defaultValue, disabled});
    if (click && !disabled && std::string(text) == label) {
        *value = !*value;
        return true;
    }
    return false;
}
} // namespace Fixture

int32_t CVarGetInteger(const char* key, int32_t fallback) {
    Fixture::lastRead = key;
    const auto found = Fixture::config.find(key);
    return found == Fixture::config.end() ? fallback : found->second;
}
void CVarSetInteger(const char* key, int32_t value) {
    Fixture::writes.emplace_back(key, value);
    Fixture::config[key] = value;
}
namespace Ship {
struct Context {
    static Context* GetRawInstance() { static Context context; return &context; }
    Context* GetWindow() { return this; }
    Context* GetGui() { return this; }
    void SaveConsoleVariablesNextFrame() { ++Fixture::saves; }
};
} // namespace Ship
namespace ShipInit {
void Init(const char* key) { Fixture::applies.emplace_back(key); }
}
namespace ImGui {
void TextUnformatted(const char*) {}
bool BeginTabItem(const char* label) {
    Fixture::Check(std::string(label) == "Link & Items", "checkbox belongs to Link & Items");
    return Fixture::tabOpen;
}
void EndTabItem() {}
} // namespace ImGui
// Release markers are presentation only; the real-ImGui What's New suite owns
// their styling/identity contract. Keep this probe on native CVar/save behavior.
namespace WhatsNew {
constexpr const char* kEnabledCvar = "gSettings.Menu.WhatsNew";
struct Scope {
    Scope(const char*, const char*, bool, bool = false) {}
};
} // namespace WhatsNew

/* HOSTS */

namespace Fixture {
void Draw(void (*draw)(), const char* host, bool expected, bool clicked = false) {
    rows.clear();
    click = clicked;
    const auto oldWrites = writes.size();
    const auto oldApplies = applies.size();
    const int oldSaves = saves;
    draw();
    const auto count = std::count_if(rows.begin(), rows.end(), [](const Row& row) {
        return row.label == label;
    });
    Check(count == 1, std::string(host) + " Link & Items renders Use Vanilla Held Swords exactly once");
    for (const Row& row : rows) {
        if (row.label != label)
            continue;
        Check(row.key == key, std::string(host) + " binds the same shared held-sword CVar");
        Check(!row.defaultValue, std::string(host) + " leaves authored held swords enabled by default");
        Check(!row.disabled, std::string(host) + " permits a live checkbox change");
        Check(row.checked == expected, std::string(host) + " displays the current shared value");
    }
    Check(writes.size() == oldWrites + (clicked ? 1 : 0), "only a click writes one CVar");
    Check(saves == oldSaves + (clicked ? 1 : 0), "only a click schedules one config save");
    Check(applies.size() == oldApplies + (clicked ? 1 : 0), "only a click applies the changed CVar");
    if (clicked && writes.size() == oldWrites + 1) {
        Check(writes.back() == std::pair<std::string, int32_t>{key, !expected},
              "the clicked editor toggles the shared held-sword value");
        Check(applies.size() > oldApplies && applies.back() == key,
              "native live-apply receives the same shared key");
    }
}
} // namespace Fixture

int main() {
    using namespace Fixture;
    const std::map<std::string, int32_t> untouched = {
        {"gEnhancements.SkijerNEI.ItemEffects", 1},
        {"gEnhancements.DinFireSword", 1},
        {"gEnhancements.DungeonItemColors", 1},
        {"gSettings.Mods.AlternateAssets", 1},
    };
    config = untouched;
    Draw(Oot::DrawLinkAndItems, "OoT", false);
    Draw(Mm::DrawLinkAndItems, "MM", false);
    Check(config == untouched, "opening either editor preserves an absent preference and unrelated settings");
    if (failures)
        return 1;

    // Existing saved values must appear in both menus without being rewritten.
    for (int32_t saved : {1, 0}) {
        config[key] = saved;
        Draw(Oot::DrawLinkAndItems, "OoT", saved != 0);
        Draw(Mm::DrawLinkAndItems, "MM", saved != 0);
    }
    config.erase(key);
    for (int repeat = 0; repeat < 2; ++repeat) {
        Draw(Oot::DrawLinkAndItems, "OoT", false, true);
        Draw(Mm::DrawLinkAndItems, "MM", true);
        Draw(Mm::DrawLinkAndItems, "MM", true, true);
        Draw(Oot::DrawLinkAndItems, "OoT", false);
        Draw(Mm::DrawLinkAndItems, "MM", false, true);
        Draw(Oot::DrawLinkAndItems, "OoT", true);
        Draw(Oot::DrawLinkAndItems, "OoT", true, true);
        Draw(Mm::DrawLinkAndItems, "MM", false);
    }
    auto expected = untouched;
    expected[key] = 0;
    Check(config == expected, "repeated cross-game changes preserve every unrelated CVar");

    // An inactive tab must not consume an attempted checkbox click.
    rows.clear();
    tabOpen = false;
    click = true;
    const auto oldWrites = writes.size();
    Oot::DrawLinkAndItems();
    Mm::DrawLinkAndItems();
    Check(rows.empty() && writes.size() == oldWrites && config == expected,
          "inactive Link & Items tabs neither draw nor change the preference");
    if (failures)
        return 1;
    std::cout << "PASS native OoT/MM cosmetic menu binding: absent default Off, saved values, shared live on/off, "
                 "save/apply on clicks, inactive tabs and unrelated CVar preservation\n";
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-rev", help="run previous production menu sources as a negative control")
    args = parser.parse_args()

    def read(path):
        if args.source_rev:
            return subprocess.check_output(["git", "show", args.source_rev + ":" + path], cwd=ROOT, text=True)
        return (ROOT / path).read_text()

    hosts = []
    for host, ui, editor, option_header, plural in (
        ("Oot", "soh/soh/SohGui", "soh/soh/Enhancements/cosmetics/CosmeticsEditor",
         "UIWidgetOptions.hpp", "s"),
        ("Mm", "mm/2s2h/BenGui", "mm/2s2h/BenGui/CosmeticEditor", "UIWidgets.hpp", ""),
    ):
        options = read(ui + "/" + option_header)
        types = "\n".join(block(options, signature) + ";" for signature in (
            "enum Colors", "enum LabelPosition" + plural, "enum ComponentAlignment" + plural,
            "struct WidgetOptions", "struct CheckboxOptions"))
        groups = re.search(r"typedef enum\s*\{[^{}]*\}\s*CosmeticGroup;", read(editor + ".h"))[0]
        binding = block(read(ui + "/UIWidgets.cpp"), "bool CVarCheckbox(")
        tab = block(read(editor + ".cpp"), 'if (ImGui::BeginTabItem("Link & Items"))')
        hosts.append("namespace " + host + " {\nnamespace UIWidgets {\n" + r'''
// Keep native option declarations intact. MM's existing self-assignment and
// enum/method name reuse are unrelated to the binding under test.
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsequence-point"
#if __GNUC__ >= 13
#pragma GCC diagnostic ignored "-Wchanges-meaning"
#endif
#endif
''' + types + r'''
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
bool Checkbox(const char* label, bool* value, const CheckboxOptions& options) {
    return Fixture::Checkbox(label, value, options.defaultValue, options.disabled);
}
void Separator(bool, bool, float, float) {}
''' + binding + "\n} // namespace UIWidgets\n" + groups + r'''
const auto THEME_COLOR = UIWidgets::Colors::LightBlue;
void DrawCosmeticGroup(CosmeticGroup) {}
void CosmeticEditorDrawGroup(CosmeticGroup, const char* = nullptr) {}
void DrawLinkAndItems() {
''' + tab + "\n}\n} // namespace " + host + "\n")

    with tempfile.TemporaryDirectory(prefix="vanilla-sword-menu-") as temporary:
        directory = Path(temporary)
        source = directory / "menu.cpp"
        source.write_text(FIXTURE.replace("/* HOSTS */", "\n".join(hosts)))
        executable = directory / "menu"
        subprocess.run([*shlex.split(os.environ.get("CXX", "c++")), "-std=c++20", "-Wall", "-Wextra", "-Werror",
                        str(source), "-o", str(executable)], check=True)
        raise SystemExit(subprocess.run([str(executable)]).returncode)


if __name__ == "__main__":
    main()
