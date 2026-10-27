"""Small native resource decoder used by the bounded Temple/Sages reconstruction."""
import math
from pathlib import Path
import struct
import xml.etree.ElementTree as ET
import zipfile

EXPANDED = {0x20,0x31,0x32,0x33,0x35,0x36,0x42}


def header(kind, version=0):
    return struct.pack('<B3xIIQ',0,kind,version,0xDEADBEEFDEADBEEF).ljust(64,b'\0')


def crc64(value):
    if not hasattr(crc64,'table'):
        table=[]
        for i in range(256):
            v=i<<56
            for _ in range(8):
                v=((v<<1) ^ (0x42F0E1EBA9EA3693 if v>>63 else 0)) & 0xffffffffffffffff
            table.append(v)
        crc64.table=table
    crc=0xffffffffffffffff
    for byte in value.encode():
        crc=((crc<<8) ^ crc64.table[(crc>>56)^byte]) & 0xffffffffffffffff
    return crc


def xml(tree):
    ET.indent(tree,space='  ')
    return ET.tostring(tree,encoding='utf-8')


def archive(path, prefixes=()):
    if zipfile.is_zipfile(path):
        with zipfile.ZipFile(path) as z:
            names=z.namelist()
            return {n:z.read(n) for n in names if not n.endswith('/') and
                    (not prefixes or any(n.startswith(p) for p in prefixes))}, names
    try:
        import mpyq
    except ImportError as e:
        raise RuntimeError('MPQ donor reading requires mpyq==0.2.5; inputs are never edited') from e
    a=mpyq.MPQArchive(str(path))
    names=[n.decode() if isinstance(n,bytes) else n for n in a.files]
    try:
        return {n:a.read_file(n) for n in names if not prefixes or any(n.startswith(p) for p in prefixes)},names
    finally:
        a.file.close()


def words(data):
    assert data[4:8]==b'TLDO'
    off=72
    while off+8<=len(data):
        a,b=struct.unpack_from('<II',data,off)
        size=16 if a>>24 in EXPANDED else 8
        h=None
        if size==16:
            hi,lo=struct.unpack_from('<II',data,off+8);h=(hi<<32)|lo
        yield off,a,b,h
        off+=size
        if a>>24==0xdf:
            break


def vertex_rows(data):
    if data.lstrip().startswith(b'<'):
        tree=ET.fromstring(data)
        return [(int(v.get('X')),int(v.get('Y')),int(v.get('Z')),0,
                 int(v.get('S')),int(v.get('T')),int(v.get('R')),int(v.get('G')),
                 int(v.get('B')),int(v.get('A'))) for v in tree]
    if data[4:8]==b'RRAO':
        assert struct.unpack_from('<I',data,64)[0]==25
        count=struct.unpack_from('<I',data,68)[0];off=72
    else:
        count=struct.unpack_from('<I',data,64)[0];off=68
    assert off+count*16==len(data),(data[4:8],count,len(data))
    return [struct.unpack_from('<3hH2h4B',data,off+i*16) for i in range(count)]


def vertex_xml(rows):
    tree=ET.Element('Vertex',Version='0')
    for v in rows:
        vals=list(v[:3])+list(v[4:])
        ET.SubElement(tree,'Vtx',dict(zip(('X','Y','Z','S','T','R','G','B','A'),map(lambda x:str(round(x)),vals))))
    return xml(tree)


def transform_vertices(data, transform):
    if data.lstrip().startswith(b'<'):
        tree=ET.fromstring(data)
        for v in tree:
            p=transform([int(v.get(a)) for a in ('X','Y','Z')])
            for a,x in zip(('X','Y','Z'),p):
                assert -32768<=round(x)<=32767
                v.set(a,str(round(x)))
        return xml(tree)
    result=bytearray(data)
    off=72 if data[4:8]==b'RRAO' else 68
    for i,v in enumerate(vertex_rows(data)):
        p=tuple(round(x) for x in transform(v[:3]));assert all(-32768<=x<=32767 for x in p)
        struct.pack_into('<3h',result,off+i*16,*p)
    return bytes(result)


