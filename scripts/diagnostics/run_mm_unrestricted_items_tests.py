"""Exercise MM's actual button update with scene item restrictions and live toggles."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

from run_mm_scene_randomization_tests import function

ROOT = Path(__file__).resolve().parents[2]


def declaration(source, name):
    start = source.index(name)
    start = source.rfind("\n", 0, start) + 1
    return source[start:source.index("\n};", start) + 3]


def main():
    source = (ROOT / "mm/src/code/z_parameter.c").read_text()
    inventory = (ROOT / "mm/src/code/z_inventory.c").read_text()
    bodies = [declaration(source, "gPlayerFormItemRestrictions["),
              declaration(inventory, "gEquipMasks["), declaration(inventory, "gEquipShifts["),
              source[source.rfind("typedef enum {", 0, source.index("PICTO_BOX_STATE_OFF")):
                     source.index("Input sPostmanTimerInput")],
              source[source.index("#define RESTRICTIONS_TABLE_END"):
                     source.index("s16 sPictoState")],
              function(source, "Interface_SetSceneRestrictions"),
              function(source, "Interface_UpdateButtonsPart2")]
    flags = ["-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DMM_BUILD_DLL", "-DCONTROLLERBUTTONS_T=uint32_t",
             "-DNON_EQUIVALENT", "-DNON_MATCHING", "-Wno-int-conversion", "-Wno-incompatible-pointer-types",
             "-Werror=implicit-function-declaration"]
    flags += ["-I" + str(ROOT / p) for p in
              ("mm/include", "mm/include/PR", "mm/src", "mm", "mm/2s2h", "mm/assets",
               "libultraship/include", "libultraship/src", "combo")]
    flags += shlex.split(os.environ.get("MM_ITEMS_TEST_CFLAGS", ""))
    cc = shlex.split(os.environ.get("CC", "cc"))
    with tempfile.TemporaryDirectory(prefix="mm-unrestricted-items-") as temporary:
        build = Path(temporary)
        (build / "unrestricted_items_production.inc").write_text("\n".join(bodies))
        binary = build / "unrestricted_items_test"
        result = subprocess.run([*cc, "-std=gnu11", *flags, "-I" + str(build),
                                 str(ROOT / "mm/tests/unrestricted_items_test.c"), "-o", str(binary)],
                                capture_output=True, text=True)
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)
        subprocess.run([str(binary)], check=True)
        result = subprocess.run([*cc, "-std=gnu11", *flags, "-fsyntax-only",
                                 str(ROOT / "mm/src/code/z_parameter.c")], capture_output=True, text=True)
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)
        print("PASS real-header syntax: mm/src/code/z_parameter.c")


if __name__ == "__main__":
    main()
