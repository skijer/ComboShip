"""Rebuild only the twins' shoulder strips inside their private pose assets.

The original model, source atlas, head, hands, skeleton and native clips remain
in the input archive. The replacement strips use the existing v1 flex palette.
"""
import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path
from types import SimpleNamespace
import numpy as np
from scipy.spatial import cKDTree
from scipy.linalg import polar
from native_npc_models import SourceModel, NativeRig, animation_frames, emit_mesh, wrapper, asset, unit
from native_npc_skin import prepare, decode, STEPS
from native_npc_twins import SOURCE_SHA256
from verify_native_npc_models import Archive

def perimeter_angles(points,groups,graph,approximate):
    """Parameterize the actual garment edge, including its concave corners."""
    lookup={int(group):i for i,group in enumerate(groups)}
    seed=int(groups[np.argmin(approximate)]);current=seed;previous=None;order=[]
    while current not in order:
        order.append(current)
        neighbors=[n for n in graph[current] if n in lookup]
        if len(neighbors)!=2:raise ValueError('The garment armhole must be a simple closed boundary')
        if previous is None:
            following=min(neighbors,key=lambda n:(approximate[lookup[n]]-approximate[lookup[current]])%(2*np.pi))
        else:following=next(n for n in neighbors if n!=previous)
        previous,current=current,following
    if current!=seed or len(order)!=len(groups):raise ValueError('The garment armhole is disconnected')
    indices=np.array([lookup[n] for n in order])
    angle=np.unwrap(approximate[indices])
    # Keep the source angular spacing while eliminating small reversals at
    # concave armhole corners; sorting raw polar angles would cut across them.
    epsilon=1e-4;values=angle-np.arange(len(angle))*epsilon;blocks=[]
    for value in values:
        blocks.append([float(value),1])
        while len(blocks)>1 and blocks[-2][0]>blocks[-1][0]:
            b=blocks.pop();a=blocks.pop();weight=a[1]+b[1]
            blocks.append([(a[0]*a[1]+b[0]*b[1])/weight,weight])
    angle=np.concatenate([np.full(weight,value) for value,weight in blocks])+np.arange(len(angle))*epsilon
    if angle[-1]-angle[0]>=2*np.pi:raise ValueError('The garment boundary winds more than once')
    result=np.empty(len(points));result[indices]=(angle+np.pi)%(2*np.pi)-np.pi
    return result

