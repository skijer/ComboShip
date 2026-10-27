#!/usr/bin/env python3
"""Check the preview evaluates the native flame combiner, not its raw mask."""
from pathlib import Path
import argparse,sys,zipfile
import numpy as np

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/black_market'))
import granny_preview as room

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--native-oot',type=Path,required=True)
a=p.parse_args()
uploaded=[]
def upload(gpu,pixels,width,height,**kwargs):
    uploaded.append(pixels.copy());return len(uploaded)-1
room.preview.presentation.upload=upload
with zipfile.ZipFile(a.native_oot) as native:
    room.native_effect(native,'objects/gameplay_keep/gEffFire1DL',position=(130,76,-8),scale=.0075)
    pixels=uploaded[-1]
    assert pixels.shape==(256,128,4),'the raw 32x128 mask is not the final two-texture flame'
    flame,_,_=room.native_texture(native,'objects/gameplay_keep/gDecorativeFlameTex')
    mask,_,_=room.native_texture(native,'objects/gameplay_keep/gDecorativeFlameMaskTex')
    # Sample at UV texel centers. Native FC272C60/350CE37F alpha is
    # clamp(TEXEL0 + (TEXEL1 - 1) * PRIM_LOD_FRAC) * PRIM_ALPHA.
    for x,y in [(10,10),(62,94),(86,174),(110,230)]:
        s=(x+.5)/4;t=(y+.5)/4
        sample=lambda image: image[min(int(t),len(image)-1),min(int(s),image.shape[1]-1),3]/255
        expected=max(0,min(1,sample(flame)+(sample(mask)-1)*128/255))*255
        assert abs(int(pixels[y,x,3])-round(expected))<=1,(x,y,pixels[y,x,3],expected)
        prim=np.array([1,200/255,0]);env=np.array([1,0,0])
        expected_rgb=(prim-env)*((sample(mask)-prim)*128/255+sample(flame))+env
        expected_rgb=np.rint(np.clip(expected_rgb,0,1)*255)
        assert np.all(np.abs(pixels[y,x,:3].astype(int)-expected_rgb)<=1)
    assert np.count_nonzero(pixels[:,:,3]==0)>pixels.shape[0]*pixels.shape[1]*.4
    room.native_effect(native,'objects/gameplay_keep/gEffFire1DL',tick=7,position=(130,76,-8),scale=.0075)
    assert not np.array_equal(uploaded[-1],pixels),'native mask must scroll independently of the clamped flame texture'
print('PASS flame preview: native two-texture alpha/color, clamp/wrap and transparent mask exterior')
