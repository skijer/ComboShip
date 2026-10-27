#!/usr/bin/env python3
"""Exercise production audio-menu routing and night selection with real ImGui.

Set IMGUI_DIR to the project's pinned Dear ImGui source directory. Only the
native widget and audio-engine boundaries are fixtures; menu branches, widget
registrations, seasonal navigation and the night-track selector come from the
production AudioEditor.cpp files.
"""
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, name):
    match = re.search(r"^(?:static )?(?:void|bool) " + re.escape(name) + r"\([^;]*?\)\s*\{.*?^}",
                      source, re.M | re.S)
    return match[0] if match else ""


def main():
    imgui = Path(os.environ["IMGUI_DIR"])
    with tempfile.TemporaryDirectory(prefix="audio-menu-") as temporary:
        build = Path(temporary)
        for host, path in (("oot", "soh/soh/Enhancements/audio/AudioEditor.cpp"),
                           ("mm", "mm/2s2h/Enhancements/Audio/AudioEditor.cpp")):
            source = (ROOT / path).read_text()
            declarations = "\n".join(re.findall(r"^static (?:WidgetInfo|std::array<WidgetInfo,[^\n]+)[^\n]*;",
                                               source, re.M))
            if host == "oot":
                choices = source[source.index("static const std::map<int32_t, const char*> proximityWeatherThunderStyles"):
                                 source.index("static void DrawHyruleFieldNightTrack()")]
                start = source.index('    hyruleFieldNightMusic = { .name =')
                end = source.index('    randomAudioGenModes = { .name =', start)
                registration = source[start:end]
                night = function(source, "DrawHyruleFieldNightTrack")
                search = ""
            else:
                choices = night = ""
                search = function(source, "AddAudioSearchWidget")
                start = source.index('    weatherWidgets[0] = { .name =')
                end = source.index('    lowHpAlarm = { .name =', start)
                registration = source[start:end]
            start = source.index("    UIWidgets::PushStyleTabs(THEME_COLOR);", source.index("void AudioEditor::DrawElement()"))
            end = source.index('        if (ImGui::BeginTabItem("Background Music"))', start)
            # Keep the real audio-options/weather branch, closing only the
            # unrelated music-table tab bar omitted from this focused probe.
            draw = ("void DrawAudioMenu() {\n" + source[start:end] +
                    "ImGui::EndTabBar(); } UIWidgets::PopStyleTabs();\n}\n")
            generated = "\n".join((declarations, choices, search, night,
                                     function(source, "DrawWeatherOptions"),
                                     "void RegisterWeatherWidgets() {\n" + registration + "\n}", draw))
            (build / "production_audio.inc").write_text(generated)
            binary = build / (host + "_menu")
            command = [*shlex.split(os.environ.get("CXX", "c++")), "-std=c++20", "-Wall", "-Wextra", "-Werror",
                       "-Wno-unused-variable", "-Wno-missing-field-initializers", "-DTEST_MM=" + str(int(host == "mm")),
                       "-I" + str(ROOT / "combo"), "-I" + str(imgui), "-I" + str(build),
                       str(ROOT / "tests/audio_menu/menu_test.cpp")]
            command += [str(imgui / name) for name in ("imgui.cpp", "imgui_draw.cpp", "imgui_tables.cpp", "imgui_widgets.cpp")]
            subprocess.run([*command, "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
