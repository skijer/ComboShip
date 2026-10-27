"""Offline default vapor/torch draws from production poses and native MM inputs.

No native pixels are bundled. Filtering and lighting remain preview-only.
Call InteriorEffects(native_mm_path).draws(view, tick) from a room preview.
"""
import ctypes as C
import json
import os
from pathlib import Path
import subprocess
import tempfile
import zipfile

import numpy as np
from native_preview import NativeKeeper, crc64
from granny_preview import native_texture
import preview

ROOT = Path(__file__).resolve().parents[2]


class InteriorEffects:
    def __init__(self, native_mm):
        self.temporary = tempfile.TemporaryDirectory(prefix='market-effects-preview-')
        self.binary = Path(self.temporary.name) / 'poses'
        roots = ('mm', 'mm/include', 'mm/include/PR', 'mm/src', 'mm/assets', 'mm/2s2h', 'libultraship/include', 'combo')
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-O1', '-DF3DEX_GBI_2', '-DCOMBO_BUILD',
                        '-w', '-I'+str(ROOT), *['-I'+str(ROOT/p) for p in roots],
                        str(ROOT/'tools/black_market/interior_effects_preview.cpp'), '-o', str(self.binary)], check=True)
        with zipfile.ZipFile(native_mm) as archive:
            self.cloud, w, h = native_texture(archive, 'objects/gameplay_keep/gEffDust1Tex')
            self.cloud_texture = preview.presentation.upload(preview.gpu, self.cloud, w, h, wrap_s=0x812f, wrap_t=0x812f)
            self.fire, _, _ = native_texture(archive, 'objects/gameplay_keep/gEffFireTex')
            self.mask, _, _ = native_texture(archive, 'objects/gameplay_keep/gEffFireMaskTex')
            decoder = NativeKeeper.__new__(NativeKeeper)
            decoder.archive = archive; decoder.vertices = {}; decoder.display_lists = {}
            decoder.hashes = {crc64(n): n for n in archive.namelist()}
            indices, vertices = [], None
            for op, w0, w1, resource in decoder.commands('objects/gameplay_keep/gEffFire1DL'):
                if op == 0x32: vertices = decoder.vertex_array(resource)
                elif op == 6:
                    indices += [((word >> shift) & 255)//2 for word in (w0,w1) for shift in (16,8,0)]
                if op in (0xfa,0xfb): raise ValueError('Native flame overwrites caller palette')
            self.fire_vertices = np.array([np.r_[v['xyz'],v['uv'],[1,1,1,1]] for v in vertices[indices]],dtype=np.float32)
            self.fire_vertices[:,3:5] /= [32*32,64*32]
        self.flame_texture = None

    def poses(self, frame):
        return json.loads(subprocess.check_output([str(self.binary), str(int(frame))], text=True))

    def draws(self, view, tick=0):
        poses = self.poses(tick)
        eye, at = np.asarray(view[0]), np.asarray(view[1])
        forward = at-eye; forward = forward/np.linalg.norm(forward)
        right = np.cross(forward,[0,1,0]); right /= np.linalg.norm(right)
        up = np.cross(right,forward)
        result = []
        y,x = np.mgrid[:256,:128]; s=(x+.5)/4; t=(y+.5)/4
        tex0=self.fire[np.minimum(t.astype(int),63),np.minimum(s.astype(int),31)]/255
        prim=np.array(poses['prim'])/255; env=np.array(poses['env'][:3])/255
        for torch, (px,py,pz,scale) in enumerate(poses['torches']):
            tex1=self.mask[(t.astype(int)+(int(tick)+torch*17)*5)%128,s.astype(int)%32]/255
            combined=(tex1[:,:,:3]-prim[:3])*(128/255)+tex0[:,:,:3]
            rgb=(prim[:3]-env)*combined+env
            alpha=np.clip(tex0[:,:,3]+(tex1[:,:,3]-1)*(128/255),0,1)*prim[3]
            pixels=np.rint(np.clip(np.dstack((rgb,alpha)),0,1)*255).astype(np.uint8)
            texture=preview.presentation.upload(preview.gpu,pixels,128,256,wrap_s=0x812f,wrap_t=0x812f)
            points=self.fire_vertices.copy()
            points[:,:3]=points[:,0,None]*right*scale+points[:,1,None]*[0,1,0]*scale+[px,py,pz]
            result.append((texture,np.ascontiguousarray(points)))
        ordered=sorted(poses['puffs'],key=lambda p:np.sum((np.array(p[:3])-eye)**2),reverse=True)
        for x,y,z,w,h,alpha in ordered:
            if not alpha: continue
            points=[]
            for sx,sy,u,v in ((-1,-1,0,1),(1,-1,1,1),(1,1,1,0),(-1,1,0,0)):
                p=np.rint(np.array([x,y,z])+right*sx*w+up*sy*h)
                points.append([*p,u,v,218/255,226/255,218/255,alpha/255])
            result.append((self.cloud_texture,np.ascontiguousarray(np.array(points,dtype=np.float32)[[0,1,2,0,2,3]])))
        return result
