"""Share the accepted shoulder skin with every native/cutscene animation."""
import argparse
import hashlib
import json
from pathlib import Path
import zipfile

from native_npc_models import NativeRig, asset, wrapper
from verify_native_npc_models import Archive

PARENT_SHA256='e14308e3f2cc85efce299ea3785ab73e740aa968fe3bf540b8bce01e791ef5c5'
HOSTS=(7,8,11)

def shared_shoulders(records,namespace):
    """Use the reviewed skin in fitted rest space; all live joints still move."""
    out=asset.O2R()
    idle=namespace+'/PoseAnimations/objects/object_os_anime/gKokiriIdleAnim/Skin/Limbs/'
    for host in HOSTS:
        source=idle+f'Limb{host:02d}'
        if 'alt/'+source not in records:
            raise ValueError('Reviewed shoulder source is missing: '+source)
        wrapper(out,'alt/'+namespace+f'/Skin/Limbs/Limb{host:02d}',[source])
    return out.files

def expand(native,baseline,output):
    native,baseline,output=map(Path,(native,baseline,output))
    if output.resolve() in (native.resolve(),baseline.resolve()):raise ValueError('Preserve accepted inputs')
    if hashlib.sha256(baseline.read_bytes()).hexdigest()!=PARENT_SHA256:
        raise ValueError('Use the face/shoulder candidate accepted by cor')
    archive=Archive(native,baseline);rig=NativeRig(archive,'twins',True)
    records={n:archive.candidate.read(n) for n in archive.candidate.namelist()}
    meta=json.loads(records['twins_NativeNpcModelInfo.json'])
    if meta['variant']!='twins' or meta['native_sha256']!=hashlib.sha256(native.read_bytes()).hexdigest():
        raise ValueError('Native/model input differs from the accepted pack')
    changed=shared_shoulders(records,rig.ns);sources={};counts={}
    for host in HOSTS:
        destination=rig.ns+f'/Skin/Limbs/Limb{host:02d}'
        source=rig.ns+'/PoseAnimations/objects/object_os_anime/gKokiriIdleAnim'+f'/Skin/Limbs/Limb{host:02d}'
        sources[destination]=source
        before=archive.geometry(destination,rig,host,triangles_only=True)
        after=archive.geometry(source,rig,host,triangles_only=True)
        if before!=int(meta['body_triangles'][str(host)]):
            raise ValueError('Shared skin differs from the reviewed source topology')
        counts[str(host)]=dict(previous=before,shared=after)
    report=dict(parent_sha256=PARENT_SHA256,sources=sources,triangle_counts=counts,
                accepted_head_face_shoulder_pose_meshes_palette_and_native_clips_byte_identical=True,
                method='Reviewed idle shoulder skin reused through live flex matrices for all other animations',
                runtime_tested=False)
    meta['joint_skin']['shared_shoulders']=report
    records.update(changed)
    records['twins_NativeNpcModelInfo.json']=json.dumps(meta,indent=2).encode()
    records[rig.ns+'/PoseCoverageInfo.json']=json.dumps(report,indent=2).encode()
    output.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(output,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
        for name,raw in sorted(records.items()):
            info=zipfile.ZipInfo(name,(2026,10,19,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED
            z.writestr(info,raw)
    return dict(report,output_sha256=hashlib.sha256(output.read_bytes()).hexdigest())

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('native','baseline','output'):p.add_argument(name)
    a=p.parse_args();print(json.dumps(expand(a.native,a.baseline,a.output),indent=2))
