"""Reviewed forged swords with user-requested get-item silhouette corrections.

The original recipes and approved GLBs remain preserved in the import bundle.
Updated part dimensions are visual review targets, not measured Nintendo GI
vertices. This module does not install assets or modify runtime rendering code.
"""
import math
import numpy as np
from meshkit import Model, unit, bezier, rotation, earclip, TAU


def rgb(value):
    return np.array([int(value[i:i+2],16)/255 for i in (1,3,5)])


def metal_map(color,size=256,polish=1):
    v,u=np.mgrid[:size,:size]/(size-1)
    broad=np.exp(-((u+.12*v-.41)/.16)**2)
    key=np.exp(-((u+.08*v-.635)/.045)**2)
    rim=np.exp(-((u-.29*v-.64)/.055)**2)
    dark=np.exp(-((u+.13*v-.48)/.034)**2)
    env=np.clip(.63+.31*broad-.16*dark+.12*v,.43,1)
    color=np.asarray(color)
    base=color[None,None,:]*255*env[...,None]
    shine=(key*.81+rim*.40)[...,None]*(color*.48+.52)*255*polish
    grain=.8*np.sin(u*math.tau*105)+.3*np.sin(u*math.tau*43+v*math.tau*2)
    out=np.uint8(np.clip(base+shine+grain[...,None],0,255))
    return np.concatenate([out,np.full((size,size,1),255,np.uint8)],2)


def metal(m,name,color,polish=1):
    color=rgb(color) if isinstance(color,str) else np.array(color)
    m.material(name,color,metal=.95,rough=.22,reflection=1)
    m.materials[name]['tex']=metal_map(color,polish=polish)
    m.materials[name]['source_hex']='#'+''.join(f'{round(c*255):02X}' for c in color)
    return name


def material_set(m):
    for n,c,p in [('steel','#AABDD0',1),('edge','#E5EDF3',1.05),
                  ('gold','#E0A800',1),('gold_edge','#F2D17D',1),
                  ('gold_recess','#7F561E',.5),('violet','#62539F',.82),
                  ('violet_edge','#9382C4',.9),('rose','#C35D7A',1),
                  ('rose_edge','#F0AAC0',1),('jade','#246F4D',.7),
                  ('jade_edge','#79BE84',.85),('white','#F8F8F8',.85),
                  ('blue_gem','#4870D0',.95)]:
        metal(m,n,c,p)
    m.material('crease',rgb('#30273F'),rough=.8)
    m.material('blue_leather',rgb('#28244E'),'leather',rough=.76)
    m.material('blue_wrap',rgb('#645D8C'),'leather',rough=.7)
    m.material('white_leather',rgb('#F8F8F8'),'leather',rough=.76)
    m.material('white_wrap',rgb('#D8D7DC'),'leather',rough=.7)
    m.material('red_leather',rgb('#8C282F'),'leather',rough=.76)
    m.material('green_leather',rgb('#315C39'),'leather',rough=.78)
    tex=m.materials['white']['tex'].copy()
    tex[:,:,:3]=np.maximum(tex[:,:,:3],np.array([220,220,224]))
    m.materials['white']['tex']=tex


def model(slug,name):
    m=Model(slug,name,'objects/nei_gi_redesign/'+slug+'/gi_dl',1.,.66)
    material_set(m)
    m.markers.update(visual_review_candidate=True,runtime_tested=False,
                     preview_front_angle=16,preview_back_angle=164)
    return m


def poly(m,name,mat,xy,z=0,depth=1,bevel=.35):
    # Clockwise extrusion outlines keep mirrored bevel faces pointing outward.
    xy=np.asarray(xy,float)
    signed_area=np.sum(xy[:,0]*np.roll(xy[:,1],-1)-xy[:,1]*np.roll(xy[:,0],-1))
    if signed_area>0:xy=xy[::-1]
    m.polygon(name,mat,xy,depth=depth,z=z,bevel=min(bevel,depth*.8))


def stroke(m,name,mat,xy,z,r=.18,sides=5):
    m.tube(name,mat,np.c_[xy,np.full(len(xy),z)],r,sides)


