#!/usr/bin/env python3
"""Exercise the OoT equipment grid and actual GUI texture upload methods.

Archive lookup, Context/window construction and GPU submission are controlled
boundaries. The cell UI, icon selection, ownership setters, resource scope,
registry and Fast3dGui conversion/cache methods are production code. Pixels use
real Fast::Texture objects and real Dear ImGui input handling.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, signature):
    start = source.index(signature)
    cursor = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[cursor] == '{') - (source[cursor] == '}')
        cursor += 1
    return source[start:cursor]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-rev', help='read the OoT editor from an exact earlier revision for RED')
    args = parser.parse_args()
    path = 'soh/soh/Enhancements/debugger/debugSaveEditor.cpp'
    editor = (subprocess.check_output(['git', 'show', args.source_rev + ':' + path], cwd=ROOT, text=True)
              if args.source_rev else (ROOT / path).read_text())
    gui = (ROOT / 'libultraship/src/fast/Fast3dGui.cpp').read_text()
    equipment = (ROOT / 'soh/mods/extended_equipment.c').read_text()
    scope = (ROOT / 'libultraship/include/ship/resource/ResourceManagerScope.h').read_text()
    registry = (ROOT / 'libultraship/src/ship/resource/CrossRMRegistry.cpp').read_text()

    production = scope[scope.index('namespace Ship {'):]
    production += registry[registry.index('namespace Ship {'):]
    for signature in ('ImTextureID Fast3dGui::GetTextureById(', 'bool Fast3dGui::HasTextureByName(',
                      'ImTextureID Fast3dGui::GetTextureByName(', 'ImVec2 Fast3dGui::GetTextureSize(',
                      'void Fast3dGui::LoadGuiTexture(const std::string& name, const Fast::Texture&',
                      'void Fast3dGui::LoadGuiTexture(const std::string& name, const std::string& path',
                      'void Fast3dGui::UnloadTexture('):
        production += '\nnamespace Fast {\n' + function(gui, signature) + '\n}\n'
    for signature in ('static u32 ExtEquip_GetBit(', 'u8 ExtEquip_HasItem(', 'void ExtEquip_GiveItem(',
                      'void ExtEquip_RemoveItem(', 'void* ExtEquip_GetIcon('):
        production += '\n' + function(equipment, signature)

    helper = 'static bool NeiEditor_RegisterIkanaIcon('
    if helper in editor:
        production += '\n' + function(editor, helper)
    # Keep both the actual registration path and the actual cell interactions.
    labels = re.search(r'static const char\* extEquipNames\[4\]\[3\] = \{.*?\n        };', editor, re.S)[0]
    start = editor.index('            // Draw equipment grid: 4 rows x 3 columns')
    end = editor.index('            // Skijer 2026-07-15: the upgrade-column passive toggles', start)
    production += '\nvoid DrawGrid() {\n' + labels + '\n' + editor[start:end] + '\n}\n'

    imgui = Path(os.environ['IMGUI_DIR'])
    cxx = shlex.split(os.environ.get('CXX', 'c++'))
    with tempfile.TemporaryDirectory(prefix='ikana-oot-ui-icon-') as temporary:
        build = Path(temporary)
        (build / 'oot_icon_production.inc').write_text(production)
        (build / 'spdlog').mkdir()
        (build / 'spdlog/spdlog.h').write_text('#pragma once\n#define SPDLOG_TRACE(...) ((void)0)\n')
        common = ['-std=c++20', '-DIMGUI_ENABLE_TEST_ENGINE', '-DImTextureID=void*',
                  '-I' + str(build), '-I' + str(imgui), '-I' + str(ROOT),
                  '-I' + str(ROOT / 'libultraship/include')]
        # Build the unchanged input/conversion dependencies once for both game modes.
        sources = [imgui / name for name in ('imgui.cpp', 'imgui_draw.cpp', 'imgui_tables.cpp', 'imgui_widgets.cpp')]
        sources += [ROOT / 'libultraship/src/ship/resource/Resource.cpp',
                    ROOT / 'libultraship/src/fast/resource/type/Texture.cpp']
        def compile_dependency(index_source):
            index, source = index_source
            obj = build / f'dependency-{index}.o'
            subprocess.run([*cxx, *common, '-c', str(source), '-o', str(obj)], check=True)
            return str(obj)
        with ThreadPoolExecutor(max_workers=4) as pool:
            objects = list(pool.map(compile_dependency, enumerate(sources)))
        for name, flags in (('combo', ['-DCOMBO_BUILD']), ('standalone', [])):
            binary = build / name
            subprocess.run([*cxx, *common, '-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter',
                            *flags, str(ROOT / 'tests/ikana_editor_icon/oot_ui_icon_test.cpp'),
                            *objects, '-o', str(binary)], check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == '__main__':
    main()
