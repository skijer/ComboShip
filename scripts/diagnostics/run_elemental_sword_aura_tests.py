"""Execute real aura GBI with pack access disabled, then compile both host modes."""
from pathlib import Path
import argparse
import json
import os
import re
import subprocess
import tempfile

from run_mm_din_fire_tests import FLAGS as MM_FLAGS

ROOT = Path(__file__).resolve().parents[2]

def function(source, name):
    start = source.index(name + "(")
    start = source.rfind("\n", 0, start) + 1
    brace = source.index("{", start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    oot = ["-std=gnu2x", "-O1", "-g", "-DNDEBUG", "-DF3DEX_GBI_2", "-DLOG_LEVEL_GAME_PRINTS=0",
           "-Werror=implicit-function-declaration"]
    for path in ("soh/include", "soh/src", "soh/assets", "soh", "libultraship/include", "combo"):
        oot.append("-I" + str(ROOT / path))
    for path in ("CMake/soh-cvars.cmake", "CMake/lus-cvars.cmake"):
        for key, value in re.findall(r'set\((CVAR_PREFIX_\w+)\s+"?([^\s"\)]+)', (ROOT / path).read_text()):
            oot.append(f'-D{key}="{value}"')
    with tempfile.TemporaryDirectory(prefix="elemental-sword-aura-") as directory:
        original = json.loads((ROOT / "tools/elemental_sword_aura/fire_source.json").read_text())
        alias = original["equipment_aliases"]["broken"]["commands"]
        (Path(directory) / "aura_accepted_alias.inc").write_text(
            "static Gfx sAcceptedBrokenAlias[] = {\n" + "\n".join(
                "    { .words = { .w0 = 0x%08Xu, .w1 = 0x%08Xu } }," % tuple(command) for command in alias) + "\n};\n")
        for host, flags in (("soh", oot), ("mm", MM_FLAGS)):
            weapon = (ROOT / host / "mods/items/logic/weapon_upgrades.c").read_text()
            bindings = function(weapon, "WeaponUpgrade_ApplyHeldSwordDL")
            if host == "soh":
                equipment = (ROOT / "soh/soh/Enhancements/customequipment.cpp").read_text()
                bindings += "\n" + function(equipment, "BuildHandItemDL") + "\n" + function(equipment, "BuildSwordHandItemDL")
            else:
                native = (ROOT / "mm/src/code/z_player_lib.c").read_text()
                start = native.index("            if ((player->heldItemId == ITEM_SWORD_MASTER) &&")
                end = native.index("            // Skijer's NEI: OoT Megaton Hammer", start)
                bindings += "\nstatic void AuraNativeImportedHands(Player* player, Gfx** dList) {\n" + native[start:end] + "\n}\n"
            (Path(directory) / "aura_held_production.inc").write_text(bindings)
            for combined in (False, True):
                selected = [flag for flag in flags if flag != "-DCOMBO_BUILD"]
                if combined: selected.append("-DCOMBO_BUILD")
                selected.append("-I" + directory)
                if host == "mm": selected.append("-DSWORD_AURA_TEST_MM")
                if args.sanitize:
                    selected += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                                 "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
                binary = Path(directory) / (host + ("-combo" if combined else "-solo"))
                subprocess.run([os.environ.get("CC", "cc"), *selected,
                    f"{host}/tests/elemental_sword_aura_test.c", f"{host}/src/code/din_fire_sword.c",
                    "-lm", "-o", str(binary)], cwd=ROOT, check=True)
                subprocess.run([str(binary)], check=True)
                # The real selector and renderer share one executable here;
                # inventory and damage-checkbox gates cannot be stubbed open.
                gated = str(binary) + "-state"
                subprocess.run([os.environ.get("CC", "cc"), *selected,
                    "tests/elemental_swords/aura_state_integration.c", f"{host}/src/code/din_fire_sword.c",
                    f"{host}/src/code/sword_element.c", "-lm", "-o", gated], cwd=ROOT, check=True)
                subprocess.run([gated], check=True)
                # A complete native TU/object compile, independent of fixture linkage.
                subprocess.run([os.environ.get("CC", "cc"), *selected, "-c",
                    f"{host}/src/code/din_fire_sword.c", "-o", str(binary) + ".o"], cwd=ROOT, check=True)
                print("PASS full native aura source", host, "Combo" if combined else "solo")

if __name__ == "__main__": main()