def blade(m,rings,face='steel',edge='edge',thick=1.25,name='Forged blade'):
    fractions=np.array([-1,-.83,-.70,-.18,-.07,0,.07,.18,.70,.83,1])
    profile=np.array([0,.55,.93,1.02,.94,.90,.94,1.02,.93,.55,0])*thick
    for side in [-1,1]:
        for j in range(len(fractions)-1):
            points=[];normals=[];uv=[];tri=[]
            for i,ring in enumerate(rings):
                y,w=ring[:2];offset=ring[2] if len(ring)>2 else 0
                w=max(w,.04)
                for k in [j,j+1]:
                    z=profile[k]*min(1,w/.9)*side
                    points.append([fractions[k]*w+offset,y,z])
                    dz=np.gradient(profile,fractions*w)[k]
                    normals.append(unit([-dz,0,side]))
                    uv.append([(fractions[k]+1)/2,y/30])
                if i:
                    a=(i-1)*2;tri.extend([[a,a+1,a+2],[a+1,a+3,a+2]])
            m.add(name+' cutting bevel' if j in [0,1,8,9] else name+' fuller face',
                  edge if j in [0,9] else face,points,normals,uv,tri)


def grip(m,y0,y1,radius,body='blue_leather',wrap='blue_wrap',turns=8):
    m.lathe('Shaped oval leather grip',body,[(y0,radius*.78),(y0+1,radius),
           ((y0+y1)/2,radius*.96),(y1-1,radius),(y1,radius*.82)],12)
    t=np.linspace(0,math.tau*turns,turns*10+1)
    y=np.linspace(y0+.8,y1-.8,len(t))
    path=np.c_[np.cos(t)*(radius+.10),y,np.sin(t)*(radius+.10)]
    m.tube('Raised diagonal leather wrap',wrap,path,.24,5)


def gem(m,mat,x,y,z,rx,ry):
    m.sphere('Faceted inset gemstone',mat,[x,y,z],[rx,ry,.72],4,8,flat=True)


def triforce(m,y,z,mat,scale=1):
    for x,yy in [(0,y+2.2*scale),(-1.4*scale,y),(1.4*scale,y)]:
        pts=np.array([(x,yy+1.8*scale),(x-1.5*scale,yy-.8*scale),(x+1.5*scale,yy-.8*scale)])
        poly(m,'Recessed Triforce engraving',mat,pts,z,.18,.065)


