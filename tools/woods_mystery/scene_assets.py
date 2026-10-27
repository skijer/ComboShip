"""Read the scene resources used by this POC; previews are asset renders, not gameplay."""
from pathlib import Path
import struct, zipfile, xml.etree.ElementTree as ET, io
import numpy as np
from PIL import Image

MASK=(1<<64)-1
TABLE=[]
for i in range(256):
    c=i<<56
    for _ in range(8): c=((c<<1)^0x42f0e1eba9ea3693 if c&(1<<63) else c<<1)&MASK
    TABLE.append(c)
def crc64(s):
    c=MASK
    for b in s.encode(): c=(TABLE[((c>>56)^b)&255]^(c<<8))&MASK
    return c
def canonical(s): return s[4:] if s.startswith('alt/') else s
EXT={32,49,50,51,53,54,66,71}
def commands(b):
    p=72
    while p+8<=len(b):
        a,c=struct.unpack_from('<II',b,p); op=a>>24; size=16 if op in EXT else 8
        if p+size>len(b): raise ValueError(f'Truncated command at {p}')
        h=None
        if size==16:
            hi,lo=struct.unpack_from('<II',b,p+8); h=(hi<<32)|lo
        yield p,op,a,c,h,size
        p+=size
        if op==0xdf: break
def vertices(b):
    if b.startswith(b'<'):
        root=ET.fromstring(b)
        rows=[[int(v.get(k,'0')) for k in ['X','Y','Z','S','T','R','G','B','A']] for v in root]
        return np.asarray(rows,dtype=float)
    typ,n=struct.unpack_from('<II',b,64)
    assert typ==25
    ver=struct.unpack_from('<I',b,8)[0]
    size=24 if ver==1 else 16; rows=[]
    for i in range(n):
        p=72+i*size
        xyz=struct.unpack_from('<iii' if ver else '<hhh',b,p)
        off=12 if ver else 6
        uv=struct.unpack_from('<hh',b,p+off+2)
        rgba=struct.unpack_from('BBBB',b,p+off+6)
        rows.append([*xyz,*uv,*rgba])
    return np.asarray(rows,dtype=float)
