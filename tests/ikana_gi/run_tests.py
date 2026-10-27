#!/usr/bin/env python3
"""Check real Ikana pose producers, both grids and the engine's texture route."""
from pathlib import Path
import argparse, hashlib, os, re, struct, subprocess, tempfile
import xml.etree.ElementTree as ET
from zipfile import ZipFile
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument("--sanitize",action="store_true")
parser.add_argument("--archive",type=Path,action="append",default=[],help="optional authored NEI GI O2R geometry proof")
parser.add_argument("--fados-archive",type=Path,help="local supplied Fados worn-shield O2R; mesh data is not distributed")
args=parser.parse_args()
ROOT=Path(__file__).resolve().parents[2]
def function(source,name):
    match=re.search(r'^(?![ \t]*/)(?:static\s+)?[^\n{};]+\b'+re.escape(name)+r'\([^;{}]*\)\s*\{',source,re.M)
    if not match:raise RuntimeError('Missing production function '+name)
    cursor,depth=match.end(),1
    while depth:
        depth+=(source[cursor]=='{')-(source[cursor]=='}');cursor+=1
    return source[match.start():cursor]
def read(path):return (ROOT/path).read_text()
source=read('tests/ikana_gi/presentation_test.cpp')
source=source.replace('/* OOT_NATIVE_DRAW */',function(read('soh/soh/Enhancements/randomizer/draw.cpp'),'DrawMmGiModel')+'\n'+function(read('soh/soh/Enhancements/randomizer/draw.cpp'),'Randomizer_DrawExtShieldOfIkana'))
owner=read('combo/menu/ComboItemDrawOOT.h')
case=re.search(r'        case RG_EXT_SHIELD_OF_IKANA:.*?(?=\n        case )',owner,re.S)[0]
source=source.replace('/* OOT_DESCRIPTOR */',function(owner,'CwSimple')+'\nint DescribeIkana(CwItemDrawInfo* out){switch(RG_EXT_SHIELD_OF_IKANA){\n'+case+'\n}return 0;}')
native=function(read('mm/2s2h/Rando/DrawItem.cpp'),'DrawResolvedItem')
case=re.search(r'        case RI_SHIELD_MIRROR:.*?(?=\n        case )',native,re.S)
if case:case=case[0]
else:case=re.search(r'        default:\s*GetItem_Draw\(gPlayState,.*?break;',native,re.S)[0]
source=source.replace('/* MM_NATIVE_DRAW */','void DrawNativeIkanaGI(){const int randoItemId=RI_SHIELD_MIRROR;switch(randoItemId){\n'+case+'\n}}')
foreign_mm=read('combo/menu/ComboForeignDrawMM.h')
source=source.replace('/* MM_SIMPLE_INFO */',re.search(r'struct ComboForeignDrawInfoOOT \{.*?\n\};',foreign_mm,re.S)[0])
source=source.replace('/* MM_SIMPLE_CONSUMER */',function(foreign_mm,'MM_DrawForeignSimple'))
source=source.replace('/* TEXTURE_RESOLVER */',function(read('libultraship/src/fast/interpreter.cpp'),'ComboLoadTextureResource'))
equipment=read('mm/src/overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_equipment.c')
source=source.replace('/* EQUIPMENT_SELECTOR */',
    '// Availability follows this fixture\'s actual renderer/owner store.\n'
    'u8 MM_IsRgba32TextureAvailable(const char* path) { return ComboLoadTextureResource(path) != nullptr; }\n'+
    function(equipment,'KaleidoEquip_OotTex'))
tables=[]
for host,namespace in [('mm','MmEquip'),('soh','OotEquip')]:
    data=read(host+'/mods/extended_equipment.c')
    table=re.search(r'static\s+const\s+char\s*\*\s*sExtEquipIconPaths\[4\]\[3\] = \{.*?\n\};',data,re.S)[0]
    tables.append('namespace '+namespace+'{\n#include "'+host+'/assets/'+('2s2h' if host=='mm' else 'soh')+'_assets.h"\n'+
                  ('#include "mm/mods/equipment/ext_equip_icon_assets.h"\n' if host=='mm' else '')+
                  table+'\n'+function(data,'ExtEquip_GetIcon')+'\n}')
