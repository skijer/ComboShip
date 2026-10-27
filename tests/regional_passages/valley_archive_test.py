#!/usr/bin/env python3
"""Active room-graph/continuous body evidence, portable reduced fixtures by default.

Optional archive inputs are read-only. No archive bytes are copied into fixtures:
only derived local triangles, graph identities and protected-data digests.
"""
import argparse, hashlib, json, math, re, struct, sys, zipfile
from pathlib import Path
import xml.etree.ElementTree as ET
from fractions import Fraction
from ice_platform_clearance_test import body_prism_clearance
ROOT=Path(__file__).resolve().parents[2]
PREFIX='scenes/shared/spot08_scene/'
MASK=(1<<64)-1
TABLE=[]
for i in range(256):
    c=i<<56
    for _ in range(8):c=((c<<1)^0x42f0e1eba9ea3693 if c&(1<<63) else c<<1)&MASK
    TABLE.append(c)
def crc64(s):
    c=MASK
    for b in s.encode():c=(TABLE[((c>>56)^b)&255]^(c<<8))&MASK
    return c

def collision(b):
    nv=struct.unpack_from('<I',b,76)[0]
    vertices=[struct.unpack_from('<hhh',b,80+6*i)for i in range(nv)]
    off=80+6*nv;count=struct.unpack_from('<I',b,off)[0]
    polys=[struct.unpack_from('<HHHHhhhh',b,off+4+16*i)for i in range(count)]
    return vertices,polys,hashlib.sha256(b[off+4+16*count:]).hexdigest()

def vertices(b):
    if b.startswith(b'<'):
        return [[float(v.get(k,'0'))for k in ('X','Y','Z')]for v in ET.fromstring(b)]
    kind,n=struct.unpack_from('<II',b,64);assert kind==25
    version=struct.unpack_from('<I',b,8)[0];stride=24 if version==1 else 16
    return [list(struct.unpack_from('<iii'if version else'<hhh',b,72+i*stride))for i in range(n)]

def commands(b):
    off=72
    while off+8<=len(b):
        a,c=struct.unpack_from('<II',b,off);op=a>>24;size=16 if op in (32,49,50,51,53,54,66,71)else 8
        h=None
        if size==16:
            hi,lo=struct.unpack_from('<II',b,off+8);h=hi<<32|lo
        yield (off-72)//8,op,a,c,h
        off+=size
        if op==0xdf:break

