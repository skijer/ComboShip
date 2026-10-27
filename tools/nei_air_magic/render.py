"""Render the actual candidate meshes/texels with standard alpha blending."""
import argparse,ctypes as C,struct,sys,subprocess,math
from pathlib import Path
import numpy as np
from PIL import Image,ImageDraw,ImageFont
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/nei_gi/runtime_preview'))
from gl_context import context,gl,ptr,integer,uint
W,H=1440,1000
context(W,H)
F=C.c_float;D=C.c_double
ClearColor=gl('glClearColor',None,F,F,F,F);Clear=gl('glClear',None,uint)
Enable=gl('glEnable',None,uint);Disable=gl('glDisable',None,uint)
Viewport=gl('glViewport',None,integer,integer,integer,integer)
MatrixMode=gl('glMatrixMode',None,uint);LoadIdentity=gl('glLoadIdentity',None)
Ortho=gl('glOrtho',None,D,D,D,D,D,D);Rotate=gl('glRotatef',None,F,F,F,F)
Begin=gl('glBegin',None,uint);End=gl('glEnd',None)
Vertex=gl('glVertex3f',None,F,F,F);Color=gl('glColor4f',None,F,F,F,F)
BlendFunc=gl('glBlendFunc',None,uint,uint);DepthMask=gl('glDepthMask',None,C.c_ubyte)
ReadPixels=gl('glReadPixels',None,integer,integer,integer,integer,uint,uint,ptr)
EnableClient=gl('glEnableClientState',None,uint);DisableClient=gl('glDisableClientState',None,uint)
VertexPointer=gl('glVertexPointer',None,integer,uint,integer,ptr);ColorPointer=gl('glColorPointer',None,integer,uint,integer,ptr)
DrawArrays=gl('glDrawArrays',None,uint,integer,integer)
GenTextures=gl('glGenTextures',None,integer,C.POINTER(uint));BindTexture=gl('glBindTexture',None,uint,uint)
TexParameteri=gl('glTexParameteri',None,uint,uint,integer)
TexImage=gl('glTexImage2D',None,uint,integer,integer,integer,integer,integer,uint,uint,ptr)
TexCoordPointer=gl('glTexCoordPointer',None,integer,uint,integer,ptr)
ClearColor(.025,.037,.057,1);Enable(0x0BE2);BlendFunc(0x0302,0x0303)
font=lambda n:ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',n)
NAMES=['LIGHTNING ROD  /  electrical projectile','TORNADO ROD  /  enveloping silver wind',
       'GUST JAR  /  inward suction','GUST JAR  /  outward wind']
NOTES=['White core, warm rim, branching crackle','One body envelope; neutral figure for scale',
       'Strands flow toward the nozzle','Strands flow away from the nozzle']
def load(path):
 raw=Path(path).read_bytes();at=0;frames=[]
 dtype=np.dtype([('p','<f4',3),('rgba','u1',4),('uv','<f4',2)])
 for frame in range(180):
  row=[]
  for panel in range(4):
   layers=struct.unpack_from('<I',raw,at)[0];at+=4;layerdata=[]
   for _ in range(layers):
    material,n=struct.unpack_from('<II',raw,at);at+=8
    layerdata.append((material,np.frombuffer(raw,dtype=dtype,count=n,offset=at)));at+=n*24
   row.append(layerdata)
  frames.append(row)
 assert at==len(raw)
 return frames
textures={}
def texture(material):
 if material in textures:return textures[material]
 name={1:'silver_wisp',2:'lightning_filament'}[material]
 raw=(ROOT/'soh/assets/custom/objects/nei_air_magic'/name).read_bytes();w,h=struct.unpack_from('<II',raw,68)
 pixels=np.frombuffer(raw,np.uint8,offset=92).reshape(h,w,4)
 tid=uint();GenTextures(1,C.byref(tid));BindTexture(0x0DE1,tid.value)
 TexParameteri(0x0DE1,0x2801,0x2601);TexParameteri(0x0DE1,0x2800,0x2601)
 TexParameteri(0x0DE1,0x2802,0x2901);TexParameteri(0x0DE1,0x2803,0x812F)
 TexImage(0x0DE1,0,0x1908,w,h,0,0x1908,0x1401,pixels.ctypes.data)
 textures[material]=tid.value;return tid.value
def drawmesh(data,material):
 Disable(0x0B44);Disable(0x0DE1);Enable(0x0B71);DepthMask(0)
 EnableClient(0x8074);EnableClient(0x8076)
 VertexPointer(3,0x1406,24,data.ctypes.data);ColorPointer(4,0x1401,24,data.ctypes.data+12)
 if material:
  Enable(0x0DE1);BindTexture(0x0DE1,texture(material));EnableClient(0x8078)
  TexCoordPointer(2,0x1406,24,data.ctypes.data+16)
 DrawArrays(0x0004,0,len(data));DisableClient(0x8074);DisableClient(0x8076);DisableClient(0x8078);Disable(0x0DE1)
def ellipsoid(center,size,rgb):
 # An opaque neutral scale reference, never included in the effect export.
 Enable(0x0B71);DepthMask(1);Disable(0x0DE1);Begin(0x0004)
 for ring in range(8):
  for j in range(12):
   a0=ring*math.pi/8;a1=(ring+1)*math.pi/8;b0=j*math.tau/12;b1=(j+1)*math.tau/12
   for a,b in [(a0,b0),(a1,b0),(a1,b1),(a0,b0),(a1,b1),(a0,b1)]:
    shade=.65+.35*(math.sin(a)*math.cos(b)*.3+math.cos(a)*.7)
    Color(*(c*shade for c in rgb),1)
    Vertex(center[0]+size[0]*math.sin(a)*math.cos(b),center[1]+size[1]*math.cos(a),center[2]+size[2]*math.sin(a)*math.sin(b))
 End()
