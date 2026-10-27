#!/usr/bin/env python3
"""Run native receipt selection/fallback bodies with selected resource geometry.

The original callback remains a rendering boundary; production receipt flags,
descriptor refusal, fallback scope and selected bounds traversal all execute.
No scene execution or currently mounted pack selection is claimed.
"""
import os
import re
import subprocess
import sys
import tempfile
import json
import xml.etree.ElementTree as ET
from pathlib import Path
from run_mm_scene_randomization_tests import function

ROOT = Path(__file__).resolve().parents[2]
native = (ROOT / 'mm/2s2h/Rando/NeiGiPresentation.cpp').read_text()
oot = (ROOT / 'soh/soh/Enhancements/randomizer/NeiGiPresentation.cpp').read_text()
oot_draw = (ROOT / 'soh/soh/Enhancements/randomizer/draw.cpp').read_text()
renderer = (ROOT / 'soh/soh/Enhancements/randomizer/NeiGiMeshRenderer.inc').read_text()
draw = (ROOT / 'mm/2s2h/Rando/DrawItem.cpp').read_text()
header = (ROOT / 'mm/2s2h/Rando/NeiGiPresentation.h').read_text()
model = (ROOT / 'tests/sword_fallback/model_fit_test.cpp').read_text()
resources = (ROOT / 'combo/NeiAssetPriorityResource.h').read_text()
bridge = (ROOT / 'mm/2s2h/BenPort.cpp').read_text()
companion = (ROOT / 'mm/mods/transformation_masks/assets/mm_asset_loader.cpp').read_text()
common = model[:model.index('using f32=')]
common = common.replace('name+=7;\n        auto it=', 'name+=7;\n        if(name[0]==\'@\'){const char* colon=strchr(name,\':\');if(!colon)return nullptr;name=colon+1;}\n        auto it=')
ids = sorted(set(re.findall(r'\bRI_[A-Z0-9_]+\b', native)))
declarations = 'enum RandoItemId {' + ','.join(ids) + '};\n'
binding_start = native.index('struct Binding {')
binding_end = native.index('// Only roots', binding_start)
declarations += native[binding_start:binding_end]
declarations += header[header.index('class MM_NeiGiFallbackShimmer'):]
custom = (ROOT / 'mm/2s2h/CustomItem/CustomItem.h').read_text()
flags_start = custom.index('enum CustomItemFlags')
flags_end = custom.index('};',flags_start)+2
declarations += '\nnamespace CustomItem {' + custom[flags_start:flags_end] + '}\n'
production = '\n'.join(function(native, name) for name in [
    'HasMmLegacyGiMod', 'GetSelectedOwnerGi', 'MM_DescribeNeiGi', 'MM_TryDrawNeiGi'])
production += '\n' + native[native.index('MM_NeiGiFallbackShimmer::MM_NeiGiFallbackShimmer'):]
start = draw.index('    const bool shop = actor && actor->id == ACTOR_EN_GIRLA;')
end = draw.index('    const int dungeonOwner', start)
# The outer COMBO_BUILD guard starts before this extraction. Remove only its
# closing directive; keep nested diagnostic guards balanced.
dispatch = ''.join(draw[start:end].rsplit('#endif', 1))
production += '\nvoid DrawReceipt(RandoItemId randoItemId, Actor* actor) {\n' + dispatch + '\nDrawNativeSword();\n}\n'
template = (ROOT / 'tests/sword_fallback/receipt_test.cpp').read_text()
source = template.replace('/* RESOURCE_TYPES */', common).replace('/* DECLARATIONS */', declarations)
resource_fit = '\n'.join(function(resources, name) for name in ['GetDinSwordGiProfile', 'GetGiModelsFit'])
if 'struct GiFitResourceLoader' in resources:
    start=resources.index('struct GiFitResourceLoader')
    resource_fit=resources[start:resources.index('inline bool GetGiModelsFit',start)]+resource_fit
