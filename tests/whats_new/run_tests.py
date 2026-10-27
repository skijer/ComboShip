#!/usr/bin/env python3
"""Run the native menu registrations and the real headless ImGui highlight probe.

Set IMGUI_DIR to the project's pinned Dear ImGui source directory.
"""
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
IMGUI = next((Path(path) for path in [os.environ.get("IMGUI_DIR", ""), *os.environ.get("CPATH", "").split(os.pathsep)]
              if path and (Path(path) / "imgui.cpp").is_file()), None)


class WhatsNewTests(unittest.TestCase):
    def run_probe(self, path):
        self.assertIsNotNone(IMGUI, "Set IMGUI_DIR or CPATH to the pinned ImGui source directory")
        environment = os.environ.copy()
        environment["IMGUI_DIR"] = str(IMGUI.resolve())
        subprocess.run(["python3", "-B", str(ROOT / path)], check=True, env=environment)

    def test_audio_menu_navigation(self):
        self.run_probe("tests/audio_menu/run_tests.py")

    def test_cosmetic_effect_families(self):
        self.run_probe("tests/cosmetics_ui/run_tests.py")

    def test_generate_and_market_controls(self):
        self.run_probe("tests/whats_new/run_generate_tests.py")

    def test_real_imgui_and_native_controls(self):
        self.assertIsNotNone(IMGUI, "Set IMGUI_DIR or CPATH to the pinned ImGui source directory")
        imgui = IMGUI
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            pieces = []
            for namespace, path, mm in (
                ("OotWidgets", "soh/soh/SohGui/UIWidgets.cpp", False),
                ("MmWidgets", "mm/2s2h/BenGui/UIWidgets.cpp", True),
            ):
                source = (ROOT / path).read_text()

                def function(signature):
                    start = source.index(signature)
                    return source[start:source.index("\n}", start) + 2]

                options = r'''
enum class Colors { LightBlue };
enum class LabelPositions { Near, Far, Above, None };
using LabelPosition = LabelPositions;
enum class ComponentAlignments { Left, Right };
using ComponentAlignment = ComponentAlignments;
const std::map<Colors, ImVec4> ColorValues{{Colors::LightBlue, ImVec4(0.3f, 0.6f, 0.8f, 1)}};
struct CheckboxOptions {
    bool disabled = false;
    LabelPositions labelPosition = LabelPositions::Near;
    ComponentAlignments alignment = ComponentAlignments::Left;
    Colors color = Colors::LightBlue;
    ImVec2 padding = ImVec2(10, 8);
'''
                options += 'const char* tooltip = ""; const char* disabledTooltip = "";\n' if mm else \
                           'std::string tooltip; std::string disabledTooltip;\n'
                options += "};\n"
                options += "std::string WrappedText(const char*, unsigned int = 80);\n"
                options += "std::string WrappedText(const std::string&, unsigned int = 80);\n"
                functions = [function(signature) for signature in (
                    "std::string WrappedText(const char*", "std::string WrappedText(const std::string&",
                    "void PushStyleCheckbox(const ImVec4&", "void PushStyleCheckbox(Colors",
                    "void PopStyleCheckbox()", "void RenderText(", "bool Checkbox(")]
                pieces.append("namespace " + namespace + " {\n" + options + "\n".join(functions) + "\n}\n")
            (directory / "native_checkboxes.inc").write_text("\n".join(pieces))
            binary = directory / "render"
            sanitizers = ["-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer",
                          "-fno-pie", "-no-pie"] if os.environ.get("SANITIZE") else []
            command = [*shlex.split(os.environ.get("CXX", "c++")), "-std=c++20", "-Wall", "-Wextra", "-Werror",
                       *sanitizers, "-DIMGUI_DEFINE_MATH_OPERATORS=", "-I" + str(ROOT / "combo"),
                       "-I" + str(imgui), "-I" + str(directory),
                       str(ROOT / "tests/whats_new/render_test.cpp")]
            command += [str(imgui / name) for name in ("imgui.cpp", "imgui_draw.cpp", "imgui_tables.cpp", "imgui_widgets.cpp")]
            subprocess.run([*command, "-o", str(binary)], check=True)
            run = subprocess.run([str(binary)], capture_output=True, text=True)
            self.assertEqual(run.returncode, 0, run.stderr)
            print(run.stdout.strip())

    def test_native_toggle_registration(self):
        for path in ("soh/soh/SohGui/SohMenuSettings.cpp", "mm/2s2h/BenGui/BenMenu.cpp"):
            with self.subTest(host=path), tempfile.TemporaryDirectory() as temporary:
                source = (ROOT / path).read_text()
                match = re.search(r'AddWidget\(path, "What\'s New", WIDGET_CVAR_CHECKBOX\).*?;', source, re.S)
                registration = match.group(0) if match else ""
                helper = ROOT / "combo/gui/WhatsNew.h"
                include = '#include "gui/WhatsNew.h"\n' if helper.exists() else ""
                fixture = include + r'''
#include <cassert>
#include <string>
struct CheckboxOptions {
    bool value = false;
    CheckboxOptions& DefaultValue(bool v) { value = v; return *this; }
    CheckboxOptions& Tooltip(const char*) { return *this; }
};
struct Widget {
    std::string key;
    bool defaultValue = false;
    Widget& CVar(const char* k) { key = k; return *this; }
    Widget& RaceDisable(bool) { return *this; }
    Widget& Options(const CheckboxOptions& o) { defaultValue = o.value; return *this; }
};
int count = 0;
Widget widget;
Widget& AddWidget(int, const char*, int) { ++count; return widget; }
int main() {
    [[maybe_unused]] int path = 0;
    [[maybe_unused]] constexpr int WIDGET_CVAR_CHECKBOX = 0;
    // The exact production registration is inserted here.
''' + registration + r'''
    assert(count == 1 && "native menu must expose What's New");
    assert(widget.key == "gSettings.Menu.WhatsNew");
    assert(widget.defaultValue && "existing and fresh configs default to highlights on");
}
'''
                directory = Path(temporary)
                unit = directory / "registration.cpp"
                unit.write_text(fixture)
                binary = directory / "registration"
                command = [*shlex.split(os.environ.get("CXX", "c++")), "-std=c++20", "-Wall", "-Wextra", "-Werror",
                           "-I" + str(ROOT / "combo")]
                if IMGUI:
                    command += ["-I" + str(IMGUI)]
                subprocess.run([*command, str(unit), "-o", str(binary)], check=True)
                run = subprocess.run([str(binary)], capture_output=True, text=True)
                self.assertEqual(run.returncode, 0, run.stderr)


if __name__ == "__main__":
    unittest.main()
