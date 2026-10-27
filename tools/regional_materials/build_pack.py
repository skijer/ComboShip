#!/usr/bin/env python3
"""Build optional drop-in regional textures; never modifies a game archive."""
import argparse
import json
from pathlib import Path
import struct
import zipfile
from PIL import Image

# key, stable path suffix, native logical width/height, native type/bytes, debug color
MATERIALS = (
 ('LakeStone','lake_hylia/stone',32,64,2,2,(135,80,195)),
 ('LakeSand','lake_hylia/sand',32,32,2,2,(190,165,112)),
 ('SnowheadStone','snowhead/stone',32,32,2,2,(80,125,175)),
 ('SnowheadSnow','snowhead/ascent_snow',32,32,2,2,(230,245,255)),
 ('SnowheadIce','snowhead/ice',32,32,2,2,(50,190,225)),
 ('ValleySnow','snowhead/valley_snow',32,32,2,2,(195,220,250)),
 ('LakeWater','lake_hylia/water',64,64,6,1,(35,135,205)),
)
PREFIX='textures/comboship/regional/'

def resource(image,width,height,typ,native_bytes):
 image=image.convert('RGBA')
 if image.width%width or image.height%height:
  raise ValueError(f'image {image.size} must be an integer multiple of logical tile {width}x{height}')
 if image.width>4096 or image.height>4096:raise ValueError('image dimensions exceed 4096')
 header=bytearray(64)
 struct.pack_into('<IIQ',header,4,0x4F544558,1,0xDEADBEEFDEADBEEF)
 # Preserve the sampler's native type; Flags=RAW selects RGBA32 image bytes.
 return bytes(header)+struct.pack('<IIIIffI',typ,image.width,image.height,1,
     image.width*4/(width*native_bytes),image.height/height,image.width*image.height*4)+image.tobytes()

def build(output,input_dir=None,scale=4):
 files={};manifest=[]
 for key,suffix,w,h,typ,bpp,color in MATERIALS:
  if input_dir:
   path=Path(input_dir)/(key+'.png')
   if not path.is_file():continue
   image=Image.open(path).convert('RGBA')
  else:
   image=Image.new('RGBA',(w*scale,h*scale))
   image.putdata([(*tuple(min(255,c+(18 if ((x//(8*scale))+(y//(8*scale)))%2 else -18))for c in color),255)
                  for y in range(image.height)for x in range(image.width)])
  files[PREFIX+suffix]=resource(image,w,h,typ,bpp)
  manifest.append(dict(material=key,path=PREFIX+suffix,logical_size=[w,h],image_size=list(image.size),native_type=typ))
 if not files:raise ValueError('no recognized material PNG files')
 files['regional-materials.json']=(json.dumps(manifest,indent=2,sort_keys=True)+'\n').encode()
 output=Path(output);output.parent.mkdir(parents=True,exist_ok=True)
 with zipfile.ZipFile(output,'w',compression=zipfile.ZIP_STORED)as z:
  for name,data in sorted(files.items()):
   info=zipfile.ZipInfo(name,date_time=(1980,1,1,0,0,0));info.external_attr=0o100644<<16;info.create_system=3
   z.writestr(info,data)
 return len(manifest)

def main():
 p=argparse.ArgumentParser(description=__doc__);mode=p.add_mutually_exclusive_group(required=True)
 mode.add_argument('--input-dir',type=Path);mode.add_argument('--debug-swatches',action='store_true')
 p.add_argument('--scale',type=int,default=4);p.add_argument('--output',required=True,type=Path)
 a=p.parse_args()
 if not 1<=a.scale<=32:p.error('--scale must be 1..32')
 print(f'Wrote {build(a.output,a.input_dir,a.scale)} regional replacement textures: {a.output}')
if __name__=='__main__':main()
