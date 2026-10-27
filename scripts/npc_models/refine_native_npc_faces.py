"""Rebuild only the selected model's private head; preserve its newer body pack."""
import argparse,hashlib,json,zipfile
import numpy as np
from pathlib import Path
from native_npc_models import SourceModel,NativeRig,fit_model,emit_mesh,export_face_parts,asset
from native_npc_face_shading import SOURCES
from verify_native_npc_models import Archive

def refine(baseline,native,source_path,variant,output):
    baseline,native,source_path,output=map(Path,(baseline,native,source_path,output))
    if output.resolve() in (baseline.resolve(),native.resolve(),source_path.resolve()):raise ValueError('Preserve inputs')
    if variant not in SOURCES:raise ValueError('This repair is twins or young Malon only')
    a=Archive(native,baseline);s=SourceModel(source_path);manifest=variant.replace('-','_')+'_NativeNpcModelInfo.json'
    records={n:a.candidate.read(n) for n in a.candidate.namelist()};meta=json.loads(records[manifest])
    if meta['variant']!=variant or meta['source_sha256']!=hashlib.sha256(s.raw).hexdigest() or meta['source_sha256']!=SOURCES[variant]:raise ValueError('Head source differs')
    if meta['native_sha256']!=hashlib.sha256(native.read_bytes()).hexdigest():raise ValueError('Native source differs')
    rig=NativeRig(a.native,variant);model=fit_model(s,rig);head=rig.profile['head'];ns=rig.ns
    if variant=='young-malon':
        # The newer body fitter has a few different hair seam seats. Reuse
        # the selected pack's exact quantized head points, matched by original
        # diffuse UV and bone, while retaining the repaired normal field.
        old_rig=NativeRig(a,variant,True)
        data=a.geometry(ns+f'/Body/Limb{head:02d}',old_rig,head)
        lookup={}
        for point,uv,bone in zip(data['local'],np.rint(data['uv']*512*32).astype(int),data['bones']):
            lookup.setdefault((int(bone),*uv),set()).add(tuple(point))
        ids=np.unique(model['faces'][model['hosts']==head])
        for i in ids:
            bone=int(model['bones'][i]);limb=rig.limbs[bone]
            uv=np.rint(model['uv'][i]*512*32).astype(int);key=(bone,*uv)
            if key not in lookup:raise ValueError('Source head UV/bone differs from the selected pack')
            local=(model['positions'][i]-limb.T)@limb.M
            point=min(lookup[key],key=lambda q:np.linalg.norm(local-q))
            if np.linalg.norm(local-point)>100:raise ValueError('Source head point differs beyond the preserved seam fit')
            model['positions'][i]=limb.M@np.asarray(point)+limb.T
    w=asset.O2R();tex=ns+'/Textures/Diffuse'
    from native_npc_face import export_twin_face_diffuse
    tex=export_twin_face_diffuse(w,s,model,rig,variant,tex)
    emit_mesh(w,'alt/'+ns+f'/Body/Limb{head:02d}',model,rig,model['faces'][model['hosts']==head],tex,head)
    export_face_parts(w,s,model,rig,variant,tex)
    changed=[]
    for n,raw in w.files.items():
        if '/Textures/' in n:
            if n in records and records[n]!=raw:
                label={'EyesNeutral':'neutral','EyesHalf':'half','EyesClosed':'closed'}.get(n.rsplit('/',1)[-1])
                spec=json.loads(Path(__file__).with_name('face_art').joinpath(variant,'projection.json').read_text())
                allowed=spec.get('lash_cleanup',{}).get('panels',{}).get(label,{}) if variant=='twins' else {}
                old_hash=hashlib.sha256(a.texture(n.removeprefix('alt/'))[3]).hexdigest()
                if old_hash not in (allowed.get('original_pixel_sha256'),allowed.get('revised_pixel_sha256')):
                    raise ValueError('Existing eye artwork differs from the audited lash-cleanup baseline')
        if n not in records or records[n]!=raw:changed.append(n)
        records[n]=raw
    report=dict(parent_sha256=hashlib.sha256(baseline.read_bytes()).hexdigest(),changed_head_resources=changed,
                body_rig_palette_animations_brooch_and_original_diffuse_unchanged=True,
                edited_eye_textures=[n for n in changed if '/Textures/Eyes' in n],
                painted_eye_skin_topology_preserved=variant=='young-malon',
                hair_excluded_from_eye_patches=variant=='young-malon',
                open_eye_source_depth_preserved=True,runtime_tested=False)
    meta['face_refinement']=report;meta['geometry_repairs']['face_shading']=model['face_shading_repair']
    records[manifest]=json.dumps(meta,indent=2).encode();output.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(output,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
        for n,raw in sorted(records.items()):
            info=zipfile.ZipInfo(n,(2026,10,19,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED;z.writestr(info,raw)
    return dict(report,output_sha256=hashlib.sha256(output.read_bytes()).hexdigest())

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('baseline','native','source','variant','output'):p.add_argument(name)
    a=p.parse_args();print(json.dumps(refine(a.baseline,a.native,a.source,a.variant,a.output),indent=2))
