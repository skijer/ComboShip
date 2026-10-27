import collections
import csv
import hashlib
import json
import re
import shutil
import subprocess
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent
SRC = Path('/workspace/scratch/3a7edcf1bb43/ComboShip')
OUT = ROOT / 'equipment-plando-20261007'
catalog = json.loads((OUT / 'Source-Coverage.json').read_text())
old = json.loads((ROOT / 'baseline/focused/Randomizer/01-MM-GI-Focused-20261007.json').read_text())
BUILD = catalog['build']

def block(source, signature):
    start = source.index(signature)
    opening = source.index('{', start)
    depth = 0
    for end in range(opening, len(source)):
        depth += (source[end] == '{') - (source[end] == '}')
        if depth == 0:
            return source[start:end + 1]
    raise AssertionError(signature)

def case_slice(source, first, following, start=0):
    left = source.index(first, start)
    return source[left:source.index(following, left)]

checks_src = (SRC / 'mm/2s2h/Rando/StaticData/Checks.cpp').read_text()
items_src = (SRC / 'mm/2s2h/Rando/StaticData/Items.cpp').read_text()
utils_src = (SRC / 'mm/2s2h/ShipUtils.cpp').read_text()
functions = [block(utils_src, 'std::string convertEnumToReadableName(')]
for signature in ('static std::string ComboPrettifyCheck(',
                  'static const std::unordered_map<RandoCheckId, std::string>& ComboCheckEmitNames()',
                  'const std::string& GetCheckDisplayName(', 'RandoCheckId GetCheckIdFromDisplayName('):
    functions.append(block(checks_src, signature))
for signature in ('RandoItemId GetItemIdFromName(',
                  'static const std::unordered_map<RandoItemId, std::string>& ComboItemEmitNames()',
                  'const std::string& GetItemDisplayName(', 'RandoItemId GetItemIdFromDisplayName('):
    functions.append(block(items_src, signature))

compiled = '''#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
'''
check_tokens = list(catalog['mmChecks'].values())
item_tokens = [token for token, name in catalog['mmRows']]
compiled += 'enum RandoCheckId {' + ','.join(['RC_UNKNOWN=0'] + [x for x in check_tokens if x != 'RC_UNKNOWN']) + '};\n'
compiled += 'enum RandoItemId {' + ','.join(['RI_UNKNOWN=0'] + [x for x in item_tokens if x != 'RI_UNKNOWN']) + '};\n'
compiled += 'struct Check { const char* name; };\nstd::map<RandoCheckId,Check> Checks={\n'
compiled += ',\n'.join('{' + token + ',{' + json.dumps(token) + '}}' for token in check_tokens) + '};\n'
compiled += 'struct Item { const char* spoilerName; const char* name; };\nstd::map<RandoItemId,Item> Items={\n'
compiled += ',\n'.join('{' + token + ',{' + json.dumps(token) + ',' + json.dumps(name) + '}}'
                       for token, name in catalog['mmRows']) + '};\n'
compiled += '\n'.join(functions)

# Compile the actual donor sword-tier selection cases with explicit inventory inputs.
oot_src = (SRC / 'soh/soh/Enhancements/randomizer/item.cpp').read_text()
donor_cases = case_slice(oot_src, 'case RG_PROGRESSIVE_GORONSWORD: // todo progressive?', 'case RG_STONE_OF_AGONY:')
rg_tokens = sorted(set(re.findall(r'\bRG_\w+', donor_cases)))
compiled += '\nenum RandomizerGet {' + ','.join(rg_tokens) + '};\n'
compiled += '''
enum { EQUIP_INV_SWORD_KOKIRI=0, EQUIP_INV_SWORD_MASTER=1, EQUIP_INV_SWORD_BIGGORON=2, ITEM_HAMMER=8, ITEM_NONE=255 };
struct LogicFixture {
    struct Save { struct Inventory { unsigned equipment=0; } inventory; } save;
    unsigned hammer=ITEM_NONE;
    Save* GetSaveContext() { return &save; }
    unsigned CurrentInventory(unsigned) { return hammer; }
};
LogicFixture fixture;
bool hasRazor=false;
bool WeaponUpgrade_HasRazor() { return hasRazor; }
RandomizerGet ResolveDonor(RandomizerGet rg) {
    auto* logic=&fixture;
    RandomizerGet actual=rg;
    switch(rg) {
'''
compiled += donor_cases + '\n default: break; } return actual; }\n'

