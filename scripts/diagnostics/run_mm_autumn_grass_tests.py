"""Exercise actual cuttable-grass draw bodies and their seasonal command scopes."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
from run_mm_weather_tests import production_function

ROOT = Path(__file__).resolve().parents[2]
sources = [ROOT / "mm/src/overlays/actors/ovl_En_Kusa/z_en_kusa.c",
           ROOT / "mm/src/overlays/actors/ovl_Obj_Grass/z_obj_grass.c",
           ROOT / "mm/src/overlays/actors/ovl_Obj_Grass_Carry/z_obj_grass_carry.c"]
with tempfile.TemporaryDirectory(prefix="mm-autumn-grass-") as temporary:
    build = Path(temporary)
    bodies = []
    for path, names in zip(sources, [("EnKusa_DrawBush", "EnKusa_DrawGrass"),
                                    ("ObjGrass_DrawOpa", "ObjGrass_DrawXlu"), ("ObjGrassCarry_Draw",)]):
        text = path.read_text()
        bodies.extend(production_function(text, name) for name in names)
    (build / "autumn_grass_production.inc").write_text("\n".join(bodies))
    flags = ["-std=gnu17", "-O1", "-g", "-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DMM_BUILD_DLL",
             "-DCONTROLLERBUTTONS_T=uint32_t", "-DNON_EQUIVALENT", "-DNON_MATCHING",
             "-Werror=implicit-function-declaration", "-Wno-incompatible-pointer-types", "-Wno-int-conversion"]
    flags += ["-I" + str(ROOT / path) for path in ("mm/include", "mm/include/PR", "mm/src", "mm/assets",
              "mm", "mm/2s2h", "libultraship/include", "libultraship/src", "combo")]
    flags += ["-I" + str(build), *shlex.split(os.environ.get("MM_FOLIAGE_TEST_CFLAGS", ""))]
    compiler = os.environ.get("CC", "cc")
    subprocess.run([compiler, *flags, "-fsyntax-only", *map(str, sources)], check=True)
    binary = build / "test"
    subprocess.run([compiler, *flags, str(ROOT / "mm/tests/autumn_grass_test.c"), "-lm", "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS":"detect_leaks=0"})
