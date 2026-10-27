#!/usr/bin/env python3
"""Align the POC5 doorway with its existing room-edge trunk; preserve all scenery."""
import argparse
import copy
import hashlib
import json
import math
from pathlib import Path
import re
import struct
import xml.etree.ElementTree as ET
import zipfile

BASE_SHA256='e6c5defb683088100e23301fc9d44fa5a3ab17640905ff3b375a51724270005b'
CENTER_BASE_SHA256='58b6b10dd1ac2cf532fab99b00dbed22ff5ca0a701f388ede2e8d920a75fa7f7'
RECENTER_BASE_SHA256='caa96a5941c24a07a32cf109c9a26e3fffa149ab2b9a5eab696e613c400a37ff'
RIGHT_BASE_SHA256='9e62cc55104968710b795d3765f01ba607184438fc8e910f37da41fdece94ded'
ROOT='custom/cor_mm3d/lost_woods_poc1/room_0/'
OLD_ROOT=ROOT+'opa_combo_return_edge_v4'
NEW_ROOT=ROOT+'opa_combo_return_aligned_v5'
DOOR={'alt/'+ROOT+n+'_vtx' for n in ('ReturnRootTunnel','ReturnTunnelMouth','ReturnTunnelPinkLight','ReturnMushrooms_-1','ReturnMushrooms_1')}
ANCHOR=(-677,-357)
YAW=12288


def sha(data):return hashlib.sha256(data).hexdigest()


def align(source,target,center_left=False,left_more=False,tiny_right=False):
    if sum((center_left,left_more,tiny_right))>1:raise ValueError('choose one positioning mode')
    if source.resolve()==target.resolve() or target.exists():raise ValueError('choose a new output')
    baseline=RIGHT_BASE_SHA256 if tiny_right else RECENTER_BASE_SHA256 if left_more else CENTER_BASE_SHA256 if center_left else BASE_SHA256
    if sha(source.read_bytes())!=baseline:raise ValueError('unexpected archive baseline for this positioning mode')
    old_root=NEW_ROOT if center_left else OLD_ROOT
    new_root=ROOT+'opa_combo_return_centered_v6' if center_left else NEW_ROOT
    anchor=(-684,-340) if center_left else ANCHOR
    delta=(-7,17) if center_left else None
    version=6 if center_left else 5
    if left_more:
        old_root=ROOT+'opa_combo_return_centered_v6';new_root=ROOT+'opa_combo_return_centered_v7'
        anchor=(-698,-306);delta=(-14,34);version=7
    elif tiny_right:
        old_root=ROOT+'opa_combo_return_centered_v7';new_root=ROOT+'opa_combo_return_centered_v8'
        anchor=(-695,-313);delta=(3,-7);version=8
    angle=YAW*math.tau/65536;s,c=math.sin(angle),math.cos(angle)
    created=False;changed={}
    try:
        with zipfile.ZipFile(source) as before:
            names=before.namelist()
            if before.testzip() is not None or len(names)!=len(set(names)):raise ValueError('baseline integrity failure')
            headers={n for n in names if re.fullmatch(r'alt/scenes/nonmq/Z2_LOST_WOODS/Z2_LOST_WOODS_room_00(?:Set_[0-9A-F]+)?',n)}
            if len(headers)!=4:raise ValueError('expected four room-zero setups')
            with zipfile.ZipFile(target,'x',zipfile.ZIP_DEFLATED,compresslevel=6) as after:
                created=True
                for info in before.infolist():
                    original=before.read(info);data=original
                    if info.filename in DOOR:
                        tree=ET.fromstring(data)
                        for v in tree:
                            if delta:
                                v.set('X',str(int(v.get('X'))+delta[0]));v.set('Z',str(int(v.get('Z'))+delta[1]))
                            else:
                                u=-(int(v.get('Z'))+350);d=int(v.get('X'))+660
                                v.set('X',str(round(ANCHOR[0]+u*c+d*s)))
                                v.set('Z',str(round(ANCHOR[1]-u*s+d*c)))
                        data=ET.tostring(tree,encoding='utf-8')+b'\n'
                    elif info.filename in headers:
                        old=struct.pack('<I',len(old_root))+old_root.encode()
                        if data.count(old)!=1:raise ValueError('expected one active prior mesh')
                        data=data.replace(old,struct.pack('<I',len(new_root))+new_root.encode())
                    if data!=original:changed[info.filename]={'before_sha256':sha(original),'after_sha256':sha(data)}
                    after.writestr(copy.copy(info),data)
                wrapper=copy.copy(before.getinfo('alt/'+old_root));wrapper.filename='alt/'+new_root
                after.writestr(wrapper,before.read('alt/'+old_root))
            if set(changed)!=DOOR|headers:raise ValueError('unexpected archive delta')
        return {'baseline_sha256':baseline,'candidate_sha256':sha(target.read_bytes()),
                'changed_resources':changed,'added_resource':'alt/'+new_root,'door_anchor':[anchor[0],0,anchor[1]],
                'door_yaw':YAW,'yaw_degrees':YAW*360/65536,'rotation_from_parent_degrees':0 if delta else -22.5,
                'preservation':'Only the five doorway X/Z vertex payloads and four room-zero bindings change. All Y/UV/RGBA, display lists, textures, mushroom/stump repairs and scene geometry are preserved.',
                'status':'Static candidate; matching v'+str(version)+' source required for gameplay. Runtime acceptance pending.'}
    except BaseException:
        if created:target.unlink(missing_ok=True)
        raise


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('source',type=Path);p.add_argument('target',type=Path);p.add_argument('--receipt',type=Path,required=True)
    modes=p.add_mutually_exclusive_group()
    modes.add_argument('--center-left',action='store_true',help='Nudge the POC6 doorway left, retaining its angle')
    modes.add_argument('--left-more',action='store_true',help='Move the POC7 doorway farther left, retaining its angle')
    modes.add_argument('--tiny-right',action='store_true',help='Nudge the POC8 doorway a little right, retaining its angle')
    a=p.parse_args()
    if a.receipt.exists() or a.receipt.resolve() in (a.source.resolve(),a.target.resolve()):raise ValueError('choose a new distinct receipt')
    created=False
    try:
        with a.receipt.open('x') as stream:
            created=True;json.dump(align(a.source,a.target,a.center_left,a.left_more,a.tiny_right),stream,indent=2);stream.write('\n')
    except BaseException:
        if created:a.receipt.unlink(missing_ok=True)
        raise
    print('PASS doorway positioning: nine resources + new identity wrapper, all scenery/textures preserved')


if __name__=='__main__':main()
