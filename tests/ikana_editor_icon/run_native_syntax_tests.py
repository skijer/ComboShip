#!/usr/bin/env python3
"""Compile edited ownership/accessor arms against both actual game headers."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_mm_scene_randomization_tests import function
for host in ('mm','soh'):
    source=(ROOT/host/'mods/extended_equipment.c').read_text()
    prefix='''#include "z64.h"
#include "functions.h"
#include "variables.h"
#include "mods/extended_equipment.h"
#include "mods/nei_save.h"
#include "combo/menu/ComboItemIconOwnership.h"
'''
    prefix+='#include "'+('mm/2s2h' if host=='mm' else 'soh/soh')+'/FleetShipCombo/FleetComboIds.h"\n'
    # Unchanged adjacent icon entries are declared by the actual generated asset
    # headers in each game. Accessor control flow and edited ownership are copied
    # verbatim, leaving unrelated behavior/model unity imports out of this TU.
    prefix+='static const char* sExtEquipIconPaths[4][3];\n'
    code=prefix+'\n'.join(function(source,n) for n in ('ExtEquip_GetBit','ExtEquip_HasItem','ExtEquip_GiveItem','ExtEquip_RemoveItem','ExtEquip_GetIcon'))
    with tempfile.TemporaryDirectory() as temp:
        p=Path(temp)/'native.c';p.write_text(code)
        flags=['-DF3DEX_GBI_2','-DCOMBO_BUILD','-DMM_BUILD_DLL' if host=='mm' else '-DSOH_BUILD_DLL','-DCONTROLLERBUTTONS_T=uint32_t','-DLOG_LEVEL_GAME_PRINTS=0']
        flags+=['-I'+str(ROOT/p) for p in (host,host+'/include',host+'/include/PR',host+'/src',host+'/assets',('mm/2s2h' if host=='mm' else 'soh/soh'),'libultraship/include','')]
        subprocess.run([os.environ.get('CC','cc'),'-std=gnu11',*flags,'-Werror=implicit-function-declaration','-fsyntax-only',str(p)],check=True)
        print('PASS actual '+host+' headers: edited owner grant/remove and icon getter')
