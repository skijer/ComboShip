"""Execute the named production resolver and MM imported OoT boss-soul dispatch."""
import argparse
import os
import re
import json
from pathlib import Path
import subprocess
import tempfile
from run_mm_scene_randomization_tests import function

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--production-root', type=Path, default=ROOT)
parser.add_argument('--sanitize', action='store_true')
args = parser.parse_args()
host = (args.production_root/'combo/menu/ComboForeignDrawMM.h').read_text()
native = (args.production_root/'mm/2s2h/Rando/DrawItem.cpp').read_text()
fixture = (ROOT/'tests/mm_presentation/mm_oot_boss_import_bridge_test.cpp').read_text()
items = (args.production_root/'soh/soh/Enhancements/randomizer/item_list.cpp').read_text()
ids = ('GOHMA', 'KING_DODONGO', 'BARINADE', 'PHANTOM_GANON', 'VOLVAGIA', 'MORPHA', 'BONGO_BONGO', 'TWINROVA', 'GANON')
names = [re.search(r'itemTable\[RG_'+item+r'_SOUL\] =[^\n]*Text\{\s*"([^"]+)"', items).group(1) for item in ids]
fixture = fixture.replace('/* DONOR_NAMES */', 'constexpr const char* names[] = {'+', '.join(json.dumps(name) for name in names)+'};')
a = host.index('struct ComboForeignDrawInfoOOT {')
b = host.index('\n};', a) + 3
fixture = fixture.replace('/* HOST_INFO */', host[a:b])
fixture=fixture.replace('/* HOST_RESOLVER */','inline bool ResourceMgr_IsAltAssetsEnabled(){return true;}\n#include "combo/menu/ComboSwordGiAssetSelection.h"\n/* HOST_RESOLVER */')
fixture = fixture.replace('/* HOST_RESOLVER */', 'int32_t ComboNativeMmImport(const char*) {return -1;}\n' + function(host, 'ComboFillForeignDrawInfoOOT'))
helper = function(host, 'MM_TryDrawOotBossSoul') if 'MM_TryDrawOotBossSoul(' in host else 'bool MM_TryDrawOotBossSoul(RandoItemId) {return false;}'
fixture = fixture.replace('/* HOST_IMPORT_HELPER */', helper)
fixture = fixture.replace('/* NATIVE_IMPORT_DRAW */', function(native, 'DrawOotBossSoul'))
with tempfile.TemporaryDirectory(prefix='mm-oot-boss-import-') as td:
    path = Path(td)/'test.cpp'
    path.write_text(fixture)
    binary = Path(td)/'test'
    flags = ['-DCOMBO_BUILD', '-std=c++20', '-Wall', '-Wextra', '-Wno-unused-parameter']
    if args.sanitize:
        flags += ['-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
    subprocess.run([os.environ.get('CXX', 'c++'), *flags, '-I'+str(args.production_root), str(path), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