def master_sword(true=False):
    slug='true_master_sword' if true else 'master_sword'
    m=model(slug,'True Master Sword' if true else 'Master Sword')
    m.markers.update(guard_baseline_y=0,original_gi_vertex_data_available=False,
                     proportion_reference='User-provided steel/gold sword images; component ratios are visual targets, not original GI vertex measurements.')
    face='gold' if true else 'steel';edge='gold_edge' if true else 'edge'
    hilt='white' if true else 'violet';hilt_edge='white' if true else 'violet_edge'
    if true:
        # The gold reference has its own stepped blade and capped bar guard.
        # Its longer grip preserves the supplied blade/hilt ratio while keeping
        # the approved white fittings and blue jewel.
        blade(m,[(-7,2.1),(1,2.1),(5,3.35),(10,3.35),(14,5.35),
                 (19,5.35),(62,4.65),(86,3.65),(96,1.75),(103,.04)],face,edge,1.40)
        poly(m,'True Master straight capped crossbar',hilt,
             [(-17.4,-1.7),(17.4,-1.7),(17.4,1.7),(-17.4,1.7)],0,3.6,.65)
        for sign in [-1,1]:
            m.sphere('True Master rounded crossbar end cap',hilt_edge,
                     [sign*17.4,0,0],[1.6,1.95,1.95],6,10)
            for side in [-1,1]:
                stroke(m,'True Master bar polished ridge',hilt_edge,
                       [[sign*4.0,.6],[sign*16.7,.6]],side*1.86,.22,6)
    else:
        # Long parallel edges replace the compact swelling of the last pass.
        # The latest review widens the connecting blade root to 72% of the body.
        blade(m,[(-7,3.6),(1,3.6),(14.5,3.6),(17,3.85),(19,4.9),
                 (23,5.0),(90,4.8),(94,4.75),(103,2.7),(110,.04)],face,edge,1.35)
        wing=np.array([(0,1),(3.8,4),(8.9,4.6),(15.2,8.8),(16.4,10.4),
                       (15.85,4.0),(13.35,1.7),(8.9,.5),(6.25,-3.8),(3,-5.1),(0,-3.4)])
        for sign in [-1,1]:
            poly(m,'Sculpted winged crossguard',hilt,wing*[sign,1],0,3.6,1)
            for side in [-1,1]:
                path=bezier([3*sign,.0],[7.1*sign,3.4],[12.3*sign,3.0],[15.2*sign,7.0],18)
                stroke(m,'Wing ridge polished bevel',hilt_edge,path,side*1.85,.28,6)
                for k in range(3):
                    xy=np.array([(4.5+k*2.65,1.1),(7.1+k*2.3,2.9),(8.9+k*2.15,4.1)])*[sign,1]
                    stroke(m,'Cut feather channel','crease',xy,side*1.91,.13,4)
    poly(m,'Crossguard central faceted mount',hilt_edge,[(0,5),(4.4,1),(3.0,-4.2),(0,-6),(-3,-4.2),(-4.4,1)],0,5,.8)
    for side in [-1,1]:
        poly(m,'Gem recessed bezel','steel' if true else 'gold_recess',[(0,3.0),(2.6,.0),(0,-3.2),(-2.6,0)],side*2.65,.35,.1)
        gem(m,'blue_gem' if true else 'gold_edge',0,-.2,side*2.95,1.75,2.35)
        triforce(m,23 if true else 27,side*(1.43 if true else 1.38),
                 'gold_recess' if true else 'gold',.82)
    grip_end=-30 if true else -24
    grip(m,grip_end,-6,2.5,'white_leather' if true else 'blue_leather',
         'white_wrap' if true else 'blue_wrap',9 if true else 7)
    m.lathe('Upper grip forged collar',hilt,[(-7,2.65),(-6,2.85),(-4.7,2.75)],12)
    m.lathe('Stepped pommel socket',hilt_edge,
            [(grip_end-2,2.5),(grip_end-.5,3.0),(grip_end+.4,2.55)],12)
    rear=-37 if true else -30
    m.lathe('Faceted sword pommel',hilt,
            [(rear,.4),(rear+1.3,2.3),(grip_end-3.5,3.1),(grip_end-2,2.5)],8)
    m.markers.update(reference_blade_to_hilt_ratio=103/37 if true else 110/30,
                     reference_blade_tip_y=103 if true else 110,
                     reference_hilt_rear_y=rear)
    m.notes=['Gold reference: long stepped gold blade and straight rounded-cap guard; longer white grip and blue inset gem.' if true else
             'Steel reference with the latest reviewed blade-root width: long nearly parallel blade, broad connecting ricasso, stepped shoulders and violet winged guard.',
             'Reflection bands use the game-supported G_TEXTURE_GEN material, not an image overlay.',
             'Level-four pigments: blade #E0A800, white hilt/grip #F8F8F8, gem #4870D0.' if true else 'Steel blade, blue-violet metal hilt and leather grip, golden central gem.',
             'Reconstructed from the distinct user-provided steel/gold silhouettes; these models deliberately have different blade and hilt proportions.',
             'Part dimensions are visual review targets, not measured Nintendo get-item coordinates.']
    return m


