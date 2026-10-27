"""Surface fitting adapted from the accepted Fado shoulder and finger fitter."""
import numpy as np


def fit_saria_cloth(model, source, rig):
    """Keep each segmented tunic on one coherent source-to-chest surface.

    Auto weights on these torso pieces include clavicle and arm influences.
    Mixing that fit with the joint-band fit folds individual cloth triangles.
    Sleeves retain continuous arm bands. These explicit cloth pieces contain
    no face geometry; auto-rigged collar vertices must also follow the chest.
    """
    import hashlib
    from native_npc_models import unit
    key=hashlib.sha256(source.raw).hexdigest() if hasattr(source,'raw') else ''
    pieces={
        'dddeaed512e7b4aaedf960e1ddab13e74c480897e0f44404fb980c0c9ce5d63a':('tripo_part_1',),
        '902dfca93f4640a65a72e4266f855966fa708b412e0742789552f55f85c94aff':
            ('tripo_part_11','tripo_part_5','tripo_part_9','tripo_part_23'),
    }.get(key)
    if pieces is None:return
    names=[m.get('name','') for m in source.meshes]
    chosen=np.isin(model['mesh_ids'],[names.index(n) for n in pieces if n in names])
    # A detached fragment of the traditional shoulder piece belongs to the
    # sleeve, beyond the torso's actual source silhouette.
    if len(pieces)>1:chosen &= abs(model['source_positions'][:,0])<.10
    chest=rig.profile['chest'];mapping=model['source_bone_transforms'][chest]
    p=model['source_positions'];n=np.concatenate([m['normals'] for m in source.meshes])
    model['positions'][chosen]=p[chosen]@mapping[:3,:3].T+mapping[:3,3]
    model['normals'][chosen]=unit(n[chosen]@np.linalg.inv(mapping[:3,:3]))
    model['weights'][chosen]=0;model['weights'][chosen,chest]=1
    model['bones'][chosen]=chest
    attached=model.setdefault('rigid_attachments',np.full(len(p),-1,int))
    attached[chosen]=chest
    if len(pieces)>1:
        # The yoke also contains a disconnected 44-triangle source shard,
        # separated 3–11mm from the actual sleeve. It floats beside the arm
        # even with correct ownership; keep it out of the compiled surface.
        fragment=(model['mesh_ids']==names.index('tripo_part_9'))&(p[:,0]>.10)
        removed=fragment[model['faces']].all(1)
        if int(removed.sum())!=44:raise ValueError('The audited detached tunic shard changed')
        model['source_face_count']=len(model['faces'])
        model['removed_segment_faces']=np.flatnonzero(removed)
        model['faces']=model['faces'][~removed];model['hosts']=model['hosts'][~removed]
    # The closed sleeve caps are separate pieces. Their inner contact rims
    # need the same chest attachment as the tunic before blending to the arm.
    sleeve_names=('tripo_part_8','tripo_part_7') if len(pieces)==1 else ('tripo_part_10','tripo_part_7')
    named={n:source.world[j][:3,3] for n,j in zip(source.names,source.joints)}
    authored=model.setdefault('authored_joint_mask',np.zeros(len(p),bool))
    weights=model.setdefault('authored_joint_weights',np.zeros_like(model['weights']))
    for side,piece in zip(('Left','Right'),sleeve_names):
        from scipy.spatial import cKDTree
        arm=rig.profile[side.lower()][3]
        axis=unit(named[side+'ForeArm']-named[side+'Arm'])
        delta=p-named[side+'Arm'];along=delta@axis
        radial=np.linalg.norm(delta-along[:,None]*axis,axis=1)
        sleeve=model['mesh_ids']==names.index(piece)
        near=cKDTree(p[chosen]).query(p)[0]
        contact=sleeve&(abs(along)<.020)&(radial<.055)&(near<.012)
        if not contact.any():continue
        distance=cKDTree(p[contact]).query(p)[0]
        strength=np.clip(1-distance/.035,0,1);strength=strength*strength*(3-2*strength)
        ids=np.flatnonzero(sleeve&(strength>0)&(along<.085)&(along>-.07)&(radial<.09))
        t=np.clip((along[ids]+.025)/.09,0,1);t=t*t*(3-2*t)
        t*=1-strength[ids]
        ac=model['source_bone_transforms'][chest];aa=model['source_bone_transforms'][arm]
        model['positions'][ids]=(p[ids]@ac[:3,:3].T+ac[:3,3])*(1-t[:,None])+(p[ids]@aa[:3,:3].T+aa[:3,3])*t[:,None]
        model['normals'][ids]=unit((n[ids]@np.linalg.inv(ac[:3,:3]))*(1-t[:,None])+(n[ids]@np.linalg.inv(aa[:3,:3]))*t[:,None])
        model['weights'][ids]=0;model['weights'][ids,chest]=1-t;model['weights'][ids,arm]=t
        model['bones'][ids]=np.where(t<.5,chest,arm)
        authored[ids]=True;weights[ids]=model['weights'][ids]


