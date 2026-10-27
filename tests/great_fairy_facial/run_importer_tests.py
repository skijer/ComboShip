#!/usr/bin/env python3
"""Compile the real MM texture-animation importer/reader and test owned paths."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[2]
SOURCES = [
    "tests/great_fairy_facial/texture_cycles_test.cpp",
    "mm/2s2h/resource/importer/TextureAnimationFactory.cpp",
    "mm/2s2h/resource/type/TextureAnimation.cpp",
    "libultraship/src/ship/resource/ResourceFactoryBinary.cpp",
    "libultraship/src/ship/resource/Resource.cpp",
    "libultraship/src/ship/utils/binarytools/BinaryReader.cpp",
    "libultraship/src/ship/utils/binarytools/MemoryStream.cpp",
    "libultraship/src/ship/utils/binarytools/Stream.cpp",
]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--candidate", type=Path)
    parser.add_argument("--native", type=Path)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="mm-great-fairy-importer-") as directory:
        temp = Path(directory)
        log = temp / "spdlog/spdlog.h"
        log.parent.mkdir()
        # Logging is the sole replaced service; parser, containers, ownership,
        # native parameter structs and binary readers are production code.
        log.write_text("#pragma once\n" + "".join("#define SPDLOG_%s(...) ((void)0)\n" % level
                       for level in ["TRACE", "DEBUG", "INFO", "WARN", "ERROR", "CRITICAL"]))
        binary = temp / "importer_test"
        command = [os.environ.get("CXX", "c++"), "-std=c++20", "-O1", "-g",
                   "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer",
                   "-DF3DEX_GBI_2", "-I" + str(temp), "-I" + str(ROOT / "mm"),
                   "-I" + str(ROOT / "libultraship/include"),
                   "-idirafter", str(ROOT / "libultraship/include/libultraship/libultra"),
                   *[str(ROOT / source) for source in SOURCES], "-o", str(binary)]
        subprocess.run(command, cwd=ROOT, check=True)
        inputs = []
        for label, path, prefix in [("candidate", args.candidate, "alt/"), ("native", args.native, "")]:
            if path is None:
                continue
            assert label != "native" or args.candidate, "Pass candidate before native"
            with zipfile.ZipFile(path) as archive:
                target = temp / (label + ".bin")
                target.write_bytes(archive.read(prefix + "objects/object_dy_obj/gGreatFairyAppearenceTexAnim"))
                inputs.append(str(target))
        env = os.environ.copy()
        # LeakSanitizer is unsupported under this host's ptrace supervisor.
        # Address/undefined sanitizers still check accesses and destruction.
        env["ASAN_OPTIONS"] = env.get("ASAN_OPTIONS", "") + ":detect_leaks=0"
        subprocess.run([str(binary), *inputs], check=True, env=env)


if __name__ == "__main__":
    main()
