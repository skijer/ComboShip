"""Software projection of native exported state and sprites; not gameplay capture."""
import argparse,json,re,subprocess
from pathlib import Path
import numpy as np
from PIL import Image,ImageDraw,ImageFont

ROOT=Path(__file__).resolve().parents[2]
W,H=1280,720
TAN=0.5773503
ASPECT=16/9

def project(v):
    x,y,z=v[:3]
    return np.array([(1-x/(z*TAN*ASPECT))*W/2,(1-(y-100)/(z*TAN))*H/2,z])

def triangle(image,vertices):
    p=np.array([project(v) for v in vertices]);alpha=np.array([v[3] for v in vertices])
    xmin=max(0,int(np.floor(p[:,0].min())));xmax=min(W,int(np.ceil(p[:,0].max())))
    ymin=max(0,int(np.floor(p[:,1].min())));ymax=min(H,int(np.ceil(p[:,1].max())))
    if xmax<=xmin or ymax<=ymin:return
    yy,xx=np.mgrid[ymin:ymax,xmin:xmax];xx=xx+.5;yy=yy+.5
    den=(p[1,1]-p[2,1])*(p[0,0]-p[2,0])+(p[2,0]-p[1,0])*(p[0,1]-p[2,1])
    if abs(den)<1e-6:return
    a=((p[1,1]-p[2,1])*(xx-p[2,0])+(p[2,0]-p[1,0])*(yy-p[2,1]))/den
    b=((p[2,1]-p[0,1])*(xx-p[2,0])+(p[0,0]-p[2,0])*(yy-p[2,1]))/den
    c=1-a-b;mask=(a>=0)&(b>=0)&(c>=0)
    # Match perspective-correct vertex alpha of the browser projection.
    opacity=(a*alpha[0]/p[0,2]+b*alpha[1]/p[1,2]+c*alpha[2]/p[2,2])/(a/p[0,2]+b/p[1,2]+c/p[2,2])
    opacity=np.where(mask,opacity,0)[...,None]
    patch=image[ymin:ymax,xmin:xmax]
    patch[:]=patch*(1-opacity)+np.array([255,244,204])*opacity

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--samples',type=Path,required=True);parser.add_argument('--background',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True);parser.add_argument('--stills',type=Path,required=True)
    args=parser.parse_args();args.stills.mkdir(parents=True,exist_ok=True)
    frames=json.loads(args.samples.read_text())
    source=(ROOT/'mm/2s2h/Enhancements/Graphics/MMSummerAtmosphereTextures.h').read_text()
    textures=[]
    for name in ('kDandelionTexture','kGlowTexture'):
        body=re.search(name+r' = \{(.*?)\};',source,re.S).group(1)
        textures.append(np.array([int(x,16)&15 for x in re.findall(r'0x[0-9A-Fa-f]+',body)],dtype=np.uint8).reshape(16,16)*17)
    base=np.asarray(Image.open(args.background).convert('RGB').resize((W,H),Image.Resampling.BILINEAR),dtype=np.float32)
    labels={12:'DAY · Dandelion fluff',18:'DUSK · Fluff fades into fireflies',23:'NIGHT · Softly pulsing fireflies'}
    try:font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',20)
    except OSError:font=ImageFont.load_default()
    proc=subprocess.Popen(['ffmpeg','-hide_banner','-loglevel','error','-y','-f','rawvideo','-pixel_format','rgb24','-video_size',f'{W}x{H+90}','-framerate','20','-i','pipe:0','-an','-c:v','libx264','-crf','18','-pix_fmt','yuv420p','-movflags','+faststart',str(args.output)],stdin=subprocess.PIPE)
    for index,frame in enumerate(frames):
        hour=frame['input']['hour'];beams=frame['input']['sunbeams'];tint={12:[1.3,1.25,1.18],18:[.82,.74,.72],23:[.34,.40,.58]}[hour]
        image=np.clip(base*np.array(tint),0,255)
        for p in frame['particles']:
            if p['alpha']<1/255:continue
            x,y,z=project(p['position']);r=p['radius']*H/(2*z*TAN)
            x0=max(0,int(np.floor(x-r)));x1=min(W,int(np.ceil(x+r)))
            y0=max(0,int(np.floor(y-r)));y1=min(H,int(np.ceil(y+r)))
            if x1<=x0 or y1<=y0:continue
            # This software raster uses screen-space texture u increasing right,
            # equivalent to native corners position + view.right * localX.
            sprite=Image.fromarray(textures[p['kind']]).resize((x1-x0,y1-y0),Image.Resampling.BILINEAR)
            a=np.asarray(sprite,dtype=np.float32)[...,None]*p['alpha']/255
            color=np.array([238,255,154] if p['kind'] else [255,244,204])
            patch=image[y0:y1,x0:x1];patch[:]=patch*(1-a)+color*a
        for beam in frame['beams']:
            for row in range(3):
                for col in range(2):
                    i=row*3+col
                    triangle(image,[beam[j] for j in (i,i+3,i+4)])
                    triangle(image,[beam[j] for j in (i,i+4,i+1)])
        result=Image.new('RGB',(W,H+90),(15,23,17));result.paste(Image.fromarray(image.astype(np.uint8)),(0,0))
        draw=ImageDraw.Draw(result)
        draw.text((20,H+12),labels[hour]+(' · Soft sunbeams ON (review option)' if beams and hour!=23 else ' · Soft sunbeams OFF'),fill=(225,235,213),font=font)
        draw.text((20,H+46),'Native exported particles and IA8 sprites · simulated lighting · no scene occlusion or gameplay capture',fill=(166,183,158),font=font)
        if index in (60,140,200,300):result.save(args.stills/f'summer-{index:03}.png')
        proc.stdin.write(result.tobytes())
    proc.stdin.close();code=proc.wait()
    if code:raise SystemExit(code)
    print(f'PASS motion artifact: {len(frames)} native frames, 20 fps, {W}x{H+90}; four stills saved')

if __name__=='__main__':main()
