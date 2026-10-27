#!/usr/bin/env python3
"""Native adult spot08 ice versus the complete production shelf, continuously.

The fixtures below were decoded from the native room_0Set_000330 actor list
and object_spot08_obj visual/collision meshes. Both native visual meshes are
geometrically identical to their collision meshes (including the submerged
tips). No donor archive is needed for the default regression. --native-archive
additionally compares those exact resources to an available donor or candidate.

A sphere around every scaled native mesh encloses EVERY yaw/roll orientation,
including player-driven roll. The twin's full 100-unit orbital disk is added.
Distance to every rounded shelf triangle's XZ projection therefore proves
continuous separation even where its foundation extends below native bank ground. This is
deliberately stronger than a finite set of poses or actor AABBs. Two extra
units cover dynamic collision's integer truncation and float roundoff; the
minimum gap is required in addition to that reserve. Bobbing/sinking change
only Y and cannot invalidate a strictly horizontal separation proof.
"""
from pathlib import Path
import argparse
import math
import re
import struct
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[2]
ICEBERG = (
    (1469,-1262,7),(1473,150,-842),(1465,150,855),(729,-1262,1276),
    (-8,150,1696),(-740,-1262,1269),(-1473,150,842),(-1469,-1262,-7),
    (-1465,150,-855),(-729,-1262,-1276),(8,150,-1696),(740,-1262,-1269),(0,-1985,0),
)
ICEBERG_FACES = (
    (0,1,2),(2,3,0),(2,4,3),(4,5,3),(4,6,5),(7,5,6),(6,8,7),(8,9,7),
    (9,8,10),(10,11,9),(11,10,1),(1,0,11),(5,12,3),(7,12,5),(9,12,7),
    (11,12,9),(0,12,11),(3,12,0),(1,10,8),(1,8,6),(1,6,4),(1,4,2),
)
RAMP = (
    (2800,-571,-511),(2400,-571,-2165),(-469,533,-2000),(180,498,-673),
    (3921,1422,724),(3921,1422,-489),(2278,1422,-273),(2081,1422,1073),
    (-615,919,-192),(-156,1005,1643),(-2085,594,-1916),(-2731,622,-1531),
    (-3200,622,-400),(-2731,622,731),(-1600,622,1200),(-69,-3378,1600),
    (-469,-571,2000),(-1852,-2874,-400),(-469,-3378,-2000),(-869,-571,-2400),
    (2264,-571,2000),(2256,-3378,611),(3200,-571,1600),(1052,-571,-2527),
    (2034,-2722,-2054),(3600,-571,-111),
)
RAMP_FACES = (
    (0,1,2),(0,2,3),(4,5,6),(4,6,7),(6,8,9),(6,9,7),(5,0,6),(0,3,6),
    (3,8,6),(2,10,8),(10,11,8),(11,12,8),(12,13,8),(13,14,8),
    (15,16,14),(16,9,14),(17,15,14),(17,14,13),(17,13,12),(17,12,11),
    (17,11,10),(10,18,17),(18,10,19),(10,2,19),(7,9,20),(9,16,20),
    (16,15,20),(15,21,20),(4,20,22),(4,7,20),(20,21,22),(1,23,2),
    (23,19,2),(1,24,23),(24,18,23),(18,19,23),(24,1,0),(25,4,22),
    (25,5,4),(25,0,5),(21,25,22),(21,0,25),(0,21,24),(8,3,2),
    (8,14,9),(17,18,15),(18,21,15),(18,24,21),
)
# Actor index, home X/Y/Z, native home yaw, params. Adult night uses the same
# list by native scene-layer-3 fallback. Orbit actor39 also spawns param0x123.
ADULT_ACTORS = (
    (29,-1200,0,0,0,0x214),(31,-185,20,-32,16384,0x1),
    (32,974,20,90,0,0x11),(33,-214,20,-1393,0,0x11),
    (34,255,20,-285,16384,0x10),(35,-253,20,-834,0,0x12),
    (36,35,20,-1125,0,0x12),(37,102,20,-628,0,0x12),
    (38,-649,20,-10,0,0x20),(39,599,20,-87,0,0x23),
)
NATIVE_ICE_EXIT = ((238,107,-1850),(-364,188,-2884),(-792,107,-1786))
# Native collision141 and the corresponding native visible cliff triangle.
# Its shallowest overlap with corrected floor19 lies BETWEEN the old ray grid
# samples. Exact minimum85.9732577, so a sampled96-height test was insufficient.
NATIVE_CRITICAL_ROOF = ((13,235,-1645),(351,92,-1656),(447,2379,-1836))
ROOF_CONTROL_FLOOR = ((63,100,-1672),(323,200,-1265),(477,200,-1435))


