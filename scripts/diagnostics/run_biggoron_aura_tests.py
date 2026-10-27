"""Exercise real Biggoron profile identity, generated GBI and native resource boundaries."""
from pathlib import Path
import argparse
import os
import re
import subprocess
import sys
import tempfile

from run_mm_din_fire_tests import FLAGS as MM_FLAGS
from run_elemental_sword_aura_tests import function

ROOT = Path(__file__).resolve().parents[2]


def oot_flags():
    flags = ["-std=gnu2x", "-O1", "-g", "-DNDEBUG", "-DF3DEX_GBI_2", "-DLOG_LEVEL_GAME_PRINTS=0",
             "-Werror=implicit-function-declaration"]
    flags += ["-I" + str(ROOT / path) for path in
              ("soh/include", "soh/src", "soh/assets", "soh", "libultraship/include", "combo")]
    for path in ("CMake/soh-cvars.cmake", "CMake/lus-cvars.cmake"):
        for key, value in re.findall(r'set\((CVAR_PREFIX_\w+)\s+"?([^\s"\)]+)', (ROOT / path).read_text()):
            flags.append(f'-D{key}="{value}"')
    return flags


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    parser.add_argument("--host", choices=("soh", "mm", "both"), default="both")
    args = parser.parse_args()
    subprocess.run([sys.executable, "-B", "tests/elemental_swords/biggoron_depth_coverage.py"], cwd=ROOT, check=True)
    with tempfile.TemporaryDirectory(prefix="biggoron-aura-") as directory:
        for host, flags in (("soh", oot_flags()), ("mm", MM_FLAGS)):
            if args.host != "both" and args.host != host:
                continue
            source = (ROOT / host / "src/code/z_player_lib.c").read_text()
            if host == "soh":
                boundary = function(source, "Player_ResolveLimbDLForDummyOrLocal")
            else:
                start = source.index("            if ((player->heldItemId == ITEM_SWORD_BGS) && !WeaponUpgrade_HasGreatFairy() &&")
                end = source.index("            // Skijer's NEI: OoT Megaton Hammer", start)
                boundary = "static void BiggoronNativeImportedHand(Player* player, Gfx** dList) {\n" + source[start:end] + "\n}\n"
            (Path(directory) / "biggoron_native_production.inc").write_text(boundary)
            weapon = (ROOT / host / "mods/items/logic/weapon_upgrades.c").read_text()
            (Path(directory) / "aura_held_production.inc").write_text(function(weapon, "WeaponUpgrade_ApplyHeldSwordDL"))
            for combined in (False, True):
                selected = [flag for flag in flags if flag != "-DCOMBO_BUILD"]
                if combined:
                    selected.append("-DCOMBO_BUILD")
                selected.append("-I" + directory)
                if host == "mm":
                    selected += ["-DBIGGORON_TEST_MM", "-DSWORD_AURA_TEST_MM"]
                if args.sanitize:
                    selected += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                                 "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
                binary = Path(directory) / (host + ("-combo" if combined else "-solo"))
                subprocess.run([os.environ.get("CC", "cc"), *selected,
                                "tests/elemental_swords/biggoron_geometry_identity.c", "-lm", "-o", str(binary)],
                               cwd=ROOT, check=True)
                subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
                native = str(binary) + "-native"
                subprocess.run([os.environ.get("CC", "cc"), *selected,
                                "tests/elemental_swords/biggoron_resolved_hand.c", "-lm", "-o", native],
                               cwd=ROOT, check=True)
                subprocess.run([native], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
                capture = str(binary) + "-capture"
                subprocess.run([os.environ.get("CC", "cc"), *selected,
                                "tests/elemental_swords/biggoron_capture_diagnostics.c",
                                f"{host}/src/code/din_fire_sword.c", "-lm", "-o", capture], cwd=ROOT, check=True)
                subprocess.run([capture], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
                builtin = str(binary) + "-builtin"
                subprocess.run([os.environ.get("CC", "cc"), *selected,
                                "tests/elemental_swords/biggoron_builtin_render.c",
                                f"{host}/src/code/din_fire_sword.c", "-lm", "-o", builtin], cwd=ROOT, check=True)
                subprocess.run([builtin], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
                if host == "soh":
                    knife = str(binary) + "-knife"
                    subprocess.run([os.environ.get("CC", "cc"), *selected,
                                    "tests/elemental_swords/knife_builtin_render.c",
                                    "soh/src/code/din_fire_sword.c", "-lm", "-o", knife], cwd=ROOT, check=True)
                    subprocess.run([knife], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
                subprocess.run([os.environ.get("CC", "cc"), *selected, "-fsyntax-only",
                                f"{host}/src/code/z_player_lib.c"], cwd=ROOT, check=True)
                print("PASS Biggoron", host, "Combo" if combined else "solo")


if __name__ == "__main__":
    main()
