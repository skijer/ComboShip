#!/usr/bin/env python3
"""Run OoT form lifetime entrypoints against the real MM sound bank.

The actual update dispatcher and cleanup bodies are compiled on engine types.
Rendering, assets, unrelated form actions and audio output are boundaries. Init
and Reset run their sound/effect cleanup prefixes; their later asset/equipment
work is outside this regression. This does not replace listening in game.
"""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/nei_item_stow"))
from run_ballchain_tests import flags


def function(source, name):
    match = re.search(r"^[^\n{};]+\b" + re.escape(name) + r"\([^;{}]*\)\s*\{", source, re.M)
    assert match, "Missing production function: " + name
    cursor, depth = match.end(), 1
    while depth:
        depth += (source[cursor] == "{") - (source[cursor] == "}")
        cursor += 1
    return source[match.start():cursor]


def main():
    source = (ROOT / "soh/mods/transformation_masks/mm_player_form.cpp").read_text()
    assets = (ROOT / "soh/mods/transformation_masks/assets/mm_asset_loader.cpp").read_text()
    pieces = []
    for enum in ("MmFormStateId", "GoronActionId"):
        pieces.append(re.search(r"typedef enum " + enum + r" \{.*?\} " + enum + ";", source, re.S).group(0))
    pieces.append(function(assets, "MmDirectAudio_StopById"))
    pieces.append(function(assets, "MmSfx_Stop"))
    native = ROOT / "soh/mods/transformation_masks/mm_native_transform.inc.h"
    if native.exists(): pieces.append(function(native.read_text(), "MmForm_StopNativeMaskSfx"))
    pieces.append(function(source, "MmForm_StopGoronRollSfx"))
    for name in ("MmForm_UpdateGoronRollSfx", "MmForm_RegisterGoronRollSfxCleanup"):
        if re.search(r"^static void " + name + r"\(", source, re.M):
            pieces.append(function(source, name))
    init = function(source, "MmForm_Init")
    pieces.append(init[:init.index("    // === Cleanup from previous scene ===")] + "}\n")
    reset = function(source, "MmForm_Reset")
    pieces.append(reset[:reset.index("    MmForm_ClearZoraSwimTrails")] + "}\n")
    pieces.extend(function(source, name) for name in ("MmForm_OnDeath", "MmForm_Update"))
    pieces.append(function((ROOT / "soh/src/code/z_actor.c").read_text(), "Actor_Destroy"))

    cases = sys.argv[1:] or ["scene-init", "reset", "death", "destroy", "void", "transition-trigger",
                           "transition-mode", "room", "action-exit", "form-exit", "unavailable", "queued",
                           "rolling", "transformation-cues"]
    with tempfile.TemporaryDirectory(prefix="goron-audio-") as temp:
        directory = Path(temp)
        sound_types = assets[assets.index("#define MM_DIRECT_MAX_SOUNDS"):]
        sound_types = sound_types[:sound_types.index("} MmPlayingSound;") + len("} MmPlayingSound;")]
        (directory / "goron_audio_types.inc").write_text(sound_types)
        (directory / "goron_audio.inc").write_text("\n".join(pieces))
        engine = directory / "engine.cpp"
        engine.write_text('''#include "mods/sound_translator/mm_audio_sfx.h"
#undef SFX_FLAG
#undef SFX_BANK_SHIFT
#undef SFX_BANK_MASK
#undef SFX_INDEX
#undef SFX_BANK
#include "mods/sound_translator/mm_audio_sfx.cpp"
''')
        includes = [flag for flag in flags()
                    if not flag.startswith("-std=") and flag != "-Wno-incompatible-pointer-types"]
        binary = str(directory / "audio")
        subprocess.run([os.environ.get("CXX", "c++"), "-std=gnu++20",
                        "-I" + str(ROOT / "tests/nei_leaf/stubs"), *includes, "-I" + temp,
                        "-O1", "-g", "-fno-pie", "-no-pie", "-fsanitize=address,undefined",
                        "-fno-sanitize-recover=all", "-ffunction-sections", "-fdata-sections",
                        str(ROOT / "tests/goron_audio/lifetime_test.cpp"), str(engine),
                        str(ROOT / "soh/mods/sound_translator/mm_audio_sfx_dispatch.cpp"),
                        str(ROOT / "soh/mods/sound_translator/mm_audio_sfx_params.cpp"),
                        "-Wl,--gc-sections", "-lm", "-o", binary], check=True)
        failed = []
        for mode in ("sequence", "direct"):
            for case in cases:
                result = subprocess.run([binary, case, mode],
                                        env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"}, timeout=10)
                if result.returncode:
                    failed.append(mode + "/" + case)
        if failed:
            raise SystemExit("FAIL Goron audio: " + ", ".join(failed))


if __name__ == "__main__":
    main()
