"""Authored equipment GI candidates, using the accepted quantized mesh pipeline.

Each candidate has its own silhouette, modeled seams/engraving, and repeatable
material swatches.  These are presentation resources only; no held/equipped
geometry or gameplay is produced by this module.
"""
import argparse
import math
import shutil
from pathlib import Path

import numpy as np
from meshkit import Model, TAU, bezier, rotation, unit


NAMES = {
    'divine_shield': 'Divine Shield',
    'sheikah_shield': 'Sheikah Shield / Kite Shield',
    'shield_of_ikana': 'Shield of Ikana',
    'magic_cape': 'Magic Cape',
    'spirit_breastplate': 'Spirit Breastplate',
    'sages_tunic': "Sages' Tunic",
    'champions_tunic': "Champion's Tunic",
    'pegasus_anklet': 'Pegasus Anklet',
    'trident': 'Trident',
    'climb_boots': 'Climb Boots',
    'roc_boots': "Roc's Boots",
    'cane_of_byrna': 'Cane of Byrna',
    'four_sword': 'Four Sword',
    'pendant_of_memories': 'Pendant of Memories',
}


def _model(slug):
    return Model(slug, NAMES[slug], 'objects/nei_gi_redesign/' + slug + '/gi_dl', 1., .65)


def _finish(m):
    """Fit the GI, then repair winding after the final 1/16-unit quantization."""
    p=np.concatenate([part['p'] for part in m.parts])
    span=np.ptp(p,axis=0).max()
    if span<95:
        k=95/span
        m.transform(scale=[k,k,k])
    elif span>123:
        k=123/span
        m.transform(scale=[k,k,k])
    for part in m.parts:
        part['p']=np.rint(part['p']*16)/16
        part['n']=unit(part['n'])
        f=part['tri'];p=part['p']
        fn=np.cross(p[f[:,1]]-p[f[:,0]],p[f[:,2]]-p[f[:,0]])
        good=np.linalg.norm(fn,axis=1)>1e-7
        f=f[good];fn=fn[good]
        flip=np.einsum('ij,ij->i',fn,part['n'][f].sum(axis=1))<0
        f[flip]=f[flip][:,[0,2,1]]
        part['tri']=f
    m.parts=[part for part in m.parts if len(part['tri'])]
    m.notes.append('GI author span fitted to 61.75–79.95 effective units; final quantization winding checked. Runtime untested.')
    return m


def _cloth(color, size=256):
    y, x = np.mgrid[:size, :size]
    rng = np.random.default_rng(2218)
    v = .94 + .025*np.cos(x*math.pi) + .022*np.cos(y*math.pi)
    v += .012*np.sin((x+y)*TAU/17) + rng.normal(0, .007, (size, size))
    rgb = np.uint8(np.clip(np.array(color)[None, None, :]*255*v[..., None], 0, 255))
    return np.concatenate([rgb, np.full((size, size, 1), 255, np.uint8)], axis=2)


def _leather(color, size=256):
    y, x = np.mgrid[:size, :size]/size
    noise = np.random.default_rng(167).normal(0, .018, (size, size))
    pores = np.cos((x+np.sin(y*TAU*4)*.02)*TAU*43)*np.cos(y*TAU*47)
    v = .87 + pores*.05 + .045*np.cos(y*TAU*2) + noise
    rgb = np.uint8(np.clip(np.array(color)[None, None, :]*255*v[..., None], 0, 255))
    return np.concatenate([rgb, np.full((size, size, 1), 255, np.uint8)], axis=2)


def _metals(m):
    m.material('gold', [.76, .49, .14], 'metal', .83, .35)
    m.material('gold_edge', [.98, .77, .32], 'metal', .86, .26)
    m.material('old_gold', [.38, .27, .105], 'metal', .75, .57)
    m.material('silver', [.70, .77, .83], 'metal', .86, .3)
    m.material('steel_dark', [.25, .30, .34], 'metal', .82, .5)
    m.material('ivory', [.96, .90, .71], 'ceramic', .12, .6)


def _poly(m, name, mat, xy, z=0, depth=.75, bevel=.22):
    m.polygon(name, mat, xy, depth=depth, z=z, bevel=min(bevel, depth*.8))


def _stroke(m, name, mat, xy, z=0, r=.35, sides=5):
    path = np.c_[np.asarray(xy, float), np.full(len(xy), z)]
    m.tube(name, mat, path, r, sides)


def _inside(part):
    """The visible lining is the inward face of a hollow cuff or band."""
    part['n']=-part['n']
    part['tri']=part['tri'][:,[0,2,1]]


def _bow(m, start, width, strength=3.3):
    """Curve a complete embossed assembly into a shallow shield shell."""
    for p in m.parts[start:]:
        x = p['p'][:, 0]
        p['p'][:, 2] += strength*(1-(x/width)**2)
        p['p'] = np.rint(p['p']*16)/16
        n = p['n'].copy()
        n[:, 0] -= (-2*strength*x/width**2)*n[:, 2]
        p['n'] = unit(n)


def _shield_back(m, width=27, y=0, z=-3):
    # Two broad embossed grip mounts and a real loop clear of the backing.
    for x in [-width*.49, width*.49]:
        _poly(m, 'Leather strap steel anchor', 'steel_dark',
              [(x-3,y-13),(x+3,y-13),(x+3,y+13),(x-3,y+13)], z, 1.0, .3)
        for yy in [y-10, y+10]:
            m.sphere('Grip anchor rivet', 'silver', [x,yy,z-.85], [.95,.95,.55], 5, 8)
    a = np.linspace(0, math.pi, 16)
    path = np.c_[np.linspace(-width*.48,width*.48,16),
                 np.full(16,y), z-2-np.sin(a)*6.7]
    m.tube('Arched rear hand strap', 'back_leather', path, 2.4, 9)
    # A second strap running vertically creates a readable fitted grip from rear.
    m.tube('Rear forearm brace', 'back_leather',
           [[-width*.45,y-13,z-1],[-width*.62,y-6,z-5],
            [-width*.62,y+6,z-5],[-width*.45,y+13,z-1]], 1.9, 8)


def divine_shield():
    m = _model('divine_shield'); _metals(m)
    m.material('lavender', [.51,.25,.62], 'ceramic', .55, .32)
    m.material('purple', [.34,.105,.50], 'ceramic', .5, .35)
    m.material('pale_lilac', [.89,.83,.93], 'metal', .7, .36)
    m.material('pink_enamel', [.83,.21,.66], 'ceramic', .46, .25)
    m.material('back_leather', [.22,.12,.17], tex=_leather([.22,.12,.17]), rough=.8)
    outline = [(-34,32),(-22,40),(-10,43),(0,51),(10,43),(22,40),(34,32),
               (33,8),(28,-15),(19,-33),(0,-49),(-19,-33),(-28,-15),(-33,8)]
    start = len(m.parts)
    _poly(m,'Forged silver shield edge','silver',outline,0,5,1.5)
    inside=np.asarray(outline)*[.89,.92]
    _poly(m,'Raised lavender outer field','lavender',inside,2.55,1.2,.5)
    _poly(m,'Deep violet heraldic field','purple',inside*[.89,.90],3.35,.9,.35)
    m.tube('Continuous lilac rolled edge','pale_lilac',
           np.c_[np.vstack([inside,inside[0]]),np.full(len(inside)+1,3.15)],.8,6)
    # The original material carries angular golden wings around a purple center.
    # Those motifs are modeled shallow gold castings, with a separated upper crest.
    for sign in [-1,1]:
        wing=np.array([(3,12),(14,24),(28,30),(27,21),(20,17),(29,12),
                       (27,3),(17,7),(10,3),(7,-1),(3,3)])*[sign,1]
        _poly(m,'Upper cast goddess wing','gold_edge',wing,4.15,.8,.25)
        wing=np.array([(4,-2),(10,-7),(22,-12),(19,-20),(11,-14),
                       (15,-26),(9,-31),(5,-16),(1,-13)])*[sign,1]
        _poly(m,'Lower sweeping gold wing','gold',wing,4.17,.85,.3)
        for j in range(3):
            xy=np.array([(7+j*4,13+j*3),(17+j*3,19+j*3),(23+j,22+j*3)])*[sign,1]
            _stroke(m,'Fine goddess wing engraving','old_gold',xy,4.63,.22,4)
        crest=np.array([(2,38),(8,42),(15,38),(23,36),(16,28),(9,26)])*[sign,1]
        _poly(m,'Pale upper crest feather','pale_lilac',crest,4.25,.8,.2)
    _poly(m,'Vertical sacred center lozenge','pink_enamel',
          [(0,28),(7,12),(4,-9),(0,-20),(-4,-9),(-7,12)],4.6,.9,.35)
    _poly(m,'Lower heart enamel','pink_enamel',
          [(0,-21),(6,-17),(9,-22),(6,-33),(0,-39),(-6,-33),(-9,-22),(-6,-17)],4.4,.8,.25)
    _poly(m,'Small ivory crest center','ivory',[(0,42),(3,35),(0,31),(-3,35)],4.75,.7,.2)
    for x,y in [(-28,29),(28,29),(-23,-13),(23,-13),(0,-43)]:
        m.sphere('Shield rim flush rivet','pale_lilac',[x,y,3.4],[1.0,1.0,.65],5,8)
    _bow(m,start,35,3.3)
    _shield_back(m,30,-1,-3)
    m.notes=['Canonical Divine/Goddess shield palette: lilac silver edge, purple field, pale crown and gold wings.',
             'New bowed forged shell, raised heraldic castings and real rear leather grip; GI only.']
    return _finish(m)


