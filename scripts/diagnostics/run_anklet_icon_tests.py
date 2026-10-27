"""Execute the production MM receipt and both hosts' equipment icon selectors.

Unrelated resource symbols and the catalog service are recorder seams. The
target paths and native slot indices come from the production functions.
"""
from pathlib import Path
import os
import re
import subprocess
import tempfile
from run_time_pedestal_tests import functions

ROOT = Path(__file__).resolve().parents[2]
ANKLET = '__OTR__textures/icon_item_custom/gItemIconPegasusAnkletTex'


def run(source, name, includes=()):
    with tempfile.TemporaryDirectory(prefix='anklet-icon-') as tmp:
        source_file, binary = Path(tmp) / (name + '.cpp'), Path(tmp) / name
        source_file.write_text(source)
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-Wall', '-Wextra', '-Werror',
                        '-I' + str(ROOT), *['-I' + str(x) for x in includes],
                        str(source_file), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True)


mm_source = (ROOT / 'mm/2s2h/Rando/StaticData/Items.cpp').read_text()
mm_functions = functions(mm_source)
route = mm_functions['GetIconTexturePath']
message = mm_functions['GetIconForZMessage']
tokens = re.sub(r'//[^\n]*|/\*.*?\*/|"(?:[^"\\]|\\.)*"', '', route, flags=re.S)
assets = sorted(set(re.findall(r'\bg[A-Z]\w+', tokens)) - {'gItemIcons'})
run(r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include "mm/2s2h/Rando/Types.h"
#include "mm/include/z64item.h"
#include "combo/menu/ComboSongDrawMM.h"
#include "combo/menu/ComboItemIconOwnership.h"
using s16 = int16_t; using u8 = uint8_t;
static const char* gItemIcons[256]{};
static s16 D_801CFF94[250]{};
namespace Rando::StaticData {
struct Item { s16 itemId = ITEM_NONE, getItemId = GI_NONE; };
std::map<RandoItemId, Item> Items;
}
static std::string staged;
static int stagedSize;
void Message_StageCustomItemIcon(void* tex, s16 size) { staged = static_cast<const char*>(tex); stagedSize = size; }
void Message_StageCustomItemIconTint(void*, s16, s16, u8, u8, u8, u8) { assert(false); }
const char* GetIconTexturePath(RandoItemId);
''' + '\n'.join(f'static const char* {asset} = "{asset}";' for asset in assets) + '\n' + route + '\n' + message + r'''
int main() {
    assert(!std::strcmp(GetIconTexturePath(RI_OOT_EXT_PEGASUS_ANKLET), "''' + ANKLET + r'''") &&
           "MM native/imported Pegasus receipts must select the existing anklet texture");
    assert(GetIconForZMessage(RI_OOT_EXT_PEGASUS_ANKLET) == 0xF5);
    assert(staged == "''' + ANKLET + r'''" && stagedSize == 32);
    assert(!std::strcmp(GetIconTexturePath(RI_OOT_EXT_CLIMB_BOOTS),
           "__OTR__textures/icon_item_custom/gItemIconClimbBootsTex"));
    assert(!std::strcmp(GetIconTexturePath(RI_OOT_EXT_ROC_BOOTS),
           "__OTR__textures/icon_item_custom/gItemIconRocBootsTex"));
}
''', 'mm_receipt')

for host in ('soh', 'mm'):
    source = (ROOT / host / 'mods/extended_equipment.c').read_text()
    paths = re.search(r'static\s+const\s+char\s*\*\s*sExtEquipIconPaths\[4\]\[3\] = \{.*?\};', source, re.S)[0]
    getter = functions(source)['ExtEquip_GetIcon']
    # SoH shares these existing asset-name macros through its generated asset
    # header; the focused header is enough for this resource lookup boundary.
    run(r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include "mm/mods/equipment/ext_equip_icon_assets.h"
#include "combo/menu/ComboItemIconOwnership.h"
using s16 = int16_t; using u8 = uint8_t;
''' + paths + '\n' + getter + r'''
int main() {
    assert(!std::strcmp(static_cast<const char*>(ExtEquip_GetIcon(3, 1)), "''' + ANKLET + r'''") &&
           "The Pegasus inventory slot must use the anklet's existing artwork");
    assert(!std::strcmp(static_cast<const char*>(ExtEquip_GetIcon(3, 2)), dgItemIconClimbBootsTex));
    assert(!std::strcmp(static_cast<const char*>(ExtEquip_GetIcon(3, 3)), dgItemIconRocBootsTex));
    assert(!ExtEquip_GetIcon(-1, 1) && !ExtEquip_GetIcon(4, 1) && !ExtEquip_GetIcon(3, 0) && !ExtEquip_GetIcon(3, 4));
}
''', host + '_equipment', [ROOT / host / 'include'])
    native = ROOT / host / 'assets/custom/textures/icon_item_custom'
    assert (native / 'gItemIconPegasusAnkletTex.rgba32.png').read_bytes() != \
           (native / 'gItemIconPegasusBootsTex.rgba32.png').read_bytes(), 'anklet artwork must not be a boots alias'

print('PASS MM receipt staging, both native inventory slots, stock anklet artwork and unrelated boots')
