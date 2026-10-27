#!/usr/bin/env python3
"""Check mushroom chart continuity and the screenshot-marked room-edge portal."""
import argparse
from collections import Counter,defaultdict
from pathlib import Path
import struct
import re
import xml.etree.ElementTree as ET
import zipfile
import numpy as np

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--baseline',type=Path,required=True)
parser.add_argument('--candidate',type=Path,required=True)
args=parser.parse_args()


def triangles(z,path):
    vertices=list(ET.fromstring(z.read(path+'_vtx')))
    cache={};out=[]
    for command in ET.fromstring(z.read(path)):
        if command.tag=='LoadVertices':
            cache.update({int(command.get('VertexBufferIndex'))+i:int(command.get('VertexOffset'))+i
                          for i in range(int(command.get('Count')))})
        elif command.tag=='Triangle1':
            out.append([vertices[cache[int(command.get(f'V0{i}'))]] for i in range(3)])
    return out


def position(v):return tuple(int(v.get(c)) for c in ('X','Y','Z'))


def chart(t):
    a,b,c=map(position,t)
    # Cross-product Y distinguishes the cap/lip from the downward-facing gills.
    ny=(b[2]-a[2])*(c[0]-a[0])-(b[0]-a[0])*(c[2]-a[2])
    return 'cap' if ny>=0 else 'underside'


