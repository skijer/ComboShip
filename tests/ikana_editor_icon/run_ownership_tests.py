#!/usr/bin/env python3
"""Execute both hosts' real grant/remove stores. No equipment implies ownership."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_mm_scene_randomization_tests import function
for host in ('soh', 'mm'):
    equipment = (ROOT / host / 'mods/extended_equipment.c').read_text()
    source = '''#include <cassert>
#include <cstdint>
#include <cstdio>
using s16=int16_t; using u8=uint8_t; using u32=uint32_t;
#define EQUIP_TYPE_SHIELD 1
#define EXT_EQUIP_OWNED_SHIFT 16
#include "mm/2s2h/FleetShipCombo/FleetComboIds.h"
struct Save { u32 extEquipOwnedBits=0; uint16_t shieldOwned=0; u8 vanillaShieldSkin=0; } state;
Save* Nei_Save() { return &state; }
int equipped=0;
int nativeShield=2;
#define EQUIP_VALUE_SHIELD_MIRROR 2
#define EQUIP_VALUE_SHIELD_NONE 0
#define GET_CUR_EQUIP_VALUE(type) (nativeShield)
void Inventory_ChangeEquipment(int value) { nativeShield=value; }
u8 ExtEquip_GetCurrent(s16) { return equipped; }
void ExtEquip_SetSlot(s16, u8 value) { equipped=value; }
'''
    source += '\n'.join(function(equipment, name) for name in (
        'ExtEquip_GetBit', 'ExtEquip_HasItem', 'ExtEquip_GiveItem', 'ExtEquip_RemoveItem'))
    source += '''
int main() {
    state.shieldOwned=FC_SHIELD_MIRROR_OOT;
    const auto before=state;
    equipped=3;
    assert(!ExtEquip_HasItem(1,3));
    assert(state.extEquipOwnedBits==before.extEquipOwnedBits && state.shieldOwned==before.shieldOwned);
    ExtEquip_GiveItem(1,3);
    assert(ExtEquip_HasItem(1,3));
    assert(state.shieldOwned==(FC_SHIELD_MIRROR_OOT|FC_SHIELD_IKANA));
    ExtEquip_RemoveItem(1,3);
    assert(!ExtEquip_HasItem(1,3) && equipped==0);
    assert(state.shieldOwned==FC_SHIELD_MIRROR_OOT);
    ExtEquip_GiveItem(1,1);
    assert(state.shieldOwned==FC_SHIELD_MIRROR_OOT);
    ExtEquip_RemoveItem(1,1);
    assert(state.shieldOwned==FC_SHIELD_MIRROR_OOT);
    equipped=0; nativeShield=2;
    ExtEquip_GiveItem(1,3);
    ExtEquip_RemoveItem(1,3);
    // Only MM has an equivalent native shield nibble. Removing Ikana must
    // release it too so the existing sync publisher cannot infer it again.
    NATIVE_REMOVAL_ASSERT
    state.vanillaShieldSkin=2; nativeShield=2;
    ExtEquip_GiveItem(1,3);
    ExtEquip_RemoveItem(1,3);
    assert(nativeShield==2 && state.shieldOwned==FC_SHIELD_MIRROR_OOT);
    puts("PASS explicit Ikana ownership grant/remove, equipped-only stays unowned, OoT Mirror preserved");
}
'''
    source=source.replace('NATIVE_REMOVAL_ASSERT', 'assert(nativeShield==0);' if host=='mm' else 'assert(nativeShield==2);')
    with tempfile.TemporaryDirectory() as temp:
        path=Path(temp)/'test.cpp';path.write_text(source)
        binary=Path(temp)/'test'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-I'+str(ROOT),str(path),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)
