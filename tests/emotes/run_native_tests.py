#!/usr/bin/env python3
"""Run the production adapters with actual native structs; supply external headers in CPATH."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix="emotes-native-") as temp:
    for game in ("soh", "mm"):
        includes = [game + "/include", game + "/include/PR", game + "/src", game, game + "/assets",
                    "mm/2s2h" if game == "mm" else "soh/soh",
                    "libultraship/include", "libultraship/src", "combo", "combo/menu"]
        flags = ["-DCOMBO_BUILD", "-DCOMBO_EMOTES_MM=" + str(int(game == "mm")), "-DF3DEX_GBI_2",
                 "-DCONTROLLERBUTTONS_T=uint32_t", "-DNON_MATCHING", "-DNON_EQUIVALENT", "-DLOG_LEVEL_GAME_PRINTS=0",
                 "-DFMT_HEADER_ONLY"]
        flags += ["-I" + str(ROOT / p) for p in includes]
        exe = Path(temp) / (game + "_emotes")
        subprocess.run([*shlex.split(os.environ.get("CXX", "c++")), "-std=gnu++20", *flags,
                        str(ROOT / "tests/emotes/native_adapter_test.cpp"), "-o", str(exe)], check=True)
        subprocess.run([str(exe), str(ROOT / game / "assets/custom/emotes/wave.bin")], check=True)
