"""Model-specific arm retargeting stored as private native animation assets."""
import warnings
import hashlib
import json
from pathlib import Path
import numpy as np
from scipy.spatial.transform import Rotation

from native_npc_models import NativeRig, animation_frames, align, unit
from edit_native_npc_pose import encode_animation

CLIPS = {
    "saria": ["gSariaWaitArmsToSideAnim", "gSariaPlayingOcarinaAnim", "gSariaOcarinaToMouthAnim", "gSariaStopPlayingOcarinaAnim",
              "gSariaReturnToOcarinaAnim", "gSariaSitting1Anim", "gSariaSitting2Anim", "gSariaSitting3Anim",
              "gSariaHandsBehindBackWaitAnim", "gSariaTransitionHandsSideToBackAnim"],
    "young-malon": ["gMalonChildIdleAnim", "gMalonChildRaiseHandsAnim", "gMalonChildSingAnim"],
    "malon": ["gMalonAdultIdleAnim", "gMalonAdultStandStillAnim", "gMalonAdultSingAnim"],
    "twins": ["gKokiriIdleAnim", "gKokiriStandingArmsBehindBackAnim", "gKokiriSittingCrossedArmsLegsAnim"],
}


def hand_grip(source, variant):
    path=Path(__file__).with_name('grip_art')/(variant+'.json')
    if not path.exists():return None
    spec=json.loads(path.read_text())
    if spec['source_sha256']!=hashlib.sha256(source.raw).hexdigest():
        raise ValueError('Hand contact profile belongs to a different source model')
    if spec.get('prop_scale')!=.8:
        raise ValueError('Hand contact profile uses a different ocarina scale')
    return spec['hands']


def waiting_hand_profile(source, variant):
    path = Path(__file__).with_name('pose_art')/(variant+'-waiting.json')
    if not path.exists():
        return None
    spec = json.loads(path.read_text())
    if spec['source_sha256'] != hashlib.sha256(source.raw).hexdigest():
        raise ValueError('Waiting hand profile belongs to a different source model')
    return spec


def apply_waiting_hand_fit(rig, frame, spec, amount=1.):
    """Seat the complete left arm; roll its thumb away from the other sleeve."""
    side = spec['hand']
    hand = rig.profile[side][-1]
    rig.pose(frame)
    target = rig.limbs[hand].T+body_frame(rig)[0]@np.asarray(spec['body_offset'])*amount
    rotation = Rotation.from_euler('xyz', np.asarray(spec['local_rotation_degrees'])*amount, degrees=True).as_matrix()
    matrix = rig.limbs[hand].M@rotation
    parent = parents(rig)
    error = solve_arm(rig, frame, side, target, matrix, parent)
    angle = float(spec.get('elbow_swivel_degrees', 0.))*amount
    if angle:
        arm, fore, hand = rig.profile[side][-3:]
        shoulder, elbow, wrist = [rig.limbs[i].T.copy() for i in (arm, fore, hand)]
        # The wrist stays seated while the two-link elbow circle routes the
        # sleeve around the opposite hand instead of sweeping through it.
        turn = Rotation.from_rotvec(unit(wrist-shoulder)*np.radians(angle)).as_matrix()
        seated_elbow = shoulder+turn@(elbow-shoulder)
        set_global(rig, frame, arm, align(elbow-shoulder, seated_elbow-shoulder)@rig.limbs[arm].M, parent)
        elbow, current = [rig.limbs[i].T.copy() for i in (fore, hand)]
        set_global(rig, frame, fore, align(current-elbow, wrist-elbow)@rig.limbs[fore].M, parent)
        set_global(rig, frame, hand, matrix, parent)
        error = float(np.linalg.norm(rig.limbs[hand].T-target))
    return error



def seat_wrist_targets(targets, matrices, hands, mouth_blend=1.):
    """Move each complete arm to the authored hand-local grip seat.

    This translation belongs to the IK wrist target, never the hand mesh.
    The forearm can then reach the same wrist without stretching its surface.
    """
    for side in ('left', 'right'):
        settings = (hands or {}).get(side.title(), {})
        offset = np.asarray(settings.get('wrist_offset', [0., 0., 0.]), float)
        start = float(settings.get('approach_start', 0.))
        end = float(settings.get('approach_end', 1.))
        if not 0 <= start < end <= 1:
            raise ValueError('Wrist approach interval must satisfy 0 <= start < end <= 1')
        strength = np.clip((end-mouth_blend)/(end-start), 0., 1.)
        strength = strength*strength*(3-2*strength)
        offset = offset+np.asarray(settings.get('approach_wrist_offset', [0., 0., 0.]))*strength
        targets[side] += matrices[side]@offset


