"""Offline production meshes, native arrow materials, and existing NEI crystals.

Usage: render_preview.py SAMPLES OOT_ARCHIVE OUTPUT_DIR [--still]
This is a presentation review, not held/shop runtime proof.
"""
import ctypes as C
from pathlib import Path
import io, json, struct, subprocess, sys, zipfile
import numpy as np
from PIL import Image, ImageDraw, ImageFont
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/nei_gi/runtime_preview'))
from gl_context import context, gl, ptr, integer, uint
source,archive,dest=map(Path,sys.argv[1:4]);dest.mkdir(parents=True,exist_ok=True)
raw=source.read_bytes();frames,item_count=struct.unpack_from('<II',raw);at=8
assert item_count==9
Dtype=np.dtype([('p','<f4',3),('rgba','u1',4),('uv','<f4',2)])
samples=[];clouds=[]
for tick in range(frames):
 modes=[]
 for mode in range(3):
  items=[]
  for item in range(item_count):
   hot,edge=struct.unpack_from('<II',raw,at);at+=8;layers=[]
   for layer in range(3 if item<3 else 5 if item<6 else 4):
    n,=struct.unpack_from('<I',raw,at);at+=4
    layers.append(np.frombuffer(raw,Dtype,n,at));at+=n*24
   items.append((hot,edge,layers))
  modes.append(items)
 samples.append(modes)
 clouds.append(np.frombuffer(raw,np.uint8,6*4096,at).reshape(6,64,64));at+=6*4096
assert at==len(raw)
# Decode the actual vanilla OPA display-list topology and vertex positions.
with zipfile.ZipFile(archive) as z:
 vtx=z.read('objects/object_gi_m_arrow/object_gi_m_arrowVtx_000000')
 dl=z.read('objects/object_gi_m_arrow/gGiMagicArrowDL')
 env_data=z.read('objects/gameplay_keep/gEffUnknown10Tex')
 fmt,ew,eh,esize=struct.unpack_from('<4I',env_data,64)
 assert fmt==6 and (ew,eh,esize)==(32,32,1024)
 native_intensity=np.frombuffer(env_data,np.uint8,esize,80).reshape(eh,ew)/255
points=[];normals=[];coords=[]
for i in range(struct.unpack_from('<I',vtx,68)[0]):
 v=struct.unpack_from('<3hH2h4b',vtx,72+16*i)
 points.append(v[:3]);normals.append(np.array(v[6:9],float)/127);coords.append(v[4:6])
