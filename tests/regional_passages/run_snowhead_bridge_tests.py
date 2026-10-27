#!/usr/bin/env python3
"""Compile production mesh and verify it against native and Alt spot08 collision."""
from pathlib import Path
import os, subprocess, tempfile, argparse, sys
parser=argparse.ArgumentParser()
parser.add_argument("--scene-dir",default=os.environ.get("COMBO_SCENE_EVIDENCE"))
parser.add_argument("--native-archive",default=os.environ.get("COMBO_NATIVE_OOT_ARCHIVE"))
args=parser.parse_args()
ROOT=Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix='snowhead-bridge-') as td:
    binary=Path(td)/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',
                    '-I'+str(ROOT/'combo'),str(ROOT/'tests/regional_passages/snowhead_bridge_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
    # Every actual shelf triangle participates, including the submerged sides,
    # apron and throat. Exact native placements/meshes are embedded in this
    # portable regression; no donor archive or one-pose AABBs are required.
    mesh_text=subprocess.check_output([str(binary),'--mesh'],text=True)
    ice_command=[os.environ.get('PYTHON','python3'),
                 str(ROOT/'tests/regional_passages/ice_platform_clearance_test.py')]
    if args.native_archive:ice_command+=['--native-archive',args.native_archive]
    subprocess.run(ice_command,
                   input=mesh_text,text=True,check=True)
    if args.scene_dir:
        import numpy as np, struct
        from ice_platform_clearance_test import exit_clearance, body_prism_clearance
        # Native adult GetHeight68 / ceilingCheckHeight56, child44/40.
        # Exact native triangle clipping gives85.9732577 minimum overhead;
        # probe the whole shelf width to80 (12 above adult GetHeight), while
        # the Snowhead throat keeps its separate180-unit opening requirement.
        route_headroom=80
        authored=np.array([[float(x) for x in line.split()] for line in subprocess.check_output([str(binary),'--mesh'],text=True).splitlines()]).reshape(-1,3,3)
        evidence=Path(args.scene_dir)
        def crosses(a,b,t):
            e1,e2=t[1]-t[0],t[2]-t[0]; direction=b-a;h=np.cross(direction,e2);det=e1@h
            if abs(det)<1e-7:return False
            s=a-t[0];u=s@h/det;q=np.cross(s,e1);v=direction@q/det;d=e2@q/det
            return 0<=u<=1 and 0<=v and u+v<=1 and 1e-5<d<1-1e-5
        # Every authored top triangle is interleaved with its foundation sides.
        for variant in ('','alt/'):
            b=(evidence/(variant+'scenes/shared/spot08_scene/spot08_sceneCollisionHeader_002CE0')).read_bytes()
            nv=struct.unpack_from('<I',b,76)[0];vertices=np.array([struct.unpack_from('<hhh',b,80+6*i) for i in range(nv)],float)
            off=80+6*nv;np_=struct.unpack_from('<I',b,off)[0];polys=[struct.unpack_from('<HHHHhhhh',b,off+4+16*i) for i in range(np_)]
            native=np.array([vertices[[p[1]&8191,p[2]&8191,p[3]&8191]] for p in polys]);hits=[]
            floor_indices=[i for i in range(len(authored)) if i%6<2]
            body_prism_clearance(authored,native,route_headroom,label=(variant or 'native')+' complete collision')
            for i in floor_indices:
                t=authored[i]
                # Dense barycentric samples of actual production triangles across
                # full authored shelf width,80-unit body envelope, native/Alt controls.
                for u in np.linspace(.03,.97,12):
                    for v in np.linspace(.03,1-u,12):
                        if u+v>=1:continue
                        p=t[0]+u*(t[1]-t[0])+v*(t[2]-t[0]);a=p+[0,5,0];c=p+[0,route_headroom,0]
                        mask=np.all(native.max(1)>=np.minimum(a,c),axis=1)&np.all(native.min(1)<=np.maximum(a,c),axis=1)
                        for j in np.flatnonzero(mask):
                            if crosses(a,c,native[j]):hits.append((i,int(j),p.tolist()));break
            assert not hits,f'{variant or "native"} body/headroom collision: {hits[:8]}'
            # The right-side route remains >120 from the ACTUAL exit triangle.
            # A blanket min-Z comparison includes a remote western exit vertex
            # and rejected a physically clear route; use exact full footprints.
            exit_clearance(authored,native[81 if variant else 87],minimum_gap=120)
            off+=4+16*np_;n=struct.unpack_from('<I',b,off)[0];off+=4+8*n
            n=struct.unpack_from('<I',b,off)[0];off+=4+8*n
            n=struct.unpack_from('<I',b,off)[0];off+=4+6*n
            n=struct.unpack_from('<I',b,off)[0];off+=4
            waters=[struct.unpack_from('<5hI',b,off+i*14) for i in range(n)]
            assert any(x<=1000<=x+w and z<=850<=z+d and y==20 for x,y,z,w,d,_ in waters),waters
            # First slope begins20 units submerged: both child/adult
            # swim onto the same continuous slope, with no vertical lip to mantle.
            print('PASS',variant or 'native','spot08 actual mesh/body clearance and native Ice exit separation')
        import xml.etree.ElementTree as E
        for variant in ['', 'alt/']:
            base=evidence/'scenes/shared/spot08_scene';alt=evidence/(variant+'scenes/shared/spot08_scene');triangles=[];paths=[]
            def resolve(p):
                f=evidence/(variant+p)
                return f if f.exists() else evidence/p
            for file in base.glob('*_tri_*'):
                f=alt/file.name if (alt/file.name).exists() else file
                if not f.read_bytes().startswith(b'<DisplayList'):continue
                buffer={}
                for n in E.fromstring(f.read_text()):
                    if n.tag=='LoadVertices':
                        vertices=E.fromstring(resolve(n.attrib['Path']).read_text());v=[tuple(float(a.attrib[k])for k in ['X','Y','Z'])for a in vertices]
                        off=int(n.attrib.get('VertexOffset',0));idx=int(n.attrib.get('VertexBufferIndex',0));count=int(n.attrib['Count']);buffer.update({idx+i:v[off+i]for i in range(count)})
                    if n.tag in ('Triangles2','Triangle2','Triangle1'):
                        for prefix in ['V0','V1']if n.tag in ('Triangles2','Triangle2') else ['V0']:
                            t=np.array([buffer[int(n.attrib[prefix+str(i)])]for i in range(3)])
                            if np.all(t[:,1]==20):continue
                            triangles.append(t);paths.append(f.name)
            triangles=np.array(triangles);hits=[]
            body_prism_clearance(authored,triangles,route_headroom,label=(variant or 'native')+' complete render')
            for i in [j for j in range(len(authored))if j%6<2]:
                t=authored[i]
                for u in np.linspace(.03,.97,12):
                    for v in np.linspace(.03,1-u,12):
                        if u+v>=1:continue
                        p=t[0]+u*(t[1]-t[0])+v*(t[2]-t[0]);a=p+[0,5,0];b=p+[0,route_headroom,0]
                        mask=np.all(triangles.max(1)>=np.minimum(a,b),axis=1)&np.all(triangles.min(1)<=np.maximum(a,b),axis=1)
                        for j in np.flatnonzero(mask):
                            if crosses(a,b,triangles[j]):hits.append((i,paths[j],p.tolist()));break
            assert not hits,f'{variant or "native"} visible terrain crosses ascent: {hits[:8]}'
            print('PASS',variant or 'native',len(triangles),'render triangles clear full ascent body envelope')
    production=(ROOT/'soh/soh/Enhancements/Gameplay/ComboFountainSnowheadBridge.cpp').read_text()
    # Renderer and hook registration are compiled separately against native
    # headers; the lifecycle fixture exercises actual production bodies.
    body=production[production.index('namespace {'):production.index('void Draw(')]
    body+='void Draw(Actor*,PlayState*) {}\n'
    body+=production[production.index('void Ensure('):production.index('void Register()')]
    body+='}\n'+production[production.index('extern "C" int ComboFountainSnowheadBridge_Get('):production.rindex('#endif')]
    import re
    def function(source,name):
        match=re.search(r'^\w+[ *]+(?:'+re.escape(name)+r')\([^;{]*\)\s*\{',source,re.M)
        assert match, name
        begin=match.start();opening=source.index('{',match.start());depth=1;end=opening+1
        while depth:
            depth+=(source[end]=='{')-(source[end]=='}');end+=1
        return source[begin:end]
    collision=(ROOT/'soh/src/code/z_bgcheck.c').read_text()
    math=(ROOT/'soh/src/code/sys_math3d.c').read_text()
    camera=(ROOT/'soh/src/code/z_camera.c').read_text()
    actor=(ROOT/'soh/src/code/z_bg_item.c').read_text()
    native_bodies=[]
    for names,source in [(['BgActor_SetActor','DynaPoly_SetBgActor','BgCheck_GetCollisionHeader',
            'SurfaceType_GetData','SurfaceType_GetCamDataIndex','BgCheck_GetBgCamSettingImpl',
            'CollisionPoly_GetNormalF','BgCheck_Vec3sToVec3f','CollisionPoly_GetVertices',
            'CollisionPoly_CheckXIntersectApprox','CollisionPoly_CheckZIntersectApprox','CollisionPoly_CheckYIntersectApprox1',
            'BgCheck_ComputeWallDisplacement','BgCheck_SphVsDynaWallInBgActor'],collision),
            (['Math3D_CirSquareVsTriSquare','Math3D_PointDistToLine2D','Math3D_Planef',
            'Math3D_DistPlaneToPos','Math3D_TriChkPointParaXImpl','Math3D_TriChkPointParaZImpl',
            'Math3D_TriChkPointParaXIntersect','Math3D_TriChkPointParaZIntersect','Math3D_TriChkPointParaYImpl','Math3D_TriChkPointParaYIntersectDist'],math),
            (['Math_Vec3s_ToVec3f'],(ROOT/'soh/src/code/z_lib.c').read_text()),
            (['Camera_GetDataIdxForPoly'],camera),(['DynaPolyActor_Init'],actor)]:
        for name in names:
            text=function(source,name)
            if name=='DynaPoly_SetBgActor':text=text.replace('DynaPoly_SetBgActor','Native_DynaPoly_SetBgActor')
            native_bodies.append(text)
    native=Path(td)/'native.cpp'
    fixture=(ROOT/'tests/regional_passages/snowhead_bridge_test.cpp').read_text()
    native.write_text(fixture.replace('/* NATIVE_ENGINE */','extern "C" {\n'+'\n'.join(native_bodies)+'\n}').replace('/* NATIVE_PRODUCTION */',body))
    env=dict(os.environ)
    if env.get('COMBO_NEI_FIXTURE_CPATH'):
        env['CPATH']=env['COMBO_NEI_FIXTURE_CPATH']+os.pathsep+env.get('CPATH','')
    paths=['soh','soh/soh','soh/include','soh/include/PR','soh/src','soh/assets','libultraship/include','combo']
    flags=['-std=c++20','-w','-DCOMBO_BUILD','-DF3DEX_GBI_2','-DFMT_HEADER_ONLY',*['-I'+str(ROOT/p) for p in paths]]
    # A syntax-only compile accepts anonymous-namespace interpolation imports
    # that cannot link. Compile the actual full Draw object and inspect its ABI.
    draw_object=Path(td)/'snowhead-render.o'
    subprocess.run([os.environ.get('CXX','c++'),*flags,'-c',str(ROOT/'soh/soh/Enhancements/Gameplay/ComboFountainSnowheadBridge.cpp'),'-o',str(draw_object)],check=True,env=env)
    imports=subprocess.check_output(['nm','-u',str(draw_object)],text=True,env=env)
    for hook in ('FrameInterpolation_RecordOpenChild','FrameInterpolation_RecordCloseChild'):
        assert any(line.split()==['U',hook]for line in imports.splitlines()), 'Native render hook must retain C linkage: '+hook
    assert not any('FrameInterpolation_Record' in line and '_ZN' in line for line in imports.splitlines())
    print('PASS actual Snowhead render object: native interpolation imports retain unmangled C ABI')
    subprocess.run([os.environ.get('CXX','c++'),*flags,'-DSNOWHEAD_NATIVE_TEST','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',str(native),'-o',str(binary)],check=True,env=env)
    subprocess.run([str(binary)],check=True,env={**env,'ASAN_OPTIONS':'detect_leaks=0'})

# Execute actual native effect initialization and the actor-owned replacement
# renderer; attempted effect spawns are not sufficient presentation evidence.
chill_command=[sys.executable,str(ROOT/"tests/regional_passages/run_chill_tests.py")]
if args.native_archive:chill_command += ["--native-archive",str(args.native_archive)]
subprocess.run(chill_command,check=True,env=env)
