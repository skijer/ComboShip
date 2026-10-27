#!/usr/bin/env python3
"""Rebuild the isolated seven-resource prop candidate using only Python stdlib."""
import argparse,base64,hashlib,io,json,stat,zipfile,zlib
from pathlib import Path
ROOT=Path(__file__).resolve().parent
PREFIX='custom/skiejer_market/'
NAMES=tuple(sorted([PREFIX+'texture/baseline_'+n for n in ('counter_atlas','fittings_atlas','ceramic','timber')]+[PREFIX+'geometry/mahouya_room_0DL_006548_vtx_'+str(n) for n in (2,14,24)]))
def sha(b):return hashlib.sha256(b).hexdigest()
def require(ok,message):
 if not ok:raise ValueError(message)
def build(root=ROOT):
 manifest=json.loads((root/'overlay.json').read_text());require(tuple(manifest['resources'])==NAMES,'fixed resource inventory')
 resources={}
 for number,name in enumerate(NAMES,1):
  record=manifest['resources'][name];require(0<record['bytes']<=2*1024*1024 and 1<=len(record['parts'])<=22,'resource bounds')
  pieces=[]
  for index,part in enumerate(record['parts'],1):
   require(part['name']==f'overlay-{number:02}.zlib.{index:03}.b64','part path/order')
   text=(root/part['name']).read_bytes();require(len(text)<=131072 and sha(text)==part['base64_sha256'],'base64 bounds/hash')
   b=base64.b64decode(text,validate=True);require(0<len(b)<=98304 and len(b)==part['bytes'] and sha(b)==part['sha256'],'part bounds/hash');pieces.append(b)
  packed=b''.join(pieces);require(sha(packed)==record['packed_sha256'],'packed hash')
  decoder=zlib.decompressobj();data=decoder.decompress(packed,record['bytes']+1)
  require(len(data)==record['bytes'] and decoder.eof and not decoder.unused_data and not decoder.unconsumed_tail and sha(data)==record['sha256'],'resource bounds/hash')
  resources[name]=data
 out=io.BytesIO()
 with zipfile.ZipFile(out,'w',compression=zipfile.ZIP_STORED) as z:
  for name,data in sorted(resources.items()):
   info=zipfile.ZipInfo(name,(1980,1,1,0,0,0));info.create_system=3;info.external_attr=(stat.S_IFREG|0o644)<<16;z.writestr(info,data)
 data=out.getvalue();require(sha(data)==manifest['archive_sha256'],'archive hash');return data
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path);a=p.parse_args();data=build()
 if a.output:a.output.write_bytes(data)
 print(json.dumps({'status':'PASS','bytes':len(data),'sha256':sha(data)}))
