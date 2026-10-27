"""Check the actual XML loader and OoT button dispatch, not just resource paths."""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

from run_articulated_tests import ROOT, flags


def function(source, name):
    masked = re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"',
                    lambda match: re.sub(r'[^\n]', ' ', match[0]), source, flags=re.S)
    match = re.search(r'^(?:int8_t|s32|void) ' + name + r'\([^)]*\) \{', masked, re.M)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (masked[end] == '{') - (masked[end] == '}')
        end += 1
    return re.sub(r'\bthis\b', 'player', source[match.start():end])


def run(directory, case, sanitize):
    binary = directory / case
    compiler = [os.environ.get("CXX", "c++"), "-std=c++20"]
    if sanitize:
        compiler += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
    if case == "xml":
        # Logging is the only replaced engine dependency; XML parsing, resource
        # types and command generation are compiled from production unchanged.
        (directory / "spdlog").mkdir()
        (directory / "spdlog/spdlog.h").write_text(
            "#pragma once\n#define SPDLOG_ERROR(...) ((void)0)\n#define SPDLOG_TRACE(...) ((void)0)\n")
        sources = [
            "tests/nei_held/equipment_xml_loader_test.cpp",
            "libultraship/src/fast/resource/factory/DisplayListFactory.cpp",
            "libultraship/src/fast/resource/type/DisplayList.cpp",
            "libultraship/src/ship/resource/ResourceFactoryXML.cpp",
            "libultraship/src/ship/resource/ResourceFactoryBinary.cpp",
            "libultraship/src/ship/resource/Resource.cpp",
            "ZAPDTR/lib/tinyxml2/tinyxml2.cpp",
        ]
        compiler += ["-DF3DEX_GBI_2", "-ffunction-sections", "-fdata-sections", "-include", "unordered_map",
                     "-I" + str(directory), "-I" + str(ROOT / "libultraship/include"),
                     "-I" + str(ROOT / "ZAPDTR/lib/tinyxml2")]
        arguments = [str(ROOT / "soh/assets/custom")]
    else:
        player = (ROOT / "soh/src/overlays/actors/ovl_player_actor/z_player.c").read_text()
        array = re.search(r's8 sItemActions\[\] = \{.*?\n\};', player, re.S)[0]
        extended = (ROOT / "soh/mods/extended_player.c").read_text()
        library = (ROOT / "soh/src/code/z_player_lib.c").read_text()
        (directory / "oot_equipment_action.inc").write_text(
            array + "\n" + function(extended, "ExtPlayer_GetItemAction") + "\n" +
            "\n".join(function(library, name) for name in
                      ["Player_ActionToMeleeWeapon", "Player_IsFDHoldingSword", "Player_GetMeleeWeaponHeld"]) + "\n" +
            "\n".join(function(player, name) for name in
                      ["func_8083C50C", "Player_ActionHandler_8", "func_80842B7C"]))
        compiler += ["-DCOMBO_BUILD", *flags()[1:], "-I" + str(directory)]
        sources = ["tests/nei_held/oot_equipment_action_test.cpp"]
        arguments = []
    subprocess.run([*compiler, *(str(ROOT / source) for source in sources),
                    "-Wl,--gc-sections", "-o", str(binary)], check=True)
    subprocess.run([str(binary), *arguments], check=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--case", choices=["xml", "action"])
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    for case in ([args.case] if args.case else ["xml", "action"]):
        with tempfile.TemporaryDirectory(prefix="held-equipment-regression-") as temporary:
            run(Path(temporary), case, args.sanitize)
