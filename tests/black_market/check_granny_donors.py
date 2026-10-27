#!/usr/bin/env python3
"""Transport the real Djipi reference graph through the production RAM adapter.

Only resource-reference commands need native encoding in this fixture. Other
commands carry distinct opaque tokens to prove they pass through byte-for-byte;
native controller compilation separately verifies the actual rendering APIs.
External geometry is temporary and never packaged into a replacement archive.
"""
from pathlib import Path
import argparse, hashlib, json, os, struct, subprocess, sys, tempfile, zlib
import xml.etree.ElementTree as E
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/black_market'))
from build import archive

def check(maps,textures,stormlib,output):
    inputs={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in (maps,textures)}
    resources=archive(maps,stormlib);resources.update(archive(textures,stormlib))
    prefix='alt/scenes/shared/mahouya_scene/'
    records=[]
    def string(s):
        value=s.encode();return struct.pack('<I',len(value))+value
    for name,data in sorted(resources.items()):
        if not name.startswith(prefix):continue
        if not data.lstrip().startswith(b'<'):
            records.append(string(name)+struct.pack('<II',2,0));continue
        tree=E.fromstring(data)
        if tree.tag=='Vertex':
            vertices=b''.join(struct.pack('<3hH2h4B',*[int(v.get(k)) for k in ('X','Y','Z')],0,
                *[int(v.get(k)) for k in ('S','T','R','G','B','A')]) for v in tree)
            records.append(string(name)+struct.pack('<II',1,len(tree))+vertices);continue
        assert tree.tag=='DisplayList'
        commands=[]
        def command(w0,w1=0,path=''):
            commands.append(struct.pack('<IQ',w0,w1)+string(path))
        for i,e in enumerate(tree):
            if e.tag=='CallDisplayList':command(0x27000000,path=e.get('Path'))
            elif e.tag=='LoadVertices':
                command(0x24000000,path=e.get('Path'))
                command(int(e.get('Count')),(int(e.get('VertexBufferIndex'))<<16)|int(e.get('VertexOffset')))
            elif e.tag=='SetTextureImage':
                command(0x25000000|zlib.crc32(E.tostring(e))&0xffffff,path=e.get('Path'))
                command(0xe7000000) # Native XML factory's trailing PipeSync.
            elif e.tag=='EndDisplayList':command(0xdf000000)
            else:command(0xe7000000|zlib.crc32(E.tostring(e))&0xffffff,i)
        records.append(string(name)+struct.pack('<II',0,len(commands))+b''.join(commands))
    with tempfile.TemporaryDirectory(prefix='granny-donor-check-') as temp:
        temp=Path(temp);fixture=temp/'graph.bin';fixture.write_bytes(struct.pack('<I',len(records))+b''.join(records))
        binary=temp/'test';counter=temp/'counter.bin'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-DF3DEX_GBI_2','-g',
            '-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',
            '-I'+str(ROOT),'-I'+str(ROOT/'libultraship/include'),
            str(ROOT/'tests/black_market/granny_donor_test.cpp'),'-o',str(binary)],check=True)
        result=json.loads(subprocess.check_output([str(binary),str(fixture),str(counter)],text=True,
            env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'}))
        raised=counter.read_bytes()
    native_rows=list(struct.iter_unpack('<3hH2h4B',raised))
    original=list(E.fromstring(resources[prefix+'mahouya_room_0DL_006548_vtx_2']))
    changed=[]
    for i,(v,row) in enumerate(zip(original,native_rows)):
        expected=[int(v.get(k)) for k in ('X','Y','Z')]+[0]+[int(v.get(k)) for k in ('S','T','R','G','B','A')]
        if list(row)!=expected:
            expected[1]+=11;assert list(row)==expected;changed.append(i)
    assert len(changed)==46 and len(native_rows)==1197
    reachable=set(); used_textures=set()
    def visit(path):
        if path in reachable:return
        reachable.add(path)
        for node in E.fromstring(resources[path]):
            if node.tag=='CallDisplayList':visit('alt/'+node.get('Path'))
            elif node.tag=='SetTextureImage':used_textures.add('alt/'+node.get('Path'))
    visit(prefix+'mahouya_room_0DL_006548')
    assert result['lists']==len(reachable) and result['textures']==len(used_textures),result
    for p,digest in inputs.items():assert hashlib.sha256(Path(p).read_bytes()).hexdigest()==digest
    result.update(inputs=inputs,changed_vertices=changed,added_counter_vertices=8,
        source_sha256={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in (
            'combo/BlackMarketGranny.h','combo/BlackMarketGrannyResources.h',
            'tests/black_market/granny_donor_test.cpp','tests/black_market/check_granny_donors.py')},
        native_runtime_proven=False,fixture='Actual external graph and vertices; non-reference Gfx commands are opaque preservation tokens')
    output.parent.mkdir(parents=True,exist_ok=True);output.write_text(json.dumps(result,indent=2)+'\n')
    return resources,native_rows,result

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for key in ('maps','textures','stormlib','output'):p.add_argument('--'+key,type=Path,required=True)
    a=p.parse_args();_,_,result=check(a.maps,a.textures,a.stormlib,a.output);print(json.dumps(result,indent=2))
