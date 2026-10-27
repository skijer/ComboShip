"""Offline contact measurements against closed prop triangle meshes."""
import numpy as np


def clearance(points, triangles):
    """Signed nearest-surface distance; negative points penetrate the prop.

    Solid-angle winding tests the closed mesh, including concave regions.
    Distance uses triangle interiors and edges rather than only vertices.
    """
    points=np.asarray(points,float);tri=np.asarray(triangles,float)
    if not len(points):return np.empty(0)
    a,b,c=tri[:,0],tri[:,1],tri[:,2]
    ab,ac=b-a,c-a
    cross=np.cross(ab,ac)
    normal=cross/np.maximum(np.linalg.norm(cross,axis=1,keepdims=True),1e-15)
    delta=points[:,None,:]-a
    height=np.einsum('nfi,fi->nf',delta,normal)
    projected=delta-height[...,None]*normal
    aa=np.einsum('fi,fi->f',ab,ab);bb=np.einsum('fi,fi->f',ab,ac);cc=np.einsum('fi,fi->f',ac,ac)
    d=np.einsum('nfi,fi->nf',projected,ab);e=np.einsum('nfi,fi->nf',projected,ac)
    det=aa*cc-bb*bb
    safe=np.where(abs(det)>1e-15,det,1.)
    u=(cc*d-bb*e)/safe;v=(aa*e-bb*d)/safe
    interior=(abs(det)>1e-15)&(u>=0)&(v>=0)&(u+v<=1)
    squared=np.where(interior,height*height,np.inf)
    for start,end in ((a,b),(b,c),(c,a)):
        edge=end-start
        t=np.einsum('nfi,fi->nf',points[:,None,:]-start,edge)/np.maximum(np.einsum('fi,fi->f',edge,edge),1e-15)
        nearest=start+np.clip(t,0,1)[...,None]*edge
        squared=np.minimum(squared,np.sum((points[:,None,:]-nearest)**2,axis=-1))
    distance=np.sqrt(squared.min(1))
    va,vb,vc=[tri[None,:,i,:]-points[:,None,:] for i in range(3)]
    la,lb,lc=[np.linalg.norm(x,axis=-1) for x in (va,vb,vc)]
    numerator=np.einsum('nfi,nfi->nf',va,np.cross(vb,vc))
    denominator=(la*lb*lc+np.einsum('nfi,nfi->nf',va,vb)*lc+
                 np.einsum('nfi,nfi->nf',vb,vc)*la+np.einsum('nfi,nfi->nf',vc,va)*lb)
    inside=abs((2*np.arctan2(numerator,denominator)).sum(1))>2*np.pi
    return np.where(inside,-distance,distance)


def surface_crossings(first, second):
    """Indices of first-mesh triangles cut by second-mesh triangle edges.

    Check edges in both directions, including coplanar overlap. The repair
    also requires positive sampled surface clearance from the prop.
    """
    first=np.asarray(first,float);second=np.asarray(second,float)
    def edges_hit(a,b):
        hits=np.zeros((len(a),len(b)),bool)
        e1=b[:,1]-b[:,0];e2=b[:,2]-b[:,0]
        for start,end in ((0,1),(1,2),(2,0)):
            direction=a[:,end]-a[:,start]
            h=np.cross(direction[:,None,:],e2)
            det=np.einsum('bri,ri->br',h,e1)
            valid=abs(det)>1e-9;inverse=np.where(valid,1/np.where(valid,det,1),0)
            s=a[:,None,start,:]-b[:,0]
            u=np.einsum('bri,bri->br',s,h)*inverse
            q=np.cross(s,e1)
            v=np.einsum('bi,bri->br',direction,q)*inverse
            t=np.einsum('ri,bri->br',e2,q)*inverse
            hits |= valid&(u>=-1e-8)&(v>=-1e-8)&(u+v<=1+1e-8)&(t>1e-8)&(t<1-1e-8)
        return hits
    hit=edges_hit(first,second)|edges_hit(second,first).T
    na=np.cross(first[:,1]-first[:,0],first[:,2]-first[:,0])
    nb=np.cross(second[:,1]-second[:,0],second[:,2]-second[:,0])
    la=np.linalg.norm(na,axis=1);lb=np.linalg.norm(nb,axis=1)
    ua=na/np.maximum(la[:,None],1e-15);ub=nb/np.maximum(lb[:,None],1e-15)
    parallel=abs(ua@ub.T)>1-1e-10
    distance=abs(np.einsum('afi,ai->af',second[None,:,0]-first[:,None,0],ua))
    overlap=(np.maximum(first.min(1)[:,None],second.min(1))<=
             np.minimum(first.max(1)[:,None],second.max(1))+1e-7).all(2)
    candidates=np.argwhere(parallel&(distance<1e-7)&overlap&(la[:,None]>1e-9)&(lb>1e-9))
    def cross(a,b):return a[0]*b[1]-a[1]*b[0]
    def inside(p,t):
        signs=[cross(t[(i+1)%3]-t[i],p-t[i]) for i in range(3)]
        return min(signs)>=-1e-7 or max(signs)<=1e-7
    for i,j in candidates:
        axes=[k for k in range(3) if k!=int(np.argmax(abs(ua[i])))]
        a,b=first[i][:,axes],second[j][:,axes]
        intersects=any(inside(p,b) for p in a) or any(inside(p,a) for p in b)
        if not intersects:
            for k in range(3):
                p,q=a[k],a[(k+1)%3]
                for l in range(3):
                    r,s=b[l],b[(l+1)%3]
                    bound=(np.maximum(np.minimum(p,q),np.minimum(r,s))<=
                           np.minimum(np.maximum(p,q),np.maximum(r,s))+1e-7).all()
                    intersects |= bool(bound and cross(q-p,r-p)*cross(q-p,s-p)<=1e-7 and
                                       cross(s-r,p-r)*cross(s-r,q-r)<=1e-7)
        hit[i,j] |= intersects
    return np.flatnonzero(hit.any(1))


