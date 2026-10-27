"""Anatomical arm anchors and garment clearance for the two supplied Malons."""
import hashlib
import numpy as np

CHILD_SOURCE='3160899f85770bcd86f664ed73fd7d12277eea2905f42933e79be150e0d2cd3b'


def basket_hand_matrix(matrix, forward, side):
    """Keep the native grip direction while rolling the knuckles outward."""
    from native_npc_models import unit
    long=matrix[:,0]
    dorsal=unit(forward-long*np.dot(forward,long))
    z=dorsal*(1 if side=='left' else -1)
    return np.column_stack([long,unit(np.cross(z,long)),z])


def level_sole(matrix, local_sole, local_forward):
    """Level the sole while retaining the toe's projected world heading.

    Return the new rotation and ankle height for ground contact. Rotating
    only about a raised ankle would leave the replacement boot floating.
    """
    from native_npc_models import unit
    points=np.asarray(local_sole)@matrix.T
    normal=np.linalg.svd(points-points.mean(0),full_matrices=False)[2][-1]
    if normal[1]<0:normal=-normal
    forward=unit(matrix@local_forward)
    old_z=unit(forward-normal*np.dot(forward,normal))
    old_x=unit(np.cross(normal,old_z))
    up=np.array([0.,1.,0.]);new_z=unit(forward-up*np.dot(forward,up))
    new_x=unit(np.cross(up,new_z))
    rotation=np.column_stack([new_x,up,new_z])@np.column_stack([old_x,normal,old_z]).T@matrix
    height=-float((np.asarray(local_sole)@rotation.T)[:,1].min())
    return rotation,height


def child_boot_transform(source,rig,side,basis,scale,ankle,source_ankle):
    """Remove the small body-frame pitch from the child's planted boots."""
    if rig.name!='young-malon':return None
    from native_npc_models import unit
    p=np.concatenate([m['positions'] for m in source.meshes])
    sign=1 if side=='Left' else -1
    sole=p[(p[:,1]<p[:,1].min()+.003)&(p[:,0]*sign>0)]
    if len(sole)<8:raise ValueError('Child boot has no reliable sole surface')
    # Use the same heading-preserving fit as the basket stance, in source
    # space. The boot's ankle translation stays shared with its skin map.
    rotation,height=level_sole(basis,(sole-source_ankle)*scale,np.array([0.,0.,1.]))
    a=rotation*scale;t=ankle-a@source_ankle
    t[1]+=height-ankle[1]
    return a,t


def plant_basket_feet(model,rig,frame,parent,bind_m,bind_t):
    """Fit the static basket stance to the replacement's longer boots."""
    from native_npc_pose_fit import elbow_point,set_global
    from native_npc_models import unit
    p=model['source_positions'];report={}
    for side,sign in [('left',1),('right',-1)]:
        thigh,knee,foot=rig.profile[side][:3]
        sole=(p[:,1]<p[:,1].min()+.003)&(p[:,0]*sign>0)
        if not np.all(model['bones'][sole]==foot):raise ValueError('Sole is not rigidly attached to its foot')
        local=(model['positions'][sole]-bind_t[foot])@bind_m[foot]
        local_forward=bind_m[foot].T@unit(model['source_bone_transforms'][foot,:3,2])
        old_matrix=rig.limbs[foot].M.copy()
        matrix,height=level_sole(old_matrix,local,local_forward)
        hip,knee_point,ankle=[rig.limbs[i].T.copy() for i in (thigh,knee,foot)]
        target=ankle.copy();target[1]=height
        bend=knee_point-(hip+ankle)/2
        new_knee,new_ankle=elbow_point(hip,knee_point,ankle,target,pole=bend)
        from native_npc_models import align
        set_global(rig,frame,thigh,align(knee_point-hip,new_knee-hip)@rig.limbs[thigh].M,parent)
        k,a=[rig.limbs[i].T.copy() for i in (knee,foot)]
        set_global(rig,frame,knee,align(a-k,new_ankle-k)@rig.limbs[knee].M,parent)
        set_global(rig,frame,foot,matrix,parent)
        posed=local@rig.limbs[foot].M.T+rig.limbs[foot].T
        before=old_matrix@local_forward;after=rig.limbs[foot].M@local_forward
        yaw_error=np.arctan2(before[0],before[2])-np.arctan2(after[0],after[2])
        yaw_error=(yaw_error+np.pi)%(2*np.pi)-np.pi
        report[side]=dict(ankle_error=float(np.linalg.norm(rig.limbs[foot].T-target)),
                          sole_min_y=float(posed[:,1].min()),sole_y_span=float(np.ptp(posed[:,1])),
                          toe_heading_error_degrees=float(np.rad2deg(yaw_error)))
    return report


