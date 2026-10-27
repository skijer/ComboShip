#!/usr/bin/env python3
"""Check production lock geometry, texture scrolling and invalid-frame guards."""
from pathlib import Path
import ctypes as C
import subprocess
import sys
import tempfile
import numpy as np

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/black_market'))
from presentation_preview import triangle_box_intersects

with tempfile.TemporaryDirectory(prefix='lock-mesh-') as temporary:
    library=Path(temporary)/'lock.so'
    subprocess.run(['c++','-std=c++20','-O2','-ffast-math','-shared','-fPIC','-I'+str(ROOT),
                    str(ROOT/'tools/black_market/lock_preview.cpp'),'-o',str(library)],check=True)
    native=C.CDLL(str(library))
    native.BlackMarketLock_Preview.argtypes=[C.POINTER(C.c_float),C.POINTER(C.c_float),C.c_uint,C.c_int,C.POINTER(C.c_float)]
    native.BlackMarketLock_Preview.restype=C.c_int
    def draw(low,high,tick=0,selected=0):
        low=np.array(low,dtype=np.float32); high=np.array(high,dtype=np.float32)
        vertices=np.empty((96,9),dtype=np.float32)
        count=native.BlackMarketLock_Preview(low.ctypes.data_as(C.POINTER(C.c_float)),
            high.ctypes.data_as(C.POINTER(C.c_float)),tick,selected,vertices.ctypes.data_as(C.POINTER(C.c_float)))
        return vertices[:count]
    for low,high in (([-3,-12,-2],[7,16,4]),([-1,-20,-1],[2,10,1]),([-13,-2,-13],[17,3,10])):
        first=draw(low,high); later=draw(low,high,31); selected=draw(low,high,31,1)
        assert len(first)==96 and np.isfinite(first).all()
        assert np.linalg.norm((first[:,:3].min(0)+first[:,:3].max(0))/2-(np.array(low)+high)/2)<.04
        assert not any(triangle_box_intersects(t,np.array(low),np.array(high)) for t in first[:,:3].reshape(-1,3,3))
        assert first[:,2].min()<low[2] and first[:,2].max()>high[2]
        assert np.array_equal(first[:,:3],later[:,:3]) and not np.array_equal(first[:,3:5],later[:,3:5])
        assert np.array_equal(later[:,:5],selected[:,:5]) and selected[:,8].max()<=.65
    for low,high in (([float('nan'),0,0],[1,1,1]),([0,0,0],[float('inf'),1,1]),
                     ([2,0,0],[1,1,1]),([0,0,0],[1000,1,1])):
        assert len(draw(low,high))==0
print('PASS native lock mesh: fitted asymmetric centers, front/back wrap, no model-volume clipping, IA8 UV scrolling and fast-math-safe guards')
