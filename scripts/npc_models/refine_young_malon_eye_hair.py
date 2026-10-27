"""Narrow child Malon oblique-eye and hair-root archive repair."""
import argparse
from collections import Counter,defaultdict
import hashlib
import json
from pathlib import Path
import struct
import zipfile
import numpy as np
from PIL import Image
from scipy.interpolate import RBFInterpolator
from native_npc_models import NativeRig,SourceModel,asset,emit_mesh,fit_model,unit
from native_npc_face import expression_subdivision
from refine_young_malon_face_idle import (BOUNDS,SOURCE,eye_artwork,
                                        eye_curves,smooth,texture_resource)
from verify_native_npc_models import Archive

PARENT='40eef326945f6d3d8181c1ad1e77e4f96bcd099ad928ba533d500e6bebb7072c'


def paint_hair_roots(points, uv, faces, mesh_ids, atlas, patches):
    """Consume registered crop edits only on audited hair/root UV texels.

    The mesh owner and interpolated source height gate the transfer. Original
    body/face islands, UV layout, alpha and lower hair are protected.
    """
    atlas=np.asarray(atlas)
    out=atlas.copy();height,width=atlas.shape[:2]
    applied=np.zeros((height,width),float)
    points=np.asarray(points);pixel=np.asarray(uv)*[width-1,height-1]
    mesh_ids=np.asarray(mesh_ids);faces=np.asarray(faces)
    for patch in patches:
        x0,y0,x1,y1=patch['bounds']
        art=None
        if 'chroma' not in patch:
            raw=patch['artwork']
            image=Image.fromarray(raw) if isinstance(raw,np.ndarray) else Image.open(raw).convert('RGBA')
            art=np.asarray(image.resize((x1-x0,y1-y0),Image.Resampling.LANCZOS))
        selected=np.isin(mesh_ids[faces],patch['meshes']).all(1)
        selected&=(points[faces,1]>patch['min_y']).any(1)
        for face in faces[selected]:
            q=pixel[face]
            low=np.maximum(np.floor(q.min(0)).astype(int),[x0,y0])
            high=np.minimum(np.ceil(q.max(0)).astype(int),[x1-1,y1-1])
            if np.any(high<low):continue
            matrix=np.column_stack([q[0]-q[2],q[1]-q[2]])
            if abs(np.linalg.det(matrix))<1e-10:continue
            xx,yy=np.meshgrid(np.arange(low[0],high[0]+1),np.arange(low[1],high[1]+1))
            bary=(np.stack([xx,yy],-1)-q[2])@np.linalg.inv(matrix).T
            bary=np.dstack([bary,1-bary.sum(-1)])
            inside=bary.min(-1)>=-1e-6
            source_y=bary@points[face,1]
            height_weight=smooth((source_y-patch['min_y'])/.012)
            original=atlas[yy,xx].astype(float)
            # Warm dark strands are already good. Fade the authored repair
            # in only where the source roots become pink/gray/peach.
            if 'chroma' in patch:
                # Transfer the authored copper chroma to the opposite roots.
                # Original red-channel detail keeps their strand highlights;
                # warm gold pixels are excluded from this gray/pink repair.
                ratio=original[:,:,2]/np.maximum(original[:,:,0],1)
                color_weight=smooth((ratio-.38)/.18)
                paint=original.copy()
                paint[:,:,:3]=original[:,:,:1]*np.asarray(patch['chroma'])
            else:
                ratio=original[:,:,1]/np.maximum(original[:,:,0],1)
                color_weight=smooth((ratio-.45)/.20)
                paint=art[yy-y0,xx-x0].astype(float)
            weight=inside*height_weight*color_weight
            if patch.get('fill_only'):weight*=applied[yy,xx]==0
            active=weight>applied[yy,xx]
            if not active.any():continue
            mixed=np.rint(original*(1-weight[:,:,None])+paint*weight[:,:,None]).clip(0,255).astype(np.uint8)
            mixed[:,:,3]=atlas[yy,xx,3]
            out[yy[active],xx[active]]=mixed[active]
            applied[yy[active],xx[active]]=weight[active]
    return out,applied


def eye_regions(mesh):
    """Recover the original ordered eye-face provenance, including UV copies."""
    p,faces=mesh['positions'],mesh['faces'];remaining=np.ones(len(faces),bool)
    for side,cx in (('L',.031),('R',-.031)):
        point=p[faces]
        inside=(((point[:,:,0]-cx)/.027)**2+((point[:,:,1]-.855)/.025)**2<1.25**2)&(point[:,:,2]>.015)
        chosen=inside.any(1)&remaining;remaining[chosen]=False
        yield side,cx,faces[chosen],np.flatnonzero(chosen)


