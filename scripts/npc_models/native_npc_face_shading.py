"""Narrow skin-normal repairs; facial positions, UVs and artwork are preserved."""
import hashlib
import numpy as np
from native_npc_surface import _filter_normals

SOURCES={'twins':'1e02c29c4af7920b2c518f9f63f18858920709f1406d8c00a7e24f5f81f1102e',
         'young-malon':'3160899f85770bcd86f664ed73fd7d12277eea2905f42933e79be150e0d2cd3b'}

def smooth(x):
    x=np.clip(x,0.,1.);return x*x*(3-2*x)

def envelope(points):
    x,y,z=np.asarray(points).T
    front=smooth((z-.025)/.015)*smooth((.078-abs(x))/.015)
    forehead=smooth((y-.886)/.010)*smooth((.935-y)/.012)
    cheeks=smooth((abs(x)-.030)/.012)*smooth((y-.790)/.014)*smooth((.840-y)/.012)
    return .95*front*np.maximum(forehead,cheeks)

def soften_face_normals(points,normals,skin,variant):
    if variant not in SOURCES:return np.asarray(normals).copy()
    strength=envelope(points)*np.asarray(skin,bool)
    if variant=='twins':
        # The forehead already has its audited continuous curved normal field.
        strength[np.asarray(points)[:,1]>.862]=0
    return _filter_normals(points,normals,strength)

def twin_skin_normal_field(points,normals,faces,pieces,main):
    """Reconstruct continuous skin shading from the unchanged source surface.

    The supplied face has split, conflicting authored normals. Weld geometry
    samples, average incident areas, and diffuse only the frontal skin field;
    separate eyes, brows, ears and hair keep their source shading.
    """
    from native_npc_models import unit
    points=np.asarray(points);result=np.asarray(normals).copy()
    selected=faces[(pieces[faces]==main).all(1)];ids=np.unique(selected)
    p,inverse=np.unique(np.round(points[ids],6),axis=0,return_inverse=True)
    lookup=np.full(len(points),-1,int);lookup[ids]=inverse;tri=lookup[selected]
    cross=np.cross(p[tri[:,1]]-p[tri[:,0]],p[tri[:,2]]-p[tri[:,0]])
    total=np.zeros_like(p)
    for k in range(3):np.add.at(total,tri[:,k],cross)
    field=unit(total)
    frontal=(p[:,2]>.025)&(abs(p[:,0])<.08)
    if np.median(field[frontal,2])<0:field=-field
    adjacency=[set() for _ in p]
    for a,b,c in tri:
        adjacency[a].update((b,c));adjacency[b].update((a,c));adjacency[c].update((a,b))
    x,y,z=p.T
    front=smooth((.102-abs(x))/.022)*smooth((z-.015)/.020)*smooth((y-.762)/.016)
    amount=.45*front
    nose=(abs(x)<.018)&(y>.79)&(y<.842);amount[nose]*=.3
    for _ in range(18):
        average=np.array([field[list(neighbors)].mean(0) if neighbors else field[i]
                          for i,neighbors in enumerate(adjacency)])
        field=unit(field*(1-amount[:,None])+average*amount[:,None])
    weight=front[inverse,None]
    result[ids]=unit(normals[ids]*(1-weight)+field[inverse]*weight)
    return result

