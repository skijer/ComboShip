#!/usr/bin/env python3
"""Verify the doorway follows the marked trunk face without changing scenery."""
import argparse
from collections import Counter
import math
from pathlib import Path
import re
import struct
import xml.etree.ElementTree as ET
import zipfile

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--baseline',type=Path,required=True)
p.add_argument('--candidate',type=Path,required=True)
modes=p.add_mutually_exclusive_group()
modes.add_argument('--center-left',action='store_true',help='Verify the POC6 to POC7 lateral centering adjustment')
modes.add_argument('--left-more',action='store_true',help='Verify the larger POC7 to POC8 leftward adjustment')
modes.add_argument('--tiny-right',action='store_true',help='Verify the small POC8 to POC9 rightward adjustment')
a=p.parse_args()
root='custom/cor_mm3d/lost_woods_poc1/room_0/'
old_root=root+'opa_combo_return_edge_v4';new_root=root+'opa_combo_return_aligned_v5'
door={root+n for n in ('ReturnRootTunnel','ReturnTunnelMouth','ReturnTunnelPinkLight','ReturnMushrooms_-1','ReturnMushrooms_1')}
angle=12288*math.tau/65536;s,c=math.sin(angle),math.cos(angle)
anchor=(-677.,-357.)
delta=None;version=5
if a.center_left:
    old_root=new_root;new_root=root+'opa_combo_return_centered_v6'
    anchor=(-684.,-340.);delta=(-7,17);version=6
elif a.left_more:
    old_root=root+'opa_combo_return_centered_v6';new_root=root+'opa_combo_return_centered_v7'
    anchor=(-698.,-306.);delta=(-14,34);version=7
elif a.tiny_right:
    old_root=root+'opa_combo_return_centered_v7';new_root=root+'opa_combo_return_centered_v8'
    anchor=(-695.,-313.);delta=(3,-7);version=8
if delta:
    lateral=delta[0]*c-delta[1]*s
    assert (0<lateral<10 if a.tiny_right else lateral<-18),'wrong adjustment direction or magnitude'
    assert abs(delta[0]*s+delta[1]*c)<.1,'doorway depth must be retained'


def xyz(v):return tuple(int(v.get(k)) for k in ('X','Y','Z'))


def triangles(z,path):
    vs=list(ET.fromstring(z.read('alt/'+path+'_vtx')));cache={};out=[]
    for command in ET.fromstring(z.read('alt/'+path)):
        if command.tag=='LoadVertices':
            cache.update({int(command.get('VertexBufferIndex'))+i:int(command.get('VertexOffset'))+i for i in range(int(command.get('Count')))})
        elif command.tag=='Triangle1':out.append([xyz(vs[cache[int(command.get(f'V0{i}'))]]) for i in range(3)])
    return out


def clip(poly,axis,bound,sign):
    out=[]
    for x,y in zip(poly,poly[1:]+poly[:1]):
        ix=sign*(x[axis]-bound)>=0;iy=sign*(y[axis]-bound)>=0
        if ix:out.append(x)
        if ix!=iy:
            t=(bound-x[axis])/(y[axis]-x[axis]);out.append(tuple(v+t*(w-v) for v,w in zip(x,y)))
    return out


def local(v):
    x,y,z=v;x-=anchor[0];z-=anchor[1]
    return (c*x-s*z,y,s*x+c*z)