def refine(native,source_path,candidate,fitted,output):
    base=Path(candidate);source=SourceModel(source_path)
    if hashlib.sha256(source.raw).hexdigest()!=SOURCE_SHA256:
        raise ValueError('This shoulder repair is bounded to the reviewed twins GLB')
    with np.load(fitted,allow_pickle=False) as saved:
        required=('positions','normals','source_positions','uv','weights','faces','bones','hosts','scale',
                  'posed_source_positions','posed_source_normals','head_transform','source_bone_transforms',
                  'authored_joint_mask','authored_joint_weights','rigid_attachments','mesh_ids','boundary',
                  'bind_matrices','bind_positions')
        model={k:saved[k] for k in required if k in saved.files}
    np.testing.assert_array_equal(model['source_positions'],np.concatenate([m['positions'] for m in source.meshes]))
    np.testing.assert_array_equal(model['uv'],np.concatenate([m['uv'] for m in source.meshes]))
    archive=Archive(native,base);rig=NativeRig(archive,'twins',True);pal=prepare(source,model,rig)
    previous_palette=decode(archive.read(rig.ns+'/Skin/Palette'))
    np.testing.assert_allclose(pal.inverse,previous_palette.inverse,rtol=0,atol=1.3e-4,
                               err_msg='The fitted checkpoint does not match the archive skin palette')
    if pal.blends!=previous_palette.blends:
        raise ValueError('The fitted checkpoint does not match the archive blend-slot order')
    g=pal.geometry_for(model);bm=np.array([l.M.copy() for l in rig.limbs]);bt=np.array([l.T.copy() for l in rig.limbs])
    np.testing.assert_allclose(model['bind_matrices'],bm,atol=1e-8)
    np.testing.assert_allclose(model['bind_positions'],bt,atol=1e-6)
    slots=rig.skel.slot_to_limb;sp=model['source_positions'];mi=model['mesh_ids'];fs=model['faces']
    joints={n:source.world[i][:3,3] for n,i in zip(source.names,source.joints)}
    image=np.asarray(source.image);out=asset.O2R();pose_records=[]

    def blend_slot(arm,t):
     a,b=slots.index(rig.profile['chest']),slots.index(arm);q=int(np.rint(t*STEPS))
     if q==0:return a
     if q==STEPS:return b
     key=(a,b,q)
     if key not in pal.blends:pal.blends.append(key)
     return len(slots)+pal.blends.index(key)
    for side in ('left','right'):
     for step in range(STEPS+1):blend_slot(rig.profile[side][3],step/STEPS)

    def periodic(theta,angle,values):
     order=np.argsort(angle);a=angle[order];v=np.asarray(values)[order];ae=np.r_[a[-1]-2*np.pi,a,a[0]+2*np.pi];ve=np.concatenate([v[-1:],v,v[:1]])
     if v.ndim==1:return np.interp(theta,ae,ve)
     return np.column_stack([np.interp(theta,ae,ve[:,k]) for k in range(v.shape[1])])

    def colors(uv):
     from native_assets.render import Tex,_sample_bilinear
     xy=np.asarray(uv)*np.array(source.image.size)
     return _sample_bilinear(Tex(image,2,2),xy[:,0],xy[:,1],2,2)[:,:3]

    def edge_colors(info,source_center,theta,v,w,uv):
     samples=[]
     for angle in theta:
      direction=np.array([np.cos(angle),np.sin(angle)]);found=None
      for a,b in zip(info['end'][::2],info['end'][1::2]):
       first=a[1]-source_center;last=b[1]-source_center
       p0=np.array([first@v,first@w]);p1=np.array([last@v,last@w]);matrix=np.column_stack([p1-p0,-direction])
       if abs(np.linalg.det(matrix))<1e-12:continue
       t,r=np.linalg.solve(matrix,-p0)
       if -.00001<=t<=1.00001 and r>0:
        found=uv[a[0]]*(1-t)+uv[b[0]]*t;break
      if found is None:raise ValueError('The shoulder cut is not a closed star-shaped ring')
      samples.append(found)
     return colors(np.asarray(samples))

    def encode_geometry(P,N,UV,S,F,path,host,texture,mat):
     P=np.asarray(P);N=unit(np.asarray(N));S=np.asarray(S);canonical=P.copy();cn=N.copy()
     for slot in np.unique(S):
      if slot<len(slots):continue
      mask=S==slot;M=mat[slot];inv=np.linalg.inv(M);canonical[mask]=P[mask]@inv[:3,:3].T+inv[:3,3];cn[mask]=unit(N[mask]@inv[:3,:3].T)
     proxy=SimpleNamespace(vertex_slots=S,geometry_for=lambda m:SimpleNamespace(rest_positions=P,rest_normals=N,vertex_positions=canonical,vertex_normals=cn))
     m=dict(positions=P,normals=N,uv=np.asarray(UV),bones=np.full(len(P),host),faces=np.asarray(F,int))
     emit_mesh(out,'alt/'+path,m,rig,m['faces'],texture,host,proxy)

    for clip in ['gKokiriIdleAnim','gKokiriStandingArmsBehindBackAnim','gKokiriSittingCrossedArmsLegsAnim']:
     key=rig.ns+'/PoseAnimations/objects/object_os_anime/'+clip;frames=animation_frames(archive.read(key));rig.pose(frames[0 if 'Idle' in clip else len(frames)//2]);pose=np.tile(np.eye(4),(len(rig.limbs),1,1))
     for i,l in enumerate(rig.limbs):pose[i,:3,:3]=l.M;pose[i,:3,3]=l.T
     mat=pal.matrices(pose);world=np.zeros_like(g.rest_positions);wn=np.zeros_like(world)
     for slot in np.unique(pal.vertex_slots):
      mask=pal.vertex_slots==slot;M=mat[slot]
      if slot<len(slots):
       b=slots[slot];local=(g.rest_positions[mask]-bt[b])@bm[b];normal=g.rest_normals[mask]@bm[b]
      else:local=g.vertex_positions[mask];normal=g.vertex_normals[mask]
      world[mask]=local@M[:3,:3].T+M[:3,3];wn[mask]=unit(normal@M[:3,:3].T)
     P=list(world);N=list(wn);UV=list(model['uv']);S=list(pal.vertex_slots);F=[];H=[];caps=[]
     region={};infos={};removed_body=np.zeros(len(fs),bool)
     centroid_rgb=colors(model['uv'][fs].mean(1));face_pos=sp[fs].mean(1)
     body_faces=(mi[fs]==0).all(1)
     for sign in (1,-1):
      removed_body|=body_faces&(face_pos[:,0]*sign>.035)&(face_pos[:,1]>.64)&(centroid_rgb[:,0]>centroid_rgb[:,1]*1.05)&(centroid_rgb[:,1]>centroid_rgb[:,2]*1.08)
     # Find the new garment boundary after excluding the source skin flaps.
     _,source_group=np.unique(np.round(sp,5),axis=0,return_inverse=True)
     for cleanup in range(16):
      kept_body=fs[body_faces&~removed_body];bf=source_group[kept_body]
      edges=np.sort(np.vstack([bf[:,[0,1]],bf[:,[1,2]],bf[:,[2,0]]]),axis=1);edges,counts=np.unique(edges,axis=0,return_counts=True);edges=edges[counts==1]
      degree=np.bincount(edges.ravel(),minlength=int(source_group.max())+1)
      pinched=np.flatnonzero(degree>2)
      if not len(pinched):break
      addition=body_faces&~removed_body&np.isin(source_group[fs],pinched).any(1)
      if not addition.any() or not ((np.abs(face_pos[addition,0])>.025)&(face_pos[addition,1]>.62)).all():
       raise ValueError('Shoulder boundary cleanup escaped the bounded shoulder region')
      removed_body|=addition
     else:raise ValueError('The shoulder boundary cleanup did not converge')
     graph={}
     for x,y in edges:graph.setdefault(int(x),set()).add(int(y));graph.setdefault(int(y),set()).add(int(x))
     loops=[];seen=set()
     for seed in graph:
      if seed in seen:continue
      todo=[seed];group=[]
      while todo:
       node=todo.pop()
       if node in seen:continue
       seen.add(node);group.append(node);todo.extend(graph[node]-seen)
      loop=[]
      for node in group:loop.append(next(int(i) for i in kept_body.ravel() if source_group[i]==node))
      loops.append(np.array(loop))

     for side,piece in [('left',17),('right',2)]:
      prefix=side.title();arm=rig.profile[side][3];u=unit(joints[prefix+'ForeArm']-joints[prefix+'Arm']);cut=np.linalg.norm(joints[prefix+'ForeArm']-joints[prefix+'Arm'])*.62;along=(sp-joints[prefix+'Arm'])@u
      armids=np.flatnonzero(mi==piece);bodyids=np.flatnonzero(mi==0);d,near=cKDTree(sp[bodyids]).query(sp[armids]);contact=armids[d<1e-5];bodycontact=bodyids[near[d<1e-5]]
      _,keep=np.unique(np.round(sp[contact],5),axis=0,return_index=True);armcontact=contact[keep]
      sign=1 if side=='left' else -1
      bodycontact=next(loop for loop in loops if (sp[loop].mean(0)[0]*sign>.035 and sp[loop].mean(0)[1]>.64))
      contact=bodycontact
      v=unit(np.array([0.,1.,0.])-u*u[1]);w=np.cross(u,v);center=sp[contact].mean(0)
      infos[piece]=dict(side=side,arm=arm,u=u,cut=cut,along=along,contact=contact,bodycontact=bodycontact,v=v,w=w,center=center,end=[],armcontact=armcontact)
      region[piece]=(along<cut)
     for face_index,(face,host) in enumerate(zip(fs,model['hosts'])):
      if removed_body[face_index]:continue
      piece=int(mi[face[0]])
      if piece not in infos or not (mi[face]==piece).all() or not region[piece][face].any():F.append(face.tolist());H.append(int(host));continue
      info=infos[piece];value=info['along'][face]-info['cut'];poly=[]
      for k in range(3):
       a,b=int(face[k]),int(face[(k+1)%3]);va,vb=value[k],value[(k+1)%3]
       if va>=0:poly.append(a)
       if (va<0)!=(vb<0):
        t=va/(va-vb);index=len(P);P.append(world[a]*(1-t)+world[b]*t);N.append(unit(wn[a]*(1-t)+wn[b]*t));UV.append(model['uv'][a]*(1-t)+model['uv'][b]*t);S.append(slots.index(info['arm']));poly.append(index);info['end'].append((index,sp[a]*(1-t)+sp[b]*t))
      for k in range(1,len(poly)-1):F.append([poly[0],poly[k],poly[k+1]]);H.append(int(host))
     P=np.asarray(P);N=np.asarray(N);UV=np.asarray(UV);S=np.asarray(S);F=np.asarray(F,int);H=np.asarray(H)
     for piece,info in infos.items():
      arm=info['arm'];u,v,w=[info[x] for x in ('u','v','w')];i0=info['bodycontact'];p0=world[i0];n0=wn[i0];src0=sp[info['contact']]
      end=info['end'];src1=np.array([x[1] for x in end]);_,keep=np.unique(np.round(src1,5),axis=0,return_index=True);ids1=np.array([end[k][0] for k in keep]);src1=src1[keep];p1=P[ids1];n1=N[ids1]
      approximate=np.arctan2((src0-src0.mean(0))@w,(src0-src0.mean(0))@v)
      angle0=perimeter_angles(src0,source_group[i0],graph,approximate)
      angle1=np.arctan2((src1-src1.mean(0))@w,(src1-src1.mean(0))@v)
      theta=np.linspace(-np.pi,np.pi,256,endpoint=False);proximal=periodic(theta,angle0,p0)
      shifts=np.linspace(-np.pi,np.pi,256,endpoint=False)
      shift=min(shifts,key=lambda delta:np.square(proximal-periodic((theta+delta+np.pi)%(2*np.pi)-np.pi,angle1,p1)).sum())
      angle1=(angle1-shift+np.pi)%(2*np.pi)-np.pi
      c0=p0.mean(0);c1=p1.mean(0);maps=[]
      for bone in (rig.profile['chest'],arm):
       W=np.eye(4);W[:3,:3]=rig.limbs[bone].M@bm[bone].T;W[:3,3]=rig.limbs[bone].T-W[:3,:3]@bt[bone];maps.append(W@model['source_bone_transforms'][bone])
      r0=polar(maps[0][:3,:3])[0];length=np.linalg.norm(c1-c0)
      outward=r0[:,0]*(1 if info['side']=='left' else -1)
      cp=[];cu=[];cs=[];expected=[];rings=[];boundary_normals={}
      for ring,t in enumerate(np.linspace(0,1,11)):
       angles=angle0 if ring==0 else angle1 if ring==10 else np.linspace(-np.pi,np.pi,64,endpoint=False)
       order=np.argsort(angles);angles=angles[order];center=c0*(1-t)+c1*t+outward*(length*t*(1-t))
       points=periodic(angles,angle0,p0)*(1-t)+periodic(angles,angle1,p1)*t
       points+=outward*(length*t*(1-t))
       if ring==0:points=p0[order]
       if ring==10:points=p1[order]
       ringids=[]
       for k,(angle,p) in enumerate(zip(angles,points)):
        idx=len(cp);cp.append(p);cu.append([(angle+np.pi)/(2*np.pi),t]);cs.append(int(S[i0[order[k]]]) if ring==0 else blend_slot(arm,t));expected.append(unit(p-center));ringids.append(idx)
        if ring==0:boundary_normals[idx]=n0[order[k]]
        if ring==10:boundary_normals[idx]=n1[order[k]]
       rings.append((np.array(ringids),angles))
      cf=[]
      for (ia,aa),(ib,ab) in zip(rings,rings[1:]):
       a=b=0;na=len(ia);nb=len(ib)
       while a<na or b<nb:
        nexta=aa[(a+1)%na]+(2*np.pi if a+1>=na else 0) if a<na else np.inf
        nextb=ab[(b+1)%nb]+(2*np.pi if b+1>=nb else 0) if b<nb else np.inf
        if nexta<nextb:cf.append([ia[a%na],ia[(a+1)%na],ib[b%nb]]);a+=1
        else:cf.append([ia[a%na],ib[(b+1)%nb],ib[b%nb]]);b+=1
      cp=np.asarray(cp);cu=np.asarray(cu);cs=np.asarray(cs);cf=np.asarray(cf,int);expected=np.asarray(expected)
      adjacency=[set() for _ in cp]
      for a,b,c in cf:
       adjacency[a].update((b,c));adjacency[b].update((a,c));adjacency[c].update((a,b))
      free=np.flatnonzero((cu[:,1]>0)&(cu[:,1]<1))
      for _ in range(24 if 'Crossed' in clip else 0):
       average=np.array([cp[list(adjacency[i])].mean(0) for i in free])
       cp[free]+=.5*(average-cp[free])
      cross=np.cross(cp[cf[:,1]]-cp[cf[:,0]],cp[cf[:,2]]-cp[cf[:,0]])
      if (cross*expected[cf].mean(1)).sum()<0:cf=cf[:,[0,2,1]]
      cross=np.cross(cp[cf[:,1]]-cp[cf[:,0]],cp[cf[:,2]]-cp[cf[:,0]]);cn=np.zeros_like(cp)
      for k in range(3):np.add.at(cn,cf[:,k],cross)
      cn=unit(cn)
      for idx,n in boundary_normals.items():cn[idx]=n
      # The source armhole has concave, unevenly spaced corners. Filter their
      # shading through the strip without changing the verified surface.
      for _ in range(24):
       average=np.array([cn[list(adjacency[i])].mean(0) for i in free])
       cn[free]=unit(cn[free]*.5+average*.5)
      near_end=np.flatnonzero(cu[:,1]>.5)
      mix=((cu[near_end,1]-.5)*2)[:,None];mix=mix*mix*(3-2*mix)
      target=periodic(cu[near_end,0]*(2*np.pi)-np.pi,angle1,n1)
      cn[near_end]=unit(cn[near_end]*(1-mix)+target*mix)
      # Duplicate only the cylindrical UV seam, keeping every boundary position.
      cp=list(cp);cn=list(cn);cu=list(cu);cs=list(cs)
      for f in cf:
       if np.ptp(np.array(cu)[f,0])>.5:
        for k,idx in enumerate(f):
         if cu[idx][0]<.5:f[k]=len(cp);cp.append(cp[idx]);cn.append(cn[idx]);cu.append([cu[idx][0]+1,cu[idx][1]]);cs.append(cs[idx])
      x=-np.pi+(np.arange(256)+.5)/256*(2*np.pi);skin=colors(model['uv'][info['armcontact']]);valid=(skin[:,0]>skin[:,1]*1.03)&(skin[:,1]>skin[:,2]*1.08)&(skin.mean(1)>90);tone=np.median(skin[valid],axis=0);edge0=np.tile(tone,(len(x),1));edge1=edge_colors(info,src1.mean(0),x+shift,v,w,UV);h=np.linspace(0,1,128)[:,None,None];rgb=np.rint(edge0[None]*(1-h)+edge1[None]*h).clip(0,255).astype(np.uint8);rgba=np.concatenate([rgb,np.full((128,256,1),255,np.uint8)],axis=2)
      texture=key+'/Skin/Textures/ShoulderSkin'+info['side'].title();out.add('alt/'+texture,asset._header(asset.T_OTEX,1)+struct.pack('<IIIIffI',1,256,128,1,.5,.25,rgba.nbytes)+rgba.tobytes())
      path=key+'/Skin/Shoulders/'+info['side'].title();encode_geometry(cp,cn,cu,cs,cf,path,arm,texture,mat);caps.append((arm,path))
     for host in (7,8,11):
      path=key+f'/Skin/Limbs/Limb{host:02d}';surface=path+'/Surface';encode_geometry(P,N,UV,S,F[H==host],surface,host,rig.ns+'/Textures/Diffuse',mat);wrapper(out,'alt/'+path,[surface]+[p for h,p in caps if h==host])
     pose_records.append(dict(clip=clip,shoulder_fairing_iterations=24 if 'Crossed' in clip else 0,removed_source_garment_skin_faces=int(removed_body.sum()),armhole_vertices={info['side']:len(info['contact']) for info in infos.values()}))
    out.add('alt/'+rig.ns+'/Skin/Palette',pal.resource())
    protected=[n for n in archive.candidate.namelist() if '/Rig/' in n or '/Body/' in n or '/Faces/' in n or '/FaceParts/' in n or '/Hands/' in n or n.endswith('/Skel')]
    assert not set(protected).intersection(out.files),'Protected resources were changed'
    report=dict(source_sha256=SOURCE_SHA256,parent_sha256=hashlib.sha256(base.read_bytes()).hexdigest(),
                native_sha256=hashlib.file_digest(open(native,'rb'),'sha256').hexdigest(),
                method='bounded shoulder strip retopology with source-matched skin bake',
                palette_version=1,native_matrices=len(slots),blend_matrices=len(pal.blends),poses=pose_records,
                original_head_body_hands_rig_and_diffuse_resources_byte_identical=True,
                native_clips_and_private_arm_fits_byte_identical=True,runtime_tested=False)
    report['changed_resources']=sorted(out.files)
    out.add(rig.ns+'/ShoulderRepairInfo.json',json.dumps(report,indent=2).encode())
    target=Path(output)
    if target.resolve() in (Path(native).resolve(),Path(source_path).resolve(),base.resolve(),Path(fitted).resolve()):
        raise ValueError('The candidate must be separate from every input')
    target.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(target,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as dest:
        for name in sorted(set(archive.candidate.namelist())|set(out.files)):
            raw=out.files[name] if name in out.files else archive.candidate.read(name)
            info=zipfile.ZipInfo(name,(2026,10,19,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED;dest.writestr(info,raw)
    archive.candidate.close();archive.native.close()
    report['output_sha256']=hashlib.sha256(target.read_bytes()).hexdigest()
    return report

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('native','source','candidate','fitted','output'):parser.add_argument(name,type=Path)
    args=parser.parse_args()
    print(json.dumps(refine(args.native,args.source,args.candidate,args.fitted,args.output),indent=2))
