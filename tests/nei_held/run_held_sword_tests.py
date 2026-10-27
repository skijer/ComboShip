"""Execute production held-sword selection with each host's real graphics ABI."""
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/nei_held"))
from run_articulated_tests import flags as oot_flags
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_mm_nei_tests import flags as mm_flags


def function(source, name):
    match = re.search(r"(?:static )?u8 " + name + r"\([^)]*\) \{", source)
    assert match, name
    end = match.end()
    depth = 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end]


def main():
    with tempfile.TemporaryDirectory(prefix="nei-held-sword-") as temporary:
        directory = Path(temporary)
        for host, flags in [("soh", ["-DCOMBO_BUILD", *oot_flags()]), ("mm", ["-std=c++20", "-DNEI_EQUIPMENT_MM", *mm_flags()])]:
            source = (ROOT / host / "mods/items/logic/weapon_upgrades.c").read_text()
            (directory / "held_sword_bindings.inc").write_text(function(source, "WeaponUpgrade_ApplyHeldSwordDL"))
            if host == "mm":
                native = (ROOT / "mm/src/code/z_player_lib.c").read_text()
                begin = native.index("            u8 handIsSpokenFor =")
                end = native.index("            // ⚠️ ESTE OCULTADO", begin)
                (directory / "held_sword_native_bindings.inc").write_text(
                    "static void ApplyNativeHeldSwordStage(PlayState* play, Player* player, Gfx** dList) {\n"
                    "u8 extOwnsSwordDL = 0;\n" + native[begin:end] + "\n}\n")
            binary = directory / host
            bridge = ([str(ROOT / "mm/2s2h/Rando/NeiResourceRouting.cpp"),
                       "-Wl,--export-dynamic-symbol=OOT_NeiResourceExists",
                       "-Wl,--export-dynamic-symbol=OOT_NeiEnsureGiBaseOwner"] if host == "mm" else [])
            subprocess.run([os.environ.get("CXX", "c++"), *flags, "-I" + str(directory),
                            str(ROOT / "tests/nei_held/held_sword_runtime_test.cpp"), *bridge, "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True)
    subprocess.run([sys.executable, "-B", str(ROOT / "tests/nei_held/held_sword_geometry_test.py")], check=True)


if __name__ == "__main__":
    main()
