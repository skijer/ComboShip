"""Render production song effects with a schematic note for scale.

Native note geometry/lighting are not available here. A muted eighth-note guide
is deliberately labeled. Alpha blending and GI camera are offline approximations.
"""
import argparse
import ctypes as C
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import numpy as np
from PIL import Image, ImageDraw, ImageFont

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
sys.path.insert(0, str(ROOT / 'tools/nei_gi/runtime_preview'))
from gl_context import context, gl, integer, ptr, uint

W, H = 340, 310
context(W, H)
F, D = C.c_float, C.c_double
clear_color = gl('glClearColor', None, F, F, F, F)
clear = gl('glClear', None, uint)
enable = gl('glEnable', None, uint)
blend = gl('glBlendFunc', None, uint, uint)
viewport = gl('glViewport', None, integer, integer, integer, integer)
mode = gl('glMatrixMode', None, uint)
identity = gl('glLoadIdentity', None)
ortho = gl('glOrtho', None, D, D, D, D, D, D)
rotate = gl('glRotatef', None, F, F, F, F)
enable_client = gl('glEnableClientState', None, uint)
disable_client = gl('glDisableClientState', None, uint)
vertices = gl('glVertexPointer', None, integer, uint, integer, ptr)
colors = gl('glColorPointer', None, integer, uint, integer, ptr)
draw = gl('glDrawArrays', None, uint, integer, integer)
read = gl('glReadPixels', None, integer, integer, integer, integer, uint, uint, ptr)
enable(0x0BE2)
blend(0x0302, 0x0303)
clear_color(.035, .045, .06, 1)
font = lambda size: ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf', size)
TITLES = ['Song of Healing', 'Sonata of Awakening', 'Goron Lullaby Intro', 'Goron Lullaby',
          'New Wave Bossa Nova', 'Elegy of Emptiness', 'Oath to Order', 'Inverted Song of Time', 'Song of Double Time']
CAPTIONS = ['Pink hearts + spirit trails', 'Green leaves + seeds', 'Three varied musical notes', 'Six varied musical notes',
            'Blue bubbles + three staggered ripples', 'Amber dust + #FF6200 shimmer', 'Violet glints + #620062 shimmer',
            'Counterclockwise dial + motes', 'Paired hourglasses + sand']


def submit(data):
    if not len(data): return
    enable_client(0x8074); enable_client(0x8076)
    vertices(3, 0x1406, 16, data.ctypes.data)
    colors(4, 0x1401, 16, data.ctypes.data + 12)
    draw(0x0004, 0, len(data) // 16)
    disable_client(0x8074); disable_client(0x8076)


def note_guide():
    # A guide, not a substitute claim about the native N64 music-note mesh.
    triangles = []
    def tri(a,b,c): triangles.extend([a,b,c])
    def rect(x0,y0,x1,y1):
        tri((x0,y0,0),(x1,y0,0),(x1,y1,0));tri((x0,y0,0),(x1,y1,0),(x0,y1,0))
    rect(3,-7,5,15)
    tri((5,15,0),(12,11,0),(5,7,0))
    for j in range(24):
        a,b=j*np.pi/12,(j+1)*np.pi/12
        tri((0,-8,0),(6*np.cos(a),-8+3.7*np.sin(a),0),(6*np.cos(b),-8+3.7*np.sin(b),0))
    dtype=np.dtype([('position','<f4',3),('rgba','u1',4)])
    packed=np.zeros(len(triangles),dtype=dtype)
    packed['position']=triangles;packed['rgba']=(102,116,134,125)
    return packed.view('u1')


GUIDE = note_guide()


def render(layers, label, caption):
    clear(0x4000);viewport(0,0,W,H)
    mode(0x1701);identity();ortho(-40,40,-36,36,-200,200)
    mode(0x1700);identity();rotate(12,1,0,0);rotate(25,0,1,0)
    submit(GUIDE)
    for data in layers: submit(data)
    pixels=np.zeros((H,W,4),dtype='u1')
    read(0,0,W,H,0x1908,0x1401,pixels.ctypes.data)
    tile=Image.new('RGB',(W,H+58),'#131d27')
    tile.paste(Image.fromarray(pixels[::-1].copy()).convert('RGB'),(0,58))
    text=ImageDraw.Draw(tile)
    text.text((12,7),label,font=font(18),fill='#edf4fb')
    text.text((12,33),caption,font=font(13),fill='#a7bccd')
    return tile


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--out',type=Path,required=True)
    args=p.parse_args();args.out.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='mm-song-preview-') as temp:
        binary=Path(temp)/'export';data=Path(temp)/'triangles.bin'
        subprocess.run(['c++','-std=c++20','-O2','-I'+str(ROOT/'soh/soh/Enhancements/randomizer'),
                        '-I'+str(ROOT/'combo/menu'),str(HERE/'export_preview.cpp'),'-o',str(binary)],check=True)
        subprocess.run([str(binary),str(data)],check=True)
        packed=data.read_bytes()
    frames=[];offset=0;budgets={}
    for frame in range(80):
        songs=[]
        for song in range(9):
            layers=[]
            for layer in range(2):
                count=struct.unpack_from('<I',packed,offset)[0];offset+=4
                layers.append(np.frombuffer(packed[offset:offset+16*count],dtype='u1').copy());offset+=16*count
                budgets[TITLES[song]+(' shimmer' if layer==0 else ' particles')]=count
            songs.append(layers)
        frames.append(songs)
    assert offset==len(packed)
    def sheet(frame,indices,columns,title):
        rows=(len(indices)+columns-1)//columns
        page=Image.new('RGB',(W*columns,104+(H+58)*rows),'#131d27')
        text=ImageDraw.Draw(page)
        text.text((14,10),title,font=font(23),fill='#edf4fb')
        text.text((14,46),'Native clef unchanged · gray shape = preview guide ONLY',font=font(14),fill='#f2dba4')
        text.text((14,70),'Production effect meshes · offline preview',font=font(14),fill='#a7bccd')
        for n,song in enumerate(indices):
            page.paste(render(frames[frame][song],TITLES[song],CAPTIONS[song]),((n%columns)*W,104+(n//columns)*(H+58)))
        return page
    sheet(14,list(range(9)),3,'MM song GI effects · POC2').save(args.out/'MM_Song_GI_Overview_POC2.png')
    for name,indices,cols,title in [
        ('MM_Lullaby_Bossa_POC2.gif',[2,3,4],3,'Lullaby and Bossa Nova · revised effects'),
        ('MM_Elegy_Oath_POC2.gif',[5,6],2,'Elegy and Oath · dust and glints')]:
        animation=[sheet(frame,indices,cols,title) for frame in range(80)]
        animation[0].save(args.out/name,save_all=True,append_images=animation[1:],duration=100,loop=0)
        print(args.out/name,flush=True)
    (args.out/'preview_metadata.json').write_text(json.dumps({'vertices':budgets,'frames':80,'sample_step':3,
        'note':'native clef unchanged; gray shape is only a schematic scale guide',
        'runtime':'offline; not captured or accepted'},indent=2)+'\n')


if __name__=='__main__':main()