def verify_archive(native, candidate):
    """Read quantized hand meshes and test every frame in the ocarina family."""
    from native_npc_models import NativeRig,animation_frames
    from verify_native_npc_models import Archive
    archive=Archive(native,candidate);rig=NativeRig(archive,'saria',private=True)
    prop_path=rig.ns+'/Props/OcarinaGrip'
    if 'alt/'+prop_path not in archive.candidate.namelist():prop_path=rig.ns+'/Props/Ocarina'
    prop=archive.geometry(prop_path,rig,14)['local'].reshape(-1,3,3)
    result={}
    for name in ('gSariaPlayingOcarinaAnim','gSariaOcarinaToMouthAnim',
                 'gSariaStopPlayingOcarinaAnim','gSariaReturnToOcarinaAnim'):
        private=rig.ns+'/PoseAnimations/objects/object_sa/'+name
        frames=animation_frames(archive.read(private));report={}
        for side in ('left','right'):
            hand=rig.profile[side][-1]
            path=private+'/Skin/Hands/'+side.title()
            data=archive.geometry(path,rig,hand)
            # Repeated cache vertices must not change the measured minima.
            minimum=np.inf;inside=0;crossings=0;worst=0
            for index,frame in enumerate(frames):
                rig.pose(frame);matrices=archive.matrices(rig)
                p=np.zeros_like(data['local'])
                inverse=np.linalg.inv(matrices[14])
                for bone in np.unique(data['bones']):
                    mask=data['bones']==bone;m=inverse@matrices[bone]
                    p[mask]=data['local'][mask]@m[:3,:3].T+m[:3,3]
                triangles=p.reshape(-1,3,3)
                # Vertex, edge midpoint and centroid samples catch enclosure
                # as well as cuts detected by the exact segment/triangle test.
                samples=np.concatenate([p,triangles.mean(1),
                    (triangles[:,0]+triangles[:,1])/2,(triangles[:,1]+triangles[:,2])/2,
                    (triangles[:,2]+triangles[:,0])/2])
                d=clearance(np.unique(np.round(samples,7),axis=0),prop)
                if d.min()<minimum:minimum=float(d.min());worst=index
                inside=max(inside,int((d<-.05).sum()))
                crossings=max(crossings,len(surface_crossings(triangles,prop)))
            report[side]=dict(min_clearance=round(minimum,4),max_inside_samples=inside,
                              max_crossing_triangles=crossings,worst_frame=worst,frames=len(frames))
        result[name]=report
    archive.native.close();archive.candidate.close()
    return result


if __name__=='__main__':
    import argparse,json
    from pathlib import Path
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('native');parser.add_argument('candidate')
    parser.add_argument('--output',type=Path)
    args=parser.parse_args();report=verify_archive(args.native,args.candidate)
    result=json.dumps(report,indent=2)
    if args.output:args.output.write_text(result+'\n')
    print(result)
    failed=any(side['min_clearance']<=.5 or side['max_inside_samples'] or side['max_crossing_triangles']
               for clip in report.values() for side in clip.values())
    raise SystemExit(1 if failed else 0)
