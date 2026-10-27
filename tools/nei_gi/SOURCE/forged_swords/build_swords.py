"""Rebuild the seven reviewed sword candidates; never installs or packages a mod."""
import argparse
import hashlib
import json
from pathlib import Path
import sys
import numpy as np

ROOT=Path(__file__).resolve().parent
sys.path.insert(0,str(ROOT/'SOURCE'))
import meshkit
import preview
import sword_forged


def build(output):
    output=Path(output);output.mkdir(parents=True,exist_ok=True)
    meshkit.ROOT=output/'GAME_ASSETS'
    glbs=output/'PREVIEW_GLBS';glbs.mkdir(exist_ok=True)
    records={}
    for slug in sword_forged.BUILDERS:
        m=sword_forged.build(slug)
        for part in m.parts:
            p,t,n=part['p'],part['tri'],part['n']
            assert np.isfinite(p).all() and np.isfinite(n).all()
            assert np.allclose(np.linalg.norm(n,axis=1),1,atol=1e-6)
            assert np.max(np.abs(p*16))<32768
            assert np.all(np.linalg.norm(np.cross(p[t[:,1]]-p[t[:,0]],p[t[:,2]]-p[t[:,0]]),axis=1)>1e-7)
        assert all(a['alpha']==1 for a in m.materials.values())
        stats=meshkit.export_resources(m)
        path=glbs/(slug+'.glb');preview.glb(m,path)
        dl=(meshkit.ROOT/'RESOURCES'/m.entry).read_text()
        assert 'G_TEXTURE_GEN="1"' in dl
        assert 'G_RM_AA_ZB_XLU_SURF2' not in dl
        records[slug]={
            'name':m.name,'resource_entry':m.entry,'triangle_count':sum(len(p['tri']) for p in m.parts),
            'author_quantization':16,'resource_scale':m.native_scale,'draw_scale':m.draw_scale,
            'glb_sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'export':stats,'notes':m.notes,
            'runtime_tested':False,
        }
        print('Exported',slug,flush=True)
    (output/'MODEL_MANIFEST.json').write_text(json.dumps(records,indent=2)+'\n')
    return records


if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,default=ROOT)
    args=ap.parse_args();build(args.output)
