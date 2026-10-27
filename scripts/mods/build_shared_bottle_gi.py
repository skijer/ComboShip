#!/usr/bin/env python3
"""Build a reusable Peyton/TP bottle casing and empty/potion GIs for both owners.

The source is cor's recovered POC3 OoT archive. Geometry, UVs and texture pixels
stay byte-identical; only private dependency paths and scoped materials change.
The matching source patch routes this recipe through the translucent stream.
"""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import struct
import xml.etree.ElementTree as ET
import zipfile

BASE = 'objects/combo_bottle_gi/'
PREFIXES = ('objects/cor_tp_potion_gi_poc1/', 'objects/cor_tp_potion_gi_poc3/')
MM_RED_BODY = 'alt/objects/object_gi_bottle_red/gGiRedPotionBottleDL'

def digest(data):
    return hashlib.sha256(data).hexdigest()

def key(name):
    for prefix in PREFIXES:
        if name.startswith(prefix):
            return BASE + name[len(prefix):]
    raise ValueError('Unexpected bottle dependency: '+name)

def xml(root):
    ET.indent(root,space='  ')
    return ET.tostring(root)+b'\n'

def wrapper(*paths):
    root=ET.Element('DisplayList',Version='0')
    for path in paths:
        ET.SubElement(root,'CallDisplayList',Path=path)
    ET.SubElement(root,'EndDisplayList')
    return xml(root)

def compose(contents=()):
    """Contents in shell coordinates draw first; the same clear casing draws last.

    Each contents root owns its color/material/placement and restores its matrix.
    Pass a tuple of reusable contents roots; no casing geometry is duplicated.
    """
    return wrapper(*contents,BASE+'BottleShell')

def material(blob, *, alpha=None, lod=None, clear_fog=False):
    root=ET.fromstring(blob)
    if alpha is not None:
        for node in root.findall('SetPrimColor'):
            node.set('A',str(alpha))
            if lod is not None:
                node.set('L',str(lod))
    if clear_fog:
        for node in root.findall('SetGeometryMode'):
            node.attrib.pop('G_FOG',None)
        root.insert(1,ET.Element('ClearGeometryMode',G_FOG='1',G_TEXTURE_GEN_LINEAR='1'))
        for node in root.findall('SetRenderMode'):
            node.set('Mode1','G_RM_PASS')
    return xml(root)

def canonical_resources(source):
    original={name:source.read(name) for name in source.namelist() if name.startswith(PREFIXES)}
    resources={}
    preserved=[]
    for name,blob in original.items():
        target=key(name)
        if blob.startswith(b'<'):
            root=ET.fromstring(blob)
            for node in root.iter():
                if 'Path' in node.attrib:
                    node.set('Path',key(node.get('Path')))
            # Preserve vertex bytes exactly; vertices have no dependency paths.
            out=blob if root.tag=='Vertex' else xml(root)
        else:
            out=blob
        resources[target]=out
        if 'Vtx' in name or '_vtx_' in name or not blob.startswith(b'<'):
            preserved.append({'source':name,'candidate':target,'sha256':digest(blob)})
    resources[BASE+'PotionMarker']=wrapper()
    resources[BASE+'EmptyOpaque']=wrapper()
    resources[BASE+'EmptyXlu']=compose()
    # Existing fitted liquid and meniscus remain. Reduce the white response that
    # previously obscured live red/green hex values over the entire liquid volume.
    resources[BASE+'LiquidGlossMaterial']=material(resources[BASE+'LiquidGlossMaterial'],alpha=32,lod=48)
    resources[BASE+'LiquidRimMaterial']=material(resources[BASE+'LiquidRimMaterial'],alpha=72,lod=64)
    liquid=ET.fromstring(resources[BASE+'Liquid'])
    for node in liquid.findall('CallDisplayList'):
        if node.get('Path')==BASE+'CleanupOpa':
            node.set('Path',BASE+'CleanupXlu')
    resources[BASE+'Liquid']=xml(liquid)
    for name,alpha in [('shell/mat_gGiBlueFireChamberstickDL_BottleOutside',48),
                       ('shell/mat_gGiBlueFireChamberstickDL_BottleGlassOutside',32)]:
        resources[BASE+name]=material(resources[BASE+name],alpha=alpha,clear_fog=True)
    # A native MM red potion bottle has a separate GI row from potion refills.
    # Keep Cosmetic Editor's actual command 5/6 ABI, then use the common contents.
    red=ET.Element('DisplayList',Version='0')
    for node in [ET.Element('PipeSync'),ET.Element('Grayscale',Enabled='false'),
                 ET.Element('ClearGeometryMode',G_FOG='1'),ET.Element('SetAlphaCompare',Mode='0'),
                 ET.Element('PipeSync'),ET.Element('SetPrimColor',M='0',L='0',R='255',G='70',B='50',A='255'),
                 ET.Element('SetEnvColor',R='127',G='35',B='25',A='255'),
                 ET.Element('CallDisplayList',Path=BASE+'Liquid'),
                 ET.Element('CallDisplayList',Path=BASE+'BottleShell'),ET.Element('EndDisplayList')]:
        red.append(node)
    resources[BASE+'RedPotion']=xml(red)
    return resources,preserved

