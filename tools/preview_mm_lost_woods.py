#!/usr/bin/env python3
"""Render packed forest vertices and image-row UVs with neutral preview lighting.

Uses system Mesa/EGL via the repository's existing context helper. These are
asset inspections, not game captures, collision proof or exact game lighting.
"""
import argparse
import ctypes as C
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET
import zipfile

import numpy as np
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/nei_gi/runtime_preview'))
from gl_context import context, gl, ptr, integer, uint

W, H = 1050, 760
F, D = C.c_float, C.c_double
context(W, H)
Clear = gl('glClear', None, uint)
ClearColor = gl('glClearColor', None, F, F, F, F)
Enable = gl('glEnable', None, uint)
Disable = gl('glDisable', None, uint)
MatrixMode = gl('glMatrixMode', None, uint)
LoadIdentity = gl('glLoadIdentity', None)
MultMatrix = gl('glMultMatrixf', None, ptr)
Frustum = gl('glFrustum', None, D, D, D, D, D, D)
BindTexture = gl('glBindTexture', None, uint, uint)
TexParameteri = gl('glTexParameteri', None, uint, uint, integer)
TexImage = gl('glTexImage2D', None, uint, integer, integer, integer, integer, integer, uint, uint, ptr)
GenTextures = gl('glGenTextures', None, integer, ptr)
EnableClient = gl('glEnableClientState', None, uint)
DisableClient = gl('glDisableClientState', None, uint)
VertexPointer = gl('glVertexPointer', None, integer, uint, integer, ptr)
TexCoordPointer = gl('glTexCoordPointer', None, integer, uint, integer, ptr)
ColorPointer = gl('glColorPointer', None, integer, uint, integer, ptr)
DrawArrays = gl('glDrawArrays', None, uint, integer, integer)
ReadPixels = gl('glReadPixels', None, integer, integer, integer, integer, uint, uint, ptr)
DepthMask = gl('glDepthMask', None, C.c_ubyte)
gl('glBlendFunc', None, uint, uint)(0x0302, 0x0303)
gl('glAlphaFunc', None, uint, F)(0x0204, 0.015)
Enable(0x0B71)
Enable(0x0BE2)
Enable(0x0BC0)
font = ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf', 18)