# Compile the actual native MM chain selection cases with explicit ownership inputs.
native_src = (SRC / 'mm/2s2h/Rando/ConvertItem.cpp').read_text()
native_start = native_src.index('// OoT chains -> their concrete tier')
native_cases = case_slice(native_src, 'case RI_OOT_PROGRESSIVE_HAMMER:', 'case RI_OOT_STONE_OF_AGONY:', native_start)
native_cases += case_slice(native_src, 'case RI_PROGRESSIVE_SWORD:', 'default:', native_start)
compiled += '''
enum { FC_OOT_SWORD_MASTER=0, FC_OOT_SWORD_BIGGORON=1, EQUIP_TYPE_SWORD=0,
       EQUIP_VALUE_SWORD_NONE=0, EQUIP_VALUE_SWORD_KOKIRI=1, EQUIP_VALUE_SWORD_RAZOR=2,
       ITEM_SWORD_KOKIRI=10, ITEM_SWORD_RAZOR=11 };
struct NeiFixture { bool ootHammerOwned=false; int comboObtained[2]={0,0}; } nei;
NeiFixture* Nei_Save() { return &nei; }
int nativeSword=0;
#define GET_CUR_EQUIP_VALUE(x) nativeSword
#define STOLEN_ITEM_1 0
#define STOLEN_ITEM_2 0
RandoItemId ResolveNative(RandoItemId randoItemId) { switch(randoItemId) {
'''
compiled += native_cases + '\n default: return randoItemId; } }\n'

assertions = []
records = []
seeds = {}
for path in sorted((OUT / 'Randomizer').glob('*.json')):
    data = json.loads(path.read_text())
    seeds[path.name] = data
    assert data['fileType'] == 'ComboShipRandomizer' and data['version'] == 1
    assert data['startingGame'] == 'MM' and data['sharedItems'] == []
    assert data['_review']['build'] == BUILD
    assert data['masterSeed'] == data['displaySeed']
    digits = f"{data['displaySeed']:010d}"
    assert data['file_hash'] == [int(digits[i:i + 2]) for i in range(0, 10, 2)]
    assert data['oot']['placements'] == old['oot']['placements']
    for key in ['MapsCompassesGiveInformation', 'ShuffleDungeonsEntrances', 'ShuffleBossEntrances',
                'MixDungeons', 'MixBosses', 'DecoupleEntrances', 'MQDungeons']:
        assert data['oot']['settings']['gRandoSettings.' + key] == old['oot']['settings']['gRandoSettings.' + key]
    assert data['mm']['settings']['gRando.Options.RO_STARTING_MAPS_AND_COMPASSES'] == 0
    assert not any(any(word in token for word in ['SWORD', 'SHIELD', 'TUNIC', 'BOOTS', 'CAPE', 'PENDANT'])
                   for token in data['mm']['settings']['gRando.StartingItems'])
    foreign = {entry['checkName']: entry for entry in data['foreign']}
    assert len(foreign) == len(data['foreign'])
    assert set(foreign) <= data['mm']['placements'].keys()
    order = data['_review']['pickupOrder']
    assert len({entry['check'] for entry in order}) == len(order)
    for entry in order:
        assert entry['check'] in data['mm']['placements']
        if entry['itemGame'] == 'oot':
            assert foreign[entry['check']]['itemName'] == entry['placedItem']
        else:
            assert entry['check'] not in foreign
            assert entry['placedItem'] in catalog['mmItems']
    for check, item in data['mm']['placements'].items():
        assert check in catalog['mmChecks'], check
        option_by_type = {'RCTYPE_POT': 'RO_SHUFFLE_POT_DROPS', 'RCTYPE_CRATE': 'RO_SHUFFLE_CRATE_DROPS',
                          'RCTYPE_FREESTANDING': 'RO_SHUFFLE_FREESTANDING_ITEMS',
                          'RCTYPE_TREE': 'RO_SHUFFLE_TREE_DROPS'}
        check_type = catalog['mmCheckTypes'][catalog['mmChecks'][check]]
        if check_type in option_by_type:
            assert data['mm']['settings']['gRando.Options.' + option_by_type[check_type]] == 1, (check, check_type)
        assertions.append('assert(GetCheckIdFromDisplayName(' + json.dumps(check) + ')==' + catalog['mmChecks'][check] + ');')
        if check in foreign:
            row = foreign[check]
            assert row['checkGame'] == 'mm' and row['itemGame'] == 'oot'
            assert row['itemName'] in catalog['ootItems']
            assert item == row['itemName'] + ' (OOT)' and row['displayName'] == item
            assert row['advancement'] is True and row['category'] == 'major'
            assertions.append('assert(GetItemIdFromName("RI_COMBO_FOREIGN")==RI_COMBO_FOREIGN);')
        else:
            assert item in catalog['mmItems'], item
            assertions.append('assert(GetItemIdFromDisplayName(' + json.dumps(item) + ')==' + catalog['mmItems'][item] + ');')
    records.append({'file': path.name, 'testPickups': len(order), 'foreignPickups': len(foreign),
                    'MMHintSupportChecks': 4, 'OoTHintSupportChecks': 9,
                    'sha256': hashlib.sha256(path.read_bytes()).hexdigest()})