def seat_ocarina(vertices, hands):
    """Compensate the right wrist seat to retain the prop's world target.

    Achieved transforms still have the arm solver's reach/quantization error.
    """
    offset = np.asarray((hands or {}).get('Right', {}).get('wrist_offset', [0., 0., 0.]))
    return np.asarray(vertices, float)-offset


def support_carried_ocarina(targets, body, hands, mouth_blend):
    """Keep the lowered instrument and both hands above the custom knees.

    An authored body-space carry offset fades out early in the mouth blend;
    playing contact and finger/wrist geometry are unaffected.
    """
    settings = (hands or {}).get('Right', {})
    end = float(settings.get('carried_support_end', .25))
    if not 0 < end <= 1:
        raise ValueError('Carried ocarina support end must be in (0,1]')
    strength = np.clip(1-mouth_blend/end, 0., 1.)
    strength = strength*strength*(3-2*strength)
    offset = body@np.asarray(settings.get('carried_offset', [0., 0., 0.]))*strength
    for side in targets:
        targets[side] += offset


def bind_ocarina_texture(raw, original, replacement):
    """Change only the one native texture reference in a private prop list."""
    import struct
    from native_npc_models import asset
    data = bytearray(raw)
    old, new = asset.crc64(original), asset.crc64(replacement)
    offset, count = 72, 0
    while offset+8 <= len(data):
        first = struct.unpack_from('<I', data, offset)[0]
        op = first >> 24
        size = 16 if op in (0x20, 0x31, 0x32, 0x33, 0x35, 0x36) else 8
        if op == 0x20 and struct.unpack_from('<II', data, offset+8) == (old >> 32, old & 0xffffffff):
            struct.pack_into('<II', data, offset+8, new >> 32, new & 0xffffffff)
            count += 1
        offset += size
    if count != 1:
        raise ValueError('Expected exactly one texture reference to the native ocarina')
    return bytes(data)


def ocarina_hd_resource(image_path):
    """Raw RGBA32 at 8x pixels with the native RGBA16 tile byte stride.

    TEX_FLAG_LOAD_AS_RAW uploads four bytes per texel. Its horizontal scale
    counts bytes against the native two-byte tile, hence 16 rather than 8.
    """
    import struct
    from PIL import Image
    from native_npc_models import asset
    pixels = np.asarray(Image.open(image_path).convert('RGBA'), dtype=np.uint8)
    if pixels.shape != (128, 256, 4):
        raise ValueError('Expected a 256x128 Fairy Ocarina atlas')
    return (asset._header(asset.T_OTEX, 1)+
            struct.pack('<IIIIffI', 1, 256, 128, 1, 16., 8., pixels.nbytes)+pixels.tobytes())


def export_ocarina(archive, native, namespace, hands):
    """Keep the shared prop unchanged; seat a second prop for grip clips only."""
    import struct
    from native_npc_models import asset
    prop = native.read('objects/object_sa/gSariaRightHandAndOcarinaDL')
    if len(prop) != 696 or prop[384:392] != struct.pack('<II', 0xE7000000, 0):
        raise ValueError('Native Saria prop layout differs from the audited source')
    original = asset.read_vertex_array(native.read('objects/object_sa/object_saVtx_006948'))[:20]
    art = Path(__file__).with_name('prop_art')/'saria-fairy-ocarina.png'
    texture = namespace+'/Props/Textures/FairyOcarinaHD'
    if art.exists():
        archive.add('alt/'+texture, ocarina_hd_resource(art))
    for suffix, seat in (('', None), ('Grip', hands)):
        path = namespace+'/Props/Ocarina'+suffix
        vertex_path = path+'Vertices'
        xyz = seat_ocarina(np.array([row[:3] for row in original])*.8, seat)
        records = [(*[round(v) for v in point], *row[3:]) for row, point in zip(original, xyz)]
        archive.add('alt/'+vertex_path, asset.build_vertex_array(records))
        d = asset.DLBuilder()
        d.marker(path)
        offset = 88
        while offset < 384:
            first, second = struct.unpack_from('<II', prop, offset)
            size = 16 if first >> 24 in (0x20, 0x31, 0x32, 0x33) else 8
            if first >> 24 == 0x32:
                hashed = asset.crc64(vertex_path)
                d.words.extend([first, second, hashed >> 32, hashed & 0xFFFFFFFF])
            else:
                d.words.extend(struct.unpack_from(f'<{size//4}I', prop, offset))
            offset += size
        d.end()
        raw = d.to_resource()
        if art.exists():
            raw = bind_ocarina_texture(raw, 'objects/object_sa/gSariaFairyOcarinaTex', texture)
        archive.add('alt/'+path, raw)