def sheikah_shield():
    m=_model('sheikah_shield'); _metals(m)
    m.material('red_lacquer',[.61,.15,.12],'wood',.2,.5)
    m.material('red_edge',[.79,.30,.20],'metal',.65,.44)
    m.material('blue_inlay',[.16,.39,.56],'ceramic',.4,.38)
    m.material('chalk',[.88,.79,.63],'stone',0,.8)
    m.material('back_leather',[.24,.12,.055],tex=_leather([.24,.12,.055]),rough=.83)
    outline=[(0,50),(13,43),(24,26),(23,5),(17,-16),(0,-40),(-17,-16),(-23,5),(-24,26),(-13,43)]
    start=len(m.parts)
    _poly(m,'Narrow Rito kite wooden core','back_leather',outline,0,5,1.5)
    _poly(m,'Red lacquer kite shell','red_lacquer',np.asarray(outline)*[.93,.95],2.5,1.25,.45)
    edge=np.c_[np.vstack([outline,outline[0]]),np.full(len(outline)+1,2.7)]
    m.tube('Warm red metal rolled border','red_edge',edge,1.05,7)
    _poly(m,'Pale tip cap','gold_edge',[(0,50),(6.2,45),(4.3,40),(-4.3,40),(-6.2,45)],3.6,.8,.3)
    _poly(m,'Blue kite top inlay','blue_inlay',[(0,43),(5,40),(0,33),(-5,40)],4.0,.45,.16)
    # Stylized Rito bird, matching the red/ivory Kite Shield identity used by the
    # Sheikah Shield baseline callback, rather than introducing a slate eye.
    _poly(m,'Ivory Rito bird body','chalk',[(0,28),(4,21),(3,10),(0,6),(-3,10),(-4,21)],3.9,.6,.18)
    for sign in [-1,1]:
        xy=np.array([(2,20),(9,24),(17,22),(13,19),(18,15),(10,14),
                     (15,9),(6,11),(3,14)])*[sign,1]
        _poly(m,'Rito spread wing carving','chalk',xy,3.9,.65,.2)
        _stroke(m,'Blue kite shoulder stripe','blue_inlay',
                np.array([(4,34),(14,30),(19,22)])*[sign,1],3.8,.95,6)
        _stroke(m,'Lower cream diagonal stripe','chalk',
                np.array([(5,-2),(13,-6),(16,-13)])*[sign,1],3.8,.95,6)
    _poly(m,'Bird forked tail','chalk',[(-3,8),(-5,0),(0,4),(5,0),(3,8)],4.0,.65,.18)
    _poly(m,'Blue lower kite chevron','blue_inlay',[(0,-18),(6,-12),(0,-30),(-6,-12)],3.8,.6,.2)
    for y in [-5,3,31]:
        _stroke(m,'Fine engraved kite cross line','old_gold',[(-13,y),(0,y-2),(13,y)],4.02,.17,4)
    _bow(m,start,26,2.8)
    _shield_back(m,23,10,-3)
    # Three characteristic slender tail attachments extend the kite silhouette.
    for x,y,z in [(-16,-12,-.5),(0,-37,-.5),(16,-12,-.5)]:
        path=bezier([x,y,z],[x*.95,y-7,z],[x*.75,y-14,z+1],[x*.80,y-18,z],10)
        m.tube('Rito kite tail leather cord','back_leather',path,.95,6)
        start=len(m.parts)
        _poly(m,'Kite tail pale feather vane','chalk',
              [(-1.3,3),(1.9,1),(2.4,-6),(0,-10),(-2.1,-5)],0,.65,.2)
        _poly(m,'Tail blue binding','blue_inlay',[(-1.8,2),(1.8,2),(1.8,-.3),(-1.8,-.3)],.1,.7,.15)
        m.transform(rotation('z',-12 if x<0 else 12),offset=path[-1],start=start)
    m.notes=['Sheikah Shield canonical callback uses the red, blue and ivory Rito Kite Shield.',
             'New slender bowed wooden kite, modeled raptor engraving, rolled edge, tail cords and rear straps.']
    return _finish(m)


def shield_of_ikana():
    m=_model('shield_of_ikana'); _metals(m)
    m.material('mirror',[.64,.62,.74],'metal',.92,.22)
    m.material('face_silver',[.80,.76,.87],'metal',.86,.28)
    m.material('rose_crease',[.46,.19,.28],'metal',.55,.44)
    m.material('dark_eye',[.125,.06,.11],'ceramic',.25,.48)
    m.material('eye_red',[.67,.19,.18],'ceramic',.45,.34)
    m.material('back_leather',[.23,.13,.10],tex=_leather([.23,.13,.10]),rough=.85)
    outline=[(0,49),(15,47),(29,37),(34,18),(32,-6),(23,-29),
             (11,-43),(0,-50),(-11,-43),(-23,-29),(-32,-6),(-34,18),(-29,37),(-15,47)]
    start=len(m.parts)
    _poly(m,'Bronze mirror shield rim','gold',outline,0,5,1.7)
    inner=np.asarray(outline)*[.88,.92]
    _poly(m,'Lavender polished mirror field','mirror',inner,2.65,1.5,.7)
    m.tube('Bright inner mirror lip','gold_edge',
           np.c_[np.vstack([inner,inner[0]]),np.full(len(inner)+1,3.25)],.85,7)
    # Majora's Mask mirror shield: the face, raised brow, deep eyes and elongated
    # screaming mouth are independent relief surfaces, not a flat logo texture.
    for sign in [-1,1]:
        brow=np.array([(3,19),(8,32),(17,31),(24,21),(25,12),(20,16),(16,24),(8,23)])*[sign,1]
        _poly(m,'Screaming face raised brow','face_silver',brow,4.0,2.0,.45)
        eye=np.array([(6,18),(12,22),(21,15),(20,4),(14,0),(8,6)])*[sign,1]
        _poly(m,'Inset mirror eye socket','rose_crease',eye,4.18,.9,.3)
        _poly(m,'Dark screaming eye cavity','dark_eye',eye*np.array([.84,.79])+[sign*1.5,1],4.72,.8,.25)
        m.sphere('Red narrow mirror eye','eye_red',[sign*13.4,11,5.2],[3.1,5.2,.75],7,12)
        cheek=np.array([(22,5),(26,-4),(23,-19),(15,-26),(13,-19),(19,-10)])*[sign,1]
        _poly(m,'Long raised mirror cheek','face_silver',cheek,4.0,1.25,.35)
        _stroke(m,'Cheek etched tear line','rose_crease',
                np.array([(24,0),(23,-10),(19,-19)])*[sign,1],4.9,.55,6)
        _stroke(m,'Forehead radiating engraving','gold',
                np.array([(4,34),(11,38),(18,35)])*[sign,1],4.0,.38,5)
    _poly(m,'Face raised nose bridge','face_silver',
          [(-3,23),(3,23),(5,0),(8,-5),(0,-10),(-8,-5),(-5,0)],4.5,2.4,.6)
    # Long oval open mouth is recess-colored geometry bordered by a raised ring.
    m.sphere('Screaming mouth dark recess','dark_eye',[0,-24,4.0],[9,17,1.4],12,20)
    m.ring('Screaming mouth rose contour','rose_crease',[0,-24,5.15],[9.6,17.9],1.15,'z',40,8)
    m.ring('Screaming mouth pale raised lip','face_silver',[0,-24,5.6],[10.7,19.0],.68,'z',40,6)
    _poly(m,'Screaming face chin','face_silver',[(-10,-42),(0,-46),(10,-42),(6,-37),(-6,-37)],3.9,1.0,.3)
    for x,y in [(-25,31),(25,31),(-24,-21),(24,-21),(0,43)]:
        m.sphere('Mirror rim inset rivet','gold_edge',[x,y,3.4],[.95,.95,.6],5,8)
    _bow(m,start,35,2.7)
    _shield_back(m,30,3,-3)
    m.notes=['Shield of Ikana canonical model is the MM screaming-face Mirror Shield.',
             'New bronze-edged lavender mirror with modeled brows, recessed red eyes, nose, tear grooves and screaming mouth.']
    return _finish(m)


