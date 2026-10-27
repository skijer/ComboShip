"""Bounded per-vessel palette and bottle UV treatment over original shop assets.

Only known ceramic pixel islands and ten connected glass bottles are affected.
Positions, faces, collision, alpha, material commands and other pixels stay exact.
"""
import base64
from functools import lru_cache
import hashlib
import json
from pathlib import Path
import xml.etree.ElementTree as ET
import zlib

ASSETS = Path(__file__).parent / 'assets/interior'
GEOMETRY = 'custom/skiejer_market/geometry/mahouya_room_0DL_006548'
# Bounds identify complete connected original pieces, not broad spatial boxes.
# Pixel rectangles are exclusive right/bottom in the existing counter atlas.
POTS = (
    ('right-back', ((89,8,304),(142,41,363)), 180,174, (0,0,188,174), '#759BBF'),
    ('right-middle', ((94,8,218),(143,47,267)), 152,148, (190,0,382,174), '#A7749F'),
    ('left', ((-120,0,0),(-62,54,54)), 116,106, (382,0,480,174), '#7DA893'),
    ('front-right', ((42,0,20),(105,39,79)), 94,88, (220,272,414,414), '#D5A06A'),
)
CANISTERS = (
    (((139,84,156),(154,109,174)),67,'#B694BD'),
    (((138,84,140),(155,109,155)),60,'#91B9AD'),
    (((139,84,121),(154,109,138)),67,'#CCAC81'),
)
# Sorted from the counterward end of the right-side row toward the entry.
# Each original body/neck/base is a single connected component.
BOTTLES = (
    (((133,84,195),(143,108,206)),46,42,0),
    (((138,84,213),(144,105,220)),35,30,2),
    (((148,84,226),(154,119,234)),31,30,1),
    (((142,84,238),(152,127,249)),35,30,3),
    (((136,84,252),(151,98,268)),31,30,0),
    (((145,85,269),(157,120,283)),34,30,1),
    (((140,84,300),(149,122,311)),31,30,2),
    (((133,84,312),(143,111,323)),35,30,3),
    (((138,84,329),(144,110,336)),37,30,0),
    (((148,84,342),(154,119,350)),29,30,2),
)


def rgb(color):
    return tuple(int(color[i:i+2],16) for i in (1,3,5))


def _piece(parts, bounds, vertices, triangles):
    matches = [part for part in parts if part['bounds'] == [list(p) for p in bounds]]
    if (len(matches) != 1 or len(matches[0]['indices']) != vertices or
            matches[0]['triangles'] != triangles):
        raise ValueError('Original shop prop component identity changed: ' + str(bounds))
    return matches[0]


def _map_uv(vertices, component, rect, mirror=False, swap=False):
    # A four-pixel gutter keeps native bilinear samples inside this material.
    target = ((rect[0]+4,rect[1]+4),(rect[2]-4,rect[3]-4))
    low, high = component['uv_bounds']
    for index in component['indices']:
        vertex = vertices[index]
        original = (int(vertex.get('S')),int(vertex.get('T')))
        for axis,key in enumerate(('S','T')):
            source_axis = 1-axis if swap else axis
            span = high[source_axis]-low[source_axis]
            if span <= 0:
                raise ValueError('Degenerate original prop UV span')
            unit = (original[source_axis]-low[source_axis])/span
            if axis == 0 and mirror:
                unit = 1-unit
            vertex.set(key,str(round(4*(target[0][axis] + unit*(target[1][axis]-target[0][axis])))))


