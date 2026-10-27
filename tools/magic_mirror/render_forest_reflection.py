#!/usr/bin/env python3
"""Render the preserved real Lost Woods room into a fixed Mirror reflection.

Uses the existing packed-mesh Mesa preview, actual scene geometry/UVs/textures,
neutral preview lighting and no actors/HUD. This is not a gameplay screenshot.
"""
import argparse
import ctypes as C
import hashlib
import json
from pathlib import Path
import sys
import struct
from types import SimpleNamespace
from zipfile import ZipFile

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]

def authored_items(scene, source):
    """Decode the real world-space Black Market and portal display lists."""
    sys.path.insert(0, str(ROOT/'tools/black_market'))
    import native_forest as native
    hashes = {native.crc64(name):name for name in source.namelist()}
    textures, vertices, result = {}, {}, []
    sys.path.insert(0,str(ROOT/'scripts/assets'))
    from prepare_native_forest import prepared_resources, DEFAULT_MANIFEST
    presentation = prepared_resources(DEFAULT_MANIFEST)
    dtype=np.dtype([('xyz','<i2',3),('flag','<u2'),('uv','<i2',2),('rgba','u1',4)])

    def upload(path, mask=False):
        key=(path,mask)
        if key in textures: return textures[key]
        raw=source.read(path)
        version=struct.unpack_from('<I',raw,8)[0]
        kind,w,h,field=struct.unpack_from('<4I',raw,64)
        data=raw[92 if version == 1 else 80:]
        if (version == 1 and field & 1) or kind == 1:
            pixels=np.frombuffer(data,np.uint8).reshape(h,w,4).copy()
        elif kind == 2:
            packed=np.frombuffer(data,'>u2').astype(np.uint16)
            pixels=np.column_stack((((packed>>11)&31)*255//31,((packed>>6)&31)*255//31,
                                    ((packed>>1)&31)*255//31,(packed&1)*255)).astype(np.uint8).reshape(h,w,4)
        elif kind == 6:
            intensity=np.frombuffer(data,np.uint8).reshape(h,w)
            pixels=np.full((h,w,4),255,np.uint8)
            if mask: pixels[:,:,3]=intensity
            else: pixels[:,:,:3]=intensity[:,:,None]
        else:
            raise ValueError('Unsupported fixed-scene texture format: '+path)
        tid=scene.uint();scene.GenTextures(1,C.byref(tid));scene.BindTexture(0x0DE1,tid.value)
        for prop,value in ((0x2801,0x2601),(0x2800,0x2601),(0x2802,0x2901),(0x2803,0x2901)):
            scene.TexParameteri(0x0DE1,prop,value)
        scene.TexImage(0x0DE1,0,0x1908,w,h,0,0x1908,0x1401,pixels.ctypes.data)
        textures[key]=tid.value
        return tid.value

    def vertex_rows(path):
        if path not in vertices:
            raw=source.read(path)
            if path.endswith('/recipe7624751371212635_vtx'):
                # Match the current native-v2 sign atlas fix, not its older
                # authoring donor's glyph-side coordinates.
                raw=presentation['custom/comboship/native_forest/building/recipe7624751371212635_vtx']
            if raw.startswith(b'<Vertex'):
                import xml.etree.ElementTree as ET
                vertices[path]=np.array([[float(v.get(k)) for k in ('X','Y','Z','S','T','R','G','B','A')]
                                         for v in ET.fromstring(raw)],dtype=float)
            else:
                kind,count=struct.unpack_from('<II',raw,64)
                assert kind == 25 and len(raw) == 72+16*count
                v=np.frombuffer(raw,dtype,count=count,offset=72)
                vertices[path]=np.column_stack((v['xyz'],v['uv'],v['rgba'])).astype(float)
        return vertices[path]

    def decode(path, portal=False):
        texture=None;size=np.array([32.,32.]);cache={};triangles=[];prim=np.ones(4)
        for offset,w0,w1,op in native.display_commands(source.read(path),path):
            resource=None
            if op in (0x20,0x31,0x32):
                hi,lo=struct.unpack_from('<II',source.read(path),offset+8)
                resource=hashes[(hi<<32)|lo]
            if op == 0x20 and texture is None: texture=resource
            elif op == 0xFA: prim=np.array([(w1>>s)&255 for s in (24,16,8,0)])/255
            elif op == 0xF2 and w1>>24 == 0:
                size=np.array([(((w1>>12)&4095)-((w0>>12)&4095))/4+1,
                               ((w1&4095)-(w0&4095))/4+1])
            elif op == 0x32:
                count=(w0>>12)&255;start=((w0>>1)&127)-count
                rows=vertex_rows(resource)[w1//16:w1//16+count].copy()
                if portal:
                    rows[:,5:8]=prim[:3];rows[:,8]=prim[3]
                else:
                    normals=rows[:,5:8].astype(np.uint8).view(np.int8).astype(float)
                    normals/=np.maximum(np.linalg.norm(normals,axis=1,keepdims=True),1)
                    rows[:,5:8]=(.65+.35*np.maximum(normals@np.array([-.35,.8,-.48]),0))[:,None]
                    rows[:,8]/=255
                for i,row in enumerate(rows): cache[start+i]=row
            elif op in (0x05,0x06,0x26):
                if op == 0x26: indices=[w0&65535,(w1>>16)&65535,w1&65535]
                else: indices=[((word>>s)&255)//2 for word in ((w0,w1) if op == 0x06 else (w0,)) for s in (16,8,0)]
                rows=np.array([cache[i] for i in indices],dtype=np.float32)
                rows[:,3:5]/=size*32
                triangles.extend(rows)
        if triangles:
            result.append((path.rsplit('/',1)[-1], upload(texture,portal),
                           np.ascontiguousarray(triangles,dtype=np.float32),portal))

    for slug in ('recipe3635031522573470','recipe215851318676066','recipe7624751371212635','paste2461349930323740'):
        decode('custom/prelude/Z2_LOST_WOODS/'+slug)
    decode('custom/cor_mm3d/lost_woods_poc1/room_0/ReturnTunnelMouth',True)
    return result

def draw_saria(scene, eye, target):
    path=ROOT/'tools/magic_mirror/reflection/saria-silhouette.png'
    image=Image.open(path).convert('RGBA')
    # Trim only transparent sprite margin using the solid silhouette bounds.
    bounds=image.getchannel('A').point(lambda a:255 if a>=128 else 0).getbbox()
    image=image.crop(bounds)
    pixels=np.ascontiguousarray(image)
    tid=scene.uint();scene.GenTextures(1,C.byref(tid));scene.BindTexture(0x0DE1,tid.value)
    for prop,value in ((0x2801,0x2601),(0x2800,0x2601),(0x2802,0x812F),(0x2803,0x812F)):
        scene.TexParameteri(0x0DE1,prop,value)
    scene.TexImage(0x0DE1,0,0x1908,image.width,image.height,0,0x1908,0x1401,pixels.ctypes.data)
    forward=np.asarray(target,dtype=float)-np.asarray(eye,dtype=float)
    right=np.cross(forward,[0,1,0]);right/=np.linalg.norm(right)
    base=np.array([-630.,0.,-156.]);height=62.;half=height*image.width/image.height/2
    points=[base-right*half,base+right*half,base+right*half+[0,height,0],base-right*half+[0,height,0]]
    uv=[(0,1),(1,1),(1,0),(0,0)]
    rows=np.array([list(points[i])+list(uv[i])+[1,1,1,1] for i in (0,1,2,0,2,3)],dtype=np.float32)
    scene.draw(np.ascontiguousarray(rows),tid.value)
    return dict(source='Reference-derived silhouette sprite; no native actor is spawned',
                sprite_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),position=base.tolist(),height=height)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--expected-source-sha256', help='Optional author-supplied input integrity check')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--eye', type=float, nargs=3, default=(-100, 150, 300))
    parser.add_argument('--target', type=float, nargs=3, default=(-470, 75, 120))
    args = parser.parse_args()
    digest = hashlib.sha256(args.source.read_bytes()).hexdigest()
    if args.expected_source_sha256 and digest != args.expected_source_sha256:
        raise ValueError('Unexpected forest source; resolve its provenance before rebaking')
    sys.path.insert(0, str(ROOT/'tools'))
    import preview_mm_lost_woods as scene

    class SourceView:
        def __init__(self, path): self.archive = ZipFile(path)
        def __enter__(self): return self
        def __exit__(self, *unused): self.archive.close()
        def namelist(self):
            # The fixed view omits the binary animated portal-mouth overlay;
            # the room terrain, root tunnel and mushrooms use the XML path.
            return [n for n in self.archive.namelist() if not n.endswith('/ReturnTunnelMouth')]
        def read(self, name):
            # The exact scene uses base resource names; the older preview also
            # supports an Alt-only input. Resolve its request to the real key.
            if name not in self.archive.namelist() and name.startswith('alt/'):
                name = name[4:]
            return self.archive.read(name)

    scene.zipfile = SimpleNamespace(ZipFile=SourceView)
    items = scene.meshes(args.source, 0)
    with ZipFile(args.source) as source:
        additions = authored_items(scene,source)
    if not items:
        raise ValueError('Forest room zero has no renderable packed meshes')
    scene.ClearColor(0.045, 0.065, 0.075, 1)
    scene.Clear(0x00004000 | 0x00000100)
    scene.camera(args.eye, args.target)
    for translucent in (False, True):
        scene.DepthMask(not translucent)
        for name, texture, array in items:
            if ('PinkLight' in name) == translucent:
                scene.draw(array, texture)
        for name,texture,array,xlu in additions:
            if xlu == translucent:
                scene.draw(array,texture)
    saria=draw_saria(scene,args.eye,args.target)
    scene.DepthMask(True)
    pixels = np.empty((scene.H, scene.W, 4), dtype=np.uint8)
    scene.ReadPixels(0, 0, scene.W, scene.H, 0x1908, 0x1401, pixels.ctypes.data)
    image = Image.fromarray(pixels[::-1].copy(), 'RGBA')
    left = (scene.W-scene.H)//2
    image = image.crop((left, 0, left+scene.H, scene.H)).resize((512, 512), Image.Resampling.LANCZOS)
    image.putalpha(255) # The captured scene is opaque; glass supplies its own alpha.
    args.output.parent.mkdir(parents=True, exist_ok=True)
    image.save(args.output, compress_level=9)
    receipt = dict(room=0, eye=args.eye, target=args.target,
                   source_meshes=len(items)+len(additions), output_sha256=hashlib.sha256(args.output.read_bytes()).hexdigest(),
                   rendering='Actual packed room geometry, UVs and textures with current native sign UVs; neutral Mesa asset lighting; no HUD',
                   saria=saria,
                   omitted_effects=['Runtime particles; animated material scroll frozen at its source phase'],
                   status='Fixed scene view, not a live reflection or gameplay screenshot')
    args.output.with_suffix('.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(receipt, indent=2))

if __name__ == '__main__':
    main()
