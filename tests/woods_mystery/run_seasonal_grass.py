"""Exercise actual Woods actor draw bodies without changing actor/gameplay data."""
from pathlib import Path
import os
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_mm_weather_tests import production_function

with tempfile.TemporaryDirectory(prefix='woods-seasonal-actors-') as directory:
    build = Path(directory)
    bodies = []
    for relative, names in [
        ('ovl_En_Kusa/z_en_kusa.c', ('EnKusa_DrawBush', 'EnKusa_DrawGrass')),
        ('ovl_Obj_Grass/z_obj_grass.c', ('ObjGrass_DrawOpa', 'ObjGrass_DrawXlu')),
        ('ovl_Obj_Grass_Carry/z_obj_grass_carry.c', ('ObjGrassCarry_Draw',)),
    ]:
        source = (ROOT / 'mm/src/overlays/actors' / relative).read_text()
        bodies += [production_function(source, name) for name in names]
    (build / 'autumn_grass_production.inc').write_text('\n'.join(bodies))
    flags = ['-std=gnu17', '-O1', '-w', '-DF3DEX_GBI_2', '-DCOMBO_BUILD', '-DMM_BUILD_DLL',
             '-DCONTROLLERBUTTONS_T=uint32_t', '-DNON_EQUIVALENT', '-DNON_MATCHING']
    flags += ['-I' + str(ROOT / p) for p in ('mm/include', 'mm/include/PR', 'mm/src', 'mm/assets',
              'mm', 'mm/2s2h', 'mm/tests', 'libultraship/include', 'libultraship/src', 'combo')]
    flags += shlex.split(os.environ.get('MM_FOLIAGE_TEST_CFLAGS', ''))
    binary = build / 'test'
    subprocess.run([os.environ.get('CC', 'cc'), *flags, '-I' + str(build),
                    str(Path(__file__).with_name('seasonal_grass_test.c')), '-lm', '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