source = source.replace('/* RESOURCE_FIT */', 'namespace NeiAssetPriority {\n'+resource_fit+'\n}\n')
source = source.replace('/* RESOURCE_API */', function(bridge,'ResourceMgr_GetGiModelsFitForGame')+'\n'+
                        function(bridge,'ResourceMgr_GetGiModelBoundsForGame'))
companion_fit = ''
if 'class OotGiResourceLoader' in companion:
    begin=companion.index('class OotGiResourceLoader')
    end=companion.index('extern "C" int MmAssets_GetOotGiModelFit',begin)
    companion_fit=companion[begin:end]+function(companion,'MmAssets_GetOotGiModelFit')+'\n'+function(companion,'MmAssets_GetOotGiModelBounds')
source = source.replace('/* COMPANION_FIT */',companion_fit)
source = source.replace('/* PRODUCTION */', production)
oot_production = '\n'.join(function(oot_draw, name) for name in [
    'Randomizer_DrawProgressiveKokiriSword', 'Randomizer_DrawProgressiveMasterSword', 'Randomizer_DrawProgressiveBGS',
    'Randomizer_DrawExtFourSwordPresentation', 'Randomizer_DrawExtFourSword'])
oot_production += '\n'+'\n'.join(function(oot, name) for name in ['Spin', 'HasLegacyGiMod', 'PresentationShimmer', 'NeiGi_DrawEffects', 'NeiGi_DrawImpl'])
# The full TU declares these private effect helpers before NeiGi_DrawImpl.
# Keep their exact production signatures, but fail if a sword enters that lane.
elemental_boundaries = '\n'.join(
    function(renderer, name).split('{', 1)[0] +
    '{ assert(false && "sword receipt must not enter elemental crystal sheen"); ' + result + ' }'
    for name, result in [('NeiGi_DrawMeshMaterial', 'return false;'), ('NeiGi_RestoreElemental', '')])
sages_declaration = re.search(r'^void NeiGi_DrawSagesTunicMedallions\([^;]+;',
    (ROOT / 'soh/soh/Enhancements/randomizer/NeiGiRender.h').read_text(), re.M)[0]
sages_boundary = sages_declaration + '\n' + function(renderer, 'NeiGi_DrawSagesTunicMedallions').split('{', 1)[0] + \
    '{ assert(false && "sword receipt must not enter Sage medallion fountain"); }'
oot_production = elemental_boundaries + '\n' + sages_boundary + '\n' + oot_production
source = source.replace('/* OOT_PRODUCTION */', oot_production)

