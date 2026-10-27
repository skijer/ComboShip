#!/usr/bin/env python3
"""Offline original-room preview using external inputs; creates no classic asset pack."""
from pathlib import Path
import argparse,hashlib,json,struct,subprocess,sys,zipfile
import xml.etree.ElementTree as E
import numpy as np

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tests/black_market'))
from check_granny_donors import check
import preview
from native_preview import NativeKeeper
from build import crc64

PREFIX='scenes/shared/mahouya_scene/'
VERTEX=PREFIX+'mahouya_room_0DL_006548_vtx_2'
DRAW=PREFIX+'mahouya_room_0DL_006548_tri_2'

class ExternalRoom:
    def __init__(self,resources,rows):
        self.resources=resources;self.rows=rows
    def read(self,path):
        if path=='custom/skiejer_market/room':path=PREFIX+'mahouya_room_0DL_006548'
        if path==VERTEX:
            root=E.Element('Vertex',Version='0')
            for row in self.rows:
                values=list(row[:3])+list(row[4:])
                E.SubElement(root,'Vtx',dict(zip(('X','Y','Z','S','T','R','G','B','A'),map(str,values))))
            return E.tostring(root)
        data=self.resources[path if path.startswith('alt/') else 'alt/'+path]
        if path==DRAW:
            root=E.fromstring(data);end=root[-1];root.remove(end)
            for offset,faces in ((1189,((0,2,1),(0,3,2))),(1193,((0,1,2),(0,2,3)))):
                E.SubElement(root,'LoadVertices',Path=VERTEX,VertexBufferIndex='0',VertexOffset=str(offset),Count='4')
                E.SubElement(root,'Triangles2',dict(zip(('V00','V01','V02','V10','V11','V12'),map(str,sum(faces,())))),Flag0='0',Flag1='0')
            root.append(end);return E.tostring(root)
        return data