def meshes(pack, room=0):
    result, textures = [], {}
    with zipfile.ZipFile(pack) as z:
        for name in z.namelist():
            if f'/room_{room}/' not in name or name.endswith('_vtx') or not name+'_vtx' in z.namelist():
                continue
            dl = ET.fromstring(z.read(name))
            vertex = ET.fromstring(z.read(name+'_vtx'))
            rows = np.array([[float(v.get(c)) for c in ('X','Y','Z','S','T','R','G','B','A')]
                             for v in vertex], dtype=np.float32)
            texture = dl.find('SetTextureImage')
            texture_id = None
            if texture is not None:
                image_name = 'alt/' + texture.get('Path')
                tile = dl.find('SetTileSize')
                rows[:, 3] /= (int(tile.get('Lrs')) // 4 + 1) * 32
                rows[:, 4] /= (int(tile.get('Lrt')) // 4 + 1) * 32
                if image_name not in textures:
                    data = z.read(image_name)
                    _, width, height, _ = struct.unpack_from('<4I', data, 64)
                    pixels = np.frombuffer(data[92:], dtype=np.uint8).copy()
                    tid = uint()
                    GenTextures(1, C.byref(tid))
                    BindTexture(0x0DE1, tid.value)
                    for prop, value in [(0x2801,0x2601),(0x2800,0x2601),(0x2802,0x2901),(0x2803,0x2901)]:
                        TexParameteri(0x0DE1, prop, value)
                    # No image flip: row zero is sampled by T=0, like the host.
                    TexImage(0x0DE1, 0, 0x1908, width, height, 0, 0x1908, 0x1401, pixels.ctypes.data)
                    textures[image_name] = tid.value
                texture_id = textures[image_name]
            rows[:, 5:8] = 0.25 + rows[:, 5:8] / 255 * 0.75 if texture is not None else rows[:, 5:8] / 255
            rows[:, 8] /= 255
            cache, triangles = {}, []
            for command in dl:
                if command.tag == 'LoadVertices':
                    first = int(command.get('VertexBufferIndex'))
                    offset = int(command.get('VertexOffset'))
                    cache.update({first+i: offset+i for i in range(int(command.get('Count')))})
                elif command.tag in ('Triangle1', 'Triangle2'):
                    for slot in range(1 if command.tag == 'Triangle1' else 2):
                        triangles.append([cache[int(command.get(f'V{slot}{i}'))] for i in range(3)])
            array = np.ascontiguousarray(rows[np.array(triangles)].reshape(-1, 9))
            result.append((Path(name).name, texture_id, array))
    return result


def draw(array, texture):
    if texture is None:
        Disable(0x0DE1)
    else:
        Enable(0x0DE1)
        BindTexture(0x0DE1, texture)
    for option in (0x8074,0x8076,0x8078):
        EnableClient(option)
    VertexPointer(3, 0x1406, 36, array.ctypes.data)
    TexCoordPointer(2, 0x1406, 36, array.ctypes.data+12)
    ColorPointer(4, 0x1406, 36, array.ctypes.data+20)
    DrawArrays(0x0004, 0, len(array))
    for option in (0x8074,0x8076,0x8078):
        DisableClient(option)


def camera(eye, target):
    eye, target = np.array(eye,dtype=float), np.array(target,dtype=float)
    forward = target-eye
    forward /= np.linalg.norm(forward)
    right = np.cross(forward, [0,1,0])
    right /= np.linalg.norm(right)
    up = np.cross(right, forward)
    matrix = np.eye(4, dtype=np.float32)
    matrix[:3,:3] = np.array([right,up,-forward])
    matrix[:3,3] = -matrix[:3,:3] @ eye
    MatrixMode(0x1701)
    LoadIdentity()
    near, far = 1.0, 9000.0
    half = np.tan(np.deg2rad(45)/2) * near
    Frustum(-half*W/H, half*W/H, -half, half, near, far)
    MatrixMode(0x1700)
    LoadIdentity()
    column_major = np.ascontiguousarray(matrix.T)
    MultMatrix(column_major.ctypes.data)


def finish_frames(anchor=(-390,120),yaw=16384):
    source = r'''
#include "mm/2s2h/Enhancements/Graphics/ComboForestPortalEffect.h"
#include <cstdio>
int main(){for(unsigned frame=0;frame<72;frame+=6){
std::array<ComboForestPortalEffect::Vertex,ComboForestPortalEffect::kVertexCount> v{};
ComboForestPortalEffect::Build(frame,true,v); std::printf("[");
for(unsigned i=0;i<v.size();++i)std::printf("%s[%f,%f,%u,%u,%u,%u]",i?",":"",v[i].x,v[i].y,v[i].r,v[i].g,v[i].b,v[i].a);
std::puts("]");}}
'''
    with tempfile.TemporaryDirectory(prefix='forest-finish-export-') as temp:
        p = Path(temp)
        (p/'effect.cpp').write_text(source)
        subprocess.run(['c++','-std=c++20','-I'+str(ROOT),str(p/'effect.cpp'),'-o',str(p/'effect')], check=True)
        data = subprocess.check_output([str(p/'effect')], text=True)
    frames = []
    angle=yaw*np.pi/32768;s,c=np.sin(angle),np.cos(angle)
    for line in data.splitlines():
        vertices = np.array(json.loads(line), dtype=np.float32)
        rows = []
        for i in range(26):
            rows.extend(vertices[[4*i+j for j in (0,1,2,0,2,3)]])
        for i in range(6):
            start = 104 + i*5
            rows.extend(vertices[[start+j for j in (0,1,2,0,2,3,0,3,4,0,4,1)]])
        rows = np.array(rows, dtype=np.float32)
        array = np.zeros((len(rows),9), dtype=np.float32)
        # Same yaw/anchor and s16 conversion as the submitted native vertices.
        u=rows[:,0].astype(np.int16)
        array[:,:3] = np.column_stack((anchor[0]+u*c+10*s,rows[:,1].astype(np.int16),anchor[1]-u*s+10*c))
        array[:,5:] = rows[:,2:] / 255
        frames.append(np.ascontiguousarray(array))
    return frames


def render(items, view, label, omit_door=False, finish=None, camera_override=None):
    views = {'stump': ((-310,240,330),(-520,65,120)),
             'grass': ((-160,100,410),(-310,28,150)),
             'corner': ((-100,150,300),(-470,75,120)),
             'layout': ((-150,1550,700),(-120,0,80))}
    ClearColor(0.045,0.065,0.075,1)
    Clear(0x00004000 | 0x00000100)
    camera(*(camera_override or views[view]))
    for translucent in (False, True):
        DepthMask(not translucent)
        for name, texture, array in items:
            if ('PinkLight' in name) != translucent or (omit_door and name.startswith('Return')):
                continue
            if view == 'layout':
                triangles = array.reshape(-1,3,9)
                array = np.ascontiguousarray(triangles[triangles[:,:,1].max(1)<220].reshape(-1,9))
            draw(array, texture)
    if finish is not None and not omit_door:
        draw(finish, None)
    DepthMask(True)
    pixels = np.empty((H,W,4), dtype=np.uint8)
    ReadPixels(0,0,W,H,0x1908,0x1401,pixels.ctypes.data)
    image = Image.fromarray(pixels[::-1].copy()).convert('RGB')
    d = ImageDraw.Draw(image)
    d.rectangle((0,0,W,34), fill=(18,24,30))
    d.text((12,7),label,font=font,fill='white')
    d.rectangle((0,H-32,W,H),fill=(18,24,30))
    d.text((12,H-27),'Packed asset render • neutral lighting • not in-game proof',font=font,fill='#c3cdd4')
    return image


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--baseline',type=Path,required=True)
    parser.add_argument('--candidate',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    modes=parser.add_mutually_exclusive_group()
    modes.add_argument('--edge-seams',action='store_true',help='Compare the POC4a archive with the POC5 room-edge/mushroom follow-up')
    modes.add_argument('--aligned',action='store_true',help='Compare the POC5 doorway with the POC6 trunk-aligned doorway')
    modes.add_argument('--center-left',action='store_true',help='Compare the POC6 doorway with the POC7 leftward adjustment')
    modes.add_argument('--left-more',action='store_true',help='Compare the POC7 doorway with the larger POC8 leftward adjustment')
    modes.add_argument('--tiny-right',action='store_true',help='Compare the POC8 doorway with the small POC9 rightward adjustment')
    args = parser.parse_args()
    args.output.mkdir(parents=True,exist_ok=True)
    if args.aligned or args.center_left or args.left_more or args.tiny_right:
        labels=('POC6 previous position','POC7 shifted left on trunk') if args.center_left else ('POC5 previous angle','POC6 aligned with trunk face')
        anchor=(-684,-340) if args.center_left else (-677,-357)
        prefix='centered' if args.center_left else 'aligned'
        if args.left_more:
            labels=('POC7 previous position','POC8 moved farther left');anchor=(-698,-306);prefix='recentered'
        elif args.tiny_right:
            labels=('POC8 previous position','POC9 tiny nudge right');anchor=(-695,-313);prefix='nudged'
        before,after=meshes(args.baseline),meshes(args.candidate)
        for name,eye,target in [('close',(-270,195,-170),(-690,65,-350)),('wide',(280,390,-70),(-630,30,-60))]:
            sheet=Image.new('RGB',(W*2,H))
            for i,(items,label) in enumerate([(before,labels[0]),(after,labels[1])]):
                sheet.paste(render(items,'corner',label,camera_override=(eye,target)),(i*W,0))
            sheet.save(args.output/(name+'-comparison.png'))
        eye,target=(-270,195,-170),(-690,65,-350)
        frames=[render(after,'corner',labels[1]+' + native effect sample',finish=f,camera_override=(eye,target))
                for f in finish_frames(anchor,12288)]
        frames[0].save(args.output/(prefix+'-finish.png'))
        frames[0].save(args.output/(prefix+'-finish.gif'),save_all=True,append_images=frames[1:],duration=200,loop=0)
        print('PASS positioned doorway previews: packed assets and matching native finish geometry; no game-runtime claim')
        return
    if args.edge_seams:
        views=[('portal',0,(280,390,-70),(-630,30,-60)),
               ('mushrooms',1,(1944,237,-4),(2360,225,-4)),
               ('mushroom-close',1,(2130,310,-100),(2306,248,-185))]
        for name,room,eye,target in views:
            sheet=Image.new('RGB',(W*2,H))
            for i,(pack,label) in enumerate([(args.baseline,'POC4a archive'),(args.candidate,'POC5 room-edge / mushroom seams')]):
                sheet.paste(render(meshes(pack,room),'corner',label,camera_override=(eye,target)),(i*W,0))
            sheet.save(args.output/(name+'-comparison.png'))
        items=meshes(args.candidate)
        eye,target=(-270,195,-170),(-690,65,-350)
        frames=[render(items,'corner','POC5 native trim/shimmer geometry sample',finish=f,camera_override=(eye,target))
                for f in finish_frames((-660,-350))]
        frames[0].save(args.output/'portal-finish.png')
        frames[0].save(args.output/'portal-finish.gif',save_all=True,append_images=frames[1:],duration=200,loop=0)
        print('PASS POC5 previews: packed room-zero/one assets and native finish geometry; no game-runtime claim')
        return
    before, after = meshes(args.baseline), meshes(args.candidate)
    effects = finish_frames()
    for view in ('stump','grass','layout'):
        images = [render(before,view,'POC3 baseline',omit_door=view!='layout'),
                  render(after,view,'POC4 candidate',omit_door=view!='layout',finish=effects[0])]
        sheet = Image.new('RGB',(W*2,H))
        sheet.paste(images[0],(0,0));sheet.paste(images[1],(W,0))
        sheet.save(args.output/(view+'-comparison.jpg'),quality=92)
    render(after,'corner','POC4 • corner-stump doorway + native purple finish',finish=effects[0]).save(args.output/'corner-doorway.png')
    frames = [render(after,'corner','POC4 • native finish sampled over 72 gameplay frames',finish=f) for f in effects]
    frames[0].save(args.output/'corner-finish.gif',save_all=True,append_images=frames[1:],duration=200,loop=0)
    print('PASS preview: original packed vertices/UVs/textures and exported native effect geometry; no image flip')


if __name__ == '__main__':
    main()
