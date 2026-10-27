#!/usr/bin/env python3
"""Run real ImGui input against the production Generate/appearance controls.

Set IMGUI_DIR to the project's pinned Dear ImGui source directory. --source-rev
uses older widget code with the current release inventory as a negative control.
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
    masked = re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"',
                    lambda match: re.sub(r'[^\n]', ' ', match[0]), source, flags=re.S)
    start = source.index(signature)
    end = masked.index("{", start) + 1
    depth = 1
    while depth:
        depth += (masked[end] == "{") - (masked[end] == "}")
        end += 1
    return source[start:end]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-rev", help="read production widget code from this revision")
    args = parser.parse_args()
    imgui = next((Path(path) for path in [os.environ.get("IMGUI_DIR", ""),
                                        *os.environ.get("CPATH", "").split(os.pathsep)]
                  if path and (Path(path) / "imgui.cpp").is_file()), None)
    if imgui is None:
        parser.error("Set IMGUI_DIR or CPATH to the pinned Dear ImGui source directory")

    def read(path):
        if args.source_rev:
            return subprocess.check_output(["git", "show", args.source_rev + ":" + path], cwd=ROOT, text=True)
        return (ROOT / path).read_text()

    panel = read("combo/gui/ComboMenu.cpp")
    start = panel.index('ImGui::SeparatorText("Entrances");', panel.index("void ComboMenu::DrawComboPanel("))
    entrances = panel[start:panel.index("// Shared Items", start)]
    appearance = block(read("combo/gui/BlackMarketAppearance.h"), "inline void DrawAppearance()")
    with tempfile.TemporaryDirectory(prefix="generate-release-") as temporary:
        directory = Path(temporary)
        (directory / "generate_controls.inc").write_text(entrances)
        (directory / "black_market_appearance.inc").write_text(appearance)
        binary = directory / "generate"
        sanitizers = ["-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer",
                      "-fno-pie", "-no-pie"] if os.environ.get("SANITIZE") else []
        command = [*shlex.split(os.environ.get("CXX", "c++")), "-std=c++20", "-Wall", "-Wextra", "-Werror",
                   *sanitizers, "-DIMGUI_DEFINE_MATH_OPERATORS=", "-DF3DEX_GBI_2",
                   "-I" + str(ROOT / "combo"), "-I" + str(ROOT / "libultraship/include"),
                   "-I" + str(imgui), "-I" + str(directory),
                   str(ROOT / "tests/whats_new/generate_test.cpp")]
        command += [str(imgui / name) for name in ("imgui.cpp", "imgui_draw.cpp", "imgui_tables.cpp", "imgui_widgets.cpp")]
        subprocess.run([*command, "-o", str(binary)], check=True)
        raise SystemExit(subprocess.run([str(binary)]).returncode)


if __name__ == "__main__":
    main()