def gilded_sword():
    m=model('gilded_sword','Gilded Sword')
    m.markers.update(guard_baseline_y=0,original_gi_vertex_data_available=False,
                     proportion_reference='User-provided Gilded Sword image with the subsequent user-requested shorter blade; dimensions are visual review targets, not original GI vertex measurements.',
                     reference_blade_to_hilt_ratio=100/27,reference_blade_tip_y=100,
                     reference_hilt_rear_y=-27)
    blade(m,[(-7,2.1),(-3,2.1),(3,3.7),(6.125,5.4)],thick=1.30,
          name='Continuous Gilded blade tang and seat')
    for side in [-1,1]:
        for lo,hi in [(6,46.25),(46.25,86.5),(86.5,127)]:
            rows=[]
            for y in np.linspace(lo,hi,9):
                width=np.interp(y,[6,46.25,86.5,108,119,127],[5.4,5.2,4.9,4.6,2.7,.04])
                d=.90*(1-abs(y-(lo+hi)/2)/((hi-lo)/2))
                # Keep every strip ordered even at the broad diamond centre.
                fractions=np.array([-1,-.985,-.955,-d-.018,-d,0,d,d+.018,.955,.985,1])
                points=[]
                for k,f in enumerate(fractions):
                    h=np.interp(abs(f),[0,.70,.82,1],[1.35,1.15,.65,0])
                    if 4<=k<=6:h-=.11
                    if k in [3,7]:h-=.16
                    # Shorten only the patterned segment, retaining its width,
                    # thickness and three diamonds; the blade seat stays fixed.
                    shortened_y=6+(y-6)*94/121
                    points.append([f*width,shortened_y,side*h*min(1,width/.7)])
                rows.append(points)
            rows=np.array(rows)
            for j in range(10):
                mat='edge' if j in [0,9] else 'gold' if j in [4,5] else 'gold_recess' if j in [3,6] else 'steel'
                p=rows[:,[j,j+1]].reshape(-1,3);tri=[]
                for i in range(8):
                    a=2*i;tri.extend([[a,a+1,a+2],[a+1,a+3,a+2]])
                m.add('Tempered gold fitted diamond' if mat=='gold' else 'Forged blade inlay surround',mat,p,
                      np.tile([0,0,side],(len(p),1)),np.c_[p[:,0]/10+.5,p[:,1]/26],tri,flat=True)
    for sign in [-1,1]:
        wing=np.array([(0,1),(4.5,1.0),(8.5,-1),(9.4,5.1),(12.9,10.2),(15,12.4),
                       (13.8,7.1),(11.3,3.7),(11.3,-3.8),(5.5,-2.5),(0,-3)])*[sign,1]
        poly(m,'Forged silver forked quillon','steel',wing,0,3.7,.85)
        for side in [-1,1]:
            stroke(m,'Quillon edge catching light','edge',
                   np.array([(1,1.0),(5.4,.3),(9.1,-1.1),(10.3,4.8),(13.9,10.3)])*[sign,1],side*1.9,.30,6)
            stroke(m,'Quillon recessed dark channel','crease',
                   np.array([(3.2,-1.2),(8.7,-2.0),(10.2,3.4),(12.8,7.4)])*[sign,1],side*1.91,.20,5)
    poly(m,'Chased silver crossguard centre','steel',[(0,5),(5,2),(4,-2),(0,-5),(-4,-2),(-5,2)],0,4.7,.75)
    for side in [-1,1]:
        for sign in [-1,1]:
            stroke(m,'Crossguard engraved chevron','crease',
                   np.array([(0,3),(3.1,.8),(0,-2.8)])*[sign,1],side*2.43,.16,4)
    grip(m,-20,-5,2.25,'red_leather','red_leather',6)
    for side in [-1,1]:
        poly(m,'Grip golden diamond mount','gold_recess',[(0,-5),(2.6,-11.7),(0,-19),(-2.6,-11.7)],side*2.30,.55,.2)
        poly(m,'Grip inlaid gold lozenge','gold',[(0,-6),(2.05,-11.7),(0,-18),(-2.05,-11.7)],side*2.63,.28,.11)
    m.lathe('Fluted grip lower collar','steel',[(-22,2.1),(-20.8,2.8),(-19.6,2.2)],10)
    m.lathe('Gilded sword faceted pommel','steel',[(-27,.4),(-25.5,3.4),(-23.6,2.9),(-22,2.1)],6)
    m.notes=['User reference with the latest requested shorter blade: three broad contiguous gold diamonds, bent silver quillons with dark inset channels, short red grip and faceted silver pommel.',
             'Three physical tempered-gold inlays occupy most of the blade width and have independent reflection materials.',
             'The patterned blade segment was shortened from 121 to 94 author units while preserving the blade seat, guard and grip. This user-requested review adjustment supersedes the initial image-derived length target.',
             'Absolute get-item size is a review target, not measured Nintendo GI coordinates.']
    return m


def leaf(m,name,mat,base,tip,width,z,side=1):
    base=np.array(base,float);tip=np.array(tip,float);axis=tip-base
    perp=unit([-axis[1],axis[0]])*width;a=base+axis*.44
    p=np.array([[*base,z],[*(a+perp),z],[*tip,z],[*(a-perp),z],
                [*(base+axis*.5),z+side*.5]])
    m.add(name,mat,p,np.tile([0,0,side],(5,1)),
          [[.5,0],[0,.5],[.5,1],[1,.5],[.5,.5]],[[0,1,4],[1,2,4],[2,3,4],[3,0,4]],flat=True)