def anatomical_joints(source, rig, points):
    """Place the child's shoulder hinge at its actual sleeve/torso junction.

    The automatic skeleton starts each arm near the sternum. Lowering that
    arm rotates the entire upper sleeve into the dress. These anchors are
    authored on the supplied source surface, without scaling the body.
    """
    points={k:v.copy() for k,v in points.items()}
    if rig.name=='young-malon' and hasattr(source,'raw') and hashlib.sha256(source.raw).hexdigest()==CHILD_SOURCE:
        for side,sign in [('Left',1),('Right',-1)]:
            points[side+'Arm'][0]=sign*.088
    return points


def reachable_wrist(shoulder,target,length):
    """Keep lateral/front cloth clearance, raising an unreachable low wrist."""
    out=np.asarray(target,float).copy();delta=out-shoulder
    horizontal=np.linalg.norm(delta[[0,2]])
    if horizontal>=length:
        delta[[0,2]]*=length*.995/horizontal
        out[[0,2]]=shoulder[[0,2]]+delta[[0,2]]
    vertical=np.sqrt(max(length*length-np.dot(delta[[0,2]],delta[[0,2]]),0.))
    out[1]=np.clip(out[1],shoulder[1]-vertical,shoulder[1]+vertical)
    return out


def garment_points(source,model,rig,bind_m,bind_t):
    names=[m.get('name','') for m in source.meshes]
    parts=['tripo_part_0'] if rig.name=='young-malon' else ['tripo_part_0','tripo_part_5']
    chosen=np.isin(model['mesh_ids'],[names.index(n) for n in parts if n in names])
    if rig.name=='young-malon':chosen &= model['source_positions'][:,1]<.66
    result=np.zeros((int(chosen.sum()),3));p=model['positions'][chosen];bones=model['bones'][chosen]
    for b in np.unique(bones):
        ids=bones==b;limb=rig.limbs[int(b)]
        result[ids]=(p[ids]-bind_t[b])@bind_m[b]@limb.M.T+limb.T
    return result


def child_idle_pose(shoulder,length,cloth,hand_vertices,forward,scale,side='left'):
    """Hang the arm beside the dress, with fingertips down and palm inward.

    Choose the nearest outside of the dress cross-section. Always clearing
    its front instead holds the arms forward and shortens their silhouette.
    """
    from native_npc_models import unit
    from scipy.spatial import ConvexHull
    down=np.array([0.,-1.,0.])
    forward=unit(np.array([forward[0],0.,forward[2]]))
    matrix=np.column_stack([down,-forward,unit(np.cross(down,-forward))])
    outward=matrix[:,2]*(1 if side=='left' else -1)
    clearance=unit(outward-forward*.39)[[0,2]]
    offset=hand_vertices@matrix.T
    reach=length*.9995
    target=np.asarray(shoulder,float).copy();target[1]-=reach
    for _ in range(5):
        lo,hi=target[1]+offset[:,1].min()-25,target[1]+offset[:,1].max()+25
        band=(cloth[:,1]>=lo)&(cloth[:,1]<=hi)
        if band.sum()>3:
            hull=ConvexHull(cloth[band][:,[0,2]])
            normals=hull.equations[:,:2];constants=hull.equations[:,2]
            projection=normals@clearance
            keep=projection>.05
            normals,constants=normals[keep],constants[keep]
            projection=projection[keep]
            hand_min=(offset[:,[0,2]]@normals.T).min(0)
            gap=np.maximum(.006*scale-(normals@shoulder[[0,2]]+hand_min+constants),0.)
            # Move along one stable side/back direction. Choosing a fresh
            # nearest face normal each frame makes the wrist pop sideways
            # when two almost equally near cloth facets exchange order.
            distance=float(np.min(gap/projection))
            target[[0,2]]=shoulder[[0,2]]+clearance*distance
        horizontal=np.linalg.norm((target-shoulder)[[0,2]])
        if horizontal>=reach:raise ValueError('A hanging child arm cannot clear the supplied dress within its reach')
        target[1]=shoulder[1]-np.sqrt(reach*reach-horizontal*horizontal)
    pole=target-shoulder;pole[1]=0.
    return target,matrix,unit(pole) if np.linalg.norm(pole)>1e-8 else forward