main = seeds['01-MM-All-Equipment-Progressive-20261007.json']
assert len(main['_review']['pickupOrder']) == 90
assert len(main['foreign']) == 89
assert {entry['expectedGI'] for entry in main['_review']['pickupOrder'][:44]} == set(catalog['equipmentCoverage'])
assert len(catalog['equipmentCoverage']) == 44
# Independently audit every donor equipment row and every live extended-equipment item.
donor_item_table = (SRC / 'soh/soh/Enhancements/randomizer/item_list.cpp').read_text()
required_equipment = {match[2] for match in re.finditer(
    r'^\s*itemTable\[(RG_\w+)\]\s*=([^\n]+)', donor_item_table, re.M)
    if 'ITEMTYPE_EQUIP' in match[2]}
required_equipment = {re.search(r'"([^"\n]*)"', line)[1] for line in required_equipment}
required_extended = {name for name, token in catalog['ootItems'].items() if token.startswith('RG_EXT_')}
assert required_equipment | required_extended <= set(catalog['equipmentCoverage'])
native_equipment = {name for token, name in catalog['mmRows'] if re.search(
    r'RI\(' + re.escape(token) + r',[^\n]*\bITEM_(SWORD|SHIELD)_', items_src)}
assert native_equipment <= set(catalog['equipmentCoverage'])
assert collections.Counter(entry['placedItem'] for entry in main['_review']['pickupOrder'][:7]) == {
    'Progressive Kokiri Sword': 3, 'Progressive Master Sword': 2, "Progressive Biggoron's Sword": 2}
assert [(entry['placedItem'], entry['group']) for entry in main['_review']['pickupOrder'][44:]] == [
    (row['itemName'], next(entry['group'] for entry in old['_review']['pickupOrder'] if entry['check'] == row['checkName']))
    for row in old['foreign']]
assert all(entry['itemGame'] == 'oot' for entry in main['_review']['pickupOrder'][:43])
assert main['_review']['pickupOrder'][43]['expectedGI'] == "Hero's Shield"
direct = seeds['02-MM-Direct-Equipment-Control.json']
assert {entry['placedItem'] for entry in direct['_review']['pickupOrder']} == set(catalog['equipmentCoverage'])
assert sum('Tingle maps' == entry['group'] for entry in main['_review']['pickupOrder']) == 6