with zipfile.ZipFile(a.baseline) as before,zipfile.ZipFile(a.candidate) as after:
    assert after.testzip() is None and len(after.namelist())==len(set(after.namelist()))
    assert set(after.namelist())-set(before.namelist())=={'alt/'+new_root},'missing aligned doorway identity'
    assert not set(before.namelist())-set(after.namelist())
    headers={n for n in before.namelist() if re.fullmatch(r'alt/scenes/nonmq/Z2_LOST_WOODS/Z2_LOST_WOODS_room_00(?:Set_[0-9A-F]+)?',n)}
    assert len(headers)==4
    changed={n for n in before.namelist() if before.read(n)!=after.read(n)}
    assert changed==headers|{'alt/'+n+'_vtx' for n in door},'texture or scenery changed during alignment'
    for n in headers:
        old=struct.pack('<I',len(old_root))+old_root.encode();new=struct.pack('<I',len(new_root))+new_root.encode()
        assert after.read(n).count(new)==1 and old_root.encode() not in after.read(n)
        assert after.read(n).replace(new,old)==before.read(n)
    count=0
    for n in door:
        old=list(ET.fromstring(before.read('alt/'+n+'_vtx')));new=list(ET.fromstring(after.read('alt/'+n+'_vtx')))
        assert len(old)==len(new)
        for x,y in zip(old,new):
            # Previous v4 depth is world X; its horizontal coordinate is -Z.
            u=-(int(x.get('Z'))+350);d=int(x.get('X'))+660
            expected=((int(x.get('X'))+delta[0],int(x.get('Y')),int(x.get('Z'))+delta[1]) if delta else
                      (round(anchor[0]+u*c+d*s),int(x.get('Y')),round(anchor[1]-u*s+d*c)))
            assert xyz(y)==expected,'doorway pieces do not share the new rigid transform'
            assert {k:v for k,v in x.attrib.items() if k not in ('X','Z')}=={k:v for k,v in y.attrib.items() if k not in ('X','Z')}
            count+=1
    visited=Counter()
    def walk(path,stack=()):
        assert path not in stack
        visited[path]+=1
        for command in ET.fromstring(after.read('alt/'+path)):
            if command.tag=='CallDisplayList':walk(command.get('Path'),stack+(path,))
    walk(new_root);walk(root+'xlu')
    assert all(visited[n]==1 for n in door),'missing or duplicate doorway'
    assert not any(visited[root+n] for n in ('opa_combo_return_v1','opa_combo_return_stump_v2','opa_combo_return_corner_v3','opa_combo_return_edge_v4'))
    if delta:assert visited[root+'opa_combo_return_aligned_v5']==0
    if a.left_more or a.tiny_right:assert visited[root+'opa_combo_return_centered_v6']==0
    if a.tiny_right:assert visited[root+'opa_combo_return_centered_v7']==0
    mouth=[local(xyz(v)) for v in ET.fromstring(after.read('alt/'+root+'ReturnTunnelMouth_vtx'))]
    assert all(abs(v[2]+24.5)<1.1 for v in mouth),'mouth is not parallel to the selected 67.5-degree plane'
    native=[];tree_triangles=[]
    for name in ('Geometry6','Geometry7','Geometry8'):
        for tri in triangles(after,root+name):
            tree_triangles.append(tri)
            poly=tri
            for axis,bound,sign in [(0,-1237,1),(0,-600,-1)]:
                if poly:poly=clip(poly,axis,bound,sign)
            poly=[local(v) for v in poly]
            for axis,bound,sign in [(0,-68,1),(0,68,-1),(1,0,1),(1,134,-1)]:
                if poly:poly=clip(poly,axis,bound,sign)
            native+=poly
    clearance=min(v[2] for v in mouth)-max(v[2] for v in native)
    assert clearance>1,'native roots cross the visible opening after rotation'
    # Independently fit the visible bark sections surrounding this opening.
    samples=[]
    for height in (60,90,120):
        segments=[]
        for tri in tree_triangles:
            points=[]
            for x,y in zip(tri,tri[1:]+tri[:1]):
                if min(x[1],y[1])<=height<max(x[1],y[1]):
                    t=(height-x[1])/(y[1]-x[1]);points.append(tuple(v+t*(w-v) for v,w in zip(x,y)))
            if len(points)==2:segments.append(points)
        for z in range(-430,-269,20):
            hits=[]
            for x,y in segments:
                if x[2]!=y[2] and min(x[2],y[2])<=z<=max(x[2],y[2]):
                    hit=x[0]+(y[0]-x[0])*(z-x[2])/(y[2]-x[2])
                    if -1237<hit<-600:hits.append(hit)
            if hits:samples.append((z,max(hits)))
    assert len(samples)==27
    mean_z=sum(z for z,x in samples)/len(samples);mean_x=sum(x for z,x in samples)/len(samples)
    bark_slope=sum((z-mean_z)*(x-mean_x) for z,x in samples)/sum((z-mean_z)**2 for z,x in samples)
    assert abs(-c/s-bark_slope)<.025,'doorway plane does not follow the local trunk surface'
    bark_yaw=math.degrees(math.atan2(1,-bark_slope))
    print(f'PASS aligned doorway: yaw 12288 (67.5 degrees), measured bark normal {bark_yaw:.2f} degrees, anchor {anchor}')
    print(f'PASS {count} rigidly moved doorway vertices; native-root clearance {clearance:.2f}')
    print(f'PASS exact nine-resource delta + v{version} wrapper; four setups; one doorway; all textures, mushroom/stump repairs and scenery byte-identical')