def _sheet(m,name,mat,grid,inward=False):
    grid=np.asarray(grid,float);rows,cols=grid.shape[:2]
    normals=unit(np.cross(np.gradient(grid,axis=0),np.gradient(grid,axis=1)))
    if np.mean(normals[:,:,2])<0:normals=-normals
    if inward:normals=-normals
    tr=[]
    for i in range(rows-1):
        for j in range(cols-1):
            a=i*cols+j;b=a+cols;tr.extend([[a,b,a+1],[a+1,b,b+1]])
    u,v=np.meshgrid(np.linspace(0,1,cols),np.linspace(0,1,rows))
    m.add(name,mat,grid.reshape(-1,3),normals.reshape(-1,3),np.c_[u.ravel(),v.ravel()],tr)


def magic_cape():
    m=_model('magic_cape'); _metals(m)
    m.material('crimson_cloth',[.72,.045,.055],tex=_cloth([.72,.045,.055]),rough=.78)
    m.material('crimson_piping',[.91,.10,.09],tex=_cloth([.91,.10,.09]),rough=.7)
    m.material('wine_lining',[.24,.025,.05],tex=_cloth([.24,.025,.05]),rough=.86)
    m.material('brocade',[.92,.67,.18],'metal',.45,.48)
    angles=np.linspace(-math.radians(112),math.radians(112),55)
    ts=np.linspace(0,1,22)
    outer=[]
    for t in ts:
        row=[]
        for a in angles:
            rx=13+23*t;rz=8+10*t
            fold=math.sin(a*6+.25)*2.3*math.sin(math.pi*t*.9)
            y=45*(1-t)+(-46-4*math.cos(a)+2*math.sin(a*3)**2)*t
            row.append([(rx+fold)*math.sin(a),y,(rz+fold)*math.cos(a)+4*math.sin(math.pi*t)])
        outer.append(row)
    outer=np.array(outer);normal=unit(outer*np.array([1,0,1]))
    _sheet(m,'Deep crimson draped cape','crimson_cloth',outer)
    _sheet(m,'Rich wine inner lining','wine_lining',outer-normal*.75,True)
    m.tube('Crimson rolled cape hem','crimson_piping',outer[-1],.75,7)
    for j in [0,-1]:m.tube('Gold brocade cape edge','brocade',outer[:,j]+normal[:,j]*.55,.85,7)
    # The icon's gold foliage and central sun are stitched dimensional brocade.
    def cloth_z(x,y):
        t=np.clip((45-y)/94,0,1);rx=13+23*t;rz=8+10*t
        a=math.asin(np.clip(x/rx,-.98,.98))
        fold=math.sin(a*6+.25)*2.3*math.sin(math.pi*t*.9)
        return (rz+fold)*math.cos(a)+4*math.sin(math.pi*t)+.7
    def sew(name,xy,r=.4):
        m.tube(name,'brocade',[[x,y,cloth_z(x,y)] for x,y in xy],r,5)
    sun=np.array([(math.cos(a)*8.8,18+math.sin(a)*10) for a in np.linspace(0,TAU,33)])
    sew('Brocade central sun oval',sun,.72)
    sew('Brocade central sun lower petal',[(-3,14),(0,21),(3,14),(0,9),(-3,14)],.75)
    for sign in [-1,1]:
        vine=bezier([sign*4,7,0],[sign*19,-4,0],[sign*7,-24,0],[sign*20,-38,0],30)[:,:2]
        sew('Long embroidered cape vine',vine,.65)
        for i in [6,13,20,26]:
            x,y=vine[i]
            leaf=np.concatenate([
                bezier([x,y,0],[x+sign*2.5,y+5,0],[x+sign*8,y+7,0],[x+sign*6.3,y+1.5,0],12),
                bezier([x+sign*6.3,y+1.5,0],[x+sign*5,y-4,0],[x+sign*1.5,y-3,0],[x,y,0],12)[1:]
            ])[:,:2]
            sew('Gold embroidery curved leaf',leaf,.46)
            sew('Gold leaf central vein',[(x,y),(x+sign*3.0,y+1),(x+sign*6.3,y+1.5)],.28)
        sew('Upper gold cape flourish',np.array([(2,32),(8,36),(13,29),(12,22)])*[sign,1],.58)
        sew('Lower cape floral scroll',np.array([(2,-31),(8,-27),(11,-33),(7,-39),(2,-39)])*[sign,1],.55)
    sew('Embroidered cape center stem',[(0,7),(0,-4),(0,-14),(0,-25)],.6)
    # Hollow raised neck roll; the reverse view shows open lining and a clasp.
    collar=[]
    for t in np.linspace(0,1,5):
        collar.append([[(13+1.3*t)*math.sin(a),45+7*t,(8+1.1*t)*math.cos(a)] for a in angles])
    collar=np.array(collar)
    _sheet(m,'Turned crimson cape collar','crimson_cloth',collar)
    _sheet(m,'Dark open collar interior','wine_lining',collar-unit(collar*np.array([1,0,1]))*.75,True)
    m.tube('Gold collar embroidered edge','brocade',collar[-1],.68,7)
    m.tube('Reverse neck clasp chain','gold',[[12.1,44,-2.8],[0,40,-6.4],[-12.1,44,-2.8]],.68,6)
    m.sphere('Small reverse clasp medallion','gold_edge',[0,40,-6.4],[2.1,2.1,.75],6,12)
    m.notes=['Canonical Magic Cape icon: crimson cloth with dense gold sun and foliage embroidery.',
             'New modeled pleats, open turned collar, wine lining, sewn brocade vines and rear clasp; GI only.']
    return _finish(m)


def _ellipse_body(m,name,mat,rings,sides=48,fold=.9,cap=False):
    """Hollow elliptical garment body with smooth normals derived from folds."""
    grid=[]
    for i,(y,rx,rz) in enumerate(rings):
        t=i/(len(rings)-1);row=[]
        for j in range(sides+1):
            a=TAU*j/sides
            ripple=fold*(.8*math.sin(a*7+t*2)+.3*math.cos(a*11-t*3))*math.sin(math.pi*t)
            row.append([(rx+ripple)*math.sin(a),y+.3*fold*math.cos(a*6)*math.sin(math.pi*t),(rz+ripple)*math.cos(a)])
        grid.append(row)
    grid=np.array(grid);norm=unit(np.cross(np.gradient(grid,axis=0),np.gradient(grid,axis=1)))
    radial=grid*np.array([1,0,1])
    if np.mean(np.sum(norm*radial,axis=-1))<0:norm=-norm
    tr=[];cols=sides+1
    for i in range(len(rings)-1):
        for j in range(sides):
            a=i*cols+j;b=a+cols;tr.extend([[a,b,a+1],[a+1,b,b+1]])
    u,v=np.meshgrid(np.linspace(0,1,sides+1),np.linspace(0,1,len(rings)))
    m.add(name,mat,grid.reshape(-1,3),norm.reshape(-1,3),np.c_[u.ravel(),v.ravel()],tr)
    return grid


def _sleeve(m,sign,mat,trim,long=False):
    start=len(m.parts)
    length=23 if long else 17
    m.lathe('Hollow stitched sleeve',mat,[(-2,12),(2,13),(length*.5,12.1),(length,10.8)],32,cap=False)
    m.lathe('Sleeve dark inner cuff','lining',[(length-3,10.0),(length,10.0)],32,cap=False)
    _inside(m.parts[-1])
    m.ring('Rolled sleeve cuff edge',trim,[0,length,0],10.8,.7,'y',32,6)
    m.transform(rotation('z',-sign*72),offset=[sign*22,22,0],scale=[1,1,.86],start=start)