def relief_panel(m,name,mat,outline,z,side=1,height=1.2):
    outline=np.array(outline,float);center=outline.mean(0);span=np.ptp(outline,axis=0)
    points=[];normals=[];uv=[];tris=[];count=len(outline)
    for scale,lift in [(1,0),(.78,.65),(.36,.95)]:
        for xy in center+(outline-center)*scale:
            points.append([*xy,z+side*height*lift])
            direction=(xy-center)/np.maximum(span/2,1)
            normals.append(unit([direction[0]*.75,direction[1]*.28,side]))
            uv.append((xy-center)/span+.5)
    for ring in range(2):
        for i in range(count):
            a=ring*count+i;b=ring*count+(i+1)%count
            tris.extend([[a,b,a+count],[b,b+count,a+count]])
    points.append([*center,z+side*height]);normals.append([0,0,side]);uv.append([.5,.5])
    for i in range(count):tris.append([2*count+i,2*count+(i+1)%count,3*count])
    m.add(name,mat,points,normals,uv,tris)


def great_fairy_sword():
    m=model('great_fairy_sword','Great Fairy Sword')
    m.markers.update(guard_baseline_y=0,original_gi_vertex_data_available=False,
                     proportion_reference='Secondary stock held geometry; absolute get-item dimensions are approximate.')
    # Secondary stock held geometry supplies blade/guard/grip ratios; absolute
    # get-item size remains a review target. Build each part independently.
    blade(m,[(-6,1.8),(-2,2.2),(2,2.6),(5,3.5),(18,7.2),(30,9.65),(81.5,13.65),(133,9.65),(154.5,.04)],
          'rose','rose_edge',1.70,'Rose-tempered fairy blade')
    tip=[(0,154.5),(-9.65,133),(-7.7,121),(0,133),(7.7,121),(9.65,133)]
    root=[(0,4),(-6.5,18),(-9.1,30),(-4.0,24),(0,14),(4.0,24),(9.1,30),(6.5,18)]
    for side in [-1,1]:
        relief_panel(m,'Domed lilac forged blade tip','violet_edge',tip,side*1.92,side,1.05)
        relief_panel(m,'Domed lily blade shoulder','violet_edge',root,side*1.95,side,1.0)
        panel=[(0,133),(-7.4,120),(-8.0,45),(0,31),(8.0,45),(7.4,120)]
        poly(m,'Recessed fairy motif border','violet',np.asarray(panel)*[1.08,1],side*1.61,.38,.14)
        poly(m,'Emerald enamel motif field','jade',panel,side*1.85,.25,.09)
        for sign in [-1,1]:
            path=bezier([sign*.3,41],[sign*8.8,70],[sign*-3.6,98],[sign*3.45,126],30)
            stroke(m,'Raised fairy vine stem','jade_edge',path,side*2.13,.19,5)
            for k,y in enumerate([55,73,92,112]):
                x=sign*(1.9+1.1*math.sin(k*1.4));dest=[sign*6.3,y+8]
                leaf(m,'Embossed fairy leaf','jade_edge',[x,y],dest,1.15,side*2.22,side)
                stroke(m,'Leaf central engraving','jade',[[x,y],dest],side*2.76,.09,4)
        for y in [67,101]:
            for k in range(5):
                a=math.tau*k/5
                leaf(m,'Small fairy rosette petal','jade_edge',[0,y],
                     [math.cos(a)*2.55,y+math.sin(a)*3.2],.95,side*2.2,side)
            gem(m,'gold_edge',0,y,side*2.65,.65,.65)
        stroke(m,'Lilac central blade frame','violet_edge',np.vstack([panel,panel[0]]),side*2.03,.17,5)
    for sign in [-1,1]:
        wing=np.array([(0,1),(4.7,2),(12.6,6),(18.9,6.5),(22,4),(15.7,4),
                       (9.4,-.5),(4.7,-3.7),(0,-3.8)])*[sign,1]
        poly(m,'Lily-shaped fairy quillon','violet_edge',wing,0,3.5,.8)
        for side in [-1,1]:
            stroke(m,'Quillon petal vein','violet',
                   np.array([(0,-1.7),(6.3,1.0),(14.1,4.9),(18.9,5)])*[sign,1],side*1.82,.18,5)
            leaf(m,'Leaf under fairy crossguard','jade_edge',[sign*4.7,-4],[sign*15.7,-7],1.3,side*1.8,side)
    grip(m,-39.3,-4.5,2.5,'green_leather','green_leather',11)
    m.lathe('Fairy blade-to-grip fitted collar','violet_edge',
            [(-6.5,2.25),(-5.5,2.85),(-3.25,2.75),(-2.5,2.1)],12)
    for side in [-1,1]:
        for y in [-37.3,-28.2,-19.1,-10]:
            for sign in [-1,1]:
                leaf(m,'Sculpted leaf grip wrap','jade_edge',[0,y],[sign*2.8,y+5],1.0,side*2.58,side)
    m.lathe('Fairy grip lower collar','jade',[(-40.6,1.5),(-39.7,2.6),(-38.2,2.55)],12)
    m.sphere('Fairy leaf bud pommel','jade_edge',[0,-40.8,0],[2.0,1.5,2.0],6,10)
    m.notes=['Rebuilt rose-metal blade, lilac lily fittings, emerald field and sculpted leaves/floral ornament.',
             'All blade, handle and ornament surfaces are opaque; no enlarged low-resolution black/green motif is used.',
             'User-requested long two-handed get-item silhouette; botanical relief anchors and grip leaves follow the new part dimensions.',
             'Blade, guard and grip ratios follow secondary stock held geometry; absolute get-item size is approximate, not measured from Nintendo GI vertices.']
    return m