tier_checks = '''
assert(ResolveDonor(RG_PROGRESSIVE_KOKIRI_SWORD)==RG_KOKIRI_SWORD);
fixture.save.inventory.equipment=1; assert(ResolveDonor(RG_PROGRESSIVE_KOKIRI_SWORD)==RG_RAZOR_SWORD);
hasRazor=true; assert(ResolveDonor(RG_PROGRESSIVE_KOKIRI_SWORD)==RG_GILDED_SWORD);
fixture.save.inventory.equipment=0; assert(ResolveDonor(RG_PROGRESSIVE_MASTER_SWORD)==RG_MASTER_SWORD);
fixture.save.inventory.equipment=2; assert(ResolveDonor(RG_PROGRESSIVE_MASTER_SWORD)==RG_TRUE_MASTER_SWORD);
fixture.save.inventory.equipment=0; assert(ResolveDonor(RG_PROGRESSIVE_BGS)==RG_BIGGORON_SWORD);
fixture.save.inventory.equipment=4; assert(ResolveDonor(RG_PROGRESSIVE_BGS)==RG_GREAT_FAIRY_SWORD);
assert(ResolveDonor(RG_PROGRESSIVE_GORONSWORD)==RG_BIGGORON_SWORD);
assert(ResolveDonor(RG_PROGRESSIVE_HAMMER)==RG_MEGATON_HAMMER);
fixture.hammer=ITEM_HAMMER; assert(ResolveDonor(RG_PROGRESSIVE_HAMMER)==RG_IRON_KNUCKLE_AXE);
assert(ResolveNative(RI_PROGRESSIVE_SWORD)==RI_SWORD_KOKIRI);
nativeSword=1; assert(ResolveNative(RI_PROGRESSIVE_SWORD)==RI_SWORD_RAZOR);
nativeSword=2; assert(ResolveNative(RI_PROGRESSIVE_SWORD)==RI_SWORD_GILDED);
assert(ResolveNative(RI_OOT_PROGRESSIVE_MASTER_SWORD)==RI_OOT_MASTER_SWORD);
nei.comboObtained[FC_OOT_SWORD_MASTER]=1; assert(ResolveNative(RI_OOT_PROGRESSIVE_MASTER_SWORD)==RI_OOT_TRUE_MASTER_SWORD);
assert(ResolveNative(RI_OOT_PROGRESSIVE_BGS)==RI_OOT_BIGGORON_SWORD);
nei.comboObtained[FC_OOT_SWORD_BIGGORON]=1; assert(ResolveNative(RI_OOT_PROGRESSIVE_BGS)==RI_GREAT_FAIRY_SWORD);
assert(ResolveNative(RI_OOT_PROGRESSIVE_HAMMER)==RI_OOT_HAMMER);
nei.ootHammerOwned=true; assert(ResolveNative(RI_OOT_PROGRESSIVE_HAMMER)==RI_OOT_IRON_KNUCKLE_AXE);
'''
compiled += '\nint main(){\n' + '\n'.join(assertions) + tier_checks
compiled += 'std::cout << "PASS: ' + str(len(assertions)) + ' production lookup assertions and 19 production tier-selection assertions\\n";}\n'
probe = ROOT / 'equipment_catalog_probe.cpp'
probe.write_text(compiled)
subprocess.run(['c++', '-std=c++17', '-O1', str(probe), '-o', str(ROOT / 'equipment_catalog_probe')], check=True)
run = subprocess.run([str(ROOT / 'equipment_catalog_probe')], text=True, capture_output=True, check=True)
print(run.stdout.strip())

with (OUT / 'Pickup-Checklist.csv').open(encoding='utf-8-sig') as stream:
    checklist = list(csv.DictReader(stream))
expected = [(name, entry) for name, data in seeds.items() for entry in data['_review']['pickupOrder']]
assert len(checklist) == len(expected) == 150
for row, (name, entry) in zip(checklist, expected, strict=True):
    assert row['seed'] == name and int(row['order']) == entry['order']
    assert row['MM check'] == entry['check'] and row['placed item'] == entry['placedItem']
    assert row['expected GI / tier'] == entry['expectedGI']

report = {'build': BUILD, 'status': 'source and compiled production lookup/tier fixtures passed',
          'mainTestPickups': 90, 'equipmentGIs': 44, 'foreignEquipmentChecks': 43,
          'productionLookupAssertions': len(assertions), 'productionTierAssertions': 19,
          'independentEquipmentAudit': {'donorEquipmentRows': len(required_equipment),
                                       'extendedEquipmentRows': len(required_extended),
                                       'nativeSwordAndShieldRows': len(native_equipment)},
          'previousFailure': 'Previous foreign array omitted all sword and custom tunic test cases; the main seed had no progressive sword copies.',
          'fixtures': 'Production function/case bodies with source-derived catalogs and explicit inventory inputs. No game execution or complete importer/save hydration.',
          'notRun': ['Full native importer', 'Save initialization and cross-game grant hydration',
                     'Generated entrance/boss layout', 'Physical pot/check ordering',
                     'Alt/vanilla model presentation, clipping, shimmer and particles', 'Beatability'],
          'files': records}
(OUT / 'Validation.json').write_text(json.dumps(report, indent=2) + '\n')
print('PASS: 44 equipment GIs, all progressive sword names and copies, 46 retained regressions, exact 150-row checklist.')
