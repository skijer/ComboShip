"""POC12: remove oblique iris shearing while retaining the accepted POC11 head."""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import zipfile
import numpy as np
from native_npc_models import NativeRig, SourceModel, fit_model, emit_mesh, asset
from refine_young_malon_eye_hair import retain_quantized_normals
from refine_young_malon_face_idle import SOURCE
from verify_native_npc_models import Archive

PARENT='f8b915f53716b3b54e32286bdbb6cac022dc49e02f6a4c2c1167f737bb27ba69'
CURVES={'L':np.array([.0805276807,-.00549492164,.00201890834,-.003,-.003,0]),
        'R':np.array([.08179354016,.00494163106,-.00140809526,-.003,-.003,0])}

def iris_depth(points,uv,rim_uv,curve,cx):
    """Constrain the interior to the fitted iris; leave the source seam exact.

    A depth interpolation from the irregular orbital perimeter creates a
    diagonal ridge across the iris. Use the previously fitted shallow iris
    surface inside, easing into the unchanged perimeter over .012 UV units.
    """
    out=points.copy();x=(points[:,0]-cx)/.024;y=(points[:,1]-.855)/.018
    z=np.column_stack([np.ones(len(x)),x,y,x*x,y*y,x*y])@curve
    distance=np.linalg.norm(uv[:,None,:]-rim_uv[None,:,:],axis=2).min(1)
    w=np.clip(distance/.012,0,1);w=w*w*(3-2*w)
    out[:,2]=points[:,2]*(1-w)+z*w
    return out

def refine(native,baseline,source_path,output):
    native,baseline,source_path,output=map(Path,(native,baseline,source_path,output))
    for p in (native,baseline,source_path):
        if output.resolve()==p.resolve() or (output.exists() and output.samefile(p)):
            raise ValueError('Output aliases a protected input')
    if hashlib.sha256(baseline.read_bytes()).hexdigest()!=PARENT:raise ValueError('Expected exact accepted POC11 parent')
    source=SourceModel(source_path)
    if hashlib.sha256(source.raw).hexdigest()!=SOURCE:raise ValueError('Unexpected GLB')
    a=Archive(native,baseline);rig=NativeRig(a,'young-malon',True);head=rig.limbs[14]
    fitted=NativeRig(a.native,'young-malon');model=fit_model(source,fitted);t=model['head_transform']
    np.testing.assert_allclose(head.T,fitted.limbs[14].T)
    info='young_malon_NativeNpcModelInfo.json';meta=json.loads(a.candidate.read(info))
    sheets=meta['geometry_repairs']['young_malon_eye_hair_poc11']['eye_sheets'];writer=asset.O2R()
    report={}
    for side,cx in [('L',.030),('R',-.032)]:
        count=sheets[side]['added_triangles'];base=a.geometry(rig.ns+'/FaceParts/'+side+'0',rig,14)
        # Quantized UV is stable across triangle-expanded copies and every state.
        cap_uv=base['uv'][-count*3:];ids=np.rint(cap_uv*16384).astype(int).reshape(-1,3,2)
        edges=Counter(tuple(sorted((tuple(x),tuple(y)))) for tri in ids for x,y in zip(tri,np.roll(tri,-1,axis=0)))
        rim_uv=np.asarray(sorted({p for e,c in edges.items() if c==1 for p in e}))/16384
        report[side]={'cap_triangles':count,'rim_vertices':len(rim_uv)}
        for state in range(5):
            path=rig.ns+f'/FaceParts/{side}{state}';g=a.geometry(path,rig,14);local=g['local'].copy()
            np.testing.assert_array_equal(g['uv'],base['uv'])
            np.testing.assert_array_equal(g['local'],base['local'])
            cap=local[-count*3:]
            source_points=(cap@head.M.T+head.T-t[:,3])@np.linalg.inv(t[:,:3]).T
            corrected=iris_depth(source_points,cap_uv,rim_uv,CURVES[side],cx)
            moved=(corrected@t[:,:3].T+t[:,3]-head.T)@head.M
            seam=np.linalg.norm(cap_uv[:,None,:]-rim_uv[None,:,:],axis=2).min(1)==0
            moved[seam]=cap[seam];local[-count*3:]=moved
            mesh=dict(positions=local@head.M.T+head.T,normals=g['normals']@head.M.T,
                      uv=g['uv'],bones=np.full(len(local),14),faces=g['faces'])
            emit_mesh(writer,'alt/'+path,mesh,rig,g['faces'],g['textures'][0],14)
            retain_quantized_normals(writer,path,local,g['uv'],g['normals'])
    meta['geometry_repairs']['young_malon_iris_poc12']=dict(parent_sha256=PARENT,eyes=report,
        scope='Interior eye depth only; orbital seams, UV, normals, textures, face, hair, body and animations retained',runtime_tested=False)
    writer.files[info]=json.dumps(meta,indent=2).encode()
    with zipfile.ZipFile(output,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
        for name in sorted(a.candidate.namelist()):
            entry=zipfile.ZipInfo(name,(2026,10,19,0,0,0));entry.compress_type=zipfile.ZIP_DEFLATED
            z.writestr(entry,writer.files.get(name,a.candidate.read(name)))
    return dict(parent_sha256=PARENT,output_sha256=hashlib.sha256(output.read_bytes()).hexdigest(),eyes=report)

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    for key in ('native','baseline','source','output'):parser.add_argument(key)
    args=parser.parse_args();print(json.dumps(refine(args.native,args.baseline,args.source,args.output),indent=2))