def kokiri_sword(mm=False):
    slug='mm_kokiri_sword' if mm else 'kokiri_sword'
    m=model(slug,'Kokiri Sword (MM)' if mm else 'Kokiri Sword (OoT)')
    metal(m,'bronze','#AD7D46',.85);metal(m,'brass','#CAB169',.9);metal(m,'brass_edge','#E2D098',1)
    m.material('walnut',rgb('#874921'),'wood',rough=.57)
    m.material('brown_leather',rgb('#713A1B'),'leather',rough=.75)
    m.material('brown_wrap',rgb('#BB8150'),'leather',rough=.72)
    if mm:
        blade(m,[(-6,2.2,0),(1,2.5,0),(5,6.0,0),(49,5.9,0),
                 (54,5.0,-.9),(63,.04,-5.8)],thick=1.6,name='MM Kokiri forged blade')
        guard=[(-13,-2),(-14,0),(-13,4),(-11.5,5),(11.5,5),(13,4),(14,0),(13,-2)]
        poly(m,'Rounded brass rectangular guard','brass',guard,0,5.4,1.25)
        for side in [-1,1]:
            stroke(m,'Machined guard rim','brass_edge',
                   [[-11.3,-1],[-12.5,.5],[-11.3,3.8],[11.3,3.8],[12.5,.5],[11.3,-1]],side*2.78,.20,5)
            for x in [-8.4,8.4]:
                m.sphere('Recessed guard fastener socket','crease',[x,1.1,side*2.77],[1.23,1.23,.16],6,10)
                m.sphere('Guard steel flush rivet','steel',[x,1.1,side*2.88],[.65,.65,.24],5,8)
        grip(m,-25,-2,2.55,'white_leather','white_wrap',8)
        m.lathe('Upper Kokiri guard ferrule','steel',[(-4.1,2.4),(-2.5,3.0),(-1.2,2.8)],12)
        m.lathe('Kokiri pommel ferrule','steel',[(-26.5,2.15),(-25.5,2.9),(-24,2.6)],12)
        m.lathe('Violet Kokiri pommel','violet',[(-30,.6),(-28.6,2.9),(-26.5,2.6)],8)
        m.notes=['MM Kokiri identity: broad clipped steel blade, rounded brass bar guard with two recessed fasteners, pale wrapped grip and violet pommel.',
                 'Rebuilt beveled steel, machined guard rims and grip fittings; continuous blade tang passes into the guard and upper grip.']
    else:
        metal(m,'ruby','#C41425',.50);m.materials['ruby'].update(metal=.12,rough=.16)
        blade(m,[(-6,2.15),(1,2.6),(5,6.2),(40,6.3),(48,5.8),(61,.04)],thick=1.65,name='OoT Kokiri forged blade')
        for sign in [-1,1]:
            guard=np.array([(0,2),(5,3),(8.5,7),(11,8.5),(13,7),(11,2),
                            (10,-2),(6,-2.5),(2,-4),(0,-4)])*[sign,1]
            poly(m,'Carved walnut Kokiri crossguard','walnut',guard,0,4.5,1.05)
        poly(m,'Kokiri central bronze gemstone mount','bronze',
             [(0,4.1),(3.6,1),(2.6,-3.4),(0,-5.3),(-2.6,-3.4),(-3.6,1)],0,5.2,.85)
        for side in [-1,1]:gem(m,'ruby',0,-.3,side*2.8,1.40,1.80)
        grip(m,-25,-4,2.7,'brown_leather','brown_wrap',7)
        m.lathe('Kokiri bronze upper ferrule','bronze',[(-6,2.6),(-4,3.2),(-3,2.8)],12)
        m.lathe('Kokiri bronze pommel collar','bronze',[(-27,2.7),(-25.3,3),(-24.1,2.6)],12)
        poly(m,'Walnut flared Kokiri pommel','walnut',
             [(-3.1,-26),(3.1,-26),(4.7,-29.5),(4.4,-32),(-4.4,-32),(-4.7,-29.5)],0,4.8,1)
        for side in [-1,1]:stroke(m,'Pommel bronze lip','bronze',[[-3.8,-30.7],[3.8,-30.7]],side*2.4,.25,5)
        m.notes=['OoT Kokiri identity: short steel blade, solid brown guard, red jewel, leather grip and flared wooden pommel.',
                 'Clean walnut guard without added wire ornament; mirrored bevel faces point outward and the blade tang extends through its central mount.']
    return m


