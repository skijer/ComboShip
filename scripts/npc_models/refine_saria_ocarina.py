"""Apply the private Fairy Ocarina HD atlas without changing any model fit."""
import argparse,hashlib,json,zipfile
from pathlib import Path
from native_npc_pose_fit import bind_ocarina_texture,ocarina_hd_resource
from native_assets.path_guard import require_separate_destination

def refine(baseline,output):
    baseline,output=map(Path,(baseline,output))
    art=Path(__file__).with_name('prop_art')/'saria-fairy-ocarina.png'
    require_separate_destination(output,baseline,art)
    with zipfile.ZipFile(baseline) as z:records={n:z.read(n) for n in z.namelist()}
    manifests=[n for n in records if n.endswith('_NativeNpcModelInfo.json')]
    if len(manifests)!=1:raise ValueError('Expected one Saria manifest')
    m=json.loads(records[manifests[0]])
    if m['variant'] not in ('saria-anime','saria-render'):raise ValueError('Expected Saria')
    texture=m['namespace']+'/Props/Textures/FairyOcarinaHD'
    if 'alt/'+texture in records:raise ValueError('Parent already contains the HD prop')
    changed=[]
    for suffix in ('','Grip'):
        path='alt/'+m['namespace']+'/Props/Ocarina'+suffix
        records[path]=bind_ocarina_texture(records[path],'objects/object_sa/gSariaFairyOcarinaTex',texture)
        changed.append(path)
    records['alt/'+texture]=ocarina_hd_resource(art)
    report=dict(parent_sha256=hashlib.sha256(baseline.read_bytes()).hexdigest(),
                texture=texture,texture_size=[256,128],native_texture_size=[32,16],scale=[16,8],
                texture_encoding='raw RGBA32; native RGBA16 tile byte stride and UVs retained',
                art_sha256=hashlib.sha256(art.read_bytes()).hexdigest(),
                changed_display_lists=changed,all_geometry_rig_skin_poses_faces_and_other_textures_unchanged=True,
                runtime_tested=False)
    m['geometry_repairs']['ocarina_texture']=report
    records[manifests[0]]=json.dumps(m,indent=2).encode()
    output.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(output,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
        for name,data in sorted(records.items()):
            info=zipfile.ZipInfo(name,(2026,10,19,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED;z.writestr(info,data)
    return dict(report,output_sha256=hashlib.sha256(output.read_bytes()).hexdigest())

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('baseline',type=Path);p.add_argument('output',type=Path)
    a=p.parse_args();print(json.dumps(refine(a.baseline,a.output),indent=2))
