from pathlib import Path
import json, math, struct, zipfile, hashlib
import xml.etree.ElementTree as ET
import numpy as np

MASK=(1<<64)-1
POLY=0x42F0E1EBA9EA3693
TABLE=[]
for b in range(256):
    crc=b<<56
    for _ in range(8): crc=((crc<<1)^POLY)&MASK if crc&(1<<63) else (crc<<1)&MASK
    TABLE.append(crc)

def crc64(path):
    crc=MASK
    for b in path.encode(): crc=((crc<<8)&MASK)^TABLE[((crc>>56)^b)&255]
    return crc

class Archive:
    def __init__(self,path):
        self.path=path
        self.z=zipfile.ZipFile(path)
        self.index={crc64(n):n for n in self.z.namelist()}
        assert len(self.index)==len(self.z.namelist())

    def commands(self,path):
        data=self.z.read(path)
        if data.lstrip().startswith(b'<'):
            root=ET.fromstring(data)
            assert root.tag=='DisplayList',(path,root.tag)
            for node in root:
                tag=node.tag;a=node.attrib
                if tag=='DisplayList': yield 0x31,0x31000000,0,a['Path'].removeprefix('__OTR__')
                elif tag=='LoadVertices':
                    n=int(a['Count']);v0=int(a.get('VertexBufferIndex','0'));offset=int(a.get('VertexOffset','0'))
                    yield 0x32,(0x32<<24)|(n<<12)|((n+v0)*2),offset*16,a['Path'].removeprefix('__OTR__')
                elif tag=='Triangle1':
                    yield 0x05,(0x05<<24)|(int(a['V0'])*2<<16)|(int(a['V1'])*2<<8)|int(a['V2'])*2,0,None
                elif tag=='Triangles2':
                    w0=(0x06<<24)|(int(a['V00'])*2<<16)|(int(a['V01'])*2<<8)|int(a['V02'])*2
                    w1=(int(a['V10'])*2<<16)|(int(a['V11'])*2<<8)|int(a['V12'])*2
                    yield 0x06,w0,w1,None
                elif tag in ('Matrix','PopMatrix'): yield 0xDA,0,0,a.get('Path')
                elif tag=='EndDisplayList': yield 0xDF,0,0,None
                else:
                    assert not any(s in tag.lower() for s in ('matrix','vertex','vertices','triangle','branch')),(path,'Unsupported XML geometry command',tag)
            return
        assert struct.unpack_from('<I',data,4)[0]==0x4F444C54,(path,'Not display list')
        assert data[64]==4,(path,'Unsupported microcode')
        i=72
        while i<len(data):
            w0,w1=struct.unpack_from('<II',data,i)
            op=w0>>24
            ref=None
            if op in (0x20,0x31,0x32,0x33,0x36,0x42):
                hi,lo=struct.unpack_from('<II',data,i+8)
                ref=self.index.get((hi<<32)|lo)
                i+=16
            else: i+=8
            yield op,w0,w1,ref
            if op==0xDF: break

    def mesh(self,root):
        vertices=[];triangles=[];cache={};visited=set();missing=set();matrices=[];vertex_resources=set()
        modelview=np.eye(4);stack=[]
        def execute(path,depth=0):
            nonlocal modelview
            assert depth<32,(root,'Recursive DL')
            visited.add(path)
            for op,w0,w1,ref in self.commands(path):
                if op==0x31:
                    if ref: execute(ref,depth+1)
                    else: missing.add((path,hex(w0),hex(w1)))
                    if (w0>>16)&1: return
                elif op==0x32:
                    if ref is None: raise RuntimeError((path,'Unresolved vertex resource'))
                    raw=self.z.read(ref)
                    if raw.lstrip().startswith(b'<'):
                        tree=ET.fromstring(raw);assert tree.tag=='Vertex',(ref,tree.tag)
                        coords=[tuple(int(v.attrib[k]) for k in ('X','Y','Z')) for v in tree]
                        count=len(coords)
                    else:
                        assert struct.unpack_from('<I',raw,4)[0]==0x4F415252,(ref,'Not vertex array')
                        kind,count=struct.unpack_from('<II',raw,64)
                        assert kind==25 and len(raw)==72+count*16,(ref,kind,count,len(raw))
                        coords=[struct.unpack_from('<hhh',raw,72+j*16) for j in range(count)]
                    n=(w0>>12)&255;end=(w0>>1)&127;v0=end-n
                    assert w1%16==0,(ref,w1)
                    start=w1//16
                    assert 0<=v0<=32 and 0< n<=32 and end<=32 and start+n<=count,(ref,n,v0,start,count)
                    vertex_resources.add(ref)
                    for j in range(n):
                        xyz=(np.append(coords[start+j],1)@modelview)[:3]
                        cache[v0+j]=len(vertices);vertices.append(xyz)
                elif op in (0x05,0x06):
                    words=[w0] if op==0x05 else [w0,w1]
                    for word in words:
                        indices=[((word>>s)&255)//2 for s in (16,8,0)]
                        assert all(k in cache for k in indices),(path,'Uninitialized vertex',indices)
                        triangles.append([cache[k] for k in indices])
                elif op==0x36:
                    matrices.append((path,hex(op),ref))
                    assert ref is not None,(path,'Unresolved matrix')
                    raw=self.z.read(ref)
                    assert struct.unpack_from('<I',raw,4)[0]==0x4F4D5458 and len(raw)==128,(ref,'Not binary matrix')
                    words=struct.unpack_from('<16I',raw,64)
                    fixed=[]
                    for integer,fraction in zip(words[:8],words[8:]):
                        fixed.extend(((integer&0xffff0000)|(fraction>>16),(integer<<16&0xffff0000)|(fraction&0xffff)))
                    decoded=np.array([v if v<2**31 else v-2**32 for v in fixed],dtype=float).reshape(4,4)/65536
                    flags=(w0&255)^1
                    assert not flags&~1,(path,'Unsupported projection/load matrix')
                    if flags&1:stack.append(modelview.copy())
                    modelview=decoded@modelview
                elif op==0xD8:
                    matrices.append((path,hex(op),ref))
                    assert w1%64==0 and 0<w1//64<=len(stack),(path,'Unbalanced matrix pop')
                    modelview=stack[-w1//64];del stack[-w1//64:]
                elif op in (0xDA,0x29): raise AssertionError((root,'Unsupported XML matrix',path,ref))
        execute(root)
        assert not missing,(root,missing)
        assert not stack and np.array_equal(modelview,np.eye(4)),(root,'Unrestored modelview')
        points=np.asarray(vertices,dtype=float);faces=np.asarray(triangles,dtype=int)
        unique=np.unique(points,axis=0)
        _,_,axes=np.linalg.svd(unique-unique.mean(0),full_matrices=False)
        axis=axes[0]
        if axis[0]<0: axis=-axis
        angle=1.8;z=np.array([[math.cos(angle),-math.sin(angle),0],[math.sin(angle),math.cos(angle),0],[0,0,1.]])
        x=np.array([[1.,0,0],[0,0,1.],[0,-1.,0]])
        old=x@z@axis;new=z@axis
        result={'root':root,'display_lists':len(visited),'vertex_resources':len(vertex_resources),'triangles':len(faces),'bounds':[points.min(0).tolist(),points.max(0).tolist()],
                'axis':axis.tolist(),'old_vertical_fraction':float(abs(old[1])),'new_vertical_fraction':float(abs(new[1])),
                'old_elevation_degrees':math.degrees(math.asin(min(1,abs(old[1])))),'new_elevation_degrees':math.degrees(math.asin(min(1,abs(new[1])))),
                'embedded_matrix_commands':len(matrices)}
        return result,points,faces

def roots_for(archive):
    ns=set(archive.z.namelist())
    roots=['alt/objects/object_custom_equip/gCustomKokiriSwordDL','alt/objects/object_custom_equip/gCustomMasterSwordDL','alt/objects/object_custom_equip/gCustomLongswordDL',
           'objects/din_fire_sword/progressive/child/SwordDL','objects/din_fire_sword/progressive/adult/SwordDL','objects/din_fire_sword/progressive/bgs/SwordDL',
           'objects/object_link_child/DinSleekEquipmentPOC1_MM_Child/SwordDL','objects/object_link_child/DinSleekEquipmentPOC1_MM_Child/RazorSwordDL',
           'objects/object_link_child/DinSleekEquipmentPOC1_MM_Child/MasterSwordDL','objects/object_link_child/DinSleekEquipmentPOC1_MM_Child/TwoHandedSwordDL']
    return [n for n in roots if n in ns]
