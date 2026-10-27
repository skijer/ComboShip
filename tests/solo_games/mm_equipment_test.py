#!/usr/bin/env python3
"""MM native solo must retain saved equipment ownership when the legacy Combo bridge is off."""
from pathlib import Path
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[2]
source=(ROOT/'mm/src/overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_equipment.c').read_text()
def block(marker):
    start=source.index(marker);opening=source.index('{',start);end=opening+1;depth=1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[start:end]
helper=block('static u8 KaleidoEquip_UsesLegacyFreeEquipment(') if 'static u8 KaleidoEquip_UsesLegacyFreeEquipment(' in source else ''
program=r'''
#include <cassert>
#include <cstdint>
#include <iostream>
using u8=uint8_t;
enum {CELL_EMPTY,CELL_SWORD,CELL_SWORD_MASTER,CELL_SWORD_BGS,CELL_SHIELD_DEKU,CELL_SHIELD_OOT_MIRROR,CELL_SHIELD,CELL_TUNIC,CELL_BOOTS,CELL_DISPLAY};
enum {EQUIP_TYPE_SWORD,EQUIP_TYPE_SHIELD};
constexpr int EQUIP_VALUE_SWORD_KOKIRI=1,EQUIP_VALUE_SWORD_GILDED=3,ITEM_SWORD_KOKIRI=10,EQUIP_SLOT_B=0;
constexpr int FCI_KOKIRI_SWORD=0,FC_OOT_SWORD_MASTER=0,FC_OOT_SWORD_BIGGORON=1,FC_OOT_TUNIC_GORON=2,FC_OOT_TUNIC_ZORA=3,FC_OOT_BOOTS_IRON=4,FC_OOT_BOOTS_HOVER=5;
constexpr int FC_SHIELD_DEKU=1,FC_SHIELD_MIRROR_OOT=2,FC_SHIELD_HYLIAN=4,FC_SHIELD_IKANA=8;
struct EquipCell{int equipType,index;};
struct {int comboObtainedFc[1]={};int comboObtained[6]={};int shieldOwned=0,vanillaShieldSkin=0;} nei;
auto* Nei_Save(){return &nei;}
int legacyActive=-1,equip[2]={1,1},button=ITEM_SWORD_KOKIRI;
int FleetShipCombo_GetActiveGame(){return legacyActive;}
int WeaponUpgrade_KokiriLevel(){return 0;}
bool WeaponUpgrade_HasGreatFairy(){return false;}
int ExtEquip_GetCurrent(int){return 0;}
bool ExtEquip_HasItem(int,u8){return false;}
#define GET_CUR_EQUIP_VALUE(type) equip[type]
#define BUTTON_ITEM_EQUIP(form,slot) button
'''+helper+'\n'+block('static u8 KaleidoEquip_CellOwned(')+r'''
bool owned(int type,int index=0){EquipCell c{type,index};return KaleidoEquip_CellOwned(&c);}
int main(){
#ifdef COMBO_BUILD
    // The native vanilla starter remains usable with no cross-game receipt.
    assert(owned(CELL_SWORD) && owned(CELL_SHIELD,1));
    for(int type:{CELL_SWORD_MASTER,CELL_SWORD_BGS,CELL_SHIELD_DEKU,CELL_SHIELD_OOT_MIRROR})assert(!owned(type));
    assert(!owned(CELL_TUNIC,1) && !owned(CELL_BOOTS,1));
    // A native randomizer start without equipment cannot equip an unearned sword/shield.
    equip[0]=equip[1]=0;button=255;
    assert(!owned(CELL_SWORD) && !owned(CELL_SHIELD,1));
    nei.comboObtainedFc[FCI_KOKIRI_SWORD]=1;nei.shieldOwned=FC_SHIELD_HYLIAN;
    assert(owned(CELL_SWORD) && owned(CELL_SHIELD,1));
    nei.comboObtained[FC_OOT_SWORD_MASTER]=1;assert(owned(CELL_SWORD_MASTER));
    // Combined play retains the same local ownership contract.
    legacyActive=1;assert(owned(CELL_SWORD_MASTER));assert(!owned(CELL_SHIELD_DEKU));
    std::cout<<"MM native equipment progression passed\n";
#else
    // Preserve the donor's optional two-process/standalone equipment behavior.
    assert(owned(CELL_SWORD_MASTER) && owned(CELL_SHIELD_DEKU) && owned(CELL_TUNIC,1));
    legacyActive=1;assert(!owned(CELL_SWORD_MASTER) && !owned(CELL_SHIELD_DEKU) && !owned(CELL_TUNIC,1));
    std::cout<<"MM legacy equipment policy preserved\n";
#endif
}
'''
with tempfile.TemporaryDirectory(prefix='mm-solo-equipment-') as directory:
    tmp=Path(directory);cpp=tmp/'test.cpp';cpp.write_text(program)
    for name,flags in [('native',['-DCOMBO_BUILD']),('legacy',[])]:
        binary=tmp/name
        subprocess.run(['g++','-std=c++20',*flags,str(cpp),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)