def twin_face_normal_field(model,normals):
    """Use one skin field across the source's overlaid brow/eyelid margins."""
    from native_npc_models import unit
    from native_npc_fit import twin_face_pieces
    from scipy.spatial import cKDTree
    p=model['source_positions'];main,face_pieces=twin_face_pieces(model)
    revised=twin_skin_normal_field(p,normals,model['faces'],model['mesh_ids'],main)
    guide=(model['mesh_ids']==main)&(p[:,2]>.025)&(p[:,1]>.775)
    followers=face_pieces&(model['mesh_ids']!=main)&(p[:,2]>.025)
    backing=(model.get('twin_band_piece',np.zeros(len(p),bool))&
             (p[:,1]<.925)&(p[:,2]>.015)&(abs(p[:,0])<.085))
    followers|=backing
    ids=np.flatnonzero(guide);targets=np.flatnonzero(followers)
    if not len(targets):return revised
    distance,near=cKDTree(p[ids,:2]).query(p[targets,:2],k=min(12,len(ids)))
    if distance.ndim==1:distance=distance[:,None];near=near[:,None]
    weight=np.exp(-(distance/.018)**2)
    field=unit((revised[ids[near]]*weight[:,:,None]).sum(1))
    blend=np.where(backing[targets],smooth((.925-p[targets,1])/.007),1.)[:,None]
    revised[targets]=unit(revised[targets]*(1-blend)+field*blend)
    exact=distance[:,0]<1e-7
    revised[targets[exact]]=revised[ids[near[exact,0]]]
    # Overlapping orbital layers are not one welded manifold. Derive one
    # continuous field from stable surrounding skin so a depth switch cannot
    # expose a differently shaded polygon. The source surface is unchanged.
    x=p[:,0]/.08;y=(p[:,1]-.85)/.08
    design=np.column_stack([np.ones(len(p)),x,y,x*x,y*y,x*y,x*x*x,y*y*y,x*x*y,x*y*y])
    radius=np.minimum(np.hypot((p[:,0]-.04)/.028,(p[:,1]-.84)/.027),
                      np.hypot((p[:,0]+.04)/.028,(p[:,1]-.84)/.027))
    stable=guide&(radius>1.4)&(abs(p[:,0])<.08)&(p[:,1]<.918)
    if np.count_nonzero(stable)>=20:
        coef=np.linalg.lstsq(design[stable],revised[stable],rcond=None)[0]
        model['twin_surface_normal_coefficients']=coef
        field=unit(design@coef)
        blend=(smooth((p[:,2]-.015)/.020)*smooth((p[:,1]-.765)/.015)*
               smooth((.10-abs(p[:,0]))/.02)*face_pieces)[:,None]
        revised=unit(revised*(1-blend)+field*blend)
    if 'twin_forehead_curve' in model:
        # The band backing and skin use different triangulations. A common
        # analytic normal field removes their visible forehead lighting edge.
        coef=model['twin_forehead_curve'];x=p[:,0];y=p[:,1]-.89
        field=unit(np.column_stack([-(coef[1]+2*coef[3]*x+coef[5]*y),
                                    -(coef[2]+2*coef[4]*y+coef[5]*x),np.ones(len(p))]))
        weight=(smooth((p[:,1]-.875)/.020)*smooth((.925-p[:,1])/.007)*
                smooth((.09-abs(x))/.015)*(p[:,2]>.015)*(face_pieces|backing))[:,None]
        revised=unit(revised*(1-weight)+field*weight)
    return revised

def repair_model(source,model,rig):
    variant=rig.name
    if variant not in SOURCES or hashlib.sha256(getattr(source,'raw',b'')).hexdigest()!=SOURCES[variant]:return None
    from native_npc_models import unit
    p=model['source_positions'];uv=model['uv'];image=np.asarray(source.image)
    rgb=image[np.clip((uv[:,1]*image.shape[0]).astype(int),0,image.shape[0]-1),
              np.clip((uv[:,0]*image.shape[1]).astype(int),0,image.shape[1]-1),:3].astype(float)
    skin=(model['bones']==rig.profile['head'])&(rgb[:,0]>rgb[:,1]*1.015)&(rgb[:,1]>rgb[:,2]*1.08)&(rgb.mean(1)>85)
    a=model['head_transform'][:,:3]
    old=unit(model['normals']@a)
    if variant=='twins':
        revised=twin_face_normal_field(model,old)
    else:revised=soften_face_normals(p,old,skin,variant)
    changed=np.linalg.norm(old-revised,axis=1)>1e-10
    model['face_shading_mask']=changed
    model['normals'][changed]=unit(revised[changed]@np.linalg.inv(a))
    model['posed_source_normals'][changed]=revised[changed]
    return dict(source_sha256=SOURCES[variant],skin_vertices=int(changed.sum()),
                positions_uvs_artwork_unchanged=True,normals_only=True)