def figure():
 c=(.16,.24,.31)
 for pos,size in [((0,38,0),(4.6,5.6,4.3)),((0,24,0),(6.2,9.8,3.7)),((0,15,0),(5,4.5,3.5)),
                  ((-3,7,0),(2.2,7,2.2)),((3,7,0),(2.2,7,2.2)),((-8,23,0),(1.8,7.8,1.8)),((8,23,0),(1.8,7.8,1.8))]:ellipsoid(pos,size,c)
def grid(extent,step):
 Disable(0x0B71);DepthMask(0);Color(.06,.09,.13,1);Begin(0x0001)
 for i in range(-extent,extent+1,step):
  Vertex(-extent,-2,i);Vertex(extent,-2,i);Vertex(i,-2,-extent);Vertex(i,-2,extent)
 End()
def draw(data):
 DepthMask(1);Clear(0x4000|0x0100)
 cw=720;ch=405
 for k in range(4):
  left=(k%2)*cw;top=(k//2)*440+135
  Viewport(left,H-top-ch,cw,ch);MatrixMode(0x1701);LoadIdentity()
  low,high=[(-31,47),(-8,56),(-125,135),(-125,135)][k]
  half=(high-low)*cw/ch/2;Ortho(-half,half,low,high,-700,700)
  MatrixMode(0x1700);LoadIdentity();Rotate(24,1,0,0);Rotate(-18,0,1,0)
  if k==1:grid(60,10);figure()
  elif k>=2:
   grid(240,40);ellipsoid((-107,0,0),(10,13,10),(.17,.22,.29))
  for material,layer in data[k]:drawmesh(layer,material)
 pixels=np.zeros((H,W,4),np.uint8);ReadPixels(0,0,W,H,0x1908,0x1401,pixels.ctypes.data)
 im=Image.fromarray(pixels[::-1].copy()).convert('RGB');d=ImageDraw.Draw(im)
 d.text((30,22),'NEI  /  AIR & LIGHTNING',font=font(30),fill='#edf2f8')
 d.text((30,66),'POC 01  •  actual candidate geometry and medallion materials',font=font(19),fill='#9aaabd')
 for k in range(4):
  x=(k%2)*cw+30;y=(k//2)*440+116
  d.text((x,y),NAMES[k],font=font(20),fill='#e4eaf2')
  d.text((x,y+30),NOTES[k],font=font(16),fill='#91a4b9')
 d.line((720,112,720,930),fill='#26354a');d.line((30,541,1410,541),fill='#26354a')
 d.text((30,960),'MM visual POC • 20 Hz animation • offline render; in-game appearance pending',font=font(17),fill='#8ca0b6')
 return im
def sheet(out):
 im=Image.new('RGB',(1300,1050),'#080d15');d=ImageDraw.Draw(im)
 d.text((32,24),'NEI  /  MATERIAL PREVIEW',font=font(29),fill='#edf2f8')
 d.text((32,68),'Approved medallion texels, reused with the new silver wind / lightning geometry',font=font(18),fill='#9aabbe')
 for i,(name,label) in enumerate([('silver_wisp','WIND  /  soft wisp surface'),('lightning_filament','LIGHTNING  /  luminous filament surface')]):
  raw=(ROOT/'soh/assets/custom/objects/nei_air_magic'/name).read_bytes();w,h=struct.unpack_from('<II',raw,68)
  tex=Image.frombytes('RGBA',(w,h),raw[92:]);tex.thumbnail((550,780))
  bg=Image.new('RGBA',tex.size,(9,15,24,255));bg.alpha_composite(tex)
  x=65+i*640+(550-tex.width)//2;im.paste(bg.convert('RGB'),(x,172))
  d.text((65+i*640,122),label,font=font(20),fill='#e4edf7')
  d.text((65+i*640,980),f'{w} × {h} RGBA • shape/color supplied by geometry',font=font(15),fill='#91a4b9')
 im.save(out/'NEI_AirLightning_POC1_Textures.png')
if __name__=='__main__':
 ap=argparse.ArgumentParser();ap.add_argument('samples');ap.add_argument('out');ap.add_argument('--still',action='store_true');a=ap.parse_args()
 out=Path(a.out);out.mkdir(parents=True,exist_ok=True);frames=load(a.samples)
 draw(frames[30]).save(out/'NEI_AirLightning_POC1_Still.png');sheet(out)
 if not a.still:
  video=out/'NEI_AirLightning_POC1.mp4'
  proc=subprocess.Popen(['ffmpeg','-y','-loglevel','error','-f','rawvideo','-pix_fmt','rgb24','-s',f'{W}x{H}',
       '-r','20','-i','-','-an','-c:v','libx264','-crf','19','-pix_fmt','yuv420p','-movflags','+faststart',str(video)],stdin=subprocess.PIPE)
  gifs=[]
  for i,row in enumerate(frames):
   im=draw(row);proc.stdin.write(im.tobytes())
   if i%2==0:gifs.append(im.resize((960,667),Image.Resampling.LANCZOS))
  proc.stdin.close();assert proc.wait()==0
  gifs[0].save(out/'NEI_AirLightning_POC1.gif',save_all=True,append_images=gifs[1:],duration=100,loop=0,optimize=False)
 print(out)
