"""No native foliage triangle passes through the diagonal bridge's wood."""
from pathlib import Path
import sys
import numpy as np
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'tools/woods_mystery'))
from build_candidate import Resources,ROOT,SCENE,read_mesh_header

r=Resources(sys.argv[2],sys.argv[1]);axis=np.array([1,-1])/np.sqrt(2);side=np.array([1,1])/np.sqrt(2)
wood=[]
for t in r.mesh(ROOT+'room_1/geometry'):
    v=t['v'];q=v[:,[0,2]]-[110,-110]
    if t['path']==ROOT+'room_1/geometry' and (t['texture'] or '').endswith('/lw_tutu_01_2A000') and (q@axis).min()>=-1 and (q@axis).max()<=152 and abs(q@side).max()<=70:
        wood.append(t)
assert len(wood)==142
_,_,entries=read_mesh_header(r.get(SCENE+'Z2_26SARUNOMORI_room_01'))
foliage=[t for p in dict.fromkeys(p for _,o,x in entries for p in(o,x) if p) for t in r.mesh(p)
         if (t['texture'] or '').startswith(ROOT+'textures/henriko_')]

def crosses(a,b,triangle):
    p,q,r=triangle;direction=b-a;e1=q-p;e2=r-p;h=np.cross(direction,e2);det=e1@h
    if abs(det)<1e-9:return False
    s=a-p;u=(s@h)/det;c=np.cross(s,e1);v=(direction@c)/det;distance=(e2@c)/det
    return u>=-1e-7 and v>=-1e-7 and u+v<=1+1e-7 and 1e-6<distance<1-1e-6

hits=[]
for i,f in enumerate(foliage):
    a=f['v'][:,:3]
    for j,w in enumerate(wood):
        b=w['v'][:,:3]
        if ((a.min(0)>b.max(0))|(b.min(0)>a.max(0))).any():continue
        if any(crosses(t[k],t[(k+1)%3],other) for t,other in[(a,b),(b,a)] for k in range(3)):
            hits.append((f['path'],i,j))
assert not hits,f'Canopy enters the approved bridge wood: {len(hits)} intersecting pairs, first{hits[:3]}'
print(f'PASS: {len(foliage)} native foliage triangles clear all142bridge plank triangles; walltop alpha coverage is checked separately')