def texture(b):
    if b[:4]==b'\x89PNG': return Image.open(io.BytesIO(b)).convert('RGBA')
    ver=struct.unpack_from('<I',b,8)[0]
    fmt,w,h=struct.unpack_from('<III',b,64)
    data=b[92:] if ver==1 else b[80:]
    flags=struct.unpack_from('<I',b,76)[0] if ver==1 else 0
    # POC9 HD replacements retain the original RGBA16 type, with the v1 raw
    # image flag and RGBA32 payload. Decode that payload before native formats.
    if fmt==1 or (ver==1 and flags&1 and len(data)==w*h*4):
        return Image.frombytes('RGBA',(w,h),data[:w*h*4])
    if fmt==2:
        p=np.frombuffer(data[:w*h*2],dtype='>u2').astype(np.uint32)
        a=np.stack([((p>>11)&31)*255//31,((p>>6)&31)*255//31,((p>>1)&31)*255//31,(p&1)*255],axis=-1).astype('uint8')
        return Image.fromarray(a.reshape(h,w,4))
    if fmt in [5,7]:
        p=np.frombuffer(data[:(w*h+1)//2],dtype='uint8')
        p=np.stack([p>>4,p&15],axis=1).reshape(-1)[:w*h]
        if fmt==5: i=p*17; a=np.full_like(i,255)
        else: i=(p>>1)*255//7; a=(p&1)*255
    elif fmt in [6,8]:
        p=np.frombuffer(data[:w*h],dtype='uint8')
        if fmt==6: i=p; a=np.full_like(i,255)
        else: i=(p>>4)*17; a=(p&15)*17
    elif fmt==9:
        p=np.frombuffer(data[:w*h*2],dtype='uint8').reshape(-1,2); i=p[:,0];a=p[:,1]
    else:
        return Image.new('RGBA',(w,h),(170,165,150,255))
    return Image.fromarray(np.stack([i,i,i,a],axis=-1).reshape(h,w,4))

class Resources:
    def __init__(self,*archives):
        self.files={}; self.hashes={}; self.tex={}; self.vtx={}
        for ar in archives:
            with zipfile.ZipFile(ar) as z:
                for n in z.namelist(): self.files[n]=z.read(n)
        self.reindex()
    def reindex(self):
        self.hashes={crc64(canonical(n)):canonical(n) for n in self.files}
    def get(self,n): return self.files.get('alt/'+canonical(n),self.files.get(canonical(n)))
    def v(self,n):
        n=canonical(n)
        if n not in self.vtx: self.vtx[n]=vertices(self.get(n))
        return self.vtx[n]
    def t(self,n):
        n=canonical(n)
        if n not in self.tex: self.tex[n]=texture(self.get(n))
        return self.tex[n]
    def mesh(self,path,seen=None):
        seen=set() if seen is None else seen
        path=canonical(path)
        if path in seen: raise ValueError(f'Cycle at {path}')
        b=self.get(path)
        if b is None: raise ValueError('Missing '+path)
        seen.add(path); cache={}; tex=None; tw=th=32; origin=(0,0); tile=0; out=[]; lighting=False
        def tri(ids,offset):
            if len(set(ids))<3:return
            if not all(i in cache for i in ids):raise ValueError(f'Unloaded vertices {path} at {offset}')
            vs=np.stack([cache[i][0] for i in ids]); refs=[cache[i][1] for i in ids]
            out.append({'v':vs,'refs':refs,'texture':tex,'tile':(tw,th),'origin':origin,'path':path,'offset':offset,'lit':lighting})
        if b.startswith(b'<'):
            root=ET.fromstring(b);off=72
            for el in root:
                tag=el.tag; a=el.attrib
                if tag=='SetTextureImage':tex=a.get('Path',tex)
                elif tag=='SetTileSize' and int(a.get('T','0'))==tile:
                    tw=(int(a.get('Lrs','124'))-int(a.get('Uls','0')))/4+1
                    th=(int(a.get('Lrt','124'))-int(a.get('Ult','0')))/4+1
                    origin=(int(a.get('Uls','0'))/4,int(a.get('Ult','0'))/4)
                elif tag=='LoadVertices':
                    vp=a['Path']; arr=self.v(vp); n=int(a['Count']); start=int(a.get('VertexOffset','0'));dest=int(a.get('VertexBufferIndex','0'))
                    for i in range(n):cache[dest+i]=(arr[start+i],(vp,start+i))
                elif tag=='Triangle1':tri([int(a[f'V0{i}'])for i in range(3)],off)
                elif tag=='Triangle2':
                    tri([int(a[f'V0{i}'])for i in range(3)],off);tri([int(a[f'V1{i}'])for i in range(3)],off)
                elif tag=='CallDisplayList':out.extend(self.mesh(a['Path'],seen))
                # Native XML factories interpret geometry flags by attribute
                # presence, including attributes whose text happens to be "0".
                elif tag=='SetGeometryMode':lighting=lighting or 'G_LIGHTING' in a
                elif tag=='ClearGeometryMode' and 'G_LIGHTING' in a:lighting=False
                off+=24 if tag=='SetTextureImage' else 16 if tag in ['LoadVertices','CallDisplayList','BranchDisplayList','SetTextureImageHash'] else 8
        else:
            for off,op,a,c,h,size in commands(b):
                if op==32:
                    tex=self.hashes.get(h)
                    if tex is None:raise ValueError(f'Missing texture {h:016x} at {path}:{off}')
                elif op==50:
                    vp=self.hashes[h];arr=self.v(vp);n=(a>>12)&255;dest=((a>>1)&127)-n;start=c//16
                    for i in range(n):cache[dest+i]=(arr[start+i],(vp,start+i))
                elif op==5:tri([((a>>s)&255)//2 for s in [16,8,0]],off)
                elif op==6:
                    tri([((a>>s)&255)//2 for s in [16,8,0]],off);tri([((c>>s)&255)//2 for s in [16,8,0]],off)
                elif op in [49,53]:
                    if h not in self.hashes:raise ValueError(f'Missing call {h:016x} at {path}:{off}')
                    out.extend(self.mesh(self.hashes[h],seen))
                elif op==0xf2 and ((c>>24)&7)==tile:
                    uls=(a>>12)&4095;ult=a&4095;lrs=(c>>12)&4095;lrt=c&4095
                    tw=(lrs-uls)/4+1;th=(lrt-ult)/4+1;origin=(uls/4,ult/4)
                elif op==0xd9:lighting=bool(c&0x20000) or (lighting and bool(a&0x20000))
        seen.remove(path)
        return out

def render(resources,triangles,path,eye,target,width=1200,height=850,ortho_width=800,label=None):
    from PIL import ImageDraw,ImageFont
    eye=np.asarray(eye,float);target=np.asarray(target,float);forward=target-eye;forward/=np.linalg.norm(forward)
    right=np.cross(forward,[0,1,0]);right/=np.linalg.norm(right);up=np.cross(right,forward)
    scale=width/ortho_width
    bg=np.zeros((height,width,4),dtype=np.uint8);bg[:]=[24,29,33,255]
    depth=np.full((height,width),np.inf,dtype=np.float32)
    texcache={}
    for tr in triangles:
        v=tr['v'];p=v[:,:3];q=p-eye
        pp=np.column_stack([q@right*scale+width/2,height/2-q@up*scale]);zz=q@forward
        x0=max(0,int(np.floor(pp[:,0].min())));x1=min(width-1,int(np.ceil(pp[:,0].max())))
        y0=max(0,int(np.floor(pp[:,1].min())));y1=min(height-1,int(np.ceil(pp[:,1].max())))
        if x0>x1 or y0>y1:continue
        a,b,c=pp;den=(b[1]-c[1])*(a[0]-c[0])+(c[0]-b[0])*(a[1]-c[1])
        if abs(den)<1e-6:continue
        yy,xx=np.mgrid[y0:y1+1,x0:x1+1];xx=xx+.5;yy=yy+.5
        wa=((b[1]-c[1])*(xx-c[0])+(c[0]-b[0])*(yy-c[1]))/den
        wb=((c[1]-a[1])*(xx-c[0])+(a[0]-c[0])*(yy-c[1]))/den;wc=1-wa-wb
        dd=wa*zz[0]+wb*zz[1]+wc*zz[2];zpart=depth[y0:y1+1,x0:x1+1]
        valid=(wa>=-1e-5)&(wb>=-1e-5)&(wc>=-1e-5)&(dd<zpart)&(dd>0)
        if not valid.any():continue
        uv=v[:,3:5]/32
        u=(wa*uv[0,0]+wb*uv[1,0]+wc*uv[2,0]-tr['origin'][0])/tr['tile'][0]
        w=(wa*uv[0,1]+wb*uv[1,1]+wc*uv[2,1]-tr['origin'][1])/tr['tile'][1]
        tn=tr['texture']
        if tn:
            if tn not in texcache:texcache[tn]=np.asarray(resources.t(tn))
            tx=texcache[tn];th,tw=tx.shape[:2]
            ix=(np.floor(u*tw).astype(int)%tw);iy=(np.floor(w*th).astype(int)%th)
            pixels=tx[iy,ix].copy()
        else:pixels=np.zeros((*wa.shape,4),dtype=np.uint8);pixels[:]=[150,145,130,255]
        valid &= pixels[:,:,3]>80
        if not valid.any():continue
        if tr['lit']:
            n=np.cross(p[1]-p[0],p[2]-p[0]);n/=max(1e-8,np.linalg.norm(n))
            light=.68+.32*abs(np.dot(n,np.array([.3,.8,.5])/np.sqrt(.98)))
            col=np.full((*wa.shape,3),light)
        else:
            colors=v[:,5:8]/255
            col=wa[:,:,None]*colors[0]+wb[:,:,None]*colors[1]+wc[:,:,None]*colors[2]
            col=.45+.55*col
        pixels[:,:,:3]=np.clip(pixels[:,:,:3]*col,0,255).astype('uint8')
        out=bg[y0:y1+1,x0:x1+1];out[valid]=pixels[valid];zpart[valid]=dd[valid]
    im=Image.fromarray(bg)
    if label:
        d=ImageDraw.Draw(im);font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',20)
        d.rectangle((0,height-45,width,height),fill=(12,16,19,235));d.text((18,height-34),label,fill='white',font=font)
    im.save(path)
    return im
