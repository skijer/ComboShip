#!/usr/bin/env python3
"""Exercise authored production geometry against supplied real spot06 collision."""
import argparse, os, re, struct, subprocess, tempfile, zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--scene-dir','--evidence',dest='scene_dir',type=Path,help='Opt-in extracted scene tree for the full1050-poly evidence check')
p.add_argument('--sanitize',action='store_true')
p.add_argument('--provider-ref',help='Read-only draw/provider regression control at this git ref')
p.add_argument('--native-only',action='store_true',help='With --scene-dir, run only full native collision; do not infer Alt coverage')
p.add_argument('--texture-archive',type=Path,help='Read-only donor archive to verify the native stone resource format/tile byte extent')
p.add_argument('--native-case',choices=['all','camera','shell'],default='all',help='Isolate native review regressions; default runs both')
a=p.parse_args()
if a.native_only and not a.scene_dir:
    p.error('--native-only requires --scene-dir')
provider_path='soh/soh/Enhancements/Gameplay/ComboLakeHyliaBridge.cpp'
provider=(subprocess.check_output(['git','show',a.provider_ref+':'+provider_path],cwd=ROOT,text=True)
          if a.provider_ref else (ROOT/provider_path).read_text())
loads=re.findall(r'gDPLoadTextureBlock\(POLY_OPA_DISP\+\+,\s*([A-Za-z0-9_]+),\s*'
                 r'(G_IM_FMT_\w+),\s*(G_IM_SIZ_\w+),\s*(\d+),\s*(\d+),\s*0,\s*'
                 r'G_TX_WRAP,\s*G_TX_WRAP,\s*(\d+),\s*(\d+)',provider)
assert loads == [('stone','G_IM_FMT_RGBA','G_IM_SIZ_16b','32','64','5','6'),
                 ('stone','G_IM_FMT_RGBA','G_IM_SIZ_16b','32','32','5','5'),
                 ('sand','G_IM_FMT_RGBA','G_IM_SIZ_16b','32','32','5','5'),
                 ('sand','G_IM_FMT_RGBA','G_IM_SIZ_16b','16','16','4','4')], \
       f'Stone/sand must preserve separate native and replacement tile dimensions, got {loads}'
if a.texture_archive:
    with zipfile.ZipFile(a.texture_archive) as archive:
        for suffix,width,height in (('010EB8',32,32),('00C2B8',16,16)):
            path='scenes/shared/spot06_scene/spot06_sceneTex_'+suffix
            data=archive.read(path)
            typ,w,h=struct.unpack_from('<III',data,64)
            version=struct.unpack_from('<I',data,8)[0]
            assert typ==2, 'stone/sand fallbacks must retain native RGBA16 interpretation'
            offset=92 if version==1 else 80
            raw=version==1 and struct.unpack_from('<I',data,76)[0]&1
            if raw:
                hs,vs=struct.unpack_from('<ff',data,80)
                assert width*2*hs==w*4 and height*vs==h, 'HD metadata must preserve the selected logical tile'
                assert len(data)-offset==w*h*4
            else:
                assert (w,h)==(width,height)
                assert len(data)-offset==width*height*2, 'native tile payload must fit the exact resource'
    print('PASS current clean-stone 010EB8 32x32 and sand 00C2B8 16x16 resource identities/metadata/payloads')

def collision(path):
    data=path.read_bytes(); offset=64
    def read(fmt):
        nonlocal offset
        value=struct.unpack_from('<'+fmt,data,offset);offset+=struct.calcsize('<'+fmt)
        return value[0] if len(value)==1 else value
    read('6h'); verts=[read('3h') for _ in range(read('I'))]
    polys=[read('4H4h') for _ in range(read('I'))]
    return [tuple(verts[i&8191] for i in poly[1:4]) for poly in polys]