def prepare_vertices(resources, prepared, components):
    """Return only these three vertex owners, retaining prior torch translations."""
    changed = {}
    for suffix in (2,14,24):
        path = f'{GEOMETRY}_vtx_{suffix}'
        root = ET.fromstring(prepared.get(path, resources[path]))
        vertices = list(root)
        parts = components(resources, f'{GEOMETRY}_tri_{suffix}')
        if suffix == 2:
            for _,bounds,count,triangles,rect,_ in POTS:
                component = _piece(parts,bounds,count,triangles)
                _map_uv(vertices,component,rect)
        elif suffix == 14:
            for index,(bounds,count,triangles,variant) in enumerate(BOTTLES):
                component = _piece(parts,bounds,count,triangles)
                # Distinct windows and alternated horizontal direction avoid
                # stamping the same highlight onto every bottle of a color.
                inset = index % 3
                rect = (variant*80+inset, index%4, (variant+1)*80-inset, 122-index%5)
                # Native S follows height and native T wraps circumference.
                # Swap axes so generated vertical glass streaks stay upright.
                _map_uv(vertices,component,rect,index%2 == 1,swap=True)
        else:
            for bounds,count,color in CANISTERS:
                component = _piece(parts,bounds,count,58)
                tint = rgb(color)
                for index in component['indices']:
                    vertex = vertices[index]
                    for channel,value in zip('RGB',tint):
                        vertex.set(channel,str((int(vertex.get(channel))*value+127)//255))
        ET.indent(root,space='  ')
        changed[path] = ET.tostring(root,encoding='utf-8') + b'\n'
    return changed


def _luminance(data, offset):
    return (77*data[offset]+150*data[offset+1]+29*data[offset+2]+128)//256


@lru_cache(maxsize=1)
def glass_strip():
    manifest = json.loads((ASSETS/'prop-glass.json').read_text())
    name = 'prop-glass.zlib.001.b64'
    if (manifest.get('version') != 1 or manifest.get('file') != name or
            manifest.get('dimensions') != [320,122] or manifest.get('decoded_bytes') != 320*122*4):
        raise ValueError('Shop glass manifest identity changed')
    text = (ASSETS/name).read_bytes()
    if len(text) > 128*1024 or hashlib.sha256(text).hexdigest() != manifest['base64_sha256']:
        raise ValueError('Shop glass encoded size/hash changed')
    packed = base64.b64decode(text,validate=True)
    if (len(packed) > 96*1024 or len(packed) != manifest['packed_bytes'] or
            hashlib.sha256(packed).hexdigest() != manifest['packed_sha256']):
        raise ValueError('Shop glass packed size/hash changed')
    decoder = zlib.decompressobj()
    pixels = decoder.decompress(packed,320*122*4+1)
    if (len(pixels) != 320*122*4 or not decoder.eof or decoder.unused_data or decoder.unconsumed_tail or
            hashlib.sha256(pixels).hexdigest() != manifest['pixels_sha256'] or
            any(alpha != 255 for alpha in pixels[3::4])):
        raise ValueError('Shop glass decoded pixels changed')
    return pixels


def texture_pixels(name, pixels):
    """Code-native palette tinting; no change to original encoded source hashes."""
    if name not in ('counter_atlas','ceramic','fittings_atlas'):
        return pixels
    if len(pixels) != 512*512*4:
        raise ValueError('Unexpected original shop texture dimensions')
    result = bytearray(pixels)
    if name == 'counter_atlas':
        for _,_,_,_,rect,color in POTS:
            tint = rgb(color)
            for y in range(rect[1],rect[3]):
                for x in range(rect[0],rect[2]):
                    offset = 4*(y*512+x)
                    level = _luminance(pixels,offset)
                    result[offset:offset+3] = bytes(min(255,(level*c+64)//128) for c in tint)
    elif name == 'ceramic':
        # Only the three material23 canister bodies use this standalone upload;
        # original atlas ceramic pixels are independent and remain separate.
        for offset in range(0,len(pixels),4):
            level = min(255,2*_luminance(pixels,offset))
            result[offset:offset+3] = bytes((level,level,level))
    else:
        strip = glass_strip()
        for y in range(122):
            result[y*512*4:(y*512+320)*4] = strip[y*320*4:(y+1)*320*4]
    return bytes(result)
