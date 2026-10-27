"""Asset-only standing-arm and leg-chroma refinement of a selected Saria pack."""
import argparse
import hashlib
import json
from pathlib import Path
import zipfile
import numpy as np
from native_npc_models import SourceModel,NativeRig,animation_frames,asset
from native_npc_pose_fit import fit_saria_standing_arms
from native_npc_color import repair_traditional_saria_diffuse,repair_traditional_saria_legs
from edit_native_npc_pose import encode_animation
from verify_native_npc_models import Archive


def refine(baseline,native,source_path,output):
    baseline,native,source_path,output=map(Path,(baseline,native,source_path,output))
    if output.resolve() in (baseline.resolve(),native.resolve(),source_path.resolve()):
        raise ValueError('Preserve all input files; choose a separate candidate')
    archive=Archive(native,baseline)
    records={n:archive.candidate.read(n) for n in archive.candidate.namelist()}
    manifests=[n for n in records if n.endswith('_NativeNpcModelInfo.json')]
    if len(manifests)!=1:raise ValueError('Expected one model manifest')
    manifest=json.loads(records[manifests[0]])
    if manifest['variant'] not in ('saria-anime','saria-render'):raise ValueError('Expected Saria')
    sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
    if sha(native)!=manifest['native_sha256'] or sha(source_path)!=manifest['source_sha256']:
        raise ValueError('Native/source differs from the selected baseline')
    rig=NativeRig(archive,'saria',private=True)
    path='objects/object_sa/gSariaWaitArmsToSideAnim'
    frames=animation_frames(archive.native.read(path));fitted=frames.copy()
    for frame in fitted:fit_saria_standing_arms(rig,frame)
    key='alt/'+rig.ns+'/PoseAnimations/'+path
    changes={key:encode_animation(fitted)}
    report=dict(parent_sha256=sha(baseline),standing_frames=len(frames),
                standing_changed_rotation_joints=[11,12,14,15],native_root_preserved=True,
                geometry_uv_skin_palette_and_grips_unchanged=True,runtime_tested=False)
    if manifest['variant']=='saria-render':
        source=SourceModel(source_path)
        _,reference=repair_traditional_saria_diffuse(source)
        tex='alt/'+rig.ns+'/Textures/SkinCorrectedDiffuse'
        raw=records[tex];kind,w,h,pixels=asset.read_texture(raw)
        if kind!=1 or len(raw)-len(pixels)!=0x5c:raise ValueError('Expected RGBA32 v1 texture')
        before=np.frombuffer(pixels,np.uint8).reshape(h,w,4)
        corrected,report['leg_color']=repair_traditional_saria_legs(source,before,reference['reference_linear_chroma'])
        changes[tex]=raw[:0x5c]+corrected.tobytes()
        color_info=manifest['geometry_repairs']['skin_chroma']
        color_info['legs']=report['leg_color']
        color_info['pixel_sha256']=hashlib.sha256(corrected.tobytes()).hexdigest()
    report['changed_resources']=sorted(changes)
    manifest['asset_refinement']=report
    records.update(changes)
    records[manifests[0]]=json.dumps(manifest,indent=2).encode()
    output.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(output,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
        for name,data in sorted(records.items()):
            info=zipfile.ZipInfo(name,(2026,10,19,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED;z.writestr(info,data)
    return dict(report,output_sha256=sha(output))

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for arg in ('baseline','native','source','output'):p.add_argument(arg,type=Path)
    a=p.parse_args();print(json.dumps(refine(a.baseline,a.native,a.source,a.output),indent=2))
