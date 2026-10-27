#!/usr/bin/env python3
"""Render both shelf variants from actual source/assets, without a mock textbox."""
from pathlib import Path
import argparse,hashlib,json,subprocess,sys,zipfile
from PIL import ImageDraw,ImageFont
import granny_preview as gp
import preview

ROOT=Path(__file__).resolve().parents[2]

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('maps','textures','stormlib','native-oot','market','stock','output'):
        p.add_argument('--'+name,type=Path,required=True)
    p.add_argument('--motion',action='store_true',help='Render a short magical shelf-scroll preview')
    a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
    resources,counter,donor=gp.check(a.maps,a.textures,a.stormlib,a.output/'donor-preservation.json')
    stock=json.loads(a.stock.read_text())['stock'];shelves=preview.presentation.shelf_positions()
    sys.path.insert(0,str(ROOT/'tests/black_market'))
    from check_browsing_frame import camera_views
    cameras=camera_views()
    images=[]
    with zipfile.ZipFile(a.market) as market,zipfile.ZipFile(a.native_oot) as native:
        keeper=gp.NativeKeeper(market);placement=preview.presentation.keeper_placement(market)
        models={item['slug']:preview.presentation.checkpoint(preview.gpu,item['slug'])[0] for item in stock}
        lock=preview.presentation.native_lock().texture()
        lock_texture=preview.presentation.upload(preview.gpu,lock,lock.shape[1],lock.shape[0],wrap_t=0x812f)
        smoke=gp.native_effect(native,gp.PREFIX+'mahouya_room_0DL_006A78')
        shelf_textures=preview.presentation.granny_shelf_textures(preview.gpu,resources)
        for variant in ('magical','granny'):
            rows,textures=preview.streams(market if variant=='magical' else gp.ExternalRoom(resources,counter),native_colors=variant=='granny')
            for name in market.namelist():
                if '/keeper/' in name and 'Texture' in name:
                    pixels,w,h=gp.native_texture(market,name);textures[name]=preview.presentation.upload(preview.gpu,pixels,w,h)
            for name,view,fov,selected,clears in [
                ('front',preview.OVERVIEW_VIEW,preview.OVERVIEW_FOV,None,3),
                ('with-seals',preview.OVERVIEW_VIEW,preview.OVERVIEW_FOV,None,0),
                ('bank',(cameras['right']['eye'],cameras['right']['at']),cameras['right']['fov'],0,3)]:
                effects=[]
                if variant=='granny':
                    effects=smoke+[draw for x in (130,-139) for draw in gp.native_effect(native,
                        'objects/gameplay_keep/gEffFire1DL',position=(x,76,-8),scale=.0075,view=view)]
                image=preview.render(rows,textures,0,None,keeper=keeper,placement=placement,stock=stock,
                    shelves=shelves,models=models,lock_texture=lock_texture,view=view,fov=fov,
                    selected=selected,clears=clears,room_xlu=effects,
                    granny_shelves=variant=='granny',shelf_textures=shelf_textures)
                draw=ImageDraw.Draw(image)
                font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',17)
                label='Magical alcove · scrolling shelf material' if variant=='magical' else 'Granny interior · original timber, burgundy lining and carved trim'
                draw.text((18,12),label,font=font,fill='#f3eafc',stroke_width=2,stroke_fill='#15101b')
                detail='Offline assets · no mock textbox · '+('seals visible' if clears==0 else 'seals hidden')+' · smoke/lighting approximate'
                draw.text((18,image.height-28),detail,font=font,fill='#e2d3eb',stroke_width=2,stroke_fill='#15101b')
                output=a.output/(variant+'-'+name+'.png');image.save(output);images.append(output)
                print('Rendered',output.name,flush=True)
            if variant=='magical' and a.motion:
                video=a.output/'magical-shelf-scroll.mp4'
                process=subprocess.Popen(['ffmpeg','-loglevel','error','-y','-f','rawvideo','-vcodec','rawvideo',
                    '-pix_fmt','rgb24','-s',f'{preview.gpu.W}x{preview.gpu.H}','-r','10','-i','-','-an',
                    '-c:v','libx264','-preset','fast','-crf','23','-pix_fmt','yuv420p','-movflags','+faststart',str(video)],stdin=subprocess.PIPE)
                for frame in range(40):
                    image=preview.render(rows,textures,frame*2,None,keeper=keeper,placement=placement,stock=stock,
                        shelves=shelves,models=models,lock_texture=lock_texture,view=preview.OVERVIEW_VIEW,
                        fov=preview.OVERVIEW_FOV,clears=3)
                    draw=ImageDraw.Draw(image)
                    draw.text((18,12),'Magical shelves · separate scrolling surface',font=font,fill='#f3eafc',stroke_width=2,stroke_fill='#15101b')
                    draw.text((18,image.height-28),'Offline source/assets preview · seals hidden · native runtime not shown',font=font,fill='#e2d3eb',stroke_width=2,stroke_fill='#15101b')
                    process.stdin.write(image.tobytes())
                process.stdin.close();assert process.wait()==0;images.append(video)
                print('Rendered',video.name,flush=True)
    paths=['combo/BlackMarketShelves.h','combo/BlackMarketShelvesNative.h','combo/BlackMarketBrowsing.h',
        'combo/BlackMarketGrannyResources.h','combo/gui/ComboCosmeticsSync.cpp',
        'mm/2s2h/Enhancements/Gameplay/BlackMarket.cpp','soh/soh/Enhancements/Gameplay/ComboBlackMarketNative.cpp',
        'mm/2s2h/BenGui/CosmeticEditor.cpp','soh/soh/Enhancements/cosmetics/CosmeticsEditor.cpp',
        'tools/black_market/shelves_preview.cpp','tools/black_market/shelves_room_preview.py',
        'tools/black_market/preview.py','tools/black_market/presentation_preview.py','tools/black_market/granny_preview.py']
    (a.output/'preview-evidence.json').write_text(json.dumps(dict(preview_only=True,native_runtime_proven=False,
        shelf_geometry='Approved shared geometry; exact source-authored flow and external Granny atlas sampling',mock_textbox=False,
        source_sha256={path:hashlib.sha256((ROOT/path).read_bytes()).hexdigest() for path in paths},
        image_sha256={path.name:hashlib.sha256(path.read_bytes()).hexdigest() for path in images},
        input_sha256={str(path):hashlib.sha256(path.read_bytes()).hexdigest() for path in (a.maps,a.textures,a.native_oot,a.market)},
        notes=['Both variants draw the same source fixture. Original assets are read only.',
            'Magical flow is a distinct source-authored 64x64 I8 texture with native (-1,+1) quarter-texel scroll.',
            'Granny wood/velvet/trim sample exact external original textures at the production UVs; no color tint or scrolling.',
            'Cosmetics > World > Black Market Shelves controls only the magical material in both engines.',
            'Bank view uses the production browsing camera; front views are offline overview cameras.',
            'The native message system is used in-game. No replacement dialogue UI is added.',
            'Native room effects/filtering/lighting and executable behavior remain unproven.']),indent=2)+'\n')

if __name__=='__main__':main()
