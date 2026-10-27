#!/usr/bin/env python3
"""Run the complete MM SFX loader against real resource owners and audio types."""
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(path, name):
    source = (ROOT / path).read_text()
    match = re.search(r"^[A-Za-z_][\w:<>, *&]*\s+" + re.escape(name) +
                      r"\([^;{}]*\)\s*\{", source, re.M)
    if match is None:
        raise RuntimeError(f"Missing production function: {name}")
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end]


def main():
    with tempfile.TemporaryDirectory(prefix="mm-sfx-lifetime-") as temporary:
        build = Path(temporary)
        (build / "libultraship/log").mkdir(parents=True)
        (build / "libultraship/libultraship.h").write_text(
            "#pragma once\n#include <libultraship/libultra.h>\n")
        (build / "libultraship/log/luslog.h").write_text(
            "#pragma once\n#define LUSLOG_LEVEL_INFO 2\n#define lusprintf(...) ((void)0)\n")
        (build / "spdlog").mkdir()
        (build / "spdlog/spdlog.h").write_text("#pragma once\n#define SPDLOG_TRACE(...) ((void)0)\n")
        folder = "soh/mods/sound_translator/"
        playback = (ROOT / (folder + "mm_sfx_synth_playback.cpp")).read_text()
        error_macros = "\n".join(line for line in playback.splitlines() if line.startswith("#define AUDIO_ERROR"))
        (build / "instrument_read.inc").write_text("\n".join((error_macros,
            function(folder + "mm_sfx_synth_glue.cpp", "AudioLoad_IsFontLoadComplete"),
            function(folder + "mm_sfx_synth_playback.cpp", "AudioPlayback_GetInstrumentInner"),
            function(folder + "mm_sfx_synth_seqplayer.cpp", "AudioScript_GetInstrument"),
        )))
        assets = "soh/mods/transformation_masks/assets/mm_asset_loader.cpp"
        (build / "resident_load.inc").write_text("\n".join((
            function(assets, "MmAssets_LoadResourceObjectFromMmArchive"),
            function(assets, "MmAssets_LoadFromMmArchive"),
            function(assets, "MmSfx_LoadSequence"),
        )))
        binary = build / "loader_test"
        subprocess.run([
            *shlex.split(os.environ.get("CXX", "c++")), "-std=c++20", "-g", "-O1",
            "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
            "-fno-omit-frame-pointer", "-DF3DEX_GBI_2",
            "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections",
            "-I" + str(build), "-I" + str(ROOT / "soh"),
            "-I" + str(ROOT / "soh/include"), "-I" + str(ROOT / "soh/assets"),
            "-I" + str(ROOT / "libultraship/include"),
            str(ROOT / "tests/mm_sfx_lifetime/loader_test.cpp"),
            str(ROOT / "soh/soh/resource/type/AudioSoundFont.cpp"),
            str(ROOT / "soh/soh/resource/type/AudioSequence.cpp"),
            str(ROOT / "libultraship/src/ship/resource/Resource.cpp"),
            "-o", str(binary),
        ], check=True)
        for case in ("evicted-font", "evicted-sequence", "native-owner", "meta-owner", "missing-assets", "retry"):
            result = subprocess.run([str(binary), case], capture_output=True, text=True,
                                    env=dict(os.environ, ASAN_OPTIONS="detect_leaks=0"), timeout=10)
            print(result.stdout.strip(), flush=True)
            if result.returncode:
                print(result.stderr, flush=True)
                raise SystemExit(f"FAIL {case} (exit {result.returncode})")
    subprocess.run([sys.executable, "-B", str(ROOT / "tests/mm_sfx_lifetime/run_parser_tests.py")], check=True)


if __name__ == "__main__":
    main()