def elbow_point(shoulder, elbow, wrist, target, pole=None):
    """Two-link IK with the native elbow's bend plane as its pole."""
    a, b = np.linalg.norm(elbow-shoulder), np.linalg.norm(wrist-elbow)
    vector = target-shoulder
    length = np.clip(np.linalg.norm(vector), abs(a-b)+.01, a+b-.01)
    direction = unit(vector)
    bend = elbow-shoulder if pole is None else np.asarray(pole,float)
    pole = bend-direction*np.dot(bend, direction)
    if np.linalg.norm(pole) < 1e-5:
        axis = np.eye(3)[np.argmin(abs(direction))]
        pole = axis-direction*np.dot(axis, direction)
    along = (a*a-b*b+length*length)/(2*length)
    height = np.sqrt(max(a*a-along*along, 0))
    return shoulder+direction*along+unit(pole)*height, shoulder+direction*length


def parents(rig):
    out = {}
    def walk(i, parent):
        while i != 255:
            out[i] = parent
            if rig.limbs[i].child != 255:
                walk(rig.limbs[i].child, i)
            i = rig.limbs[i].sibling
    walk(0, None)
    return out


def set_global(rig, frame, index, matrix, parent):
    local = rig.limbs[parent[index]].M.T@matrix
    with warnings.catch_warnings():
        warnings.simplefilter("ignore", UserWarning)
        angles = Rotation.from_matrix(local).as_euler("xyz")
    frame[index+1] = (np.rint(angles*(65536/(2*np.pi))).astype(int)+32768)%65536-32768
    rig.pose(frame)


def solve_arm(rig, frame, side, wrist_target, hand_matrix, parent, pole=None):
    arm, fore, hand = rig.profile[side][3:]
    shoulder, elbow, wrist = [rig.limbs[i].T.copy() for i in (arm, fore, hand)]
    target_elbow, target_wrist = elbow_point(shoulder, elbow, wrist, wrist_target,pole=pole)
    set_global(rig, frame, arm, align(elbow-shoulder, target_elbow-shoulder)@rig.limbs[arm].M, parent)
    elbow, wrist = rig.limbs[fore].T.copy(), rig.limbs[hand].T.copy()
    set_global(rig, frame, fore, align(wrist-elbow, target_wrist-elbow)@rig.limbs[fore].M, parent)
    set_global(rig, frame, hand, hand_matrix, parent)
    return float(np.linalg.norm(rig.limbs[hand].T-wrist_target))


def fit_saria_standing_arms(rig, frame):
    """Let forearms and fingers hang down; preserve elbows and skin bindings.

    Only the elbow and wrist rotations change. Rotating complete limbs keeps
    each hand attached, with the original palm roll and all upper-arm motion.
    """
    parent = parents(rig)
    down = np.array([0., -1., 0.])
    rig.pose(frame)
    for side in ('left', 'right'):
        fore, hand = rig.profile[side][-2:]
        fingers = rig.limbs[hand].M.copy()
        direction = rig.limbs[hand].T-rig.limbs[fore].T
        set_global(rig, frame, fore, align(direction, down)@rig.limbs[fore].M, parent)
        set_global(rig, frame, hand, align(fingers[:, 0], down)@fingers, parent)


def body_frame(rig):
    p = rig.profile
    hip, head = rig.limbs[p["hips"]].T, rig.limbs[p["head"]].T
    left, right = rig.limbs[p["left"][3]].T, rig.limbs[p["right"][3]].T
    x = unit(left-right); z = unit(np.cross(x, head-hip)); y = unit(np.cross(z, x))
    return np.column_stack([x, y, z]), np.linalg.norm(left-right), np.linalg.norm(head-hip)


