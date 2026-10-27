"""Execute OoT static custom recipes, concrete award resolution and MM host submission."""
import os
import json
import re
from pathlib import Path
import subprocess
import sys
import tempfile
from run_mm_scene_randomization_tests import function

ROOT = Path(__file__).resolve().parents[2]
owner = (ROOT/'combo/menu/ComboItemDrawOOT.h').read_text()
host = (ROOT/'combo/menu/ComboForeignDrawMM.h').read_text()
# Keep the actual static callback switch; unrelated soul/key branches have independent fixtures.
a = owner.index('    switch (rg) {', owner.index('// Bombchu Bag:'))
b = owner.index('\n// Items whose model', a)
stats=owner[owner.index('    // RPG stat models'):owner.index('    // Boss souls: bespoke')]
body = 'static int32_t OOT_DescribeCustomDraw(RandomizerGet rg, CwItemDrawInfo* out) {\n' + stats + owner[a:b]
helpers = function(owner, 'CwSimple') + '\n' + function(owner, 'CwLayerEnv')
if '// Static custom GI bridge recipes.' in owner:
    a = owner.index('// Static custom GI bridge recipes.')
    b = owner.index('extern "C" int32_t OOT_DescribeMmMaskDraw', a)
    helpers += '\n' + owner[a:b]
fixture = (ROOT/'tests/mm_presentation/static_gi_bridge_test.cpp').read_text()
# The production resolver uses a once-only warning and the native song-note asset.
# Logging is outside this draw-submission fixture, as in the mask bridge fixture.
fixture = ('#include <atomic>\n#define SPDLOG_WARN(...) ((void)0)\n'
           '#include "objects/object_gi_melody/object_gi_melody.h"\n' + fixture)
fixture = fixture.replace('/* OWNER_HELPERS */', helpers)
fixture = fixture.replace('/* OWNER_ALT_QUERY */', function((ROOT/'soh/soh/ResourceManagerHelpers.cpp').read_text(), 'OOT_NeiAltAssetsEnabled'))
fixture = fixture.replace('/* OWNER_RESOURCE_QUERY */', function((ROOT/'soh/soh/ResourceManagerHelpers.cpp').read_text(), 'OOT_NeiResourceExists'))
fixture = fixture.replace('/* OWNER_MAGIC_DESCRIPTOR */', function(owner, 'OOT_DescribeMagicJar'))
collectible = (ROOT/'combo/menu/ComboCollectibleShimmerHost.h').read_text()
fixture = fixture.replace('/* OWNER_DESCRIPTOR */', function(collectible, 'ComboCollectibleShimmer_ProfileForDrawId') + '\n' + body + '\n' + function(owner, 'OOT_FillItemDrawInfo') + '\n' +
                          function(owner, 'OOT_IsStateDependentDraw') + '\n' + function(owner, 'OOT_DrawDependency'))
fixture = fixture.replace('/* OWNER_EXPORT */', function(owner, 'OOT_GetItemDrawInfo'))
# Execute the exported English-name boundary using the catalog's actual enum
# ordering. Duplicate names (the two Pendants) must select the same row as boot.
catalog = (ROOT/'soh/soh/Enhancements/randomizer/item_list.cpp').read_text()
entries = dict(re.findall(r'itemTable\[(RG_\w+)\]\s*=\s*Item\(\s*RG_\w+\s*,\s*Text\{\s*"([^"]+)"', catalog))
order = re.findall(r'RANDO_ENUM_ITEM\((RG_\w+)\)', (ROOT/'soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerGet.h').read_text())
fixture = fixture.replace('/* NAME_MAP_INIT */', '\n'.join(
    f'Rando::StaticData::itemNameToEnum[{json.dumps(entries[rg])}]={rg};' for rg in order if rg in entries))
fixture = fixture.replace('/* OWNER_MAGIC_QUERY */', function((ROOT/'soh/soh/ResourceManagerHelpers.cpp').read_text(), 'OOT_MagicJarUsesCustomAsset'))
fixture = fixture.replace('/* HOST_INFO */', host[host.index('struct ComboForeignDrawInfoOOT {'):host.index('\n};', host.index('struct ComboForeignDrawInfoOOT {'))+3])
fixture=fixture.replace('/* HOST_RESOLVER */','inline bool ResourceMgr_IsAltAssetsEnabled(){return true;}\n#include "combo/menu/ComboSwordGiAssetSelection.h"\n/* HOST_RESOLVER */')
fixture = fixture.replace('/* HOST_RESOLVER */', 'int32_t ComboNativeMmImport(const char*) {return -1;}\n' + function(host, 'ComboFillForeignDrawInfoOOT'))
fixture = fixture.replace('/* HOST_NATIVE_DRAW */', function(host, 'MM_DrawForeignNativeEquipment'))
fixture = fixture.replace('/* HOST_AXE_DRAW */', function((ROOT/'mm/2s2h/Rando/DrawItem.cpp').read_text(), 'DrawOotIronKnuckleAxe'))
native = (ROOT/'mm/2s2h/Rando/DrawItem.cpp').read_text()
fixture = fixture.replace('/* HOST_TUNIC_DRAW */', '\n'.join(function(native, name) for name in (
    'LoadNeiLegacyGfx', 'DrawOotMedallion', 'DrawOotGetItemOpaOpaTint', 'DrawOotTunicTint',
    'DrawOotExtSpiritBreastplate', 'DrawOotExtChampionsTunic', 'DrawOotExtSagesTunic')))
if 'inline void MM_DrawForeignCustomGi(' in host:
    fixture = fixture.replace('/* HOST_CUSTOM_DRAW */', '#include "combo/menu/ComboSwordGiEffectFit.h"\n' +
                              function(host, 'MM_DrawForeignCustomGi'))
else:
    fixture = fixture.replace('/* HOST_CUSTOM_DRAW */', 'void MM_DrawForeignCustomGi(const ComboForeignDrawInfoOOT*) {}')
with tempfile.TemporaryDirectory(prefix='mm-static-gi-') as td:
    path=Path(td)/'test.cpp';path.write_text(fixture);binary=Path(td)/'test'
    flags=['-std=c++20','-DCOMBO_BUILD','-Wall','-Wextra','-Wno-unused-parameter']
    if '--sanitize' in sys.argv: flags+=['-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie']
    subprocess.run([os.environ.get('CXX','c++'),*flags,'-I'+str(ROOT),'-I'+str(ROOT/'soh/include'),'-I'+str(ROOT/'soh/assets'),str(path),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