x,y=np.deg2rad([12,25]);sx,cx,sy,cy=np.sin(x),np.cos(x),np.sin(y),np.cos(y)
R=np.array([[cy,0,sy],[sx*sy,cx,-sx*cy],[-cx*sy,sx,cx*cy]])
cache={};core=[];at=72;prim=np.ones(3);env=np.zeros(3)
geometry=0;scale=np.ones(2);shift=np.ones(2)
while at<len(dl):
 w0,w1=struct.unpack_from('<II',dl,at);at+=8;op=w0>>24
 if op==0x32:
  n=(w0>>12)&255;base=((w0&255)>>1)-n
  for i in range(n):cache[base+i]=w1//16+i
 if op in (0x20,0x32,0x33):at+=8
 if op==0xFA:prim=np.array([(w1>>24)&255,(w1>>16)&255,(w1>>8)&255])/255
 if op==0xFB:env=np.array([(w1>>24)&255,(w1>>16)&255,(w1>>8)&255])/255
 if op==0xD9:geometry=(geometry&(w0&0xFFFFFF))|w1
 if op==0xD7:scale=np.array([w1>>16,w1&65535])
 if op==0xF5 and not (w1>>24&7):shift=2.**np.array([w1&15,w1>>10&15])
 if op in (0x05,0x06):
  for word in ([w0,w1] if op==0x06 else [w0]):
   ids=[cache[(word>>s&255)//2] for s in (16,8,0)]
   triangle=[]
   for i in ids:
    if geometry&0x40000:
     normal=np.clip((normals[i]@R.T)[:2],-1,1)
     generated=np.arccos(-normal)/(2*np.pi) if geometry&0x80000 else (normal+1)/4
     uv=np.floor(generated*scale)/1024/shift
    else:uv=np.array(coords[i])*scale/65536/1024/shift
    triangle.append((points[i],uv))
   core.append((tuple(prim)+tuple(env),triangle))
W,H=1080,1406;context(W,H);F,D=C.c_float,C.c_double
ClearColor=gl('glClearColor',None,F,F,F,F);Clear=gl('glClear',None,uint)
Enable=gl('glEnable',None,uint);Disable=gl('glDisable',None,uint)
DepthMask=gl('glDepthMask',None,C.c_ubyte);Blend=gl('glBlendFunc',None,uint,uint)
Viewport=gl('glViewport',None,integer,integer,integer,integer)
MatrixMode=gl('glMatrixMode',None,uint);Identity=gl('glLoadIdentity',None)
Ortho=gl('glOrtho',None,D,D,D,D,D,D);Rotate=gl('glRotatef',None,F,F,F,F)
Begin=gl('glBegin',None,uint);End=gl('glEnd',None)
Vertex=gl('glVertex3f',None,F,F,F);Color=gl('glColor4f',None,F,F,F,F)
TexCoord=gl('glTexCoord2f',None,F,F)
EnableClient=gl('glEnableClientState',None,uint);DisableClient=gl('glDisableClientState',None,uint)
VertexPointer=gl('glVertexPointer',None,integer,uint,integer,ptr)
ColorPointer=gl('glColorPointer',None,integer,uint,integer,ptr)
TexCoordPointer=gl('glTexCoordPointer',None,integer,uint,integer,ptr)
DrawArrays=gl('glDrawArrays',None,uint,integer,integer)
GenTextures=gl('glGenTextures',None,integer,C.POINTER(uint));Bind=gl('glBindTexture',None,uint,uint)
TexParameteri=gl('glTexParameteri',None,uint,uint,integer)
TexImage=gl('glTexImage2D',None,uint,integer,integer,integer,integer,integer,uint,uint,ptr)
Read=gl('glReadPixels',None,integer,integer,integer,integer,uint,uint,ptr)
texids=(uint*9)();GenTextures(9,texids)
for texture in texids:
 Bind(0x0DE1,texture)
 for param in (0x2800,0x2801):TexParameteri(0x0DE1,param,0x2601)
 for param in (0x2802,0x2803):TexParameteri(0x0DE1,param,0x812F)
def upload(pixels,repeat=False):
 tid=uint();GenTextures(1,C.byref(tid));Bind(0x0DE1,tid.value)
 for param in (0x2800,0x2801):TexParameteri(0x0DE1,param,0x2601)
 for param in (0x2802,0x2803):TexParameteri(0x0DE1,param,0x2901 if repeat else 0x812F)
 pixels=np.ascontiguousarray(pixels,np.uint8)
 TexImage(0x0DE1,0,0x1908,pixels.shape[1],pixels.shape[0],0,0x1908,0x1401,pixels.ctypes.data)
 return tid.value
native_materials={}
for material,_ in core:
 if material in native_materials:continue
 prim=np.array(material[:3]);env=np.array(material[3:])
 # Both native cycles use (PRIMITIVE-ENVIRONMENT)*input+ENVIRONMENT.
 first=env+(prim-env)*native_intensity[:,:,None]
 color=env+(prim-env)*first
 pixels=np.empty((32,32,4),np.uint8);pixels[:,:,:3]=np.rint(color*255);pixels[:,:,3]=255
 native_materials[material]=upload(pixels,True)
def model(slug):
 checkpoint=ROOT/'tools/nei_gi/CHECKPOINTS'/slug
 data=(checkpoint/(slug+'.glb')).read_bytes();n=struct.unpack_from('<I',data,12)[0]
 doc=json.loads(data[20:20+n]);binary=data[28+n:]
 metadata=json.loads((checkpoint/'checkpoint.json').read_text())
 factor=doc['nodes'][-1]['scale'][0]*metadata['draw_scale']
 def accessor(i):
  a=doc['accessors'][i];v=doc['bufferViews'][a['bufferView']]
  cols={'VEC3':3,'VEC2':2,'SCALAR':1}[a['type']]
  return np.frombuffer(binary,{5126:'<f4',5125:'<u4'}[a['componentType']],a['count']*cols,
                       v.get('byteOffset',0)+a.get('byteOffset',0)).reshape(-1,cols)
 materials=[]
 for mat in doc['materials']:
  pbr=mat['pbrMetallicRoughness'];tid=0
  if 'baseColorTexture' in pbr:
   im=doc['images'][doc['textures'][pbr['baseColorTexture']['index']]['source']]
   v=doc['bufferViews'][im['bufferView']];start=v.get('byteOffset',0)
   tid=upload(np.array(Image.open(io.BytesIO(binary[start:start+v['byteLength']])).convert('RGBA')),True)
  materials.append((tid,pbr['baseColorFactor'],any(mat.get('emissiveFactor',[0])),mat.get('alphaMode')=='BLEND'))
 light=np.array([-.4,.75,.6]);light/=np.linalg.norm(light)
 passes=[[],[]]
 for entry in doc['meshes']:
  for primitive in entry['primitives']:
   tid,base,emissive,transparent=materials[primitive['material']];a=primitive['attributes']
   pos=accessor(a['POSITION'])*factor;normal=accessor(a['NORMAL'])@R.T
   uv=accessor(a['TEXCOORD_0']);indices=accessor(primitive['indices']).ravel()
   shade=np.ones(len(pos)) if emissive else .42+.58*np.maximum(normal@light,0)
   colors=np.c_[shade[:,None]*base[:3],np.full(len(pos),base[3])]
   passes[int(transparent)].append((tid,pos[indices],uv[indices],colors[indices]))
 return passes
nei_models=[model(slug) for slug in ('hylia_grace','zonai_permafrost','demise_destruction')]
def draw_model(parts):
 Enable(0x0B44)
 for tid,positions,uv,colors in parts:
  if tid:Enable(0x0DE1);Bind(0x0DE1,tid)
  else:Disable(0x0DE1)
  Begin(0x0004)
  for p,t,c in zip(positions,uv,colors):Color(*c);TexCoord(*t);Vertex(*p)
  End()
 Disable(0x0B44);Disable(0x0DE1)
art=[]
for slug in ('fire','ice','light'):
 b=(ROOT/'soh/assets/custom/objects/nei_elemental_arrow_gi'/slug).read_bytes()
 _,w,h,_,_,_,size=struct.unpack_from('<4I2fI',b,64)
 art.append(np.frombuffer(b,np.uint8,size,92).reshape(h,w,4))
Enable(0x0BE2);Blend(0x0302,0x0303);ClearColor(.04,.052,.075,1)
font=lambda size:ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',size)
def rgb(n):return np.array([(n>>16)&255,(n>>8)&255,n&255],float)/255
names=['FIRE ARROW','ICE ARROW','LIGHT ARROW',"DIN’S FIRE","FARORE’S WIND","NAYRU’S LOVE",
       "HYLIA’S GRACE",'ZONAI PERMAFROST','DEMISE DESTRUCTION']
notes=['Flame mantle + curling embers','Frost mantle + fine crystals','Radiant mantle + white-gold arcs',
       'Volatile fire + facet sheen','Turbulent wind + facet sheen','Protective barrier + facet sheen',
       'Existing pink core + facet sheen','Existing turquoise core + sheen','Existing black core + facet sheen']
def texture(tick,mode,item,hot,edge):
 if item<3:
  pixels=art[item]
 else:
  intensity=clouds[tick][item-3]/255
  pixels=np.empty((64,64,4),np.uint8)
  pixels[:,:,:3]=np.round(255*(rgb(edge)[None,None,:]+(rgb(hot)-rgb(edge))[None,None,:]*intensity[:,:,None]))
  pixels[:,:,3]=np.round(intensity*255)
 pixels=np.ascontiguousarray(pixels);Bind(0x0DE1,texids[item])
 TexImage(0x0DE1,0,0x1908,pixels.shape[1],pixels.shape[0],0,0x1908,0x1401,pixels.ctypes.data)
def render(tick,mode=0):
 DepthMask(1);Clear(0x4000|0x100)
 for item,(hot,edge,layers) in enumerate(samples[tick][mode]):
  row,col=divmod(item,3);Viewport(col*360,111+(2-row)*405,360,330)
  MatrixMode(0x1701);Identity();Ortho(-46,46,-42,42,-150,150)
  MatrixMode(0x1700);Identity();Rotate(12,1,0,0);Rotate(25,0,1,0)
  Enable(0x0B71);DepthMask(1);Disable(0x0DE1)
  if item<3:
   Enable(0x0B44);Enable(0x0DE1)
   for material,tri in core:
    Bind(0x0DE1,native_materials[material]);Color(1,1,1,1);Begin(0x0004)
    for p,uv in tri:TexCoord(*uv);Vertex(*p)
    End()
   Disable(0x0B44);Disable(0x0DE1)
  elif item>=6:draw_model(nei_models[item-6][0])
  DepthMask(0)
  texture(tick,mode,item,hot,edge)
  for layer,mesh in enumerate(layers):
   if item>=6 and layer==3:
    DisableClient(0x8074);DisableClient(0x8076);DisableClient(0x8078)
    draw_model(nei_models[item-6][1])
   textured=layer==(0 if item<3 or item>=6 else 1)
   if textured:
    Enable(0x0DE1);Bind(0x0DE1,texids[item]);EnableClient(0x8078)
    TexCoordPointer(2,0x1406,24,mesh.ctypes.data+16)
   else:Disable(0x0DE1);DisableClient(0x8078)
   EnableClient(0x8074);EnableClient(0x8076)
   VertexPointer(3,0x1406,24,mesh.ctypes.data);ColorPointer(4,0x1401,24,mesh.ctypes.data+12)
   DrawArrays(0x0004,0,len(mesh))
  DisableClient(0x8074);DisableClient(0x8076);DisableClient(0x8078)
 pixels=np.empty((H,W,4),np.uint8);Read(0,0,W,H,0x1908,0x1401,pixels.ctypes.data)
 result=Image.fromarray(pixels[::-1].copy()).convert('RGB');d=ImageDraw.Draw(result)
 title=['ELEMENTAL GI / CRYSTAL SHEEN','SHOP FOOTPRINT / ELEMENTAL GIs','LIVE PRIMARY + SECONDARY / SIX ELEMENTAL GIs'][mode]
 d.text((24,20),title,font=font(25),fill='#EDF1F8')
 d.text((24,57),'Native arrow materials · encompassing tip energy · reflection bands on six spell crystals',font=font(17),fill='#AAB9CE')
 for i,name in enumerate(names):
  row,col=divmod(i,3);y=117+row*405
  d.text((col*360+24,y),name,font=font(21),fill='#EDF1F8')
  d.text((col*360+24,y+347),notes[i],font=font(16),fill='#B5C1D1')
 d.text((24,H-60),'Production effects + native arrow material + existing NEI meshes; approximate lighting.',font=font(15),fill='#93A5BC')
 d.text((24,H-34),'Canonical views · offline visual candidate · held and shelf runtime review pending.',font=font(15),fill='#93A5BC')
 return result
for mode,name in enumerate(['Elemental_GI_Sheen.png','Elemental_GI_Sheen_Shelf.png','Elemental_GI_Sheen_Colors.png']):render(16,mode).save(dest/name)
if '--still' not in sys.argv:
 movie=dest/'Elemental_GI_Sheen.mp4'
 proc=subprocess.Popen(['ffmpeg','-y','-loglevel','error','-f','rawvideo','-pix_fmt','rgb24','-s',f'{W}x{H}','-r','10','-i','pipe:0','-c:v','libx264','-crf','18','-pix_fmt','yuv420p','-movflags','+faststart',str(movie)],stdin=subprocess.PIPE)
 for tick in range(frames):proc.stdin.write(render(tick).tobytes())
 proc.stdin.close();assert proc.wait()==0
 subprocess.run(['ffmpeg','-y','-loglevel','error','-i',str(movie),'-filter_complex','[0:v]fps=10,scale=864:-1:flags=lanczos,split[a][b];[a]palettegen=stats_mode=diff[p];[b][p]paletteuse=dither=sierra2_4a',str(dest/'Elemental_GI_Sheen.gif')],check=True)
print('Rendered native arrow materials and nine production GIs, including six faceted crystal sheen passes')