def eye_disk(local,normal,uv,source_rim,curve,center_x,transform,head):
    """Replace the folded cavity with one sheet, locking its complete rim."""
    def cross(a,b):return a[0]*b[1]-a[1]*b[0]
    # An orbital rim can be concave. A fan from its mean crosses the inset;
    # ear clipping retains the actual boundary without assuming star shape.
    polygon=list(range(len(uv)))
    signed=sum(cross(a,b) for a,b in zip(uv,np.roll(uv,-1,axis=0)))
    if signed>0:polygon.reverse()  # Clockwise UV gives an outward source face.
    triangles=[]
    while len(polygon)>3:
        ears=[]
        for i,b in enumerate(polygon):
            a,c=polygon[i-1],polygon[(i+1)%len(polygon)]
            area=cross(uv[b]-uv[a],uv[c]-uv[a])
            if area>=-1e-12:continue
            occupied=False
            for j in polygon:
                if j in (a,b,c):continue
                point=uv[j]
                if all(cross(uv[v]-uv[u],point-uv[u])<=1e-12 for u,v in ((a,b),(b,c),(c,a))):
                    occupied=True;break
            if not occupied:ears.append((-area,i,(a,b,c)))
        if not ears:raise ValueError('Eye rim cannot be triangulated without crossing')
        _,index,triangle=max(ears)
        triangles.append(triangle);polygon.pop(index)
    triangles.append(tuple(polygon))
    initial=np.asarray(triangles,int)
    st=list(uv)
    # Refine shared interior edges on both sides; never insert a point on
    # the source perimeter, where integer rounding would open a tiny seam.
    for _ in range(3):
        edges=Counter(tuple(sorted((a,b))) for face in triangles for a,b in zip(face,np.roll(face,-1)))
        middle={}
        for edge,count in edges.items():
            if count==2:
                middle[edge]=len(st);st.append((np.asarray(st[edge[0]])+st[edge[1]])/2)
        refined=[]
        for face in triangles:
            a,b,c=face
            mids=[middle.get(tuple(sorted(edge))) for edge in ((a,b),(b,c),(c,a))]
            count=sum(m is not None for m in mids)
            if count==0:refined.append(face);continue
            if count==3:
                ab,bc,ca=mids
                refined.extend([(a,ab,ca),(ab,b,bc),(ca,bc,c),(ab,bc,ca)])
                continue
            for shift in range(3):
                rotated=face[shift:]+face[:shift];a,b,c=rotated
                ab=middle.get(tuple(sorted((a,b))));bc=middle.get(tuple(sorted((b,c))))
                if (count==1 and ab is not None) or (count==2 and ab is not None and bc is not None):break
            if count==1:refined.extend([(a,ab,c),(ab,b,c)])
            else:refined.extend([(a,ab,c),(ab,bc,c),(ab,b,bc)])
        triangles=refined
    st=np.asarray(st);center_uv=uv.mean(0)
    def inside(point):
        return any(all(cross(uv[v]-uv[u],point-uv[u])<=1e-12 for u,v in zip(face,np.roll(face,-1))) for face in initial)
    if not inside(center_uv):
        largest=max(initial,key=lambda f:abs(cross(uv[f[1]]-uv[f[0]],uv[f[2]]-uv[f[0]])))
        center_uv=uv[largest].mean(0)
    xy_map=np.linalg.lstsq(np.column_stack([uv,np.ones(len(uv))]),source_rim[:,:2],rcond=None)[0]
    center_xy=np.r_[center_uv,1]@xy_map
    x,y=(center_xy[0]-center_x)/.024,(center_xy[1]-.855)/.018
    center_z=np.array([1,x,y,x*x,y*y,x*y])@curve
    anchors=np.vstack([uv,center_uv])
    depth=RBFInterpolator(anchors,np.r_[source_rim[:,2],center_z],kernel='thin_plate_spline')
    field=RBFInterpolator(anchors,np.vstack([normal,unit(normal.mean(0))]),kernel='thin_plate_spline')
    points=np.column_stack([np.column_stack([st,np.ones(len(st))])@xy_map,depth(st)])
    positions=(points@transform[:,:3].T+transform[:,3]-head.T)@head.M
    normals=unit(field(st))
    positions[:len(local)]=local;normals[:len(local)]=normal
    return positions,normals,st,np.asarray(triangles,int)


