#!/usr/bin/env python3
"""Evaluate the production launcher target and compile its Windows resources."""
import argparse
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def checked(command, **kwargs):
    result = subprocess.run(command, capture_output=True, text=True, **kwargs)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    return result.stdout


def graph(directory, system, cmake):
    source = directory / (system + "-source")
    build = directory / (system + "-build")
    source.mkdir()
    (source / "CMake").mkdir()
    shutil.copy2(ROOT / "CMake/NeiWolfAsset.cmake", source / "CMake/NeiWolfAsset.cmake")
    imgui = source / "imgui/misc/cpp"
    imgui.mkdir(parents=True)
    (imgui / "imgui_stdlib.cpp").write_text("// dependency boundary; not compiled\n")
    (source / "stub.cpp").write_text("// dependency boundary; not compiled\n")
    # Only package/library boundaries are fixtures. Execute the complete real
    # combo/CMakeLists.txt to obtain target SOURCES and its configured resource.
    harness = r'''
cmake_minimum_required(VERSION 3.26)
project(LauncherResourceGraph LANGUAGES C CXX)
set(CMAKE_SYSTEM_NAME "@SYSTEM@")
set(COMBO_RELEASE_VERSION "0.4.0")
set(COMBO_PROJECT_TEAM "github.com/Varuuna/ComboShip")
set(NEI_WOLF_LINK_ASSET "")
foreach(target soh 2ship libultraship)
  add_library(${target} STATIC EXCLUDE_FROM_ALL "${CMAKE_CURRENT_SOURCE_DIR}/stub.cpp")
endforeach()
foreach(target nlohmann_json::nlohmann_json SDL2::SDL2 SDL2_net::SDL2_net)
  add_library(${target} INTERFACE IMPORTED GLOBAL)
endforeach()
function(find_package)
endfunction()
include(FetchContent)
FetchContent_Declare(ImGui SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/imgui")
FetchContent_SetPopulated(ImGui SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/imgui"
                         BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/imgui")
add_subdirectory("@ROOT@/combo" "${CMAKE_CURRENT_BINARY_DIR}/combo")
get_target_property(sources ComboShip SOURCES)
set(resources "")
foreach(path IN LISTS sources)
  if(path MATCHES "\\.rc$")
    get_filename_component(path "${path}" ABSOLUTE BASE_DIR "@ROOT@/combo")
    list(APPEND resources "${path}")
  endif()
endforeach()
list(JOIN resources "\n" resources)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/resources.txt" "${resources}")
'''
    harness = harness.replace("@SYSTEM@", system).replace("@ROOT@", ROOT.as_posix())
    (source / "CMakeLists.txt").write_text(harness)
    checked([cmake, "-S", str(source), "-B", str(build)])
    paths = [Path(path) for path in (build / "resources.txt").read_text().splitlines()]
    return paths


def resource_key(data, at):
    value = struct.unpack_from("<H", data, at)[0]
    if value == 0xFFFF:
        return struct.unpack_from("<H", data, at + 2)[0], at + 4
    end = at
    while struct.unpack_from("<H", data, end)[0]:
        end += 2
    return data[at:end].decode("utf-16le"), end + 2


def records(path):
    data = path.read_bytes()
    result = {}
    at = 0
    while at < len(data):
        size, header = struct.unpack_from("<II", data, at)
        kind, cursor = resource_key(data, at + 8)
        name, cursor = resource_key(data, cursor)
        cursor = (cursor + 3) & ~3
        language = struct.unpack_from("<H", data, cursor + 6)[0]
        if size:
            key = (kind, name, language)
            assert key not in result, "duplicate compiled resource " + str(key)
            result[key] = data[at + header:at + header + size]
        at = (at + header + size + 3) & ~3
    return result


def compile_resource(resource, directory, zig):
    output = directory / "launcher.res"
    if zig:
        checked([zig, "rc", "/fo", str(output), "--", str(resource)])
    else:
        compiler = shutil.which("rc")
        assert compiler, "Windows resource compiler rc.exe is required"
        checked([compiler, "/nologo", "/fo", str(output), str(resource)])
    compiled = records(output)
    icons = [payload for (kind, _, _), payload in compiled.items() if kind == 3]
    groups = [key for key in compiled if key[0] == 14]
    versions = [key for key in compiled if key[0] == 16]
    original = (ROOT / "combo/FLEET.ico").read_bytes()
    count = struct.unpack_from("<H", original, 4)[0]
    images = []
    for index in range(count):
        size, start = struct.unpack_from("<II", original, 6 + 16 * index + 8)
        images.append(original[start:start + size])
    assert len(groups) == 1 and len(versions) == 1
    assert sorted(icons) == sorted(images), "compiled Fleet icon images changed"
    # Link the exact resource object into a minimal Windows EXE. The full game
    # DLLs are not needed to reproduce the launcher's resource-conversion error.
    main = directory / "main.c"
    main.write_text("int main(void) { return 0; }\n")
    exe = directory / "launcher-resource-test.exe"
    if zig:
        checked([zig, "cc", "-target", "x86_64-windows-gnu", str(main), str(output), "-o", str(exe)])
    else:
        obj = directory / "main.obj"
        checked(["cl", "/nologo", "/c", str(main), "/Fo" + str(obj)])
        checked(["link", "/nologo", "/subsystem:console", "/out:" + str(exe), str(obj), str(output)])
    assert exe.read_bytes()[:2] == b"MZ"
    print("PASS Windows resource compile/link: one icon group, exact Fleet images, version info, PE executable")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cmake", default="cmake")
    parser.add_argument("--zig", help="Optional native-host Zig for Windows resource compile/link")
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="combo-launcher-resource-") as temporary:
        directory = Path(temporary)
        windows = graph(directory, "Windows", args.cmake)
        assert len(windows) == 1, "launcher has duplicate resource inputs: " + repr(windows)
        generated = windows[0].read_text()
        assert generated.count(" ICON ") == 1
        assert (ROOT / "combo/FLEET.ico").as_posix() in generated
        assert re.search(r"FILEVERSION\s+0,4,0,0", generated)
        assert re.search(r"PRODUCTVERSION\s+0,4,0,0", generated)
        assert not graph(directory, "Linux", args.cmake), "Windows resources leaked into non-Windows target"
        print("PASS production launcher graph: one Windows resource, Fleet icon/version metadata, no Linux resources")
        if args.zig or os.name == "nt":
            compile_resource(windows[0], directory, args.zig)


if __name__ == "__main__":
    main()
