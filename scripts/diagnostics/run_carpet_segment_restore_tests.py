#!/usr/bin/env python3
"""Exercise the native carpet's emitted GBI stream without a game or GPU."""
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile
from run_child_ruto_face_test import function

ROOT = Path(__file__).resolve().parents[2]


def main():
    source_path = ROOT / "soh/src/overlays/actors/ovl_En_Jsjutan/z_en_jsjutan.c"
    production = function(source_path.read_text(), "EnJsjutan_Draw")
    fixture = (ROOT / "soh/tests/carpet_segment_restore_test.c").read_text().replace(
        "/* PRODUCTION_CARPET_DRAW */", production)
    flags = ["-std=gnu11", "-DF3DEX_GBI_2", "-DLOG_LEVEL_GAME_PRINTS=0", "-DNDEBUG",
             "-Werror=implicit-function-declaration", "-Wno-int-conversion",
             "-Wno-incompatible-pointer-types", "-Wno-discarded-qualifiers",
             "-Ilibultraship/include", "-Isoh/include", "-Isoh/src", "-Isoh/assets", "-Isoh", "-Isoh/mods"]
    flags += shlex.split(os.environ.get("CARPET_TEST_CFLAGS", ""))
    for path in ("CMake/soh-cvars.cmake", "CMake/lus-cvars.cmake"):
        for key, value in re.findall(r'set\((CVAR_PREFIX_\w+)\s+"?([^\s"\)]+)', (ROOT / path).read_text()):
            flags.append(f'-D{key}="{value}"')
    with tempfile.TemporaryDirectory(prefix="carpet-segment-") as temporary:
        cfile, binary = Path(temporary) / "draw.c", Path(temporary) / "draw"
        cfile.write_text(fixture)
        subprocess.run([os.environ.get("CC", "cc"), *flags, str(cfile), "-o", str(binary)], cwd=ROOT, check=True)
        subprocess.run([str(binary)], cwd=ROOT, check=True)
        subprocess.run([os.environ.get("CC", "cc"), *flags, "-fsyntax-only", str(source_path)], cwd=ROOT, check=True)
        print("PASS full carpet actor compiles against the baseline's production headers")


if __name__ == "__main__":
    main()