def retain_quantized_normals(writer,path,local,uv,normal):
    """Do not renormalize the previously accepted retained border normals."""
    lookup={}
    for point,st,n in zip(np.rint(local).astype(int),np.rint(uv*16384).astype(int),np.rint(normal*127).astype(np.int8)):
        key=(*point,*st)
        if key in lookup and lookup[key]!=n.tobytes():
            raise ValueError('Conflicting normals on a retained eye seam')
        lookup[key]=n.tobytes()
    name='alt/'+path+'/V';raw=bytearray(writer.files[name])
    for i in range(struct.unpack_from('<I',raw,68)[0]):
        offset=72+i*16;xyz=struct.unpack_from('<hhh',raw,offset);st=struct.unpack_from('<hh',raw,offset+8)
        raw[offset+12:offset+15]=lookup[(*xyz,*st)]
    writer.files[name]=bytes(raw)


def rebuild_eye_sheets(archive,source,rig,transform,aperture):
    mesh=source.meshes[25];p=mesh['positions'];faces=mesh['faces'];head=rig.limbs[14]
    curves=eye_curves(p,aperture);writer=asset.O2R();reports={}
    for side,cx,region,source_faces in eye_regions(mesh):
        indices=region.ravel();original_faces=np.arange(len(indices)).reshape(-1,3)
        bary,origin=expression_subdivision(original_faces,levels=2)
        original_uv=(p[indices,:2]-BOUNDS[:2])/np.subtract(BOUNDS[2:],BOUNDS[:2]);original_uv[:,1]=1-original_uv[:,1]
        original_uv=np.rint(original_uv*16384)/16384
        expected=np.einsum('fij,fjk->fik',bary,original_uv[origin]).reshape(-1,2)
        eye=archive.geometry(rig.ns+'/FaceParts/'+side+'0',rig,14)
        np.testing.assert_allclose(expected,eye['uv'],atol=2/16384)
        radius=((p[:,0]-cx)/.020)**2+((p[:,1]-.855)/.014)**2
        patch=(radius[faces]<1).any(1)&(p[faces,2]>.05).all(1)
        selected=patch[source_faces]
        if selected.sum()!=78 or patch.sum()!=78:raise ValueError('Eye cavity provenance changed')
        remove=np.tile(selected,9)
        edges=Counter(tuple(sorted((int(i),int(j)))) for face in faces[patch] for i,j in zip(face,np.roll(face,-1)))
        graph=defaultdict(set)
        for (i,j),count in edges.items():
            if count==1:graph[i].add(j);graph[j].add(i)
        if not graph or not all(len(v)==2 for v in graph.values()):raise ValueError('Eye rim is not a simple loop')
        rim=[min(graph)];last=None;current=rim[0]
        while True:
            nxt=next(v for v in graph[current] if v!=last)
            if nxt==rim[0]:break
            if nxt in rim:raise ValueError('Eye rim crosses itself')
            rim.append(nxt);last,current=current,nxt
        if len(rim)!=len(graph):raise ValueError('Eye cavity has multiple boundaries')
        vertex_records={}
        for k,(b,face) in enumerate(zip(bary,origin)):
            for j,weights in enumerate(b):
                corner=np.flatnonzero(weights==1)
                if len(corner)==1:
                    index=int(indices[face[corner[0]]])
                    vertex_records[index]=(eye['local'][k*3+j],eye['normals'][k*3+j],eye['uv'][k*3+j])
        rim_local=np.array([vertex_records[i][0] for i in rim])
        rim_normal=np.array([vertex_records[i][1] for i in rim]);rim_uv=np.array([vertex_records[i][2] for i in rim])
        rim_source=(rim_local@head.M.T+head.T-transform[:,3])@np.linalg.inv(transform[:,:3]).T
        cap=eye_disk(rim_local,rim_normal,rim_uv,rim_source,curves[1 if side=='L' else 0],
                     .030 if side=='L' else -.032,transform,head)
        positions,normals,uv,triangles=cap
        reports[side]=dict(source_faces=source_faces[selected].tolist(),rim= rim,
                           retained_rim_xyz_and_normals_exact=True,removed_subtriangles=int(remove.sum()),
                           added_triangles=len(triangles),outside_cavity_triangles_unchanged=True)
        for state in range(5):
            path=rig.ns+f'/FaceParts/{side}{state}';data=archive.geometry(path,rig,14)
            for name in ('local','normals','uv'):np.testing.assert_array_equal(data[name],eye[name])
            length=len(data['local']);local=np.vstack([data['local'],positions])
            normal=np.vstack([data['normals'],normals]);out_uv=np.vstack([data['uv'],uv])
            out_faces=np.vstack([data['faces'][~remove],triangles+length])
            output=dict(positions=local@head.M.T+head.T,normals=normal@head.M.T,
                        uv=out_uv,bones=np.full(len(local),14),faces=out_faces)
            emit_mesh(writer,'alt/'+path,output,rig,out_faces,data['textures'][0],14)
            retain_quantized_normals(writer,path,local,out_uv,normal)
    return writer.files,reports


