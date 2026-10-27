"""Exercise MM's accepted fire sphere rendering with named, fresh resource bindings."""
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
    source = (ROOT / "mm/expansions/sw97/actors/spells/z_magic_fire.inc.c").read_text()
    struct = source[source.index("typedef struct MagicFire {"):source.index("} MagicFire;") + len("} MagicFire;")]
    indices = re.search(r"static u8 sVertexIndices\[\] = \{.*?\};", source, re.S)[0]
    names = re.findall(r"^static const .*sMagicFire(?:Oot|Medallion).*;$", source, re.M)
    helpers = [function(source, name) for name in
               ("MagicFire_GetOotMaterialDL", "MagicFire_GetOotModelDL", "MagicFire_GetOotTex",
                "MagicFire_GetOotSphereVtx", "MagicFireDins_Draw")]
    with tempfile.TemporaryDirectory(prefix="mm-magic-fire-bindings-") as temp:
        (Path(temp) / "magic_fire_draw_production.inc").write_text("\n".join([struct, indices, *names, *helpers]))
        flags += ["-I" + temp]
        binary = Path(temp) / "test"
        compiled = subprocess.run([*shlex.split(os.environ.get("CC", "cc")), *flags,
                        "mm/tests/magic_fire_bindings_test.c",
                        "-Wl,--gc-sections", "-lm", "-o", str(binary)], cwd=ROOT, capture_output=True, text=True)
        if compiled.returncode:
            raise RuntimeError(compiled.stdout + compiled.stderr)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
