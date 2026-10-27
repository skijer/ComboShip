"""Sample texture alpha above actual wall tops, including near-wall views.

This CPU coverage check uses wrapped bilinear alpha and the native texture-edge
cutoff. It does not replace renderer verification of filtering, fog or culling.
"""
from pathlib import Path
import collections
import sys
import numpy as np

sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'tools/woods_mystery'))
from build_candidate import Resources, ROOT, SCENE, WALL, read_mesh_header

candidate=Resources(sys.argv[2],sys.argv[1]);native=Resources(sys.argv[2])

def mesh(resources,room):
    _,_,entries=read_mesh_header(resources.get(SCENE+f'Z2_26SARUNOMORI_room_{room:02d}'))
    return [t for p in dict.fromkeys(p for _,o,x in entries for p in (o,x) if p) for t in resources.mesh(p)]

def intersections(triangles,eyes,rays):
    for t in triangles:
        p,q,r=t['v'][:,:3];e1=q-p;e2=r-p;h=np.cross(rays,e2)
        det=h@e1;valid=abs(det)>1e-9;inv=np.zeros(len(rays));inv[valid]=1/det[valid]
        s=eyes-p;u=np.einsum('ij,ij->i',h,s)*inv;c=np.cross(s,e1)
        v=np.einsum('ij,ij->i',rays,c)*inv;distance=(c@e2)*inv
        ix=np.where(valid&(u>=-1e-7)&(v>=-1e-7)&(u+v<=1+1e-7)&(distance>1e-6))[0]
        if len(ix):yield t,ix,u[ix],v[ix]

def alpha(image,tile,uv,half_texel):
    h,w=image.shape;xy=(uv/32+half_texel)/np.array(tile)*[w,h]-.5
    lo=np.floor(xy).astype(int);f=xy-lo;x=lo[:,0]%w;y=lo[:,1]%h
    return ((1-f[:,0])*(1-f[:,1])*image[y,x]+f[:,0]*(1-f[:,1])*image[y,(x+1)%w]
            +(1-f[:,0])*f[:,1]*image[(y+1)%h,x]+f[:,0]*f[:,1]*image[(y+1)%h,(x+1)%w])

total=0
for room in range(9):
    before=mesh(native,room);after=mesh(candidate,room)
    walls=[t for t in before if t['texture']==WALL]
    edges=collections.Counter(tuple(sorted((tuple(v[:3]),tuple(t['v'][(i+1)%3,:3]))))
                              for t in walls for i,v in enumerate(t['v']))
    tops=[e for e,n in edges.items() if n==1 and min(v[1] for v in e)>=120]
    assert len(tops)==24,(room,len(tops))
    center=np.array([-880*(room//3),0,880*(1-room%3)])
    eyes=[];rays=[]
    for aa,bb in tops:
        for f in np.linspace(0,1,9):
            point=np.array(aa)*(1-f)+np.array(bb)*f
            for up in (1,8,20):
                for near in (0,.65):
                    eye=center+(point-center)*[near,0,near]+[0,65,0]
                    ray=point+[0,up,0]-eye;ray/=np.linalg.norm(ray)
                    eyes.append(eye);rays.append(ray)
    eyes=np.array(eyes);rays=np.array(rays)
    foliage=[t for t in after if (t['texture'] or '').startswith(ROOT+'textures/henriko_')]
    opaque={ROOT+'textures/'+m for m in ('rock','vines','grass','black')}
    shell=[t for t in after if t['texture'] in opaque or t['texture'] is None or
           (t['texture'] or '').endswith(('/lw_tutu_01_2A000','/lw_wood_02_2CA00'))]
    blocked=np.zeros(len(rays),bool)
    for t,ix,u,v in intersections(shell,eyes,rays):blocked[ix]=True
    hits=list(intersections(foliage,eyes,rays))
    images={name:np.asarray(candidate.t(name))[:,:,3]/255 for name in {t['texture'] for t in foliage}}
    # Standard fog supplies SHADE alpha1 in the native interpreter. Also test
    # the unshifted texture convention used by some inspection renderers.
    for half_texel in (0,.5):
        covered=blocked.copy()
        for t,ix,u,v in hits:
            uv=t['v'][0,3:5]*(1-u-v)[:,None]+t['v'][1,3:5]*u[:,None]+t['v'][2,3:5]*v[:,None]
            covered[ix[alpha(images[t['texture']],t['tile'],uv,half_texel)>.19]]=True
        missing=np.where(~covered)[0]
        assert not len(missing),f'Floating wall/canopy gap: room{room}, half_texel{half_texel}, {len(missing)} sky rays; first eye{eyes[missing[0]]}, ray{rays[missing[0]]}'
        total+=len(rays)
print(f'PASS: {total} alpha-aware walltop rays across all9 rooms; original walltop edges, center/near-wall eyes and both texture conventions')
