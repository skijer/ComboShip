#!/usr/bin/env python3
"""Run the production enchantment selector against both real game ABIs."""
from pathlib import Path
import os
import re
import subprocess
import tempfile
import sys
from run_mm_din_fire_tests import FLAGS as MM_FLAGS

ROOT = Path(__file__).resolve().parents[2]

def main():
    soh = ["-std=gnu2x", "-O1", "-g", "-DF3DEX_GBI_2", "-DLOG_LEVEL_GAME_PRINTS=0",
           "-Werror=implicit-function-declaration"]
    soh += ["-I" + str(ROOT / p) for p in ("soh/include", "soh/src", "soh/assets", "soh", "libultraship/include")]
    for path in ("CMake/soh-cvars.cmake", "CMake/lus-cvars.cmake"):
        for key, value in re.findall(r'set\((CVAR_PREFIX_\w+)\s+"?([^\s"\)]+)', (ROOT / path).read_text()):
            soh.append(f'-D{key}="{value}"')
    combat = "--combat" in sys.argv
    with tempfile.TemporaryDirectory(prefix="sword-element-state-") as directory:
        variants = (
            ("soh", "solo", soh),
            ("soh", "combo", [*soh, "-DCOMBO_BUILD"]),
            ("mm", "solo", [f for f in MM_FLAGS if f not in ("-DCOMBO_BUILD", "-DMM_BUILD_DLL")]),
            ("mm", "combo", MM_FLAGS),
        )
        for engine, mode, flags in variants:
            if engine == "mm":
                flags = [*flags, "-DSWORD_ELEMENT_TEST_MM"]
            source = ROOT / engine / "src/code/sword_element.c"
            binary = str(Path(directory) / (engine + "-" + mode))
            command = [os.environ.get("CC", "cc"), *flags, "tests/elemental_swords/state_test.c"]
            if "--sanitize" in sys.argv:
                command += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                            "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
            if combat:
                command += ["-DSWORD_ELEMENT_TEST_COMBAT", "-ffunction-sections", "-fdata-sections",
                            str(ROOT / engine / "src/code/din_fire_sword.c"), "-Wl,--gc-sections"]
            if source.exists():
                command.append(str(source))
            subprocess.run([*command, "-lm", "-o", binary], cwd=ROOT, check=True)
            subprocess.run([binary], check=True)
        menu = [os.environ.get("CXX", "c++"), "-std=c++20", "-I" + str(ROOT / "combo"),
                "-I" + str(ROOT / "libultraship/include")]
        if "--sanitize" in sys.argv:
            menu += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                     "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
        menu += [str(ROOT / "tests/elemental_swords/override_menu_test.cpp"),
                 "-o", str(Path(directory) / "override-menu")]
        subprocess.run(menu, cwd=ROOT, check=True)
        subprocess.run([menu[-1]], check=True)

if __name__ == "__main__":
    main()
