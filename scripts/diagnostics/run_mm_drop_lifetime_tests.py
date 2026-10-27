"""Exercise actual MM drop updates through timeout, pickup and toggle transitions."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

from run_mm_scene_randomization_tests import function

ROOT = Path(__file__).resolve().parents[2]


def main():
    paths = ["mm/src/code/z_en_item00.c", "mm/src/overlays/actors/ovl_En_Tanron5/z_en_tanron5.c"]
    item, twinmold = [(ROOT / path).read_text() for path in paths]
    bodies = [function(item, name) for name in ["func_800A640C", "func_800A6A40", "EnItem00_Update"]]
    bodies += [twinmold[twinmold.index("typedef enum TwinmoldPropItemDropType {"):
                        twinmold.index("s32 sFragmentAndItemDropCount")],
               function(twinmold, "EnTanron5_RuinFragmentItemDrop_Update")]
    flags = ["-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DMM_BUILD_DLL", "-DCONTROLLERBUTTONS_T=uint32_t",
             "-DNON_EQUIVALENT", "-DNON_MATCHING", "-Wno-int-conversion", "-Wno-incompatible-pointer-types",
             "-Werror=implicit-function-declaration"]
    flags += ["-I" + str(ROOT / p) for p in
              ("mm/include", "mm/include/PR", "mm/src", "mm", "mm/2s2h", "mm/assets",
               "libultraship/include", "libultraship/src", "combo", "combo/menu")]
    cc = shlex.split(os.environ.get("CC", "cc"))
    with tempfile.TemporaryDirectory(prefix="mm-drop-lifetime-") as temporary:
        build = Path(temporary)
        (build / "drop_lifetime_production.inc").write_text("\n".join(bodies))
        binary = build / "drop_lifetime_test"
        result = subprocess.run([*cc, "-std=gnu11", *flags, "-I" + str(build),
                                 str(ROOT / "mm/tests/drop_lifetime_test.c"), "-lm", "-o", str(binary)],
                                capture_output=True, text=True)
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)
        subprocess.run([str(binary)], check=True)
        for path in paths:
            result = subprocess.run([*cc, "-std=gnu11", *flags, "-fsyntax-only", str(ROOT / path)],
                                    capture_output=True, text=True)
            if result.returncode:
                raise RuntimeError(result.stdout + result.stderr)
            print("PASS real-header syntax: " + path)


if __name__ == "__main__":
    main()