def derive(archive,alt=False):
    with zipfile.ZipFile(archive) as z:
        names=set(z.namelist());hashes={crc64(n.removeprefix('alt/')):n.removeprefix('alt/')for n in names}
        def get(n):return z.read('alt/'+n)if alt and 'alt/'+n in names else z.read(n)
        # Every setup uses room mesh roots. Walk those calls recursively with the
        # same active Alt/fallback resolver, never enumerate loose XML leaves.
        room_names=sorted(n for n in names if re.fullmatch(PREFIX+r'spot08_room_0(?:Set_[0-9A-F]+)?',n))
        setup_roots={n:sorted(set(m.decode()for m in re.findall(rb'scenes/shared/spot08_scene/spot08_room_0(?:Set_[0-9A-F]{6})?DL_[0-9A-F]{6}',get(n))))for n in room_names}
        assert len(setup_roots)==6
        assert all(roots==next(iter(setup_roots.values()))for roots in setup_roots.values())
        cache={};triangles=[];locations=[];active=set()
        def tri(ids,path,index):
            assert all(i in cache for i in ids)
            triangles.append([cache[i]for i in ids]);locations.append([path,index])
        def walk(path,stack=()):
            assert path not in stack,'display-list cycle'
            stack=(*stack,path);active.add(path);b=get(path)
            if b.startswith(b'<'):
                for index,el in enumerate(ET.fromstring(b)):
                    a=el.attrib
                    if el.tag=='LoadVertices':
                        vs=vertices(get(a['Path']));off=int(a.get('VertexOffset',0));dest=int(a.get('VertexBufferIndex',0))
                        cache.update({dest+i:vs[off+i]for i in range(int(a['Count']))})
                    elif el.tag in ('Triangles2','Triangle2','Triangle1'):
                        for prefix in ('V0','V1')if el.tag!='Triangle1'else ('V0',):tri([int(a[prefix+str(i)])for i in range(3)],path,index)
                    elif el.tag in ('CallDisplayList','JumpToDisplayList'):walk(a['Path'],stack)
            else:
                for index,op,a,c,h in commands(b):
                    if op==50:
                        vs=vertices(get(hashes[h]));n=a>>12&255;dest=((a>>1)&127)-n;off=c//16
                        cache.update({dest+i:vs[off+i]for i in range(n)})
                    elif op in (5,6):
                        tri([((a>>s)&255)//2 for s in (16,8,0)],path,index)
                        if op==6:tri([((c>>s)&255)//2 for s in (16,8,0)],path,index)
                    elif op in (49,53):walk(hashes[h],stack)
        for root in next(iter(setup_roots.values())):walk(root)
        v,polys,protected=collision(get(PREFIX+'spot08_sceneCollisionHeader_002CE0'))
        col=[[v[i&8191]for i in p[1:4]]for p in polys]
        # Reduce to ALL triangles intersecting the complete valley bounds + body
        # height. This cannot discard any obstacle to the tested corridor.
        lo=(1516,180,-458);hi=(4179,900,606)
        def overlaps(t):return all(max(p[k]for p in t)>=lo[k] and min(p[k]for p in t)<=hi[k]for k in range(3))
        backdrop=[i for i,loc in enumerate(locations)if loc[0]==PREFIX+'spot08_room_0DL_002538' and loc[1]in (27,28,42,43)]
        return {'render':[t for t in triangles if overlaps(t)],'backdrop':[triangles[i]for i in backdrop],
                'collision':[t for t in col if overlaps(t)],'setups':len(setup_roots),
                'active_lists':len(active),'protected_collision_metadata_sha256':protected,
                'protected_room_headers_sha256':hashlib.sha256(b''.join(get(n)for n in room_names)).hexdigest()}

def clip(poly,k,val,sign):
    out=[]
    for a,b in zip(poly,poly[1:]+poly[:1]):
        da=sign*(a[k]-val);db=sign*(b[k]-val)
        if da>=0:out.append(a)
        if (da>=0)!=(db>=0):out.append([a[j]+da/(da-db)*(b[j]-a[j])for j in range(3)])
    return out

def projected_area(poly):
    return abs(sum(a[0]*b[1]-a[1]*b[0] for a,b in zip(poly,poly[1:]+poly[:1])))/2

def edge_clip(poly,a,b,side):
    """Exact half-plane clipping, independent of the 3D obstacle-prism clipper."""
    def distance(p):return side*((b[0]-a[0])*(p[1]-a[1])-(b[1]-a[1])*(p[0]-a[0]))
    out=[]
    for p,q in zip(poly,poly[1:]+poly[:1]):
        dp,dq=distance(p),distance(q)
        if dp>=0:out.append(p)
        if (dp>=0)!=(dq>=0):
            u=dp/(dp-dq)
            out.append(tuple(p[k]+u*(q[k]-p[k]) for k in range(2)))
    return out

def unsupported_regions(floors,rectangle):
    """Subtract the projected floor UNION from the entire claimed rectangle.

    Fraction arithmetic makes this a complete-area proof, not a grid/ray probe
    or summed triangle areas (overlaps must not compensate for missing floor).
    """
    lo,hi,bottom,top=map(lambda x:Fraction(str(x)),rectangle)
    remaining=[[(lo,bottom),(hi,bottom),(hi,top),(lo,top)]]
    for floor in floors:
        triangle=[tuple(Fraction(str(p[k])) for k in (0,2)) for p in floor]
        a,b,c=triangle
        winding=(b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])
        if not winding:continue
        side=1 if winding>0 else -1
        next_remaining=[]
        for poly in remaining:
            inside=poly
            # Each outside piece is disjoint except for its closed boundary;
            # the residual inside all three edges is the covered intersection.
            for a,b in zip(triangle,triangle[1:]+triangle[:1]):
                outside=edge_clip(inside,a,b,-side)
                if projected_area(outside)>0:next_remaining.append(outside)
                inside=edge_clip(inside,a,b,side)
                if not inside:break
        remaining=next_remaining
        if not remaining:break
    return remaining

def require_complete_support(floors,rectangle,label):
    missing=unsupported_regions(floors,rectangle)
    area=sum(projected_area(poly) for poly in missing)
    assert not missing, f'{label}: unsupported floor area {float(area):.6f}, witness {missing[0][0]}'


def corridor(floors,bounds):
    lo,hi,half=bounds;mesh=[]
    for source in floors:
        t=source
        for k,v,sgn in ((0,lo,1),(0,hi,-1),(2,-half,1),(2,half,-1)):t=clip(t,k,v,sgn)
        for i in range(1,len(t)-1):
            tri=[t[0],t[i],t[i+1]];a,b,c=tri
            if abs((b[0]-a[0])*(c[2]-a[2])-(b[2]-a[2])*(c[0]-a[0]))>.001:mesh.append(tri)
    return mesh

def key(t):return tuple(sorted(tuple(p)for p in t))
def check(fixtures,mesh,floors,bands):
    # Union controls: duplicate overlapping coverage cannot conceal a hole.
    a=[[0,0,0],[1,0,0],[0,0,1]]
    b=[[1,0,0],[1,0,1],[0,0,1]]
    assert sum(projected_area(p) for p in unsupported_regions([a,a],(0,1,0,1)))==Fraction(1,2)
    require_complete_support([a,b],(0,1,0,1),'complete square control')
    # The original accepted center has ground, but its body square crosses the
    # slanted end of the floor. Retain this independent negative control.
    assert unsupported_regions(floors,(4082-18,4082+18,-18,18))
    for bounds in bands:
        lo,hi,half=bounds
        require_complete_support(floors,(lo,hi,-half,half),f'complete band {bounds}')
    for name,f in fixtures.items():
        original={key(t)for t in f['collision']}
        missing=[t for t in mesh if key(t)not in original]
        assert len(missing)==(0 if name=='export_alt'else 43),(name,len(missing))
        # Native control reproduces the old missing floor and visible obstruction.
        if name=='native':
            assert len(f['backdrop'])==8
            try:body_prism_clearance(corridor(floors,(1700,2200,120)),f['backdrop'],80,'old backdrop',list(range(len(corridor(floors,(1700,2200,120))))))
            except AssertionError:pass
            else:raise AssertionError('old native tree screen must obstruct the valley control')
        removed={key(t)for t in f['backdrop']}
        render=[t for t in f['render']if key(t)not in removed]
        for bounds in bands:
            route=corridor(floors,bounds)
            assert route
            for kind,obstacles in [('collision',f['collision']),('active render',render)]:
                body_prism_clearance(route,obstacles,80,name+' '+kind,list(range(len(route))))
        assert f['setups']==6
        # Exact rendered matches establish the donor-fit claim, independent of
        # collision normals or mesh indices. Native intentionally gains surfaces.
        if name.startswith('export'):
            rendered={key(t)for t in render}
            assert all(key(t)in rendered for t in mesh)
        print(f'PASS {name}: six active setups, missing collision {len(missing)}, targeted backdrop {len(removed)}, complete floor support and three body bands, protected collision metadata/room headers recorded')

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--native-archive');p.add_argument('--scene-archive');p.add_argument('--write-derived-fixture');p.add_argument('--bands',required=True,type=json.loads);a=p.parse_args()
    if a.native_archive or a.scene_archive:
        assert a.native_archive and a.scene_archive
        fixtures={'native':derive(a.native_archive),'export':derive(a.scene_archive),'export_alt':derive(a.scene_archive,True)}
        if a.write_derived_fixture:Path(a.write_derived_fixture).write_text(json.dumps(fixtures,separators=(',',':'))+'\n')
    else:fixtures=json.loads((Path(__file__).with_name('valley_derived_fixture.json')).read_text())
    rows=[[float(x)for x in line.split()]for line in sys.stdin if line.strip()]
    mesh=[[r[:3],r[3:6],r[6:9]]for r in rows];floors=[t for t,r in zip(mesh,rows)if r[9]]
    assert len(mesh)==43 and len(floors)==27
    check(fixtures,mesh,floors,a.bands)
