"""Actual native MM drawer -> mesh renderer -> matrices -> frame interpolator."""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_mm_nei_tests import flags
with tempfile.TemporaryDirectory(prefix='mm-nei-interpolation-') as td:
    # Compile the unchanged accepted donor's adapter against the same MM graphics
    # boundary, with distinct entry names. Native structs never cross engines.
    donor=(ROOT/'soh/soh/Enhancements/randomizer/NeiUsedMagicPresentation.cpp').read_text()
    donor=donor.replace('"NeiUsedMagicPresentation.h"','"2s2h/Rando/NeiUsedMagicPresentation.h"')
    for header in ['NeiGiRender.h','NeiUsedMagicPolicy.h','../../../../combo/gameplay/MagicMirrorCast.h']:
        donor=donor.replace('"'+header+'"','"'+str((ROOT/'soh/soh/Enhancements/randomizer'/header).resolve())+'"')
    donor=donor.replace('#include "soh/ResourceManagerHelpers.h"','extern "C" { uint8_t ResourceMgr_FileExists(const char*); uint8_t ResourceMgr_FileAltExists(const char*); bool ResourceMgr_IsAltAssetsEnabled(); }')
    donor=re.sub(r'void NeiUsedMagic_(\w+)\(',r'void Baseline_\1(',donor)
    baseline=Path(td)/'donor.cpp';baseline.write_text(donor)
    sources=['mm/mods/items/objects/object_firerod.c','mm/mods/items/objects/object_icerod.c','mm/src/code/sys_matrix.c','mm/src/code/z_skin_matrix.c','mm/2s2h/Rando/NeiUsedMagicPresentation.cpp','mm/2s2h/Rando/NeiGiPresentation.cpp','mm/2s2h/Enhancements/FrameInterpolation/FrameInterpolation.cpp','tests/mm_nei/interpolation_test.cpp',str(baseline)]
    objects=[]
    for i,source in enumerate(sources):
        is_c=source.endswith('.c');obj=str(Path(td)/f'{i}.o')
        subprocess.run([os.environ.get('CC' if is_c else 'CXX','cc' if is_c else 'c++'),'-std=gnu2x' if is_c else '-std=c++20','-I'+str(ROOT/'tests/mm_nei/stubs'),*flags(),'-O1','-ffunction-sections','-fdata-sections','-Wno-int-conversion',*(['-Wno-incompatible-pointer-types'] if is_c else []),'-c',str(ROOT/source),'-o',obj],check=True)
        objects.append(obj)
    binary=str(Path(td)/'interpolation')
    subprocess.run([os.environ.get('CXX','c++'),*objects,'-Wl,--gc-sections','-o',binary],check=True)
    subprocess.run([binary],check=True)