def refine(native,baseline,source_path,art_directory,output):
    native,baseline,source_path,art_directory,output=map(Path,(native,baseline,source_path,art_directory,output))
    protected=[native,baseline,source_path,*art_directory.glob('*'),
               art_directory.parent/'young-malon-poc9/iris-edit.png']
    if any(output.resolve()==p.resolve() or (output.exists() and p.exists() and output.samefile(p)) for p in protected):
        raise ValueError('Output aliases a protected input; choose a separate candidate archive')
    if hashlib.sha256(baseline.read_bytes()).hexdigest()!=PARENT:raise ValueError('Use the exact reviewed POC10 pack')
    source=SourceModel(source_path)
    if hashlib.sha256(source.raw).hexdigest()!=SOURCE:raise ValueError('Unexpected source GLB')
    archive=Archive(native,baseline);rig=NativeRig(archive,'young-malon',True)
    fitted=NativeRig(archive.native,'young-malon');model=fit_model(source,fitted)
    np.testing.assert_allclose(fitted.limbs[14].T,rig.limbs[14].T,atol=1e-6)
    ns=rig.ns;spec=json.loads((art_directory/'projection.json').read_text())
    if spec['source_sha256']!=SOURCE:raise ValueError('Hair artwork belongs to another model')
    for patch in spec['patches']:
        if 'artwork' in patch:patch['artwork']=art_directory/patch['artwork']
    # POC10's neutral keeps the original aperture outline, except the already
    # accepted authored interior. Its color segmentation still recovers it.
    artwork=art_directory.parent/'young-malon-poc9/iris-edit.png'
    _,_,aperture=eye_artwork(archive,ns,artwork)
    changes,eyes=rebuild_eye_sheets(archive,source,rig,model['head_transform'],aperture)
    # This texture is already head-private. Reuse its binding rather than
    # duplicating a full atlas or touching any body/mouth display lists.
    face_texture=ns+'/Textures/FaceDiffusePOC9'
    kind,w,h,pixels=archive.texture(face_texture)
    if kind!=1:raise ValueError('Unexpected face atlas format')
    old=np.frombuffer(pixels,np.uint8).reshape(h,w,4)
    painted,mask=paint_hair_roots(model['source_positions'],model['uv'],model['faces'],model['mesh_ids'],old,spec['patches'])
    changes['alt/'+face_texture]=texture_resource(painted)
    report=dict(parent_sha256=PARENT,source_sha256=SOURCE,eye_sheets=eyes,
                painted_hair_root_texels=int(np.count_nonzero(mask)),
                original_diffuse_and_all_eye_artwork_byte_identical=True,
                all_pose_skin_rig_and_body_vertex_resources_byte_identical=True,runtime_tested=False)
    info='young_malon_NativeNpcModelInfo.json';meta=json.loads(archive.candidate.read(info))
    meta.setdefault('geometry_repairs',{})['young_malon_eye_hair_poc11']=report
    changes[info]=json.dumps(meta,indent=2).encode()
    allowed=lambda name:(name==info or name=='alt/'+face_texture or name.startswith('alt/'+ns+'/FaceParts/'))
    if any(not allowed(name) for name in changes):raise ValueError('Repair escaped the private head')
    output.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(output,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
        for name in sorted(set(archive.candidate.namelist())|set(changes)):
            entry=zipfile.ZipInfo(name,(2026,10,19,0,0,0));entry.compress_type=zipfile.ZIP_DEFLATED
            z.writestr(entry,changes[name] if name in changes else archive.candidate.read(name))
    return dict(report,changed_resources=sorted(changes),output_sha256=hashlib.sha256(output.read_bytes()).hexdigest())


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('native','baseline','source','art_directory','output'):parser.add_argument(name)
    args=parser.parse_args()
    print(json.dumps(refine(args.native,args.baseline,args.source,args.art_directory,args.output),indent=2))
