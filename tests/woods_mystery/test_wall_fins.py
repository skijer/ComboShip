"""No curved native wall fin projects inside the fitted corridor backing."""
from pathlib import Path
import sys
import numpy as np
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'tools/woods_mystery'))
from build_candidate import Resources,ROOT,SCENE,read_mesh_header

current=Resources(sys.argv[2],sys.argv[1]);previous=Resources(sys.argv[2],sys.argv[3])
def mesh(r,room):
    _,_,entries=read_mesh_header(r.get(SCENE+f'Z2_26SARUNOMORI_room_{room:02d}'))
    return [t for p in dict.fromkeys(p for _,o,x in entries for p in(o,x) if p) for t in r.mesh(p)]
def sample(t):
    v=t['v'][:,:3]
    return np.vstack([v,(v+np.roll(v,1,axis=0))/2,v.mean(0)])
def fascia(t,center):
    v=t['v'][:,:3]-center
    return any((v[:,axis]==sign*240).all() for axis in[0,2] for sign in[-1,1])
def area(t):
    a,b,c=t['v'][:,:3];return np.linalg.norm(np.cross(b-a,c-a))/2
def inside(points,center,d):
    p=points-center;along=p[:,[0,2]]@d;across=p[:,[0,2]]@[-d[1],d[0]]
    return (along>200.1)&(along<449.9)&(abs(across)<115.9)&(p[:,1]>-3.9)&(p[:,1]<144.9)
def nearest(ts,eyes,rays):
    result=np.full(len(rays),np.inf)
    for t in ts:
        a,b,c=t['v'][:,:3];e1=b-a;e2=c-a;h=np.cross(rays,e2);det=h@e1
        ok=abs(det)>1e-9;inv=np.zeros(len(rays));inv[ok]=1/det[ok];s=eyes-a
        u=np.einsum('ij,ij->i',s,h)*inv;q=np.cross(s,e1);v=np.einsum('ij,ij->i',rays,q)*inv;distance=(q@e2)*inv
        ix=np.where(ok&(u>=-1e-7)&(v>=-1e-7)&(u+v<=1+1e-7)&(distance>1e-6))[0]
        result[ix]=np.minimum(result[ix],distance[ix])
    return result

ray_count=0;removed_witnesses=0;outward_count=0
for room in range(9):
    center=np.array([-880*(room//3),0,880*(1-room%3)])
    before=mesh(previous,room);after=mesh(current,room)
    iswall=lambda t:t['path'].startswith(SCENE) and t['texture'] in {ROOT+'textures/rock',ROOT+'textures/vines'}
    walls=[t for t in after if iswall(t)];old=[t for t in before if iswall(t)]
    a=sum(area(t) for t in old if fascia(t,center));b=sum(area(t) for t in walls if fascia(t,center))
    assert abs(a-b)<=max(1,a*.001),f'Flat native fascia changed in room{room}: {a}->{b}'
    opaque=[t for t in after if t['texture'] in {ROOT+'textures/'+m for m in['grass','rock','vines','black']} or
            (t['texture'] or '').endswith(('/lw_tutu_01_2A000','/lw_wood_02_2CA00'))]
    eyes=[];rays=[];limits=[];open_mouth_views=[]
    for d in map(np.array,[(1,0),(-1,0),(0,1),(0,-1)]):
        for t in walls:
            if not fascia(t,center):
                assert not inside(sample(t),center,d).any(),f'Native wall fin still enters the backed corridor: room{room}, direction{d}, face{t["v"][:,:3]}'
        witnesses=np.concatenate([sample(t)[inside(sample(t),center,d)] for t in old if not fascia(t,center)])
        removed_witnesses+=len(witnesses)
        side=np.array([-d[1],0,d[0]]);axis=np.array([d[0],0,d[1]])
        for point in witnesses:
            for eye in[center+[0,65,0],center+axis*250+side*80+[0,65,0]]:
                ray=point-eye;distance=np.linalg.norm(ray)
                if distance<1e-6:continue
                ray/=distance;local_eye=eye-center;along=local_eye@axis;across=local_eye@side
                speed=ray@axis;near=(200-along)/speed if speed<-1e-8 else -1
                exit_point=local_eye+near*ray
                # Looking back out of a tunnel's open mouth may naturally see
                # the far side of the clearing. This is not an opaque seam.
                outward=(200<=along<=450 and abs(across)<=116 and -4<=local_eye[1]<=145 and near>0
                         and abs(exit_point@side)<=116+1e-7 and -4<=exit_point[1]<=145)
                eyes.append(eye);rays.append(ray);limits.append(distance+100);open_mouth_views.append(outward)
    if eyes:
        distances=nearest(opaque,np.array(eyes),np.array(rays))
        bad=np.where(~np.isfinite(distances)|((distances>limits)&~np.array(open_mouth_views)))[0]
        examples=[dict(eye=eyes[i].tolist(),target=(eyes[i]+rays[i]*(limits[i]-100)).tolist(),distance=float(distances[i]),limit=float(limits[i])) for i in bad[:6]]
        assert not len(bad),f'Wall trim exposes a sky gap or distant cap in room{room}: {len(bad)} witnesses, {examples}'
        ray_count+=len(eyes)
        outward_count+=sum(open_mouth_views)
assert removed_witnesses>100
print(f'PASS: curved wall fins removed in all9rooms; flat fascia retained; {ray_count} removed-surface rays keep coverage ({outward_count} look out through the open mouth); no sky misses')
