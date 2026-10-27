import collections
import copy
import csv
import hashlib
import json
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent
SRC = Path('/workspace/scratch/3a7edcf1bb43/ComboShip')
OUT = ROOT / 'equipment-plando-20261007'
BUILD = '13901c677d0084e0305eec4672c0557f4434aea7'
assert subprocess.check_output(['git', '-C', str(SRC), 'rev-parse', 'HEAD'], text=True).strip() == BUILD
OLD = json.loads((ROOT / 'baseline/focused/Randomizer/01-MM-GI-Focused-20261007.json').read_text())
OUT.mkdir(exist_ok=True)
(OUT / 'Randomizer').mkdir(exist_ok=True)

sources = {
    'mmItems': SRC / 'mm/2s2h/Rando/StaticData/Items.cpp',
    'mmChecks': SRC / 'mm/2s2h/Rando/StaticData/Checks.cpp',
    'ootItems': SRC / 'soh/soh/Enhancements/randomizer/item_list.cpp',
}
mm_rows = [(m[1], m[2]) for m in re.finditer(
    r'^\s*RI\((RI_\w+),\s*"[^"\n]*",\s*"([^"\n]*)"', sources['mmItems'].read_text(), re.M)]
name_counts = collections.Counter(name for token, name in mm_rows if token != 'RI_TRIFORCE_PIECE_PREVIOUS')
mm_items = {(name if name_counts[name] == 1 and token != 'RI_TRIFORCE_PIECE_PREVIOUS' else token): token
            for token, name in mm_rows}
oot_items = {m[2]: m[1] for m in re.finditer(
    r'^\s*itemTable\[(RG_\w+)\]\s*=.*?"([^"\n]*)"', sources['ootItems'].read_text(), re.M)}
check_tokens = re.findall(r'^\s*RC\((RC_\w+),', sources['mmChecks'].read_text(), re.M)
def pretty(token):
    return ' '.join('HP' if part.lower() == 'hp' else part.capitalize() for part in token[3:].split('_'))
check_counts = collections.Counter(pretty(token) for token in check_tokens)
mm_checks = {(token if check_counts[pretty(token)] > 1 else pretty(token)): token for token in check_tokens}
assert len(mm_checks) == len(set(check_tokens))

# A row names both the placed item and the concrete model expected when received.
equipment = [
    ('Progressive Kokiri Sword', 'Kokiri Sword', 'Kokiri chain 1/3'),
    ('Progressive Kokiri Sword', 'Razor Sword', 'Kokiri chain 2/3'),
    ('Progressive Kokiri Sword', 'Gilded Sword', 'Kokiri chain 3/3'),
    ('Progressive Master Sword', 'Master Sword', 'Master chain 1/2'),
    ('Progressive Master Sword', 'True Master Sword', 'Master chain 2/2'),
    ("Progressive Biggoron's Sword", "Biggoron's Sword", 'Biggoron chain 1/2'),
    ("Progressive Biggoron's Sword", "Great Fairy's Sword", 'Biggoron chain 2/2'),
]
equipment += [(item, item, 'Equipment') for item in [
    'Four Sword', 'Trident', 'Cane of Byrna', "Champion's Tunic", "Sage's Tunic", 'Magic Tunic', 'Magic Cape',
    'Goron Tunic', 'Zora Tunic', 'Iron Boots', 'Hover Boots', 'Pegasus Boots', 'Climb Boots', "Roc's Boots",
    'Pendant of Memories', 'Deku Shield', 'Hylian Shield', 'Mirror Shield', 'Goddess Shield', 'Kite Shield',
    'Shield of Ikana', 'Megaton Hammer', "Iron Knuckle's Axe", "Giant's Knife", "Broken Goron's Sword",
    "Roc's Feather", "Roc's Cape", 'Power Bracelet', "Goron's Bracelet", 'Silver Gauntlets', 'Golden Gauntlets',
    'Bronze Scale', 'Silver Scale', 'Golden Scale', 'Stone of Agony', 'Quartz of Motion',
]]
assert len(equipment) == 43
equipment.append(("Hero's Shield", "Hero's Shield", 'Native MM equipment'))

