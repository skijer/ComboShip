"""Execute donor key recipes, native editor rainbow formula and MM draw/cache bodies."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
from run_mm_scene_randomization_tests import function

ROOT = Path(__file__).resolve().parents[2]
owner = (ROOT/'combo/menu/ComboItemDrawOOT.h').read_text()
host = (ROOT/'combo/menu/ComboForeignDrawMM.h').read_text()
draw = (ROOT/'soh/soh/Enhancements/randomizer/draw.cpp').read_text()
editor = (ROOT/'soh/soh/Enhancements/cosmetics/CosmeticsEditor.cpp').read_text()
header = (ROOT/'soh/soh/Enhancements/cosmetics/CosmeticsEditor.h').read_text()
fixture = (ROOT/'tests/mm_presentation/oot_key_cosmetic_bridge_test.cpp').read_text()
a=header.index('typedef enum {');b=header.index('#ifdef __cplusplus',a)
a2=header.index('#define COSMETIC_OPTION');b2=header.index('extern std::map<std::string, CosmeticOption>',a2)
fixture=fixture.replace('/* EDITOR_DECLARATIONS */',header[a:b]+header[a2:b2])
a=editor.index('std::map<std::string, CosmeticOption> cosmeticOptions = {');b=editor.index('\n};',a)+3
fixture=fixture.replace('/* EDITOR_OPTIONS */',editor[a:b])
fixture=fixture.replace('/* EDITOR_TICK */',function(editor,'CosmeticsUpdateTick'))
if 'OOT_SetGiCosmeticFrame(' in editor:
 a=editor.index('// Resource-only foreign GI cosmetic sampler.');b=editor.index('// Runs every frame',a)
 fixture=fixture.replace('/* EDITOR_SAMPLER */',editor[a:b])
else:
 fixture=fixture.replace('/* EDITOR_SAMPLER */','void OOT_SetGiCosmeticFrame(uint32_t) {}')
helpers='\n'.join(function(owner,n) for n in ('CwSimple','CwLayerEnv','CwLayerPrim'))
a=owner.index('    bool customKeys =');b=owner.index('    // Overworld keys:',a)
fixture=fixture.replace('/* OWNER_KEY_RECIPES */',helpers+'\nint DescribeKeys(RandomizerGet rg,CwItemDrawInfo* out) {\n'+owner[a:b]+' return 0;\n}')
a=draw.index('const char* SmallBodyCvarValue');b=draw.index('Color_RGB8 MapOrCompassColor',a)
fixture=fixture.replace('/* KEY_PALETTE_TABLES */',draw[a:b])
a=host.index('struct ComboForeignDrawInfoOOT {');b=host.index('\n};',a)+3
fixture=fixture.replace('/* HOST_INFO */',host[a:b])
fixture=fixture.replace('/* HOST_RESOLVER */','inline bool ResourceMgr_IsAltAssetsEnabled(){return true;}\n#include "combo/menu/ComboSwordGiAssetSelection.h"\n/* HOST_RESOLVER */')
fixture=fixture.replace('/* HOST_RESOLVER */','int32_t ComboNativeMmImport(const char*) {return -1;}\n'+function(host,'ComboFillForeignDrawInfoOOT'))
a=host.index('struct ComboForeignDrawCacheOOT {');b=host.index('} // namespace',a)
fixture=fixture.replace('/* HOST_CACHE */',host[a:b])
fixture=fixture.replace('/* HOST_GRAY_DRAW */',function(host,'MM_DrawForeignGrayscaleLayers') if 'MM_DrawForeignGrayscaleLayers(' in host else 'void MM_DrawForeignGrayscaleLayers(const ComboForeignDrawInfoOOT*) {}')
fixture=fixture.replace('/* HOST_COLOR_DRAW */',function(host,'MM_DrawForeignColorLayers'))
if (ROOT/'combo/menu/ComboLiveCosmetics.h').exists():
 fixture=fixture.replace('/* LIVE_WRAPPER */','#include "combo/menu/ComboLiveCosmetics.h"')
else:
 fixture=fixture.replace('/* LIVE_WRAPPER */','Color_RGB8 CwLiveCosmeticColor(const char* path,Color_RGB8 fallback) {return CVarGetColor24(path,fallback);}')
with tempfile.TemporaryDirectory(prefix='oot-key-cosmetic-') as td:
 path=Path(td)/'test.cpp';path.write_text(fixture);binary=Path(td)/'test'
 flags=['-DCOMBO_BUILD','-std=c++20','-Wall','-Wextra','-Wno-unused-parameter','-Wno-missing-field-initializers']
 if '--sanitize' in sys.argv: flags+=['-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie']
 includes=['-I'+str(ROOT/p) for p in ('','soh/include','soh/assets')]
 subprocess.run([os.environ.get('CXX','c++'),*flags,*includes,str(path),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