def export_poses(archive, native, source, model, rig, variant):
    """Preserve native timing/root; fit arms and the audited basket stance."""
    if rig.name not in CLIPS:
        return {}
    reference = NativeRig(native, rig.name)
    parent = parents(rig)
    bind_m = np.array([l.M.copy() for l in rig.limbs])
    bind_t = np.array([l.T.copy() for l in rig.limbs])
    source_points = model["source_positions"]
    from native_npc_face import LANDMARKS
    mouth_y = LANDMARKS[variant][4]
    mouth_area = (abs(source_points[:, 0]) < .008) & (abs(source_points[:, 1]-mouth_y) < .008) & (source_points[:, 2] > .015)
    mouth_z = np.median(source_points[mouth_area, 2])
    ht = model["head_transform"]
    mouth_bind = ht[:, :3]@np.array([0., mouth_y, mouth_z])+ht[:, 3]
    head = rig.profile["head"]
    mouth_local = bind_m[head].T@(mouth_bind-bind_t[head])
    ob = rig.profile.get("animation_object", rig.profile["object"])
    result = {}
    waiting_fit = waiting_hand_profile(source, variant) if source is not None else None
    native_palms, custom_palms = {}, {}
    hand_vertices = {}
    if source is not None:
        from verify_native_npc_models import Archive
        # The archive decoder only needs native resources for this control.
        class NativeGeometry(Archive):
            def read(self, path):
                return native.read(path)
        control = NativeGeometry.__new__(NativeGeometry)
        from native_npc_models import asset
        control.hashes = {asset.crc64(n): n for n in native.namelist()}
        control.vertices, control.textures = {}, {}
        for side in ("left", "right"):
            hand = rig.profile[side][-1]
            data = control.geometry(reference.near[hand], reference, hand)
            native_palms[side] = data["local"][data["bones"] == hand].mean(0)
            ids = model["bones"] == hand
            custom_palms[side] = ((model["positions"][ids]-bind_t[hand])@bind_m[hand]).mean(0)
            hand_vertices[side] = (model["positions"][ids]-bind_t[hand])@bind_m[hand]
    relaxed_palms = {side: value.copy() for side, value in custom_palms.items()}
    for name in CLIPS[rig.name]:
        custom_palms = {side: value.copy() for side, value in relaxed_palms.items()}
        active_hand_vertices={side:value.copy() for side,value in hand_vertices.items()}
        path = f"objects/{ob}/{name}"
        if path not in native.namelist():
            continue
        original = animation_frames(native.read(path)); fitted = original.copy()
        grip = None
        contact = None
        curls = (1.05 if name == 'gKokiriSittingCrossedArmsLegsAnim' else
                 .85 if rig.name == 'saria' and 'Ocarina' in name else
                 .85 if name == "gMalonAdultStandStillAnim" else
                 .12 if name == 'gMalonChildIdleAnim' else
                 .04 if rig.name in ('young-malon','malon') and 'SingAnim' in name else
                 -.08 if "SingAnim" in name else 0.)
        if curls and source is not None:
            from native_npc_models import fit_model
            contact=hand_grip(source,variant) if rig.name=='saria' and 'Ocarina' in name else None
            if name=='gMalonChildIdleAnim':
                contact={side:dict(curls={'Index':.10,'Middle':.12,'Ring':.14,'Pinky':.16,'Thumb':.08})
                         for side in ('Left','Right')}
            grip = fit_model(source, NativeRig(native, rig.name), hand_curls=curls,hand_grip=contact)
            grip['texture_path']=model.get('texture_path',rig.ns+'/Textures/Diffuse')
            for side in ("left", "right"):
                hand = rig.profile[side][-1]; ids = grip["bones"] == hand
                custom_palms[side] = ((grip["positions"][ids]-bind_t[hand])@bind_m[hand]).mean(0)
                active_hand_vertices[side]=(grip['positions'][ids]-bind_t[hand])@bind_m[hand]
        idle_profile=None
        if name=='gMalonChildIdleAnim' and source is not None:
            from native_npc_malon import child_idle_profile
            idle_profile=child_idle_profile(source,model,rig,original,bind_m,bind_t,active_hand_vertices)
        errors = [];stance=[]
        for index, frame in enumerate(fitted):
            reference.pose(original[index]); rig.pose(frame)
            if rig.name == 'saria' and name == 'gSariaWaitArmsToSideAnim':
                fit_saria_standing_arms(rig, frame)
                continue
            if name=='gMalonAdultStandStillAnim':
                # The native basket skirt tilts 7.4 degrees sideways, which
                # is conspicuous on this replacement's long drape. Keep it
                # aligned with its own accepted standing skirt fit.
                frame[10]=rig.frames[0,10]
                rig.pose(frame)
            nr, nw, nh = body_frame(reference); fr, fw, fh = body_frame(rig)
            mapping = fr@np.diag([fw/nw, fh/nh, fw/nw])@nr.T
            nhp = reference.limbs[reference.profile["hips"]].T
            fhp = rig.limbs[rig.profile["hips"]].T
            targets, matrices = {}, {}
            for side in ("left", "right"):
                hand = rig.profile[side][-1]
                targets[side] = fhp+mapping@(reference.limbs[hand].T-nhp)
                matrices[side] = reference.limbs[hand].M.copy()
                if native_palms and ("SingAnim" in name or name in ("gSariaHandsBehindBackWaitAnim", "gMalonAdultStandStillAnim", 'gKokiriSittingCrossedArmsLegsAnim')):
                    center = reference.limbs[hand].T+matrices[side]@native_palms[side]
                    center = fhp+mapping@(center-nhp)
                    if name=='gMalonAdultStandStillAnim':
                        from native_npc_malon import basket_hand_matrix
                        matrices[side]=basket_hand_matrix(matrices[side],fr[:,2],side)
                    if "SingAnim" in name and source is not None:
                        # Open source hands need to lie on the replacement's
                        # own chest, rather than retain the native mitten's
                        # inward cup and end up inside the scarf or brooch.
                        chest = rig.profile["chest"]
                        forward = rig.limbs[chest].M@bind_m[chest].T@unit(ht[:, 2])
                        long = unit(fr@np.array([-.78 if side == "left" else .78, .62, 0.]))
                        z_axis = forward*(1 if side == "left" else -1)
                        y_axis = unit(np.cross(z_axis, long)); long = unit(np.cross(y_axis, z_axis))
                        matrices[side] = np.column_stack([long, y_axis, z_axis])
                        joints = {n: source.world[j][:3, 3] for n, j in zip(source.names, source.joints)}
                        hip_y = joints["Hips"][1]
                        region = ((source_points[:, 1] > hip_y+.08) &
                                  (source_points[:, 1] < joints["Neck"][1]) &
                                  (abs(source_points[:, 0]) < .065) &
                                  (model["bones"] == chest))
                        z = max(np.quantile(source_points[region, 2], .98), .115 if rig.name == "young-malon" else .12)
                        a = ht[:, :3]; t = bind_t[rig.profile["hips"]]-a@joints["Hips"]
                        front = a@np.array([0., hip_y+.13, z])+t
                        front = rig.limbs[chest].M@bind_m[chest].T@(front-bind_t[chest])+rig.limbs[chest].T
                        layer=(.022 if side=='left' else .008) if rig.name=='young-malon' else (.042 if side=='left' else .018)
                        plane = np.dot(front, forward)+layer*float(model["scale"])
                        center += forward*max(0., plane-np.dot(center, forward))
                    targets[side] = center-matrices[side]@custom_palms[side]
            if rig.name == "saria" and name in ("gSariaPlayingOcarinaAnim", "gSariaOcarinaToMouthAnim",
                                                "gSariaStopPlayingOcarinaAnim", "gSariaReturnToOcarinaAnim"):
                # Native ocarina vertex 9 is the end of the mouthpiece.
                progress = index/max(len(fitted)-1, 1)
                amount = (1. if name == "gSariaPlayingOcarinaAnim" else 1-progress if name == "gSariaStopPlayingOcarinaAnim" else progress)
                amount = amount*amount*(3-2*amount)
                seat_wrist_targets(targets, matrices, contact, amount)
                mouth = rig.limbs[head].M@mouth_local+rig.limbs[head].T
                mouthpiece = seat_ocarina(np.array([614., -130., 450.])*.8, contact)
                desired_right = mouth-matrices["right"]@mouthpiece
                delta = (desired_right-targets["right"])*amount
                for side in targets:
                    targets[side] += delta
                support_carried_ocarina(targets, fr, contact, amount)
            if source is not None and rig.name == "saria" and name == "gSariaHandsBehindBackWaitAnim":
                # The replacement's torso has its own depth. The native
                # wrist position alone left the larger palms inside it.
                joints = {n: source.world[j][:3, 3] for n, j in zip(source.names, source.joints)}
                hip_y = joints["Hips"][1]
                waist = ((source_points[:, 1] > hip_y+.02) & (source_points[:, 1] < hip_y+.11) &
                         (abs(source_points[:, 0]) < .075) & (model["bones"] == rig.profile["chest"]))
                if waist.any():
                    z = np.quantile(source_points[waist, 2], .03)
                    a = ht[:, :3]; t = bind_t[rig.profile["hips"]]-a@joints["Hips"]
                    back = a@np.array([0., hip_y+.07, z])+t
                    chest = rig.profile["chest"]
                    back = rig.limbs[chest].M@bind_m[chest].T@(back-bind_t[chest])+rig.limbs[chest].T
                    forward = rig.limbs[chest].M@bind_m[chest].T@unit(a[:, 2])
                    plane = np.dot(back, forward)-.018*float(model["scale"])
                    for side in targets:
                        center = targets[side]+matrices[side]@custom_palms[side]
                        targets[side] -= forward*max(0., np.dot(center, forward)-plane)
            if name not in ("gKokiriIdleAnim", 'gKokiriStandingArmsBehindBackAnim', "gMalonAdultIdleAnim"):
                poles={}
                if source is not None and rig.name in ('young-malon','malon'):
                    from native_npc_malon import clear_arm_targets
                    poles=clear_arm_targets(source,model,rig,reference,name,bind_m,bind_t,targets,matrices,active_hand_vertices,idle_profile)
                for side in ("left", "right"):
                    errors.append(solve_arm(rig, frame, side, targets[side], matrices[side], parent,pole=poles.get(side)))
            if waiting_fit and name in ('gSariaHandsBehindBackWaitAnim', 'gSariaTransitionHandsSideToBackAnim'):
                t = 1. if name == 'gSariaHandsBehindBackWaitAnim' else index/max(len(fitted)-1, 1)
                errors.append(apply_waiting_hand_fit(rig, frame, waiting_fit, t*t*(3-2*t)))
            if name=='gMalonAdultStandStillAnim':
                from native_npc_malon import plant_basket_feet
                stance.append(plant_basket_feet(model,rig,frame,parent,bind_m,bind_t))
        if not np.array_equal(original[:, 0], fitted[:, 0]):
            raise ValueError("Retargeting changed native root motion")
        private = rig.ns+"/PoseAnimations/"+path
        unchanged = np.array_equal(original, fitted)
        archive.add("alt/"+private, native.read(path) if unchanged else encode_animation(fitted))
        result[name] = dict(frames=len(fitted), max_wrist_error=round(max(errors, default=0.), 3),
                            native_root_preserved=True, arms_only=not bool(stance), native_clip_identical=unchanged)
        if stance:
            result[name]['planted_stance']=dict(
                joint_channels=[i+1 for side in ('left','right') for i in rig.profile[side][:3]],
                frames=stance,native_toe_heading_preserved=True,
                skirt_joint_channel=10,skirt_aligned_to_native_idle=True)
        if name == "gMalonAdultStandStillAnim" and native_palms:
            reference.pose(original[len(original)//2]); rig.pose(fitted[len(fitted)//2])
            result[name]["basket_fit"] = export_basket(archive, native, reference, rig, native_palms, custom_palms, private)
        if rig.name in ("twins", "young-malon") and source is not None:
            result[name]["shoulder_limbs"] = export_shoulder_pose(archive, source, model, rig, fitted,
                                                                  bind_m, bind_t, private)
        if grip is not None:
            export_grip(archive, grip, rig, private, rig.name == "saria")
            result[name]["finger_curl"] = curls
            if 'hand_weight_repairs' in grip:
                result[name]['hand_weight_repairs'] = grip['hand_weight_repairs']
    rig.pose(rig.frames[0])
    return result


def export_grip(archive, model, rig, private, ocarina=False):
    from native_npc_models import emit_mesh, wrapper
    rig.pose(rig.frames[0])
    texpath=model.get('texture_path',rig.ns+'/Textures/Diffuse')
    if ocarina:
        # Finger skin weights can reach the wrist rim. Export both neighboring
        # surfaces with the same source pose, without translating either mesh.
        for side in ('left','right'):
            fore=rig.profile[side][-2]
            for suffix,palette in (('',None),('/Skin',getattr(rig,'skin_palette',None))):
                if suffix and palette is None:continue
                emit_mesh(archive,'alt/'+private+suffix+f'/Limbs/Limb{fore:02d}',model,rig,
                          model['faces'][model['hosts']==fore],texpath,fore,palette)
    for side in ("left", "right"):
        hand = rig.profile[side][-1]
        path = private+f"/Hands/{side.title()}"
        emit_mesh(archive, "alt/"+path, model, rig, model["faces"][model["hosts"] == hand],
                  texpath, hand)
        pieces = [path]
        if ocarina and side == "right":
            pieces.append(rig.ns+"/Props/OcarinaGrip")
        wrapper(archive, "alt/"+private+f"/Limbs/Limb{hand:02d}", pieces)
        palette = getattr(rig, 'skin_palette', None)
        if palette is not None:
            path = private+f'/Skin/Hands/{side.title()}'
            emit_mesh(archive, 'alt/'+path, model, rig, model['faces'][model['hosts'] == hand],
                      texpath, hand, palette)
            pieces = [path]
            if ocarina and side == 'right':
                pieces.append(rig.ns+'/Props/OcarinaGrip')
            wrapper(archive, 'alt/'+private+f'/Skin/Limbs/Limb{hand:02d}', pieces)


def export_basket(archive, native, reference, rig, native_palms, custom_palms, private):
    """Keep the native basket materials and fit its handle to both palms."""
    import struct
    from native_npc_models import asset
    old, new = [], []
    for side in ("left", "right"):
        h = rig.profile[side][-1]
        old.append(reference.limbs[h].T+reference.limbs[h].M@native_palms[side])
        new.append(rig.limbs[h].T+rig.limbs[h].M@custom_palms[side])
    scale = np.linalg.norm(new[1]-new[0])/np.linalg.norm(old[1]-old[0])
    rotation = align(old[1]-old[0], new[1]-new[0])
    old_center, new_center = np.mean(old, axis=0), np.mean(new, axis=0)
    hand = rig.profile["left"][-1]
    before, after = reference.limbs[hand], rig.limbs[hand]
    vertex_source = "objects/object_ma2/object_ma2Vtx_004810"
    rows = asset.read_vertex_array(native.read(vertex_source)); revised = []
    for row in rows:
        p = before.M@np.asarray(row[:3])+before.T
        p = scale*rotation@(p-old_center)+new_center
        p = np.rint(after.M.T@(p-after.T)).astype(int)
        n = np.asarray(row[6:9], int); n[n > 127] -= 256
        n = np.rint(after.M.T@rotation@before.M@n).astype(int)
        revised.append((*p, *row[3:6], *n, row[9]))
    path = private+"/Props/Basket"
    vertices = path+"/V"
    archive.add("alt/"+vertices, asset.build_vertex_array(revised))
    raw = bytearray(native.read("objects/object_ma2/gMalonAdultBasketDL"))
    offset = 72
    while offset+8 <= len(raw):
        first, second = struct.unpack_from("<II", raw, offset); op = first >> 24
        size = 16 if op in (0x20, 0x31, 0x32, 0x33, 0x35, 0x36) else 8
        if op in (0x32, 0x33):
            hashed = asset.crc64(vertices if op == 0x32 else path)
            struct.pack_into("<II", raw, offset+8, hashed >> 32, hashed & 0xffffffff)
        offset += size
    archive.add("alt/"+path, bytes(raw))
    rig.pose(rig.frames[0])
    return dict(scale=round(float(scale), 6), native_materials_unchanged=True, grip_centers="both fitted palms")


def export_shoulder_pose(archive, source, model, rig, frames, bind_m, bind_t, private):
    """Bake the cap's chest/arm blend for this native pose family.

    The native format has one matrix per vertex. A pose-specific DL keeps
    the rounded cap blend when the arm folds behind the back or across the
    chest, while the shared seam ring retains exactly one matrix owner.
    """
    from native_npc_models import emit_mesh
    rig.pose(frames[len(frames)//2])
    chest = rig.profile["chest"]
    out = {k: model[k].copy() for k in ("positions", "normals", "bones", "uv", "faces")}
    def transform(bone, p, n):
        limb = rig.limbs[bone]
        return ((p-bind_t[bone])@bind_m[bone]@limb.M.T+limb.T,
                n@bind_m[bone]@limb.M.T)
    for bone in np.unique(model["bones"]):
        mask = model["bones"] == bone
        out["positions"][mask], out["normals"][mask] = transform(bone, model["positions"][mask], model["normals"][mask])
    affected = np.zeros(len(model["positions"]), bool)
    names = [m.get("name", "") for m in source.meshes]
    joints = {n: source.world[i][:3, 3] for n, i in zip(source.names, source.joints)}
    if rig.name == "young-malon":
        for side in ("left", "right"):
            prefix = side.title(); arm, fore = rig.profile[side][3:5]
            direction = unit(joints[prefix+"ForeArm"]-joints[prefix+"Arm"])
            distance = abs((model["source_positions"]-joints[prefix+"ForeArm"])@direction)
            weight = np.clip(1-distance/.06, 0, 1)
            mask = np.isin(model["bones"], [arm, fore]) & (weight > 0)
            soft = np.zeros((np.count_nonzero(mask), 3))
            for bone in np.flatnonzero((model["weights"][mask] > 0).any(0)):
                point, _ = transform(int(bone), model["positions"][mask], model["normals"][mask])
                soft += point*model["weights"][mask, bone, None]
            out["positions"][mask] = (out["positions"][mask]*(1-weight[mask, None])+soft*weight[mask, None])
            affected |= mask
    pairs = (("left", "tripo_part_4"), ("right", "tripo_part_10")) if rig.name == "twins" else (("left", "tripo_part_9"), ("right", "tripo_part_8"))
    for side, part in pairs:
        prefix = side.title()
        direction = unit(joints[prefix+"ForeArm"]-joints[prefix+"Arm"])
        distance = (model["source_positions"]-joints[prefix+"Arm"])@direction
        from native_npc_fit import cap_vertices
        mask = cap_vertices(model, source, rig, prefix, part) & (distance < .085)
        arm = rig.profile[side][3]
        blend = np.clip((distance[mask]+.025)/.09, 0, 1)
        blend = (blend*blend*(3-2*blend))[:, None]
        # Blend the two source-to-bone maps directly. Blending an already
        # blended bind cap a second time creates the protruding shoulder flap.
        source_p = model["source_positions"][mask]
        source_n = np.concatenate([m["normals"] for m in source.meshes])[mask]
        body_a = model["head_transform"][:, :3]
        body_t = bind_t[rig.profile["hips"]]-body_a@joints["Hips"]
        pc, nc = transform(chest, source_p@body_a.T+body_t, unit(source_n@np.linalg.inv(body_a)))
        fore = rig.profile[side][4]
        u = unit(joints[prefix+"ForeArm"]-joints[prefix+"Arm"])
        ratio = np.linalg.norm(bind_t[fore]-bind_t[arm])/np.linalg.norm(joints[prefix+"ForeArm"]-joints[prefix+"Arm"])
        arm_a = align(u, bind_t[fore]-bind_t[arm])@(float(model["scale"])*np.eye(3)+(ratio-float(model["scale"]))*np.outer(u, u))
        arm_t = bind_t[arm]-arm_a@joints[prefix+"Arm"]
        pa, na = transform(arm, source_p@arm_a.T+arm_t, unit(source_n@np.linalg.inv(arm_a)))
        out["positions"][mask] = pc*(1-blend)+pa*blend
        out["normals"][mask] = unit(nc*(1-blend)+na*blend)
        affected |= mask
    # Shared split copies must receive the same correction and matrix owner.
    _, group = np.unique(np.round(model["source_positions"], 6), axis=0, return_inverse=True)
    for g in np.unique(group[affected]):
        ids = np.flatnonzero(group == g)
        chosen = ids[affected[ids]]
        out["positions"][ids] = out["positions"][chosen].mean(0)
        affected[ids] = True
    face_normal = np.cross(out["positions"][model["faces"][:, 1]]-out["positions"][model["faces"][:, 0]],
                           out["positions"][model["faces"][:, 2]]-out["positions"][model["faces"][:, 0]])
    sums = np.zeros((group.max()+1, 3))
    for corner in range(3):
        np.add.at(sums, group[model["faces"][:, corner]], face_normal)
    out["normals"][affected] = unit(sums[group[affected]])
    hosts = np.unique(model["hosts"][affected[model["faces"]].any(1)])
    for host in hosts:
        path = private+f"/Limbs/Limb{host:02d}"
        chosen = model["hosts"] == host
        if rig.name == "young-malon" and host == chest:
            from native_npc_brooch import source_mask
            from native_npc_models import wrapper
            chosen &= ~source_mask(source, model)
            emit_mesh(archive, "alt/"+path+"/Surface", out, rig, model["faces"][chosen], rig.ns+"/Textures/Diffuse", int(host))
            wrapper(archive, "alt/"+path, [path+"/Surface", rig.ns+"/Body/BowserBrooch"])
        else:
            emit_mesh(archive, "alt/"+path, out, rig, model["faces"][chosen], rig.ns+"/Textures/Diffuse", int(host))
    rig.pose(rig.frames[0])
    return hosts.tolist()
