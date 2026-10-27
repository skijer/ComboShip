"""Bounded archive-only tunic and crossed-arm repairs for the reviewed Twins."""
import argparse
from collections import defaultdict
import hashlib
import json
from pathlib import Path
from types import SimpleNamespace
import zipfile

import numpy as np

from native_npc_models import SourceModel, NativeRig, animation_frames, emit_mesh, unit, asset
from native_npc_pose_fit import parents, solve_arm
from native_npc_skin import prepare
from native_npc_twins import SOURCE_SHA256
from edit_native_npc_pose import encode_animation
from verify_native_npc_models import Archive

PARENT_SHA256 = '85ea0720d30cdc20eb70277383ade3c1af033a500a7fcb87e94d144aa1e58868'
HIPS_CLIP = 'objects/object_os_anime/gKokiriStandingHandsOnHipsAnim'
CROSSED_CLIP = 'objects/object_os_anime/gKokiriSittingCrossedArmsLegsAnim'


def coherent_tunic(out, archive, source, model, rig):
    """Keep this standing pose's continuous tunic on its torso, across hosts."""
    palette = prepare(source, model, rig)
    reference = palette.geometry_for(model)
    points, uv = model['source_positions'], model['uv']
    cloth = (model['mesh_ids'] == 0) & (points[:, 1] < .64)
    lookup = defaultdict(list)
    for i in np.flatnonzero(cloth):
        lookup[tuple(np.rint(uv[i]*16384).astype(int))].append(int(i))
    chest = rig.profile['chest']
    transform = model['source_bone_transforms'][chest]
    source_normals = np.concatenate([m['normals'] for m in source.meshes])
    target = points@transform[:3, :3].T+transform[:3, 3]
    target_normal = unit(source_normals@np.linalg.inv(transform[:3, :3]))
    bind_m, bind_t = model['bind_matrices'], model['bind_positions']
    slots = rig.skel.slot_to_limb
    report = []
    for host in (0, 1, 4, 7):
        path = rig.ns+f'/Skin/Limbs/Limb{host:02d}'
        data = archive.geometry(path, rig, host)
        if set(data['textures']) != {rig.ns+'/Textures/Diffuse'}:
            raise ValueError('The bounded tunic host has unexpected materials')
        p, n = data['local'].copy(), data['normals'].copy()
        vertex_slots = np.zeros(len(p), int)
        for bone in np.unique(data['bones']):
            ids = data['bones'] == bone
            if bone < len(rig.limbs):
                p[ids] = p[ids]@bind_m[bone].T+bind_t[bone]
                n[ids] = n[ids]@bind_m[bone].T
                vertex_slots[ids] = slots.index(int(bone))
            else:
                vertex_slots[ids] = len(slots)+bone-len(rig.limbs)
        changed = 0
        for i, key in enumerate(np.rint(data['uv']*16384).astype(int)):
            candidates = lookup.get(tuple(key))
            if not candidates:
                continue
            distances = np.linalg.norm(reference.rest_positions[candidates]-p[i], axis=1)
            chosen = candidates[int(np.argmin(distances))]
            if min(distances) > 3:
                raise ValueError('A tunic atlas coordinate has ambiguous source ownership')
            p[i], n[i] = target[chosen], target_normal[chosen]
            vertex_slots[i] = slots.index(chest)
            changed += 1
        # Rebuild the actual matrix-load groups as well as vertex flags. A
        # flag-only buffer patch would leave the previous display-list matrix.
        proxy = SimpleNamespace(vertex_slots=vertex_slots, geometry_for=lambda m:
            SimpleNamespace(rest_positions=p, rest_normals=n,
                            vertex_positions=data['local'], vertex_normals=data['normals']))
        mesh = dict(positions=p, normals=n, uv=data['uv'], faces=data['faces'],
                    bones=np.full(len(p), host))
        destination = rig.ns+'/PoseAnimations/'+HIPS_CLIP+f'/Skin/Limbs/Limb{host:02d}'
        emit_mesh(out, 'alt/'+destination, mesh, rig, mesh['faces'],
                  rig.ns+'/Textures/Diffuse', host, proxy)
        report.append(dict(host=host,triangle_count=len(mesh['faces']),
                           cloth_triangle_corners=changed,source_path=path,destination=destination))
    return report