def cross(a,b):
    return a[0]*b[1]-a[1]*b[0]


def sub(a,b):
    return tuple(x-y for x,y in zip(a,b))


def dot(a,b):
    return sum(x*y for x,y in zip(a,b))


def point_segment(p,a,b):
    d=sub(b,a)
    u=max(0.,min(1.,dot(sub(p,a),d)/dot(d,d))) if dot(d,d) else 0.
    return math.dist(p,tuple(x+u*y for x,y in zip(a,d)))


def point_triangle_xz(p,triangle):
    a,b,c=[(v[0],v[2]) for v in triangle]
    denominator=cross(sub(b,a),sub(c,a))
    if abs(denominator)>1e-10:
        u=cross(sub(p,a),sub(c,a))/denominator
        v=cross(sub(b,a),sub(p,a))/denominator
        if u>=0 and v>=0 and u+v<=1:
            return 0.
    return min(point_segment(p,a,b),point_segment(p,b,c),point_segment(p,c,a))


def triangles_xz_distance(a,b):
    aa=[(v[0],v[2])for v in a];bb=[(v[0],v[2])for v in b]
    for i in range(3):
        p,q=aa[i],aa[(i+1)%3]
        for j in range(3):
            r,s=bb[j],bb[(j+1)%3]
            c1,c2=cross(sub(q,p),sub(r,p)),cross(sub(q,p),sub(s,p))
            c3,c4=cross(sub(s,r),sub(p,r)),cross(sub(s,r),sub(q,r))
            if c1*c2<0 and c3*c4<0:return 0.
            if min(point_segment(p,r,s),point_segment(q,r,s),
                   point_segment(r,p,q),point_segment(s,p,q))<1e-9:return 0.
    return min([point_triangle_xz(p,b)for p in aa]+[point_triangle_xz(p,a)for p in bb])


def exit_clearance(mesh,exit_triangle=NATIVE_ICE_EXIT,minimum_gap=120.):
    distance,triangle=min((triangles_xz_distance(t,exit_triangle),i)for i,t in enumerate(mesh))
    assert distance>minimum_gap, ('Shelf approaches native Ice Cavern exit footprint',distance,triangle)
    print(f'PASS exact full-shelf/native Ice Cavern exit footprint clearance {distance:.3f} (triangle{triangle})')
    return distance,triangle


def floor_projection_clip(floor,native):
    """Clip native 3D triangle to an entire floor triangle's vertical prism.

    The gap to the affine floor plane is linear, so extrema are attained at
    clipped-polygon vertices. This also handles vertical native walls whose
    projected triangle has zero area; no ray samples are involved.
    """
    f=[(v[0],v[2])for v in floor]
    denominator=cross(sub(f[1],f[0]),sub(f[2],f[0]))
    assert abs(denominator)>1e-10, 'Walkable floor must have nonzero XZ area'
    sign=1 if denominator>0 else -1
    polygon=[tuple(p)for p in native]
    for i in range(3):
        a,b=f[i],f[(i+1)%3]
        signed=lambda p:sign*cross(sub(b,a),sub((p[0],p[2]),a))
        clipped=[]
        if not polygon:return []
        previous=polygon[-1];dp=signed(previous)
        for current in polygon:
            dc=signed(current)
            if (dc>=-1e-8)!=(dp>=-1e-8):
                u=dp/(dp-dc)
                clipped.append(tuple(p+u*(q-p)for p,q in zip(previous,current)))
            if dc>=-1e-8:clipped.append(current)
            previous,dp=current,dc
        polygon=clipped
    result=[]
    for p in polygon:
        q=(p[0],p[2]);u=cross(sub(q,f[0]),sub(f[2],f[0]))/denominator
        v=cross(sub(f[1],f[0]),sub(q,f[0]))/denominator
        floor_y=floor[0][1]+u*(floor[1][1]-floor[0][1])+v*(floor[2][1]-floor[0][1])
        result.append((p[1]-floor_y,(p[0],floor_y,p[2]),p))
    return result


