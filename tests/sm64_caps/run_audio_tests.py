"""Run the unchanged cap state machines against each game's native SFX bank."""
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    failed = False
    for game, folder in (("oot", "soh/expansions/sm64"), ("mm", "mm/2s2h/expansions/sm64")):
        with tempfile.TemporaryDirectory(prefix=f"{game}-cap-audio-") as temp:
            temp = Path(temp)
            source = (ROOT / folder / "sm64_mario_items.c").read_text()
            start = source.index("#define SM64_CAP_SLOT_WING")
            end = source.index("// --- HUD read accessors", start)
            owner = (ROOT / folder / "sm64_mario.c").read_text()
            defines = re.findall(r"^#define SM64_(?:MARIO_\w+|ACT_FLAG_AIR|ACT_PUTTING_ON_CAP)\s+.*", owner, re.M)
            header = (ROOT / folder / "sm64_mario.h").read_text()
            defines += re.findall(r"^#define SM64_CAP_PHASE_\w+\s+.*", header, re.M)
            (temp / "cap_state.inc").write_text("\n".join(defines) + "\n" + source[start:end])
            flags = ["-std=gnu2x", "-DF3DEX_GBI_2", "-DLOG_LEVEL_GAME_PRINTS=0",
                     "-Werror=implicit-function-declaration", "-ffunction-sections", "-fdata-sections",
                     "-g", "-fsanitize=address,undefined", "-fno-pie", "-no-pie", "-I" + str(temp)]
            if game == "mm":
                flags += ["-DTEST_MM"]
                includes = ("mm/include", "mm/include/PR", "mm/src", "mm/assets", "mm/2s2h", "mm")
                sources = ["mm/src/audio/sfx.c", "mm/src/audio/sfx_params.c"]
            else:
                includes = ("soh/include", "soh/src", "soh/assets", "soh")
                sources = ["soh/src/code/code_800F7260.c", "soh/src/code/audio_sound_params.c"]
                for path in ("CMake/soh-cvars.cmake", "CMake/lus-cvars.cmake"):
                    for key, value in re.findall(r'set\((CVAR_PREFIX_\w+)\s+"?([^\s"\)]+)', (ROOT / path).read_text()):
                        flags.append(f'-D{key}="{value}"')
            flags += ["-I" + str(ROOT / p) for p in (*includes, "libultraship/include")]
            binary = temp / "test"
            subprocess.run([os.environ.get("CC", "cc"), *flags,
                            str(ROOT / "tests/sm64_caps/audio_test.c"),
                            *(str(ROOT / p) for p in sources), "-Wl,--gc-sections", "-lm", "-o", str(binary)], check=True)
            result = subprocess.run([str(binary)], timeout=10,
                                    env=dict(os.environ, ASAN_OPTIONS="detect_leaks=0"))
            failed |= result.returncode != 0
    if failed:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
