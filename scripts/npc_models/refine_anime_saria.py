"""Preserve a selected Anime Saria pack while refining its smile and waiting fit."""
import argparse,hashlib,json,zipfile
from pathlib import Path
from native_npc_models import NativeRig,animation_frames
from native_npc_pose_fit import apply_waiting_hand_fit
from edit_native_npc_pose import encode_animation
from verify_native_npc_models import Archive

def refine(baseline,native,smile_build,output):
    baseline,native,smile_build,output=map(Path,(baseline,native,smile_build,output))
    if output.resolve() in (baseline.resolve(),native.resolve(),smile_build.resolve()):raise ValueError('Preserve input files')
    a=Archive(native,baseline);records={n:a.candidate.read(n) for n in a.candidate.namelist()}
    manifest='saria_anime_NativeNpcModelInfo.json';m=json.loads(records[manifest]);ns=m['namespace']
    if m['variant']!='saria-anime':raise ValueError('This refinement is Anime Saria only')
    if hashlib.sha256(native.read_bytes()).hexdigest()!=m['native_sha256']:raise ValueError('Native source differs')
    with zipfile.ZipFile(smile_build) as donor:
        info=json.loads(donor.read(manifest))
        if any(info[k]!=m[k] for k in ('variant','source_sha256','native_sha256')):raise ValueError('Smile source differs')
        changed_head=[]
        for n in records:
            if n.startswith(('alt/'+ns+'/Body/Limb15','alt/'+ns+'/FaceParts/')):
                if n not in donor.namelist():raise ValueError('Smile donor lost a head resource')
                raw=donor.read(n)
                if raw!=records[n]:records[n]=raw;changed_head.append(n)
        m['geometry_repairs']['smile_repair']=info['geometry_repairs']['smile_repair']
    if not changed_head:raise ValueError('Smile donor has no changes')
    spec=json.loads(Path(__file__).with_name('pose_art').joinpath('saria-anime-waiting.json').read_text())
    if spec['source_sha256']!=m['source_sha256']:raise ValueError('Waiting fit source differs')
    rig=NativeRig(a,'saria',private=True);changed_clips=[]
    for name in ('gSariaHandsBehindBackWaitAnim','gSariaTransitionHandsSideToBackAnim'):
        key='alt/'+ns+'/PoseAnimations/objects/object_sa/'+name
        frames=animation_frames(records[key]);fitted=frames.copy()
        for i,f in enumerate(fitted):
            t=1. if name=='gSariaHandsBehindBackWaitAnim' else i/max(len(fitted)-1,1)
            apply_waiting_hand_fit(rig,f,spec,t*t*(3-2*t))
        records[key]=encode_animation(fitted);changed_clips.append(key)
    report=dict(parent_sha256=hashlib.sha256(baseline.read_bytes()).hexdigest(),
                changed_head_resources=changed_head,changed_animation_resources=changed_clips,
                waiting_fit=spec,all_textures_ocarina_grips_props_and_other_poses_unchanged=True,
                traditional_saria_unchanged=True,runtime_tested=False)
    m['anime_refinement']=report;records[manifest]=json.dumps(m,indent=2).encode()
    output.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(output,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
        for n,raw in sorted(records.items()):
            entry=zipfile.ZipInfo(n,(2026,10,19,0,0,0));entry.compress_type=zipfile.ZIP_DEFLATED;z.writestr(entry,raw)
    return dict(report,output_sha256=hashlib.sha256(output.read_bytes()).hexdigest())

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for k in ('baseline','native','smile_build','output'):p.add_argument(k,type=Path)
    a=p.parse_args();print(json.dumps(refine(a.baseline,a.native,a.smile_build,a.output),indent=2))