def patch_archive(baseline,native,source_path,variant,output):
    """Apply new source normals to matching original head points in any pack.

    This allows the newer Malon pose/brooch pack to receive the normal repair
    without replacing any body, animation, material or fitted rig resources.
    """
    import json,struct,zipfile
    from pathlib import Path
    from native_npc_models import SourceModel,NativeRig,fit_model
    from verify_native_npc_models import Archive
    baseline,native,source_path,output=map(Path,(baseline,native,source_path,output))
    if output.resolve() in (baseline.resolve(),native.resolve(),source_path.resolve()):raise ValueError('Preserve inputs')
    archive=Archive(native,baseline);source=SourceModel(source_path)
    manifest=variant.replace('-','_')+'_NativeNpcModelInfo.json'
    records={n:archive.candidate.read(n) for n in archive.candidate.namelist()};meta=json.loads(records[manifest])
    if meta['variant']!=variant or meta['source_sha256']!=SOURCES[variant] or hashlib.sha256(source.raw).hexdigest()!=SOURCES[variant]:raise ValueError('Face shading source mismatch')
    if meta['native_sha256']!=hashlib.sha256(native.read_bytes()).hexdigest():raise ValueError('Native source mismatch')
    rig=NativeRig(archive.native,variant);model=fit_model(source,rig);head=rig.profile['head'];limb=rig.limbs[head]
    local=np.rint((model['positions']-limb.T)@limb.M).astype(int)
    normal=np.rint(np.clip(model['normals']@limb.M,-1,1)*127).astype(np.int8)
    allowed=model['face_shading_mask']
    lookup={tuple(point):n for point,n in zip(local[allowed],normal[allowed])}
    changed=[];vertices=0
    ns=meta['namespace']
    for name,raw in list(records.items()):
        if not name.startswith(('alt/'+ns+'/FaceParts/','alt/'+ns+f'/Body/Limb{head:02d}')):continue
        if raw[4:8]!=b'RRAO':continue
        if struct.unpack_from('<I',raw,64)[0]!=25:continue
        revised=bytearray(raw);count=struct.unpack_from('<I',raw,68)[0]
        for i in range(count):
            offset=72+i*16;point=struct.unpack_from('<hhh',raw,offset)
            if point not in lookup:continue
            encoded=lookup[point].tobytes()
            if encoded!=raw[offset+12:offset+15]:
                revised[offset+12:offset+15]=encoded;vertices+=1
        if revised!=raw:records[name]=bytes(revised);changed.append(name)
    if not changed:raise ValueError('No matching head normals were updated')
    report=dict(parent_sha256=hashlib.sha256(baseline.read_bytes()).hexdigest(),changed_head_buffers=changed,
                vertex_records_changed=vertices,normal_bytes_only=True,body_rig_poses_brooch_and_textures_unchanged=True,
                source_repair=model['face_shading_repair'],runtime_tested=False)
    meta['face_shading_refinement']=report;records[manifest]=json.dumps(meta,indent=2).encode()
    output.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(output,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
        for name,raw in sorted(records.items()):
            info=zipfile.ZipInfo(name,(2026,10,19,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED;z.writestr(info,raw)
    return dict(report,output_sha256=hashlib.sha256(output.read_bytes()).hexdigest())

if __name__=='__main__':
    import argparse,json
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('baseline','native','source','variant','output'):p.add_argument(name)
    a=p.parse_args();print(json.dumps(patch_archive(a.baseline,a.native,a.source,a.variant,a.output),indent=2))
