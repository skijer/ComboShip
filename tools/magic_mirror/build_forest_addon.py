#!/usr/bin/env python3
"""Build optional forest glass, private cast textures and NEI-shadow Mirror icon."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import tempfile
from zipfile import ZIP_DEFLATED, ZipFile, ZipInfo
from PIL import Image, ImageFilter
from generate import render, load_mesh, ROOT

REFLECTION = 'textures/magic_mirror/gMagicMirrorForestReflectionTex'
ICON = 'textures/icon_item_custom/gItemIconMagicMirrorTex'

def texture(image, kind):
    image = image.convert('RGBA')
    width, height = image.size
    header = bytearray(64)
    struct.pack_into('<BB2xIIQ', header, 0, 0, 1, 0x4F544558, 1, 0xDEADBEEFDEADBEEF)
    bpp = {1:32, 2:16}[kind]
    logical_width = 64 if kind == 2 else 32
    body = struct.pack('<4I2fI', kind, width, height, 1, width/logical_width*32/bpp, height/32, width*height*4)
    return bytes(header)+body+image.tobytes()

def nei_icon(art):
    # Exact fit/shadow parameters from the existing authored NEI companion:
    # logical 32px, offset (2,1.5), blur .3, opacity .96, black.
    art = art.crop(art.getchannel('A').getbbox())
    scale = 16
    left, top, right, bottom = scale, scale, scale*3+.9*scale, scale*3.4
    factor = min((512-left-right)/art.width, (512-top-bottom)/art.height)
    art = art.resize((round(art.width*factor), round(art.height*factor)), Image.Resampling.LANCZOS)
    fitted = Image.new('RGBA', (512,512))
    fitted.alpha_composite(art, (round(left+(512-left-right-art.width)/2), round(top+(512-top-bottom-art.height)/2)))
    mask = fitted.getchannel('A').point(lambda a:min(255,a*4))
    shifted = Image.new('L', (512,512))
    shifted.paste(mask, (32,24))
    shifted = shifted.filter(ImageFilter.GaussianBlur(4.8)).point(lambda a:round(a*.96))
    under = Image.new('RGBA', (512,512)); under.putalpha(shifted)
    return Image.alpha_composite(under, fitted)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--preview-dir', type=Path, required=True)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.preview_dir.mkdir(parents=True, exist_ok=True)
    forest = Image.open(ROOT/'tools/magic_mirror/reflection/forest-scene.png').convert('RGBA')
    with tempfile.TemporaryDirectory(prefix='forest-mirror-export-') as directory:
        executable = Path(directory)/'export'
        subprocess.run(['c++','-std=c++17','-O2',str(Path(__file__).with_name('export_geometry.cpp')),
                        '-o',str(executable)], check=True)
        artwork = render(load_mesh(executable,0),(1024,1024),border=48,roll=-.35)
    icon = nei_icon(artwork.resize((512,512),Image.Resampling.LANCZOS))
    icon.save(args.preview_dir/'MagicMirror_Forest_NEI_Icon.png',compress_level=9)
    data = {REFLECTION:texture(forest,2), ICON:texture(icon,1)}
    for name in ('forest_seal', 'forest_wisp'):
        data['objects/magic_mirror/' + name] = (ROOT/'soh/assets/custom/objects/magic_mirror'/name).read_bytes()
    entries = {prefix+path:raw for prefix in ('','alt/') for path,raw in data.items()}
    with ZipFile(args.output,'w',compression=ZIP_DEFLATED,compresslevel=9) as archive:
        for path,raw in sorted(entries.items()):
            info = ZipInfo(path,(2026,10,26,0,0,0))
            info.create_system=3;info.external_attr=0o100644<<16
            archive.writestr(info,raw,compress_type=ZIP_DEFLATED,compresslevel=9)
    with ZipFile(args.output) as archive:
        assert set(archive.namelist()) == set(entries)
        for path,raw in entries.items():
            actual=archive.read(path)
            assert actual == raw
            kind,w,h,flags,sx,sy,length=struct.unpack_from('<4I2fI',actual,64)
            assert flags == 1 and (w,h,length,len(actual)) == (512,512,1048576,1048668)
            assert (kind,sx,sy) == ((2,16.,16.) if path.endswith('ForestReflectionTex') else (1,16.,16.))
    report = dict(archive=args.output.name,sha256=hashlib.sha256(args.output.read_bytes()).hexdigest(),
                  resources={p:hashlib.sha256(raw).hexdigest() for p,raw in sorted(entries.items())},
                  reflection='Fixed rendered view of the actual preserved Lost Woods room; not live rendering',
                  load_order='Load after the existing NEI Magic Mirror/Gerudo icon companion in both game hosts',
                  requires='Matching probe source build for forest glass/cast/camera/audio; old builds only see the icon',
                  runtime='Unverified configured gameplay; exact resource-envelope and native draw checks only')
    args.output.with_suffix('.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))

if __name__ == '__main__': main()
