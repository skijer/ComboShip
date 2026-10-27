"""Verify approved bottle source bytes, packaged triangles/pixels and shimmer policies."""
from pathlib import Path
from collections import Counter
import hashlib
import json
import os
import re
import struct
import subprocess
import tempfile
import xml.etree.ElementTree as ET

import numpy as np
from PIL import Image

from run_mm_scene_randomization_tests import function

ROOT = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix='bottle-polish-') as temp:
    source = Path(temp) / 'test.cpp'
    source.write_text('#include <cassert>\n#include "combo/menu/ComboBottleContents.h"\nint main(){unsigned char color[4]{};assert(ComboBottleContents_Profile("__OTR__@mm:objects/combo_bottle_gi/SeahorseBottle")==4);assert(ComboBottleContents_Color(4,color));assert(color[0]==255 && color[1]==230 && color[2]==109);}')
    binary = Path(temp) / 'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-I'+str(ROOT),str(source),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
    enum=re.search(r'typedef enum \{\s*RI_UNKNOWN,.*?\} RandoItemId;', (ROOT/'mm/2s2h/Rando/Types.h').read_text(),re.S)[0]
    source.write_text('#include <cassert>\n#include <cstdint>\n'+enum+'\n#include "combo/menu/ComboOotBottleShimmerMM.h"\nint main(){uint8_t color[4]{};assert(MM_OotBottleShimmerColor(RI_OOT_BOTTLE_MAGIC_MUSHROOM,color));assert(color[0]==222 && color[1]==100 && color[2]==245);assert(MM_OotBottleShimmerColor(RI_OOT_BOTTLE_FAIRY,color));assert(color[0]==255 && color[1]==160 && color[2]==235);}')
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-I'+str(ROOT),str(source),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',
                    '-I'+str(ROOT),'-I'+str(ROOT/'combo/menu'),str(ROOT/'tests/bottle_gi/polish_policy_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)

SOURCE = ROOT/'tools/nei_gi/SOURCE/bottle_polish_20261008'
approved = json.loads((SOURCE/'approved-inputs.json').read_text())
report = json.loads((SOURCE/'resources.json').read_text())
digest = lambda data: hashlib.sha256(data).hexdigest()
for path, expected in approved['models'].items():
    assert digest((SOURCE/path).read_bytes()) == expected, (path,'accepted model bytes changed')
for host in ('soh','mm'):
    assets = ROOT/host/'assets/custom'
    for path, expected in approved['casing_and_markers'].items():
        assert digest((assets/path).read_bytes()) == expected, (host,path,'protected casing bytes changed')
    for path, expected in report['resources'].items():
        assert digest((assets/path).read_bytes()) == expected, (host,path,'serialized resources changed')
contents = (ROOT/'combo/menu/ComboBottleContentsDraw.h').read_text()
assert digest(function(contents,'ComboBottleContents_Gold').strip().encode()) == approved['gold_function_sha256'], 'gold motion function changed'


def face_key(points):
    points = tuple(points)
    return min(points[i:]+points[:i] for i in range(3))


for slug, basename, triangle_count in [('mushroom','Magic_Mushroom',2688),('princess','princess',1001),('seahorse','seahorse',578)]:
    source = SOURCE/'models'/slug/(basename+'.obj')
    p, uv, normal, maps, expected = [], [], [], {}, Counter()
    material = None
    for line in source.with_suffix('.mtl').read_text().splitlines():
        fields = line.split()
        if fields[0]=='newmtl': material=fields[1]
        elif fields[0]=='map_Kd': maps[material]=source.parent/fields[1]
    for line in source.read_text().splitlines():
        fields = line.split()
        if not fields: continue
        if fields[0]=='v': p.append(tuple(round(float(x)*256) for x in fields[1:4]))
        elif fields[0]=='vt': uv.append((round(float(fields[1])*1024),round((1-float(fields[2]))*1024)))
        elif fields[0]=='vn':
            n=np.asarray([float(x) for x in fields[1:4]])
            normal.append(tuple(np.rint(n/max(np.linalg.norm(n),1e-12)*127).astype(int)&255))
        elif fields[0]=='usemtl': material=fields[1]
        elif fields[0]=='f':
            refs=[tuple(int(x)-1 for x in token.split('/')) for token in fields[1:]]
            for i in range(1,len(refs)-1):
                expected[(material,face_key([p[a]+uv[b]+normal[c]+(255,) for a,b,c in [refs[0],refs[i],refs[i+1]]]))]+=1
    assert sum(expected.values())==triangle_count
    base='objects/combo_bottle_gi/polish/'+slug+'/'
    assets=ROOT/'soh/assets/custom'
    data=(assets/(base+'scale_mtx')).read_bytes()
    assert len(data)==128 and struct.unpack_from('<I',data,4)[0]==0x4F4D5458
    words=struct.unpack_from('<16I',data,64)
    fixed=[((((words[i]>>shift)&65535)<<16)|((words[i+8]>>shift)&65535))/65536 for i in range(8) for shift in (16,0)]
    assert fixed==[1/256 if i in (0,5,10) else 1 if i==15 else 0 for i in range(16)]
    records=[tuple(int(v.get(k)) for k in ('X','Y','Z','S','T','R','G','B','A'))
             for v in ET.parse(assets/(base+'mesh_opa_vtx')).getroot()]
    dl=ET.parse(assets/(base+'gi_dl')).getroot()
    actual,cache,depth,loads=Counter(),{},0,0
    material=None
    render_mode=None
    for cmd in dl:
        if 'Path' in cmd.attrib:
            assert cmd.get('Path').startswith(base) and (assets/cmd.get('Path')).is_file()
        if cmd.tag=='SetRenderMode': render_mode=cmd.get('Mode2')
        if cmd.tag=='Matrix':
            assert cmd.get('Param')=='G_MTX_PUSH';depth+=1
        elif cmd.tag=='PopMatrix': depth-=1;assert depth>=0
        elif cmd.tag=='LoadTextureBlock':
            material=cmd.get('Path')[len(base):-len('_tex')]
            rgba=np.asarray(Image.open(maps[material]).convert('RGBA'))
            texture=(assets/cmd.get('Path')).read_bytes()
            kind,w,h,flags,sx,sy,size=struct.unpack_from('<IIIIffI',texture,64)
            assert (kind,w,h,flags,sx,sy,size)==(1,rgba.shape[1],rgba.shape[0],1,rgba.shape[1]/32,rgba.shape[0]/32,rgba.size)
            assert texture[92:]==rgba.tobytes(), (slug,material,'approved texture pixels changed')
            assert cmd.get('Width')==cmd.get('Height')=='32'
            assert cmd.get('CMT_TXClamp' if slug=='mushroom' else 'CMT_TXWrap')=='1'
        elif cmd.tag=='LoadVertices':
            offset,count,start=(int(cmd.get(k)) for k in ('VertexOffset','Count','VertexBufferIndex'))
            assert 0<count<=32 and start+count<=32 and offset+count<=len(records)
            cache={start+i:records[offset+i] for i in range(count)};loads+=1
        elif cmd.tag in ('Triangle1','Triangles2'):
            assert render_mode==('G_RM_AA_ZB_XLU_SURF2' if slug=='princess' else 'G_RM_AA_ZB_OPA_SURF2'), (slug,'donor texture alpha needs the correct host pass')
            fields=[('V00','V01','V02')]+([('V10','V11','V12')] if cmd.tag=='Triangles2' else [])
            for fields_one in fields:
                vertices=[cache[int(cmd.get(k))] for k in fields_one]
                points=np.asarray([v[:3] for v in vertices])
                assert np.any(np.cross(points[1]-points[0],points[2]-points[0])), (slug,'quantization collapsed a triangle')
                actual[(material,face_key(vertices))]+=1
    assert depth==0 and dl[-1].tag=='EndDisplayList' and actual==expected, (slug,'source geometry/UV/normal/winding changed')
    assert loads==report['models'][slug]['vertex_loads']
print('PASS accepted source hashes; both-host closed resource graphs, exact pixels/UVs/normals/winding, fine coordinate fit, casing and gold preservation')

# Rebuild in isolation; compare every shipped byte against the production writer.
with tempfile.TemporaryDirectory(prefix='bottle-polish-rebuild-') as temp:
    subprocess.run([os.environ.get('PYTHON','python3'),str(ROOT/'tools/nei_gi/build_bottle_polish.py'),'--repo',temp],check=True)
    for path in report['resources']:
        assert (Path(temp)/'soh/assets/custom'/path).read_bytes()==(ROOT/'soh/assets/custom'/path).read_bytes()
print('PASS deterministic resource rebuild and explicit content/shimmer mapping')
