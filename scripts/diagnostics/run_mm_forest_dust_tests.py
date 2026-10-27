"""Exercise MM's tagged forest tornado particles and native dust isolation."""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile
import re
from run_mm_scene_randomization_tests import function

ROOT = Path(__file__).resolve().parents[2]


def main():
    flags = ["-std=gnu2x", "-O1", "-g", "-ffunction-sections", "-fdata-sections", "-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DMM_BUILD_DLL",
             "-DCONTROLLERBUTTONS_T=uint32_t", "-DNON_EQUIVALENT", "-DNON_MATCHING",
             "-Werror=implicit-function-declaration", "-Wno-incompatible-pointer-types", "-Wno-int-conversion"]
    flags += ["-I" + str(ROOT / path) for path in
              ("mm/include", "mm/include/PR", "mm/src", "mm/assets", "mm", "mm/2s2h",
               "libultraship/include", "libultraship/src", "combo")]
    source = (ROOT / "mm/expansions/sw97/actors/spells/z_magic_wind.inc.c").read_text()
    definitions = re.findall(r"^#define TORNADO_(?:LIFT_MAX|CORE_RADIUS) .*$", source, re.M)
    with tempfile.TemporaryDirectory(prefix="mm-forest-dust-") as temp:
        (Path(temp) / "forest_tornado_production.inc").write_text(
            "\n".join(definitions) + "\n" + function(source, "MagicWind_SpawnTornadoVFX"))
        flags += ["-I" + temp]
        binary = Path(temp) / "test"
        compiled = subprocess.run([*shlex.split(os.environ.get("CC", "cc")), *flags,
                        "mm/tests/forest_dust_test.c",
                        "mm/src/code/z_effect_soft_sprite_old_init.c",
                        "-Wl,--gc-sections", "-lm", "-o", str(binary)], cwd=ROOT, capture_output=True, text=True)
        if compiled.returncode:
            raise RuntimeError(compiled.stdout + compiled.stderr)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
