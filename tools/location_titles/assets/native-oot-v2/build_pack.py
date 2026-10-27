#!/usr/bin/env python3
"""Rebuild/verify optional native OoT title art; Python standard library only."""
import argparse
import base64
import hashlib
import io
import json
from pathlib import Path
import re
import stat
import struct
import zipfile
import zlib

ROOT=Path(__file__).resolve().parent
def require(value,message):
    if not value:raise ValueError(message)
def sha(data):return hashlib.sha256(data).hexdigest()

def build(root=ROOT):
    manifest=json.loads((root/'native-title-pack.json').read_text())
    require(manifest['version']==2,'manifest version')
    glyphs=manifest['glyphs']
    require(len(glyphs)==44 and len(set(glyphs))==44,'glyph inventory')
    expected={f'custom/mm_location_titles/oot_native_v1/{ord(ch):02x}' for ch in glyphs}
    expected.add('custom/skiejer_market/title')
    expected.add('custom/mm_location_titles/oot_native_v1/profile')
    require(set(manifest['resources'])==expected,'resource inventory')
    require(all(33<=ord(ch)<127 and ch!='"' for ch in glyphs),'glyph codepoint')
    require(set('ENO&').isdisjoint(glyphs),'unprovided native glyphs')
    packed=[]
    require(0<len(manifest['parts'])<=20,'part count')
    for index,part in enumerate(manifest['parts'],1):
        require(part['name']==f'native-title-pack.zlib.{index:03}.b64','part path/order')
        encoded=(root/part['name']).read_bytes()
        require(len(encoded)==part['base64_bytes'] and len(encoded)<=128*1024 and sha(encoded)==part['base64_sha256'],'base64 size/hash')
        data=base64.b64decode(encoded,validate=True)
        require(len(data)==part['bytes'] and len(data)<=96*1024 and sha(data)==part['sha256'],'part size/hash')
        packed.append(data)
    packed=b''.join(packed)
    require(len(packed)==manifest['packed_bytes'] and sha(packed)==manifest['packed_sha256'],'packed size/hash')
    require(0<manifest['o2r_bytes']<=4*1024*1024,'archive bound')
    decoder=zlib.decompressobj()
    data=decoder.decompress(packed,manifest['o2r_bytes']+1)
    require(len(data)==manifest['o2r_bytes'] and decoder.eof and not decoder.unused_data and not decoder.unconsumed_tail,'archive decompression bounds')
    require(sha(data)==manifest['o2r_sha256'],'archive hash')
    with zipfile.ZipFile(io.BytesIO(data)) as archive:
        require(archive.namelist()==sorted(expected),'archive exact names/order')
        require(archive.testzip() is None,'archive CRC')
        for info in archive.infolist():
            require(info.compress_type==zipfile.ZIP_STORED and info.date_time==(1980,1,1,0,0,0),'archive writer parity')
            require(info.create_system==3 and info.external_attr>>16==(stat.S_IFREG|0o644),'archive member mode')
            body=archive.read(info)
            record=manifest['resources'][info.filename]
            require(len(body)==record['bytes'] and sha(body)==record['sha256'],'resource size/hash')
            require(len(body)>=92 and body[1]==1,'resource header')
            require(struct.unpack_from('<IIQ',body,4)==(0x4F544558,1,0xDEADBEEFDEADBEEF),'native texture type/version')
            kind,width,height,flags,hscale,vscale,size=struct.unpack_from('<IIIIffI',body,64)
            market=info.filename.endswith('/title')
            require((kind,width,height,flags,hscale,vscale)==(1,768 if market else 96,96,1,16,4),'HD raw texture metadata')
            pixels=body[92:]
            require(size==len(pixels)==width*height*4,'pixel payload')
            # Both custom IA8 command paths upload RGBA32 directly: four raw
            # bytes per native byte, 4x across and down, exactly matching stride.
            logical_width,logical_height=(192,24) if market else (24,24)
            require(logical_width*hscale==width*4 and logical_height*vscale==height,'native stride/height contract')
            alpha=pixels[3::4]
            require(any(a>0 for a in alpha) and any(a==0 for a in alpha),'face/transparent alpha')
            edge=[*alpha[:width],*alpha[-width:],*alpha[::width],*alpha[width-1::width]]
            require(not any(edge),'transparent perimeter')
            require(all(pixels[i:i+3]==b'\0\0\0' for i in range(0,len(pixels),4) if pixels[i+3]==0),'zero-alpha color framing')
            if not market:
                require(all(pixels[i:i+3]==b'\xff\xff\xff' for i in range(0,len(pixels),4) if pixels[i+3]),'glyph white faces')
    require(manifest['resources']['custom/skiejer_market/title']['sha256']==
            'a8d76bfa5e44fdd0d349a0f3ec007c46deac130e7468e6edbdb6f5419f446cbe','accepted Black Market bytes')
    with zipfile.ZipFile(io.BytesIO(data)) as archive:
        for ch,metric in manifest['native_metrics'].items():
            pixels=archive.read(f'custom/mm_location_titles/oot_native_v1/{ord(ch):02x}')[92:]
            # Exact 4x replication; native face crop hashes prove source parity
            # without redistributing the complete input game archive.
            alpha=pixels[3::4]
            native=[]
            for y in range(24):
                for x in range(24):
                    block=[alpha[(y*4+dy)*96+x*4+dx] for dy in range(4) for dx in range(4)]
                    require(len(set(block))==1,'lossless native pixel replication')
                    native.append(block[0])
            width=metric['crop'][2]-metric['crop'][0]
            crop=bytes(native[y*24+x] for y in range(24) for x in range(1,width+1))
            require(sha(crop)==metric['face_sha256'],'native source face parity')
    return data,manifest

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path)
    args=parser.parse_args()
    data,manifest=build()
    if args.output:args.output.write_bytes(data)
    print(json.dumps({'status':'PASS','glyphs':manifest['glyph_count'],'resources':len(manifest['resources']),
                      'o2r_bytes':len(data),'o2r_sha256':sha(data)}))
