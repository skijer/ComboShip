#!/usr/bin/env python3
"""Run the native ResourceLoader and audio importers with controlled archive I/O.

The complete production ResourceLoader, binary reader, factories and resource
owners are compiled. Only Context/global mount I/O and archive opening are seams;
the MM helper and native binary importer bodies are copied verbatim at build time.
Dependency headers must be available (the diagnostic SDK can supply them).
tinyxml2 uses the repository's bundled source.
"""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

from run_tests import ROOT, function


def main():
    with tempfile.TemporaryDirectory(prefix="mm-sfx-native-parser-") as temporary:
        build = Path(temporary)
        (build / "spdlog").mkdir()
        (build / "spdlog/spdlog.h").write_text(
            "#pragma once\n#define SPDLOG_TRACE(...) ((void)0)\n#define SPDLOG_ERROR(...) ((void)0)\n")
        (build / "native_audio_importers.inc").write_text("\n".join((
            function("soh/soh/resource/importer/AudioSoundFontFactory.cpp",
                     "ResourceFactoryBinaryAudioSoundFontV2::ReadResource"),
            function("soh/soh/resource/importer/AudioSequenceFactory.cpp",
                     "ResourceFactoryBinaryAudioSequenceV2::ReadResource"),
        )))
        (build / "native_asset_load.inc").write_text(function(
            "soh/mods/transformation_masks/assets/mm_asset_loader.cpp",
            "MmAssets_LoadResourceObjectFromMmArchive"))
        binary = build / "resource_parser_test"
        native = "libultraship/src/ship/"
        sources = [
            "tests/mm_sfx_lifetime/resource_parser_test.cpp",
            native + "resource/ResourceLoader.cpp",
            native + "resource/ResourceFactoryBinary.cpp",
            native + "resource/Resource.cpp",
            native + "resource/factory/JsonFactory.cpp",
            native + "resource/factory/ShaderFactory.cpp",
            native + "resource/type/Json.cpp",
            native + "resource/type/Shader.cpp",
            native + "utils/binarytools/Stream.cpp",
            native + "utils/binarytools/MemoryStream.cpp",
            native + "utils/binarytools/BinaryReader.cpp",
            "soh/soh/resource/type/AudioSoundFont.cpp",
            "soh/soh/resource/type/AudioSequence.cpp",
            "ZAPDTR/lib/tinyxml2/tinyxml2.cpp",
        ]
        subprocess.run([
            *shlex.split(os.environ.get("CXX", "c++")), "-std=c++20", "-g", "-O1",
            "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
            "-fno-omit-frame-pointer", "-fno-pie", "-no-pie", "-DCOMBO_BUILD",
            "-I" + str(ROOT / "tests/mm_sfx_lifetime/parser_stubs"),
            "-I" + str(build), "-I" + str(ROOT / "soh"),
            "-I" + str(ROOT / "soh/include"), "-I" + str(ROOT / "libultraship/include"),
            "-I" + str(ROOT / "ZAPDTR/lib/tinyxml2"),
            *[str(ROOT / source) for source in sources], "-o", str(binary),
        ], check=True)
        for case in ("global-font-meta", "global-sequence-meta", "local-meta-headed",
                     "local-meta-headerless", "missing-local-meta-target",
                     "local-meta-default-path", "native-headers"):
            result = subprocess.run([str(binary), case], capture_output=True, text=True,
                                    env=dict(os.environ, ASAN_OPTIONS="detect_leaks=0"), timeout=10)
            print(result.stdout.strip(), flush=True)
            if result.returncode:
                print(result.stderr, flush=True)
                raise SystemExit(f"FAIL parser {case} (exit {result.returncode})")


if __name__ == "__main__":
    main()
