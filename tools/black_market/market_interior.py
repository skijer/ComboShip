#!/usr/bin/env python3
"""Original default market surfaces over the retained, immutable room geometry.

Dedicated original architecture and prop surfaces follow the retained draw
owners and UV islands. Optional authoring rebakes encode 512px RGBA uploads;
ordinary preparation needs only the standard library.
"""
import hashlib
import base64
import json
from functools import lru_cache
from pathlib import Path
import re
import struct
import xml.etree.ElementTree as ET
import zlib
import prop_appearance

ASSETS = Path(__file__).parent / 'assets/interior'
PART_BYTES = 96 * 1024  # Each base64 source file is at most 128 KiB.
PREFIX = 'custom/skiejer_market/texture/'
BASE_NAMES = ('timber', 'slate', 'plaster')
PROP_NAMES = ('ceramic', 'bronze', 'wicker', 'celestial', 'herbs', 'curios', 'liquid', 'cloth')
DERIVED_NAMES = ('counter_atlas', 'fittings_atlas', 'walltrim_atlas')
TEXTURES = {name: PREFIX + 'baseline_' + name for name in (*BASE_NAMES, *PROP_NAMES, *DERIVED_NAMES)}
SHELF = PREFIX + 'arcane_stone'
SOURCE_HASHES = {
    'bronze': 'c5d17c93cb7c79a2d5fdda77cd5791220bef417fae24b85881d9427e2176ec62',
    'timber': '62dc29aef9b2e4b0194a5c2ce25e1bff79139deb2ab3e9a74642555d9207ec7c',
    'slate': 'e91d3c7ef5d1e78dabaadf138d76db2cd6b74c3ca7e3dca0890fd2b4d4e408db',
    'plaster': 'aef51704902f9144fb88d3492667fc143ee77c976a192bae2a7cdb0d76688b84',
    'ceramic': 'a01ed02e1ea1c108c3a74790bbf6e8481c590e7c26be106a811b60ad0f1ef6be',
    'wicker': '52cd841c267cfcfc41598c1122b4de32c854aa68f90e8cdf5135e53665079ea8',
    'celestial': 'da370b831378ac8cdea60c3e7890324e80e6c72a6a1f4143e17c05db0aa1356f',
    'herbs': '8d5e3c2f546745b171cf8f48a3a9d82f90613fa01106ba8ff6f1b5ca2f1e66f7',
    'curios': '2ffe56c06fa7f81cd94ad3e0dd302c6e8cc5b56f3fbd5cafb1c46f10f8be3c90',
    'liquid': '22ab45941f50f8963e24cf1fd34fb2f4b9bd3d36336a4998a0f304e76b59df32',
    'cloth': '93ecac406f9238a7756a0a901619f3b9f5173b2f4217bc7fac0a5172a87d73bf',
}
# Material IDs differ from the retained tri/vtx suffixes. Follow the actual
# room draw graph, not numeric suffixes or broad spatial bounds alone.
MATERIAL_ROLES = {
    0: {'surface':'liquid', 'role':'front-right cauldron liquid'},
    1: {'surface':'slate', 'role':'entry floor and threshold'},
    2: {'surface':'bronze', 'role':'overhead hanging supports'},
    3: {'surface':'timber', 'role':'ceiling and structural panels'},
    4: {'surface':'herbs', 'role':'hanging bulb/root cluster'},
    5: {'surface':'fittings_atlas', 'role':'small bottles, basins and cabinet face'},
    6: {'surface':'cloth', 'role':'left wall hanging panel'},
    7: {'surface':'cloth', 'role':'rear draped curtain'},
    8: {'surface':'timber', 'role':'right wall ledges'},
    9: {'surface':'plaster', 'role':'wall infill panels'},
    10: {'surface':'wicker', 'role':'seven hanging herb baskets'},
    11: {'surface':'walltrim_atlas', 'role':'wall panels, upper bundles/trim and small inserts'},
    12: {'surface':'curios', 'role':'small jars, bone curio and paper/label islands'},
    13: {'surface':'wicker', 'role':'three right-side lattice screens'},
    14: {'surface':'herbs', 'role':'long hanging herb leaves'},
    15: {'surface':'celestial', 'role':'three faceted celestial star ornaments'},
    16: {'surface':'liquid', 'role':'middle-right and left cauldron liquids'},
    17: {'surface':'celestial', 'role':'inactive retained material; no default draw owner'},
    18: {'surface':'curios', 'role':'open book pages and standing leather books'},
    19: {'surface':'timber', 'role':'room framing, cabinet wood and boundary repairs'},
    20: {'surface':'herbs', 'role':'short hanging herb leaves'},
    21: {'surface':'counter_atlas', 'role':'four cauldron bodies, bronze torch stands and counter fittings'},
    22: {'surface':'cloth', 'role':'floor runner'},
    23: {'surface':'ceramic', 'role':'three capped ceramic canisters'},
    24: {'surface':'slate', 'role':'main floor and boundary floor repair'},
    25: {'surface':'herbs', 'role':'right-side hanging leaves and small retained insert'},
    26: {'surface':'curios', 'role':'paper panels and bone/curio display islands'},
    27: {'surface':'liquid', 'role':'entry-right cauldron liquid'},
}

