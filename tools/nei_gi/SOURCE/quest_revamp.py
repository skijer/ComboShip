"""Remaining quest GIs, authored through the accepted NEI mesh pipeline.

Distinct meshes/materials replace placeholder/recolored GIs. Actor/held resources
are deliberately separate. All previews and game resources share triangles.
"""
import math
import numpy as np
from meshkit import Model, TAU, rotation
from models import metals, band


def model(slug, name):
    m = Model(slug, name, 'objects/nei_gi_redesign/'+slug+'/gi_dl', 1., .65)
    metals(m)
    m.material('leather', [.16,.085,.035], 'braid', rough=.85)
    m.material('stone', [.14,.16,.19], 'stone', rough=.8)
    m.material('cyan', [.19,.79,.98], 'energy', emission=.6)
    m.material('glass', [.57,.86,.98], alpha=.22, rough=.1)
    return m


def rgb(hex_value):
    return [((hex_value >> shift) & 255) / 255 for shift in (16, 8, 0)]


WANDS = {
    'elemental_wand': ('Elemental Wand', [.53,.86,1.]),
    'sand_rod': ('Sand Rod', rgb(0xE8AC48)),
    'tornado_rod': ('Tornado Rod', rgb(0x91EFBC)),
    'water_rod': ('Water Rod', rgb(0x38A6FF)),
    'meteor_rod': ('Meteor Rod', rgb(0xFF4818)),
    'storm_rod': ('Storm Rod', rgb(0xB5A8FF)),
    'shadow_scepter': ('Shadow Scepter', rgb(0x9E38DA)),
}


def wand(slug):
    name, color = WANDS[slug]
    m = model(slug, name)
    m.material('focus', color, 'energy', emission=.65, rough=.2)
    m.material('inlay', color, 'ceramic', metal=.35, rough=.25)
    m.lathe('Leather wrapped grip','leather',[(-48,3),(-44,4),(-13,3.8),(12,3),(22,4.8)],16)
    for y,r in [(-45,4.4),(-13,4.7),(17,5.1),(23,6.6)]:
        band(m,'Engraved metal collar','gold',y,r,1.4)
        band(m,'Polished collar edge','edge',y+1.2,r+.2,.3)
    m.crystal('Inset elemental pommel','focus',[0,-49,0],4.5,8,6)
    for a in (0,math.pi):
        t=np.linspace(0,1,20)
        m.tube('Spiral elemental shaft inlay','inlay',
               np.c_[4*np.cos(a+t*TAU),-12+32*t,4*np.sin(a+t*TAU)],.42,5)
    if slug == 'sand_rod':
        m.material('sandstone',[.66,.40,.16],'stone',rough=.9)
        m.lathe('Carved desert urn crown','sandstone',[(24,7),(27,10),(35,12),(42,8),(46,9)],16)
        for y,r in [(27,10),(42,9)]: band(m,'Urn gold rim','edge',y,r,.8)
        m.crystal('Amber sand focus','focus',[0,44,0],6,16,6)
        for j in range(6):
            a=TAU*j/6
            m.tube('Carved desert sun ray','edge',[[11*math.cos(a),33,11*math.sin(a)],
                   [16*math.cos(a),39,16*math.sin(a)]],.65,5)
    elif slug == 'tornado_rod':
        t=np.linspace(0,1,64);r=12*(1-t)+4
        m.tube('Open rising cyclone crown','steel',
               np.c_[r*np.cos(t*TAU*2.5),24+30*t,r*np.sin(t*TAU*2.5)],1.1,6)
        m.crystal('Wind heart','focus',[0,39,0],5,16,6)
    elif slug == 'water_rod':
        m.lathe('Glass droplet focus','focus',[(25,1),(29,8),(36,11),(43,8),(53,0.4)],20)
        for j in range(3):
            a=TAU*j/3;t=np.linspace(0,1,18);r=6+8*np.sin(t*math.pi)
            m.tube('Wave crest prong','steel',
                   np.c_[r*np.cos(a+t*.6),24+24*t,r*np.sin(a+t*.6)],np.linspace(1.3,.3,len(t)),6)
    elif slug == 'meteor_rod':
        m.material('obsidian',[.12,.08,.06],'stone',rough=.55)
        m.crystal('Split volcanic meteor','obsidian',[0,40,0],13,25,7)
        m.crystal('Exposed molten heart','focus',[0,40,10],5,18,5)
        for j in range(4):
            a=TAU*j/4;t=np.linspace(0,1,12);r=5+10*np.sin(t*math.pi*.8)
            m.tube('Forged crater claw','gold',np.c_[r*np.cos(a),23+26*t,r*np.sin(a)],
                   np.linspace(2,.3,len(t)),6)
    elif slug == 'storm_rod':
        for sign in (-1,1):
            start=len(m.parts)
            m.polygon('Lightning fork','edge',[(sign*3,23),(sign*14,38),(sign*8,38),
                      (sign*17,53),(sign*2,40),(sign*7,40)],depth=3,bevel=.6)
        m.crystal('Storm violet focus','focus',[0,36,0],5,19,6)
    elif slug == 'shadow_scepter':
        m.crystal('Black-violet shadow heart','focus',[0,40,0],7,24,6)
        for sign in (-1,1):
            m.polygon('Swept shadow wing','dark',[(sign*3,26),(sign*17,34),(sign*19,47),
                      (sign*13,42),(sign*10,34)],depth=4,bevel=1)
            m.tube('Wing silver edge','steel',[[sign*4,27,2],[sign*16,35,2],[sign*18,45,2]],.7,5)
    else:
        m.sphere('Prismatic elemental focus','focus',[0,38,0],[9,9,9],8,16)
        m.ring('Six-mode focus halo','edge',[0,38,0],15,1.2,segments=30,sides=6)
        for j,(_,col) in enumerate(list(WANDS.values())[1:]):
            mat='mode_'+str(j);m.material(mat,col,'energy',emission=.4)
            a=TAU*j/6;m.crystal('Elemental mode inset',mat,[15*math.cos(a),38+15*math.sin(a),0],2.7,6,5)
    m.transform(rotation('z',-14))
    m.markers['tip_author']=(np.array([0,39,0])@rotation('z',-14).T).tolist()
    m.notes=['New wand silhouette and material candidate, derived from each elemental theme.',
             'No Dominion Rod stand-in; GI resources only, held/action behavior preserved.']
    return m