def body_prism_clearance(mesh,native,height=80.,label='native',floor_indices=None):
    if floor_indices is None:floor_indices=[i for i in range(len(mesh))if i%6<2]
    lowest=None;failures=[];max_ground_join=0.
    for fi in floor_indices:
        floor=mesh[fi]
        bounds=[min(p[k]for p in floor)for k in (0,2)]+[max(p[k]for p in floor)for k in (0,2)]
        for ni,triangle in enumerate(native):
            if (max(p[0]for p in triangle)<bounds[0] or min(p[0]for p in triangle)>bounds[2] or
                max(p[2]for p in triangle)<bounds[1] or min(p[2]for p in triangle)>bounds[3]):continue
            clipped=floor_projection_clip(floor,triangle)
            if not clipped:continue
            low=min(clipped,key=lambda p:p[0]);high=max(p[0]for p in clipped)
            # Accepted native ground joins: collision<=0.361, IA8 visual<=1.232.
            # An explicit3-unit step/contact allowance is separate from the
            # overhead proof; this is not a claim of zero geometry intersections.
            if high<=3.+1e-7:
                max_ground_join=max(max_ground_join,high)
                continue
            if low[0]<height-1e-7:
                failures.append((fi,ni,low[0],high,low[1]))
            if low[0]>=3. and (lowest is None or low[0]<lowest[0]):
                lowest=(low[0],fi,ni,low[1])
    assert not failures, f'{label} enters complete{height:g}-unit floor/body prism: {failures[:5]}'
    if lowest:print(f'PASS exact complete {label} body prisms: min overhead{lowest[0]:.6f}, guard{height:g}, native ground join<={max_ground_join:.6f} (floor{lowest[1]})')
    return lowest


def native_footing(mesh,native):
    def floor_at(triangles,x,z,ceiling):
        ys=[]
        for a,b,c in triangles:
            ab=(b[0]-a[0],b[2]-a[2]);ac=(c[0]-a[0],c[2]-a[2]);q=(x-a[0],z-a[2]);det=cross(ab,ac)
            if det>=-1e-8:continue # Only upward, nonvertical native ground.
            u,v=cross(q,ac)/det,cross(ab,q)/det
            if min(u,v,1-u-v)>=-1e-7:
                y=a[1]+u*(b[1]-a[1])+v*(c[1]-a[1])
                if y<=ceiling+3:ys.append(y)
        return max(ys)if ys else None
    floors=[t for i,t in enumerate(mesh)if i%6<2]
    base=min(p[1]for t in mesh for p in t);samples=set()
    for t in mesh:
        bottom=[p for p in t if p[1]==base]
        if len(bottom)!=2:continue
        a,b=bottom
        for step in range(21):
            x=a[0]+(b[0]-a[0])*step/20;z=a[2]+(b[2]-a[2])*step/20
            if x<1530:samples.add((x,z)) # Native bank; beyond this is the added shared valley floor.
    lowest=10000
    for x,z in samples:
        top=floor_at(floors,x,z,1000)
        assert top is not None
        ground=floor_at(native,x,z,top)
        assert ground is not None,('missing native bank footing',x,z)
        assert base<=ground+1e-6,('unsupported ascent foundation',base,ground,x,z)
        lowest=min(lowest,ground)
    assert len(samples)>100
    print(f'PASS native bank footing: {len(samples)} actual foundation-edge samples, lowest support{lowest:.3f} above base{base:g}')


def critical_roof_controls(mesh):
    witness=min(floor_projection_clip(ROOF_CONTROL_FLOOR,NATIVE_CRITICAL_ROOF),key=lambda p:p[0])
    assert abs(witness[0]-85.97325773682807)<1e-6
    assert 80.<witness[0]<96., 'The old96-height sampled guard must fail continuously'
    body_prism_clearance(mesh,[NATIVE_CRITICAL_ROOF],label='native critical roof')


def rounded(value):
    # Production Build() uses C++ lround, away from zero on a tie.
    return math.copysign(math.floor(abs(value)+.5),value)


def load_mesh(mesh_text):
    rows=[[rounded(float(v)) for v in line.split()] for line in mesh_text.splitlines() if line.strip()]
    assert rows and all(len(row)==9 for row in rows), 'Expected actual production triangle dump'
    return [tuple(tuple(row[i:i+3]) for i in (0,3,6)) for row in rows]