def four_sword_guard(m):
    top=bezier([0,2.4],[6,1.8],[13,5],[16.5,12],25)
    bottom=bezier([0,-4.5],[8,-4.8],[17,-1],[19.5,8],25)
    top=np.vstack([top[:0:-1]*[-1,1],top]);bottom=np.vstack([bottom[:0:-1]*[-1,1],bottom])
    v=np.array([0,.10,.18,.22,.29,.33,.43,.47,.54,.58,.68,.72,.79,.83,.90,1])
    z=np.array([1.4,2.5,2.55,2.18,2.18,2.55,2.55,2.18,2.18,2.55,2.55,2.18,2.18,2.55,2.5,1.4])
    section=np.r_[np.c_[v,z],np.c_[v[::-1],-z[::-1]]]
    xy=bottom[:,None,:]+(top-bottom)[:,None,:]*section[None,:,0,None]
    grid=np.concatenate([xy,np.broadcast_to(section[None,:,1,None],(*xy.shape[:2],1))],axis=2)
    width=len(section)
    for j in range(width):
        k=(j+1)%width;points=grid[:,[j,k],:]
        tangent=np.gradient(points,axis=0);across=points[:,1]-points[:,0]
        normals=unit(np.cross(tangent,across[:,None,:]));tri=[]
        for i in range(len(grid)-1):
            a=2*i;tri.extend([[a,a+2,a+1],[a+1,a+2,a+3]])
        recessed=abs(section[j,1])==2.18 and abs(section[k,1])==2.18
        mat='guard_groove' if recessed else 'gold';p=points.reshape(-1,3)
        m.add('Four Sword integral recessed guard bands' if recessed else 'Four Sword solid curved guard',
              mat,p,normals.reshape(-1,3),np.c_[p[:,0]/40+.5,p[:,1]/18],tri)
    for i,sign in [(0,-1),(-1,1)]:
        delta=top[i]-bottom[i];normal=unit([delta[1],-delta[0],0])*sign
        m.add('Four Sword solid guard end cap','gold_edge',grid[i],
              np.tile(normal,(width,1)),section/[1,5]+[0,.5],earclip(section))


