"""Compile real blur/color and barrier/audio logic with actual game/bank types.

Audio output and archive/GPU services are boundaries; this does not listen to
the game or prove visual compatibility with a model pack.
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
    cases = sys.argv[1:] or ["colors", "mm-fin", "barrier", "swim", "land", "cadence"]
    source = (ROOT / "soh/mods/transformation_masks/mm_player_form.cpp").read_text()
    common = ["-O1", "-g", "-fno-pie", "-no-pie", "-fsanitize=address,undefined",
              "-fno-sanitize-recover=all", "-ffunction-sections", "-fdata-sections"]
    env = dict(os.environ, ASAN_OPTIONS="detect_leaks=0")
    with tempfile.TemporaryDirectory(prefix="zora-presentation-") as temporary:
        directory = Path(temporary)
        includes = ["-I" + temporary]
        if "colors" in cases:
            blur = (ROOT / "soh/src/code/z_eff_blure.c").read_text()
            (directory / "zora_blure.inc").write_text("\n".join(function(blur, name) for name in [
                "EffectBlure_InitElements", "EffectBlure_Init2", "EffectBlure_Update"]))
            binary = str(directory / "colors")
            subprocess.run([os.environ.get("CC", "cc"), *flags(), *includes, *common,
                            str(ROOT / "tests/oot_zora/cosmetic_test.c"), "-lm", "-o", binary], check=True)
            subprocess.run([binary], check=True, env=env, timeout=10)
        if "mm-fin" in cases:
            (directory / "mm_fin.inc").write_text(function(
                (ROOT / "mm/src/code/z_eff_blure.c").read_text(), "EffectBlure_GetComputedValues"))
            binary = str(directory / "mm-fin")
            mm_flags = ["-std=gnu11", "-DF3DEX_GBI_2", "-DLOG_LEVEL_GAME_PRINTS=0"]
            mm_flags += ["-I" + str(ROOT / path) for path in (
                "mm/include", "mm/src", "mm/assets", "mm", "mm/2s2h", "libultraship/include")]
            subprocess.run([os.environ.get("CC", "cc"), *mm_flags, *includes, *common,
                            str(ROOT / "tests/oot_zora/mm_fin_test.c"), "-lm", "-o", binary], check=True)
            subprocess.run([binary], check=True, env=env, timeout=10)
        audio_cases = [case for case in cases if case not in ("colors", "mm-fin")]
        if audio_cases:
            enum = re.search(r"typedef enum GoronActionId \{.*?\} GoronActionId;", source, re.S).group(0)
            bodies = enum + "\n" + "\n".join(function(source, name) for name in [
                "MmForm_PlaySfx", "MmForm_UpdateBarrier"])
            cleanup = re.search(r"^static void MmForm_UpdateZoraSwimSfx\(", source, re.M)
            if cleanup:
                bodies += "\n" + function(source, "MmForm_UpdateZoraSwimSfx")
            bodies += "\n" + function((ROOT / "soh/src/code/code_800EC960.c").read_text(), "Audio_SetBaseFilter")
            assets = (ROOT / "soh/mods/transformation_masks/assets/mm_asset_loader.cpp").read_text()
            clock = re.search(r"^void MmSfx_UpdateGameFrame\(", assets, re.M)
            if clock:
                bodies += "\n" + function(assets, "MmSfx_UpdateGameFrame")
                native = function((ROOT / "soh/src/code/code_800EC960.c").read_text(), "Audio_Update")
                assert "MmSfx_UpdateGameFrame();" in native, "MM bank clock must follow native graph audio update"
            # Execute the actual mixer prefix, through synthesis dispatch. Its
            # later PCM sample loop belongs to a different test fixture.
            mixer = function(assets, "MmDirectAudio_MixInto")
            bodies += "\n" + mixer[:mixer.index("    // Apply SoH master volume")] + "}\n"
            (directory / "zora_audio.inc").write_text(bodies)
            binary = str(directory / "audio")
            cxx_flags = ["-std=gnu++20", "-I" + str(ROOT / "tests/nei_leaf/stubs")]
            cxx_flags += [flag for flag in flags()
                          if not flag.startswith("-std=") and flag != "-Wno-incompatible-pointer-types"]
            cxx_flags += includes
            if cleanup:
                cxx_flags += ["-DZORA_AUDIO_CLEANUP"]
            if clock:
                cxx_flags += ["-DZORA_AUDIO_GAME_CLOCK"]
            engine = str(directory / "engine.cpp")
            Path(engine).write_text('''#include "mods/sound_translator/mm_audio_sfx.h"
#undef SFX_FLAG
#undef SFX_BANK_SHIFT
#undef SFX_BANK_MASK
#undef SFX_INDEX
#undef SFX_BANK
#include "mods/sound_translator/mm_audio_sfx.cpp"
''')
            subprocess.run([os.environ.get("CXX", "c++"), *cxx_flags, *common,
                            str(ROOT / "tests/oot_zora/audio_test.cpp"), engine,
                            str(ROOT / "soh/mods/sound_translator/mm_audio_sfx_params.cpp"),
                            "-Wl,--gc-sections", "-lm", "-o", binary], check=True)
            for case in audio_cases:
                subprocess.run([binary, case], check=True, env=env, timeout=10)

if __name__ == "__main__":
    main()