def _collar(m,cloth,trim):
    m.ring('Open neckline rolled seam',trim,[0,41,0],[10,7],.7,'y',40,6)
    for sign in [-1,1]:
        _poly(m,'Folded V-neck collar',cloth,
              np.array([(2,38),(9,42),(15,32),(7,20),(3,30)])*[sign,1],11.8,.8,.3)
        _stroke(m,'Collar stitched edging',trim,
                np.array([(2.4,37.6),(8.9,40.8),(13.9,32),(7.0,22)])*[sign,1],12.45,.28,4)


def _garment_surface(grid,y,angle,clearance=.70):
    """Sample the actual folded garment rather than an ideal fixed ellipse."""
    columns=grid.shape[1]-1
    column=(angle%TAU)/TAU*columns
    j=int(column);mix=column-j
    samples=grid[:,j]*(1-mix)+grid[:,j+1]*mix
    p=np.array([np.interp(y,samples[:,1],samples[:,axis]) for axis in range(3)])
    p[1]=y
    return p+unit(p*np.array([1,0,1]))*clearance


def _belt(m,body,mat='belt',trim='gold_edge',y=-13):
    # Closely sample all folds and the vertical body profile; no buried belt
    # vertices or long straight chords crossing the garment remain.
    angles=np.linspace(0,TAU,97)
    band=np.array([[_garment_surface(body,yy,a,.80) for a in angles]
                   for yy in np.linspace(y-3.2,y+3.2,7)])
    _sheet(m,'Fitted waist belt',mat,band)
    part=m.parts[-1]
    if np.mean(np.sum(part['n']*part['p']*np.array([1,0,1]),axis=1))<0:
        _inside(part)
    for edge in [band[0],band[-1]]:
        radial=unit(edge*np.array([1,0,1]))
        m.tube('Waist belt sewn piping',trim,edge+radial*.16,.24,5)
    # Cloth folds under the buckle can project beyond the belt centerline.
    # Fit its complete 4.5-by-3.7 half-extents, including rim thickness.
    buckle_skin=np.array([_garment_surface(body,yy,a,0)
                          for yy in np.linspace(y-3.7,y+3.7,17)
                          for a in np.linspace(-math.pi/2,math.pi/2,97)])
    front=buckle_skin[np.abs(buckle_skin[:,0])<=4.5,2].max()+1.05
    m.ring('Square waist buckle rim',trim,[0,y,front],[3.8,3.0],.68,'z',4,6)
    m.tube('Waist buckle cross pin',trim,[[0,y-2.9,front+.1],[0,y+2.9,front+.1]],.48,5)


def _rear_harness(m,body):
    ys=np.linspace(30,-14,45)
    xs=np.interp(-ys,[-30,-14,5,14],[-14,-9,-2,5])
    rows=[]
    for x,y in zip(xs,ys):
        # A fitted leather ribbon is naturally flat against cloth. Its full
        # width follows the torso, including the underside at the back fold.
        row=[]
        for lateral in np.linspace(-1.2,1.2,5):
            xx=x+lateral
            left,right=math.pi*.5,math.pi*1.5
            for _ in range(25):
                mid=(left+right)/2
                if _garment_surface(body,y,mid,0)[0]>xx:left=mid
                else:right=mid
            row.append(_garment_surface(body,y,(left+right)/2,.72))
        rows.append(row)
    grid=np.array(rows)
    _sheet(m,'Champion rear leather seam strap','belt',grid,True)
    for column in [0,-1]:
        p=grid[:,column]
        m.tube('Champion rear harness stitched edge','deep_blue',
               p+unit(p*np.array([1,0,1]))*.1,.13,4)


def spirit_breastplate():
    m=_model('spirit_breastplate'); _metals(m)
    m.material('spirit_leather',[.33,.12,.045],tex=_leather([.43,.17,.06]),rough=.79)
    m.material('orange_leather',[.72,.28,.065],tex=_leather([.72,.28,.065]),rough=.7)
    m.material('lining',[.14,.045,.02],tex=_cloth([.14,.045,.02]),rough=.9)
    m.material('belt',[.21,.07,.025],tex=_leather([.21,.07,.025]),rough=.85)
    rings=[(-43,22,12),(-35,23,13),(-23,21.2,12),(-13,19,11),(2,20.7,12.2),
           (18,25,14),(27,25,13.5),(33,19,11),(39,10,7)]
    body=_ellipse_body(m,'Fitted brown Spirit leather torso','spirit_leather',rings,48,.25)
    m.tube('Brown rolled breastplate hem','orange_leather',body[0],.62,6)
    _sleeve(m,-1,'spirit_leather','orange_leather');_sleeve(m,1,'spirit_leather','orange_leather')
    _collar(m,'orange_leather','gold')
    # Orange overlapping plates follow the icon's warm chest lacing. The object
    # retains a short-sleeved brown tunic outline while becoming a real cuirass.
    for sign in [-1,1]:
        for i,y in enumerate([12,1,-10]):
            xy=np.array([(3,y+5),(17+i*.5,y+5),(18.5,y-3),(4,y-5)])*[sign,1]
            _poly(m,'Segmented embossed Spirit chest plate','orange_leather',xy,13.0-i*.6,1.25,.48)
            _stroke(m,'Chest plate tooled orange edge','gold',xy[[0,1,2]],13.75-i*.6,.22,4)
        for y in [-27,-19,-9,0,10,20]:
            m.sphere('Leather breastplate side rivet','gold',[sign*21,y,9.0],[.7,.7,.48],4,7)
        _stroke(m,'Side leather fitted seam','orange_leather',
                np.array([(20,-34),(19,-21),(20,0),(22,19),(19,28)])*[sign,1],8.7,.5,5)
    for y in [22,16,10,4,-2]:
        m.tube('Crossed orange chest fastening','gold_edge',[[-3.0,y+1.7,13.7],[3,y-1.7,13.7]],.48,6)
        m.tube('Crossed orange chest fastening','orange_leather',[[3.0,y+1.7,13.4],[-3,y-1.7,13.4]],.47,6)
    _belt(m,body,y=-17)
    # A restrained Spirit medallion engraved in the leather below the lacing.
    _stroke(m,'Spirit breastplate sun engraving','gold',[(0,-27),(4,-30),(0,-36),(-4,-30),(0,-27)],12.1,.44,5)
    for j in range(8):
        a=TAU*j/8;m.tube('Spirit sun small ray','orange_leather',[[math.sin(a)*5.8,-31+math.cos(a)*5.8,12.0],[math.sin(a)*7.5,-31+math.cos(a)*7.5,12.0]],.32,4)
    m.notes=['Spirit Breastplate canonical icon is the dark brown/orange Spirit Tunic.',
             'New fitted short-sleeved leather cuirass preserves that silhouette/palette, with overlapping orange chest plates, lacing, tooled seams and sun engraving.']
    return _finish(m)


def sages_tunic():
    m=_model('sages_tunic'); _metals(m)
    m.material('white_cloth',[.96,.965,.95],tex=_cloth([.96,.965,.95]),rough=.84)
    m.material('warm_white',[.84,.83,.77],tex=_cloth([.84,.83,.77]),rough=.87)
    m.material('lining',[.37,.38,.37],tex=_cloth([.37,.38,.37]),rough=.95)
    m.material('belt',[.78,.65,.29],tex=_cloth([.78,.65,.29]),rough=.7)
    colors=[('forest',[.22,.55,.20]),('fire',[.76,.18,.08]),('water',[.12,.41,.78]),
            ('spirit',[.94,.49,.10]),('shadow',[.46,.19,.65]),('light',[.96,.83,.24])]
    for name,col in colors:m.material(name,col,'ceramic',.35,.4)
    rings=[(-50,27,14),(-42,25.5,13.6),(-28,22,12),(-15,19,11),
           (0,20,12),(18,24,13.4),(27,24,13.2),(34,18,10),(41,10,7)]
    body=_ellipse_body(m,'White woven sage robe','white_cloth',rings,48,1.15)
    m.tube('Sage robe gold hem seam','gold',body[0],.7,7)
    _sleeve(m,-1,'white_cloth','gold');_sleeve(m,1,'white_cloth','gold')
    _collar(m,'warm_white','gold_edge');_belt(m,body,'belt','gold_edge',-15)
    # A real six-medallion chain reinforces the identity without depending on
    # externally launched particle geometry to make the white robe recognizable.
    chain=bezier([-14,30,13.3],[-12,4,14],[12,4,14],[14,30,13.3],35)
    m.tube('Six sages embroidered neck chain','gold',chain,.43,5)
    for i,(name,col) in enumerate(colors):
        x=(i-2.5)*4.7;y=10+(abs(i-2.5)/2.5)**2*8
        m.sphere('Sage medallion gold mount','gold_edge',[x,y,14.1],[2.5,2.5,.65],6,12)
        m.sphere('Individual sage enamel seal',name,[x,y,14.75],[1.87,1.87,.43],6,12)
        _stroke(m,'Medallion engraved diagonal','ivory',[(x-.8,y-.7),(x+.7,y+.8)],15.25,.20,4)
    # Six colored woven border motifs around the lower hem, visible in spin.
    for j in range(12):
        a=TAU*j/12;start=len(m.parts);name=colors[j%6][0]
        _poly(m,'Sage colored hem triangle',name,[(-2.1,-45),(2.1,-45),(0,-39)],0,.36,.12)
        _stroke(m,'Gold sage hem motif border','gold_edge',[(-2.5,-45.4),(2.5,-45.4),(0,-38.4),(-2.5,-45.4)],.35,.2,4)
        m.transform(rotation('y',math.degrees(a)),offset=[math.sin(a)*25.8,0,math.cos(a)*14.0],start=start)
    for sign in [-1,1]:
        _stroke(m,'Sage robe front gold stitched seam','gold',
                np.array([(7,27),(9,0),(10,-28),(15,-47)])*[sign,1],12.7,.22,4)
    m.notes=['Canonical Sages Tunic is white with gold fittings, empowered by the six colored medallions.',
             'New long woven robe with hollow cuffs/neck, folds, V collar, six mounted enamel seals, and a stitched six-color hem.']
    return _finish(m)


