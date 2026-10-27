#!/usr/bin/env python3
"""Execute native emote registration callbacks and the shared production panel.

Only the surrounding native menu registry, ImGui input and CVar persistence are
fixtures. The registration blocks are taken unchanged from the native settings
methods. --source-rev selects older registration code as a negative control.
Supply the project's pinned ImGui and JSON headers through the normal CPATH.
"""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--source-rev")
args = parser.parse_args()


def registration(path, method):
    if args.source_rev:
        source = subprocess.check_output(
            ["git", "show", args.source_rev + ":" + path], cwd=ROOT, text=True
        )
    else:
        source = (ROOT / path).read_text()
    start = source.index("void " + method + "() {")
    end = source.index("\n}", start)
    body = source[start:end]
    blocks = re.findall(r"^#ifdef COMBO_BUILD\n(.*?)^#endif", body, re.M | re.S)
    panels = [block for block in blocks if "ComboEmotes::DrawSettings()" in block]
    assert len(panels) <= 1, "A native settings menu must register one emote panel"
    return "\n".join(panels)


with tempfile.TemporaryDirectory(prefix="emotes-menu-") as temporary:
    directory = Path(temporary)
    registrations = []
    for name, path, method in (
        ("OotMenu", "soh/soh/SohGui/SohMenuSettings.cpp", "SohMenu::AddMenuSettings"),
        ("MmMenu", "mm/2s2h/BenGui/BenMenu.cpp", "BenMenu::AddSettings"),
    ):
        registrations.append(
            "struct " + name + " : MenuFixture { void Register() {\n"
            '[[maybe_unused]] WidgetPath path{"Settings", "General", SECTION_COLUMN_1};\n'
            + registration(path, method) + "\n} };\n"
        )
    (directory / "native_emote_registration.inc").write_text("\n".join(registrations))
    binary = directory / "menu_test"
    subprocess.run(
        [*shlex.split(os.environ.get("CXX", "c++")), "-std=c++20", "-Wall", "-Wextra", "-Werror",
         "-DCOMBO_BUILD", "-I" + str(ROOT / "combo"), "-I" + str(ROOT / "libultraship/include"),
         "-I" + str(directory), str(ROOT / "tests/emotes/menu_settings_test.cpp"),
         str(ROOT / "combo/gui/ComboEmoteSettings.cpp"), "-o", str(binary)],
        check=True,
    )
    subprocess.run([str(binary)], check=True)