# Keep the earlier dungeon, soul, Tingle and bottle cases after the equipment.
regressions = [(row['itemName'], row['itemName'], next(
    entry['group'] for entry in OLD['_review']['pickupOrder'] if entry['check'] == row['checkName']))
    for row in OLD['foreign']]
assert len(regressions) == 46
route = [entry['check'] for entry in OLD['_review']['pickupOrder'][:35]]
route += [
    'Clock Town East Treasure Chest Game Human', 'Clock Town East Treasure Chest Game Deku',
    'Clock Town East Treasure Chest Game Goron', 'Clock Town East Treasure Chest Game Zora',
    'Termina Field Scrub Large Crate', 'Termina Field Freestanding Rupee 01',
    'Termina Field Man In The Tree Rupee 01', 'Termina Field Man In The Tree Rupee 02',
]
assert len(route) == 43
route += [f'Deku Playground Day {day} Rupee {i:02}' for day in (1, 2, 3) for i in range(1, 7)]
route += [f'Termina Field Guay Rupee Drop {i:02}' for i in range(1, 21)]
route += ['Road To Ikana Chest', 'Road To Ikana Grotto Chest', 'Road To Southern Swamp Grotto Chest',
          'Southern Swamp Grotto Chest', 'Deku Palace Grotto Chest', 'Deku Palace Pot 01', 'Deku Palace Pot 02',
          'Southern Swamp Clear Pot 01']
route.append('Road To Ikana Pot')
assert len(route) == 90 and len(set(route)) == 90
assert set(route) <= set(mm_checks), sorted(set(route) - set(mm_checks))
early_route = [entry['check'] for entry in OLD['_review']['pickupOrder'][:14]]

base = copy.deepcopy(OLD)
base['foreign'] = []
base['mm']['placements'] = {key: value for key, value in OLD['mm']['placements'].items()
                            if key.endswith('Boss Warp')}
# Preserve the nine donor boss rewards and entrance/boss shuffle for hint tests.
base['_review'] = {}
base['mm']['settings']['gRando.Options.RO_SHUFFLE_SHOPS'] = 1
base['mm']['settings']['gRando.Options.RO_SHUFFLE_POT_DROPS'] = 1
base['mm']['settings']['gRando.Options.RO_SHUFFLE_CRATE_DROPS'] = 1
base['mm']['settings']['gRando.Options.RO_SHUFFLE_FREESTANDING_ITEMS'] = 1
base['mm']['settings']['gRando.Options.RO_SHUFFLE_TREE_DROPS'] = 1
base['mm']['settings']['gRando.Options.RO_STARTING_MAPS_AND_COMPASSES'] = 0
for key in base['oot']['settings']:
    if key.startswith('gRandoSettings.Starting') and any(word in key for word in [
            'Sword', 'Shield', 'Tunic', 'Boots', 'Strength', 'Scale', 'StoneOfAgony', 'MegatonHammer']):
        base['oot']['settings'][key] = 0