def native_texture(pack,name):
    data=pack.read(name);version=struct.unpack_from('<I',data,8)[0]
    kind,width,height=struct.unpack_from('<III',data,64)
    if version==1:
        flags,hs,vs,size=struct.unpack_from('<IffI',data,76);pixels=data[92:]
        if flags&1:return np.frombuffer(pixels,dtype=np.uint8).reshape(height,width,4),width,height
    else:pixels=data[80:]
    values=np.frombuffer(pixels,dtype=np.uint8)
    if kind==2:
        v=np.frombuffer(pixels,dtype='>u2').astype(np.uint16)
        rgba=np.column_stack((((v>>11)&31)*255//31,((v>>6)&31)*255//31,((v>>1)&31)*255//31,(v&1)*255))
    elif kind==1:rgba=values.reshape(-1,4)
    elif kind==8:
        intensity=(values>>4)*17;rgba=np.column_stack((intensity,intensity,intensity,(values&15)*17))
    elif kind==9:
        v=values.reshape(-1,2);rgba=np.column_stack((v[:,0],v[:,0],v[:,0],v[:,1]))
    elif kind in (5,7):
        v=np.column_stack((values>>4,values&15)).reshape(-1)
        intensity=(v>>1)*255//7 if kind==7 else v*17
        alpha=(v&1)*255 if kind==7 else intensity
        rgba=np.column_stack((intensity,intensity,intensity,alpha))
    elif kind==6:rgba=np.column_stack((values,values,values,values))
    else:raise ValueError(('Unsupported external texture',name,kind,version))
    return rgba.astype(np.uint8).reshape(height,width,4),width,height

def decorative_flame(pack,*,tick,position,scale,prim,view):
    """EnLight's actual I8/I4 combiner; raster filtering remains an offline approximation."""
    assert struct.pack('<II',0xfc272c60,0x350ce37f) in pack.read('objects/gameplay_keep/gEffFire1DL')
    fire,_,_=native_texture(pack,'objects/gameplay_keep/gDecorativeFlameTex')
    mask,_,_=native_texture(pack,'objects/gameplay_keep/gDecorativeFlameMaskTex')
    # Native tile 0 clamps at 32x64. Tile 1 wraps 32x128 and scrolls
    # -20 quarter-texels/frame. Both sample the original 0..32/0..64 UVs.
    y,x=np.mgrid[:256,:128];s=(x+.5)/4;t=(y+.5)/4
    tex0=fire[np.minimum(t.astype(int),63),np.minimum(s.astype(int),31)]/255
    tex1=mask[(t.astype(int)+tick*5)%128,s.astype(int)%32]/255
    prim=np.array([1,200/255,0,1]) if prim is None else np.asarray(prim)
    env=np.array([1,0,0])
    combined=(tex1[:,:,:3]-prim[:3])*(128/255)+tex0[:,:,:3]
    rgb=(prim[:3]-env)*combined+env
    alpha=np.clip(tex0[:,:,3]+(tex1[:,:,3]-1)*(128/255),0,1)*prim[3]
    pixels=np.rint(np.clip(np.dstack((rgb,alpha)),0,1)*255).astype(np.uint8)
    texture=preview.presentation.upload(preview.gpu,pixels,128,256,wrap_s=0x812f,wrap_t=0x812f)
    decoder=NativeKeeper.__new__(NativeKeeper);decoder.archive=pack;decoder.vertices={}
    vertices=decoder.vertex_array('objects/gameplay_keep/gameplay_keepVtx_0529D0')
    points=np.array([np.r_[v['xyz'],v['uv'],[1,1,1,1]] for v in vertices[[0,1,2,0,3,1]]],dtype=np.float32)
    points[:,3:5]/=[32*32,64*32]
    # Native EnLight billboards to Camera_GetCamDirYaw; params 0x7F5
    # also flips the plane by pi after the normal facing-camera rotation.
    view=preview.WIDE_VIEW if view is None else view
    eye,target=np.array(view[0]),np.array(view[1]);delta=target-eye
    yaw=np.arctan2(delta[0],delta[2]);c,s=np.cos(yaw),np.sin(yaw)
    points[:,:3]=points[:,:3]@np.array([[c,0,-s],[0,1,0],[s,0,c]])*scale+np.array(position)
    return [(texture,np.ascontiguousarray(points))]

def native_effect(pack,root,*,tick=0,position=None,scale=1,prim=None,view=None):
    """Native triangle/texture inspection; layered effect combiner is approximate."""
    if root=='objects/gameplay_keep/gEffFire1DL':
        return decorative_flame(pack,tick=tick,position=position,scale=scale,prim=prim,view=view)
    decoder=NativeKeeper.__new__(NativeKeeper);decoder.archive=pack
    decoder.hashes={crc64(n):n for n in pack.namelist()};decoder.vertices={};decoder.display_lists={}
    cache={};draws=[];texture=None;size=[32,32];color=np.ones(4)
    def visit(name):
        nonlocal texture,size,color
        for op,w0,w1,resource in decoder.commands(name):
            if op==0x31:visit(resource)
            elif op==0x20:texture=resource
            elif op==0xfa:color=np.array([(w1>>s)&255 for s in (24,16,8,0)])/255
            elif op==0xf2 and (w1>>24)&7==0:size=[((w1>>12)&4095)//4+1,(w1&4095)//4+1]
            elif op==0x32:
                count=(w0>>12)&255;start=((w0>>1)&127)-count
                for i,v in enumerate(decoder.vertex_array(resource)[w1//16:w1//16+count]):
                    cache[start+i]=np.r_[v['xyz'],v['uv'],v['rgba']].astype(np.float32)
            elif op in (5,6):
                indices=[((w0>>s)&255)//2 for s in (16,8,0)]
                if op==6:indices += [((w1>>s)&255)//2 for s in (16,8,0)]
                points=np.array([cache[i] for i in indices],dtype=np.float32)
                points[:,3:5]/=np.array(size)*32
                points[:,5:9]/=255;points[:,5:9]*=color if prim is None else prim
                if position is not None:points[:,:3]=points[:,:3]*scale+np.array(position)
                if texture:draws.append((texture,points))
    visit(root)
    textures={}
    for name in {name for name,_ in draws}:
        pixels,width,height=native_texture(pack,name)
        textures[name]=preview.presentation.upload(preview.gpu,pixels,width,height)
    return [(textures[name],np.ascontiguousarray(points)) for name,points in draws]

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('maps','textures','stormlib','native-oot','market','stock','output'):p.add_argument('--'+name,type=Path,required=True)
    p.add_argument('--browsing',action='store_true',help='Also render production native camera poses and a short pan clip')
    a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
    resources,counter,proof=check(a.maps,a.textures,a.stormlib,a.output/'donor-preservation.json')
    rows,textures=preview.streams(ExternalRoom(resources,counter),native_colors=True)
    stock=json.loads(a.stock.read_text())['stock'];shelves=preview.presentation.shelf_positions()
    with zipfile.ZipFile(a.market) as market,zipfile.ZipFile(a.native_oot) as native:
        keeper=NativeKeeper(market);placement=preview.presentation.keeper_placement(market)
        for name in market.namelist():
            if '/keeper/' in name and 'Texture' in name:
                pixels,w,h=native_texture(market,name);textures[name]=preview.presentation.upload(preview.gpu,pixels,w,h)
        models={item['slug']:preview.presentation.checkpoint(preview.gpu,item['slug'])[0] for item in stock}
        lock=preview.presentation.native_lock().texture();lock_texture=preview.presentation.upload(preview.gpu,lock,lock.shape[1],lock.shape[0],wrap_t=0x812f)
        smoke=native_effect(native,PREFIX+'mahouya_room_0DL_006A78')
        def flame_draws(view,tick=0):
            return [draw for x in (130,-139) for draw in native_effect(native,'objects/gameplay_keep/gEffFire1DL',
                position=(x,76,-8),scale=.0075,tick=tick,view=view)]
        flames=flame_draws(preview.WIDE_VIEW)
        options=dict(keeper=keeper,placement=placement,stock=stock,shelves=shelves,models=models,lock_texture=lock_texture,room_xlu=smoke+flames,
            granny_shelves=True,shelf_textures=preview.presentation.granny_shelf_textures(preview.gpu,resources))
        for name,clears in [('regular-interior.png',3),('regular-with-seals.png',0)]:
            preview.render(rows,textures,0,None,**options,clears=clears,view=preview.WIDE_VIEW,fov=preview.WIDE_FOV).save(a.output/name)
        preview.render(rows,textures,0,None,**options,clears=3,view=preview.OVERVIEW_VIEW,
            fov=preview.OVERVIEW_FOV).save(a.output/'regular-counter.png')
        if a.browsing:
            sys.path.insert(0,str(ROOT/'tests/black_market'))
            from check_browsing_frame import camera_views
            from PIL import Image,ImageDraw,ImageFont
            cameras=camera_views()
            (a.output/'production-cameras.json').write_text(json.dumps(cameras,indent=2)+'\n')
            cursor,_,_=native_texture(native,'objects/gameplay_keep/gSelectionCursorTex')
            cursor[:,:,:3]=(cursor[:,:,:3]*np.array([0,1,80/255])).astype(np.uint8)
            cursor[:,:,3]=(cursor[:,:,3]/255*220).astype(np.uint8)
            cursor=np.concatenate((cursor,cursor[:,::-1]),axis=1)
            cursor=np.concatenate((cursor,cursor[::-1]),axis=0)
            cursor=Image.fromarray(cursor,'RGBA').resize((round(40*preview.gpu.W/320),round(40*preview.gpu.H/240)))
            def frame(camera,tick=0,selected=None):
                view=(camera['eye'],camera['at'])
                options['room_xlu']=smoke+flame_draws(view,tick)
                image=preview.render(rows,textures,tick,None,**options,clears=3,view=view,
                    fov=camera['fov'],selected=selected)
                if selected is not None:
                    x,y=preview.project(shelves[selected],view,camera['fov'])
                    image.paste(cursor,(round(x-cursor.width/2),round(y-cursor.height/2)),cursor)
                draw=ImageDraw.Draw(image)
                font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',23)
                # Reserve the real native bottom-dialogue region. This overlay
                # labels an asset preview; it is not an executable UI capture.
                top=round(142/240*image.height)
                left=round(34/320*image.width);right=round(286/320*image.width)
                draw.rounded_rectangle((left,top,right,round(206/240*image.height)),radius=18,fill='#17131f',outline='#665674',width=2)
                label='Keeper view · Left / Right to browse' if selected is None else stock[selected]['label']+' · A: Select   B: Back'
                draw.text((left+25,top+25),label,font=font,fill='#f4e9ff')
                draw.text((left+25,top+68),'Offline camera preview · seals hidden',font=font,fill='#c5b4ce')
                font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',18)
                draw.text((28,round(218/240*image.height)),'Production camera path · native flame blend · room smoke/lighting approximate',font=font,fill='#d3c6df',stroke_width=2,stroke_fill='#100c17')
                return image
            for name,slot in [('center',None),('right',0),('left',4)]:
                frame(cameras[name],selected=slot).save(a.output/('browsing-'+name+'.png'))
            video=a.output/'browsing-camera.mp4'
            process=subprocess.Popen(['ffmpeg','-loglevel','error','-y','-f','rawvideo','-vcodec','rawvideo',
                '-pix_fmt','rgb24','-s',f'{preview.gpu.W}x{preview.gpu.H}','-r','10','-i','-','-an',
                '-c:v','libx264','-preset','fast','-crf','23','-pix_fmt','yuv420p','-movflags','+faststart',str(video)],stdin=subprocess.PIPE)
            for index,camera in enumerate(cameras['frames']):
                selected=0 if 8<=index<24 else 4 if index>=38 else None
                process.stdin.write(frame(camera,index*2,selected).tobytes())
                if index%15==0:print(f'Production camera preview {index+1}/{len(cameras["frames"])}',flush=True)
            process.stdin.close();assert process.wait()==0
    record=dict(preview_only=True,native_runtime_proven=False,room='Original installed Djipi Granny',counter_top_y=41,
        input_sha256={str(path):hashlib.sha256(path.read_bytes()).hexdigest() for path in (a.maps,a.textures,a.native_oot,a.market)},
        source_sha256={str(path.relative_to(ROOT)):hashlib.sha256(path.read_bytes()).hexdigest() for path in [Path(__file__),
            ROOT/'tools/black_market/preview.py',ROOT/'combo/BlackMarketGranny.h',ROOT/'combo/BlackMarketGrannyScene.h',
            ROOT/'combo/BlackMarketBrowsing.h',ROOT/'combo/BlackMarketBrowsingNative.h',ROOT/'tools/black_market/browsing_preview.cpp']},
        archive_created=False,notes=['Original room vertex colors and all reachable original materials/textures are used.',
            'Decorative EnLight uses its two native textures, scrolling mask, colors, scale and two-cycle combiner. Offline filtering and room smoke remain approximate.',
            'Browsing outputs execute the production shared camera state; the bottom preview label reserves native textbox Y142. Native rendering requires an in-game check.',
            'Regular-interior still hides item seals for visual review; the independent toggle defaults to on.'])
    (a.output/'preview-evidence.json').write_text(json.dumps(record,indent=2)+'\n')
    print('Rendered external Granny previews; no classic O2R created.')

if __name__=='__main__':main()
