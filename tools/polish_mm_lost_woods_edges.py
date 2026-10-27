#!/usr/bin/env python3
"""POC4 follow-up: continuous mushroom charts and the screenshot-marked trunk."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import re
import struct
import xml.etree.ElementTree as ET
import zipfile

BASE_SHA256='8acc5368601ab6efdecdc419acf966d66d7fa62dcf2c5a0c9e53bd5b73de9240'
PREFIX='custom/cor_mm3d/lost_woods_poc1/'
OLD_ROOT=PREFIX+'room_0/opa_combo_return_corner_v3'
NEW_ROOT=PREFIX+'room_0/opa_combo_return_edge_v4'
TEXTURE='alt/'+PREFIX+'textures/lw_kinoko_01_FC00'
DOOR={f'alt/{PREFIX}room_0/{n}_vtx' for n in (
    'ReturnRootTunnel','ReturnTunnelMouth','ReturnTunnelPinkLight','ReturnMushrooms_-1','ReturnMushrooms_1')}
ROOM='alt/scenes/nonmq/Z2_LOST_WOODS/Z2_LOST_WOODS_room_00'


def sha(data):return hashlib.sha256(data).hexdigest()


def stitch_texture(data):
    import numpy as np
    _,width,height,_=struct.unpack_from('<4I',data,64)
    if (width,height)!=(1024,1024):raise ValueError('expected exact HD mushroom atlas dimensions')
    pixels=np.frombuffer(data[92:],np.uint8).reshape(height,width,4).copy()
    original=pixels.copy();rgb=pixels[:,:,:3].astype(float)
    yy,xx=np.indices((height,width))
    # Match the cap's left and bottom radial borders. Work only in narrow
    # padding bands; preserve the existing UV chart and interior painting.
    def segment_distance(ax,ay,bx,by):
        t=np.clip(((xx-ax)*(bx-ax)+(yy-ay)*(by-ay))/((bx-ax)**2+(by-ay)**2),0,1)
        return np.hypot(xx-(ax+t*(bx-ax)),yy-(ay+t*(by-ay)))
    distance=np.minimum.reduce([abs(xx-9.5),abs(yy-1013.5),
                                segment_distance(50.5,9.5,9.5,60.5),
                                segment_distance(962.5,1013.5,1013.5,972.5),
                                segment_distance(50.5,9.5,706.5,316.5),
                                segment_distance(706.5,316.5,1013.5,972.5)])
    weight=np.clip((6-distance)/3,0,1)[:,:,None]
    reflected=rgb[::-1,::-1].transpose(1,0,2)
    rgb=rgb*(1-weight)+(rgb+reflected)*.5*weight
    # The underside repeats two triangular charts. Their diagonal left edges
    # share positions with the vertical right edge at identical T. One small
    # mushroom variant starts its upper diagonal at S=522 instead of S=512.
    # Give both chart borders the same per-row paint, including bilinear padding.
    t=np.arange(height)+.5
    left=np.where(t<=379,512+256*(t-10)/369,768+246*(t-379)/450)-.5
    variant=np.where(t<=379,522+246*(t-10)/369,
                     np.where(t<=604,768+246*(t-379)/450,np.minimum(1014,891+123*(t-604)/215)))-.5
    center=(left+variant)*.5
    source=original[:,:,:3].astype(float)
    ix=np.floor(center).astype(int).clip(0,width-2);fraction=center-ix
    left_color=source[np.arange(height),ix]*(1-fraction[:,None])+source[np.arange(height),ix+1]*fraction[:,None]
    right_color=(source[:,1013]+source[:,1014])*.5
    matched=(left_color+right_color)*.5
    distance=np.minimum(abs(xx-1013.5),np.maximum(np.maximum(left[:,None]-xx,xx-variant[:,None]),0))
    weight=np.clip((6-distance)/3,0,1)*(yy<=836)
    rgb=rgb*(1-weight[:,:,None])+matched[:,None,:]*weight[:,:,None]
    pixels[:,:,:3]=np.rint(rgb).clip(0,255).astype(np.uint8)
    assert np.array_equal(pixels[:,:,3],original[:,:,3])
    changed=int(np.any(pixels!=original,axis=2).sum())
    if changed>=width*height*.08:raise ValueError('mushroom repair exceeded narrow seam bands')
    return data[:92]+pixels.tobytes(),changed


def polish(source,target):
    if source.resolve()==target.resolve() or target.exists():raise ValueError('choose a new output')
    if sha(source.read_bytes())!=BASE_SHA256:raise ValueError('expected exact POC4a baseline')
    created=False;changed={}
    try:
        with zipfile.ZipFile(source) as before:
            names=before.namelist()
            if before.testzip() is not None or len(names)!=len(set(names)):raise ValueError('baseline integrity failure')
            texture,pixel_count=stitch_texture(before.read(TEXTURE))
            updates={TEXTURE:texture}
            headers={n for n in names if re.fullmatch(re.escape(ROOM)+r'(?:Set_[0-9A-F]+)?',n)}
            if len(headers)!=4:raise ValueError('expected four room-zero setups')
            with zipfile.ZipFile(target,'x',zipfile.ZIP_DEFLATED,compresslevel=6) as after:
                created=True
                for info in before.infolist():
                    original=before.read(info);data=updates.get(info.filename,original)
                    if info.filename in DOOR:
                        tree=ET.fromstring(data)
                        for v in tree:
                            v.set('X',str(int(v.get('X'))-270));v.set('Z',str(int(v.get('Z'))-470))
                        data=ET.tostring(tree,encoding='utf-8')+b'\n'
                    elif info.filename in headers:
                        old=struct.pack('<I',len(OLD_ROOT))+OLD_ROOT.encode()
                        if data.count(old)!=1:raise ValueError('expected one active v3 mesh')
                        data=data.replace(old,struct.pack('<I',len(NEW_ROOT))+NEW_ROOT.encode())
                    if data!=original:changed[info.filename]={'before_sha256':sha(original),'after_sha256':sha(data)}
                    after.writestr(copy.copy(info),data)
                wrapper=copy.copy(before.getinfo('alt/'+OLD_ROOT));wrapper.filename='alt/'+NEW_ROOT
                after.writestr(wrapper,before.read('alt/'+OLD_ROOT))
            if set(changed)!=set(updates)|DOOR|headers:raise ValueError('unexpected archive delta')
        return {'baseline_sha256':BASE_SHA256,'candidate_sha256':sha(target.read_bytes()),'changed_resources':changed,
                'added_resource':'alt/'+NEW_ROOT,'mushroom_changed_pixels':pixel_count,'door_anchor':[-660,0,-350],'door_yaw':16384,
                'target':'Large room-edge trunk marked by the arrow in image(20261017-175530).png; not the small cut stump.',
                'preservation':'Only narrow mushroom-atlas RGB seam bands, five doorway X/Z resources and four mesh bindings change. Mushroom geometry/UVs, all other textures and accepted cut-stump resources are byte-identical.',
                'status':'Asset/static candidate; matching v4 source required for gameplay; runtime acceptance pending.'}
    except BaseException:
        if created:target.unlink(missing_ok=True)
        raise


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source',type=Path);parser.add_argument('target',type=Path);parser.add_argument('--receipt',type=Path,required=True)
    a=parser.parse_args()
    if a.receipt.exists() or a.receipt.resolve() in (a.source.resolve(),a.target.resolve()):raise ValueError('choose a new, distinct receipt')
    created=False
    try:
        with a.receipt.open('x') as stream:
            created=True;json.dump(polish(a.source,a.target),stream,indent=2);stream.write('\n')
    except BaseException:
        if created:a.receipt.unlink(missing_ok=True)
        raise
    print('PASS narrow archive: mushroom seam bands, five doorway moves, four v4 bindings; geometry/UVs and accepted stump preserved')


if __name__=='__main__':main()