def champions_tunic():
    m=_model('champions_tunic'); _metals(m)
    m.material('champion_blue',[.06,.46,.68],tex=_cloth([.055,.54,.77]),rough=.8)
    m.material('deep_blue',[.045,.28,.43],tex=_cloth([.045,.28,.43]),rough=.82)
    m.material('white_thread',[.88,.93,.92],tex=_cloth([.88,.93,.92]),rough=.78)
    m.material('lining',[.035,.15,.23],tex=_cloth([.035,.15,.23]),rough=.94)
    m.material('belt',[.34,.19,.075],tex=_leather([.34,.19,.075]),rough=.78)
    rings=[(-42,25,13),(-34,24.1,13),(-22,21.8,12),(-13,19,11),
           (2,20.5,12),(18,24.4,13.5),(27,24,12.8),(34,18,10),(41,10,7)]
    body=_ellipse_body(m,'Blue woven Champion tunic','champion_blue',rings,48,.9)
    m.tube('Blue rolled Champion hem','deep_blue',body[0],.7,7)
    _sleeve(m,-1,'champion_blue','white_thread');_sleeve(m,1,'champion_blue','white_thread')
    _collar(m,'champion_blue','white_thread');_belt(m,body,y=-15)
    # BotW Champion shirt's unmistakable white sword and angular chest flourishes.
    _poly(m,'White embroidered chest sword blade','white_thread',
          [(0,29),(1.15,18),(1.05,3),(0,-1),(-1.05,3),(-1.15,18)],14.2,.45,.12)
    _stroke(m,'White sword chest crossguard','white_thread',[(-5,8),(-2.5,10),(0,8.8),(2.5,10),(5,8)],14.6,.54,6)
    _stroke(m,'White sword chest handle','white_thread',[(0,8.7),(0,3),(0,1)],14.6,.55,6)
    for sign in [-1,1]:
        _stroke(m,'White Champion upper flourish','white_thread',
                np.array([(3.5,24),(8,26),(14,23),(13,19),(8,19),(4,22)])*[sign,1],14.2,.39,5)
        _stroke(m,'White Champion lower flourish','white_thread',
                np.array([(3.8,12),(8,15),(16,12),(13,6),(7,5),(4,8)])*[sign,1],14.2,.40,5)
        _stroke(m,'Champion shoulder stitched panel','deep_blue',
                np.array([(13,31),(21,24),(22,7),(18,-6)])*[sign,1],10.5,.6,5)
        _stroke(m,'White Champion lower hem pattern','white_thread',
                np.array([(4,-34),(9,-29),(13,-34),(18,-29),(22,-34)])*[sign,1],12.9,.37,5)
        for x in [8,13,18]:
            _stroke(m,'Champion small white hem diamonds','white_thread',
                    np.array([(x,-37),(x+1.3,-35),(x,-33),(x-1.3,-35),(x,-37)])*[sign,1],13.4,.26,4)
    # A single diagonal leather baldric visible around the rear gives depth.
    _rear_harness(m,body)
    m.notes=["Canonical Champion's Tunic icon: cyan-blue woven shirt with white chest embroidery and warm belt.",
             'New short tunic, open collar/cuffs, cloth folds, modeled white sword/flourish stitching, patterned hem and rear leather strap.']
    return _finish(m)


def pegasus_anklet():
    m=_model('pegasus_anklet'); _metals(m)
    m.material('teal',[.05,.67,.58],'ceramic',.45,.24)
    m.material('teal_high',[.35,.91,.78],'ceramic',.5,.23)
    # A wide hollow anklet band, as shown by its own gold/teal ring icon. It is
    # deliberately an anklet rather than the old callback's recolored boots.
    start=len(m.parts)
    m.lathe('Solid gold hollow anklet band','gold',
            [(-4,31),(-3,32),(3,32),(4,31)],56,cap=False)
    m.lathe('Visible dark gold inside band','old_gold',[(-4,29.5),(4,29.5)],56,cap=False)
    _inside(m.parts[-1])
    for y in [-3.8,3.8]:m.ring('Bright rolled anklet lip','gold_edge',[0,y,0],31.4,.8,'y',56,7)
    for j in range(12):
        a=TAU*j/12;pos=np.array([32.5*math.sin(a),0,32.5*math.cos(a)])
        st=len(m.parts)
        _poly(m,'Square teal anklet bezel','gold_edge',[(-2.8,-2.8),(2.8,-2.8),(2.8,2.8),(-2.8,2.8)],0,1.4,.35)
        _poly(m,'Fitted teal enamel anklet tile','teal',[(-2.05,-2.15),(2.05,-2.15),(2.05,2.15),(-2.05,2.15)],.9,.7,.22)
        _stroke(m,'Fine turquoise tile diagonal','teal_high',[(-1.4,-1.2),(1.3,1.25)],1.35,.18,4)
        m.transform(rotation('y',math.degrees(a)),offset=pos,start=st)
        # Engraved lines between fitted enamel squares remain metal, not gems.
        a+=TAU/24
        m.tube('Anklet chased separator', 'old_gold',[[32.6*math.sin(a),-2.6,32.6*math.cos(a)],
              [32.6*math.sin(a),2.6,32.6*math.cos(a)]],.28,4)
    m.transform(rotation('x',58),offset=[-4,11,0],scale=[1.12,1,.88],start=start)
    # Three interlocking chain links descend to a cast wing charm.
    for j in range(3):
        m.ring('Interlocked hanging anklet chain link','gold_edge',
               [29-j*.8,-6-j*5,18],[2.0,3.1],.65,'z' if j%2==0 else 'x',16,6)
    _poly(m,'Pegasus charm central foot','gold_edge',
          [(25,-19),(29,-20),(29,-30),(34,-32),(33,-35),(23,-35),(21,-32),(24,-29)],18,.9,.3)
    wing=[(25,-24),(29,-23),(35,-18),(40,-17),(37,-23),(34,-26),(31,-29),(25,-28)]
    _poly(m,'Pegasus charm raised wing','gold',wing,18.4,1.0,.35)
    for j in range(3):
        _stroke(m,'Charm engraved flight feather','gold_edge',[(27+j*2,-27),(33+j*2,-23),(36+j*2,-19)],19.05,.35,5)
    m.notes=['Pegasus Anklet canonical icon is a gold hoop with turquoise fittings and a hanging wing charm.',
             'New hollow chased gold band with fitted enamel tiles, three interlocked chain links and a sculpted wing-foot charm.']
    return _finish(m)


