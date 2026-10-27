"""Every existing rigid gameplay representation must select the authored model."""
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
for host in ['soh','mm']:
 s=(ROOT/host/'mods/extended_equipment.c').read_text()
 for model in ['BYRNA','TRIDENT','DIVINE_SHIELD','SHEIKAH_SHIELD','IKANA_SHIELD']:
  assert 'NeiEquipment_ModelDL(NEI_EQUIPMENT_'+model in s,(host,model)
 for path in ['mods/equipment/behaviors/equip_ikaxe.c','src/overlays/actors/ovl_En_Boom/z_en_boom.c']:
  assert 'NeiEquipment_ModelDL(NEI_EQUIPMENT_AXE' in (ROOT/host/path).read_text(),(host,path)
 s=(ROOT/host/'mods/equipment/behaviors/equip_foursword.c').read_text()
 assert 'NeiEquipment_HasFourSword()' in s,host
 assert 'NeiEquipment_ModelDL(NEI_EQUIPMENT_FOUR_BLADE' in s,host
 assert 'NeiEquipment_ModelDL(NEI_EQUIPMENT_FOUR_HILT' in s,host
assert (ROOT/'soh/mods/equipment/nei_equipment_resources.inc').read_bytes()==(ROOT/'mm/mods/equipment/nei_equipment_resources.inc').read_bytes()
print('PASS: both hosts shield hand/back/surf, Byrna/trident, Four Sword dispatch and held/thrown axe connected')
