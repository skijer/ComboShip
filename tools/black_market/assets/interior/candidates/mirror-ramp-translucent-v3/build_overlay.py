#!/usr/bin/env python3
"""Rebuild the isolated translucent Mirror-ramp candidate using standard Python."""
import argparse,base64,hashlib,json,stat,struct,xml.etree.ElementTree as ET,zipfile,zlib
from pathlib import Path
ROOT=Path(__file__).resolve().parent
PREFIX='custom/comboship/native_forest/'
MASK=PREFIX+'authored/prelude/mirror_ramp_flow_v2'
MATERIAL=PREFIX+'mirror_access_material'
RAMP=PREFIX+'mirror_access_root'
ROOMS=('scenes/nonmq/comboship_forest/comboship_forest_room_0','scenes/nonmq/skiejer_forest/skiejer_forest_room_0','scenes/shared/skiejer_forest_oot/skiejer_forest_oot_room_0')
def sha(b):return hashlib.sha256(b).hexdigest()
def validate(data,m):
 assert set(data)=={MASK,MATERIAL,RAMP,*ROOMS}
 mask=data[MASK];assert len(mask)==2128 and struct.unpack_from('<4I',mask,64)==(6,32,64,2048)
 assert sha(mask)==m['private_mask_sha256'] and sha(mask[:80])==m['native_mask_header_sha256']
 material=ET.fromstring(data[MATERIAL]);low=[n for n in material if n.tag=='SetOtherMode' and n.get('Cmd')=='G_SETOTHERMODE_L'][0]
 assert low.get('G_RM_AA_ZB_XLU_SURF')==low.get('G_RM_AA_ZB_XLU_SURF2')=='1' and not any('OPA' in k for k in low.attrib)
 assert material.find('SetPrimColor').get('A')=='112' and material.find('SetGeometryMode').get('G_CULL_BACK')=='1'
 assert material.find('Texture').get('Tile')=='1' and material.find('CallDisplayList').get('Path')=='>0x08000000'
 assert material.find('LoadTextureBlock').get('Path')==MASK
 root=ET.fromstring(data[RAMP]);prefix=list(root)[:m['root_geometry_prefix_nodes']]
 assert sha(b''.join(ET.tostring(n) for n in prefix))==m['root_geometry_prefix_sha256']
 restore=[n for n in root if n.tag=='SetOtherMode' and n.get('Cmd')=='G_SETOTHERMODE_L']
 assert len(restore)==1 and restore[0].get('G_RM_AA_ZB_OPA_SURF2')=='1' and restore[0].get('G_RM_FOG_SHADE_A')=='1'
 combine=root.find('SetCombineLERP');assert combine.get('Ad0')=='G_ACMUX_TEXEL0' and combine.get('Aa1')=='G_ACMUX_COMBINED' and combine.get('Ac1')=='G_ACMUX_PRIMITIVE'
 assert [n for n in root if n.tag=='SetOtherMode' and n.get('Cmd')=='G_SETOTHERMODE_H'][0].get('G_CYC_2CYCLE')=='1'
 name=RAMP.encode();pointer=struct.pack('<I',len(name))+name
 for path in ROOMS:
  room=data[path];assert room[4:8]==b'MORO' and room.count(pointer)==1
  assert room.count(bytes(4)+pointer)==1 and pointer+bytes(4) not in room
 return data

def resources(root=ROOT):
 m=json.loads((root/'overlay.json').read_text());assert len(m['parts'])==1 and m['parts'][0]['name']=='ramp-xlu.zlib.001.b64'
 part=m['parts'][0];text=(root/part['name']).read_bytes();assert len(text)<=131072 and sha(text)==part['base64_sha256']
 packed=base64.b64decode(text,validate=True);assert len(packed)<=98304 and len(packed)==part['bytes'] and sha(packed)==part['sha256']
 assert 0<m['decoded_bytes']<=32768
 d=zlib.decompressobj();raw=d.decompress(packed,m['decoded_bytes']+1);assert len(raw)==m['decoded_bytes'] and d.eof and not d.unused_data and not d.unconsumed_tail and sha(raw)==m['decoded_sha256']
 data={};at=0;assert len(m['resources'])==6
 for row in m['resources']:
  path=row['path'];assert path not in data and 0<row['bytes']<=8192
  blob=raw[at:at+row['bytes']];at+=row['bytes'];assert len(blob)==row['bytes'] and sha(blob)==row['sha256'];data[path]=blob
 assert at==len(raw)
 return validate(data,m),m

def build(output,root=ROOT):
 data,m=resources(root)
 with zipfile.ZipFile(output,'w',compression=zipfile.ZIP_STORED) as z:
  for name in sorted(data):
   info=zipfile.ZipInfo(name,(1980,1,1,0,0,0));info.create_system=3;info.external_attr=(stat.S_IFREG|0o644)<<16;z.writestr(info,data[name])
 digest=sha(Path(output).read_bytes())
 if m.get('archive_sha256'):assert digest==m['archive_sha256']
 with zipfile.ZipFile(output) as z:assert z.namelist()==sorted(data) and z.testzip() is None
 return digest
if __name__=='__main__':
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,default=Path('Black-Market-Mirror-Ramp-Translucent-v3-Candidate.o2r'));args=parser.parse_args();print('PASS',build(args.output))
