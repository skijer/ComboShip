"""Export editable GLBs and render the actual candidate triangles offline."""
import io,json,math,struct
import numpy as np
from PIL import Image,ImageDraw,ImageFont
from meshkit import ROOT,Q,unit,rotation

def glb(m,path):
    blob=bytearray();views=[];accessors=[];images=[];textures=[];materials=[]
    samplers=[dict(magFilter=9729,minFilter=9729,wrapS=10497,wrapT=10497)]
    def view(data,target=None):
        blob.extend(b'\0'*((-len(blob))%4));v=dict(buffer=0,byteOffset=len(blob),byteLength=len(data))
        if target:v['target']=target
        views.append(v);blob.extend(data);return len(views)-1
    def acc(a,typ,ct):
        i=view(a.tobytes(),34963 if typ=='SCALAR' else 34962)
        v=dict(bufferView=i,componentType=ct,count=len(a),type=typ)
        if typ=='VEC3':v.update(min=a.min(axis=0).tolist(),max=a.max(axis=0).tolist())
        accessors.append(v);return len(accessors)-1
    names=list(m.materials)
    for name,mat in m.materials.items():
        pbr=dict(baseColorFactor=mat['color']+[mat['alpha']],metallicFactor=mat['metal'],roughnessFactor=mat['rough'])
        if mat['tex'] is not None:
            buf=io.BytesIO();Image.fromarray(mat['tex']).save(buf,format='PNG')
            images.append(dict(bufferView=view(buf.getvalue()),mimeType='image/png'))
            sampler=0
            address=mat.get('sampler')
            if mat.get('reflection'):
                mode='wrap' if mat.get('texgen',{}).get('wrap') else 'clamp'
                address={'s':mode,'t':mode}
            if address:
                mode={'wrap':10497,'mirror':33648,'clamp':33071,'mirror_clamp':33071}
                samplers.append(dict(magFilter=9729,minFilter=9729,wrapS=mode[address['s']],wrapT=mode[address['t']]))
                sampler=len(samplers)-1
            textures.append(dict(source=len(images)-1,sampler=sampler));pbr['baseColorTexture']=dict(index=len(textures)-1)
        item=dict(name=name,pbrMetallicRoughness=pbr,doubleSided=mat.get('double_sided',False))
        if mat.get('reflection'):item['extras']={'neiTextureGen':'spherical','strength':mat['reflection'],**mat.get('texgen',{})}
        if mat.get('reflection'):item['extras']['neiTextureTile']=list(mat.get('tile',(32,32)))
        if mat.get('decal'):item.setdefault('extras',{})['neiDecal']=True
        if mat.get('sampler'):item.setdefault('extras',{})['neiSamplerModes']=mat['sampler']
        if mat['alpha']<1:item['alphaMode']='BLEND'
        if mat['emission']:
            item['emissiveFactor']=[mat['emission']]*3
            if 'baseColorTexture' in pbr:item['emissiveTexture']=pbr['baseColorTexture']
        materials.append(item)
    meshes=[];nodes=[]
    for p in m.parts:
        primitive=dict(attributes=dict(POSITION=acc((p['p']*Q).astype('<f4'),'VEC3',5126),NORMAL=acc(p['n'].astype('<f4'),'VEC3',5126),TEXCOORD_0=acc(p['uv'].astype('<f4'),'VEC2',5126)),indices=acc(p['tri'].ravel().astype('<u4'),'SCALAR',5125),material=names.index(p['mat']))
        if 'native_normal_bytes' in p:
            raw=p['native_normal_bytes'];native=np.where(raw>=128,raw-256,raw)/127
            primitive['attributes']['_NEI_NATIVE_NORMAL']=acc(native.astype('<f4'),'VEC3',5126)
        meshes.append(dict(name=p['name'],primitives=[primitive]));nodes.append(dict(name=p['name'],mesh=len(meshes)-1))
    root=dict(name=m.name,children=list(range(len(nodes))),scale=[m.native_scale]*3)
    nodes.append(root)
    doc=dict(asset=dict(version='2.0',generator='NEI unified GI upgrade / exact exported geometry'),buffers=[dict(byteLength=len(blob))],bufferViews=views,accessors=accessors,materials=materials,images=images,textures=textures,samplers=samplers,meshes=meshes,nodes=nodes,scenes=[dict(nodes=[len(nodes)-1])],scene=0)
    js=json.dumps(doc,separators=(',',':')).encode();js+=b' '*((-len(js))%4);blob+=b'\0'*((-len(blob))%4)
    body=struct.pack('<I4s',len(js),b'JSON')+js+struct.pack('<I4s',len(blob),b'BIN\0')+blob
    path.write_bytes(struct.pack('<4sII',b'glTF',2,len(body)+12)+body)