def source_motion_controls():
    source=(ROOT/'soh/src/overlays/actors/ovl_Bg_Spot08_Iceblock/z_bg_spot08_iceblock.c').read_text()
    compact=re.sub(r'\s+','',source)
    for statement in (
        'Actor_SetScale(&this->dyna.actor,0.2f);',
        'Actor_SetScale(&this->dyna.actor,0.1f);',
        'Actor_SetScale(&this->dyna.actor,0.05f);',
        'sin=Math_SinS(this->dyna.actor.world.rot.y)*100.0f;',
        'cos=Math_CosS(this->dyna.actor.world.rot.y)*100.0f;',
        'this->dyna.actor.world.pos.x=this->dyna.actor.home.pos.x+sin;',
        'this->dyna.actor.world.pos.z=this->dyna.actor.home.pos.z+cos;',
        'this->dyna.actor.child->world.pos.x=this->dyna.actor.home.pos.x-sin;',
        'this->dyna.actor.child->world.pos.z=this->dyna.actor.home.pos.z-cos;',
        'this->dyna.actor.world.pos.y=this->sinkOffset+this->bobOffset+this->dyna.actor.home.pos.y;',
    ):
        assert statement in compact, 'Re-audit changed native motion: '+statement
    # Ensure no native XZ translation has been added outside the known orbit.
    xz_assignments=re.findall(r'this->dyna.actor.world.pos.[xz](?:[+\-*/]?=)',compact)
    assert len(xz_assignments)==6, 'Re-audit additional native XZ motion'


def cross3(a,b):
    return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])


def segment_triangle(a,b,t):
    e1,e2=sub(t[1],t[0]),sub(t[2],t[0]);direction=sub(b,a)
    h=cross3(direction,e2);det=dot(e1,h)
    if abs(det)<1e-9:return None
    s=sub(a,t[0]);u=dot(s,h)/det;q=cross3(s,e1)
    v=dot(direction,q)/det;d=dot(e2,q)/det
    if min(u,v,1-u-v,d,1-d)>=-1e-9:
        return tuple(x+d*y for x,y in zip(a,direction))
    return None


def old_shelf_negative_control():
    # Actual former production foundation triangle10 and actual native actor33
    # collision face18 at its upright home pose. These cross at y35, even though
    # the old WALKING surface sat at y96. This keeps the original defect concrete.
    old_side=((-273,96,-1485),(-273,-50,-1485),(-124,-50,-1424))
    home=(-214,20,-1393)
    native=tuple(tuple(int(h+.1*v)for h,v in zip(home,ICEBERG[i]))for i in ICEBERG_FACES[18])
    witnesses=[]
    for a,b in ((old_side,native),(native,old_side)):
        for i in range(3):
            p=segment_triangle(a[i],a[(i+1)%3],b)
            if p is not None:witnesses.append(p)
    assert len(witnesses)>=2, 'Original foundation crossing negative control lost'
    assert any(math.dist(p,(-273,35,-1485))<1e-6 for p in witnesses)
    print('PASS original-defect control: actual native face18 crosses former foundation10 at(-273,35,-1485)')


def collision_resource(data):
    n=struct.unpack_from('<I',data,76)[0]
    vertices=tuple(struct.unpack_from('<hhh',data,80+6*i) for i in range(n))
    off=80+6*n
    n=struct.unpack_from('<I',data,off)[0]
    faces=tuple(tuple(v&8191 for v in struct.unpack_from('<3H',data,off+6+16*i)) for i in range(n))
    return vertices,faces