RUNES = {
    'sheikah_slate': ('Sheikah Slate', rgb(0x30CAFA)),
    'slate_bomb': ('Remote Bomb Rune', rgb(0x5FDCEB)),
    'slate_master_cycle': ('Master Cycle Rune', rgb(0x64E6BE)),
    'slate_stasis': ('Stasis Rune', rgb(0xFAC846)),
    'slate_cryonis': ('Cryonis Rune', rgb(0x96D7FF)),
    'slate_sensor': ('Sheikah Sensor Rune', rgb(0xC882FF)),
}


def slate(slug):
    name,color=RUNES[slug];m=model(slug,name)
    m.material('glyph',color,'energy',emission=.65)
    m.material('casing',rgb(0x292B30),'stone',rough=.75)
    m.material('bronze',rgb(0xB08A50),'metal',metal=.65,rough=.42)
    m.material('amber',rgb(0xFFAF32),'energy',emission=.7)
    m.material('pupil',rgb(0x081019),rough=.3)
    m.material('wrap',rgb(0xA38150),'braid',rough=.85)
    outline=[(-23,-32),(23,-32),(28,-27),(28,26),(22,34),(-22,34),(-28,26),(-28,-27)]
    m.polygon('Thick chamfered ancient casing','casing',outline,depth=9,bevel=2.5,closed_sides=True)
    # Localized fittings leave the charcoal silhouette visible like the reference.
    for side in (-1,1):
        m.tube('Bronze casing side rail','bronze',[[side*24,-28,4.6],[side*26,-24,4.6],
               [side*26,23,4.6],[side*21,30,4.6]],.75,6)
        m.tube('Rear casing rail','bronze',[[side*24,-28,-4.6],[side*26,-24,-4.6],
               [side*26,23,-4.6],[side*21,30,-4.6]],.6,6)
    # Broad raised lids and central iris form a sculpted emblem, rather than a screen.
    for sign in (-1,1):
        t=np.linspace(0,math.pi,22)
        m.tube('Raised bronze Sheikah eyelid','bronze',
               np.c_[17*np.cos(t),5+sign*9*np.sin(t),np.full(len(t),5.2)],1.65,7)
    m.sphere('Raised bronze iris bezel','bronze',[0,5,5.1],[7.1,9,1.7],8,20)
    m.sphere('Luminous rune iris','glyph',[0,5,6.5],[5.25,7,1],8,20)
    m.sphere('Inset dark pupil','pupil',[0,5,7.35],[2.1,3.35,.4],6,16)
    m.polygon('Bronze eye tear','bronze',[(-3,-6),(3,-6),(0,-13)],depth=1.5,z=5.2,bevel=.4,closed_sides=True)
    # Three amber forehead triangles and fine bronze circuit traces.
    for x,y in [(-11,22),(0,26),(11,22)]:
        m.polygon('Bronze triangle setting','bronze',[(x-3,y+3),(x+3,y+3),(x,y-3)],depth=1.3,z=5,bevel=.35,closed_sides=True)
        m.polygon('Amber triangular inlay','amber',[(x-1.5,y+1.5),(x+1.5,y+1.5),(x,y-1.5)],depth=.4,z=5.9,bevel=.15,closed_sides=True)
    for sign in (-1,1):
        for y in (-24,-9,18):
            m.tube('Branching bronze circuitry','bronze',[[sign*15,y+4,5],[sign*21,y+4,5],
                   [sign*21,y,5],[sign*23,y-1,5]],.75,6)
            m.sphere('Circuit terminal bronze socket','bronze',[sign*23,y-1,5],[1.65,1.65,.8],5,10)
            m.sphere('Amber circuit terminal','amber',[sign*23,y-1,5.75],[.7,.7,.25],4,8)
    # Squared handle with a wrapped central grip; opening remains visible.
    handle=[[-14,32,0],[-14,39,0],[-10,44,0],[10,44,0],[14,39,0],[14,32,0]]
    m.tube('Heavy bronze carrying handle','bronze',handle,2.2,8)
    m.tube('Wrapped handle grip','wrap',[[-9,44,0],[9,44,0]],2.5,10)
    t=np.linspace(0,1,80)
    m.tube('Spiral grip binding','edge',np.c_[-9+18*t,44+2.6*np.cos(t*TAU*7),2.6*np.sin(t*TAU*7)],.45,5)
    for sign in (-1,1):
        m.polygon('Handle mounting bracket','bronze',[(sign*10,29),(sign*17,29),
                  (sign*17,34),(sign*10,34)],depth=10,bevel=.8,closed_sides=True)
    # Rear seams and inset eye make a rotating pickup readable from both sides.
    for sign in (-1,1):
        t=np.linspace(0,math.pi,18)
        m.tube('Rear bronze eye relief','bronze',np.c_[11*np.cos(t),4+sign*5*np.sin(t),
               np.full(len(t),-4.8)],.65,5)
    m.sphere('Rear rune seal','glyph',[0,4,-5],[2.2,3,.5],6,12)
    for y in (-22,23):
        m.tube('Rear engraved casing seam','bronze',[[-18,y,-4.8],[0,y-2,-4.8],[18,y,-4.8]],.45,5)
    # A real raised rune badge below the eye, readable while the item rotates.
    if slug=='slate_bomb':
        m.sphere('Remote bomb badge','glyph',[0,-20,5.2],[4,4,1],6,12)
        m.tube('Bomb fuse','edge',[[0,-16,5.6],[1,-13,5.6],[3,-12,5.6]],.5,5)
    elif slug=='slate_master_cycle':
        for x in (-8,8):m.ring('Cycle wheel','glyph',[x,-20,5.2],3.5,.65,segments=16,sides=5)
        m.tube('Cycle chassis','glyph',[[-8,-20,5.2],[-2,-15,5.2],[5,-16,5.2],[8,-20,5.2]],.7,5)
    elif slug=='slate_stasis':
        for x in (-3.5,3.5):m.polygon('Stasis pause bar','glyph',[(x-1,-24),(x+1,-24),(x+1,-16),(x-1,-16)],depth=.6,z=5.2,bevel=.2,closed_sides=True)
    elif slug=='slate_cryonis':
        for x,h in [(-7,5),(0,9),(7,6)]:m.polygon('Cryonis pillar','glyph',[(x-2,-24),(x+2,-24),(x+2,-24+h),(x-2,-24+h)],depth=.7,z=5.2,bevel=.2,closed_sides=True)
    elif slug=='slate_sensor':
        for r in (3,6,9):
            t=np.linspace(-.8,.8,16)
            m.tube('Sensor signal arc','glyph',np.c_[r*np.sin(t),-23+r*np.cos(t),np.full(len(t),5.2)],.5,5)
    m.notes=['Upright BotW-inspired charcoal casing, sculpted bronze eye, amber circuitry and wrapped handle.',
             'Rune iris and badge use the same hex palette as the shared GI effect policy.',
             'New three-dimensional GI; original gameplay slate resource remains separate.']
    return m