# The real source XML graph and vertices must succeed through the production
# query, including all child calls; reject unknown control/geometry commands.
def spell_bounds_checks():
    base = 'objects/object_nei_magic_spell/'
    assets = ROOT / 'mm/assets/custom'
    visited, definitions, points = set(), [], {}
    material = {'PipeSync', 'SetRenderMode', 'SetCombineLERP', 'SetPrimColor', 'SetEnvColor',
                'SetTextureLUT', 'Texture', 'LoadTextureBlock', 'ClearGeometryMode', 'SetGeometryMode',
                'TileSync', 'SetTextureImage', 'SetTile', 'LoadSync', 'LoadBlock', 'SetTileSize',
                'Triangle1', 'Triangles2', 'EndDisplayList'}
    def visit(path):
        if path in visited: return
        visited.add(path)
        data = (assets / path).read_bytes()
        assert data == (ROOT / 'soh/assets/custom' / path).read_bytes(), path
        tree = ET.fromstring(data)
        if tree.tag == 'Vertex':
            coords = [tuple(int(v.attrib[axis]) for axis in ('X', 'Y', 'Z')) for v in tree]
            points[path] = coords
            definitions.append('Vertex('+json.dumps(path)+','+str(1000+len(definitions))+',{' +
                               ','.join('{'+','.join(map(str,p))+'}' for p in coords)+'});')
            return
        assert tree.tag == 'DisplayList', path
        commands=[]
        for node in tree:
            if node.tag == 'CallDisplayList':
                child=node.attrib['Path'];visit(child)
                commands.append('{uintptr_t(G_DL_OTR_FILEPATH)<<24,uintptr_t('+json.dumps('__OTR__'+child)+')}')
            elif node.tag == 'LoadVertices':
                child=node.attrib['Path'];visit(child)
                count=int(node.attrib['Count']);offset=int(node.attrib.get('VertexOffset','0'))
                commands += ['{uintptr_t(G_VTX_OTR_FILEPATH)<<24,uintptr_t('+json.dumps('__OTR__'+child)+')}',
                             '{'+str(count)+','+str(offset)+'}']
            else:
                assert node.tag in material, (path, node.tag)
        commands.append('{uintptr_t(G_ENDDL)<<24,0}')
        definitions.append('List('+json.dumps(path)+','+str(1000+len(definitions))+',{' +','.join(commands)+'});')
    roots=[base+name for name in ('gHyliaGraceGiveDL','gZonaiPermafrostGiveDL','gDemiseDestructionGiveDL')]
    for root in roots: visit(root)
    all_points=[p for resource in points.values() for p in resource]
    low=[min(p[a] for p in all_points) for a in range(3)]
    high=[max(p[a] for p in all_points) for a in range(3)]
    for root in roots:
        definitions.append('{ const char* root='+json.dumps('__OTR__'+root)+'; float low[3]{},high[3]{};'
                           'assert(ResourceMgr_GetGiModelBoundsForGame("mm",&root,1,low,high));'+
                           ''.join('assert(low['+str(a)+']=='+str(low[a])+'&&high['+str(a)+']=='+str(high[a])+');' for a in range(3))+'}')
    definitions.append('std::cout<<"PASS actual spell XML bounds: Hylia, Zonai, Demise child lists and exact vertex XYZ\\n";')
    return '\n'.join(definitions)
source = source.replace('/* REAL_SPELL_BOUNDS */', spell_bounds_checks())
flags = ['-std=c++20', '-DF3DEX_GBI_2', '-DCOMBO_BUILD', '-I'+str(ROOT), '-I'+str(ROOT/'combo/menu'),
         '-I'+str(ROOT/'libultraship/include')]
if '--sanitize' in sys.argv:
    flags += ['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie']
with tempfile.TemporaryDirectory(prefix='sword-receipt-') as temporary:
    cpp = Path(temporary)/'receipt.cpp';cpp.write_text(source)
    binary = Path(temporary)/'receipt'
    subprocess.run([os.environ.get('CXX','c++'),*flags,str(cpp),str(ROOT/'soh/soh/resource/type/Array.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
    # Compile the exact typed companion query with real Ship/Fast APIs too.
    # Only the existing global/context and archive-handle access are seams.
    typed = '''#include <unordered_map>
#include <ship/Context.h>
#include <ship/resource/ResourceManager.h>
#include <ship/resource/ResourceLoader.h>
#include <ship/resource/ResourceManagerScope.h>
#include <ship/resource/archive/Archive.h>
#include "combo/NeiGiModelBounds.h"
struct OTRGlobals { static OTRGlobals* Instance; std::shared_ptr<Ship::Context> context; };
std::vector<std::shared_ptr<Ship::Archive>> sOotArchives;
extern "C" unsigned char MmAssets_OotArchivesLoaded();
extern "C" const char* MmAssets_HashToPath(unsigned long long);
extern "C" {
'''+companion_fit+'\n}\n'
    typed_cpp=Path(temporary)/'typed.cpp';typed_cpp.write_text(typed)
    subprocess.run([os.environ.get('CXX','c++'),*flags,'-fsyntax-only',str(typed_cpp)],check=True)
    print('PASS real Ship/Fast typed companion GI bounds API and C linkage')
    # Model bounds alone omit billboard stars that extend past the blade.
    # Project the production procedural meshes through every actual Item0 camera.
    camera_binary=Path(temporary)/'effect-cameras'
    subprocess.run([os.environ.get('CXX','c++'),*flags,'-O2',
                    str(ROOT/'tests/sword_fallback/effect_camera_test.cpp'),'-o',str(camera_binary)],check=True)
    subprocess.run([str(camera_binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
