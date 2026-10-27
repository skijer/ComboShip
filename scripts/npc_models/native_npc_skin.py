"""Archive-owned two-bone skin palette for the native flex draw path.

Vertices keep the existing 16-byte format. Additional segment-D matrices blend
the current native transforms, so shoulders/knees also work between clip frames.
"""
import struct
import numpy as np
from native_assets import o2r as asset

STEPS = 16
MAGIC = 0x4E53


class Palette:
    def __init__(self, bind, slots, weights):
        self.bind = np.asarray(bind, float)
        self.slots = list(slots)
        self.inverse = np.linalg.inv(self.bind[self.slots])
        w = np.asarray(weights, float)
        if not np.isfinite(w).all() or (w < 0).any() or not np.allclose(w.sum(1), 1):
            raise ValueError('Skin weights must be finite, nonnegative and normalized')
        self.blends, self.vertex_slots = [], []
        lookup = {}
        for row in w:
            ids = np.flatnonzero(row > 1e-8)
            if len(ids) > 2 or any(int(i) not in self.slots for i in ids):
                raise ValueError('Skin vertices require one or two drawable native bones')
            a = self.slots.index(int(ids[0]))
            b = self.slots.index(int(ids[-1]))
            q = int(np.rint(row[ids[-1]]*STEPS)) if a != b else 0
            if a>b:
                a,b=b,a
                q=STEPS-q
            if a == b or q == 0:
                slot = a
            elif q == STEPS:
                slot = b
            else:
                key = (a, b, q)
                if key not in lookup:
                    lookup[key] = len(self.slots)+len(self.blends)
                    self.blends.append(key)
                slot = lookup[key]
            self.vertex_slots.append(slot)
        self.vertex_slots = np.asarray(self.vertex_slots, int)
        if len(self.slots)+len(self.blends) > 256:
            raise ValueError('Skin palette exceeds the 8-bit native matrix-slot field')

    def matrices(self, pose):
        native = np.asarray(pose)[self.slots]
        deformation = native@self.inverse
        extra = [(deformation[a]*(STEPS-q)+deformation[b]*q)/STEPS for a, b, q in self.blends]
        return np.concatenate([native, np.asarray(extra).reshape(-1, 4, 4)])

    def resource(self):
        floats = self.inverse[:, :3].astype('<f4').tobytes()
        words = [MAGIC, 1, len(self.slots), len(self.blends)]
        words.extend(struct.unpack('<'+'H'*(len(floats)//2), floats))
        for a, b, q in self.blends:
            words.extend([a, b, q])
        return (asset._header(asset.T_OARR)+struct.pack('<II', 16, len(words))+
                b''.join(struct.pack('<IH', 5, word) for word in words))

    def geometry_for(self, model):
        from types import SimpleNamespace
        from native_npc_models import unit
        p=model.get('posed_source_positions',model['source_positions']).copy()
        n=model.get('posed_source_normals',model['normals'])
        # Split copies already welded by the source fitter use one source
        # point too. This prevents a UV seam from acquiring different motion.
        for ids in self.seam_groups:
            p[ids]=p[ids].mean(0)
        canonical=p@self.canonical[:3,:3].T+self.canonical[:3,3]
        canonical_n=unit(n@np.linalg.inv(self.canonical[:3,:3]))
        rest=model['positions'].copy();normals=model['normals'].copy()
        result=np.zeros_like(rest);normal=np.zeros_like(rest)
        for bone in np.flatnonzero((self.skin_weights>0).any(0)):
            a=self.transforms[bone,:3,:3];t=self.transforms[bone,:3,3]
            weight=self.skin_weights[:,bone,None]
            result+=(p@a.T+t)*weight
            normal+=(n@np.linalg.inv(a))*weight
        rest[self.skin_mask]=result[self.skin_mask]
        normals[self.skin_mask]=unit(normal[self.skin_mask])
        if getattr(self,'rest_space',False):
            # Standard fitted-rest skinning gives blended and rigid vertices
            # the same position/normal basis at the 0/16 influence endpoints.
            canonical=rest.copy();canonical_n=normals.copy()
        return SimpleNamespace(vertex_positions=canonical,vertex_normals=canonical_n,
                               rest_positions=rest,rest_normals=normals)


def decode(raw):
    if raw[4:8] != b'RRAO' or len(raw) < 96:
        raise ValueError('Expected a skin U16 array')
    kind, count = struct.unpack_from('<II', raw, 64)
    if kind != 16 or len(raw) != 72+count*6:
        raise ValueError('Invalid skin array length')
    pairs = [struct.unpack_from('<IH', raw, 72+i*6) for i in range(count)]
    if any(t != 5 for t, _ in pairs):
        raise ValueError('Skin array requires U16 scalars')
    words = [word for _, word in pairs]
    magic, version, native, extra = words[:4]
    if magic != MAGIC or version != 1 or not 1 <= native <= 32 or not 0 < extra <= 256-native or count != 4+native*24+extra*3:
        raise ValueError('Invalid skin palette header')
    inv = np.tile(np.eye(4), (native, 1, 1))
    data = struct.pack('<'+'H'*(native*24), *words[4:4+native*24])
    inv[:, :3] = np.frombuffer(data, '<f4').reshape(native, 3, 4)
    if not np.isfinite(inv).all() or np.any(abs(np.linalg.det(inv[:, :3, :3])) < 1e-9):
        raise ValueError('Invalid skin inverse bind')
    p = Palette.__new__(Palette)
    p.bind, p.inverse, p.slots = np.linalg.inv(inv), inv, list(range(native))
    p.blends = [tuple(words[i:i+3]) for i in range(4+native*24, count, 3)]
    if any(not (0 <= a < b < native and 0 < q < STEPS) for a, b, q in p.blends):
        raise ValueError('Invalid skin blend')
    return p


def joint_weights(source, model, rig):
    """Smooth anatomical joint bands; share weights across split seam copies."""
    p = model['source_positions']
    named = {n: source.world[i][:3, 3] for n, i in zip(source.names, source.joints)}
    if 'anatomical_joint_positions' in model:
        named=dict(zip(source.names,model['anatomical_joint_positions']))
    w = np.zeros_like(model['weights'])
    affected=np.zeros(len(p),bool)
    w[np.arange(len(p)), model['bones']] = 1
    for side in ('left', 'right'):
        up, leg, foot, arm, fore, hand = rig.profile[side]
        prefix = side.title()
        zones = [(rig.profile['chest'], arm, 'Arm', 'ForeArm', .22),
                 (arm, fore, 'ForeArm', 'Hand', .24),
                 (fore, hand, 'Hand', 'HandMiddle1', .34),
                 (rig.profile['hips'], up, 'UpLeg', 'Leg', .14),
                 (up, leg, 'Leg', 'Foot', .20),
                 (leg, foot, 'Foot', 'ToeBase', .30)]
        for a, b, first, second, fraction in zones:
            if prefix+second not in named:
                continue
            center, end = named[prefix+first], named[prefix+second]
            axis = end-center; length = np.linalg.norm(axis); axis /= length
            width = max(length*fraction, .008)
            delta = p-center
            distance = delta@axis
            radius = np.linalg.norm(delta-distance[:, None]*axis, axis=1)
            region = ((distance > -.07) & (distance < .085) & (radius < .09) if first == 'Arm' else
                      (abs(distance) < width) & (radius < width*2.5))
            mask = np.isin(model['bones'], [a, b]) & region
            t = np.clip((distance[mask]+.025)/.09 if first == 'Arm' else distance[mask]/(2*width)+.5, 0, 1)
            t = t*t*(3-2*t)
            w[mask] = 0; w[mask, a] = 1-t; w[mask, b] = t
            affected |= mask
    _, groups = np.unique(np.round(model['positions'], 3), axis=0, return_inverse=True)
    for group in np.flatnonzero(np.bincount(groups) > 1):
        ids = np.flatnonzero(groups == group)
        row = w[ids].mean(0)
        keep = np.argsort(row)[-2:]
        row[np.setdiff1d(np.arange(len(row)), keep)] = 0
        w[ids] = row/row.sum()
        affected[ids]=affected[ids].any()
    authored=model.get('authored_joint_mask')
    if authored is not None:
        w[authored]=model['authored_joint_weights'][authored]
        affected[authored]=True
    model['skin_joint_mask']=affected
    attached=model.get('rigid_attachments')
    if attached is not None:
        ids=np.flatnonzero(attached>=0)
        w[ids]=0;w[ids,attached[ids]]=1
        affected[ids]=False
    return w


def prepare(source, model, rig):
    bind = np.tile(np.eye(4), (len(rig.limbs), 1, 1))
    for i, limb in enumerate(rig.limbs):
        bind[i, :3, :3], bind[i, :3, 3] = limb.M, limb.T
    palette = Palette(bind, rig.skel.slot_to_limb, joint_weights(source, model, rig))
    palette.canonical=np.eye(4);palette.canonical[:3,:3]=model['head_transform'][:,:3]
    hips=source.world[source.joints[source.names.index('Hips')]][:3,3]
    palette.canonical[:3,3]=rig.limbs[rig.profile['hips']].T-palette.canonical[:3,:3]@hips
    palette.transforms=model['source_bone_transforms']
    import hashlib
    from native_npc_twins import SOURCE_SHA256
    palette.rest_space=(rig.name=='twins' and hashlib.sha256(getattr(source,'raw',b'')).hexdigest()==SOURCE_SHA256)
    if not palette.rest_space:
        palette.inverse=palette.inverse@palette.transforms[palette.slots]@np.linalg.inv(palette.canonical)
    palette.skin_weights=np.zeros_like(model['weights'])
    for i,slot in enumerate(palette.vertex_slots):
        if slot<len(palette.slots):palette.skin_weights[i,palette.slots[slot]]=1.
        else:
            a,b,q=palette.blends[slot-len(palette.slots)]
            palette.skin_weights[i,palette.slots[a]]=(STEPS-q)/STEPS
            palette.skin_weights[i,palette.slots[b]]=q/STEPS
    palette.skin_mask=model['skin_joint_mask']
    _,group=np.unique(np.round(model['positions'],3),axis=0,return_inverse=True)
    palette.seam_groups=[np.flatnonzero(group==g) for g in np.flatnonzero(np.bincount(group)>1)]
    geometry=palette.geometry_for(model)
    palette.vertex_positions=geometry.vertex_positions
    rig.skin_palette = palette
    return palette
