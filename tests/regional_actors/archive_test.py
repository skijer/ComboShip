#!/usr/bin/env python3
"""Verify compiled catalogue against original archive metadata and complete hash graphs."""
from pathlib import Path
import ctypes as C, struct, sys, tempfile, subprocess, zipfile, hashlib, json
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from render_anju_preview import crc64
class Presentation(C.Structure):
    _fields_=[(n,C.c_char_p) for n in ['skeleton','animation','secondarySkeleton','secondaryAnimation','mask','head','eyes']]+[('limbs',C.c_uint8),('tracking',C.c_int),('secondary',C.c_bool),('kind',C.c_int),('matrices',C.c_uint8),('frames',C.c_uint16),('eyeCount',C.c_uint8),('mouthCount',C.c_uint8),('eyeSegment',C.c_uint8),('mouthSegment',C.c_uint8),('prop',C.c_char_p),('propLimb',C.c_uint8)]
def string(data,offset):
    length,=struct.unpack_from('<I',data,offset);offset+=4
    return data[offset:offset+length].decode(),offset+length
with tempfile.TemporaryDirectory() as d:
    folder=ROOT/'soh/src/overlays/actors/ovl_En_Viewer'
    subprocess.run(['cc','-shared','-fPIC','-I'+str(ROOT/'soh/include'),str(folder/'static_story_actor.c'),str(folder/'static_story_mm_actor.c'),'-lm','-o',d+'/catalog.so'],check=True)
    lib=C.CDLL(d+'/catalog.so');lib.StaticStoryMm_GetPresentation.restype=C.POINTER(Presentation)
    lib.StaticStoryMm_GetEyeTexturePath.restype=lib.StaticStoryMm_GetMouthTexturePath.restype=C.c_char_p
    for actor in json.loads((ROOT/'combo/actors/RegionalActorPresets.json').read_text())['actors']:
        assert actor['oot_actor_id']=='0x002A'
        for pose in actor['poses']:
            assert lib.StaticStoryActor_GetType(int(pose['params'],16))==actor['catalog_id']
            assert lib.StaticStoryMm_GetPresentation(actor['catalog_id'],pose['pose_id'])
    with zipfile.ZipFile(sys.argv[1]) as z:
        hashes={crc64(n):n for n in z.namelist()}
        allpaths=set()
        def read(path,kind):
            data=z.read(path);assert len(data)>=64 and data[0]==0 and data[4:8][::-1]==kind,(path,kind)
            allpaths.add(path);return data
        def graph(path,seen):
            if path in seen:return
            seen.add(path);data=read(path,b'ODLT');assert (len(data)-72)%8==0
            commands=list(struct.iter_unpack('<II',data[72:]));i=0;ended=False
            while i<len(commands):
                w0,w1=commands[i];op=w0>>24
                if op in (0x20,0x31,0x32):
                    i+=1;a,b=commands[i];target=hashes[(a<<32)|b]
                    if op==0x31:graph(target,seen)
                    elif op==0x20:read(target,b'OTEX')
                    else:
                        vertex=z.read(target);assert vertex[4:8][::-1] in (b'OARR',b'OVTX'),target;allpaths.add(target)
                elif op==0x33:i+=1
                elif op==0xDF:ended=True;break
                i+=1
            assert ended,path
        for actor,count in [(25,5),(26,2),(27,2)]:
            for pose in range(count):
                p=lib.StaticStoryMm_GetPresentation(actor,pose).contents
                data=read(p.skeleton.decode(),b'OSKL');kind,limbType,limbs,matrices,tableType,total=struct.unpack_from('<BBIIBI',data,64)
                assert (limbType,tableType,limbs,matrices,total)==(1,1,p.limbs,p.matrices,p.limbs)
                assert kind==(0 if actor==25 else 1)
                offset=79;paths=[]
                for _ in range(total):path,offset=string(data,offset);paths.append(path)
                matricesFound=0;edges=[];graphpaths=set()
                for path in paths:
                    data=read(path,b'OSLB');assert data[64]==1
                    _,o=string(data,66);v,mods=struct.unpack_from('<HI',data,o);o+=6;assert mods==0
                    _,o=string(data,o);o+=18
                    _,o=string(data,o);_,o=string(data,o);dl,o=string(data,o);_,o=string(data,o)
                    child,sibling=struct.unpack_from('BB',data,o+6);edges.append((child,sibling))
                    if dl:matricesFound+=1;graph(dl,graphpaths)
                assert matricesFound=={25:17,26:19,27:17}[actor],(actor,matricesFound,p.matrices)
                visited=set()
                def walk(index):
                    if index==255:return
                    assert index<len(edges) and index not in visited
                    visited.add(index)
                    for child in edges[index]:walk(child)
                walk(0);assert len(visited)==p.limbs
                clip=read(p.animation.decode(),b'OANM');anim,frames,words=struct.unpack_from('<IhI',clip,64)
                assert anim==0 and frames==p.frames
                offset=74+words*2; joints,=struct.unpack_from('<I',clip,offset)
                assert joints==p.limbs+1
                static,=struct.unpack_from('<H',clip,offset+4+joints*6)
                for triple in struct.iter_unpack('<HHH',clip[offset+4:offset+4+joints*6]):
                    assert all(i+(1 if i<static else frames)<=words for i in triple)
                for i in range(p.eyeCount):read(lib.StaticStoryMm_GetEyeTexturePath(actor,i).decode(),b'OTEX')
                for i in range(p.mouthCount):read(lib.StaticStoryMm_GetMouthTexturePath(actor,i).decode(),b'OTEX')
                if p.prop:
                    assert actor==26 and pose==1 and p.propLimb==17
                    graph(p.prop.decode(),graphpaths)
                print(f'PASS actor{actor} pose{pose}: limbs{limbs}, matrices{matrices}, frames{frames}, lists{len(graphpaths)}')
        print(f'PASS closed archive graph: {len(allpaths)} resources; SHA256 {hashlib.file_digest(open(sys.argv[1],"rb"),"sha256").hexdigest()}')
