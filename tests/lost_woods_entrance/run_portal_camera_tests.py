#!/usr/bin/env python3
"""Run the complete portal camera enhancement with native MM data and camera math."""
import argparse
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_time_pedestal_tests import functions

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--sanitize',action='store_true')
args=parser.parse_args()
native=[]
for file,names in [
    ('mm/src/code/z_actor.c',['Player_GetHeight','Actor_GetFocus']),
    ('mm/src/code/z_lib.c',['Math_SinS','Math_CosS']),
    ('mm/src/code/z_olib.c',['OLib_ClampMinDist','OLib_ClampMaxDist','OLib_Vec3fDistNormalize',
                           'OLib_VecSphToVec3f','OLib_VecGeoToVec3f','OLib_Vec3fToVecSph','OLib_Vec3fToVecGeo',
                           'OLib_Vec3fDiffToVecGeo','OLib_AddVecGeoToVec3f']),
    ('mm/src/code/z_camera.c',['Camera_fabsf','Camera_Vec3fMagnitude','Camera_QuadraticAttenuation',
                             'Camera_ScaledStepToCeilF','Camera_ScaledStepToCeilS','Camera_ScaledStepToCeilVec3f',
                             'Camera_SetUpdateRatesFastYaw','Camera_GetFocalActorHeight',
                             'func_800CB924','func_800CB950','func_800CBA7C',
                             'Camera_BgCheckInfo','Camera_ClampDist2','Camera_ClampLerpScale','Camera_CalcSlopeYAdj',
                             'Camera_CalcAtDefault','Camera_CalcDefaultPitch','Camera_CalcDefaultYaw','Camera_Normal1']),
    ('mm/src/overlays/actors/ovl_player_actor/z_player.c',['func_80835324','Player_StartMode_E','Player_StartMode_F']),
]:
    bodies=functions((ROOT/file).read_text(),names)
    native.extend(re.sub(r'\bthis\b','self',bodies[name]) for name in names)
path=ROOT/'mm/2s2h/Enhancements/Camera/ComboForestArrivalCamera.cpp'
enhancement=re.sub(r'^#include[^\n]*\n','',path.read_text(),flags=re.M) if path.exists() else \
    'extern "C" void ComboForestArrivalCamera_Queue(int) {}\n'
fixture=(ROOT/'tests/lost_woods_entrance/portal_camera_test.cpp').read_text()
fixture=fixture.replace('/* NATIVE_MATH */',
                        '\n'.join(body[:body.index('{')]+';' for body in native)+'\n'+'\n'.join(native))
fixture=fixture.replace('/* ENHANCEMENT */',enhancement)
data=(ROOT/'mm/src/code/z_camera_data.inc').read_text()
fixture=fixture.replace('/* NORMAL_PRESET */',re.search(r'CameraModeValue sSetNormal0ModeNormalData\[\] = \{.*?\n\};',data,re.S)[0])
flags=['-std=c++20','-w','-DF3DEX_GBI_2','-DCOMBO_BUILD','-ffunction-sections','-fdata-sections','-Wl,--gc-sections',
       *['-I'+str(ROOT/p) for p in ['mm','mm/2s2h','mm/include','mm/include/PR','mm/src','mm/assets','libultraship/include','combo']]]
if args.sanitize: flags+=['-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-sanitize-recover=all','-no-pie']
with tempfile.TemporaryDirectory(prefix='forest-portal-camera-') as temporary:
    build=Path(temporary); source=build/'test.cpp'; binary=build/'test'; source.write_text(fixture)
    objects=[]
    for file in ['mm/src/libultra/gu/sins.c','mm/src/libultra/gu/coss.c','mm/src/boot/O2/math64.c','mm/src/boot/O2/rand.c']:
        obj=build/(Path(file).stem+'.o')
        result=subprocess.run([os.environ.get('CC','cc'),'-std=gnu2x',*flags[2:],'-include','global.h',
                               '-c',str(ROOT/file),'-o',str(obj)],capture_output=True,text=True)
        if result.returncode: raise SystemExit(result.stderr[:5000])
        objects.append(str(obj))
    subprocess.run([os.environ.get('CXX','c++'),*flags,str(source),*objects,str(ROOT/'mm/2s2h/Enhancements/Gameplay/ComboRegionalPassages.cpp'),'-lm','-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