# Measured spot06 collision triangles intersecting x[-2370,-1700],
# z[3500,4100],y[-1460,-800]. Coordinate-only fixture includes the bank,
# lab approach and basin beneath the cave/shore apron; no archive needed.
MEASURED_SCENE_SHA256='e1e10b07bc84b2b43539a5fcc4721181e491b7baf2175cbce13acac8f44021d4'
MEASURED_TRIANGLES = [
    ((-2245, -1313, 3637), (-2185, -1354, 3797), (-1537, -1313, 3530)),
    ((-1149, -1465, 4130), (-1537, -1313, 3530), (-2185, -1354, 3797)),
    ((-1149, -1465, 4130), (-2185, -1354, 3797), (-2265, -1393, 4117)),
    ((-3197, -1396, 4094), (-2943, -1513, 4641), (-2265, -1393, 4117)),
    ((-3125, -1313, 3997), (-3197, -1396, 4094), (-2265, -1393, 4117)),
    ((-3125, -1313, 3997), (-2265, -1393, 4117), (-2325, -1313, 3997)),
    ((-2245, -1313, 3637), (-2245, -1313, 3797), (-2185, -1354, 3797)),
    ((-2185, -1354, 3797), (-2245, -1313, 3797), (-2325, -1313, 3997)),
    ((-2185, -1354, 3797), (-2325, -1313, 3997), (-2265, -1393, 4117)),
    ((-2472, -1033, 3065), (-2245, -1033, 3637), (-1743, -1113, 3161)),
    ((-2245, -1313, 3637), (-1546, -1295, 3492), (-1743, -1113, 3161)),
    ((-2245, -1033, 3637), (-2472, -1033, 3065), (-2445, -1033, 3717)),
    ((-2445, -1033, 3717), (-2245, -1033, 3797), (-2245, -1033, 3637)),
    ((-2445, -1033, 3717), (-2445, -1033, 3997), (-2325, -1033, 3997)),
    ((-2325, -1033, 3997), (-2245, -1033, 3797), (-2445, -1033, 3717)),
    ((-2325, -993, 3997), (-2325, -993, 3977), (-2445, -993, 3977)),
    ((-2325, -993, 3997), (-2445, -993, 3977), (-2445, -993, 3997)),
    ((-2325, -993, 3997), (-2325, -1033, 3997), (-2325, -1033, 3977)),
    ((-2325, -993, 3997), (-2325, -1033, 3977), (-2325, -993, 3977)),
    ((-2445, -993, 3997), (-2445, -1033, 3997), (-2325, -1033, 3997)),
    ((-2445, -993, 3997), (-2325, -1033, 3997), (-2325, -993, 3997)),
    ((-2325, -993, 3977), (-2325, -1033, 3977), (-2445, -1033, 3977)),
    ((-2325, -993, 3977), (-2445, -1033, 3977), (-2445, -993, 3977)),
    ((-2245, -1033, 3637), (-2245, -1313, 3637), (-1743, -1113, 3161)),
    ((-2245, -1313, 3637), (-2245, -1033, 3637), (-2245, -1033, 3797)),
    ((-2245, -1313, 3637), (-2245, -1033, 3797), (-2245, -1313, 3797)),
    ((-2245, -1313, 3797), (-2245, -1033, 3797), (-2325, -1033, 3997)),
    ((-2245, -1313, 3797), (-2325, -1033, 3997), (-2325, -1313, 3997)),
    ((-1537, -1313, 3530), (-1546, -1295, 3492), (-2245, -1313, 3637)),
    ((-2325, -1313, 3997), (-2325, -1033, 3997), (-2445, -1033, 3997)),
    ((-3125, -1313, 3997), (-2325, -1313, 3997), (-2445, -1033, 3997)),
    ((-2325, -993, 3987), (-2445, -993, 3987), (-2445, -1033, 3987)),
    ((-2325, -993, 3987), (-2445, -1033, 3987), (-2325, -1033, 3987)),
]

