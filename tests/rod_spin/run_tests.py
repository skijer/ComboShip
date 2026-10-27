"""Execute rod spin spawn/input functions with real OoT and MM data layouts.

Only unrelated engine boundaries are supplied by fixtures. The tested functions
and the native damage arrays are extracted unchanged from production sources.
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
from run_mm_nei_tests import flags as mm_flags


def source(path, ref):
    if ref:
        return subprocess.check_output(["git", "show", f"{ref}:{path}"], cwd=ROOT, text=True)
    return (ROOT / path).read_text()


def extract(path, names, ref):
    found = functions(source(path, ref), names)
    return "\n".join(found[name] for name in names)


def soh_flags():
    result = ["-DF3DEX_GBI_2", "-DLOG_LEVEL_GAME_PRINTS=0"]
    result += [f"-I{ROOT / p}" for p in
               ("soh", "soh/include", "soh/src", "soh/assets", "soh/mods", "combo/menu", "libultraship/include")]
    for config in ("CMake/soh-cvars.cmake", "CMake/lus-cvars.cmake"):
        for key, value in re.findall(r'set\((CVAR_PREFIX_\w+)\s+"?([^\s"\)]+)', (ROOT / config).read_text()):
            result.append(f'-D{key}="{value}"')
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--engine", choices=("mm", "soh", "all"), default="all")
    parser.add_argument("--source-ref", help="Read production functions from an exact Git revision for a negative control")
    parser.add_argument("--sanitize", action="store_true")
    parser.add_argument("--check", choices=("all", "spin", "input"), default="all")
    args = parser.parse_args()
    engines = ("soh", "mm") if args.engine == "all" else (args.engine,)
    with tempfile.TemporaryDirectory(prefix="rod-spin-native-") as td:
        directory = Path(td)
        for engine in engines:
            player = f"{engine}/src/overlays/actors/ovl_player_actor/z_player.c"
            lib = f"{engine}/src/code/z_player_lib.c"
            actor = source(f"{engine}/src/overlays/actors/ovl_En_M_Thunder/z_en_m_thunder.c", args.source_ref)
            if engine == "mm":
                parts = extract(lib, ("Player_MeleeWeaponFromIA", "Player_GetMeleeWeaponHeld"), args.source_ref)
                parts += "\n" + extract(player, ("func_808332A0", "func_8083A548", "Player_ActionHandler_8"), args.source_ref)
                parts += "\n" + re.search(r"static u8 sDamages\[\] = \{.*?\};", actor, re.S)[0]
                flags = mm_flags()
            else:
                parts = extract(lib, ("Player_ActionToMeleeWeapon", "Player_IsFDHoldingSword", "Player_GetMeleeWeaponHeld"), args.source_ref)
                parts += "\n" + extract(player, ("func_80837530",), args.source_ref)
                for name in ("sSpinAttackDmgFlags", "sJumpAttackDmgFlags"):
                    parts += "\n" + re.search(r"static u32 " + name + r"\[\] = \{.*?\};", actor, re.S)[0]
                flags = soh_flags()
            (directory / "production_spin.inc").write_text(parts)
            binary = directory / engine
            extra = ["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-g"] if args.sanitize else []
            subprocess.run([os.environ.get("CC", "cc"), "-std=gnu2x", *flags, *extra,
                            "-Werror=implicit-function-declaration", f"-I{directory}",
                            str(ROOT / f"tests/rod_spin/native_{engine}_test.c"), "-o", str(binary)], check=True)
            subprocess.run([str(binary), args.check], check=True)
        if args.engine in ("mm", "all") and args.check == "all":
            subprocess.run([sys.executable, "-B", str(ROOT / "tests/rod_spin/run_mm_charge_tests.py"),
                            *(["--sanitize"] if args.sanitize else [])], check=True)


if __name__ == "__main__":
    main()
