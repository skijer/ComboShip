#!/usr/bin/env python3
"""Render packed room, native animated keeper and production sample stock.

Offline asset inspection; not a gameplay capture or runtime acceptance.
"""
import argparse
import ctypes as C
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import uuid
import xml.etree.ElementTree as E
import zipfile
import numpy as np
from PIL import Image, ImageDraw, ImageFont
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import preview_mm_lost_woods as gpu
from native_preview import NativeKeeper
import presentation_preview as presentation

SOURCE_PATHS = [
    'mm/2s2h/Enhancements/Gameplay/BlackMarket.cpp','combo/rando/BlackMarket.h',
    'soh/soh/Enhancements/randomizer/NeiGiFrameFit.h','soh/soh/Enhancements/randomizer/NeiGiFrameBounds.inc',
    'soh/soh/Enhancements/randomizer/NeiGiShopFit.h','soh/soh/Enhancements/randomizer/NeiGiPresentation.cpp',
    'tools/black_market/preview.py','tools/black_market/native_preview.py',
    'tools/black_market/presentation_preview.py','tools/black_market/sample_stock.cpp',
    'tools/black_market/keeper_lean.py','tools/black_market/lock_preview.cpp',
    'combo/BlackMarketLock.h','combo/BlackMarketFrame.h','combo/BlackMarketKeeper.h',
    'combo/BlackMarketShelves.h','combo/BlackMarketShelvesNative.h','tools/black_market/shelves_preview.cpp',
    'soh/soh/Enhancements/Gameplay/ComboBlackMarketNative.cpp','tools/preview_mm_lost_woods.py',
    'combo/entrances/BlackMarketEntrance.h','tools/black_market/build.py','tools/black_market/orientation.py'
]
OVERVIEW_VIEW = ((0,76,250),(0,76,80))
OVERVIEW_FOV = 70
WIDE_VIEW = ((-11,70,381),(0,70,84))
WIDE_FOV = 80

