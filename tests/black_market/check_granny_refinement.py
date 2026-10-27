#!/usr/bin/env python3
"""Audit actual immutable Djipi/native Granny bytes through the private RAM recipe."""
from pathlib import Path
import argparse, hashlib, os, struct, subprocess, sys, tempfile, zipfile, zlib
import xml.etree.ElementTree as ET
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/black_market'))
from build import archive
import market_interior

REAR={0:[([[49,37,24],[97,37,72]],25,36)],
      2:[([[-120,0,0],[-62,54,54]],116,106),([[-103,60,42],[-92,62,53]],20,16),([[42,0,20],[105,39,79]],94,88)],
      6:[([[-104,51,27],[-53,62,53]],80,60)],18:[([[-114,51,2],[-69,51,47]],30,36)]}

def main():
 p=argparse.ArgumentParser(description=__doc__)
 for key in ('maps','textures','stormlib','native-oot'):p.add_argument('--'+key,type=Path,required=True)
 p.add_argument('--case',choices=('all','opa','vapor'),default='all');a=p.parse_args()
 inputs={str(v):hashlib.sha256(v.read_bytes()).hexdigest() for v in (a.maps,a.textures,a.native_oot)}
 resources=archive(a.maps,a.stormlib);resources.update(archive(a.textures,a.stormlib))
 aliases={**resources,**{n[4:]:v for n,v in resources.items() if n.startswith('alt/')}}
 prefix='alt/scenes/shared/mahouya_scene/'
 masks={}
 for suffix,pieces in REAR.items():
  path=prefix+f'mahouya_room_0DL_006548_tri_{suffix}'
  parts=market_interior.vertex_components(aliases,path);removed=set()
  for bounds,count,triangles in pieces:
   matches=[c for c in parts if c['bounds']==bounds]
   assert len(matches)==1 and len(matches[0]['indices'])==count and matches[0]['triangles']==triangles
   removed.update(matches[0]['indices'])
  masks[path]=removed
 def string(s):
  b=s.encode();return struct.pack('<I',len(b))+b
 records=[]
 for name,data in sorted(resources.items()):
  if not name.startswith(prefix):continue
  if not data.lstrip().startswith(b'<'):
   records.append(string(name)+struct.pack('<II',2,0));continue
  tree=ET.fromstring(data)
  if tree.tag=='Vertex':
   rows=b''.join(struct.pack('<3hH2h4B',*[int(v.get(k)) for k in ('X','Y','Z')],0,*[int(v.get(k)) for k in ('S','T','R','G','B','A')]) for v in tree)
   records.append(string(name)+struct.pack('<II',1,len(tree))+rows);continue
  assert tree.tag=='DisplayList';commands=[];cache={}
  def command(w0,w1=0,path='',expected=None):
   expected=expected or (w0,w1)
   commands.append(struct.pack('<IQ',w0,w1)+string(path)+struct.pack('<IQ',*expected))
  for i,node in enumerate(tree):
   if node.tag=='CallDisplayList':command(0x27000000,path=node.get('Path'))
   elif node.tag=='LoadVertices':
    count,slot,offset=[int(node.get(k)) for k in ('Count','VertexBufferIndex','VertexOffset')]
    command(0x24000000,path=node.get('Path'));command(count,(slot<<16)|offset)
    cache.update({slot+n:offset+n for n in range(count)})
   elif node.tag=='SetTextureImage':
    command(0x25000000|zlib.crc32(ET.tostring(node))&0xffffff,path=node.get('Path'));command(0xe7000000)
   elif node.tag in ('Triangle1','Triangles2'):
    faces=[]
    for face in range(2 if node.tag=='Triangles2' else 1):
     vertices=[int(node.get(f'V{face}{v}')) for v in range(3)]
     # Factory flags rotate winding; these audited donor flags are zero.
     assert int(node.get(f'Flag{face}','0'))==0
     faces.append(vertices)
    words=[sum((v*2)<<shift for v,shift in zip(face,(16,8,0))) for face in faces]
    if len(faces)==1:
     # Match ResourceFactoryXMLDisplayListV0's extended single-triangle codec.
     w0=0x26000000|faces[0][0];w1=faces[0][1]<<16|faces[0][2]
    else:
     w0=6<<24|words[0];w1=words[1]
    keep=[]
    for face in faces:
     selected=[cache[v] in masks.get(name,set()) for v in face]
     assert not any(selected) or all(selected)
     if not any(selected):keep.append(face)
    expected=(w0,w1)
    if not keep:expected=(0xe0000000,0)
    elif len(keep)!=len(faces):expected=(5<<24|sum((v*2)<<shift for v,shift in zip(keep[0],(16,8,0))),0)
    command(w0,w1,expected=expected)
   elif node.tag=='EndDisplayList':command(0xdf000000)
   else:command(0xe7000000|zlib.crc32(ET.tostring(node))&0xffffff,i)
  records.append(string(name)+struct.pack('<II',0,len(commands))+b''.join(commands))
 with tempfile.TemporaryDirectory(prefix='granny-refinement-') as directory:
  work=Path(directory);graph=work/'graph.bin';graph.write_bytes(struct.pack('<I',len(records))+b''.join(records))
  with zipfile.ZipFile(a.native_oot) as pack:
   native=work/'native.bin';root=pack.read(prefix[4:]+'mahouya_room_0DL_006A78')[72:]
   leaf=pack.read(prefix[4:]+'mahouya_room_0DL_006940')[72:]
   vertices=pack.read(prefix[4:]+'mahouya_room_0Vtx_0065C0')[72:]
   assert len(root)==5*8 and len(leaf)==45*8 and len(vertices)==56*16
   native.write_bytes(struct.pack('<III',5,45,56)+root+leaf+vertices)
  binary=work/'test'
  subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-DF3DEX_GBI_2','-g','-fsanitize=address,undefined',
   '-fno-sanitize-recover=all','-no-pie','-I'+str(ROOT),'-I'+str(ROOT/'libultraship/include'),
   str(ROOT/'tests/black_market/granny_refinement_test.cpp'),'-o',str(binary)],check=True)
  subprocess.run([str(binary),str(graph),str(native),a.case],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
 for path,digest in inputs.items():assert hashlib.sha256(Path(path).read_bytes()).hexdigest()==digest
 print('PASS immutable real donor archives:',inputs)
if __name__=='__main__':main()