def trident():
    m=_model('trident'); _metals(m)
    m.material('haft',[.34,.27,.11],'wood',.2,.74)
    m.material('wrap',[.13,.105,.06],tex=_leather([.13,.105,.06]),rough=.82)
    m.material('blade',[.93,.88,.67],'metal',.75,.3)
    m.material('blade_shadow',[.56,.53,.31],'metal',.8,.38)
    m.lathe('Long octagonal trident staff','haft',[(-58,1.6),(-54,2.2),(-31,2.0),(0,2.1),(29,2.4),(35,3.1)],12)
    m.lathe('Dark wrapped trident grip','wrap',[(-29,2.2),(-27,2.6),(-9,2.6),(-7,2.2)],12)
    for y in [-55,-28,-8,28,34]:
        m.lathe('Trident gold engraved collar','gold',[(y-.9,2.8),(y-.65,3.3),(y+.65,3.3),(y+.9,2.8)],12)
    for y in np.linspace(-25,-11,8):m.ring('Trident wrapped grip raised seam','old_gold',[0,y,0],2.6,.22,'y',12,4)
    m.lathe('Cast trident central socket','gold',[(29,3),(32,5.5),(38,5.5),(42,3.5)],12)
    _poly(m,'Trident socket pointed mantle','gold_edge',[(-6,37),(-4,43),(0,47),(4,43),(6,37),(0,33)],0,4,.6)
    for sign in [-1,1]:
        path=bezier([sign*2,34,0],[sign*15,31,0],[sign*21,41,0],[sign*18,48,0],18)
        m.tube('Swept golden trident fork','gold',path,np.linspace(2.5,1.7,len(path)),9)
        blade=np.array([(16,44),(22,48),(20,61),(15.5,54),(13.5,48)])*[sign,1]
        _poly(m,'Ivory outer trident spearhead','blade',blade,0,2.7,.5)
        _poly(m,'Outer spearhead darker folded facet','blade_shadow',
              np.array([(18.5,46),(20,60),(15.5,54)])*[sign,1],1.45,.3,.1)
        _stroke(m,'Trident fork fine engraving','gold_edge',
                np.array([(5,35),(13,37),(18,44)])*[sign,1],2.0,.28,4)
    _poly(m,'Long central trident spearhead','blade',[(-3.5,42),(-5,48),(0,65),(5,48),(3.5,42)],0,3.3,.6)
    _poly(m,'Central spearhead bright folded ridge','ivory',[(-.35,44),(0,64),(.85,48),(.35,44)],1.85,.4,.1)
    _stroke(m,'Central socket engraved fork','old_gold',[(-2,36),(0,40),(2,36)],2.3,.33,5)
    m.lathe('Gold trident foot ferrule','gold_edge',[(-61,1),(-60,2),(-55,2.1),(-54,1.8)],12)
    m.transform(rotation('z',-17))
    m.notes=['Canonical Trident icon: slender aged-gold staff, ivory/gold three-pronged spearhead.',
             'New long staff with wrapped grip, swept cast fork, three folded metal spear blades and engraved sockets.']
    return _finish(m)


def _foot(m,name,mat,armored=False):
    """Round heel/vamp/toe shell, with toe caps and a genuinely flat lower sole."""
    rings=[(-15,8.5,-30,10),(-11,11,-28,12),(-2,13,-25,14),
           (9,13,-27,12),(20,12,-29,10),(26,9,-30,8),(28,3,-30,4)]
    grid=[];sides=32
    for z,rx,cy,ry in rings:
        row=[]
        for j in range(sides+1):
            a=TAU*j/sides;x=rx*math.cos(a);y=max(-39,cy+ry*math.sin(a))
            row.append([x,y,z])
        grid.append(row)
    grid=np.array(grid);n=unit(np.cross(np.gradient(grid,axis=0),np.gradient(grid,axis=1)))
    radial=grid.copy();radial[:,:,2]=0;radial[:,:,1]+=29
    if np.mean(np.sum(n*radial,axis=-1))<0:n=-n
    tr=[];cols=sides+1
    for i in range(len(rings)-1):
        for j in range(sides):
            a=i*cols+j;b=a+cols;tr.extend([[a,b,a+1],[a+1,b,b+1]])
    uv=np.stack(np.meshgrid(np.linspace(0,1,cols),np.linspace(0,1,len(rings))),axis=-1)
    m.add(name,mat,grid.reshape(-1,3),n.reshape(-1,3),uv.reshape(-1,2),tr)
    # Close both end profiles; the rear view must show a finished heel rather
    # than the open construction cross-section of the vamp mesh.
    for i,sign,label in [(0,-1,'Closed sculpted boot heel'),(-1,1,'Closed sculpted boot toe')]:
        ring=grid[i]
        center=ring[:-1].mean(axis=0)
        points=np.vstack([ring,center])
        normals=np.tile([0,0,sign],(len(points),1))
        uv_cap=np.c_[points[:,0]/30+.5,(points[:,1]+30)/25+.5]
        faces=[[sides+1,j,j+1] for j in range(sides)]
        m.add(label,mat,points,normals,uv_cap,faces)
    # Flat sole uses a boot-shaped outline rather than an ellipsoid pedestal.
    xy=[(-9,-16),(9,-16),(12,-10),(14,3),(14,17),(11,27),(5,30),(-5,30),(-11,27),(-14,17),(-14,3),(-12,-10)]
    start=len(m.parts)
    _poly(m,'Thick stitched boot sole','sole',xy,0,4.3,.65)
    m.transform(rotation('x',90),offset=[0,-40,0],start=start)
    return rings


def _boot_shaft(m,mat,trim,height=42,fold=1):
    rings=[(-25,11,10),(-17,11.2,10.2),(-7,10.7,10),(5,11.8,10.6),
           (18,12.4,11),(height-5,12.9,11.6),(height,13.7,12.2)]
    grid=_ellipse_body(m,'Open leather boot shaft',mat,rings,36,fold)
    m.tube('Boot cuff rolled outer seam',trim,grid[-1],.85,7)
    m.lathe('Dark visible inside cuff','lining',[(height-8,12),(height,12)],36,cap=False)
    _inside(m.parts[-1])
    start=len(m.parts)-1;m.transform(scale=[1,1,.89],start=start)
    for y in [height-4,8,-13]:
        start=len(m.parts)
        m.lathe('Boot fitted horizontal strap',trim,[(y-2,12.65),(y+2,12.65)],32,cap=False)
        m.transform(scale=[1,1,.91],start=start)
    for y in np.linspace(-7,height-13,6):
        for sign in [-1,1]:
            m.sphere('Boot lacing eyelet','silver' if mat=='olive_leather' else 'gold_edge',[sign*4.6,y,11.8],[.75,.75,.43],4,8)
        m.tube('Crossed fitted boot lace',trim,[[-4.6,y,12],[4.6,y+4,12]],.38,5)
        m.tube('Crossed fitted boot lace',trim,[[4.6,y,12],[-4.6,y+4,12]],.38,5)


def climb_boots():
    m=_model('climb_boots'); _metals(m)
    m.material('olive_leather',[.54,.47,.11],tex=_leather([.62,.55,.14]),rough=.83)
    m.material('yellow_strap',[.81,.66,.18],tex=_leather([.81,.66,.18]),rough=.75)
    m.material('sole',[.15,.14,.085],tex=_leather([.15,.14,.085]),rough=.9)
    m.material('lining',[.18,.17,.07],tex=_cloth([.18,.17,.07]),rough=.95)
    for side in [-1,1]:
        start=len(m.parts)
        _foot(m,'Olive climbing boot toe/vamp','olive_leather',True)
        _boot_shaft(m,'olive_leather','yellow_strap',45,.7)
        # Distinct silver armored toe, heel, ankle rail, and five real crampon
        # claws per boot. These are climbing tools rather than tinted iron boots.
        m.sphere('Forged silver climbing toe cap','silver',[0,-28.5,20],[12.5,10.7,9.4],10,20)
        _poly(m,'Steel climbing cap faceplate','silver',
              [(-11,-30),(-9,-20),(0,-18),(9,-20),(11,-30),(7,-37),(-7,-37)],29.1,1.9,.6)
        _stroke(m,'Toe cap folded silver ridge','steel_dark',[(-8,-31),(0,-32),(8,-31)],30.2,.34,5)
        for x in [-9,9]:
            m.sphere('Silver toe cap rivet','silver',[x,-27,29.5],[1.25,1.25,.55],5,9)
        for sign in [-1,1]:
            st=len(m.parts)
            _poly(m,'Steel lateral ankle brace','steel_dark',[(-3,-23),(3,-23),(3,16),(-3,16)],0,1.8,.4)
            m.transform(rotation('y',sign*90),offset=[sign*12,0,-1],start=st)
            for y in [-16,10]:m.sphere('Ankle brace steel bolt','silver',[sign*13.2,y,-1],[.9,.9,.9],5,8)
        # Stepped metal hooks make the silhouette different from all wing boots.
        for x,z in [(-11,11),(11,11),(-8,24),(8,24),(0,-13)]:
            m.tube('Forged silver crampon hook','silver',
                   [[x,-38,z],[x*1.17,-41,z+2],[x*1.19,-47,z+4]], [1.5,1.35,.25],6)
        st=len(m.parts)
        m.ring('Rectangular climbing strap buckle','silver',[7.9,28,12.9],[3.2,3.1],.7,'z',4,6)
        m.tube('Climbing buckle fastening tongue','silver',[[7.9,25,13],[7.9,31,13]],.4,5)
        for zz in [-10,-2,7,16,24]:
            m.tube('Boot underside traction rib','steel_dark',[[-9,-42.3,zz],[9,-42.3,zz]],.8,6)
        m.transform(rotation('y',side*13),offset=[side*19.8,0,-side*3],start=start)
    m.notes=['Canonical Climb Boots icon: ochre/olive leather shafts with bright silver reinforced toes.',
             'New paired hollow climbing boots, leather folds, eyelet lacing, metal ankle rails, fitted toe armor and ten sculpted crampon hooks.']
    return _finish(m)


