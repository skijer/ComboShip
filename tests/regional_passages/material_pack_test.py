#!/usr/bin/env python3
from pathlib import Path
import hashlib, subprocess, tempfile, zipfile, struct, sys
ROOT=Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix='regional-pack-') as td:
 td=Path(td);a=td/'a.o2r';b=td/'b.o2r'
 for p in (a,b):subprocess.run([sys.executable,str(ROOT/'tools/regional_materials/build_pack.py'),'--debug-swatches','--scale','4','--output',str(p)],check=True)
 assert a.read_bytes()==b.read_bytes(),'archive must be deterministic'
 with zipfile.ZipFile(a) as z:
  paths=[n for n in z.namelist()if n.startswith('textures/')]
  assert len(paths)==7
  for path in paths:
   data=z.read(path); assert struct.unpack_from('<II',data,4)==(0x4F544558,1)
   typ,w,h,flags,hs,vs,size=struct.unpack_from('<IIIIffI',data,64)
   nw,nh=(32,64)if path.endswith('lake_hylia/stone') else (64,64)if path.endswith('lake_hylia/water')else(32,32)
   nativebytes=1 if path.endswith('/water')else 2
   assert (w,h,flags,size)==(nw*4,nh*4,1,w*h*4)
   assert hs==4*4/nativebytes and vs==4
   assert nw*nativebytes*hs==w*4 and nh*vs==h and len(data)==92+size
   assert all(data[i]==255 for i in range(95,len(data),4))
 print('PASS deterministic seven-material O2R: stable base paths, native logical tiles, raw RGBA32 metadata/HD byte strides')
