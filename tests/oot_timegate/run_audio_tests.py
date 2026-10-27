"""Run the complete OoT Time Gate handler against the native OoT SFX engine."""
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    flags = [
        "-std=gnu11", "-DF3DEX_GBI_2", "-DLOG_LEVEL_GAME_PRINTS=0",
        "-Werror=implicit-function-declaration", "-Wno-incompatible-pointer-types",
        "-Wno-discarded-qualifiers", "-Wno-discarded-array-qualifiers",
        "-ffunction-sections", "-fdata-sections",
        "-g", "-fsanitize=address,undefined", "-fno-pie", "-no-pie",
    ]
    for path in ("CMake/soh-cvars.cmake", "CMake/lus-cvars.cmake"):
        for key, value in re.findall(
            r'set\((CVAR_PREFIX_\w+)\s+"?([^\s"\)]+)', (ROOT / path).read_text()
        ):
            flags.append(f'-D{key}="{value}"')
    flags += ["-I" + str(ROOT / path) for path in (
        "soh/include", "soh/src", "soh/assets", "soh", "libultraship/include",
    )]
    with tempfile.TemporaryDirectory(prefix="oot-timegate-audio-") as directory:
        binary = Path(directory) / "timegate-audio"
        subprocess.run([
            os.environ.get("CC", "cc"), *flags,
            str(ROOT / "tests/oot_timegate/audio_test.c"),
            str(ROOT / "soh/src/code/code_800F7260.c"),
            str(ROOT / "soh/src/code/audio_sound_params.c"),
            "-Wl,--gc-sections", "-lm", "-o", str(binary),
        ], check=True)
        subprocess.run([str(binary)], check=True, timeout=10,
                       env=dict(os.environ, ASAN_OPTIONS="detect_leaks=0"))


if __name__ == "__main__":
    main()