# Pixel rectangles in the 512px upload, measured from existing 64-unit UVs.
# Packing flat imagegen material patches preserves every vertex S/T. The
# counter's bronze strip and four glaze islands do not share their role UVs.
ATLAS_RECTS = {
    'counter_atlas': {'ceramic': [[0,0,512,174], [220,272,414,414], [316,174,382,272]],
                      'bronze': [416,184,512,512], 'leather': [188,416,412,512]},
    'fittings_atlas': {'glass': [0,0,320,122], 'ceramic': [[0,332,480,512]]},
    'walltrim_atlas': {'wicker': [0,318,512,512], 'paper': [0,188,250,316]},
}

GEOMETRY = 'custom/skiejer_market/geometry/mahouya_room_0DL_006548'
CURIO_UV_GROUPS = {5: 'books', 8: 'jars/skull/papers', 12: 'skull details/posters'}
# Whole disjoint native stand components; the source bundle stays immutable.
# Front-facing shelves hid the original flames at Z=-8. Audited gaps in front
# of the shelves avoid the right pot bank, canisters and overhead baskets.
TORCH_STANDS = (
    {'bounds': [[111,0,-19],[145,83,13]], 'delta': [-55,0,213], 'vertices': 78},
    {'bounds': [[-156,0,-19],[-122,83,13]], 'delta': [29,0,213], 'vertices': 73},
)


