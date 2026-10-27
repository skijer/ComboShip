"""The Mirror is a standalone, append-only native randomizer identity."""
from pathlib import Path
import re
import subprocess
ROOT = Path(__file__).resolve().parents[2]
# Public PR66 has the exact pre-shuffle identities; CI fetches this immutable pin.
BASE = '0cd35e4d25f853dc68995dd88e8b1c177840e301'
# Preserve every previous serialized identity, including RI_MAX_TRAP (a real draw id).
for path, end, added in [
    ('soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerGet.h', 'RG_MAX', 'RG_MAGIC_MIRROR'),
    ('soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerCheck.h', 'RC_MAX', 'RC_LOST_WOODS_MAGIC_MIRROR_STUMP'),
    ('soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerSettingKey.h', 'RSK_MAX', 'RSK_SHUFFLE_MAGIC_MIRROR'),
    ('soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerInf.h', 'RAND_INF_MAX', 'RAND_INF_MAGIC_MIRROR_STUMP'),
]:
    current = (ROOT / path).read_text()
    assert f'RANDO_ENUM_ITEM({added})' in current, added
    original = subprocess.check_output(['git', 'show', f'{BASE}:{path}'], cwd=ROOT, text=True)
    before = re.findall(r'RANDO_ENUM_ITEM\((\w+)\)', original)
    after = re.findall(r'RANDO_ENUM_ITEM\((\w+)\)', current)
    assert after[:before.index(end)] == before[:before.index(end)], path
    assert after[before.index(end)] == added, path
mm = (ROOT / 'mm/2s2h/Rando/Types.h').read_text()
old_mm = subprocess.check_output(['git', 'show', BASE + ':mm/2s2h/Rando/Types.h'], cwd=ROOT, text=True)
for prefix, end, addition in [('RC', 'RC_MAX', 'RC_LOST_WOODS_MAGIC_MIRROR_STUMP'),
                              ('RI', 'RI_MAX', 'RI_OOT_NEI_MAGIC_MIRROR'),
                              ('RO', 'RO_MAX', 'RO_SHUFFLE_MAGIC_MIRROR')]:
    before = re.findall(r'^\s*(' + prefix + r'_\w+)\s*,', old_mm, re.M)
    after = re.findall(r'^\s*(' + prefix + r'_\w+)\s*,', mm, re.M)
    idx = before.index(end)
    assert after[:idx] == before[:idx], prefix + ' ordinal changed'
    assert after[idx] == addition, prefix + ' must be appended, including after RI_MAX_TRAP'
old_fc = subprocess.check_output(['git', 'show', BASE + ':soh/soh/FleetShipCombo/FleetComboItems.h'], cwd=ROOT, text=True)
new_fc = (ROOT / 'soh/soh/FleetShipCombo/FleetComboItems.h').read_text()
ids = lambda text: re.findall(r'X\((FCI_\w+),', text)
assert ids(new_fc) == ids(old_fc) + ['FCI_MAGIC_MIRROR']

assert 'RI_OOT_NEI_MAGIC_MIRROR,' in mm
assert 'RO_SHUFFLE_MAGIC_MIRROR,' in mm
assert 'RC_LOST_WOODS_MAGIC_MIRROR_STUMP,' in mm
for host in ['soh/soh', 'mm/2s2h']:
    table = (ROOT / host / 'FleetShipCombo/FleetComboItems.h').read_text()
    assert 'X(FCI_MAGIC_MIRROR, 1, FCI_F_NONE, RG_MAGIC_MIRROR, RI_OOT_NEI_MAGIC_MIRROR,' in table
assert (ROOT / 'soh/soh/FleetShipCombo/FleetComboItems.h').read_bytes() == (ROOT / 'mm/2s2h/FleetShipCombo/FleetComboItems.h').read_bytes()
print('PASS append-only Mirror item/check/option/flag and cross-game identity registry')
