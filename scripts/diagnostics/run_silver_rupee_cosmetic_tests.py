#!/usr/bin/env python3
"""Exercise native silver-rupee commands and the real owner/MM receipt boundary.

The CVar, allocation and resource services are fixture inputs. Native draw, owner
recipe, editor rainbow sampler, MM resolver/latch and texture handler are copied
verbatim from production. Shared boundary utilities come from the key fixture.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

from run_mm_scene_randomization_tests import function

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--sanitize', action='store_true')
parser.add_argument('--case', choices=('native_palette', 'foreign_palette', 'foreign_receipt',
                                     'foreign_rainbow', 'foreign_texture'))
args = parser.parse_args()
read = lambda path: (ROOT/path).read_text()
owner = read('combo/menu/ComboItemDrawOOT.h')
host = read('combo/menu/ComboForeignDrawMM.h')
draw = read('soh/soh/Enhancements/randomizer/draw.cpp')
editor = read('soh/soh/Enhancements/cosmetics/CosmeticsEditor.cpp')
header = read('soh/soh/Enhancements/cosmetics/CosmeticsEditor.h')
option = re.search(r'COSMETIC_OPTION\("([^"]+)",\s*"Silver Rupee",', editor)[1]
updaters = read('soh/soh/config/ConfigUpdaters.cpp')
for suffix in ('Value', 'Changed', 'Rainbow'):
    assert f'"gCosmetics.{option}.{suffix}"' in updaters

native = read('soh/tests/silver_rupee_cosmetic_test.cpp')
native = native.replace('/* REGISTERED_OPTION */', option)
native = native.replace('/* NATIVE_DRAW */', function(draw, 'Randomizer_DrawSilverRupee'))

# Reuse only the existing CVar/renderer-service utilities, before its test main.
foreign = read('tests/mm_presentation/oot_key_cosmetic_bridge_test.cpp').split('int main() {')[0]
foreign = foreign.replace('DescribeKeys', 'DescribeSilverRupee')
foreign = foreign.replace('/* KEY_PALETTE_TABLES */', '')
foreign = foreign.replace('RG_BOTTOM_OF_THE_WELL_SMALL_KEY', 'RG_SHADOW_SILVER_BLADES')
foreign = foreign.replace('/* HOST_GRAY_DRAW */', '')
foreign = foreign.replace('/* HOST_COLOR_DRAW */', '')
foreign = foreign.replace('#define gDPSetPrimColor(p, ...) ((void)(p))',
                         '#define gDPSetPrimColor(p,a,b,r,g,blue,alpha) primColors.push_back({(p)->stream,{r,g,blue,alpha}})')
foreign = foreign.replace('/* EDITOR_DECLARATIONS */',
                         header[header.index('typedef enum {'):header.index('#ifdef __cplusplus')] +
                         header[header.index('#define COSMETIC_OPTION'):
                                header.index('extern std::map<std::string, CosmeticOption>')])
a = editor.index('std::map<std::string, CosmeticOption> cosmeticOptions = {')
b = editor.index('\n};', a) + 3
foreign = foreign.replace('/* EDITOR_OPTIONS */', editor[a:b])
foreign = foreign.replace('/* EDITOR_TICK */', function(editor, 'CosmeticsUpdateTick'))
a = editor.index('// Resource-only foreign GI cosmetic sampler.')
b = editor.index('// Runs every frame', a)
foreign = foreign.replace('/* EDITOR_SAMPLER */', editor[a:b])
foreign = foreign.replace('/* LIVE_WRAPPER */', '#include "combo/menu/ComboLiveCosmetics.h"')
a = owner.index('    // Silver rupees:')
b = owner.index('    // Nut bag:', a)
recipe = function(read('combo/menu/ComboCollectibleShimmerHost.h'), 'ComboCollectibleShimmer_Color') + '\n'
recipe += '\n'.join(function(owner, n) for n in ('CwLayerPrim', 'CwLayerEnv'))
recipe += '\nint DescribeSilverRupee(RandomizerGet rg,CwItemDrawInfo* out) {\n' + owner[a:b] + 'return 0;\n}'
foreign = foreign.replace('/* OWNER_KEY_RECIPES */', recipe)
a = host.index('struct ComboForeignDrawInfoOOT {')
b = host.index('\n};', a) + 3
foreign = foreign.replace('/* HOST_INFO */', host[a:b])
foreign = foreign.replace('/* HOST_RESOLVER */',
    'inline bool ResourceMgr_IsAltAssetsEnabled(){return true;}\n'
    '#include "combo/menu/ComboSwordGiAssetSelection.h"\n'
    'int32_t ComboNativeMmImport(const char*) {return -1;}\n' + function(host, 'ComboFillForeignDrawInfoOOT'))
a = host.index('struct ComboForeignDrawCacheOOT {')
b = host.index('} // namespace', a)
foreign = foreign.replace('/* HOST_CACHE */', host[a:b])
foreign = ('#include "objects/object_gi_rupy/object_gi_rupy.h"\n'
           '#include "objects/gameplay_keep/gameplay_keep.h"\n'
           'std::vector<std::pair<int,std::vector<int>>> primColors;\n' + foreign)
# Standard container declarations must precede the extra boundary state.
foreign = '#include <vector>\n#include <utility>\n#include "combo/menu/ComboCollectibleShimmer.h"\n' + foreign
foreign += read('tests/mm_presentation/silver_rupee_cosmetic_test.cpp')
foreign = foreign.replace('/* REGISTERED_OPTION */', option)
foreign = foreign.replace('/* SILVER_HOST_DRAW */', function(host, 'MM_DrawForeignSilverRupee'))
foreign = foreign.replace('/* COLOR_HOST_DRAW */', function(host, 'MM_DrawForeignColorLayers'))

cases = [args.case] if args.case else ['native_palette', 'foreign_palette', 'foreign_receipt',
                                     'foreign_rainbow', 'foreign_texture']
with tempfile.TemporaryDirectory(prefix='silver-rupee-cosmetics-') as td:
    build = Path(td)
    flags = ['-std=c++20', '-Wall', '-Wextra', '-Wno-unused-parameter', '-Wno-missing-field-initializers',
             '-DCOMBO_BUILD', '-DF3DEX_GBI_2', '-DLOG_LEVEL_GAME_PRINTS=0',
             '-DCVAR_PREFIX_COSMETIC="gCosmetics"']
    flags += ['-I'+str(ROOT/p) for p in ('', 'soh/include', 'soh/src', 'soh/assets', 'soh',
                                       'libultraship/include', 'combo/menu')]
    if args.sanitize:
        flags += ['-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                  '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
    compiler = os.environ.get('CXX', 'c++')
    binaries = {}
    for name, source in (('native', native), ('foreign', foreign)):
        path = build/(name+'.cpp'); path.write_text(source)
        binary = build/name
        subprocess.run([compiler, *flags, str(path), '-o', str(binary)], cwd=ROOT, check=True)
        binaries[name] = binary
    failures = 0
    for case in cases:
        result = subprocess.run([str(binaries['native' if case.startswith('native') else 'foreign']), case],
                                env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'}, cwd=ROOT)
        print(f'{case}: returncode={result.returncode}', flush=True)
        failures += result.returncode != 0
    raise SystemExit(bool(failures))