def twin_face_pieces(model):
    """Separate face/eye/brow pieces from hair crossing the projected face."""
    p=model['source_positions'];pieces=model.get('mesh_ids',np.zeros(len(p),int))
    mouth=(abs(p[:,0])<.03)&(p[:,1]>.78)&(p[:,1]<.815)&(p[:,2]>.03)
    main=np.bincount(pieces[mouth]).argmax() if mouth.any() else 0
    allowed=[main]
    for piece in np.unique(pieces):
        q=p[pieces==piece]
        if q[:,1].min()>.815 and q[:,1].max()<.91 and abs(q[:,0]).max()<.08 and q[:,2].min()>.035:
            allowed.append(piece)
    return main,np.isin(pieces,allowed)


def smooth_twin_face(model, head, rgb):
    """Fit blink support without rewriting the neutral face's normal field."""
    p = model['source_positions']
    skin = ((model['bones'] == head) & (abs(p[:,0]) < .08) & (p[:,2] > .015) &
            (rgb[:,0] > rgb[:,1]*.94) & (rgb[:,1] > rgb[:,2]*1.08) & (rgb.mean(1) > 85))
    pieces=model.get('mesh_ids',np.zeros(len(p),int))
    main,face_piece=twin_face_pieces(model)
    ring = skin & (pieces==main) & (p[:,1] > .865) & (p[:,1] < .916)
    if np.count_nonzero(ring) < 12:
        return
    x, y = p[:,0], p[:,1]-.89
    design = np.column_stack([np.ones(len(p)),x,y,x*x,y*y,x*y])
    coef = np.linalg.lstsq(design[ring],p[ring,2],rcond=None)[0]
    model['twin_forehead_curve']=coef
    green=(rgb[:,1]>rgb[:,0]*1.05)&(rgb[:,1]>rgb[:,2]*1.2)
    band=green&(p[:,1]>.915)&(p[:,1]<.94)&(p[:,2]>.015)&(abs(p[:,0])<.085)
    band_piece=(pieces==np.bincount(pieces[band]).argmax()) if band.any() else np.zeros(len(p),bool)
    model['twin_face_piece']=face_piece
    model['twin_band_piece']=band_piece
    for side,cx in [('L',.04),('R',-.04)]:
        x,y=p[:,0]-cx,p[:,1]-.84
        radius=np.sqrt((x/.028)**2+(y/.027)**2)
        design=np.column_stack([np.ones(len(p)),x,y,x*x,y*y,x*y])
        ring=skin&(pieces==main)&(radius>1.1)&(radius<2.5)&(p[:,1]<.884)
        if ring.sum()<12:continue
        coef=np.linalg.lstsq(design[ring],p[ring,2],rcond=None)[0]
        model['twin_eye_curve_'+side]=coef