def hourglass():
    m=model('phantom_hourglass','Phantom Hourglass')
    m.material('sand',[.94,.78,.39],'stone',rough=.9)
    m.material('glass',[.70,.92,1.],alpha=.20,rough=.08)
    for sign in (-1,1):
        start=len(m.parts)
        m.lathe('Engraved hourglass foot','gold',[(24,11),(26,17),(29,18),(31,15)],24)
        band(m,'Foot polished lip','edge',29,18,.75)
        m.transform(rotation("x",180) if sign<0 else np.eye(3),start=start)
    # Transparent glass has a narrow waist, with two solid sand heaps inside.
    m.lathe('Clear hourglass bulbs','glass',[(-24,11),(-18,13),(-10,8),(-2,2),(2,2),(10,8),(18,13),(24,11)],28,False)
    m.lathe('Lower sand heap','sand',[(-23,10),(-18,9),(-13,5),(-8,.3)],24)
    m.lathe('Upper retained sand','sand',[(10,.3),(16,7),(20,10)],24)
    for j in range(4):
        a=TAU*j/4+.45;r=15.5
        m.tube('Twisted hourglass pillar','gold',[[r*math.cos(a),-26,r*math.sin(a)],
               [r*math.cos(a+.1),0,r*math.sin(a+.1)],[r*math.cos(a),26,r*math.sin(a)]],1.25,8)
        for y in (-21,21):m.sphere('Pillar joint','edge',[r*math.cos(a),y,r*math.sin(a)],[1.8,1.8,1.8],4,8)
    m.notes=['Open framed glass bulbs and visible sand, not a gem placeholder.',
             'Animated falling grains are a separate deterministic GI effect.']
    return m