def cloth_grip(m,y0,y1,radius=2.5,turns=7):
    m.lathe('Four Sword grip core','grip_shadow',[(y0,radius*.85),
            (y0+1,radius),(y1-1,radius),(y1,radius*.85)],16)
    pitch=(y1-y0)/turns;t=np.linspace(0,TAU*turns,turns*16+1)
    p=[];n=[];uv=[];tri=[]
    for i,a in enumerate(t):
        for j,v in enumerate([-.48,0,.48]):
            y=y0+i/(len(t)-1)*(y1-y0)+v*pitch
            taper=np.interp(y,[y0,y0+1.8,y1-1.8,y1],[.90,1,1,.90])
            r=radius*taper+.10+(1-abs(v)/.48)*.08
            p.append([r*math.cos(a),y,r*math.sin(a)])
            n.append([math.cos(a),0,math.sin(a)]);uv.append([v+.5,a/TAU])
        if i:
            for j in range(2):
                b=(i-1)*3+j;tri.extend([[b,b+3,b+1],[b+1,b+3,b+4]])
    m.add('Four Sword close-wound ivory cloth','ivory_cloth',p,n,uv,tri)


def disc(m,name,mat,y,radius,z=0,depth=1,bevel=.3):
    a=np.linspace(0,TAU,32,endpoint=False)
    poly(m,name,mat,np.c_[np.cos(a)*radius,y+np.sin(a)*radius],z,depth,bevel)


def four_sword():
    m=model('four_sword','Four Sword')
    metal(m,'ruby','#C41425',.55);m.materials['ruby'].update(metal=.12,rough=.16)
    metal(m,'pommel_blue','#20539C',.65);metal(m,'guard_groove','#493922',.22)
    m.material('grip_shadow',rgb('#413A35'),rough=.88)
    m.material('ivory_cloth',rgb('#E5D8BA'),'cloth',rough=.9)
    blade(m,[(-7,2.4),(-1,3.4),(4,4.9),(10,5.1),(54,4.35),(67,2.8),(78,.04)],thick=1.45,name='Four Sword forged blade')
    four_sword_guard(m)
    poly(m,'Four Sword forked gold blade collar','gold',
         [(-4.8,-2),(-5.8,7.5),(-2.7,5.7),(0,9),(2.7,5.7),(5.8,7.5),
          (4.8,-2),(2.5,-4),(-2.5,-4)],0,4.8,.55)
    disc(m,'Four Sword circular ruby socket','gold_edge',-.8,4.2,0,5.9,.5)
    for side in [-1,1]:
        disc(m,'Four Sword dark ruby bezel','guard_groove',-.8,3.18,side*3.05,.35,.1)
        m.ring('Four Sword concentric gold ruby bezel','gold_edge',[0,-.8,side*3.35],2.77,.23,segments=32)
        m.sphere('Four Sword round red ruby','ruby',[0,-.8,side*3.30],[2.40,2.40,1.05],10,20)
    cloth_grip(m,-25.5,-5.3)
    for y in [-25.5,-5.5]:
        m.lathe('Four Sword gold grip collar','gold_edge',[(y-1,2.3),(y-.6,2.9),(y+.6,2.9),(y+1,2.3)],16)
    disc(m,'Four Sword round gold pommel','gold',-30.5,4.8,0,4.8,.6)
    for side in [-1,1]:
        disc(m,'Four Sword blue pommel enamel','pommel_blue',-30.5,3.6,side*2.50,.35,.15)
        m.ring('Four Sword gold pommel rim','gold_edge',[0,-30.5,side*2.65],3.75,.28,segments=32)
        triforce(m,-31.5,side*2.76,'gold_edge',.76)
    m.notes=['Four Sword follows the user-supplied reference: curved gold guard with three integral recessed bands, round red ruby, ivory cloth grip and circular blue/gold Triforce pommel.',
             'No dangling cloth tails. Blade tang and forked collar join continuously; reflective steel and gold are real exported materials.']
    return m


BUILDERS={'master_sword':master_sword,'true_master_sword':lambda:master_sword(True),
          'gilded_sword':gilded_sword,'great_fairy_sword':great_fairy_sword,
          'kokiri_sword':kokiri_sword,'mm_kokiri_sword':lambda:kokiri_sword(True),'four_sword':four_sword}


def build(slug):
    m=BUILDERS[slug]();used={part['mat'] for part in m.parts}
    m.materials={k:v for k,v in m.materials.items() if k in used}
    return m
