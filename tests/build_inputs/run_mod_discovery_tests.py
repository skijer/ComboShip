#!/usr/bin/env python3
"""Use each native CMake mod glob to verify discovery after configuration.

The small project tests CMake's real incremental regeneration, independently of
the SDK/application build and without compiling substitute game code.
"""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
for host in ("soh", "mm"):
    native = (ROOT / host / "CMakeLists.txt").read_text()
    glob = re.search(r"^file\(GLOB_RECURSE mods__.*\)$", native, re.M).group(0)
    with tempfile.TemporaryDirectory(prefix="combo-mod-discovery-") as temp:
        source, build = Path(temp) / "source", Path(temp) / "build"
        modules = source / "mods/transformation_masks"
        modules.mkdir(parents=True)
        (modules / "existing.cpp").write_text("// present at initial configure\n")
        (source / "CMakeLists.txt").write_text(
            'cmake_minimum_required(VERSION 3.26)\nproject(ModDiscovery NONE)\n' + glob +
            '\nlist(SORT mods__)\nfile(WRITE "${CMAKE_CURRENT_BINARY_DIR}/mods.list" "${mods__}")\n'
            'add_custom_target(check_discovery ALL COMMAND "${CMAKE_COMMAND}" -E compare_files '
            '"${CMAKE_CURRENT_BINARY_DIR}/mods.list" "${CMAKE_CURRENT_BINARY_DIR}/expected.list")\n')
        subprocess.run(["cmake", "-S", str(source), "-B", str(build), "-G", "Ninja"], check=True)
        expected = build / "expected.list"
        expected.write_text("mods/transformation_masks/existing.cpp")
        subprocess.run(["cmake", "--build", str(build)], check=True)
        # Both newly introduced hook modules must enter a reused build without
        # requiring the user to remember a manual CMake reconfigure.
        for name in ("wolf_link_audio_hooks.cpp", "mask_progression.cpp"):
            (modules / name).write_text("// added after configure\n")
        expected.write_text(";".join("mods/transformation_masks/" + name for name in
                           sorted(("existing.cpp", "wolf_link_audio_hooks.cpp", "mask_progression.cpp"))))
        subprocess.run(["cmake", "--build", str(build)], check=True)
        print("PASS native " + host + " CMake discovers new mod hooks in an existing build")