def source_hand_pose(source, curls=0., separated=False, grip=None):
    """Bake straight relaxed fingers before collapsing their joints to a hand.

    This is the Fado open-hand operation. The source's auto-rig spread was
    previously frozen into every native pose, including singing and carrying.
    """
    from native_npc_models import align, unit
    grip=grip or {}
    def finger_curl(side,finger):
        return float(grip.get(side,{}).get('curls',{}).get(finger,curls))
    named = dict(zip(source.names, source.joints))
    targets = {}
    for side in ("Left", "Right"):
        if side+"HandMiddle1" not in named:
            continue
        palm = unit(source.world[named[side+"HandMiddle1"]][:3, 3]-source.world[named[side+"Hand"]][:3, 3])
        lateral = source.world[named[side+'HandThumb2']][:3, 3]-source.world[named[side+'Hand']][:3, 3]
        lateral = unit(lateral-palm*np.dot(lateral, palm))
        for finger in ("Index", "Middle", "Ring", "Pinky", "Thumb"):
            curl=finger_curl(side,finger)
            direction = palm
            if separated and finger != 'Thumb':
                spread = {'Index': .20, 'Middle': .025, 'Ring': -.13, 'Pinky': -.26}[finger]
                direction = unit(palm+lateral*spread*(1-min(abs(curl), 1.)*.65))
            if finger == "Thumb":
                direction = unit(source.world[named[side+"HandThumb2"]][:3, 3]-source.world[named[side+"HandThumb1"]][:3, 3])
                if separated:
                    direction = unit(direction*(1-abs(curl)*.25)+palm*abs(curl)*.25)
            for k in (1, 2, 3):
                targets[side+"Hand"+finger+str(k)] = direction
    world = {}
    for index, original in source.world.items():
        parent = source.parent[index]
        local = original if parent is None else np.linalg.inv(source.world[parent])@original
        world[index] = local.copy() if parent is None else world[parent]@local
        name = source.g["nodes"][index].get("name", "").removeprefix("mixamorig:")
        if name in ('LeftHand','RightHand'):
            side=name.removesuffix('Hand')
            world[index][:3,3]+=np.asarray(grip.get(side,{}).get('offset',[0.,0.,0.]))
        if name in targets:
            child = named.get(name[:-1]+str(int(name[-1])+1))
            if child is not None:
                offset = np.linalg.inv(original)@source.world[child]
                direction = world[index][:3, :3]@offset[:3, 3]
                world[index][:3, :3] = align(direction, targets[name])@world[index][:3, :3]
                if curls or separated:
                    from scipy.spatial.transform import Rotation
                    side = "Left" if name.startswith("Left") else "Right"
                    palm = unit(source.world[named[side+"HandMiddle1"]][:3, 3]-source.world[named[side+"Hand"]][:3, 3])
                    thumb = source.world[named[side+"HandThumb2"]][:3, 3]-source.world[named[side+"Hand"]][:3, 3]
                    axis = unit(thumb-palm*np.dot(thumb, palm))*(1 if side == "Left" else -1)
                    k = int(name[-1])
                    # Cumulative curl: the parent's rotation already bent
                    # the chain, so add each joint's bend after straightening.
                    total = sum(([0, 25, 38, 28] if "Thumb" in name else [0, 48, 68, 44])[1:k+1])
                    finger = next(f for f in ('Index','Middle','Ring','Pinky','Thumb') if f in name)
                    curl=finger_curl(side,finger)
                    # Positive grip bends toward the palm for both source
                    # hands. The unseparated path formerly hyperextended
                    # Malon's fingers behind the basket handle.
                    amount = -curl
                    if separated:
                        factor = {'Index': .70, 'Middle': .85, 'Ring': 1., 'Pinky': 1.05, 'Thumb': .75}[finger]
                        relax = {'Index': .06, 'Middle': .08, 'Ring': .12, 'Pinky': .16, 'Thumb': .035}[finger]
                        # The source palm faces -Y. The old positive curl bent
                        # away from it and away from the right-hand ocarina.
                        amount = -(curl*factor+relax*(1-min(abs(curl), 1.)))
                    world[index][:3, :3] = Rotation.from_rotvec(axis*np.deg2rad(total*amount)).as_matrix()@world[index][:3, :3]
    return [world[i]@inverse for i, inverse in zip(source.joints, source.inverse)]


