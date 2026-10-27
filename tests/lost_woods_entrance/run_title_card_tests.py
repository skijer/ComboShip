#!/usr/bin/env python3
"""Execute the MM Lost Woods enhancement and native MM title-card lifecycle.

Only hook dispatch, resource availability and GPU setup are fixture boundaries.
No game archive or GPU is required; visual correctness remains an in-game check.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import functions


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    actor = functions((ROOT / "mm/src/code/z_actor.c").read_text(),
                      ("TitleCard_InitBossName", "TitleCard_Update", "TitleCard_Draw"))
    graph = functions((ROOT / "mm/src/code/graph.c").read_text(), ("Graph_OpenDisps", "Graph_CloseDisps"))
    library = functions((ROOT / "mm/src/code/z_lib.c").read_text(), ("Math_StepToS",))
    native = "\n".join([library["Math_StepToS"], graph["Graph_OpenDisps"], graph["Graph_CloseDisps"],
                        *[actor[name] for name in ("TitleCard_InitBossName", "TitleCard_Update", "TitleCard_Draw")]])
    path = ROOT / "mm/2s2h/Enhancements/Graphics/ComboOpeningForestTitleCard.cpp"
    # The baseline intentionally supplies no new hook so the red phase fails
    # on the missing visible behavior, rather than a missing-file/compiler error.
    enhancement = re.sub(r"^#include[^\n]*\n", "", path.read_text(), flags=re.M) if path.exists() else ""
    fixture = (ROOT / "tests/lost_woods_entrance/title_card_test.cpp").read_text()
    fixture = fixture.replace("/* NATIVE_TITLE_CARD */", native).replace("/* ENHANCEMENT */", enhancement)
    # Raw G_SETTIMG uses the low pointer bit as a segmented-address tag. Test
    # guaranteed declaration alignment; a particular link may be even by luck.
    fixture = fixture.replace("/* ROUTED_ALIGNMENT */", '''
    Require(__alignof__(kRoutedLostWoodsCard) >= 2,
            "raw title texture paths must guarantee even alignment for the renderer's pointer tag");
    ''' if path.exists() else "")
    includes = ["mm", "mm/2s2h", "mm/include", "mm/include/PR", "mm/src", "mm/assets",
                "libultraship/include", "combo"]
    flags = ["-std=c++20", "-w", "-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DFMT_HEADER_ONLY",
             *["-I" + str(ROOT / path) for path in includes]]
    if args.sanitize:
        flags += ["-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-sanitize-recover=all", "-no-pie"]
    with tempfile.TemporaryDirectory(prefix="lost-woods-title-") as temporary:
        build = Path(temporary)
        source, binary = build / "test.cpp", build / "title"
        source.write_text(fixture)
        subprocess.run([os.environ.get("CXX", "c++"), *flags, str(source), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})


if __name__ == "__main__":
    main()
