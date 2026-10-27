#!/usr/bin/env python3
"""Project all four offers in each steady native shelf-bank shot."""
from pathlib import Path
import argparse,hashlib,itertools,json,os,subprocess,tempfile
import numpy as np

ROOT=Path(__file__).resolve().parents[2]
def camera_views():
    with tempfile.TemporaryDirectory(prefix='market-camera-') as temporary:
        binary=Path(temporary)/'camera'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-I'+str(ROOT),
            str(ROOT/'tools/black_market/browsing_preview.cpp'),'-o',str(binary)],check=True)
        return json.loads(subprocess.check_output([str(binary)],text=True))

def project(point,view):
    eye,at=np.array(view['eye'],float),np.array(view['at'],float)
    forward=at-eye;forward/=np.linalg.norm(forward)
    right=np.cross(forward,[0,1,0]);right/=np.linalg.norm(right)
    up=np.cross(right,forward);delta=np.array(point)-eye;depth=delta@forward
    assert depth>0
    half=np.tan(np.deg2rad(view['fov'])/2)
    return [160+(delta@right)/depth/half*120,120-(delta@up)/depth/half*120]

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--layout',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();layout=json.loads(a.layout.read_text());views=camera_views();proof={}
    for name,slots in [('right',range(4)),('left',range(4,8))]:
        bounds=[]
        for slot in slots:
            view=views['slots'][slot]
            assert view['eye']==views['center']['eye'],(name,slot,'shop eye moves during bank turn')
            assert view==views['slots'][slots.start],(name,slot,'cursor navigation changes the bank camera')
            points=np.array([project(point,view) for point in itertools.product(*zip(*layout['reserved_slot_envelopes'][slot]))])
            low,high=points.min(0),points.max(0)
            assert low[0]>8 and high[0]<312 and low[1]>8 and high[1]<138,(name,slot,'item/seal clips screen or native Y142 textbox',low,high)
            anchor=np.array(project(layout['shelves'][slot],view))
            assert anchor[1]+20<142 and anchor[1]-20>4,(name,slot,'native cursor clips textbox or top')
            bounds.append({'slot':slot,'screen_min':low.tolist(),'screen_max':high.tolist()})
        proof[name]=bounds
    a.output.write_text(json.dumps(dict(views=views,bounds=proof,layout_sha256=hashlib.sha256(a.layout.read_bytes()).hexdigest(),
        source_sha256=hashlib.sha256((ROOT/'combo/BlackMarketBrowsing.h').read_bytes()).hexdigest(),
        native_textbox_y=142,aspect='4:3',runtime_proven=False,
        scope='Every bank uses one native scene-authored pose for all four offers. Full conditional authored FrameFit item/seal envelopes include selected scale and bob. Arbitrary native/Alt models and configured gameplay remain unproven.'),indent=2)+'\n')
    print('PASS native bank framing: fixed eye and steady four-offer banks; all eight item/seal envelopes and cursors clear 4:3 screen and explicit bottom textbox')

if __name__=='__main__':main()
