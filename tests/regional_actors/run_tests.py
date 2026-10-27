#!/usr/bin/env python3
"""Regional actor native/source suite; optional original MM archive argument."""
from pathlib import Path
import os,subprocess,sys,tempfile
ROOT=Path(__file__).resolve().parents[2]
def run(args):subprocess.run([str(a) for a in args],cwd=ROOT,check=True)
with tempfile.TemporaryDirectory(prefix='regional-actors-') as d:
    flags=['-DF3DEX_GBI_2','-DCOMBO_BUILD','-DFMT_HEADER_ONLY',*['-I'+str(ROOT/p) for p in ['mm/include','mm/include/PR','mm/src','mm/assets','mm','mm/2s2h','libultraship/include','combo','combo/menu']]]
    for name,cc,std in [('native_params_test','cc','c11'),('native_presentation_test','c++','c++20')]:
        suffix='.c' if cc=='cc' else '.cpp'
        run([cc,'-std='+std,*flags,'tests/regional_actors/'+name+suffix,'-o',d+'/test']);run([d+'/test']);print('PASS '+name,flush=True)
    run(['c++','-std=c++20','-Icombo','tests/regional_actors/placement_test.cpp','-o',d+'/test']);run([d+'/test'])
run([sys.executable,'tests/regional_actors/spawn_gate_test.py'])
run([sys.executable,'scripts/diagnostics/run_regional_actor_tests.py'])
if len(sys.argv)>1:
    run([sys.executable,'tests/regional_actors/archive_test.py',sys.argv[1]])
    run([sys.executable,'tests/regional_actors/run_resource_test.py',sys.argv[1]])
    run([sys.executable,'tests/regional_actors/pose_focus_test.py',sys.argv[1]])

    terrain_root = os.environ.get('COMBO_REGIONAL_GEOMETRY_ROOT')
    if not terrain_root and (ROOT/'combo/entrances/FountainValley.h').exists():
        terrain_root = ROOT
    geometry_args = ['--terrain-root', terrain_root] if terrain_root else []
    run([sys.executable,'tests/regional_actors/posed_geometry_test.py',sys.argv[1],*geometry_args])