def vertex_components(resources, triangle_path):
    """Connected triangles, welding duplicate native positions for role audit."""
    cache, faces = {}, []
    for node in ET.fromstring(resources[triangle_path]):
        if node.tag == 'LoadVertices':
            path = node.get('Path')
            vertices = list(ET.fromstring(resources[path]))
            start, offset, count = (int(node.get(key)) for key in ('VertexBufferIndex','VertexOffset','Count'))
            for index, vertex in enumerate(vertices[offset:offset+count]):
                cache[start+index] = (path, offset+index, tuple(int(vertex.get(key)) for key in ('X','Y','Z','S','T')))
        elif node.tag in ('Triangle1','Triangles2'):
            for face in range(2 if node.tag == 'Triangles2' else 1):
                faces.append([cache[int(node.get(f'V{face}{index}'))] for index in range(3)])
    parent = {point[:3]:point[:3] for face in faces for _,_,point in face}
    def find(point):
        while parent[point] != point:
            parent[point] = parent[parent[point]]
            point = parent[point]
        return point
    for face in faces:
        root = find(face[0][2][:3])
        for _,_,point in face[1:]:
            parent[find(point[:3])] = root
    groups = {}
    for face in faces:
        groups.setdefault(find(face[0][2][:3]), []).extend(face)
    result = []
    for vertices in groups.values():
        points = [point for _,_,point in vertices]
        paths = {path for path,_,_ in vertices}
        if len(paths) != 1:
            raise ValueError('Mixed vertex owners in retained prop component')
        result.append({'path':paths.pop(), 'indices':sorted({index for _,index,_ in vertices}),
                       'bounds':[[op(point[axis] for point in points) for axis in range(3)] for op in (min,max)],
                       'uv_bounds':[[op(point[axis] for point in points) for axis in (3,4)] for op in (min,max)],
                       'triangles':len(vertices)//3})
    return sorted(result, key=lambda component:component['indices'][0])


def prepare_prop_vertices(resources):
    """Bounded role UVs and two whole-fixture moves, preserving every face."""
    changed = {}
    quadrants = {'paper':(32,32), 'leather':(1056,32), 'bone':(32,1056), 'glass':(1056,1056)}
    for suffix in CURIO_UV_GROUPS:
        components = vertex_components(resources, f'{GEOMETRY}_tri_{suffix}')
        path = f'{GEOMETRY}_vtx_{suffix}'
        root = ET.fromstring(resources[path])
        vertices = list(root)
        for component in components:
            low, high = component['bounds']
            if suffix == 5:
                role = 'paper' if low[0] >= -102 else 'leather'
            elif suffix == 8:
                role = ('bone' if component['bounds'] == [[118,74,171],[140,120,189]] else
                        'paper' if low[0] == high[0] == 159 or high[0] < -156 else 'glass')
            else:
                role = 'paper' if high[2] < 0 else 'bone'
            origin = quadrants[role]
            uv_low, uv_high = component['uv_bounds']
            for index in component['indices']:
                vertex = vertices[index]
                for axis,key in enumerate(('S','T')):
                    extent = uv_high[axis]-uv_low[axis]
                    value = int(vertex.get(key))
                    vertex.set(key, str(origin[axis] + (round((value-uv_low[axis])*960/extent) if extent else 480)))
        ET.indent(root, space='  ')
        changed[path] = ET.tostring(root, encoding='utf-8') + b'\n'
    components = vertex_components(resources, f'{GEOMETRY}_tri_2')
    root = ET.fromstring(resources[f'{GEOMETRY}_vtx_2'])
    vertices = list(root)
    for spec in TORCH_STANDS:
        matches = [component for component in components if component['bounds'] == spec['bounds']]
        if len(matches) != 1 or matches[0]['triangles'] != 68 or len(matches[0]['indices']) != spec['vertices']:
            raise ValueError('Retained torch stand component identity changed')
        for index in matches[0]['indices']:
            for axis,key in enumerate(('X','Y','Z')):
                vertices[index].set(key, str(int(vertices[index].get(key)) + spec['delta'][axis]))
    ET.indent(root, space='  ')
    changed[f'{GEOMETRY}_vtx_2'] = ET.tostring(root, encoding='utf-8') + b'\n'
    changed.update(prop_appearance.prepare_vertices(resources, changed, vertex_components))
    return changed


def prepare_platform_overlay(resources):
    """Use native opaque decal depth for the two coplanar right-pot panels."""
    path = f'{GEOMETRY}_tri_23'
    panels = vertex_components(resources, path)[1:]
    expected = (
        ([[98,8,198],[160,8,275]], list(range(63,69))),
        ([[98,8,295],[160,8,372]], list(range(69,75))),
    )
    if len(panels) != 2 or any(part['bounds'] != bounds or part['indices'] != indices or
                              part['triangles'] != 4 for part,(bounds,indices) in zip(panels,expected)):
        raise ValueError('Retained potion floor panel identity changed')
    root = ET.fromstring(resources[path])
    commands = list(root)
    load = next((index for index,node in enumerate(commands) if node.tag == 'LoadVertices' and
                 node.attrib == {'Path':f'{GEOMETRY}_vtx_23', 'VertexBufferIndex':'0',
                                 'VertexOffset':'63', 'Count':'12'}), None)
    if load is None or [node.tag for node in commands[load:]] != [
            'LoadVertices', 'Triangles2', 'Triangles2', 'Triangles2', 'Triangles2', 'EndDisplayList']:
        raise ValueError('Retained potion floor panel draw range changed')
    material = ET.fromstring(resources[f'custom/skiejer_market/geometry/mat_mahouya_room_0DL_006548_GranmaShop01'])
    mode = material.find("SetOtherMode[@Cmd='G_SETOTHERMODE_L']")
    if mode is None or mode.get('G_RM_AA_ZB_OPA_SURF2') != '1':
        raise ValueError('Retained potion floor panel depth mode changed')
    decal = dict(mode.attrib)
    decal.pop('G_RM_AA_ZB_OPA_SURF2')
    decal['G_RM_AA_ZB_OPA_DECAL2'] = '1'
    # Both panels and the timber below are exactly Y=8. Raising the panels
    # would intersect the pots' Y=8 bases. Native decal bias preserves every
    # position, UV, color, texture and collision face, then restores opaque
    # depth state before the room continues drawing.
    root.insert(load, ET.Element('PipeSync'))
    root.insert(load + 1, ET.Element('SetOtherMode', decal))
    root.insert(len(root) - 1, ET.Element('PipeSync'))
    root.insert(len(root) - 1, ET.Element('SetOtherMode', dict(mode.attrib)))
    ET.indent(root, space='  ')
    return {path:ET.tostring(root, encoding='utf-8') + b'\n'}


def resource_paths():
    return set(TEXTURES.values()) | {SHELF}


def _packed_parts(manifest, stem):
    parts = []
    for index, part in enumerate(manifest['parts'], 1):
        if part['name'] != f'{stem}.zlib.{index:03}.b64':
            raise ValueError('Original market part order/path changed')
        text = (ASSETS / part['name']).read_bytes()
        if len(text) > 128 * 1024:
            raise ValueError('Original market source part exceeds publication limit')
        data = base64.b64decode(text, validate=True)
        if len(data) != part['bytes'] or hashlib.sha256(data).hexdigest() != part['sha256']:
            raise ValueError('Original market part hash/size changed')
        parts.append(data)
    packed = b''.join(parts)
    if hashlib.sha256(packed).hexdigest() != manifest['packed_sha256']:
        raise ValueError('Original market part aggregate hash changed')
    return packed


@lru_cache(maxsize=1)
def encoded_pixels():
    manifest = json.loads((ASSETS / 'encoded.json').read_text())
    if manifest.get('version') != 1 or manifest.get('source_sha256') != {name:SOURCE_HASHES[name] for name in BASE_NAMES}:
        raise ValueError('Original market encoded surface identity changed')
    if 'parts' in manifest:
        packed = _packed_parts(manifest, 'textures')
    else:
        packed = base64.b64decode((ASSETS / 'textures.zlib.b64').read_bytes(), validate=True)
    if hashlib.sha256(packed).hexdigest() != manifest['packed_sha256']:
        raise ValueError('Original market encoded surface hash mismatch')
    decoder = zlib.decompressobj()
    decoded = decoder.decompress(packed, 3 * 512 * 512 * 4 + 1)
    if len(decoded) != 3 * 512 * 512 * 4 or not decoder.eof or decoder.unused_data or decoder.unconsumed_tail:
        raise ValueError('Original market encoded surface size mismatch')
    result = {}
    size = 512 * 512 * 4
    for index, name in enumerate(BASE_NAMES):
        pixels = decoded[index * size:(index + 1) * size]
        if hashlib.sha256(pixels).hexdigest() != manifest['pixels_sha256'][name]:
            raise ValueError('Original market surface pixels changed: ' + name)
        result[name] = pixels
    manifest = json.loads((ASSETS / 'props-encoded.json').read_text())
    names = (*PROP_NAMES, *DERIVED_NAMES)
    if (manifest.get('version') != 1 or manifest.get('names') != list(names) or
            manifest.get('source_sha256') != SOURCE_HASHES):
        raise ValueError('Original market prop source/encoding identity changed')
    packed = _packed_parts(manifest, 'props')
    decoder = zlib.decompressobj()
    size = 512 * 512 * 4
    decoded = decoder.decompress(packed, len(names) * size + 1)
    if len(decoded) != len(names) * size or not decoder.eof or decoder.unused_data or decoder.unconsumed_tail:
        raise ValueError('Original market prop decoded size changed')
    for index, name in enumerate(names):
        pixels = decoded[index * size:(index + 1) * size]
        if hashlib.sha256(pixels).hexdigest() != manifest['pixels_sha256'][name]:
            raise ValueError('Original market prop pixels changed: ' + name)
        result[name] = pixels
    return result


def rebake():
    # Pillow belongs to the optional authoring tool, never the ordinary build.
    from PIL import Image, __version__
    images = {}
    for name in (*BASE_NAMES, *PROP_NAMES):
        source = ASSETS / (name + '.png')
        if hashlib.sha256(source.read_bytes()).hexdigest() != SOURCE_HASHES[name]:
            raise ValueError('Original market surface identity changed: ' + name)
        with Image.open(source) as image:
            images[name] = image.convert('RGBA').resize((512, 512), Image.Resampling.LANCZOS)
    curios = images['curios']
    patches = {'paper':curios.crop((0,0,256,256)), 'leather':curios.crop((256,0,512,256)),
               'bone':curios.crop((0,256,256,512)), 'glass':curios.crop((256,256,512,512)), **images}
    for name in DERIVED_NAMES:
        atlas = images['plaster' if name == 'walltrim_atlas' else 'timber'].copy()
        for source, rects in ATLAS_RECTS[name].items():
            if isinstance(rects[0], int):
                rects = [rects]
            for left, top, right, bottom in rects:
                patch = patches[source].resize((right-left, bottom-top), Image.Resampling.LANCZOS)
                atlas.paste(patch, (left, top))
        images[name] = atlas
    for stem, names, filename in (('textures', BASE_NAMES, 'encoded.json'),
                                   ('props', (*PROP_NAMES, *DERIVED_NAMES), 'props-encoded.json')):
        pixels = {name:images[name].tobytes() for name in names}
        packed = zlib.compress(b''.join(pixels.values()), 9)
        parts = []
        for index, start in enumerate(range(0, len(packed), PART_BYTES), 1):
            data = packed[start:start + PART_BYTES]
            name = f'{stem}.zlib.{index:03}.b64'
            (ASSETS / name).write_bytes(base64.b64encode(data))
            parts.append({'name':name, 'bytes':len(data), 'sha256':hashlib.sha256(data).hexdigest()})
        manifest = {'version':1, 'source_sha256':({name:SOURCE_HASHES[name] for name in BASE_NAMES}
                                                  if stem == 'textures' else SOURCE_HASHES),
                    'names':list(names), 'parts':parts,
                    'packed_sha256':hashlib.sha256(packed).hexdigest(),
                    'pixels_sha256':{name:hashlib.sha256(data).hexdigest() for name,data in pixels.items()},
                    'authoring_pillow_version':__version__,
                    'encoding':'base64 parts of one zlib stream; ordered 512x512 RGBA8 uploads',
                    'processing':'Lossless RGBA encoding and existing UV atlas packing of the recorded source PNGs'}
        if stem == 'props':
            manifest['atlas_rects'] = ATLAS_RECTS
        (ASSETS / filename).write_text(json.dumps(manifest, indent=2, sort_keys=True) + '\n')
    encoded_pixels.cache_clear()


def audit_materials(resources):
    """Trace actual material calls, triangle owners and retained UV bounds."""
    rows = {group:{'active':False, 'triangles':0, 'geometry':set(), 'points':[]} for group in MATERIAL_ROLES}
    cache, active, material = {}, set(), None
    def visit(path):
        nonlocal material
        if path in active:
            raise ValueError('Recursive retained market draw: ' + path)
        active.add(path)
        if '/mat_' in path:
            material = int(path[-2:])
        for node in ET.fromstring(resources[path]):
            if node.tag == 'CallDisplayList':
                visit(node.get('Path'))
            elif node.tag == 'LoadVertices':
                vertices = list(ET.fromstring(resources[node.get('Path')]))
                start, offset, count = (int(node.get(key)) for key in ('VertexBufferIndex','VertexOffset','Count'))
                for index, vertex in enumerate(vertices[offset:offset+count]):
                    cache[start+index] = [int(vertex.get(key)) for key in ('X','Y','Z','S','T')]
            elif node.tag in ('Triangle1','Triangles2'):
                if material not in rows:
                    raise ValueError('Unowned retained market triangle')
                row = rows[material]
                row['active'] = True
                row['geometry'].add(path)
                for face in range(2 if node.tag == 'Triangles2' else 1):
                    row['triangles'] += 1
                    row['points'].extend(cache[int(node.get(f'V{face}{index}'))] for index in range(3))
        active.remove(path)
    visit('custom/skiejer_market/room')
    for row in rows.values():
        points = row.pop('points')
        row['geometry'] = sorted(row['geometry'])
        row['bounds'] = [[op(point[axis] for point in points) for axis in range(3)] for op in (min,max)] if points else []
        row['uv_bounds'] = [[op(point[axis] for point in points) for axis in (3,4)] for op in (min,max)] if points else []
    return rows


def audit_manifest(resources):
    import native_market
    rows = audit_materials(resources)
    for group,row in rows.items():
        row.update(MATERIAL_ROLES[group])
        row['material'] = f'custom/skiejer_market/geometry/mat_mahouya_room_0DL_006548_GranmaShop{group:02}'
        row['texture'] = TEXTURES[row['surface']]
        row['logical_uv_period'] = 32 if row['surface'] in BASE_NAMES else 64
    return {'version':1, 'audit':'Actual retained room call graph, native triangle connectivity and S/T islands; visual group isolation against the user reference',
            'source_poc14_sha256':native_market.SOURCE_SHA,
            'retained_resources':len(resources), 'prepared_resources':len(apply(resources)),
            'texture_resources':sorted(resource_paths()), 'materials':rows,
            'source_png_sha256':SOURCE_HASHES, 'atlas_rects_512px':ATLAS_RECTS,
            'curio_uv_changes':{'vertex_suffixes':CURIO_UV_GROUPS,
                               'method':'Connected component native S/T range affine-mapped into padded paper/leather/bone/glass quadrant, original positions/colors/faces unchanged'},
            'torch_stand_translations':TORCH_STANDS,
            'routing':'All original render/combine/alpha/cull commands preserved; every uploaded pixel opaque. Native TEX_EDGE materials remain TEX_EDGE, with no added XLU routing.',
            'ownership_note':'Material17 has no default draw owner. Active celestial material15 owns tri26; tri27 is a retained unused duplicate mesh, not material27. Material27 owns liquid tri16.'}


@lru_cache(maxsize=1)
def shelf_pixels():
    """Exact original ethereal shelf art; independent of the floor/prop streams."""
    manifest = json.loads((ASSETS / 'shelf-encoded.json').read_text())
    size = 512 * 512 * 4
    if (manifest.get('version') != 1 or manifest.get('name') != 'arcane_stone' or
            manifest.get('source_sha256') != '19c491b0ed659ed4a8735a4e433755839010fa5367524b7cfd7bc8c581c7ca5f' or
            manifest.get('decoded_bytes') != size or not 1 <= len(manifest.get('parts', [])) <= 11 or
            any(not 0 < part['bytes'] <= PART_BYTES for part in manifest['parts'])):
        raise ValueError('Original shelf source/encoding identity changed')
    packed = _packed_parts(manifest, 'shelf')
    if len(packed) != manifest['packed_bytes']:
        raise ValueError('Original shelf packed size changed')
    decoder = zlib.decompressobj()
    pixels = decoder.decompress(packed, size + 1)
    if (len(pixels) != size or not decoder.eof or decoder.unused_data or decoder.unconsumed_tail or
            hashlib.sha256(pixels).hexdigest() != manifest['pixels_sha256'] or
            any(alpha != 255 for alpha in pixels[3::4])):
        raise ValueError('Original shelf decoded pixels changed')
    return pixels


def texture(name, logical_size):
    pixels = shelf_pixels() if name == 'arcane_stone' else encoded_pixels()[name]
    pixels = prop_appearance.texture_pixels(name, pixels)
    header = bytearray(64)
    header[1] = 1
    struct.pack_into('<IIQ', header, 4, 0x4F544558, 1, 0xDEADBEEFDEADBEEF)
    # LOAD_AS_RAW uses a native RGBA32 buffer with explicit byte/row scales.
    # The material remains RGBA16, matching its retained N64 UV/tile framing.
    framing = struct.pack('<IIIIffI', 1, 512, 512, 1,
                          1024 / logical_size, 512 / logical_size, len(pixels))
    return bytes(header) + framing + pixels


def grayscale_magic_texture():
    """Neutral BM tint base; preserve the original colored source API.

    Rec.709 integer weights operate on the existing encoded RGB values, with
    nearest-integer rounding: (2126R + 7152G + 722B + 5000) // 10000.
    No exposure/contrast adjustment or resampling is applied. Alpha and the
    corrected RGBA8 LOAD_AS_RAW / logical64 upload header stay byte-identical.
    """
    original = texture('arcane_stone', 64)
    pixels = bytearray(original[92:])
    for offset in range(0, len(pixels), 4):
        red, green, blue = pixels[offset:offset+3]
        luminance = (2126*red + 7152*green + 722*blue + 5000) // 10000
        pixels[offset:offset+3] = bytes((luminance, luminance, luminance))
    return original[:92] + pixels


def apply(resources):
    import native_market
    native_market.validate_resources(resources)
    result = dict(resources)
    result.update(prepare_prop_vertices(resources))
    result.update(prepare_platform_overlay(resources))
    for name, path in TEXTURES.items():
        result[path] = texture(name, 32 if name in BASE_NAMES else 64)
    # Flow/Veil owns this animated shelf/backdrop path, not the slate floor.
    # Preserve the original detail with a neutral base for the existing tint.
    result[SHELF] = grayscale_magic_texture()
    count = 0
    for path, data in resources.items():
        if '/mat_' not in path:
            continue
        match = re.search(r'GranmaShop(\d\d)$', path)
        if not match:
            raise ValueError('Unmapped retained market material: ' + path)
        group = int(match[1])
        name = MATERIAL_ROLES[group]['surface']
        material = ET.fromstring(data)
        material.find('SetTextureImage').set('Path', TEXTURES[name])
        material.find('SetTextureLUT').set('Mode', 'G_TT_NONE')
        logical = 32 if name in BASE_NAMES else 64
        for tile in material.findall('SetTile'):
            tile.set('Line', str(logical // 4) if tile.get('Tile') == '0' else '0')
            tile.set('MaskS', '5' if logical == 32 else '6')
            tile.set('MaskT', '5' if logical == 32 else '6')
        material.find('LoadBlock').set('Lrs', str(logical * logical - 1))
        material.find('LoadBlock').set('Dxt', str(8192 // logical))
        material.find('SetTileSize').set('Lrs', str((logical-1)*4))
        material.find('SetTileSize').set('Lrt', str((logical-1)*4))
        ET.indent(material, space='  ')
        result[path] = ET.tostring(material, encoding='utf-8') + b'\n'
        count += 1
    if count != 28:
        raise ValueError('Retained market material inventory changed')
    return result


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rebake', action='store_true')
    parser.add_argument('--audit-output', type=Path)
    args = parser.parse_args()
    if not args.rebake and not args.audit_output:
        parser.error('choose --rebake or --audit-output')
    if args.rebake:
        rebake()
    if args.audit_output:
        import native_market
        resources = native_market.read_bundle(Path(__file__).parents[1] / 'native_market/bundled')
        args.audit_output.write_text(json.dumps(audit_manifest(resources),indent=2,sort_keys=True) + '\n')