def roc_boots():
    m=_model('roc_boots'); _metals(m)
    m.material('honey_leather',[.68,.41,.105],tex=_leather([.79,.50,.15]),rough=.73)
    m.material('gold_feather',[.89,.65,.26],'feather',.65,.39)
    m.material('sole',[.42,.29,.11],'metal',.72,.57)
    m.material('lining',[.29,.18,.05],tex=_cloth([.29,.18,.05]),rough=.92)
    for side in [-1,1]:
        start=len(m.parts)
        _foot(m,'Golden Roc boot sculpted foot','honey_leather')
        _boot_shaft(m,'honey_leather','gold',43,.75)
        # Gold toe and greave are shaped armor; cloth remains readable beneath.
        m.sphere('Gold Roc boot rounded toe cap','gold_edge',[0,-29,21],[12.7,9.9,9.1],10,20)
        _poly(m,'Gold front boot greave','gold',
              [(-8,29),(-6,38),(0,43),(6,38),(8,29),(6,3),(0,-10),(-6,3)],12.4,1.15,.38)
        _stroke(m,'Greave golden feather center','gold_edge',[(0,35),(0,20),(0,4),(0,-7)],13.2,.45,5)
        for j in range(3):
            for sign in [-1,1]:
                _stroke(m,'Greave etched feather barb','old_gold',
                        np.array([(0,8+j*8),(5.6,14+j*8)])*[sign,1],13.13,.27,4)
        # A fan of broad overlapping feather vanes on the outer ankle. Each vane
        # has a real central quill and split barbs, independent of the boot shell.
        for j in range(5):
            base=np.array([side*11.5,-6+j*1.4,-4.5])
            tip=np.array([side*(24+3*j),10+6*j,-7.0-j*.5])
            direction=tip[:2]-base[:2];perp=unit([-direction[1],direction[0]])*3.9
            xy=[base[:2]-perp*.5,base[:2]+perp*.6,
                base[:2]+direction*.54+perp,tip[:2],
                base[:2]+direction*.62-perp,base[:2]+direction*.15-perp*.75]
            _poly(m,'Sculpted overlapping Roc wing feather','gold_feather',xy,base[2],1.25,.35)
            m.tube('Raised Roc feather gold quill','gold_edge',[base+[0,0,.9],tip+[0,-.7,.9]],.42,5)
            for t in [.32,.51,.68]:
                p=base+(tip-base)*t
                d=np.r_[perp*.65,0]
                m.tube('Engraved wing feather barb','old_gold',[p-d+[0,0,.82],p+[0,1,.82],p+d+[0,0,.82]],.20,4)
        # Toe engraving and a pointed gold heel echo the all-gold icon.
        for sign in [-1,1]:
            _stroke(m,'Roc toe swept engraving','old_gold',
                    np.array([(2,-24),(6,-28),(8,-34)])*[sign,1],29.8,.32,4)
        m.tube('Swept Roc boot gold heel spur','gold_edge',[[0,-27,-12],[0,-30,-19],[0,-26,-22]], [2.0,1.3,.25],7)
        m.transform(rotation('y',side*11),offset=[side*20.5,0,-side*3],start=start)
    m.notes=["Canonical Roc's Boots icon is a complete honey-gold pair with winged ankles.",
             'New golden leather boots with hollow cuffs, lacing, plated greaves/toes, dimensional feather wing fans, etched barbs and swept heel spurs.']
    return _finish(m)


def cane_of_byrna():
    m=_model('cane_of_byrna'); _metals(m)
    m.material('cobalt',[.065,.13,.63],'metal',.6,.35)
    m.material('blue_edge',[.17,.34,.88],'metal',.67,.28)
    m.material('midnight_grip',[.035,.035,.21],tex=_leather([.035,.035,.21]),rough=.82)
    m.material('blue_white',[.72,.82,.98],'ceramic',.45,.28)
    m.lathe('Slender blue Byrna shaft','cobalt',[(-57,1.3),(-53,1.8),(-30,2.0),(-6,2.05),(18,2.6),(25,3)],12)
    m.lathe('Midnight blue cane grip','midnight_grip',[(-35,2),(-33,2.35),(-17,2.35),(-15,2)],12)
    for y in [-54,-35,-15,16]:
        m.lathe('Silver Byrna shaft ferrule','silver',[(y-.6,2.2),(y-.45,2.7),(y+.45,2.7),(y+.6,2.2)],12)
    for y in np.linspace(-31,-19,6):m.ring('Blue grip winding','blue_edge',[0,y,0],2.35,.22,'y',12,4)
    # Open blue crook from the original icon; no suspended stone or Somaria ruby.
    path=np.concatenate([
        bezier([0,21,0],[0,32,0],[-1,44,0],[-9,49,0],12),
        bezier([-9,49,0],[-22,58,0],[-38,48,0],[-37,34,0],15)[1:],
        bezier([-37,34,0],[-37,22,0],[-29,18,0],[-23,23,0],12)[1:]])
    m.tube('Open cobalt Byrna crook','cobalt',path,np.linspace(3.1,2.7,len(path)),12)
    m.tube('Crook bright blue bevel','blue_edge',path+[0,0,2.75],.55,5)
    m.tube('Crook white-blue edge inlay','blue_white',path[5:17]+[0,0,3.05],.7,6)
    m.lathe('Blue cane end ferrule','silver',[(-59,1.1),(-58,1.8),(-54,1.9)],12)
    m.sphere('Byrna inward silver crook tip','blue_white',path[-1],[2.8,2.8,2.8],6,12)
    # A row of diagonal enamel shaft inlays follows the sprite's white dashes.
    for y in [-7,2,11]:
        _poly(m,'Byrna silver-blue diagonal shaft inlay','blue_white',[(-1.6,y+1),(1.6,y+4),(1.6,y+2),(-1.6,y-1)],2.1,.35,.10)
    m.transform(rotation('z',-20),offset=[8,1,0])
    m.notes=['Canonical Byrna icon: an open cobalt crook with pale angular highlights and a slender blue shaft.',
             'New metal cane with leather grip, silver ferrules, continuous blue crook, inset white-blue edge and inward capped tip; no Somaria ruby.']
    return _finish(m)


