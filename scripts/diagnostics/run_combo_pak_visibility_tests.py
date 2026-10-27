#!/usr/bin/env python3
"""Exercise the production ComboShip SoH draw bridge without game dependencies."""
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / 'soh/soh/SohGui/SohMenu.cpp').read_text()
function = re.search(r'int32_t SohMenu::DrawWidgetByIndex\(.*?^}', source, re.M | re.S)
assert function, 'production draw bridge not found'
prefix = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>
namespace UIWidgets {
struct ComboboxOptions { std::map<int, const char*> comboMap; int defaultIndex = -1; };
}
struct Widget {
    int type = 1;
    const char* cVar = "slot";
    std::string name = "slot";
    std::shared_ptr<void> options;
    std::function<void()> preFunc, customFunction;
};
constexpr int WIDGET_CVAR_COMBOBOX = 1;
struct Globals { void* fontStandardLargest = this; };
namespace OTRGlobals { Globals state; Globals* Instance = &state; }
namespace ImGui { void TextDisabled(const char*, ...) {} }
std::map<std::string, int> cvars;
int CVarGetInteger(const char* key, int fallback) {
    auto it = cvars.find(key); return it == cvars.end() ? fallback : it->second;
}
float CVarGetFloat(const char*, float fallback) { return fallback; }
struct Disable { bool active; std::function<bool(Disable&)> evaluation; };
struct SohMenu {
    struct { std::vector<Widget*> flat; std::vector<bool> flatRando; } mComboExport;
    std::map<int, Disable> disabledMap;
    int drawn = 0;
    int GetMenuThemeColor() { return 0; }
    void MenuDrawItem(Widget&, int) { ++drawn; }
    int32_t DrawWidgetByIndex(int32_t, int32_t);
};
'''
suffix = r'''
int main() {
    SohMenu menu;
    Widget widget;
    auto options = std::make_shared<UIWidgets::ComboboxOptions>();
    widget.options = options;
    menu.mComboExport.flat = {&widget};
    menu.mComboExport.flatRando = {false};
    // The slot preFunc leaves an absent CVar unset: -1 is already a valid default.
    // Pack zero lacks this slot. Its dropdown must still display Default (inherit).
    options->comboMap = {{-1, "Default (inherit)"}, {4, "Another pack"}};
    menu.DrawWidgetByIndex(0, 90);
    if (menu.drawn != 1) {
        std::fprintf(stderr, "FAIL unset equipment slot disappears when pack zero lacks the item\n");
        return 1;
    }
    assert(cvars.empty()); // Rendering must not persist an arbitrary selection.
    cvars["slot"] = -1;
    menu.DrawWidgetByIndex(0, 90);
    assert(menu.drawn == 2);
    cvars["slot"] = 4;
    menu.DrawWidgetByIndex(0, 90);
    assert(menu.drawn == 3);
    cvars["slot"] = 999;
    menu.DrawWidgetByIndex(0, 90);
    assert(menu.drawn == 3); // Preserve the invalid-selection crash guard.
    cvars.clear();
    options->defaultIndex = 7;
    options->comboMap = {{7, "Nonzero default"}};
    menu.DrawWidgetByIndex(0, 90);
    assert(menu.drawn == 4);
    options->comboMap.clear();
    menu.DrawWidgetByIndex(0, 90);
    assert(menu.drawn == 4);
    widget.options.reset();
    menu.DrawWidgetByIndex(0, 90);
    assert(menu.drawn == 4);
    std::puts("PASS unset, inherit, explicit, invalid, nonzero-default, empty and null dropdowns");
}
'''
with tempfile.TemporaryDirectory(prefix='combo-pak-visibility-') as directory:
    build = Path(directory)
    cpp = build / 'test.cpp'
    cpp.write_text(prefix + function[0] + suffix)
    exe = build / 'test'
    subprocess.run([*shlex.split(os.environ.get('CXX', 'c++')), '-std=c++20',
                    '-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter', str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