checklist = []
files = []
def make_seed(name, rows, locations, seed_number, owner='oot'):
    data = copy.deepcopy(base)
    data['seed'] = name.removesuffix('.json')
    data['masterSeed'] = data['displaySeed'] = seed_number
    digits = f'{seed_number:010d}'
    data['file_hash'] = [int(digits[i:i + 2]) for i in range(0, 10, 2)]
    order = []
    for i, ((placed, expected, group), check) in enumerate(zip(rows, locations, strict=True), 1):
        row_owner = 'mm' if placed == "Hero's Shield" else owner
        if row_owner == 'oot':
            assert placed in oot_items, placed
            data['mm']['placements'][check] = placed + ' (OOT)'
            data['foreign'].append({'checkGame': 'mm', 'checkName': check, 'itemGame': 'oot', 'itemName': placed,
                                    'displayName': placed + ' (OOT)', 'advancement': True, 'category': 'major'})
        else:
            assert placed in mm_items, placed
            data['mm']['placements'][check] = placed
        entry = {'order': i, 'check': check, 'placedItem': placed, 'expectedGI': expected, 'group': group,
                 'itemGame': row_owner}
        order.append(entry)
        checklist.append({'seed': name, 'order': i, 'MM check': check, 'placed item': placed,
                          'expected GI / tier': expected, 'route': 'Foreign OoT in MM' if row_owner == 'oot' else 'Native MM',
                          'group': group, 'Alt ON': '', 'Alt OFF': '', 'ON-OFF-ON': '', 'shimmer / particles': '',
                          'clipping / stretching': '', 'notes': ''})
    data['_review'] = {'title': name, 'build': BUILD, 'purpose': 'Equipment and GI runtime test',
                       'pickupGame': 'MM', 'pickupOrder': order, 'freshSaveRequired': True,
                       'expectedGIsAreSourceDerived': True, 'runtimeAcceptance': 'untested',
                       'entranceShuffle': copy.deepcopy(OLD['_review']['entranceShuffle']),
                       'bossRewardReference': copy.deepcopy(OLD['_review']['bossRewardReference'])}
    path = OUT / 'Randomizer' / name
    path.write_text(json.dumps(data, indent=2, ensure_ascii=False) + '\n')
    files.append(path)
    return data

main = make_seed('01-MM-All-Equipment-Progressive-20261007.json', equipment + regressions, route, 2387439516)
direct = [(expected, expected, 'Direct equipment control') for placed, expected, group in equipment]
make_seed('02-MM-Direct-Equipment-Control.json', direct, route[:44], 2387439517)
native = [('Progressive Sword', expected, group) for placed, expected, group in equipment[:3]]
native += [(placed, 'Real Master Sword' if expected == 'True Master Sword' else expected, group)
           for placed, expected, group in equipment[3:7]]
native += [('Progressive Hammer', 'Megaton Hammer', 'Hammer chain 1/2'),
           ('Progressive Hammer', "Iron Knuckle's Axe", 'Hammer chain 2/2')]
native += [(item, item, 'Native custom equipment') for item in ["Champion's Tunic", "Sage's Tunic", 'Magic Tunic',
                                                             'Magic Cape', 'Pendant of Memories']]
native.append(("Hero's Shield", "Hero's Shield", 'Native MM equipment'))
make_seed('03-MM-Native-Progressive-Control.json', native, route[:15], 2387439518, 'mm')
# The older, separately named donor Goron Sword entry currently resolves only to Biggoron's Sword.
make_seed('04-MM-Legacy-Goron-Sword-Control.json',
          [('Progressive Goron Sword', "Biggoron's Sword", 'Legacy donor sword control')], early_route[:1], 2387439519)

with (OUT / 'Pickup-Checklist.csv').open('w', newline='', encoding='utf-8-sig') as stream:
    writer = csv.DictWriter(stream, fieldnames=list(checklist[0])); writer.writeheader(); writer.writerows(checklist)
catalog = {'build': BUILD, 'mmChecks': mm_checks, 'mmItems': mm_items, 'ootItems': oot_items,
           'mmRows': mm_rows, 'equipmentCoverage': sorted({expected for placed, expected, group in equipment}),
           'mmCheckTypes': {match[1]: match[2] for match in re.finditer(
               r'^\s*RC\((RC_\w+),\s*(RCTYPE_\w+)', sources['mmChecks'].read_text(), re.M)},
           'sourceHashes': {str(path.relative_to(SRC)): hashlib.sha256(path.read_bytes()).hexdigest()
                            for path in sources.values()}}
(OUT / 'Source-Coverage.json').write_text(json.dumps(catalog, indent=2) + '\n')
print(json.dumps({'files': [path.name for path in files], 'mainTestPickups': 90,
                  'equipmentChecks': 44, 'foreignEquipmentChecks': 43, 'regressionChecks': 46,
                  'progressiveSwordCopies': {'Kokiri': 3, 'Master': 2, 'Biggoron': 2},
                  'checklistRows': len(checklist)}, indent=2))