def aliases(game):
    empty=('gGiBottleStopperDL','gGiBottleDL') if game=='oot' else ('gGiEmptyBottleCorkDL','gGiEmptyBottleGlassDL')
    potion=('gGiPotionPotDL','gGiPotionLiquidDL','gGiPotionPatternDL') if game=='oot' else (
        'gGiPotionContainerPotDL','gGiPotionContainerLiquidDL','gGiPotionContainerPatternDL')
    routes={
        'alt/objects/object_gi_bottle/'+empty[0]:BASE+'EmptyOpaque',
        'alt/objects/object_gi_bottle/'+empty[1]:BASE+'EmptyXlu',
        'alt/objects/object_gi_liquid/'+potion[0]:BASE+'PotionMarker',
        'alt/objects/object_gi_liquid/'+potion[1]:BASE+'Liquid',
        'alt/objects/object_gi_liquid/'+potion[2]:BASE+'BottleShell',
    }
    if game=='mm':
        routes.update({
            'alt/objects/object_gi_bottle_red/gGiRedPotionBottleEmptyDL':BASE+'EmptyOpaque',
            'alt/objects/object_gi_bottle_red/gGiRedPotionBottleDL':BASE+'RedPotion',
        })
    return routes

def verify(resources,baseline,preserved,routes):
    for record in preserved:
        assert digest(resources[record['candidate']])==record['sha256']
        assert resources[record['candidate']]==baseline.read(record['source'])
    for name,blob in resources.items():
        if blob.startswith(b'<'):
            root=ET.fromstring(blob)
            if root.tag=='DisplayList':
                assert root[-1].tag=='EndDisplayList',name
            for node in root.iter():
                if 'Path' in node.attrib:
                    assert node.get('Path') in resources,(name,node.get('Path'))
        else:
            assert blob[4:8]==b'XETO',name
            fmt,width,height,flags=struct.unpack_from('<IIII',blob,64)
            payload=struct.unpack_from('<I',blob,88)[0]
            pixel_bytes=4 if flags&1 else 2 # Raw replacement RGBA32 / native RGBA16.
            assert fmt==2 and width>0 and height>0 and payload==width*height*pixel_bytes and len(blob)==92+payload
    for path,target in routes.items():
        if path == MM_RED_BODY:
            # The editor patches the native root, not a private nested wrapper.
            assert resources[path] == resources[target]
        else:
            assert ET.fromstring(resources[path])[0].get('Path')==target
    liquid=ET.fromstring(resources[BASE+'LiquidMaterial'])
    assert liquid.find('SetRenderMode').get('Mode2')=='G_RM_AA_ZB_XLU_SURF2'
    combine=liquid.find('SetCombineLERP')
    assert combine.get('Aa0')=='G_ACMUX_0' and combine.get('Ad0')=='G_ACMUX_SHADE'
    assert not liquid.findall('SetPrimColor') and not liquid.findall('SetEnvColor')
    red=ET.fromstring(resources[BASE+'RedPotion'])
    assert red[5].tag=='SetPrimColor' and red[6].tag=='SetEnvColor'
    if MM_RED_BODY in resources:
        for rgb in [(12,210,250),(230,93,171),(30,190,70)]:
            native=ET.fromstring(resources[MM_RED_BODY])
            assert native[5].tag=='SetPrimColor' and native[6].tag=='SetEnvColor'
            # Apply the actual native editor slots, then observe the color at
            # entry to the common liquid. A two-command alias would fail here.
            for slot,color in [(5,rgb),(6,tuple(c//2 for c in rgb))]:
                for component,value in zip(('R','G','B'),color):
                    native[slot].set(component,str(value))
            prim=env=None
            for node in native:
                if node.tag=='SetPrimColor': prim=tuple(int(node.get(c)) for c in ('R','G','B'))
                if node.tag=='SetEnvColor': env=tuple(int(node.get(c)) for c in ('R','G','B'))
                if node.tag=='CallDisplayList' and node.get('Path')==BASE+'Liquid':
                    assert prim==rgb and env==tuple(c//2 for c in rgb)
                    break
            else:
                raise AssertionError('Native red bottle never calls the common liquid')
    return {'closed_dependencies':True,'geometry_uv_texture_bytes_preserved':len(preserved),
            'translucent_liquid':True,'live_palette_not_overridden':True,'mm_red_color_slots_valid':True,
            'runtime_visual_acceptance':'untested'}

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-o2r',required=True,type=Path)
    parser.add_argument('--output',required=True,type=Path)
    args=parser.parse_args(); args.output.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(args.source_o2r) as source:
        assert source.testzip() is None
        common,preserved=canonical_resources(source)
        for game in ['oot','mm']:
            resources=copy.deepcopy(common); routes=aliases(game)
            resources.update({name:wrapper(target) for name,target in routes.items()})
            if game=='mm':
                resources[MM_RED_BODY]=resources[BASE+'RedPotion']
            checks=verify(resources,source,preserved,routes)
            name='ComboShip_Shared_Bottle_GI_POC4_'+('OoT' if game=='oot' else 'MM')+'.o2r'
            out=args.output/name
            with zipfile.ZipFile(out,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as archive:
                for path,blob in sorted(resources.items()):
                    info=zipfile.ZipInfo(path,date_time=(2026,10,7,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED
                    archive.writestr(info,blob)
            with zipfile.ZipFile(out) as archive:
                assert archive.testzip() is None
            manifest={'candidate':name,'source':args.source_o2r.name,'source_sha256':digest(args.source_o2r.read_bytes()),
                      'archive_sha256':digest(out.read_bytes()),'routes':routes,'checks':checks,'preserved':preserved,
                      'resources':{p:digest(b) for p,b in sorted(resources.items())}}
            (args.output/(name+'.manifest.json')).write_text(json.dumps(manifest,indent=2)+'\n')
            print(name+': '+str(len(resources))+' resources; closed graph, preserved geometry/UV/textures, translucent/live-palette materials')

if __name__=='__main__':main()