with tempfile.TemporaryDirectory(prefix='lake-bridge-') as temp:
    temp=Path(temp)
    label=('full1050-poly native scene collision; no fullAlt evidence' if a.native_only else
           'full1050-poly native + Alt-inherited scene collision' if a.scene_dir else
           'portable33-triangle native/Alt-inherited collision control')
    header=['#include <vector>','using ComboLakeHylia::Triangle;',
            'static constexpr const char* kSceneEvidenceLabel="'+label+'";']
    variants=['Native'] if a.native_only else ['Native','Alt']
    rel='scenes/shared/spot06_scene/spot06_sceneCollisionHeader_0055AC'
    for name in variants:
        if a.scene_dir:
            base=a.scene_dir if name=='Native' else a.scene_dir/'alt'
            path=base/rel
            if not path.exists():
                assert name=='Alt' and (base/'scenes/shared/spot06_scene/spot06_room_0').exists()
                path=a.scene_dir/rel # Alt room headers inherit native scene collision.
            triangles=collision(path)
            assert len(triangles)==1050
        else:
            triangles=MEASURED_TRIANGLES
        header.append('static const std::vector<Triangle> k'+name+'Scene = {')
        for t in triangles:header.append('{'+','.join('{'+','.join(map(str,v))+'}' for v in t)+'},')
        header.append('};')
    header.append('static const std::vector<const std::vector<Triangle>*> kSceneVariants = {'+
                  ','.join('&k'+name+'Scene' for name in variants)+'};')
    (temp/'lake_scene_evidence.h').write_text('\n'.join(header))
    binary=temp/'test'
    command=[os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I'+str(ROOT/'combo'),'-I'+str(temp)]
    if a.sanitize:command+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-no-pie']
    subprocess.run(command+[str(ROOT/'tests/regional_passages/lake_bridge_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})

def function(source,signature):
    start=0
    while True:
        start=source.index(signature,start)
        begin=source.index('{',start)
        if ';' not in source[start:begin]:break
        start+=len(signature)
    depth=1;end=begin+1
    while depth:
        if source[end]=='{':depth+=1
        elif source[end]=='}':depth-=1
        end+=1
    return source[start:end]
paths=['combo','combo/menu','soh','soh/soh','soh/include','soh/include/PR','soh/src','soh/assets','libultraship/include']

# Exercise the actual draw callback as well: geometry/physics-only execution
# cannot detect malformed GBI, dead vertex pointers or a bad native setup list.
graph=(ROOT/'soh/src/code/graph.c').read_text()
arena=(ROOT/'soh/src/code/TwoHeadArena.c').read_text()
rcp=(ROOT/'soh/src/code/z_rcp.c').read_text()
wrappers=(ROOT/'soh/soh/GbiWrap.cpp').read_text()
renderer=(ROOT/'soh/soh/Enhancements/randomizer/NeiGiMeshRenderer.inc').read_text()
interpreter=(ROOT/'libultraship/src/fast/interpreter.cpp').read_text()
draw='\n'.join(function(provider,signature) for signature in (
    'const Lake::Geometry& Geometry(', 'const Lake::SurfaceGeometry& SurfaceGeometry(', 'bool Available(', 'Vtx Vertex(',
    'const std::vector<Vtx>& RockVertices(', 'void CaveDraw('))
native_graph='\n'.join(function(arena,signature) for signature in (
    'void* THA_AllocEnd(', 'Gfx* THGA_AllocEnd('))
native_graph+='\n'+'\n'.join(function(graph,signature) for signature in (
    'void* Graph_Alloc(', 'void Graph_OpenDisps(', 'void Graph_CloseDisps('))
native_graph+='\n'+rcp[rcp.index('Gfx sSetupDL['):rcp.index('\n};',rcp.index('Gfx sSetupDL['))+3]
native_graph+='\n'+'\n'.join(function(rcp,signature) for signature in (
    'void Gfx_SetupDL_25Opa(', 'void Gfx_SetupDL_25Xlu('))
native_gbi='\n'.join(function(wrappers,signature) for signature in (
    'extern "C" void gSPDisplayList(', 'extern "C" void gSPVertex('))
first=renderer.index('struct NeiGi_ArenaBudget {')
native_arena=renderer[first:renderer.index('// Child meshes',first)]
native_arena+='\nextern "C" '+function(renderer,'bool NeiGi_CanDrawLayers(')
native_interpreter='\n'.join(function(interpreter,signature) for signature in (
    'void* Interpreter::SegAddr(', 'bool gfx_vtx_handler_f3dex2(',
    'void Interpreter::GenerateCC(', 'void Interpreter::GfxDpSetCombineMode(',
    'void Interpreter::GfxDpSetOtherMode(', 'void Interpreter::GfxSpSetOtherMode(',
    'bool gfx_set_combine_handler_rdp(', 'bool gfx_rdp_set_other_mode_rdp(',
    'bool gfx_othermode_l_handler_f3dex2(', 'void Interpreter::GfxDpSetPrimColor(',
    'bool gfx_set_prim_color_handler_rdp('))
# An obsolete implementation with these names survives inside a block comment.
# Select the live helpers following the production combine-mode setter.
combine_source=interpreter[interpreter.index('void Interpreter::GfxDpSetCombineMode('):]
native_interpreter='\n'.join(function(combine_source,signature) for signature in (
    'static inline uint32_t color_comb(', 'static inline uint32_t alpha_comb('))+'\n'+native_interpreter
interpreter_header=(ROOT/'libultraship/include/fast/interpreter.h').read_text()
native_combiner='\n'.join(function(interpreter_header,signature)+';' for signature in (
    'enum {\n    SHADER_0', 'enum class ShaderOpts', 'struct ColorCombinerKey', 'struct ColorCombiner {', 'struct RGBA {'))
native_combiner+='\n#define SHADER_OPT(opt) ((uint64_t)(1 << static_cast<int>(ShaderOpts::opt)))\n'
source=(ROOT/'tests/regional_passages/lake_render_test.cpp').read_text()
packed_start=interpreter.index('for (int j = 0; j < numInputs; j++)')
packed_end=interpreter.index('{',packed_start)+1
depth=1
while depth:
    depth+=(interpreter[packed_end]=='{')-(interpreter[packed_end]=='}')
    packed_end+=1
for marker,value in (('NATIVE_GRAPH',native_graph),('NATIVE_GBI',native_gbi),
                     ('NATIVE_ARENA',native_arena),('PRODUCTION_DRAW',draw),
                     ('NATIVE_INTERPRETER',native_interpreter),
                     ('NATIVE_COMBINER_DEFINITIONS',native_combiner),
                     ('NATIVE_PACKED_INPUTS', interpreter[packed_start:packed_end])):
    source=source.replace('/* '+marker+' */',value)
with tempfile.TemporaryDirectory(prefix='lake-render-') as temporary:
    temporary=Path(temporary);cpp=temporary/'test.cpp';cpp.write_text(source);binary=temporary/'test'
    command=[os.environ.get('CXX','c++'),'-std=c++20','-fpermissive','-w','-DF3DEX_GBI_2','-DCOMBO_BUILD',
             '-DFMT_HEADER_ONLY','-ffunction-sections','-fdata-sections','-Wl,--gc-sections',
             *['-I'+str(ROOT/p) for p in paths]]
    if a.sanitize:command+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-no-pie']
    subprocess.run(command+[str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})

# Run the production actor/provider with the engine's actual DynaPoly registration
# and destruction functions. Rendering/resource lookup is a separate native compile.
body=provider[provider.index('namespace {'):provider.index('void CaveDraw(')]
register=provider[provider.index('void RegisterCave()'):provider.index('} // namespace')]
# Exclude only the draw callback from the native physics fixture. Formatting
# must not silently retain it or hide a changed production registration shape.
register,draw_replacements=re.subn(r'\bprofile\s*\.\s*draw\s*=\s*CaveDraw\s*;',
                                   'profile.draw = nullptr;',register)
assert draw_replacements==1, f'Expected exactly one CaveDraw registration, found {draw_replacements}'
api=provider[provider.index('extern "C" int ComboLakeHyliaBridge_Get'):provider.rindex('#endif')]
production=body+register+'}\n'+api
source=(ROOT/'tests/regional_passages/lake_bridge_test.cpp').read_text()
dyna=(ROOT/'soh/src/code/z_bgcheck.c').read_text()
native='\n'.join(function(dyna,signature) for signature in [
    'void BgActor_SetActor(', 's32 DynaPoly_IsBgIdBgActor(', 's32 DynaPoly_SetBgActor(',
    'DynaPolyActor* DynaPoly_GetActor(', 'void DynaPoly_DeleteBgActor(',
    'CollisionHeader* BgCheck_GetCollisionHeader(', 'u32 SurfaceType_GetData(',
    'u32 SurfaceType_GetCamDataIndex(', 'u16 BgCheck_GetBgCamSettingImpl(',
    'void ScaleRotPos_SetValue(', 's32 ScaleRotPos_Equals(', 's32 BgActor_IsTransformUnchanged(',
    'u16 DynaSSNodeList_GetNextNodeIdx(', 'void SSNode_SetValue(', 'void DynaSSNodeList_SetSSListHead(',
    'void BgCheck_Vec3fToVec3s(', 'void DynaPoly_ExpandSRT(', 'void BgCheck_Vec3sToVec3f(',
    'void CollisionPoly_GetNormalF(', 'void CollisionPoly_GetVertices(',
    's32 CollisionPoly_CheckXIntersectApprox(', 's32 CollisionPoly_CheckZIntersectApprox(',
    's32 BgCheck_ComputeWallDisplacement(', 's32 BgCheck_SphVsDynaWallInBgActor(',
    's32 CollisionPoly_CheckYIntersectApprox1(', 's32 CollisionPoly_CheckYIntersectApprox2(',
    'f32 BgCheck_RaycastFloorDynaList(', 's32 BgCheck_CheckDynaCeilingList(',
    's32 WaterBox_GetSurfaceImpl('])
native+='\n'+function((ROOT/'soh/src/code/z_bg_item.c').read_text(),'void DynaPolyActor_Init(')
math=(ROOT/'soh/src/code/sys_math3d.c').read_text()
native+='\n'+'\n'.join(function(math,signature) for signature in [
    'f32 Math3D_Vec3fMagnitudeSq(', 'f32 Math3D_Vec3fMagnitude(', 'f32 Math3D_Vec3fDistSq(',
    'void Math3D_Vec3f_Cross(', 'void Math3D_SurfaceNorm(', 'f32 Math3D_Planef(', 'f32 Math3D_DistPlaneToPos(', 'f32 Math3D_UDistPlaneToPos(',
    's32 Math3D_CirSquareVsTriSquare(', 's32 Math3D_PointDistToLine2D(',
    's32 Math3D_TriChkPointParaXImpl(', 's32 Math3D_TriChkPointParaXIntersect(',
    's32 Math3D_TriChkPointParaZImpl(', 's32 Math3D_TriChkPointParaZIntersect(',
    's32 Math3D_TriChkPointParaYImpl(', 's32 Math3D_TriChkPointParaYIntersectDist('])
skin=(ROOT/'soh/src/code/z_skin_matrix.c').read_text()
native+='\n'+'\n'.join(function(skin,signature) for signature in [
    'void SkinMatrix_Vec3fMtxFMultXYZ(', 'void SkinMatrix_MtxFMtxFMult(',
    'void SkinMatrix_SetScale(', 'void SkinMatrix_SetRotateYXZ(', 'void SkinMatrix_SetTranslate(',
    'void SkinMatrix_SetTranslateRotateYXZScale('])
lib=(ROOT/'soh/src/code/z_lib.c').read_text()
native+='\n'+'\n'.join(function(lib,signature) for signature in [
    'void Math_Vec3f_Copy(', 'void Math_Vec3s_ToVec3f(', 'void Math_Vec3f_Diff('])
native+='\n'+function((ROOT/'soh/src/code/z_actor.c').read_text(),'PosRot* Actor_GetWorldPosShapeRot(')
native+='\n'+function((ROOT/'soh/src/code/z_camera.c').read_text(),'s32 Camera_GetDataIdxForPoly(')

source='#define COMBO_LAKE_NATIVE_CASE '+str({'all':0,'camera':1,'shell':2}[a.native_case])+'\n'+source
source=source.replace('/* NATIVE_PROVIDER */',production).replace('/* NATIVE_DYNA */',native)
with tempfile.TemporaryDirectory(prefix='lake-native-') as temp:
    temp=Path(temp);cpp=temp/'test.cpp';cpp.write_text(source);binary=temp/'test'
    paths=['combo','combo/menu','soh','soh/soh','soh/include','soh/include/PR','soh/src','soh/assets','libultraship/include']
    command=[os.environ.get('CXX','c++'),'-std=c++20','-w','-DF3DEX_GBI_2','-DCOMBO_BUILD','-DCOMBO_LAKE_NATIVE_TEST','-DFMT_HEADER_ONLY',
             '-ffunction-sections','-fdata-sections','-Wl,--gc-sections',*['-I'+str(ROOT/p) for p in paths]]
    if a.sanitize:command+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-no-pie']
    subprocess.run(command+[str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
