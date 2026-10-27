"""Source-bounded shoulder attachments and connected crossed-arm fitting."""
import hashlib
import numpy as np
from scipy.spatial import cKDTree

SOURCE_SHA256='1e02c29c4af7920b2c518f9f63f18858920709f1406d8c00a7e24f5f81f1102e'

def smooth(x):
    x=np.clip(x,0.,1.)
    return x*x*(3-2*x)

def shoulder_blend(points,center,end,contact):
    axis=end-center;axis=axis/np.linalg.norm(axis)
    along=(points-center)@axis
    amount=smooth((along+.025)/.055)
    distance=cKDTree(contact).query(points)[0]
    return amount*(1-smooth(1-distance/.018))

def repair_shoulders(source,model,rig):
    if rig.name!='twins' or hashlib.sha256(getattr(source,'raw',b'')).hexdigest()!=SOURCE_SHA256:return None
    from native_npc_models import unit
    from native_npc_surface import _filter_normals
    p=model['source_positions'];names=[m['name'] for m in source.meshes]
    joints={n:source.world[i][:3,3] for n,i in zip(source.names,source.joints)}
    torso=model['mesh_ids']==names.index('tripo_part_0');chest=rig.profile['chest']
    authored=model.setdefault('authored_joint_mask',np.zeros(len(p),bool))
    weights=model.setdefault('authored_joint_weights',np.zeros_like(model['weights']))
    source_normals=model['posed_source_normals'].copy();strength=np.zeros(len(p))
    image=np.asarray(source.image);uv=model['uv']
    rgb=image[np.clip((uv[:,1]*image.shape[0]).astype(int),0,image.shape[0]-1),
              np.clip((uv[:,0]*image.shape[1]).astype(int),0,image.shape[1]-1),:3].astype(float)
    body_skin=torso&(rgb[:,0]>rgb[:,1]*1.02)&(rgb[:,1]>rgb[:,2]*1.08)&(rgb.mean(1)>90)&(p[:,1]>.65)
    report=[]
    for side,piece in [('left','tripo_part_4'),('right','tripo_part_10')]:
        prefix=side.title();arm,fore=rig.profile[side][3:5]
        center,end=joints[prefix+'Arm'],joints[prefix+'ForeArm']
        axis=unit(end-center);along=(p-center)@axis
        length=np.linalg.norm(end-center)
        fore_length=np.linalg.norm(joints[prefix+'Hand']-end)
        arm_piece=model['mesh_ids']==names.index(piece)
        sign=1 if side=='left' else -1
        cap=(arm_piece|(body_skin&(p[:,0]*sign>.04)))&(along<length+fore_length*.55)
        distance=cKDTree(p[torso]).query(p)[0]
        contact=arm_piece&(along<.085)&(distance<.002)
        if not contact.any():raise ValueError('Twins armhole contact ring is missing')
        t=shoulder_blend(p[cap],center,end,p[contact])
        ids=np.flatnonzero(cap)
        weights[ids]=0;weights[ids,chest]=1-t;weights[ids,arm]=t
        elbow=smooth((along[ids]-length+.030)/.060)
        lower=elbow>0
        weights[ids[lower]]=0;weights[ids[lower],arm]=1-elbow[lower];weights[ids[lower],fore]=elbow[lower]
        authored[ids]=True
        # Keep the sewn copies together before the isolated strip refiner.
        rim=torso&(cKDTree(p[contact]).query(p)[0]<.002)
        weights[rim]=0;weights[rim,chest]=1;authored[rim]=True
        model['weights'][ids]=weights[ids];model['bones'][ids]=weights[ids].argmax(1)
        strength[ids]=.65*smooth((.130-along[ids])/.080)
        report.append(dict(side=side,cap_vertices=len(ids),fixed_contact_vertices=int(contact.sum()),torso_rim_vertices=int(rim.sum())))
    filtered=_filter_normals(p,source_normals,strength)
    model['posed_source_normals'][strength>0]=filtered[strength>0]
    ids=np.flatnonzero(authored)
    position=np.zeros((len(ids),3));normal=np.zeros_like(position)
    for bone in np.flatnonzero((weights[ids]>0).any(0)):
        m=model['source_bone_transforms'][bone]
        position+=(p[ids]@m[:3,:3].T+m[:3,3])*weights[ids,bone,None]
        normal+=(filtered[ids]@np.linalg.inv(m[:3,:3]))*weights[ids,bone,None]
    model['positions'][ids]=position;model['normals'][ids]=unit(normal)
    return dict(source_sha256=SOURCE_SHA256,armholes=report,current_frame_skin_blending=True)