def cap_vertices(model, source, rig, side, part):
    """The child's main dress also contains fragments of each sleeve cap."""
    names = [m.get("name", "") for m in source.meshes]
    chosen = model["mesh_ids"] == names.index(part)
    if rig.name == "young-malon":
        skin_joints = np.concatenate([m["joints"] for m in source.meshes])
        skin_weights = np.concatenate([m["weights"] for m in source.meshes])
        targets = [i for i, n in enumerate(source.names) if n in (side+"Shoulder", side+"Arm", side+"ForeArm")]
        influence = (np.isin(skin_joints, targets)*skin_weights).sum(1)
        p = model["source_positions"]
        sign = 1 if side == "Left" else -1
        chosen |= ((model["mesh_ids"] == names.index("tripo_part_0")) &
                   (p[:, 1] > .66) & (p[:, 0]*sign > .075) & (influence > .35))
    return chosen


def fit_shoulder_caps(model, source, rig, transforms, joints):
    """Anchor the armhole to the shirt and blend the cap into the upper arm.

    Fado's fixed seam/free cap distinction is retained. These segmented arm
    pieces need a bounded anatomical blend; the locally rigid solve folded
    their thin cap rings in raised-arm poses.
    """
    from native_npc_models import unit
    if rig.name not in ("twins", "young-malon"):
        return
    names = [m.get("name", "") for m in source.meshes]
    pairs = (("Left", "tripo_part_4"), ("Right", "tripo_part_10")) if rig.name == "twins" else (("Left", "tripo_part_9"), ("Right", "tripo_part_8"))
    source_n = np.concatenate([m["normals"] for m in source.meshes])
    for side, part in pairs:
        if part not in names:
            continue
        direction = unit(joints[side+"ForeArm"]-joints[side+"Arm"])
        distance = (model["source_positions"]-joints[side+"Arm"])@direction
        mask = cap_vertices(model, source, rig, side, part) & (distance < .085)
        blend = np.clip((distance[mask]+.025)/.09, 0, 1)
        blend = blend*blend*(3-2*blend)
        ac, tc = transforms["Hips"]; aa, ta = transforms[side+"Arm"]
        points = model["source_positions"][mask]
        model["positions"][mask] = ((points@ac.T+tc)*(1-blend[:, None])+
                                    (points@aa.T+ta)*blend[:, None])
        model["normals"][mask] = unit((source_n[mask]@np.linalg.inv(ac))*(1-blend[:, None])+
                                       (source_n[mask]@np.linalg.inv(aa))*blend[:, None])
        arm = rig.profile["left" if side == "Left" else "right"][3]
        model["bones"][mask] = np.where(blend < .5, rig.profile["chest"], arm)
        model["weights"][mask] = 0
        model["weights"][mask, rig.profile["chest"]] = 1-blend
        model["weights"][mask, arm] = blend


def weld_hair_normals(model, head, neck_y, rgb):
    """Share shading across duplicate red hair UV/segmentation boundaries."""
    from native_npc_models import unit
    p = model["source_positions"]
    hair = ((model["bones"] == head) & (p[:, 1] > neck_y+.06) &
            (rgb[:, 0] > rgb[:, 1]*1.3) & (rgb[:, 0] > rgb[:, 2]*1.5))
    _, group = np.unique(np.round(p, 6), axis=0, return_inverse=True)
    total = np.zeros((group.max()+1, 3)); count = np.zeros(len(total), int)
    np.add.at(total, group[hair], model["normals"][hair])
    np.add.at(count, group[hair], 1)
    selected = hair & (count[group] > 1)
    model["normals"][selected] = unit(total[group[selected]])
    # The top hair cap has separate Tripo pieces with almost-matching rings.
    # Join only their high boundary vertices; fringe and facial geometry stay
    # outside this region. The same head matrix owns every repaired point.
    from scipy.spatial import cKDTree
    seam = np.flatnonzero(hair & model["boundary"] & (p[:, 1] > .94))
    if len(seam):
        tree = cKDTree(p[seam])
        original = model["positions"].copy()
        normals = model["normals"].copy()
        for local, ids in enumerate(tree.query_ball_point(p[seam], .0018)):
            ids = seam[ids]
            if len(np.unique(model["mesh_ids"][ids])) < 2:
                continue
            model["positions"][seam[local]] = original[ids].mean(0)
            model["normals"][seam[local]] = unit(normals[ids].sum(0))
