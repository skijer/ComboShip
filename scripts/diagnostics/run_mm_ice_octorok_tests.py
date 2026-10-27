#!/usr/bin/env python3
"""Execute selected production Octorok states and compile the complete native actor."""
from pathlib import Path
import argparse
import os
import re
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[2]

def function(source,name):
    match=re.search(r'^[^\n;{}]*\b'+re.escape(name)+r'\([^;{}]*\)\s*\{',source,re.M)
    if not match:
        raise ValueError(name)
    end=match.end(); depth=1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}'); end+=1
    return source[match.start():end]+'\n'

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize',action='store_true')
    args=parser.parse_args()
    flags=['-std=gnu2x','-O1','-g','-DNDEBUG','-DF3DEX_GBI_2','-DLOG_LEVEL_GAME_PRINTS=0',
           '-Werror=implicit-function-declaration','-Wno-incompatible-pointer-types','-Wno-int-conversion']
    for path in ('soh/include','soh/src','soh/assets','soh','libultraship/include','combo'):
        flags.append('-I'+str(ROOT/path))
    for path in ('CMake/soh-cvars.cmake','CMake/lus-cvars.cmake'):
        for key,value in re.findall(r'set\((CVAR_PREFIX_\w+)\s+"?([^\s"\)]+)',(ROOT/path).read_text()):
            flags.append(f'-D{key}="{value}"')
    if args.sanitize:
        flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie']
    actor='soh/src/overlays/actors/ovl_En_Okuta/z_en_okuta.c'
    source=(ROOT/actor).read_text()
    declarations='''#include "overlays/actors/ovl_En_Okuta/z_en_okuta.h"
#include "soh/Enhancements/Items/MmIceArrows.h"
#include "objects/object_okuta/object_okuta.h"
void EnOkuta_Appear(EnOkuta*,PlayState*); void EnOkuta_Hide(EnOkuta*,PlayState*);
void EnOkuta_Shoot(EnOkuta*,PlayState*); void EnOkuta_WaitToAppear(EnOkuta*,PlayState*);
void EnOkuta_WaitToShoot(EnOkuta*,PlayState*); void EnOkuta_WaitToDie(EnOkuta*,PlayState*);
void EnOkuta_Die(EnOkuta*,PlayState*); void EnOkuta_Freeze(EnOkuta*,PlayState*);
void EnOkuta_FrozenInIceBlock(EnOkuta*,PlayState*);
void EnOkuta_SetupDie(EnOkuta*); void EnOkuta_SetupWaitToDie(EnOkuta*);
void EnOkuta_SpawnRipple(EnOkuta*,PlayState*);
'''
    start=source.index('static ColliderCylinderInit sOctorockColliderInit =')
    declarations+=source[start:source.index('\n};',start)+3]+'\n'
    names=['EnOkuta_Destroy','EnOkuta_SetupWaitToShoot','EnOkuta_SetupFreeze','EnOkuta_Freeze',
           'EnOkuta_UpdateHeadScale','EnOkuta_FrozenInIceBlock','EnOkuta_ColliderCheck','EnOkuta_Update']
    with tempfile.TemporaryDirectory(prefix='mm-ice-octorok-') as temp:
        states=Path(temp)/'states.c'; states.write_text(declarations+''.join(function(source,n) for n in names))
        block='soh/src/overlays/actors/ovl_Bg_MmIceblock/z_bg_mm_iceblock.c'
        sources=['soh/tests/mm_ice_octorok_test.c',states,block]
        for mode in ('solo','combo'):
            modeflags=flags+(['-DCOMBO_BUILD'] if mode=='combo' else [])
            # Full unmodified translation-unit structure, real headers and no runtime stubs.
            subprocess.run([os.environ.get('CC','cc'),*modeflags,'-c',actor,'-o',Path(temp)/(mode+'.o')],cwd=ROOT,check=True)
            binary=Path(temp)/('states-'+mode)
            subprocess.run([os.environ.get('CC','cc'),*modeflags,*sources,'-lm','-o',binary],cwd=ROOT,check=True)
            subprocess.run([binary],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})

if __name__=='__main__': main()