def four_sword():
    m=_model('four_sword'); _metals(m)
    m.material('blade_white',[.91,.95,.95],'metal',.9,.25)
    m.material('blade_cool',[.47,.61,.65],'metal',.9,.33)
    m.material('grip',[.31,.17,.075],tex=_leather([.31,.17,.075]),rough=.75)
    m.material('green_enamel',[.17,.45,.065],'ceramic',.44,.29)
    # A proper four-sided folded sword blade: two broad steel faces, bright
    # sharpened edges, central bevels and a point. No flattened rod substitute.
    rings=[(-15,4.5,1.35),(4,4.6,1.5),(39,4.0,1.35),(58,2.8,1.0),(66,.12,.12)]
    p=[];n=[];uv=[];tri=[]
    for i,(y,w,d) in enumerate(rings):
        for x,z in [(-w,0),(0,d),(w,0),(0,-d),(-w,0)]:
            p.append([x,y,z]);n.append(unit([x/w if w else 0,0,z/d if d else 0]));uv.append([len(p)%5/4,i/(len(rings)-1)])
    for i in range(len(rings)-1):
        for j in range(4):
            a=i*5+j;b=a+5;tri.extend([[a,b,a+1],[a+1,b,b+1]])
    m.add('Folded white Four Sword blade','blade_white',p,n,uv,tri)
    # Subtle recessed blue-steel fuller does not cover the luminous broad blade.
    _poly(m,'Cool central blade fuller','blade_cool',[(-.6,-8),(-.7,41),(0,58),(.7,41),(.6,-8)],1.55,.22,.06)
    for z in [-1.65,1.65]:
        _poly(m,'Small blade Triforce engraving','gold',[(0,-3),(2,0),(-2,0)],z,.18,.05)
    guard=[(-19,-19),(-17,-14),(-10,-13),(-5,-17),(0,-16),(5,-17),
           (10,-13),(17,-14),(19,-19),(13,-20),(6,-20),(0,-22),(-6,-20),(-13,-20)]
    _poly(m,'Four Sword golden swept crossguard','gold',guard,0,5,.7)
    _stroke(m,'Crossguard bright machined bevel','gold_edge',[(-17,-15.1),(-10,-14),(-5,-18),(0,-17.1),(5,-18),(10,-14),(17,-15.1)],2.65,.42,5)
    _poly(m,'Green enamel guard center','green_enamel',[(-3,-18),(0,-15),(3,-18),(0,-21)],2.8,.8,.2)
    m.lathe('Brown Four Sword wrapped grip','grip',[(-45,2.3),(-42,2.9),(-25,2.9),(-22,2.3)],12)
    for y in np.linspace(-41,-26,7):
        m.ring('Four Sword gold grip winding','gold',[0,y,0],2.95,.26,'y',12,4)
    for y in [-44,-24]:m.lathe('Grip gold collar','gold_edge',[(y-.9,2.8),(y-.5,3.8),(y+.5,3.8),(y+.9,2.8)],12)
    # Square green pommel and brass setting distinguish this from Master Sword.
    _poly(m,'Square golden Four Sword pommel','gold_edge',[(-4.2,-52),(4.2,-52),(4.2,-45),(-4.2,-45)],0,5,.75)
    _poly(m,'Green enamel pommel tile','green_enamel',[(-2.8,-50.8),(2.8,-50.8),(2.8,-46.1),(-2.8,-46.1)],2.8,.8,.25)
    for z in [-2.8,3.3]:_stroke(m,'Pommel engraved gold quartering','gold',[(-2,-49),(2,-49)],z,.23,4)
    m.transform(rotation('z',-22))
    m.notes=['Canonical Four Sword icon: long silver blade, gold swept guard, brown grip and green squared pommel.',
             'New diamond-section steel blade with narrow fuller, engraved Triforce, machined guard, braided grip winding and fitted green enamel pommel.']
    return _finish(m)


def pendant_of_memories():
    m=_model('pendant_of_memories');_metals(m)
    m.material('cord',[.16,.13,.085],tex=_leather([.16,.13,.085]),rough=.86)
    m.material('cord_edge',[.29,.25,.18],tex=_leather([.29,.25,.18]),rough=.8)
    m.material('blue_gem',[.055,.23,.74],'ceramic',.5,.23,emission=.09)
    m.material('blue_facet',[.19,.53,.95],'ceramic',.55,.2,emission=.09)
    m.material('blue_dark',[.035,.095,.33],'ceramic',.5,.27)
    # Verified MM game-art identity: black/brown loop, blue hexagonal gem,
    # two gold wings and a smaller dangling blue tear. This is a reconstruction
    # candidate, not a copy of the imported native mesh.
    a=np.linspace(0,TAU,81)
    path=np.c_[np.sin(a)*27,18+np.cos(a)*34,3+np.sin(a*2)*2]
    m.tube('Thin dark pendant necklace cord','cord',path,1.0,7,cap=False)
    m.tube('Cord woven highlight thread','cord_edge',path+[0,0,.85],.18,4,cap=False)
    for x in [-12,12]:
        m.ring('Golden upper pendant cord loop','gold_edge',[x,-11,3.8],[2.3,3.6],.75,'z',20,6)
        m.tube('Cord to pendant attachment','gold',[[x,-7,4],[x,-14,4]],.55,6)
    for sign in [-1,1]:
        wing=np.array([(7,-13),(15,-10),(30,-9),(26,-20),(16,-24),(7,-23)])*[sign,1]
        _poly(m,'Cast gold pendant wing panel','gold',wing,4.0,2.0,.65)
        _stroke(m,'Raised golden wing edge','gold_edge',np.vstack([wing,wing[0]]),5.2,.52,6)
        for j in range(3):
            xy=np.array([(12+j*2,-21),(16+j*3,-15),(22+j*2,-11)])*[sign,1]
            _stroke(m,'Fine engraved pendant wing fan','old_gold',xy,5.23,.25,4)
        m.sphere('Pendant outer wing stud','gold_edge',[sign*24,-12,5.35],[1.15,1.15,.55],5,10)
    hexagon=[(-8,-14),(-4.8,-10),(4.8,-10),(8,-14),(5.2,-24),(-5.2,-24)]
    _poly(m,'Central six-sided pendant gold setting','gold_edge',hexagon,5,2.5,.65)
    gem=np.asarray(hexagon);center=gem.mean(axis=0);gem=center+(gem-center)*.81
    _poly(m,'Deep blue hexagonal inset','blue_gem',gem,6.7,1.8,.48)
    # Individual broad facets catch the light as jewelry instead of a loose gem.
    for j in range(len(gem)):
        xy=[gem[j],gem[(j+1)%len(gem)],center+[0,.8]]
        _poly(m,'Blue jewel fitted facet','blue_facet' if j in [0,1,2] else 'blue_dark',xy,7.65,.32,.1)
    m.ring('Pendant lower articulated gold link','gold_edge',[0,-27,4.8],[1.8,3.0],.65,'z',18,6)
    drop=[(0,-29),(5,-34),(6,-40),(3.5,-47),(0,-52),(-3.5,-47),(-6,-40),(-5,-34)]
    _poly(m,'Teardrop pendant gold rim','gold_edge',drop,4.5,2.2,.55)
    inner=np.asarray(drop);c=inner.mean(axis=0);inner=c+(inner-c)*.76
    _poly(m,'Dangling blue tear inset','blue_gem',inner,6.0,1.6,.42)
    _poly(m,'Blue tear left luminous facet','blue_facet',[inner[0],inner[1],inner[2],c,inner[-2],inner[-1]],6.93,.25,.07)
    _poly(m,'Blue tear right deep facet','blue_dark',[inner[2],inner[3],inner[4],inner[5],c],6.91,.25,.07)
    # Engraving on the reverse distinguishes the hanging jewelry in the back view.
    _stroke(m,'Pendant reverse chased memory scroll','old_gold',[(-4,-16),(0,-13),(4,-16),(0,-20),(-4,-16)],2.9,.28,4)
    m.notes=['Canonical MM Pendant of Memories art: dark necklace loop, central blue hexagon, gold side wings and dangling blue tear.',
             'Reference verified through an image lookup; new sculpted jewelry reconstruction, not the original native archive mesh.',
             'Reference: https://www.vhv.rs/dpng/d/525-5251423_12397-pendant-of-memories-zelda-hd-png-download.png']
    return _finish(m)


BUILDERS={
    'divine_shield': divine_shield,
    'sheikah_shield': sheikah_shield,
    'shield_of_ikana': shield_of_ikana,
    'magic_cape': magic_cape,
    'spirit_breastplate': spirit_breastplate,
    'sages_tunic': sages_tunic,
    'champions_tunic': champions_tunic,
    'pegasus_anklet': pegasus_anklet,
    'trident': trident,
    'climb_boots': climb_boots,
    'roc_boots': roc_boots,
    'cane_of_byrna': cane_of_byrna,
    'four_sword': four_sword,
    'pendant_of_memories': pendant_of_memories,
}


def main():
    import meshkit
    import preview
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('items',nargs='*')
    parser.add_argument('--install',action='store_true')
    args=parser.parse_args()
    root=Path(__file__).resolve().parents[1]
    repo=root.parents[1]
    for slug in args.items or BUILDERS:
        if slug not in BUILDERS:parser.error('Unknown equipment candidate: '+slug)
        if slug == 'four_sword':
            import subprocess
            import sys
            helper=Path(__file__).with_name('forged_swords')/'rebuild.py'
            command=[sys.executable,str(helper),slug,'--output',str(root)]
            if args.install:command.append('--install')
            subprocess.run(command,check=True)
            continue
        m=BUILDERS[slug]();stats=meshkit.export_resources(m);preview.checkpoint(m,stats)
        if args.install:
            shutil.copytree(root/'RESOURCES'/m.prefix,repo/'soh/assets/custom'/m.prefix,dirs_exist_ok=True)


if __name__=='__main__':
    main()