def verify_archive(path,mesh=None):
    # Factory-exact room command decoding; validates all child/adult placements.
    archive_path=path
    def actors(data):
        pos=64
        def take(fmt):
            nonlocal pos
            v=struct.unpack_from('<'+fmt,data,pos);pos+=struct.calcsize('<'+fmt)
            return v[0] if len(v)==1 else v
        def string():
            nonlocal pos
            n=take('I');s=data[pos:pos+n].decode();pos+=n;return s
        result=[];alternate=[]
        for _ in range(take('I')):
            cmd=take('I')
            if cmd==24:alternate=[string()for _ in range(take('I'))]
            elif cmd==1:
                for i in range(take('I')):
                    a=take('H6hH')
                    if a[0]==0xD1:result.append((i,*a[1:4],a[5],a[7]))
            elif cmd in (8,2):pos+=5
            elif cmd in (16,21,7):pos+=3
            elif cmd==18:pos+=2
            elif cmd==22:pos+=1
            elif cmd==11:
                n=take('I');pos+=2*n
            elif cmd==10:
                _,typ,n=take('3B');assert typ==2
                for _ in range(n):pos+=9;string();string()
            elif cmd==20:pass
            else:raise AssertionError(('Unrecognized room command',cmd))
        assert pos==len(data)
        return tuple(result),alternate
    base='scenes/shared/spot08_scene/'
    with zipfile.ZipFile(path) as archive:
        found,alts=actors(archive.read(base+'spot08_room_0Set_000330'))
        assert found==ADULT_ACTORS, 'Native adult placements changed; re-audit'
        child,alts=actors(archive.read(base+'spot08_room_0'))
        assert child==((0,-1200,0,0,0,0x214),)
        night,_=actors(archive.read(base+'spot08_room_0Set_000700'))
        assert night==child
        assert alts[:3]==[base+'spot08_room_0Set_000700',base+'spot08_room_0Set_000330','']
        scene_vertices,scene_faces=collision_resource(archive.read(base+'spot08_sceneCollisionHeader_002CE0'))
        assert tuple(scene_vertices[i]for i in scene_faces[87])==NATIVE_ICE_EXIT
        assert tuple(scene_vertices[i]for i in scene_faces[141])==NATIVE_CRITICAL_ROOF
        for name,vertices,faces in [('Iceberg',ICEBERG,ICEBERG_FACES),('IceRamp',RAMP,RAMP_FACES)]:
            path='objects/object_spot08_obj/gZorasFountain'+name+'Col'
            assert collision_resource(archive.read(path))==(vertices,faces), 'Native mesh changed: '+path
    sys.path.insert(0,str(ROOT/'tools/woods_mystery'))
    from scene_assets import Resources
    resources=Resources(archive_path)
    for name,vertices,faces in [('Iceberg',ICEBERG,ICEBERG_FACES),('IceRamp',RAMP,RAMP_FACES)]:
        rendered=resources.mesh('objects/object_spot08_obj/gZorasFountain'+name+'DL')
        canonical=lambda triangles:sorted(tuple(sorted(tuple(p)for p in t))for t in triangles)
        assert canonical([t['v'][:,:3]for t in rendered])==canonical([[vertices[i]for i in face]for face in faces]), 'Native visual mesh changed: '+name
    print('PASS donor resource check: exact native child/adult actors and visual/collision meshes')
    if mesh is not None:
        native_collision=[[scene_vertices[i]for i in face]for face in scene_faces]
        body_prism_clearance(mesh,native_collision,label='native complete scene collision')
        native_footing(mesh,native_collision)
        rendered=[]
        for name in resources.files:
            if re.fullmatch(r'scenes/shared/spot08_scene/spot08_room_0DL_[0-9A-F]+',name):
                rendered.extend(t['v'][:,:3].tolist()for t in resources.mesh(name))
        assert rendered, 'Missing native room display lists'
        assert any(canonical([t])==canonical([NATIVE_CRITICAL_ROOF])for t in rendered), 'Critical visible cliff fixture missing'
        rendered=[t for t in rendered if not all(v[1]==20 for v in t)]
        body_prism_clearance(mesh,rendered,label='native complete scene visual')


def check(mesh,minimum_gap=24.):
    source_motion_controls()
    old_shelf_negative_control()
    critical_roof_controls(mesh)
    results=[]
    for index,x,y,z,yaw,params in ADULT_ACTORS:
        vertices=RAMP if params&0x200 else ICEBERG
        scale={0:.2,0x10:.1,0x20:.05}[params&0xF0]
        radius=max(math.sqrt(dot(v,v))for v in vertices)*scale
        orbit=100. if params&15==3 else 0.
        distance,triangle=min((point_triangle_xz((x,z),t),i) for i,t in enumerate(mesh))
        gap=distance-radius-orbit-2.
        results.append((gap,index,params,triangle))
    failures=[r for r in results if r[0]<minimum_gap]
    assert not failures, ('Native adult ice sweep approaches/intersects full shelf; '
        'gap excludes mesh radius, complete100-unit twin orbit and2-unit reserve: '+
        repr([(round(g,4),i,hex(p),t)for g,i,p,t in failures]))
    gap,index,params,triangle=min(results)
    print(f'PASS all10 native adult placements + spawned twin: continuous visual/collision '
          f'sweep clearance >= {gap:.3f} after2-unit reserve (actor{index}, shelf triangle{triangle})')
    exit_clearance(mesh)
    return results


if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('--mesh',type=Path,help='Production binary --mesh output; stdin if omitted')
    parser.add_argument('--native-archive',type=Path)
    parser.add_argument('--minimum-gap',type=float,default=24.)
    args=parser.parse_args()
    mesh=load_mesh(args.mesh.read_text() if args.mesh else sys.stdin.read())
    check(mesh,args.minimum_gap)
    if args.native_archive:verify_archive(args.native_archive,mesh)