def shadow_crystal():
    """Repackage the preserved black/orange mesh; do not reinterpret its shape."""
    from pathlib import Path
    import xml.etree.ElementTree as ET
    assets=Path(__file__).resolve().parents[3]/'soh/assets/custom'
    prefix='objects/object_nei_shadow_crystal/'
    m=Model('shadow_crystal','Shadow Crystal',
            'objects/nei_gi_redesign/shadow_crystal/gi_dl',1.,.5)
    for group in range(3):
        material=ET.parse(assets/(prefix+f'mat_shadow_crystal_{group}')).getroot()
        prim=next(command for command in material if command.tag=='SetPrimColor')
        name=f'original_{group}'
        m.material(name,[int(prim.get(channel))/255 for channel in ('R','G','B')],
                   metal=.35,rough=.2)
        positions=[];normals=[];uvs=[];triangles=[];cache={}
        for command in ET.parse(assets/(prefix+f'shadow_crystal_tri_{group}')).getroot():
            if command.tag=='LoadVertices':
                vertices=list(ET.parse(assets/command.get('Path')).getroot())
                offset,count,start=(int(command.get(key)) for key in
                                    ('VertexOffset','Count','VertexBufferIndex'))
                for index in range(count):
                    vertex=vertices[offset+index]
                    cache[start+index]=len(positions)
                    positions.append([int(vertex.get(axis)) for axis in ('X','Y','Z')])
                    normals.append([(((int(vertex.get(channel))+128)%256)-128)/127
                                    for channel in ('R','G','B')])
                    uvs.append([int(vertex.get(axis))/1024 for axis in ('S','T')])
            elif command.tag in ('Triangle1','Triangles2'):
                fields=[('V00','V01','V02')]
                if command.tag=='Triangles2':fields.append(('V10','V11','V12'))
                triangles.extend([[cache[int(command.get(key))] for key in fields_one]
                                  for fields_one in fields])
        p=np.asarray(positions,float);tri=np.asarray(triangles,int)
        # Omit only the two already-collapsed source faces. Preserve original
        # winding and byte-normal values rather than recalculating/smoothing.
        visible=np.any(np.cross(p[tri[:,1]]-p[tri[:,0]],p[tri[:,2]]-p[tri[:,0]]),axis=1)
        m.parts.append(dict(name=f'Original Shadow Crystal material {group}',mat=name,
                            p=p,n=np.asarray(normals),uv=np.asarray(uvs),tri=tri[visible]))
    m.notes=['Preserved black/orange Shadow Crystal mesh: original material groups, normals, UVs and winding.',
             'All 614 renderable source triangles retained; two pre-existing zero-area faces omitted.',
             'Opaque GI only; no interpreted violet crystal shell. Item ownership and Wolf Link behavior unchanged.']
    return m