def child_idle_profile(source,model,rig,frames,bind_m,bind_t,hand_vertices):
    """One body-relative resting pose, clear of the full idle cloth envelope.

    A new vertex entering a per-frame height slice must not change the hand
    seat abruptly. Fit once to the entire loop, then follow the chest motion.
    """
    from native_npc_pose_fit import body_frame
    chest=rig.profile['chest'];rig.pose(frames[0])
    base_m=rig.limbs[chest].M.copy();base_t=rig.limbs[chest].T.copy()
    cloud=[]
    for frame in frames:
        rig.pose(frame);c=rig.limbs[chest]
        cloth=garment_points(source,model,rig,bind_m,bind_t)
        cloud.append((cloth-c.T)@c.M@base_m.T+base_t)
    rig.pose(frames[0]);cloth=np.concatenate(cloud);fr,_,_=body_frame(rig)
    result={}
    for side in ('left','right'):
        arm,fore,hand=rig.profile[side][3:]
        s,e,w=[rig.limbs[i].T.copy() for i in (arm,fore,hand)]
        target,matrix,pole=child_idle_pose(s,np.linalg.norm(e-s)+np.linalg.norm(w-e),cloth,
                                          hand_vertices[side],fr[:,2],model['scale'],side)
        result[side]=(base_m.T@(target-s),base_m.T@matrix,base_m.T@pole)
    return result


def clear_arm_targets(source,model,rig,reference,name,bind_m,bind_t,targets,matrices,hand_vertices,idle_profile=None):
    """Solve wrist reach and native bend planes together, before arm IK."""
    if rig.name not in ('young-malon','malon'):return {}
    from native_npc_models import unit
    from native_npc_pose_fit import body_frame
    fr,_,_=body_frame(rig);nr,_,_=body_frame(reference);rotation=fr@nr.T
    cloth=None
    if name in ('gMalonChildIdleAnim','gMalonAdultStandStillAnim'):
        cloth=garment_points(source,model,rig,bind_m,bind_t)
    poles={}
    for side,sign in [('left',1),('right',-1)]:
        arm,fore,hand=rig.profile[side][3:]
        ns,ne,nw=[reference.limbs[i].T for i in (arm,fore,hand)]
        # The native bend residual is independent of the wrist translation.
        # Reusing E-S after lateral clearance instead reverses this plane.
        poles[side]=rotation@(ne-(ns+nw)/2)
        offset=hand_vertices[side]@matrices[side].T
        if name=='gMalonChildIdleAnim':
            s,e,w=[rig.limbs[i].T.copy() for i in (arm,fore,hand)]
            if idle_profile is None:
                length=np.linalg.norm(e-s)+np.linalg.norm(w-e)
                targets[side],matrices[side],poles[side]=child_idle_pose(
                    s,length,cloth,hand_vertices[side],fr[:,2],model['scale'],side)
            else:
                delta,matrix,pole=idle_profile[side];chest=rig.limbs[rig.profile['chest']].M
                targets[side]=s+chest@delta
                matrices[side]=chest@matrix;poles[side]=chest@pole
        elif name=='gMalonAdultStandStillAnim':
            forward=fr[:,2]
            lo,hi=targets[side][1]-.06*model['scale'],targets[side][1]+.06*model['scale']
            band=(cloth[:,1]>=lo)&(cloth[:,1]<=hi)
            if band.any():
                front=np.quantile(cloth[band]@forward,.995)
                minimum=np.min(offset@forward)
                targets[side]+=forward*max(0.,front-minimum+.012*model['scale']-np.dot(targets[side],forward))
            poles[side]=unit(fr@np.array([sign,0.,.65]))
        if name in ('gMalonChildRaiseHandsAnim','gMalonAdultStandStillAnim'):
            s,e,w=[rig.limbs[i].T for i in (arm,fore,hand)]
            reach=(np.linalg.norm(e-s)+np.linalg.norm(w-e))*.97
            targets[side]=reachable_wrist(s,targets[side],reach)
    return poles
