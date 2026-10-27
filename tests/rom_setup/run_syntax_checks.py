#!/usr/bin/env python3
"""Compile the changed launcher/screen/extractor translation units against real headers."""
from pathlib import Path
import argparse
import os
import re
import shlex
import subprocess

ROOT=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--standalone',action='store_true',help='check native OoT files without COMBO_BUILD')
parser.add_argument('--source-rev',help='compile a recorded revision to identify baseline failures')
args=parser.parse_args()
prefixes=dict(re.findall(r'set\((CVAR_PREFIX_\w+)\s+"([^"]+)"\)',(ROOT/'CMake/soh-cvars.cmake').read_text()))
defines=[f'-D{key}="{value}"' for key,value in prefixes.items()]
release=re.search(r'set\(COMBO_RELEASE_VERSION\s+"([^"]+)"',(ROOT/'CMakeLists.txt').read_text()).group(1)
defines.append(f'-DCOMBO_RELEASE_VERSION="{release}"')
for key,value in re.findall(r'set\((CVAR_\w+)\s+"([^"]+)"\)',(ROOT/'CMake/lus-cvars.cmake').read_text()):
    for prefix,replacement in prefixes.items():value=value.replace('${'+prefix+'}',replacement)
    defines.append(f'-D{key}="{value}"')
flags=['-std=c++20','-DCOMBO_BUILD','-DSOH_BUILD_DLL','-DF3DEX_GBI_2','-DIMGUI_DEFINE_MATH_OPERATORS=',
       '-DLOG_LEVEL_GAME_PRINTS=0','-DCONTROLLERBUTTONS_T=uint32_t','-DNON_EQUIVALENT','-DNON_MATCHING',*defines]
flags+=['-I'+str(ROOT/p) for p in ['soh/include','soh/include/PR','soh/src','soh','soh/soh','soh/assets',
                                'soh/soh/Extractor','libultraship/include','libultraship/src','combo','combo/menu']]
sources=['combo/ComboShip.cpp','combo/ComboRandoHeadless.cpp','combo/gui/ComboExtractScreen.cpp',
               'soh/soh/Extractor/Extract.cpp','soh/soh/OTRGlobals.cpp',
               'soh/soh/Enhancements/randomizer/settings.cpp']
if args.standalone:
    flags.remove('-DCOMBO_BUILD')
    flags.remove('-DSOH_BUILD_DLL')
    sources=['soh/soh/Enhancements/randomizer/settings.cpp','soh/soh/OTRGlobals.cpp']
for source in sources:
    source_args=[str(ROOT/source)]
    content=None
    if args.source_rev:
        content=subprocess.check_output(['git','show',args.source_rev+':'+source],cwd=ROOT,text=True)
        source_args=['-iquote',str((ROOT/source).parent),'-x','c++','-']
    result=subprocess.run([*shlex.split(os.environ.get('CXX','c++')),*flags,'-fsyntax-only',*source_args],
                          input=content,cwd=ROOT,capture_output=True,text=True)
    if result.returncode:raise RuntimeError(source+'\n'+result.stdout+result.stderr)
    print(f'PASS real-header syntax: {source} ({result.stderr.count("warning:")} existing compiler warnings)')