source=source.replace('/* EQUIPMENT_TABLES */','\n'.join(tables))
catalog=read('soh/soh/Enhancements/randomizer/item_list.cpp')
row=next(line for line in catalog.splitlines() if 'itemTable[RG_EXT_SHIELD_OF_IKANA] =' in line)
icon=re.search(r'\.CustomIcon\(([^,)]+)',row)[1]
source=source.replace('/* OOT_CATALOG_ICON */','static const char* OotCatalogIcon = '+icon+';')
flags=['-std=c++20','-DCOMBO_BUILD','-Wall','-Wextra','-Werror','-Wno-unused-parameter',
       *['-I'+str(ROOT/p) for p in ('','mm/include','soh/include','libultraship/include')]]
if args.sanitize:flags+=['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie']
with tempfile.TemporaryDirectory(prefix='ikana-presentation-') as temp:
    cpp=Path(temp)/'test.cpp';cpp.write_text(source);binary=Path(temp)/'test'
    subprocess.run([os.environ.get('CXX','c++'),*flags,str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
    foreign=read('tests/item_receipts/foreign_icon_test.cpp')
    foreign=foreign.replace('/* ICON_OWNER_INCLUDE */','#include "combo/menu/ComboItemIconOwnership.h"')
    foreign=foreign.replace('/* FOREIGN_ICON_SELECTOR */',function(read('mm/2s2h/Rando/DrawItem.cpp'),'ComboForeignMessageIcon'))
    for combo in (False,True):
        cpp.write_text(foreign)
        iconflags=flags if combo else [f for f in flags if f!='-DCOMBO_BUILD']
        subprocess.run([os.environ.get('CXX','c++'),*iconflags,str(cpp),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})

    exported=read('tests/ikana_gi/mm_export_test.cpp')
    mm=read('combo/menu/ComboItemDrawMM.h')
    collectible=read('combo/menu/ComboCollectibleShimmerHost.h')
    profile='#define COMBO_COLLECTIBLE_SHIMMER_HOST_MM\n'+function(collectible,'ComboCollectibleShimmer_ProfileForDrawId')+'\n#undef COMBO_COLLECTIBLE_SHIMMER_HOST_MM\n'
    exported=exported.replace('/* MM_PRODUCER */',profile+'\n'.join(function(mm,name) for name in
        ('MM_Op','MM_OpV','MM_IsProgressiveItem','MM_IsStateDependentDraw','MM_IsSwordAppearanceDependent',
         'MM_FillElementalSpellDrawInfo','MM_FillItemDrawInfo','MM_GetItemDrawInfo')))
    native=read('mm/src/code/z_draw.c')
    row=re.search(r'\{ GetItem_DrawOpa0Xlu1, \{ (gGiMirrorShieldEmptyDL), (gGiMirrorShieldDL) \} \}',native)
    assert row,'actual native MM shield row'
    exported=exported.replace('/* NATIVE_SHIELD_ROW */','dls[0]=(void*)'+row[1]+';dls[1]=(void*)'+row[2]+';')
    oot=read('combo/menu/ComboForeignDrawOOT.h')
    exported=exported.replace('/* CONSUMER_INFO */',re.search(r'struct ComboForeignDrawInfo \{.*?\n\};',oot,re.S)[0])
    exported=exported.replace('/* OOT_CONSUMER */','\n'.join(function(oot,name) for name in
        ('OOT_DrawForeignOps','OOT_DrawForeignSimple')))
    cpp.write_text(exported)
    subprocess.run([os.environ.get('CXX','c++'),*flags,'-Wno-unused-function',str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})

def serialized_geometry(name,read_bytes,paths):
    points=[tuple(int(v.get(axis)) for axis in ('X','Y','Z')) for path in paths
            for v in ET.fromstring(read_bytes(path))]
    assert points,name
    low=[min(p[a] for p in points) for a in range(3)]
    high=[max(p[a] for p in points) for a in range(3)]
    spans=[b-a for a,b in zip(low,high)]
    assert spans[1]>spans[0] and spans[2]<spans[1],(name,spans)
    print(f"Serialized Ikana geometry: {name}; {len(points)} vertices; spans={spans}; X/Y={spans[0]/spans[1]:.6f}",flush=True)
    words=struct.unpack_from('<16I',read_bytes('objects/nei_gi_redesign/shield_of_ikana/scale_mtx'),64)
    return '{"'+name+'", {'+','.join('{'+','.join(map(str,p))+'}' for p in points)+'}, {'+','.join(map(str,words))+'}}'
asset_root=ROOT/'soh/assets/custom'
asset_prefix='objects/nei_gi_redesign/shield_of_ikana/'
geometry=[serialized_geometry('repository',lambda key:(asset_root/key).read_bytes(),
    [str(p.relative_to(asset_root)) for p in sorted((asset_root/asset_prefix).glob('mesh_*_vtx'))])]
for archive in args.archive:
    with ZipFile(archive) as selected:
        geometry.append(serialized_geometry('supplied archive',selected.read,
            sorted(p for p in selected.namelist() if p.startswith(asset_prefix+'mesh_') and p.endswith('_vtx'))))
cache=read('tests/ikana_gi/oot_cache_test.cpp')
oot=read('combo/menu/ComboItemDrawOOT.h')
case=re.search(r'        case RG_EXT_SHIELD_OF_IKANA:.*?(?=\n        case )',oot,re.S)[0]
cache=cache.replace('/* OWNER_DESCRIPTOR */',function(oot,'CwSimple')+
    '\nint OOT_FillItemDrawInfo(RandomizerGet rg,CwItemDrawInfo* out){switch(rg){\n'+case+'\n default:return 0;}}\n'+
    '\n'.join(function(oot,name) for name in ('OOT_IsStateDependentDraw','OOT_DrawDependency')))
cache=cache.replace('/* OWNER_EXPORT */',function(oot,'OOT_GetItemDrawInfo'))
foreign=read('combo/menu/ComboForeignDrawMM.h')
info=foreign[foreign.index('struct ComboForeignDrawInfoOOT {'):foreign.index('\n};',foreign.index('struct ComboForeignDrawInfoOOT {'))+3]
cache=cache.replace('/* FOREIGN_RESOLVER */',info+'\n'+function(foreign,'ComboFillForeignDrawInfoOOT'))
cache=cache.replace('/* FOREIGN_CACHE */',foreign[foreign.index('struct ComboForeignDrawCacheOOT {'):foreign.index('} // namespace')])
with tempfile.TemporaryDirectory(prefix='ikana-oot-cache-') as temp:
    cpp=Path(temp)/'test.cpp';cpp.write_text(cache);binary=Path(temp)/'test'
    subprocess.run([os.environ.get('CXX','c++'),*flags,str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})

pose=read('tests/ikana_gi/model_pose_test.cpp')
pose=pose.replace('/* ACTUAL_GEOMETRY */','const std::vector<ActualGeometry> actualGeometry={'+','.join(geometry)+'};')
def local_fados_geometry(archive):
    # Keep the user's mesh local. Reproduce vertex load order from the supplied
    # XML graph rather than distributing a baked vertex stream in the repo.
    assert hashlib.sha256(archive.read_bytes()).hexdigest()==\
        'a5239d0186b22e53d6a5797c8a406fe4903f830a97c6004c191b432976a43051', 'Unexpected Fados archive'
    points,batches=[],[]
    with ZipFile(archive) as selected:
        names=set(selected.namelist())
        def resource(path):
            key='alt/'+path if 'alt/'+path in names else path
            return key,ET.fromstring(selected.read(key))
        def walk(path,parents):
            key,root=resource(path)
            assert root.tag=='DisplayList' and key not in parents and len(parents)<32
            for command in root:
                if command.tag=='CallDisplayList':
                    walk(command.attrib['Path'],parents|{key})
                elif command.tag=='LoadVertices':
                    _,vertices=resource(command.attrib['Path'])
                    assert vertices.tag=='Vertex'
                    offset,count=int(command.attrib['VertexOffset']),int(command.attrib['Count'])
                    loaded=list(vertices)[offset:offset+count]
                    assert 0<=offset and 0<count<=32 and len(loaded)==count
                    batches.append((len(points),count))
                    points.extend(tuple(int(v.attrib[a]) for a in ('X','Y','Z')) for v in loaded)
                elif 'Matrix' in command.tag:
                    raise AssertionError('The supplied Fados graph must not contain model matrices')
                elif command.tag=='EndDisplayList':
                    break
        walk('objects/object_link_child/gLinkHumanMirrorShieldDL',set())
    raw=b''.join(struct.pack('<hhh',*point) for point in points)
    assert len(points)==2290 and len(batches)==76
    assert hashlib.sha256(raw).hexdigest()==\
        'ffd72b3eb8f13e9c70938e99dfa0d3130e11286e3b26ee221eaef3913b24f328', 'Fados traversal mismatch'
    print('Local Fados archive: exact 2290 vertices / 76 load batches verified',flush=True)
    return points,batches
fados_points,fados_loads=local_fados_geometry(args.fados_archive) if args.fados_archive else ([],[])
if not args.fados_archive:
    print('SKIP private Fados mesh regression; supply --fados-archive to run it locally',flush=True)
pose=pose.replace('/* FADOS_GEOMETRY */',
    'const std::vector<std::array<int16_t,3>> fadosPoints={'+
    ','.join('{'+','.join(map(str,p))+'}' for p in fados_points)+'};\n'+
    'const std::vector<std::array<size_t,2>> fadosLoads={'+
    ','.join('{'+','.join(map(str,p))+'}' for p in fados_loads)+'};')
scope=read('libultraship/include/ship/resource/ResourceManagerScope.h')
pose=pose.replace('/* RESOURCE_SCOPE */',scope[scope.index('namespace Ship {'):])
pose=pose.replace('/* POSE_QUERY */','\n'.join(function(read('combo/NeiAssetPriorityResource.h'),name) for name in
    ('GetIkanaShieldGiPose','GetIkanaShieldGiTiltX')))
for host in ('soh','mm'):
    path='soh/soh/ResourceManagerHelpers.cpp' if host=='soh' else 'mm/2s2h/BenPort.cpp'
    candidate=pose.replace('/* OWNER_QUERY */','\n'.join(function(read(path),name) for name in
        ('ResourceMgr_GetIkanaShieldGiPoseForGame','ResourceMgr_GetIkanaShieldGiTiltXForGame')))
    sources=['libultraship/src/ship/resource/Resource.cpp',
             *['libultraship/src/fast/resource/type/'+name+'.cpp' for name in ('DisplayList','Vertex','Matrix','Texture')],
             ('soh/soh/resource/type/Array.cpp' if host=='soh' else 'mm/2s2h/resource/type/Array.cpp')]
    for combo in (False,True):
        with tempfile.TemporaryDirectory(prefix='ikana-owner-pose-') as temp:
            cpp=Path(temp)/'test.cpp';cpp.write_text(candidate);binary=Path(temp)/'test'
            nativeflags=[f for f in flags if f!='-DCOMBO_BUILD']
            nativeflags+=['-DF3DEX_GBI_2','-DFMT_HEADER_ONLY','-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_OFF']
            if combo:nativeflags+=['-DCOMBO_BUILD']
            if host=='mm':nativeflags+=['-DMM_BUILD_DLL','-DTEST_HOST_MM']
            subprocess.run([os.environ.get('CXX','c++'),*nativeflags,str(cpp),*[str(ROOT/p) for p in sources],'-o',str(binary)],check=True)
            subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
