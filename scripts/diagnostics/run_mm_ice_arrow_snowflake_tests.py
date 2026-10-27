"""Exercise MM's real ice-arrow lifecycle, native fallbacks and emitted GBI packets."""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    flags = ["-std=gnu2x", "-O1", "-g", "-ffunction-sections", "-fdata-sections", "-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DMM_BUILD_DLL",
             "-DCONTROLLERBUTTONS_T=uint32_t", "-DNON_EQUIVALENT", "-DNON_MATCHING",
             "-Werror=implicit-function-declaration", "-Wno-incompatible-pointer-types", "-Wno-int-conversion"]
    flags += ["-I" + str(ROOT / path) for path in
              ("mm/include", "mm/include/PR", "mm/src", "mm/assets", "mm", "mm/2s2h",
               "libultraship/include", "libultraship/src", "combo")]
    with tempfile.TemporaryDirectory(prefix="mm-ice-arrow-snowflake-") as temp:
        binary = Path(temp) / "test"
        compiled = subprocess.run([*shlex.split(os.environ.get("CC", "cc")), *flags,
                        "mm/tests/ice_arrow_snowflake_test.c",
                        "mm/src/overlays/actors/ovl_Arrow_Ice/z_arrow_ice.c",
                        "mm/src/overlays/actors/ovl_Arrow_Fire/z_arrow_fire.c",
                        "mm/src/overlays/actors/ovl_Arrow_Light/z_arrow_light.c",
                        "-Wl,--gc-sections", "-lm", "-o", str(binary)], cwd=ROOT, capture_output=True, text=True)
        if compiled.returncode:
            raise RuntimeError(compiled.stdout + compiled.stderr)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
