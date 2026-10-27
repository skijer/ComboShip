#!/usr/bin/env python3
"""Exercise actual OoT foliage draws with native structs and GBI commands."""
from pathlib import Path
import argparse
import os
import re
import subprocess
import tempfile
from run_soh_season_weather_tests import extract


def run(output, sanitize):
    root = Path(__file__).resolve().parents[2]
    kusa = root / "soh/src/overlays/actors/ovl_En_Kusa/z_en_kusa.c"
    wood = root / "soh/src/overlays/actors/ovl_En_Wood02/z_en_wood02.c"
    source = wood.read_text()
    body = re.search(r"typedef enum \{[^}]*} WoodDrawType;", source, re.S)[0] + "\n"
    for name in ["D_80B3BF54", "D_80B3BF70"]:
        match = re.search(r"static Gfx\* " + name + r"\[\] = \{.*?\n};", source, re.S)
        assert match, name
        body += match[0] + "\n"
    body += extract(kusa, ["EnKusa_Draw"])
    body += extract(wood, ["EnWood02_Draw"])
    (output / "season_actor_production.inc").write_text(body)
    flags = ["-std=gnu11", "-O1", "-g", "-DNDEBUG", "-DF3DEX_GBI_2", "-DCOMBO_BUILD",
             "-DCONTROLLERBUTTONS_T=uint32_t", "-DLOG_LEVEL_GAME_PRINTS=0",
             '-DCVAR_PREFIX_ENHANCEMENT="gEnhancements"', "-Werror=implicit-function-declaration",
             "-Wno-incompatible-pointer-types", "-Wno-int-conversion"]
    flags += ["-I" + str(root / p) for p in
              ["soh/include", "soh/src", "soh/assets", "soh", "libultraship/include", "combo"]]
    if sanitize:
        flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
    compiler = os.environ.get("CC", "cc")
    subprocess.run([compiler, *flags, "-fsyntax-only", str(kusa), str(wood)], cwd=root, check=True)
    binary = output / "season_actor_test"
    subprocess.run([compiler, *flags, "-I" + str(output), str(root / "soh/tests/season_actor_test.c"),
                    "-lm", "-o", str(binary)], cwd=root, check=True)
    environment = os.environ.copy()
    options = environment.get("ASAN_OPTIONS", "")
    if "detect_leaks=" not in options:
        environment["ASAN_OPTIONS"] = options + (":" if options else "") + "detect_leaks=0"
    subprocess.run([str(binary)], check=True, env=environment)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", nargs="?", type=Path)
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    if args.output:
        args.output.mkdir(parents=True, exist_ok=True)
        run(args.output.resolve(), args.sanitize)
    else:
        with tempfile.TemporaryDirectory(prefix="soh-season-actors-") as path:
            run(Path(path), args.sanitize)