failures=[]
with zipfile.ZipFile(args.baseline) as before,zipfile.ZipFile(args.candidate) as after:
    assert after.testzip() is None
    assert len(after.namelist())==len(set(after.namelist()))
    root='custom/cor_mm3d/lost_woods_poc1/room_0/'
    old_root=root+'opa_combo_return_corner_v3';new_root=root+'opa_combo_return_edge_v4'
    doorway={root+n for n in ('ReturnRootTunnel','ReturnTunnelMouth','ReturnTunnelPinkLight','ReturnMushrooms_-1','ReturnMushrooms_1')}
    headers={n for n in before.namelist() if re.fullmatch(r'alt/scenes/nonmq/Z2_LOST_WOODS/Z2_LOST_WOODS_room_00(?:Set_[0-9A-F]+)?',n)}
    assert len(headers)==4
    assert set(after.namelist())-set(before.namelist())=={'alt/'+new_root}
    assert not set(before.namelist())-set(after.namelist())
    texture='alt/custom/cor_mm3d/lost_woods_poc1/textures/lw_kinoko_01_FC00'
    changed_resources={n for n in before.namelist() if before.read(n)!=after.read(n)}
    assert changed_resources==headers|{texture}|{'alt/'+n+'_vtx' for n in doorway},changed_resources
    old_token=struct.pack('<I',len(old_root))+old_root.encode();new_token=struct.pack('<I',len(new_root))+new_root.encode()
    for n in headers:
        assert after.read(n).count(new_token)==1 and old_root.encode() not in after.read(n)
        assert after.read(n).replace(new_token,old_token)==before.read(n),'room header changed outside active mesh token'
        assert (root+'xlu').encode() in after.read(n)
    for name in doorway:
        old=ET.fromstring(before.read('alt/'+name+'_vtx'));new=ET.fromstring(after.read('alt/'+name+'_vtx'))
        assert len(old)==len(new)
        for a,b in zip(old,new):
            assert int(b.get('X'))==int(a.get('X'))-270 and int(b.get('Z'))==int(a.get('Z'))-470
            assert {k:v for k,v in a.attrib.items() if k not in ('X','Z')}=={k:v for k,v in b.attrib.items() if k not in ('X','Z')}
    visited=Counter()
    def walk(path,stack=()):
        assert path not in stack,'cyclic display list'
        visited[path]+=1
        for cmd in ET.fromstring(after.read('alt/'+path)):
            if cmd.tag=='CallDisplayList':walk(cmd.get('Path'),stack+(path,))
    walk(new_root);walk(root+'xlu')
    assert all(visited[n]==1 for n in doorway),'missing or duplicated active doorway'
    assert not any(visited[root+name] for name in ('opa_combo_return_corner_v3','opa_combo_return_stump_v2','opa_combo_return_v1'))
    original=before.read(texture);repaired=after.read(texture)
    assert original[:92]==repaired[:92]
    _,w,h,_=struct.unpack_from('<4I',original,64)
    old_pixels=np.frombuffer(original[92:],np.uint8).reshape(h,w,4)
    pixels=np.frombuffer(repaired[92:],np.uint8).reshape(h,w,4)
    assert np.array_equal(old_pixels[:,:,3],pixels[:,:,3])
    changed=np.any(old_pixels!=pixels,axis=2)
    yy,xx=np.indices((h,w))
    left=np.where(yy<=379,512+256*(yy-10)/369,768+246*(yy-379)/450)
    def near_segment(ax,ay,bx,by):
        t=np.clip(((xx-ax)*(bx-ax)+(yy-ay)*(by-ay))/((bx-ax)**2+(by-ay)**2),0,1)
        return np.hypot(xx-(ax+t*(bx-ax)),yy-(ay+t*(by-ay)))<12
    allowed=((xx<24)|(yy>=h-24)|(xx+yy<85)|(xx+yy>1960)|
             near_segment(51,10,707,317)|near_segment(707,317,1014,973)|
             ((yy<845)&((abs(xx-left)<25)|(xx>995))))
    assert not np.any(changed&~allowed),'mushroom artwork outside seam borders changed'
    assert changed.sum()<w*h*.08,'repair exceeds narrow atlas border scope'
    def sample(uv):
        x,y=(uv/1024*np.array([w,h])-.5)%np.array([w,h]);ix,iy=int(x),int(y);fx,fy=x-ix,y-iy
        return ((pixels[iy,ix,:3]*(1-fx)+pixels[iy,(ix+1)%w,:3]*fx)*(1-fy)+
                (pixels[(iy+1)%h,ix,:3]*(1-fx)+pixels[(iy+1)%h,(ix+1)%w,:3]*fx)*fy)
    for room,name in [(0,'Geometry2'),(1,'Geometry13')]:
        path=f'alt/custom/cor_mm3d/lost_woods_poc1/room_{room}/{name}'
        assert before.read(path)==after.read(path) and before.read(path+'_vtx')==after.read(path+'_vtx'),'mushroom geometry/UV/draw data changed'
        old,new=triangles(before,path),triangles(after,path)
        assert len(old)==len(new)
        edges=defaultdict(list)
        for a,b in zip(old,new):
            assert [{k:v for k,v in x.attrib.items() if k not in ('S','T')} for x in a]==[
                {k:v for k,v in x.attrib.items() if k not in ('S','T')} for x in b], 'mushroom shape/color/order changed'
            for x,y in zip(b,b[1:]+b[:1]):
                if position(x)>position(y):x,y=y,x
                edges[(position(x),position(y))].append((chart(b),x,y))
        counts=defaultdict(int);bad=defaultdict(float)
        for edge,entries in edges.items():
            if len(entries)!=2 or entries[0][0]!=entries[1][0]:continue
            kind,x,y=entries[0];_,u,v=entries[1]
            a=np.array([[int(q.get(c)) for c in ('S','T')] for q in (x,y)],float)%1024
            b=np.array([[int(q.get(c)) for c in ('S','T')] for q in (u,v)],float)%1024
            cap=np.array_equal(a,1024-b[:,::-1])
            hi,lo=(a,b) if a[:,0].sum()>=b[:,0].sum() else (b,a)
            top=(lo[:,1]<=379).all()
            expected=512+256*(lo[:,1]-10)/369 if top else 768+246*(lo[:,1]-379)/450
            gill=(np.array_equal(hi[:,1],lo[:,1]) and np.all(hi[:,0]==1014) and
                  np.all(lo[:,1]>=10) and np.all(lo[:,1]<=829) and
                  np.all(abs(lo[:,0]-expected)<=10))
            if not(cap or gill):continue
            kind='cap radial join' if cap else 'gill sector join'
            counts[kind]+=1
            jump=max(float(abs(sample(a[0]*(1-t)+a[1]*t)-sample(b[0]*(1-t)+b[1]*t)).max()) for t in np.linspace(0,1,13))
            if jump>1:
                print('SEAM FAIL',kind,a.tolist(),b.tolist(),jump)
                bad[kind]=max(bad[kind],jump)
        if bad:failures.append(f'{name}: visible seam RGB jumps {dict(bad)}')
        assert counts['gill sector join']>50,'no meaningful underside seam coverage'
        if room==1:assert counts['cap radial join']>80,'no meaningful cap seam coverage'
        print(name,'tested painted joins',dict(counts))
    mouth=ET.fromstring(after.read('alt/custom/cor_mm3d/lost_woods_poc1/room_0/ReturnTunnelMouth_vtx'))
    if not all(-685<=position(v)[0]<=-684 and -418<=position(v)[2]<=-282 for v in mouth):
        failures.append('portal is not at the large room-edge trunk marked in the new screenshot')
    def clip(poly,axis,bound,sign):
        result=[]
        for a,b in zip(poly,poly[1:]+poly[:1]):
            ia=sign*(a[axis]-bound)>=0;ib=sign*(b[axis]-bound)>=0
            if ia:result.append(a)
            if ia!=ib:
                t=(bound-a[axis])/(b[axis]-a[axis]);result.append(tuple(x+t*(y-x) for x,y in zip(a,b)))
        return result
    native=[]
    for name in ('Geometry6','Geometry7','Geometry8'):
        for tri in triangles(after,'alt/'+root+name):
            poly=list(map(position,tri))
            for axis,bound,sign in [(0,-1237,1),(0,-600,-1),(1,0,1),(1,134,-1),(2,-418,1),(2,-282,-1)]:
                if not poly:break
                poly=clip(poly,axis,bound,sign)
            native.extend(poly)
    assert native and max(v[0] for v in native)<min(position(v)[0] for v in mouth),'native trunk protrudes through mouth'
    # Preserve the accepted cut-stump result and every texture, byte for byte.
    for name in before.namelist():
        if ('/textures/' in name and name!=texture) or name.endswith('/Geometry4_vtx') or name.endswith('/Geometry4'):
            assert before.read(name)==after.read(name), 'accepted texture/stump resource changed: '+name
    if failures:raise AssertionError('\n'.join(failures))
    print(f'PASS painted cap/gill joins; {int(changed.sum())} RGB pixels ({changed.mean():.2%}); all mushroom geometry/UVs and accepted stump preserved')
    print('PASS exact archive delta, four v4 setups, one active doorway, rigid move from clearing, native trunk behind mouth')