SEASONS = {'rod_of_seasons': ('Rod of Seasons', [.78,.30,.22])}


def season_rod(slug):
    name,color=SEASONS[slug];m=model(slug,name)
    m.material('wood',[.27,.12,.045],'wood',rough=.8)
    m.material('season',color,'energy',emission=.5)
    m.lathe('Carved seasonal staff','wood',[(-46,3.5),(-40,4),(-13,3.7),(13,4),(23,6)],16)
    for y,r in [(-44,4.8),(-13,5),(17,5.5),(25,8)]:band(m,'Gold staff collar','gold',y,r,1.5)
    m.crystal('Seasonal focus','season',[0,39,0],9,24,6)
    m.ring('Season crown medallion','gold',[0,37,0],17,1.1,segments=32,sides=6)
    for j in range(4):
        a=TAU*j/4;t=np.linspace(0,1,12)
        r=6+11*np.sin(t*math.pi*.8)
        m.tube('Living branch crown','gold',np.c_[r*np.cos(a+t*.3),23+28*t,r*np.sin(a+t*.3)],np.linspace(1.9,.3,len(t)),6)
    for j,col in enumerate([[.22,.83,.40],[.95,.32,.18],[.97,.63,.19],[.35,.72,1.]]):
        mat='gem_'+str(j);m.material(mat,col,'energy',emission=.4)
        a=TAU*j/4;m.crystal('Four-season crown stone',mat,[17*math.cos(a),37+17*math.sin(a),0],3.5,8,6)
    m.transform(rotation('z',-12))
    m.notes=['Physical Rod of Seasons: carved wood, metal fittings and four inset crown stones.',
             'Seasonal/weather GIs and their effects are handled in a separate workstream.']
    return m


BUILDERS={**{s:(lambda s=s:wand(s)) for s in WANDS},
          **{s:(lambda s=s:slate(s)) for s in RUNES},
          **{s:(lambda s=s:season_rod(s)) for s in SEASONS},
          'phantom_hourglass':hourglass,'shadow_crystal':shadow_crystal}


def main():
    import argparse
    from pathlib import Path
    from build_completion import build
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('items',nargs='*')
    parser.add_argument('--install',action='store_true')
    args=parser.parse_args()
    names=args.items or list(BUILDERS)
    for name in names:
        if name not in BUILDERS:parser.error('Unknown quest GI candidate: '+name)
    if 'shadow_crystal' in names:
        # Rebuilding an opaque restoration must not leave the previous violet
        # shell in the resource tree or in the derived fit catalog.
        import shutil
        root=Path(__file__).resolve().parents[1]
        obsolete=[root/'RESOURCES/objects/nei_gi_redesign/shadow_crystal']
        if args.install:obsolete.append(root.parents[1]/'soh/assets/custom/objects/nei_gi_redesign/shadow_crystal')
        for directory in obsolete:
            if directory.is_dir():shutil.rmtree(directory)
    build({name:BUILDERS[name] for name in names},Path(__file__).resolve().parents[1],args.install)


if __name__=='__main__':main()