def hash_call(path):
    h=crc64(path)
    return struct.pack('<4I',0x31000000,0,h>>32,h&0xffffffff)


def rewrite(data,mapping):
    if data.lstrip().startswith(b'<'):
        tree=ET.fromstring(data)
        for e in tree.iter():
            if e.get('Path') in mapping:e.set('Path',mapping[e.get('Path')])
        return xml(tree)
    if data[4:8]==b'TLDO':
        result=bytearray(data)
        hashes={crc64(k):crc64(v) for k,v in mapping.items()}
        for off,a,b,h in words(data):
            if h in hashes:
                value=hashes[h];struct.pack_into('<II',result,off+8,value>>32,value&0xffffffff)
        return bytes(result)
    return data


def collect(resources,root,hashes=None):
    """Follow active calls, preserving per-command vertex-cache provenance."""
    hashes=hashes or {crc64(n):n for n in resources}
    cache={}; result=[]; active=set(); texture=None; vertex_cache={}
    def rows_for(name):
        if name not in vertex_cache:vertex_cache[name]=vertex_rows(resources[name])
        return vertex_cache[name]
    def visit(path):
        nonlocal texture
        if path.startswith('>'):return
        if path in active:raise ValueError('recursive display list '+path)
        if path not in resources:raise KeyError('unresolved display list '+path)
        active.add(path);data=resources[path]
        if data.lstrip().startswith(b'<'):
            for index,e in enumerate(ET.fromstring(data)):
                if e.tag=='CallDisplayList':visit(e.get('Path'))
                elif e.tag=='LoadVertices':
                    name=e.get('Path');start=int(e.get('VertexBufferIndex'));off=int(e.get('VertexOffset'))
                    rows=rows_for(name)
                    for j in range(int(e.get('Count'))):cache[start+j]=(name,off+j,rows[off+j])
                elif e.tag in ('SetTextureImage','LoadTextureBlock'):texture=e.get('Path')
                elif e.tag in ('Triangle1','Triangles2'):
                    triangles=[]
                    for suffix in ('0','1') if e.tag=='Triangles2' else ('0',):
                        triangles.append(tuple(cache[int(e.get('V'+suffix+str(j)))][2] for j in range(3)))
                    result.append(dict(path=path,offset=index,faces=triangles,cache=dict(cache),texture=texture))
        else:
            for off,a,b,h in words(data):
                op=a>>24
                if op==0x31:visit(hashes[h])
                elif op==0x32:
                    name=hashes[h];n=(a>>12)&255;start=((a>>1)&127)-n;first=b//16
                    rows=rows_for(name)
                    for j in range(n):cache[start+j]=(name,first+j,rows[first+j])
                elif op==0x20:texture=hashes[h]
                elif op in (0x05,0x06,0x07):
                    ix=[((a>>16)&255)//2,((a>>8)&255)//2,(a&255)//2]
                    sets=[ix]
                    if op==0x06:sets.append([((b>>16)&255)//2,((b>>8)&255)//2,(b&255)//2])
                    if op==0x07:raise ValueError('quad command requires explicit decoding')
                    triangles=[tuple(cache[j][2] for j in indices) for indices in sets]
                    result.append(dict(path=path,offset=off,faces=triangles,cache=dict(cache),texture=texture))
                elif op in (0xda,0x36):raise ValueError('unexpected geometry matrix in '+path)
        active.remove(path)
    visit(root)
    return result


def area(face):
    a,b,c=face
    u=[b[i]-a[i] for i in range(3)];v=[c[i]-a[i] for i in range(3)]
    n=(u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0])
    return math.sqrt(sum(x*x for x in n))


def subtract_box(face,lo,hi):
    if any(max(v[i] for v in face)<lo[i] or min(v[i] for v in face)>hi[i] for i in range(3)):
        return [face]
    inside=[list(face)];outside=[]
    for axis,bound,direction in ((0,lo[0],1),(0,hi[0],-1),(1,lo[1],1),(1,hi[1],-1),(2,lo[2],1),(2,hi[2],-1)):
        keep=[]
        for poly in inside:
            pin=[];pout=[]
            for j,a in enumerate(poly):
                b=poly[(j+1)%len(poly)];da=(a[axis]-bound)*direction;db=(b[axis]-bound)*direction
                (pin if da>=0 else pout).append(a)
                if (da<0 and db>0) or (da>0 and db<0):
                    t=da/(da-db);p=tuple(a[k]+t*(b[k]-a[k]) for k in range(len(a)))
                    pin.append(p);pout.append(p)
            if len(pin)>=3:keep.append(pin)
            if len(pout)>=3:outside.append(pout)
        inside=keep
        if not inside:break
    result=[]
    for p in outside:
        result += [(p[0],p[i],p[i+1]) for i in range(1,len(p)-1) if area((p[0],p[i],p[i+1]))>.1]
    return result


def geometry(faces,path,restore=None):
    rows=[v for face in faces for v in face]
    tree=ET.Element('DisplayList',Version='0')
    for off in range(0,len(rows),30):
        count=min(30,len(rows)-off)
        ET.SubElement(tree,'LoadVertices',Path=path+'_vtx',VertexBufferIndex='0',VertexOffset=str(off),Count=str(count))
        for j in range(0,count,3):ET.SubElement(tree,'Triangle1',V00=str(j),V01=str(j+1),V02=str(j+2),Flag0='0')
    if restore:
        # Callee vertex loads share the native RSP cache with the caller.
        entries=sorted(restore.items());j=0
        while j<len(entries):
            slot,(name,index,_)=entries[j];count=1
            while j+count<len(entries):
                nextslot,(nextname,nextindex,_)=entries[j+count]
                if nextslot!=slot+count or nextname!=name or nextindex!=index+count:break
                count+=1
            ET.SubElement(tree,'LoadVertices',Path=name,VertexBufferIndex=str(slot),VertexOffset=str(index),Count=str(count));j+=count
    ET.SubElement(tree,'EndDisplayList')
    return {path:xml(tree),path+'_vtx':vertex_xml(rows)}


def aperture(resources,roots,namespace,lo=(-98,-6,-790),hi=(98,260,-690)):
    edits={};extra={};seen=set();affected=0
    for root in roots:
        for record in collect(resources,root):
            key=(record['path'],record['offset'])
            if key in seen:continue
            seen.add(key)
            faces=[];changed=False
            for face in record['faces']:
                clipped=subtract_box(face,lo,hi);faces.extend(clipped)
                changed |= clipped != [face]
            if not changed:continue
            path=namespace+'/aperture_'+str(affected);affected+=1
            extra.update(geometry(faces,path,record['cache'] if faces else None))
            edits.setdefault(record['path'],[]).append((record['offset'],path))
    result={}
    for name,items in edits.items():
        data=resources[name]
        if data.lstrip().startswith(b'<'):
            tree=ET.fromstring(data)
            for index,path in items:
                old=tree[index];tree.remove(old);tree.insert(index,ET.Element('CallDisplayList',Path=path))
            result[name]=xml(tree)
        else:
            for off,path in sorted(items,reverse=True):data=data[:off]+hash_call(path)+data[off+8:]
            result[name]=data
    return result|extra,affected


def replace_faces(resources,roots,namespace,select):
    """Keep material/state commands while changing only reachable primitives."""
    edits={};extra={};seen=set();serial=0
    for root in roots:
        for record in collect(resources,root):
            key=(record['path'],record['offset'])
            if key in seen:continue
            seen.add(key);faces=select(record)
            if faces==record['faces']:continue
            path=namespace+'/part_'+str(serial);serial+=1
            extra.update(geometry(faces,path,record['cache'] if faces else None))
            edits.setdefault(record['path'],[]).append((record['offset'],path))
    result=dict(resources)
    for name,items in edits.items():
        data=resources[name]
        if data.lstrip().startswith(b'<'):
            tree=ET.fromstring(data)
            for index,path in items:
                old=tree[index];tree.remove(old);tree.insert(index,ET.Element('CallDisplayList',Path=path))
            result[name]=xml(tree)
        else:
            for off,path in sorted(items,reverse=True):data=data[:off]+hash_call(path)+data[off+8:]
            result[name]=data
    return result|extra


def read_string(data,off):
    size=struct.unpack_from('<I',data,off)[0];off+=4
    return data[off:off+size].decode(),off+size


def room_commands(data):
    """Return exact slices for the command types present in native Temple rooms."""
    count=struct.unpack_from('<I',data,64)[0];off=68;result=[]
    for _ in range(count):
        start=off;cmd=struct.unpack_from('<I',data,off)[0];off+=4
        info={}
        if cmd==0x18:
            n=struct.unpack_from('<I',data,off)[0];off+=4;paths=[]
            for _ in range(n):p,off=read_string(data,off);paths.append(p)
            info['paths']=paths
        elif cmd in (0,1):
            n=struct.unpack_from('<I',data,off)[0];off+=4;info['actors']=data[off:off+n*16];off+=n*16
        elif cmd==0x0b:
            n=struct.unpack_from('<I',data,off)[0];off+=4+n*2
        elif cmd==0x0a:
            _,kind,n=struct.unpack_from('<3B',data,off);off+=3
            if kind not in (0,2):raise ValueError('unsupported Temple mesh kind')
            paths=[]
            for _ in range(n):
                off+=1
                if kind==2:off+=8
                a,off=read_string(data,off);b,off=read_string(data,off);paths.extend([a,b])
            info['meshes']=[p for p in paths if p]
        elif cmd==0x08:off+=5
        elif cmd in (0x10,0x15):off+=3
        elif cmd==0x12:off+=2
        elif cmd==0x16:off+=1
        elif cmd==0x14:pass
        else:raise ValueError('unsupported room command '+hex(cmd))
        result.append((cmd,start,off,info))
    assert off==len(data),(off,len(data))
    return result


def collision_parts(data):
    n=struct.unpack_from('<I',data,76)[0];off=80
    vertices=[struct.unpack_from('<3h',data,off+i*6) for i in range(n)];off+=n*6
    count=struct.unpack_from('<I',data,off)[0];off+=4
    polygons=[struct.unpack_from('<4H4h',data,off+i*16) for i in range(count)];off+=count*16
    count=struct.unpack_from('<I',data,off)[0];off+=4
    surfaces=data[off:off+count*8];off+=count*8
    return vertices,polygons,surfaces,data[off:]


def central_platform_faces(data,transform,center,directions,floor_y,widths=None):
    """Keep the native center relief, opening only bridge mouths in its guard.

    The original Sages platform has six tall inward-facing perimeter walls.
    They allow entry from outside but block every exit onto a pasted bridge.
    Clip those tall walls in each bridge's radial frame, retaining their spans
    between exits and leaving the floor/medallion/small-step triangles intact.
    """
    vertices,polygons,_,_=collision_parts(data)
    directions=tuple(directions)
    widths=tuple(widths) if widths is not None else (120,)*len(directions)
    if len(widths)!=len(directions):raise ValueError('one width per central bridge is required')
    result=[]
    for poly in polygons:
        face=tuple(tuple(transform(vertices[i&0x1fff]))+(0,0,0,255,255,255,255) for i in poly[1:4])
        if max(math.hypot(p[0]-center[0],p[2]-center[2]) for p in face)>=400:continue
        low=min(p[1] for p in face);high=max(p[1] for p in face)
        if high-low<=100 or high<=floor_y+100:
            result.append(face);continue
        faces=[face]
        for (dx,dz),width in zip(directions,widths):
            length=math.hypot(dx,dz)
            if length==0 or width<=0:raise ValueError('bridge direction and width must be positive')
            dx/=length;dz/=length
            half=width/2+4 # overlap the open mouth with its side guards
            clipped=[]
            for triangle in faces:
                local=tuple((dx*(p[0]-center[0])+dz*(p[2]-center[2]),p[1],
                             -dz*(p[0]-center[0])+dx*(p[2]-center[2]),*p[3:]) for p in triangle)
                for piece in subtract_box(local,(210,low-1,-half),(360,high+1,half)):
                    clipped.append(tuple((center[0]+dx*p[0]-dz*p[2],p[1],
                                          center[2]+dz*p[0]+dx*p[2],*p[3:]) for p in piece))
            faces=clipped
        result.extend(faces)
    return result


def bridge_guard_faces(a,b,width,height=600):
    """Return collision-only, inward-facing long sides with both ends open."""
    dx=b[0]-a[0];dz=b[2]-a[2];length=math.hypot(dx,dz)
    if length==0 or width<=0 or height<=0:raise ValueError('bridge guard dimensions must be positive')
    nx=-dz/length;nz=dx/length
    faces=[]
    for side in (-1,1):
        points=[]
        for p,offset in ((a,-64),(b,-64),(b,height),(a,height)):
            points.append((p[0]+side*nx*width/2,p[1]+offset,p[2]+side*nz*width/2,
                           0,0,0,255,255,255,255))
        triangles=[(points[0],points[1],points[2]),(points[0],points[2],points[3])]
        if side==1:triangles=[(f[0],f[2],f[1]) for f in triangles]
        faces.extend(triangles)
    return faces


def append_collision(original,add_faces,door_lo=(-98,-6,-790),door_hi=(98,260,-690),camera_ignore_faces=()):
    vertices,old_polys,surfaces,tail=collision_parts(original)
    lookup={p:i for i,p in enumerate(vertices)};polys=[];pending=[]
    for p in old_polys:
        face=[tuple(vertices[i&0x1fff])+ (0,0,0,255,255,255,255) for i in p[1:4]]
        clipped=subtract_box(tuple(face),door_lo,door_hi)
        if clipped==[tuple(face)]:polys.append(p)
        else:pending.extend((f,p[0],p[1]&0xe000,p[2]&0xe000) for f in clipped)
    # A zero camera index is not neutral in the Temple: entry 0 is its opening
    # camera. Reuse a verified CAM_SET_NONE entry without changing the table.
    camera_count=struct.unpack_from('<I',tail,0)[0]
    neutral=next((i for i in range(camera_count) if struct.unpack_from('<H',tail,4+i*8)[0]==0),None)
    if neutral is None or neutral>255:raise ValueError('no neutral inherited camera for annex surfaces')
    surface=len(surfaces)//8;surfaces+=struct.pack('<II',0,neutral)
    pending.extend((f,surface,0,0) for f in add_faces)
    # flags_vIA bit 13 ignores cameras; entity exclusion is the separate bit 14.
    pending.extend((f,surface,0x2000,0) for f in camera_ignore_faces)
    for face,kind,flags_a,flags_b in pending:
        points=[tuple(round(x) for x in p[:3]) for p in face]
        a,b,c=points;u=[b[i]-a[i] for i in range(3)];v=[c[i]-a[i] for i in range(3)]
        normal=(u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0])
        length=math.sqrt(sum(x*x for x in normal))
        if length<.1:continue
        unit=tuple(x/length for x in normal);ids=[]
        for p in points:
            if p not in lookup:lookup[p]=len(vertices);vertices.append(p)
            ids.append(lookup[p])
        assert len(vertices)<8192,'native collision index capacity'
        normals=[max(-32767,min(32767,round(x*32767))) for x in unit]
        distance=round(-sum(unit[i]*a[i] for i in range(3)))
        assert -32768<=distance<=32767
        polys.append((kind,ids[0]|flags_a,ids[1]|flags_b,ids[2],*normals,distance))
    bounds=[min(p[i] for p in vertices) for i in range(3)]+[max(p[i] for p in vertices) for i in range(3)]
    result=original[:64]+struct.pack('<6hI',*bounds,len(vertices))
    result+=b''.join(struct.pack('<3h',*p) for p in vertices)
    result+=struct.pack('<I',len(polys))+b''.join(struct.pack('<4H4h',*p) for p in polys)
    result+=struct.pack('<I',len(surfaces)//8)+surfaces+tail
    return result