def streams(archive, *, native_colors=False):
    textures = {}
    state = {"texture":None,"size":[32,32],"color":[1,1,1,1],"scroll":[0,0]}
    cache, result = {}, []
    def visit(name):
        for element in E.fromstring(archive.read(name)):
            tag = element.tag
            if tag == "CallDisplayList": visit(element.get("Path"))
            elif tag == "SetTextureImage": state["texture"] = element.get("Path")
            elif tag == "SetTileSize":
                state["size"] = [int(element.get(a))//4+1 for a in ("Lrs","Lrt")]
                state["scroll"] = [0,0]
            elif tag == "ScrollTexture": state["scroll"] = [int(element.get(a)) for a in ("SpeedX","SpeedY")]
            elif tag == "SetPrimColor": state["color"] = [int(element.get(a))/255 for a in ("R","G","B","A")]
            elif tag == "LoadVertices":
                vertices = E.fromstring(archive.read(element.get("Path")))
                start,offset,count = [int(element.get(a)) for a in ("VertexBufferIndex","VertexOffset","Count")]
                for i,v in enumerate(list(vertices)[offset:offset+count]):
                    cache[start+i] = [float(v.get(a)) for a in ("X","Y","Z","S","T","R","G","B","A")]
            elif tag in ("Triangle1","Triangles2"):
                points=[]
                for face in range(2 if tag == "Triangles2" else 1):
                    keys = [f"V{face}{i}" for i in range(3)]
                    points.extend(cache[int(element.get(k))] for k in keys)
                array=np.array(points,dtype=np.float32)
                array[:,3:5]/=np.array(state["size"])*32
                array[:,5:8]=(array[:,5:8]/255 if native_colors else 0.25+array[:,5:8]/255*.75)*np.array(state["color"][:3])
                array[:,8]/=255
                result.append((state["texture"],state["size"].copy(),state["scroll"].copy(),np.ascontiguousarray(array)))
            elif tag.startswith('Triangle'):
                raise ValueError(f'Unsupported native XML triangle command {tag} in {name}')
    visit("custom/skiejer_market/room")
    for texture in {r[0] for r in result}:
        data=archive.read(texture)
        kind,width,height,flags,hs,vs,size=struct.unpack_from("<IIIIffI",data,64)
        if flags&1:
            pixels=np.frombuffer(data[92:],dtype=np.uint8).copy()
        else:
            packed=np.frombuffer(data[92:],dtype=">u2").astype(np.uint16)
            pixels=np.column_stack((((packed>>11)&31)*255//31,((packed>>6)&31)*255//31,
                                    ((packed>>1)&31)*255//31,(packed&1)*255)).astype(np.uint8).reshape(-1)
        tid=gpu.uint();gpu.GenTextures(1,C.byref(tid));gpu.BindTexture(0x0DE1,tid.value)
        for prop,value in ((0x2801,0x2601),(0x2800,0x2601),(0x2802,0x2901),(0x2803,0x2901)):
            gpu.TexParameteri(0x0DE1,prop,value)
        gpu.TexImage(0x0DE1,0,0x1908,width,height,0,0x1908,0x1401,pixels.ctypes.data)
        textures[texture]=tid.value
    return result,textures

def render(rows,textures,tick,title,*,keeper=None,placement=None,stock=(),shelves=(),models=None,
           lock_texture=None,view=None,selected=None,show_room=True,show_keeper=True,show_stock=True,clears=0,fov=WIDE_FOV,room_xlu=(),
           granny_shelves=False,shelf_textures=(),shelf_tint=None):
    gpu.DepthMask(1)
    gpu.ClearColor(.028,.021,.046,1);gpu.Clear(0x00004000|0x00000100)
    view=WIDE_VIEW if view is None else view
    gpu.camera(*view)
    gpu.MatrixMode(0x1701);gpu.LoadIdentity()
    half=np.tan(np.deg2rad(fov)/2)
    gpu.Frustum(-half*gpu.W/gpu.H,half*gpu.W/gpu.H,-half,half,1,9000)
    gpu.MatrixMode(0x1700)
    if show_room:
        for texture,size,scroll,original in rows:
            array=original.copy()
            array[:,3:5] -= np.array(scroll)*tick/(np.array(size)*4)
            gpu.draw(array,textures.get(texture))
        presentation.draw_shelves(gpu,tick,granny_shelves,shelf_textures,shelf_tint,room_textures=textures)
    if keeper and show_keeper:
        presentation.draw_keeper(gpu,keeper,textures,tick,placement)
    if show_stock:
        translucent=[]
        for slot,(item,position) in enumerate(zip(stock,shelves)):
            matrix=presentation.item_pose(item,position,slot,tick,selected==slot)
            for texture,array,transparent in models[item['slug']]:
                array=presentation.transform(array,matrix)
                if transparent:translucent.append((texture,array))
                else:gpu.draw(array,texture)
        gpu.DepthMask(0)
        for texture,array in translucent:gpu.draw(array,texture)
        for array in presentation.seal_arrays(stock,shelves,tick,selected,clears,models):gpu.draw(array,lock_texture)
    gpu.DepthMask(0)
    if show_room:
        for texture,array in room_xlu:gpu.draw(array,texture)
    gpu.DepthMask(1)
    pixels=np.empty((gpu.H,gpu.W,4),dtype=np.uint8)
    gpu.ReadPixels(0,0,gpu.W,gpu.H,0x1908,0x1401,pixels.ctypes.data)
    image=Image.fromarray(pixels[::-1].copy(),"RGBA")
    if title:
        data=title[80:]
        rgba=bytes(channel for p in data for channel in ((p>>4)*17,(p>>4)*17,(p>>4)*17,(p&15)*17))
        size=(192*gpu.W//320,24*gpu.H//240)
        card=Image.frombytes("RGBA",(192,24),rgba).resize(size,Image.Resampling.LANCZOS)
        image.alpha_composite(card,((gpu.W-size[0])//2,(gpu.H-size[1])//2))
    image=image.convert('RGB')
    image.info['stock_markers']=[(slot+1,project(np.array(pos)+[0,5,0],view,fov)) for slot,pos in enumerate(shelves)] if show_stock else []
    image.info['clears']=clears
    return image

def project(position,view,fov):
    eye,target=np.asarray(view[0],float),np.asarray(view[1],float)
    forward=target-eye;forward/=np.linalg.norm(forward)
    right=np.cross(forward,[0,1,0]);right/=np.linalg.norm(right)
    up=np.cross(right,forward);point=np.asarray(position)-eye;depth=point@forward
    if depth<=0:return None
    half=np.tan(np.deg2rad(fov)/2)
    return (gpu.W/2+(point@right)/depth/half*gpu.H/2,gpu.H/2-(point@up)/depth/half*gpu.H/2)

def annotate(image,heading,detail,stock=None):
    draw=ImageDraw.Draw(image)
    font=lambda n:ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',n)
    draw.rectangle((0,0,image.width,67),fill='#100e1b')
    draw.text((18,10),heading,font=font(25),fill='#f3eafc')
    draw.text((18,44),detail,font=font(14),fill='#c0afce')
    bottom=image.height-(116 if stock else 37)
    draw.rectangle((0,bottom,image.width,image.height),fill='#100e1b')
    if stock:
        for slot,point in image.info.get('stock_markers',[]):
            if point is None:continue
            anchor_x,anchor_y=point;x,y=anchor_x+25,anchor_y-4
            if 16<x<image.width-16 and 83<y<bottom-16:
                draw.line((anchor_x,anchor_y,x-12,y),fill='#d8b4f3',width=1)
                draw.ellipse((x-13,y-13,x+13,y+13),fill='#100e1b',outline='#d8b4f3',width=2)
                draw.text((x-5,y-10),str(slot),font=font(15),fill='#fff3ff')
        for slot,item in enumerate(stock):
            x,y=18+(slot%4)*((image.width-36)//4),bottom+9+(slot//4)*38
            draw.text((x,y),f"{slot+1}. {item['label']}",font=font(15),fill='#f3eafc')
            lock=f" · sealed:{item['clears']}" if image.info.get('clears',0)<item['clears'] else ''
            draw.text((x,y+19),f"T{item['tier']} · {item['price']} rupees"+(' · wild' if item['wildcard'] else '')+lock,font=font(11),fill='#b7a4c9')
    draw.text((18,image.height-26),'Offline assets · approximate lighting · GI effects omitted · gameplay and Alt parity untested',font=font(13),fill='#bcaaca')
    return image

def export_video(frame, stock, video, frames):
    """One complete room view per output frame; never stitch actor images."""
    temporary_video=video.parent/('.shop-presentation-'+uuid.uuid4().hex+'.mp4')
    process=subprocess.Popen(['ffmpeg','-loglevel','error','-y','-f','rawvideo','-vcodec','rawvideo','-pix_fmt','rgb24',
                              '-s',f'{gpu.W}x{gpu.H}','-r','10','-i','-','-an','-c:v','libx264','-preset','fast','-crf','23',
                              '-pix_fmt','yuv420p','-movflags','+faststart',str(temporary_video)],stdin=subprocess.PIPE)
    for index in range(frames):
        image=frame(tick=index*3,view=OVERVIEW_VIEW,fov=OVERVIEW_FOV)
        assert image.size==(gpu.W,gpu.H) and image.mode=='RGB'
        annotate(image,'Skiejer’s Black Market · counter display',
                 'Eight floating offers at the counter · zero clears: tiers 2/3 sealed',stock)
        process.stdin.write(image.tobytes())
        if index%24==0:print(f'Preview animation {index+1}/{frames}',flush=True)
    process.stdin.close();assert process.wait()==0
    temporary_video.replace(video)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("pack",type=Path);parser.add_argument("output",type=Path)
    parser.add_argument('--json-include',type=Path,help='Include directory containing nlohmann/json.hpp for production stock fixture')
    parser.add_argument('--stock',type=Path,help='Reuse sample-stock.json generated by this fixture')
    parser.add_argument('--frames',type=int,default=72)
    parser.add_argument('--extras',action='store_true',help='Also write isolated model and additional diagnostic stills')
    args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    source_hashes={p:hashlib.sha256((presentation.ROOT/p).read_bytes()).hexdigest() for p in SOURCE_PATHS}
    pack_hash=hashlib.sha256(args.pack.read_bytes()).hexdigest()
    market=json.loads(args.stock.read_text()) if args.stock else presentation.production_stock(args.json_include)
    assert market['preview_only']
    stock=market['stock'];shelves=presentation.shelf_positions()
    assert len(stock)==len(shelves)==8 and sum(i['wildcard'] for i in stock)==2
    assert [sum(i['tier']==t for i in stock) for t in (1,2,3)]==[2,4,2]
    (args.output/'sample-stock.json').write_text(json.dumps(market,indent=2)+'\n')
    with zipfile.ZipFile(args.pack) as archive:
        rows,textures=streams(archive)
        title=archive.read("custom/skiejer_market/title")
        keeper=NativeKeeper(archive);placement=presentation.keeper_placement(archive)
        for name in archive.namelist():
            if '/keeper/' in name and 'Texture' in name:
                data=archive.read(name);kind,width,height,flags,hs,vs,size=struct.unpack_from('<IIIIffI',data,64)
                assert flags&1 and len(data)==92+width*height*4
                textures[name]=presentation.upload(gpu,np.frombuffer(data[92:],dtype=np.uint8),width,height)
        models={};sources=[]
        for item in stock:
            model,meta,digest=presentation.checkpoint(gpu,item['slug'])
            assert abs(meta['draw_scale']-item['draw_scale'])<1e-6
            models[item['slug']]=model;sources.append({'slug':item['slug'],'glb_sha256':digest,'checkpoint':meta})
        lock_pixels=presentation.native_lock().texture()
        lock_texture=presentation.upload(gpu,lock_pixels,lock_pixels.shape[1],lock_pixels.shape[0],wrap_t=0x812F)
        options=dict(keeper=keeper,placement=placement,stock=stock,shelves=shelves,models=models,lock_texture=lock_texture)
        def frame(tick=0,title=None,**kw):return render(rows,textures,tick,title,**options,**kw)
        # Keep every viewport pixel for perimeter review; diagnostic labels
        # live in the evidence JSON rather than cover room geometry.
        diagnostic=frame();diagnostic.save(args.output/'wide-diagnostic.png')
        diagnostic_regions=presentation.background_regions(diagnostic)
        annotate(frame(view=OVERVIEW_VIEW,fov=OVERVIEW_FOV),'Skiejer’s Black Market · customer-side view','Native entrance behind the viewer · keeper behind her counter · flat floating offers',stock).save(args.output/'interior.png')
        if args.extras:
            annotate(frame(clears=2,view=OVERVIEW_VIEW,fov=OVERVIEW_FOV),'Skiejer’s Black Market · all sample offers unsealed','Same native item poses and room · illustrative state after two unique dungeon clears',stock).save(args.output/'interior-unsealed.png')
            annotate(frame(title=title,view=OVERVIEW_VIEW,fov=OVERVIEW_FOV),'Skiejer’s Black Market · native title card','Actual packed room, keeper and title · illustrative production-generated stock').save(args.output/'interior-title.png')
            annotate(frame(view=((-35,86,300),(0,95,0)),fov=45,show_keeper=False,show_stock=False),'Complete room architecture','Actual packed geometry and materials · no room geometry hidden in this view').save(args.output/'room-only.png')
        x,y,z=placement['position'];angle=placement['yaw']*np.pi/32768
        view=((x-22+100*np.sin(angle),y+42,z+100*np.cos(angle)),(x,y+32,z))
        annotate(frame(view=view,fov=45),'Goth Treasure Chest Shop Gal · crossed-arm lean',f'Packed 23-limb flex skeleton · lean frame 0/{keeper.frame_count} · native scale .01 · room retained').save(args.output/'keeper-in-room.png')
        if args.extras:
            annotate(frame(view=view,fov=45,show_room=False,show_stock=False),'Goth Treasure Chest Shop Gal · asset inspection','Room hidden in this isolated model view only · packed lean and per-vertex flex matrices').save(args.output/'keeper-detail.png')
        for side,sign in (('positive-x',1),('negative-x',-1)):
            view=((sign*52.5,86,220),(sign*52.5,86,110))
            if args.extras:
                annotate(frame(tick=66,view=view,fov=65,show_keeper=False,selected=1 if sign>0 else 5),f'Merchandise close-up · {side}','Keeper hidden for this item-only detail · zero clears: tiers 2/3 sealed',stock).save(args.output/('stock-'+side+'.png'))
                annotate(frame(tick=66,view=view,fov=65,show_keeper=False,clears=2),f'Merchandise close-up · {side} · unsealed','Keeper hidden for this item-only detail · room and decorative clutter retained',stock).save(args.output/('stock-'+side+'-unsealed.png'))
        annotate(frame(tick=66,view=OVERVIEW_VIEW,fov=OVERVIEW_FOV,clears=2),
                 'Eight sample offers · one continuous room view',
                 'One shopkeeper · illustrative unlocked state after two unique clears',stock).save(args.output/'stock-overview.png')
        if args.frames:
            export_video(frame,stock,args.output/'shop-presentation.mp4',args.frames)
        geometry=keeper.geometry(0,placement['position'],placement['yaw'],placement['scale'])
        points=np.concatenate([a[:,:3] for _,a in geometry])
        bounds=[]
        for slot,(item,pos) in enumerate(zip(stock,shelves)):
            points=np.concatenate([presentation.transform(a,presentation.item_pose(item,pos,slot,0))[:,:3] for _,a,_ in models[item['slug']]])
            bounds.append({'slot':slot+1,'key':item['key'],'slug':item['slug'],'position':pos,'frame':0,'selected':False,
                           'minimum':points.min(0).tolist(),'maximum':points.max(0).tolist(),
                           'caller_scale':.25,'gi_draw_scale':item['draw_scale'],'shop_fit_scale':item['fit_scale'],'shop_fit_lift':item['fit_lift']})
        keeper_points=np.concatenate([a[:,:3] for _,a in geometry])
        assert source_hashes=={p:hashlib.sha256((presentation.ROOT/p).read_bytes()).hexdigest() for p in SOURCE_PATHS}, 'Source changed during preview; regenerate from a stable checkpoint'
        assert pack_hash==hashlib.sha256(args.pack.read_bytes()).hexdigest(), 'Pack changed during preview'
        clearance=presentation.clearance_report(rows,models,stock,shelves)
        (args.output/'stock-clearance.json').write_text(json.dumps(clearance,indent=2)+'\n')
        evidence={'runtime_tested':False,'pack':str(args.pack.resolve()),'pack_sha256':pack_hash,'source_sha256':source_hashes,
                  'presentation_layout':{'video':'one continuous room camera','overview_camera':{'eye':OVERVIEW_VIEW[0],'target':OVERVIEW_VIEW[1],'vertical_fov_degrees':OVERVIEW_FOV},
                                         'stock_layout':'two flat 2x2 shop banks; both rows share one frontal depth',
                                         'keeper_draws_per_scene_view':1,'keeper_hidden_in_item_only_details':True},
                  'wide_diagnostic_camera':{'eye':WIDE_VIEW[0],'target':WIDE_VIEW[1],'vertical_fov_degrees':WIDE_FOV,'viewport':[gpu.W,gpu.H],'tick':0,'dungeon_clears':0},
                  'wide_diagnostic_background_regions':diagnostic_regions,
                  'animation_sampling':{'native_ticks_per_video_frame':3,'native_ticks_per_second':30,
                                        'first_blink_cycle_from_initial_timer':90,'subsequent_blink_phase':'illustrative; depends on absolute gameplayFrames'},
                  'room_triangles':sum(len(r[3])//3 for r in rows),'keeper':{'limbs':keeper.limb_count,'display_matrices':keeper.display_count,
                  'animation_path':keeper.animation_path,'lean_frames':keeper.frame_count,'triangles':sum(len(a)//3 for _,a in geometry),'placement':placement,
                  'frame_zero_world_bounds':[keeper_points.min(0).tolist(),keeper_points.max(0).tolist()]},'shelves':shelves,'seal_style':presentation.seal_style(),
                  'item_sources':sources,'stock_world_bounds':bounds,'continuous_model_clearance':clearance,
                  'limitations':['Offline Mesa lighting/alpha; no engine capture or runtime acceptance.',
                  'Production Generate uses a filtered eight-item example set; not a user seed/save.',
                  'Checkpoint GI models assume that runtime selects those resources; external packs and Alt parity untested.',
                  'Procedural GI energy/shimmer omitted; native shop seal triangles included.',
                  'Integer-frame native lean evaluated; no engine interpolation or camera/talk behavior proof.']}
        (args.output/'preview-evidence.json').write_text(json.dumps(evidence,indent=2)+'\n')
        print(f"PASS offline preview: {evidence['room_triangles']} room triangles; {keeper.limb_count} keeper limbs; {keeper.display_count} flex matrices; 8 production stock poses")

if __name__=='__main__':main()
