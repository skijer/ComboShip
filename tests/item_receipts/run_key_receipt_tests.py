#!/usr/bin/env python3
"""Run production native/foreign key receipts and the queued grant boundary.

--baseline REV links the old receipt builders as a behavioral negative control.
The catalog, game headers, FC mapping, font widths, queue and native key grants
are production data/code. Only loaded engine services and dormant save delivery
are replaced, as the real games occupy different DLLs.
"""
from pathlib import Path
import argparse
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--baseline')
parser.add_argument('--sanitizers', action='store_true')
parser.add_argument('--imports-only', action='store_true', help='isolate the imported dungeon-key negative control')
args = parser.parse_args()


def block(source, signature):
    begin = source.index('{', source.index(signature))
    depth, end = 1, begin + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[begin:end]


def receipt_source(path):
    if args.baseline:
        return subprocess.check_output(['git', 'show', args.baseline + ':' + path], cwd=ROOT, text=True)
    return (ROOT / path).read_text()


compiler = os.environ.get('CXX', 'c++')
sanitize = ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer'] if args.sanitizers else []
enum_gets = re.findall(r'RANDO_ENUM_ITEM\((RG_\w+)\)',
    (ROOT / 'soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerGet.h').read_text())
donor_catalog = (ROOT / 'soh/soh/Enhancements/randomizer/item_list.cpp').read_text()
key_rows = []
for first, last, boss in [('RG_FOREST_TEMPLE_SMALL_KEY', 'RG_TREASURE_GAME_SMALL_KEY', False),
                         ('RG_FOREST_TEMPLE_BOSS_KEY', 'RG_GANONS_CASTLE_BOSS_KEY', True)]:
    for rg in enum_gets[enum_gets.index(first):enum_gets.index(last) + 1]:
        row = re.search(r'itemTable\[' + rg + r'\]\s*=\s*Item\([^\n]+', donor_catalog)[0]
        name = re.search(r'Text\{\s*("(?:[^"\\]|\\.)*")', row)[1]
        article = re.search(r'\{\s*("(?:[^"\\]|\\.)*")\s*,\s*"(?:[^"\\]|\\.)*"\s*,\s*"(?:[^"\\]|\\.)*"\s*\}', row[row.index('MOD_'):])[1]
        color = re.search(r'"(%[wrgbcpyB])"', row[row.index('MOD_'):])
        key_rows.append('{' + f'{rg}, {name}, {article}, "{color[1] if color else "%g"}", {str(boss).lower()}' + '}')
donor = (ROOT / 'tests/item_receipts/key_donor_test.cpp').read_text()
donor = donor.replace('/* KEY_CATALOG */', 'const KeyRow keyRows[] = {\n' + ',\n'.join(key_rows) + '\n};')
export = receipt_source('soh/soh/Enhancements/randomizer/Messages/ItemMessages.cpp')
vanilla_builder = re.search(r'^static bool BuildVanillaEquipmentBottleReceipt\([^;]*\) \{', export, re.M)
if vanilla_builder:
    donor = donor.replace('/* DONOR_EXPORT */', vanilla_builder[0][:-1] +
        block(export, vanilla_builder[0]) + '\n/* DONOR_EXPORT */')
key_builder = re.search(r'^bool BuildDungeonKeyReceiptMessage\([^;]*\) \{', export, re.M)
if key_builder:
    donor = donor.replace('/* DONOR_EXPORT */', key_builder[0][:-1] +
        block(export, key_builder[0]) + '\n/* DONOR_EXPORT */')
donor = donor.replace('/* DONOR_EXPORT */',
    'extern "C" COMBO_EXPORT int32_t OOT_GetItemReceiptText(const char* itemName, char* buffer, uint32_t capacity) ' +
    block(export, 'int32_t OOT_GetItemReceiptText'))
items = (ROOT / 'mm/2s2h/Rando/StaticData/Items.cpp').read_text()
catalog = items[items.index('#define RI('):items.index('// clang-format off')]
catalog += '\nnamespace Rando::StaticData { std::map<RandoItemId, RandoStaticItem> Items = ' + \
    block(items, 'std::map<RandoItemId, RandoStaticItem> Items') + ';\n}\n#undef RI\n'
glue = (ROOT / 'mm/2s2h/FleetShipCombo/FleetComboItemsGlue.cpp').read_text()
catalog += 'namespace { const int sFcNative[] = ' + block(glue, 'const int sFcNative[]') + ';\n'
catalog += 'std::unordered_map<int,int>& ReverseMap() ' + block(glue, 'ReverseMap()') + '\n}\n'
catalog += 'extern "C" int FcCombo_ItemForNative(int nativeId) ' + block(glue, 'int FcCombo_ItemForNative') + '\n'
font = (ROOT / 'mm/src/code/z_message_nes.c').read_text()
catalog += 'extern "C" { float sNESFontWidths[160] = ' + block(font, 'f32 sNESFontWidths[160]') + '; }\n'
receiver = (ROOT / 'tests/item_receipts/key_receipt_test.cpp').read_text()
queue = (ROOT / 'mm/2s2h/Rando/MiscBehavior/CheckQueue.cpp').read_text()
receiver = receiver.replace('/* QUEUE_RECEIVE */', 'void Receive(Actor* actor, PlayState* play) ' + block(queue, '.giveItem ='))
grant = (ROOT / 'mm/2s2h/Rando/GiveItem.cpp').read_text()
start = grant.index('        case RI_WOODFALL_BOSS_KEY:')
end = grant.index('        // Grants the max small keys', start)
receiver = receiver.replace('/* NATIVE_KEY_GRANTS */', grant[start:end])
with tempfile.TemporaryDirectory(prefix='key-receipts-') as directory:
    build = Path(directory)
    (build / 'donor.cpp').write_text(donor)
    (build / 'key_catalog.inc').write_text(catalog)
    (build / 'receiver.cpp').write_text(receiver)
    source = receipt_source('mm/2s2h/Rando/ItemReceiptText.cpp')
    (build / 'ItemReceiptText.cpp').write_text(source)
    donor_library = build / 'libkey_donor.so'
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter',
        '-fPIC', '-shared', '-fvisibility=hidden', *sanitize, '-I' + str(ROOT), '-I' + str(ROOT / 'combo/menu'),
        str(build / 'donor.cpp'), '-o', str(donor_library)], cwd=ROOT, check=True)
    includes = ['-I' + str(ROOT / p) for p in ('mm/include', 'mm/include/PR', 'mm/src', 'mm', 'mm/2s2h',
        'mm/2s2h/Rando', 'mm/assets', 'libultraship/include', 'libultraship/src', 'combo', 'combo/menu')]
    flags = ['-DCOMBO_BUILD', '-DMM_BUILD_DLL', '-DF3DEX_GBI_2', '-DCONTROLLERBUTTONS_T=uint32_t',
        '-DNON_EQUIVALENT', '-DNON_MATCHING', *includes, '-I' + str(build)]
    binary = build / 'keys'
    result = subprocess.run([compiler, '-std=c++20', *sanitize, *flags, str(build / 'receiver.cpp'),
        str(build / 'ItemReceiptText.cpp'), '-L' + str(build), '-lkey_donor', '-Wl,-rpath,' + str(build),
        '-rdynamic', '-ldl', '-o', str(binary)], capture_output=True, text=True, cwd=ROOT)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    subprocess.run([str(binary), *(['--imports-only'] if args.imports_only else [])], cwd=ROOT, check=True)