def render(m,angle=25,size=480,elevation=12):
    rot=rotation('x',elevation)@rotation('y',angle)
    allp=np.concatenate([p['p'] for p in m.parts]);center=(allp.min(axis=0)+allp.max(axis=0))/2
    # Fixed per-model fit for all turntable angles.
    radius=max(np.linalg.norm((allp-center)[:,[0,2]],axis=1).max(),np.ptp(allp[:,1])/2)
    scale=size/(radius*2.75)
    pix=np.zeros((size,size,3),float);yy=np.linspace(0,1,size)[:,None,None]
    pix[:]=np.array([22,27,34])+(1-yy)*np.array([9,10,12])
    zb=np.full((size,size),-1e12);light=unit([-.45,.70,.62])
    parts=sorted(m.parts,key=lambda p:m.materials[p['mat']]['alpha']<1)
    for part in parts:
        pos=(part['p']-center)@rot.T
        raw=part.get('native_normal_bytes')
        norm=(np.where(raw>=128,raw-256,raw)/127 if raw is not None else part['n'])@rot.T
        screen=np.c_[size*.5+pos[:,0]*scale,size*.52-pos[:,1]*scale]
        mat=m.materials[part['mat']];tex=mat['tex']
        for face in part['tri']:
            if not mat.get('double_sided') and np.cross(pos[face[1]]-pos[face[0]],pos[face[2]]-pos[face[0]])[2]<=0:continue
            v=screen[face];z=pos[face,2];nn=norm[face];x0,y0=v[0];x1,y1=v[1];x2,y2=v[2]
            area=(x1-x0)*(y2-y0)-(x2-x0)*(y1-y0)
            if abs(area)<1e-8:continue
            xmin=max(0,int(np.floor(v[:,0].min())));xmax=min(size-1,int(np.ceil(v[:,0].max())))
            ymin=max(0,int(np.floor(v[:,1].min())));ymax=min(size-1,int(np.ceil(v[:,1].max())))
            if xmax<xmin or ymax<ymin:continue
            xx,yy=np.meshgrid(np.arange(xmin,xmax+1)+.5,np.arange(ymin,ymax+1)+.5)
            w0=((x1-xx)*(y2-yy)-(x2-xx)*(y1-yy))/area;w1=((x2-xx)*(y0-yy)-(x0-xx)*(y2-yy))/area;w2=1-w0-w1
            depth=w0*z[0]+w1*z[1]+w2*z[2];tz=zb[ymin:ymax+1,xmin:xmax+1]
            visible=(depth>=tz-1e-4 if mat.get('decal') else depth>tz)
            mask=(w0>=0)&(w1>=0)&(w2>=0)&visible
            if not mask.any():continue
            weights=np.stack([w0[mask],w1[mask],w2[mask]],axis=1)
            normals=weights@nn if raw is not None else unit(weights@nn)
            diff=np.maximum(normals@light,0);shade=.32+.68*diff
            texalpha=np.ones(len(weights))
            if tex is None:base=np.broadcast_to(np.array(mat['color'])*255,(len(weights),3)).copy()
            else:
                if mat.get('reflection'):
                    gen=mat.get('texgen',{})
                    mapped=(np.arccos(-np.clip(normals[:,:2],-1,1))/(2*np.pi)
                            if gen.get('linear') else (normals[:,:2]+1)*.25)
                    shift=np.asarray(gen.get('shift',(0,0)))
                    divisor=np.where(shift<=10,2.**shift,2.**(shift-16))
                    uv=mapped*np.asarray(gen.get('scale',(1984,1984)))/32/np.asarray(mat.get('tile',(32,32)))/divisor
                else:uv=weights@part['uv'][face]
                h,w=tex.shape[:2]
                uv=uv.copy();address=mat.get('sampler') or {'s':'wrap','t':'wrap'}
                if mat.get('reflection') and not mat.get('texgen',{}).get('wrap'):
                    address={'s':'clamp','t':'clamp'}
                # Native mirror+clamp first constrains the rendered tile; its
                # mirror is inactive within the first 0..1 tile UV interval.
                address={axis:('clamp' if mode=='mirror_clamp' else mode) for axis,mode in address.items()}
                for axis,mode in enumerate((address['s'],address['t'])):
                    if mode=='mirror':uv[:,axis]=1-np.abs(np.mod(uv[:,axis],2)-1)
                    elif mode=='clamp':uv[:,axis]=np.clip(uv[:,axis],0,1)
                tx=np.mod(uv[:,0]*w-.5,w);ty=np.mod(uv[:,1]*h-.5,h)
                if address['s'] in ('clamp','mirror'):tx=np.clip(uv[:,0]*w-.5,0,w-1)
                if address['t'] in ('clamp','mirror'):ty=np.clip(uv[:,1]*h-.5,0,h-1)
                ix=np.floor(tx).astype(int);iy=np.floor(ty).astype(int);fx=(tx-ix)[:,None];fy=(ty-iy)[:,None]
                ix1=np.minimum(ix+1,w-1) if address['s'] in ('clamp','mirror') else (ix+1)%w
                iy1=np.minimum(iy+1,h-1) if address['t'] in ('clamp','mirror') else (iy+1)%h
                sampled=(tex[iy,ix]*(1-fx)+tex[iy,ix1]*fx)*(1-fy)+(tex[iy1,ix]*(1-fx)+tex[iy1,ix1]*fx)*fy
                base=sampled[:,:3];texalpha=sampled[:,3]/255
            if mat['emission']:shade=shade*(1-mat['emission'])+mat['emission']
            half=unit(light+[0,0,1]);spec=np.maximum(normals@half,0)**(8+mat['rough']*40)*(0 if mat.get('decal') else (32 if mat['metal'] else 9))
            colors=base*shade[:,None]+spec[:,None];alpha=(texalpha*mat['alpha'])[:,None];target=pix[ymin:ymax+1,xmin:xmax+1]
            target[mask]=colors*alpha+target[mask]*(1-alpha)
            if mat['alpha']>=1:tz[mask]=depth[mask]
    return Image.fromarray(np.uint8(np.clip(pix,0,255)))

def checkpoint(m,stats):
    d=ROOT/'CHECKPOINTS'/m.slug;d.mkdir(parents=True,exist_ok=True)
    glb(m,d/(m.slug+'.glb'))
    positions=np.concatenate([p['p'] for p in m.parts]);stats.update(slug=m.slug,name=m.name,entry=m.entry,draw_scale=m.draw_scale,matrix_scale=m.native_scale,triangles=sum(len(p['tri']) for p in m.parts),bounds_author=[positions.min(axis=0).tolist(),positions.max(axis=0).tolist()],markers=m.markers,notes=m.notes,runtime_tested=False)
    (d/'checkpoint.json').write_text(json.dumps(stats,indent=2)+'\n')
    render(m,m.markers.get('preview_front_angle',25)).save(d/'front.png')
    render(m,m.markers.get('preview_back_angle',150)).save(d/'back.png')
    print(json.dumps(dict(checkpoint=m.slug,triangles=stats['triangles'],loads=stats['vertex_loads'])),flush=True)