def crossed_wrist_fit(archive, rig):
    """Retain native raised wrists instead of centering the larger fingertips."""
    native = NativeRig(archive, 'twins')
    original = animation_frames(archive.native.read(CROSSED_CLIP))
    fitted = original.copy()
    parent = parents(rig)
    errors, heights = [], []
    for i, frame in enumerate(fitted):
        native.pose(original[i]); rig.pose(frame)
        native_shoulders = np.mean([native.limbs[native.profile[s][-3]].T for s in ('left','right')],axis=0)
        custom_shoulders = np.mean([rig.limbs[rig.profile[s][-3]].T for s in ('left','right')],axis=0)
        targets, matrices = {}, {}
        for side in ('left','right'):
            arm, fore, hand = rig.profile[side][-3:]
            old_length = sum(np.linalg.norm(native.limbs[b].T-native.limbs[a].T)
                             for a,b in ((arm,fore),(fore,hand)))
            new_length = sum(np.linalg.norm(rig.limbs[b].T-rig.limbs[a].T)
                             for a,b in ((arm,fore),(fore,hand)))
            targets[side] = custom_shoulders+(native.limbs[hand].T-native_shoulders)*(new_length/old_length)
            matrices[side] = native.limbs[hand].M.copy()
        for side in ('left','right'):
            errors.append(solve_arm(rig,frame,side,targets[side],matrices[side],parent))
            fore,hand=rig.profile[side][-2:]
            heights.append(float(rig.limbs[hand].T[1]-rig.limbs[fore].T[1]))
    arm_ids={j+1 for s in ('left','right') for j in rig.profile[s][-3:]}
    other=sorted(set(range(original.shape[1]))-arm_ids)
    np.testing.assert_array_equal(fitted[:,other],original[:,other])
    return fitted,dict(frames=len(fitted),max_wrist_error=max(errors),
                       wrist_heights_above_elbows=heights,native_hand_rotations_preserved=True,
                       native_root_timing_and_non_arm_channels_preserved=True)


def refine(native, source_path, candidate, fitted_path, output):
    native,source_path,candidate,fitted_path,output=map(Path,(native,source_path,candidate,fitted_path,output))
    if output.resolve() in {p.resolve() for p in (native,source_path,candidate,fitted_path)}:
        raise ValueError('Keep all accepted inputs read-only')
    if hashlib.file_digest(open(candidate,'rb'),'sha256').hexdigest()!=PARENT_SHA256:
        raise ValueError('Use the exact reviewed POC7 parent')
    source=SourceModel(source_path)
    if hashlib.sha256(source.raw).hexdigest()!=SOURCE_SHA256:
        raise ValueError('This repair belongs to the reviewed Twins GLB')
    saved=np.load(fitted_path,allow_pickle=False)
    required=('positions','normals','source_positions','uv','weights','faces','bones','hosts','scale',
              'posed_source_positions','posed_source_normals','head_transform','source_bone_transforms',
              'authored_joint_mask','authored_joint_weights','mesh_ids','boundary','bind_matrices','bind_positions')
    model={k:saved[k] for k in required}
    np.testing.assert_array_equal(model['source_positions'],np.concatenate([m['positions'] for m in source.meshes]))
    archive=Archive(native,candidate);rig=NativeRig(archive,'twins',True)
    np.testing.assert_allclose(model['bind_matrices'],np.array([l.M for l in rig.limbs]),atol=1e-8)
    np.testing.assert_allclose(model['bind_positions'],np.array([l.T for l in rig.limbs]),atol=1e-6)
    out=asset.O2R()
    cloth=coherent_tunic(out,archive,source,model,rig)
    frames,arms=crossed_wrist_fit(archive,rig)
    out.add('alt/'+rig.ns+'/PoseAnimations/'+CROSSED_CLIP,encode_animation(frames))
    report=dict(parent_sha256=PARENT_SHA256,source_sha256=SOURCE_SHA256,
                pose2_coherent_tunic=cloth,pose5_whole_arm_fit=arms,
                pose3_head='Exact native downward head rotation retained',
                pose4_hands='Exact native hand rotations retained; native head/donor-leg adapter unchanged',
                accepted_head_faces_blinks_shared_shoulders_and_palette_unchanged=True,
                changed_resources=sorted(out.files),runtime_tested=False)
    info='twins_NativeNpcModelInfo.json';manifest=json.loads(archive.candidate.read(info))
    manifest.setdefault('geometry_repairs',{})['twins_pose_fits']=report
    out.add(info,json.dumps(manifest,indent=2).encode())
    out.add(rig.ns+'/PoseFitInfo.json',json.dumps(report,indent=2).encode())
    output.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(output,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
        for name in sorted(set(archive.candidate.namelist())|set(out.files)):
            raw=out.files[name] if name in out.files else archive.candidate.read(name)
            entry=zipfile.ZipInfo(name,(2026,10,19,0,0,0));entry.compress_type=zipfile.ZIP_DEFLATED
            z.writestr(entry,raw)
    report['output_sha256']=hashlib.file_digest(open(output,'rb'),'sha256').hexdigest()
    return report


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('native','source','candidate','fitted','output'):parser.add_argument(name)
    args=parser.parse_args()
    print(json.dumps(refine(args.native,args.source,args.candidate,args.fitted,args.output),indent=2))
