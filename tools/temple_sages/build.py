#!/usr/bin/env python3
"""Reconstruct a dependency-closed Temple/Sages addon; never edit donor archives."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import sys
import xml.etree.ElementTree as ET
import zipfile

sys.path.insert(0,str(Path(__file__).resolve().parent))
import resource_geometry as G
import materials as Magic
import portals as Portals
import chamber_textures as HD

TEMPLE='scenes/shared/tokinoma_scene/'
SAGES='scenes/shared/kenjyanoma_scene/'
PRIVATE='custom/temple_sages/'
COLLISION=TEMPLE+'tokinoma_sceneCollisionHeader_0032F8'
CENTER=(0,-116,-2400)
FLOOR=-120
DOOR_LO=(-92,-2,-726)
DOOR_HI=(92,242,-714)
NAMES=('forest','fire','water','spirit','shadow','light')
COLORS=((105,255,145),(255,105,40),(90,180,255),(255,195,85),(190,105,255),(255,242,180))
DIRS=((-math.sqrt(3)/2,.5),(-math.sqrt(3)/2,-.5),(0,-1),
      (math.sqrt(3)/2,-.5),(math.sqrt(3)/2,.5),(0,1))


def string(s):
    b=s.encode();return struct.pack('<I',len(b))+b


def call_list(paths):
    tree=ET.Element('DisplayList',Version='0')
    for path in paths:ET.SubElement(tree,'CallDisplayList',Path=path)
    ET.SubElement(tree,'EndDisplayList');return G.xml(tree)


def row(p,s=0,t=0,color=(255,255,255,255)):
    return (*p,0,s,t,*color)


def upward(a,b,c):
    if (b[2]-a[2])*(c[0]-a[0])-(b[0]-a[0])*(c[2]-a[2])<0:b,c=c,b
    return a,b,c


def plane(a,b,width,uv_scale=8,color=(255,255,255,255)):
    dx=b[0]-a[0];dz=b[2]-a[2];length=math.hypot(dx,dz)
    ux=-dz/length*width/2;uz=dx/length*width/2
    v=[row((a[0]+ux,a[1],a[2]+uz),0,0,color),row((a[0]-ux,a[1],a[2]-uz),1024,0,color),
       row((b[0]-ux,b[1],b[2]-uz),1024,round(length*uv_scale),color),
       row((b[0]+ux,b[1],b[2]+uz),0,round(length*uv_scale),color)]
    return [upward(v[0],v[1],v[2]),upward(v[0],v[2],v[3])]


def quad(points,normal):
    """Triangulate one authored face with an explicit outward/inward normal."""
    a,b,c=points[:3]
    u=[b[i]-a[i] for i in range(3)];v=[c[i]-a[i] for i in range(3)]
    cross=(u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0])
    faces=[(points[0],points[1],points[2]),(points[0],points[2],points[3])]
    if sum(cross[i]*normal[i] for i in range(3))<0:
        faces=[(a,c,b) for a,b,c in faces]
    return faces


def bridge_slab(a,b,width,depth=18):
    """Keep the walk surface flat; taper and shade its visible magical sides."""
    dx=b[0]-a[0];dz=b[2]-a[2];length=math.hypot(dx,dz)
    nx=-dz/length;nz=dx/length
    tops=[];bottoms=[]
    for p,side in ((a,1),(a,-1),(b,-1),(b,1)):
        tops.append((p[0]+side*nx*width/2,p[1],p[2]+side*nz*width/2))
        bottoms.append((p[0]+side*nx*(width/2-4),p[1]-depth,p[2]+side*nz*(width/2-4)))
    faces=plane(a,b,width)
    center=((a[0]+b[0])/2,(a[1]+b[1])/2,(a[2]+b[2])/2)
    for i in range(4):
        j=(i+1)%4
        points=[row(tops[i],0,0),row(tops[j],round(length*8),0),
                row(bottoms[j],round(length*8),576,(70,70,70,255)),
                row(bottoms[i],0,576,(70,70,70,255))]
        normal=((tops[i][0]+tops[j][0])/2-center[0],0,
                (tops[i][2]+tops[j][2])/2-center[2])
        faces.extend(quad(points,normal))
    points=[row(p,(i%2)*1024,(i//2)*1024,(45,45,45,255)) for i,p in enumerate(bottoms)]
    faces.extend(quad(points,(0,-1,0)))
    return faces


def enclosure_faces():
    """Compact inward casing; its only opening surrounds the existing tunnel."""
    faces=[];radius=1200;bottom=-620;top=500
    for i in range(24):
        a=i*math.tau/24;b=(i+1)*math.tau/24
        x0=radius*math.sin(a);z0=CENTER[2]+radius*math.cos(a)
        x1=radius*math.sin(b);z1=CENTER[2]+radius*math.cos(b)
        points=[row((x0,bottom,z0),0,0),row((x1,bottom,z1),2048,0),
                row((x1,top,z1),2048,8192),row((x0,top,z0),0,8192)]
        for face in quad(points,(-(x0+x1)/2,0,CENTER[2]-(z0+z1)/2)):
            faces.extend(G.subtract_box(face,(-100,-125,-1220),(100,125,-1170)))
        for y,normal,shade in ((top,(0,-1,0),180),(bottom,(0,1,0),100)):
            p=[row((0,y,CENTER[2]),1024,1024,(shade,shade,shade,255)),
               row((x0,y,z0),round(x0*2),round((z0-CENTER[2])*2),(shade,shade,shade,255)),
               row((x1,y,z1),round(x1*2),round((z1-CENTER[2])*2),(shade,shade,shade,255))]
            u=[p[1][k]-p[0][k] for k in range(3)];v=[p[2][k]-p[0][k] for k in range(3)]
            if (u[2]*v[0]-u[0]*v[2])*normal[1]<0:p[1],p[2]=p[2],p[1]
            faces.append(tuple(p))
    return faces


def portal_resources():
    return Portals.resources(PRIVATE)


def feathered_veil():
    """Feather drifting light into the existing one doorway, with no hard rim."""
    rows=[]
    for j in range(13):
        y=4+j*228/12
        for i in range(9):
            x=-88+i*176/8
            fade=min(1,(x+88)/24,(88-x)/24)*min(1,(y-4)/32,(232-y)/32)
            rows.append(row((x,y,-718),round(i/8*1024),round((1-j/12)*1536),
                            (200,235,255,round(55*max(0,fade)))))
    faces=[]
    for j in range(12):
        for i in range(8):
            k=j*9+i;a,b,c,d=rows[k],rows[k+1],rows[k+10],rows[k+9]
            faces.extend(((a,b,c),(a,c,d)))
    return faces


def veil_texture():
    from PIL import Image
    art=Image.open(Path(__file__).with_name('assets')/'portal_flow.png')
    gray=art.convert('L').resize((512,512),Image.Resampling.LANCZOS)
    pixels=Image.merge('RGBA',(gray,gray,gray,gray)).tobytes()
    return G.header(0x4f544558,1)+struct.pack('<IIIIffI',1,512,512,1,32.,16.,len(pixels))+pixels


def source_texture(data):
    kind,w,h=struct.unpack_from('<3I',data,64)
    version=struct.unpack_from('<I',data,8)[0]
    if version:
        flags,sx,sy=struct.unpack_from('<Iff',data,76)
        return kind,round(w*(2 if flags&1 else 1)/sx),round(h/sy)
    return kind,w,h


def simple_material(texture,texture_bytes,translucent=False,caller_tint=False,scroll=None):
    kind,w,h=source_texture(texture_bytes)
    formats={1:('G_IM_FMT_RGBA','G_IM_SIZ_16b'),2:('G_IM_FMT_RGBA','G_IM_SIZ_16b'),9:('G_IM_FMT_IA','G_IM_SIZ_16b'),
             8:('G_IM_FMT_IA','G_IM_SIZ_8b'),5:('G_IM_FMT_I','G_IM_SIZ_4b'),6:('G_IM_FMT_I','G_IM_SIZ_8b')}
    if kind not in formats:raise ValueError('unsupported selected native texture format '+str(kind))
    fmt,size=formats[kind]
    bits=4 if size.endswith('4b') else 8 if size.endswith('8b') else 16
    line=max(1,w*bits//64)
    tree=ET.Element('DisplayList',Version='0');ET.SubElement(tree,'PipeSync')
    ET.SubElement(tree,'ClearGeometryMode',G_LIGHTING='1',G_CULL_BACK='1',G_FOG='1')
    ET.SubElement(tree,'SetGeometryMode',G_ZBUFFER='1',G_SHADE='1',G_SHADING_SMOOTH='1')
    ET.SubElement(tree,'SetOtherMode',Cmd='G_SETOTHERMODE_H',Sft='4',Length='20',
        G_AD_NOISE='1',G_CD_MAGICSQ='1',G_CK_NONE='1',G_TC_FILT='1',G_TF_BILERP='1',
        G_TL_TILE='1',G_TD_CLAMP='1',G_TP_PERSP='1',G_CYC_2CYCLE='1',G_PM_NPRIMITIVE='1')
    modes=dict(Cmd='G_SETOTHERMODE_L',Sft='0',Length='32',G_AC_NONE='1',G_ZS_PIXEL='1',G_RM_PASS='1')
    modes['G_RM_AA_ZB_XLU_SURF2' if translucent else 'G_RM_AA_ZB_OPA_SURF2']='1'
    ET.SubElement(tree,'SetOtherMode',modes)
    ET.SubElement(tree,'SetCombineLERP',A0='G_CCMUX_TEXEL0',B0='G_CCMUX_0',C0='G_CCMUX_PRIMITIVE' if caller_tint else 'G_CCMUX_SHADE',D0='G_CCMUX_0',
        Aa0='G_ACMUX_TEXEL0' if translucent else 'G_ACMUX_0',Ab0='G_ACMUX_0',Ac0='G_ACMUX_PRIMITIVE' if caller_tint else 'G_ACMUX_SHADE',Ad0='G_ACMUX_0' if translucent else 'G_ACMUX_1',
        A1='G_CCMUX_0',B1='G_CCMUX_0',C1='G_CCMUX_0',D1='G_CCMUX_COMBINED',
        Aa1='G_ACMUX_0',Ab1='G_ACMUX_0',Ac1='G_ACMUX_0',Ad1='G_ACMUX_COMBINED')
    ET.SubElement(tree,'Texture',S='65535',T='65535',Level='0',Tile='0',On='1')
    ET.SubElement(tree,'SetTextureLUT',Mode='G_TT_NONE');ET.SubElement(tree,'TileSync')
    ET.SubElement(tree,'SetTextureImage',Path=texture,Format=fmt,Size='G_IM_SIZ_16b_LOAD_BLOCK',Width='1')
    masks=(round(math.log2(w)),round(math.log2(h)))
    desc=dict(Format=fmt,Size='G_IM_SIZ_16b_LOAD_BLOCK',Line='0',TMem='0',Tile='7',Palette='0',
        Cms0='G_TX_WRAP',Cms1='G_TX_NOMIRROR',Cmt0='G_TX_WRAP',Cmt1='G_TX_NOMIRROR',MaskS=str(masks[0]),ShiftS='0',MaskT=str(masks[1]),ShiftT='0')
    ET.SubElement(tree,'SetTile',desc);ET.SubElement(tree,'LoadSync')
    ET.SubElement(tree,'LoadBlock',Tile='7',Uls='0',Ult='0',Lrs=str(w*h*bits//16-1),Dxt=str(math.ceil(2048/line)))
    ET.SubElement(tree,'PipeSync');desc.update(Size=size,Line=str(line),Tile='0');ET.SubElement(tree,'SetTile',desc)
    ET.SubElement(tree,'SetTileSize',T='0',Uls='0',Ult='0',Lrs=str((w-1)*4),Lrt=str((h-1)*4))
    if scroll:ET.SubElement(tree,'ScrollTexture',Tile='0',SpeedX=str(scroll[0]),SpeedY=str(scroll[1]))
    if not caller_tint:ET.SubElement(tree,'SetPrimColor',M='0',L='0',R='255',G='255',B='255',A='255')
    ET.SubElement(tree,'EndDisplayList');return G.xml(tree)


SCROLLS={0x0a:((0,127,0,32,32,-1,1),(1,0,0,32,32,1,1)),
         0x08:((0,0,0,64,64,0,2),),
         0x09:((0,127,0,32,64,-1,1),(1,0,0,32,128,0,0))}


def portable(data):
    if data.lstrip().startswith(b'<'):
        tree=ET.fromstring(data)
        for index,e in reversed(list(enumerate(list(tree)))):
            path=e.get('Path','')
            if e.tag!='CallDisplayList' or not path.startswith('>'):continue
            segment=(int(path[1:],16)>>24)&255
            if segment not in SCROLLS:raise ValueError('unexpected donor segment '+path)
            tree.remove(e)
            for tile,x,y,w,h,dx,dy in reversed(SCROLLS[segment]):
                scroll=ET.Element('ScrollTexture',Tile=str(tile),SpeedX=str(dx),SpeedY=str(dy))
                size=ET.Element('SetTileSize',T=str(tile),Uls=str(x),Ult=str(y),Lrs=str(x+(w-1)*4),Lrt=str(y+(h-1)*4))
                tree.insert(index,scroll);tree.insert(index,size)
        return G.xml(tree)
    if data[4:8]!=b'TLDO':return data
    edits=[]
    for off,a,b,h in G.words(data):
        if a>>24!=0xde:continue
        segment=(b>>24)&255
        if segment not in SCROLLS:raise ValueError('unexpected donor segment '+hex(b))
        commands=b''
        for tile,x,y,w,hh,dx,dy in SCROLLS[segment]:
            commands+=struct.pack('<2I',0xf2000000|(x<<12)|y,(tile<<24)|((x+(w-1)*4)<<12)|(y+(hh-1)*4))
            commands+=struct.pack('<2I',0x4b000000|tile,((dx&0xffff)<<16)|(dy&0xffff))
        edits.append((off,commands))
    for off,commands in reversed(edits):data=data[:off]+commands+data[off+8:]
    return data


def scoped(resources,namespace,transform):
    mapping={name:namespace+name.rsplit('/',1)[-1] for name in resources}
    assert len(set(mapping.values()))==len(mapping),'private resource basename collision'
    result={}
    for name,data in resources.items():
        if data.lstrip().startswith(b'<'):
            kind=ET.fromstring(data).tag
            if kind=='Vertex':data=G.transform_vertices(data,transform)
            elif kind=='DisplayList':data=portable(data)
        elif data[4:8]==b'RRAO' and struct.unpack_from('<I',data,64)[0]==25:
            data=G.transform_vertices(data,transform)
        elif data[4:8]==b'TLDO':data=portable(data)
        result[mapping[name]]=G.rewrite(data,mapping)
    return result,mapping


def render_resources(resources):
    result={}
    for n,data in resources.items():
        if data.lstrip().startswith(b'<'):
            if ET.fromstring(data).tag in ('DisplayList','Vertex'):result[n]=data
        elif data[4:8] in (b'TLDO',b'XETO') or data[4:8]==b'RRAO' and struct.unpack_from('<I',data,64)[0]==25:
            result[n]=data
    return result


def chamber_transform(p):
    return (CENTER[0]+p[2],CENTER[1]+p[1],CENTER[2]-p[0])


def sample_line(a,b,step=14):
    n=max(1,math.ceil(math.dist(a,b)/step))
    return [[round(a[j]+(b[j]-a[j])*i/n,4) for j in range(3)] for i in range(n+1)]


def glb_tunnel(path):
    data=path.read_bytes();size=struct.unpack_from('<I',data,12)[0];gltf=json.loads(data[20:20+size]);binary=data[28+size:]
    def accessor(index):
        a=gltf['accessors'][index];view=gltf['bufferViews'][a['bufferView']]
        off=view.get('byteOffset',0)+a.get('byteOffset',0)
        size={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4}[a['type']]
        fmt={5126:'f',5123:'H',5125:'I',5121:'B'}[a['componentType']]
        stride=view.get('byteStride',struct.calcsize('<'+fmt*size))
        return [struct.unpack_from('<'+fmt*size,binary,off+i*stride) for i in range(a['count'])]
    # Primitive 0 is the donor's black end cap, not a traversable tunnel surface.
    primitive=gltf['meshes'][0]['primitives'][1];attrs=primitive['attributes']
    positions=accessor(attrs['POSITION']);uv=accessor(attrs['TEXCOORD_0'])
    colors=accessor(attrs['COLOR_0']) if 'COLOR_0' in attrs else [(1,1,1,1)]*len(positions)
    rows=[]
    for p,t,c in zip(positions,uv,colors):
        point=(1.5*(p[0]+1),96+p[1]*1.5,-717+1.5*(p[2]-190))
        shade=max(120,min(255,round(sum(c[:3])/3*255)))
        rows.append(row(point,round(t[0]*1024),round(t[1]*1024),(shade,shade,shade,255)))
    indices=[i[0] for i in accessor(primitive['indices'])] if 'indices' in primitive else list(range(len(rows)))
    faces=[tuple(rows[indices[j+i]] for i in range(3)) for j in range(0,len(indices),3)]
    faces.extend(plane((0,0,-714),(0,0,-718.5),180))
    return faces


def tunnel_atlas(path):
    from PIL import Image
    # Keep the source atlas UV layout; normalize authored art to a GPU upload.
    bitmap=Image.open(Path(__file__).with_name('assets')/'ethereal_tunnel.png').convert('RGBA')
    bitmap=bitmap.resize((1024,1024),Image.Resampling.LANCZOS)
    pixels=bitmap.tobytes()
    return G.header(0x4f544558,1)+struct.pack('<IIIIffI',1,1024,1024,1,64.,32.,len(pixels))+pixels


def room_headers(native,markers):
    result={};report=[]
    main=TEMPLE+'tokinoma_room_0'
    # All native alternate room setup headers preserve their original commands/actors.
    names=[main]+[n for n,v in native.items() if n.startswith(main+'Set_') and v[4:8]==b'MORO']
    actor_data=b''.join(struct.pack('<HhhhhhhH',0x012e,*m['position'],0,m['yaw'],0,m['params']) for m in markers)
    for name in names:
        data=native[name];replacements=[];before=None
        for cmd,start,end,info in G.room_commands(data):
            if cmd==1:
                before=info['actors'];count=len(before)//16
                replacements.append((start,end,struct.pack('<II',1,count+len(markers))+before+actor_data))
            elif cmd==0x0a:
                assert info['meshes']==[TEMPLE+'tokinoma_room_0DL_008128']
                mesh=struct.pack('<I4B',0x0a,0,0,1,0)+string(TEMPLE+'tokinoma_room_0DL_008128')+string(PRIVATE+'annex_xlu')
                replacements.append((start,end,mesh))
        assert before is not None
        for start,end,value in reversed(replacements):data=data[:start]+value+data[end:]
        result[name]=data;result['alt/'+name]=data
        report.append(dict(path=name,marker_params=[m['params'] for m in markers],
                           source_actor_bytes=before.hex(),preserved_actor_bytes=before.hex()))
    return result,report


def build(base_path,maps_path,tunnel_path,output_path):
    native,native_names=G.archive(base_path,(TEMPLE,SAGES))
    alt,_=G.archive(maps_path,('alt/'+TEMPLE,'alt/'+SAGES))
    alt={n[4:]:b for n,b in alt.items()}
    output=Magic.bridge_resources(PRIVATE);physical=[];guards=[];portable_materials=[];host_roots=[TEMPLE+'tokinoma_room_0DL_008128']
    portable_materials.extend(PRIVATE+'bridge_material_'+name for name in NAMES)
    markers=[];landings=[]
    for index,(dx,dz) in enumerate(DIRS[:5]):
        p=[round(CENTER[0]+dx*900),FLOOR,round(CENTER[2]+dz*900)]
        landings.append(p)
        yaw=round(math.atan2(-dx,-dz)*65536/math.tau)
        yaw=(yaw+32768)%65536-32768
        markers.append(dict(index=index,actor_id=0x012e,params=0xfc00|index,position=list(p),yaw=yaw))
    # Preserve original host geometry/state; only aperture primitives are replaced.
    for is_alt,source in ((True,alt),):
        host=render_resources({n:b for n,b in source.items() if n.startswith(TEMPLE)})
        roots=host_roots[:]
        if is_alt:
            roots.extend(n for n in host if '_tri_' not in n and '_vtx_' not in n and '/mat_' not in n and host[n].lstrip().startswith(b'<') and ET.fromstring(host[n]).tag=='DisplayList')
        clipped,count=G.aperture(host,roots,PRIVATE+('alt_host' if is_alt else 'native_host'),DOOR_LO,DOOR_HI)
        assert count>0,'rear doorway failed to reach host geometry'
        prefix='alt/' if is_alt else ''
        for n,b in clipped.items():output[prefix+n]=b
        host_copy=PRIVATE+'host_opa'
        output[prefix+host_copy]=source[host_roots[0]]
        output[prefix+host_roots[0]]=call_list([host_copy,PRIVATE+'annex_opa'])
        # The private host root references the same narrowly patched native leaves.
    # Keep only the center. The giant original curtain reaches into the Temple;
    # replace it with a compact shell after the source center is copied.
    source_roots=[SAGES+'kenjyanoma_room_0DL_001608',SAGES+'kenjyanoma_room_0DL_010CD8']
    for is_alt,source in ((True,alt),):
        prefix='alt/' if is_alt else ''
        chamber=render_resources({n:b for n,b in source.items() if n.startswith(SAGES)})
        central=G.replace_faces(chamber,source_roots,PRIVATE+'filter',lambda rec:[f for f in rec['faces']
            if max(math.hypot(v[0],v[2]) for v in f)<400])
        copied,mapping=scoped(central,PRIVATE+'chamber/',chamber_transform)
        output.update({prefix+n:b for n,b in copied.items()})
        portable_materials.extend(prefix+n for n,b in copied.items() if (b.lstrip().startswith(b'<') and b'ScrollTexture' in b) or b[4:8]==b'TLDO' and any(a>>24==0x4b for _,a,_,_ in G.words(b)))
        opaque=[mapping[source_roots[0]]];xlu=[mapping[source_roots[1]]]
        # Paste one actual outer pillar five times; Light is the Temple approach.
        proto=[]
        for source_root in source_roots:
            for rec in G.collect(chamber,source_root):
                tex=rec['texture']
                if is_alt and 'water_04' in tex:continue # rising beam would obscure the pad
                if not is_alt and not tex.endswith(('Tex_00D618','Tex_012D08')):continue
                faces=[f for f in rec['faces'] if all(750<v[0]<1300 and abs(v[2])<250 for v in f)]
                if faces:proto.append((tex,faces))
        assert proto
        grouped={}
        for tex,faces in proto:grouped.setdefault(tex,[]).extend(faces)
        for index,p in enumerate(landings):
            for group,(tex,faces) in enumerate(grouped.items()):
                transformed=[]
                for f in faces:
                    v=[]
                    for point in f:
                        local_y=(point[1]-316)*.2 if is_alt and point[1]>=250 else ((250-316)*.2+point[1]-250 if is_alt else point[1]-344)
                        v.append((p[0]+point[0]-1021,FLOOR+local_y,p[2]+point[2]+5,*point[3:]))
                    transformed.append(tuple(v))
                path=PRIVATE+f'pillar_{index}_{group}'
                texpath=PRIVATE+'pillar_texture_'+str(group)
                output[prefix+texpath]=chamber[tex]
                mat=path+'_material';output[prefix+mat]=simple_material(texpath,chamber[tex],scroll=(0,2) if not is_alt and tex.endswith('012D08') else None)
                output.update({prefix+n:b for n,b in G.geometry(transformed,path+'_geometry').items()})
                output[prefix+path]=call_list([mat,path+'_geometry']);opaque.append(path)
            # One collision floor per copied source pillar, not a translucent visual proxy.
            points=[row((p[0]+math.cos(i*math.pi/3)*200,FLOOR,p[2]+math.sin(i*math.pi/3)*200)) for i in range(6)]
            for i in range(1,5):physical.append(upward(points[0],points[i],points[i+1]))
        # The same authored tunnel geometry is rematerialed with native/Alt stone respectively.
        faces=glb_tunnel(tunnel_path)
        stone=tunnel_atlas(tunnel_path)
        stonepath=PRIVATE+'texture/tunnel_stone';output[prefix+stonepath]=stone
        mat=PRIVATE+'tunnel_material';output[prefix+mat]=simple_material(stonepath,stone)
        output.update({prefix+n:b for n,b in G.geometry(faces,PRIVATE+'stone_tunnel').items()})
        opaque.extend([mat,PRIVATE+'stone_tunnel'])
        # Soft two-sided veil, inside the one authored Temple doorway.
        veil=PRIVATE+'door_veil';tex=PRIVATE+'texture/veil'
        output[prefix+tex]=veil_texture()
        output[prefix+veil+'_material']=simple_material(tex,output[prefix+tex],translucent=True,scroll=(-1,2))
        output.update({prefix+n:b for n,b in G.geometry(feathered_veil(),veil+'_geometry').items()})
        output[prefix+veil]=call_list([veil+'_material',veil+'_geometry']);xlu.append(veil)
        portable_materials.append(prefix+veil+'_material')
        output.update({prefix+n:b for n,b in G.geometry(enclosure_faces(),PRIVATE+'enclosure_geometry').items()})
        output[prefix+PRIVATE+'enclosure']=call_list([PRIVATE+'shell_material',PRIVATE+'enclosure_geometry'])
        output[prefix+PRIVATE+'annex_opa']=call_list([PRIVATE+'enclosure']+opaque+[PRIVATE+'bridges'])
        output[prefix+PRIVATE+'annex_xlu']=call_list(xlu)
    # Collision comes from the authored tunnel surfaces, with no synthetic floor
    # through its ceiling. The source floor has flat/ramp/flat stages.
    tunnel=glb_tunnel(tunnel_path);physical.extend(tunnel)
    floor_start=(0,0,-718.5);floor_end=(0,FLOOR,-1288.5)
    bridge_paths=[];walk_samples={}
    for index,(dx,dz) in enumerate(DIRS):
        a=(CENTER[0]+dx*238,FLOOR,CENTER[2]+dz*238)
        b=(CENTER[0]+dx*900,FLOOR,CENTER[2]+dz*900)
        width=120
        if index==5:
            b=floor_end;width=170
        physical.extend(plane(a,b,width))
        guards.extend(G.bridge_guard_faces(a,b,width))
        path=PRIVATE+'bridge_'+NAMES[index]
        output.update(G.geometry(bridge_slab(a,b,width),path+'_geometry'))
        output[path]=call_list([PRIVATE+'bridge_material_'+NAMES[index],path+'_geometry'])
        bridge_paths.append(path)
        walk_samples[NAMES[index]]=sample_line(a,b)
    output[PRIVATE+'bridges']=call_list(bridge_paths)
    physical.extend(G.central_platform_faces(native[SAGES+'kenjyanoma_sceneCollisionHeader_00359C'],
        chamber_transform,CENTER,DIRS,FLOOR,widths=(120,120,120,120,120,170)))
    output.update(portal_resources())
    output=HD.apply(output)
    headers,actor_report=room_headers(native,markers);output.update(headers)
    col=G.append_collision(native[COLLISION],physical,DOOR_LO,DOOR_HI,camera_ignore_faces=guards)
    output[COLLISION]=col;output['alt/'+COLLISION]=col
    stages=[(0,0,-600),(0,0,-718.5),(0,0,-808.5),(0,-60,-928.5),(0,FLOOR,-1048.5),floor_end,(0,FLOOR,-1500),(0,FLOOR,-2162)]
    walks=[]
    for a,b in zip(stages,stages[1:]):walks.extend(sample_line(a,b)[bool(walks):])
    walk_samples['entry']=walks
    metadata={'version':1,'edits':{}}
    output['prelude/project/edits.json']=json.dumps(metadata,separators=(',',':'),sort_keys=True).encode()
    output['prelude/project/blobs.bin']=b''
    manifest=dict(version=3,materials_parent='f5600608f721c41a20dd07adac80b0c78606d9cc',poc1_parent='4bcddf1fa68b7b6fa2a190c1f3910638027a7d87',code_parent='fa81f7926038f2aa85c7f3e73c96fec043168b66',room=0,
        route=['temple_pedestal','stone_tunnel','light_bridge','central_platform'],extra_platform_doorways=0,
        center=CENTER,doorway_center=[0,100,-720],landings=landings,markers=markers,
        bridge_depth=18,bridge_guards=dict(height=600,camera_ignore=True,endpoints_open=True),
        enclosure=dict(radius=1200,bottom=-620,ceiling=500,entry_count=1),
        walk_samples=walk_samples,room_actor_headers=actor_report,portable_materials=sorted(set(portable_materials)),
        donor_sha256={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (base_path,maps_path,tunnel_path)},
        hd_texture_source=json.loads((HD.ASSETS/'provenance.json').read_text())['source'],
        authored_art_sha256={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (Path(__file__).with_name('assets')/'portal_flow.png',Path(__file__).with_name('assets')/'ethereal_tunnel.png')},
        portal_texture_paths={name:[PRIVATE+'portals/'+name+'/texture_flow',PRIVATE+'portals/'+name+'/texture_detail'] for name in NAMES[:5]},
        resources={n:hashlib.sha256(b).hexdigest() for n,b in sorted(output.items())},runtime_verified=False)
    output[PRIVATE+'manifest.json']=json.dumps(manifest,sort_keys=True,separators=(',',':')).encode()
    output_path.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(output_path,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as pack:
        for n,b in sorted(output.items()):
            info=zipfile.ZipInfo(n,(2026,10,26,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED;info.external_attr=0o100644<<16
            pack.writestr(info,b,compress_type=zipfile.ZIP_DEFLATED,compresslevel=9)
    output_path.with_suffix('.manifest.json').write_text(json.dumps(manifest,indent=2,sort_keys=True))
    return manifest


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--base',type=Path,required=True);p.add_argument('--maps',type=Path,required=True)
    p.add_argument('--tunnel',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();m=build(a.base,a.maps,a.tunnel,a.output)
    print(json.dumps({'output':str(a.output),'resources':len(m['resources']),'markers':len(m['markers']),'runtime_verified':False}))
