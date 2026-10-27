#!/usr/bin/env python3
"""Convert Chateau MM3D with its packed 4K atlas and clear glass materials.

The preserved original ZIP supplies the unmodified mesh/UVs and gold reflection.
The checked-in reconstructed atlas supplies the label and cork detail.
"""
from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import io
import json
from pathlib import Path
import struct
import sys
import xml.etree.ElementTree as ET
import zipfile

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/npc_models'))
from native_assets import o2r

PREFIX = 'objects/combo_chateau_romani_3ds/'
NATIVE = 'alt/objects/object_gi_bottle_21/'
SOURCE_SHA256 = '73c375486b5f5323137b99679e9a13ae6e3b2dea25a12a03e89a89ef63e4df34'
ROLES = {'material-01C807ED-material': 'Milk', 'material-03A07E84-material': 'Glass',
         'material-00A472A8-material': 'Cork', 'material-00672A80-material': 'Label'}


def source_mesh(data):
    root = ET.fromstring(data)
    ns = {'c': 'http://www.collada.org/2005/11/COLLADASchema'}
    geometry = root.findall('.//c:geometry', ns)
    if len(geometry) != 1:
        raise ValueError('Expected the single authored Chateau mesh')
    mesh = geometry[0].find('c:mesh', ns)
    arrays = {}
    for source in mesh.findall('c:source', ns):
        values = source.find('c:float_array', ns)
        if values is not None:
            stride = int(source.find('c:technique_common/c:accessor', ns).get('stride'))
            arrays[source.get('id')] = np.fromstring(values.text, sep=' ').reshape(-1, stride)
    vertices = {v.get('id'): v.find('c:input', ns).get('source')[1:]
                for v in mesh.findall('c:vertices', ns)}
    # The DAE is Z-up with an authored 0.01 scale / X rotation. Convert world
    # coordinates to the engines' Y-up convention, then fit native GI height.
    node = root.find('.//c:visual_scene/c:node', ns)
    transform = np.fromstring(node.find('c:matrix', ns).text, sep=' ').reshape(4, 4)
    z_to_y = np.array([[1, 0, 0], [0, 0, 1], [0, -1, 0]], dtype=float)
    linear = z_to_y @ transform[:3, :3]
    translation = z_to_y @ transform[:3, 3]
    normal_matrix = np.linalg.inv(linear).T
    result = {}
    for poly in mesh.findall('c:polylist', ns):
        if set(poly.find('c:vcount', ns).text.split()) != {'3'}:
            raise ValueError('Unexpected non-triangular source face')
        inputs = poly.findall('c:input', ns)
        stride = 1 + max(int(i.get('offset')) for i in inputs)
        indices = np.fromstring(poly.find('c:p', ns).text, sep=' ', dtype=int).reshape(-1, stride)
        attrs = {}
        for semantic in ('VERTEX', 'NORMAL', 'TEXCOORD'):
            attr = next(i for i in inputs if i.get('semantic') == semantic)
            key = attr.get('source')[1:]
            if semantic == 'VERTEX':
                key = vertices[key]
            attrs[semantic] = arrays[key][indices[:, int(attr.get('offset'))]]
        position = attrs['VERTEX'] @ linear.T + translation
        normal = attrs['NORMAL'] @ normal_matrix.T
        normal /= np.linalg.norm(normal, axis=1)[:, None]
        result[ROLES[poly.get('material')]] = {'position': position, 'normal': normal,
                                              'uv': attrs['TEXCOORD']}
    all_positions = np.concatenate([v['position'] for v in result.values()])
    scale = 60.0 / np.ptp(all_positions[:, 1])
    center_y = (all_positions[:, 1].min() + all_positions[:, 1].max()) / 2
    for entry in result.values():
        entry['position'][:, 1] -= center_y
        entry['position'] *= scale
    if sum(len(v['position']) // 3 for v in result.values()) != 284:
        raise ValueError('Source triangle inventory changed')
    return result


def texture(image, reflection=False):
    pixels = np.array(image.convert('RGBA'))
    if reflection:
        # A 3DS specular map contributes light. Keep RGB exactly; use its
        # intensity for coverage so black texels do not darken the glass.
        pixels[:, :, 3] = np.max(pixels[:, :, :3], axis=2)
    height, width = pixels.shape[:2]
    return o2r._header(o2r.T_OTEX) + struct.pack('<IIII', 1, width, height, pixels.nbytes) + pixels.tobytes()


def hd_texture(image, reflection=False):
    expected, native = (512, 32) if reflection else (4096, 64)
    if image.size != (expected, expected):
        raise ValueError(f'The packed HD texture must be {expected} x {expected}')
    pixels = np.array(image.convert('RGBA'))
    if reflection:
        pixels[:, :, 3] = np.max(pixels[:, :, :3], axis=2)
    # Texture V1 / LOAD_AS_RAW. The display-list tile remains 64 x 64, keeping
    # all signed vertex UVs and 12-bit tile coordinates unchanged. Scaling is
    # explicit metadata rather than overflowing native GBI fields with 4K.
    return (o2r._header(o2r.T_OTEX, 1)
            + struct.pack('<IIIIffI', 1, expected, expected, 1,
                          float(expected/native), float(expected/native), pixels.nbytes)
            + pixels.tobytes())


def combine(lit):
    # (TEXEL0 - 0) * SHADE/PRIMITIVE + 0; alpha TEXEL0 * PRIMITIVE.
    c = 4 if lit else 3
    return (0xFC000000 | 1 << 20 | c << 15 | 1 << 12 | 3 << 9 | 1 << 5 | c,
            15 << 28 | 15 << 24 | 1 << 21 | 3 << 18 | 7 << 15 | 7 << 12 | 7 << 9 | 7 << 6 | 7 << 3 | 7)


def emit_material(files, role, entry, tex_name, dimensions, alpha=255, reflection=False):
    path = PREFIX + role + 'DL'
    vertex_path = PREFIX + role + 'Vertices'
    w, h = dimensions
    records = []
    for p, n, uv in zip(entry['position'], entry['normal'], entry['uv']):
        st = np.rint([uv[0] * w * 32, (1 - uv[1]) * h * 32]).astype(int)
        normal = np.clip(np.rint(n * 127), -127, 127).astype(int)
        records.append((*np.rint(p).astype(int), 0, *st, *(normal & 255), 255))
    files[vertex_path] = o2r.build_vertex_array(records)
    d = o2r.DLBuilder()
    d.marker(path)
    d.raw(0xE7000000, 0)
    d.raw(0xE3000A01, 0)  # G_CYC_1CYCLE
    d.raw(0xE3001001, 0)  # G_TT_NONE
    d.raw(0xD9000000, 0x00220405 | (0x40000 if reflection else 0))
    # Texture generation spans [0, scale/2] in s10.5 coordinates.
    s, t = ((w - 1) * 64, (h - 1) * 64) if reflection else (65535, 65535)
    d.raw(0xD7000002, (s << 16) | t)
    d.raw(0xFA000000, 0xFFFFFF00 | alpha)
    d.raw(*combine(not reflection))
    d.settimg_hash(0, 3, w, PREFIX + tex_name)
    d.raw(0xF5180000, 0x07080200)
    d.raw(0xE6000000, 0)
    d.raw(0xF4000000, 0x07000000 | ((w - 1) * 4) << 12 | ((h - 1) * 4))
    d.raw(0xE7000000, 0)
    d.raw(0xF5180000 | ((w * 2 + 7) // 8) << 9, 0x00080200)
    d.raw(0xF2000000, ((w - 1) * 4) << 12 | ((h - 1) * 4))
    for offset in range(0, len(records), 30):
        count = min(30, len(records) - offset)
        d.vtx_hash(count, 0, offset * 16, vertex_path)
        for i in range(0, count, 3):
            d.tri1(i, i + 1, i + 2)
    # Do not leak environment mapping or the reflection's low texture scale.
    d.raw(0xD9F3FFFF, 0)
    d.raw(0xD7000002, 0xFFFFFFFF)
    d.raw(0xFA000000, 0xFFFFFFFF)
    d.end()
    files[path] = d.to_resource()


def wrapper(files, path, children):
    d = o2r.DLBuilder()
    d.marker(path)
    for child in children:
        d.dl_hash(PREFIX + child + 'DL')
    d.end()
    files[path] = d.to_resource()


def build(source, hd=True):
    source_bytes = source.read_bytes()
    if hashlib.sha256(source_bytes).hexdigest() != SOURCE_SHA256:
        raise ValueError('Input differs from the preserved user-supplied MM3D ZIP')
    files = {}
    with zipfile.ZipFile(io.BytesIO(source_bytes)) as archive:
        mesh = source_mesh(archive.read('chateau_romani.dae'))
        atlas = Image.open(io.BytesIO(archive.read('zelda_gi_superm_0.png'))).convert('RGBA')
        glass_map = Image.open(io.BytesIO(archive.read('zelda_gi_superm_2100.png')))
        gold_map = Image.open(io.BytesIO(archive.read('zelda_gi_superm_2000.png')))
        files[PREFIX + 'AtlasTex'] = texture(atlas)
        files[PREFIX + 'GlassReflectionTex'] = texture(glass_map, True)
        files[PREFIX + 'GoldReflectionTex'] = texture(gold_map, True)
    if hd:
        from prepare_chateau_romani import prepare_source_art
        prepare_source_art()
        hd_atlas = ROOT / 'tools/chateau_romani_3ds/hd/Atlas-4K.png'
        with Image.open(hd_atlas) as image:
            files[PREFIX + 'AtlasTex'] = hd_texture(image)
        hd_reflection = ROOT / 'tools/chateau_romani_3ds/hd/Glass-Reflection.png'
        with Image.open(hd_reflection) as image:
            files[PREFIX + 'GlassReflectionTex'] = hd_texture(image, reflection=True)
    for role in ('Milk', 'Cork', 'Glass', 'Label'):
        emit_material(files, 'GlassSurface' if role == 'Glass' else role, mesh[role],
                      'AtlasTex', atlas.size, (46 if hd else 128) if role == 'Glass' else 255)
    emit_material(files, 'GlassReflection', mesh['Glass'], 'GlassReflectionTex', glass_map.size, 110 if hd else 100, True)
    emit_material(files, 'LabelReflection', mesh['Label'], 'GoldReflectionTex', gold_map.size, 40 if hd else 55, True)
    wrapper(files, PREFIX + 'OpaqueDL', ['Milk', 'Cork'])
    # Label after glass preserves the rich burgundy; glass cannot wash it out.
    wrapper(files, PREFIX + 'GlassDL', ['GlassSurface', 'GlassReflection', 'Label', 'LabelReflection'])
    wrapper(files, NATIVE + 'gGiChateauRomaniBottleEmptyDL', ['Opaque'])
    wrapper(files, NATIVE + 'gGiChateauRomaniBottleDL', ['Glass'])
    label_pixels = np.array(atlas)[17:63, 1:63, :3].reshape(-1, 3)
    label_colors = Counter(tuple(map(int, p)) for p in label_pixels if p[0] > p[1] * 1.6 and p[2] > p[1] * 1.6)
    label_rgb = label_colors.most_common(1)[0][0]
    manifest = {'candidate': 'Chateau Romani MM3D 4K clear glass POC2' if hd else 'Chateau Romani MM3D POC1', 'source_sha256': SOURCE_SHA256,
                'source_credit': 'Nintendo / Grezzo; model ripped by Guy for The Models Resource',
                'source_triangles': {role: len(v['position']) // 3 for role, v in mesh.items()},
                'draw_triangles': {'opaque': 96, 'translucent_including_reflections': 376},
                'height': 60, 'label_hex': '#%02X%02X%02X' % label_rgb,
                'shimmer': {'label_hex': '#4A0C41', 'gold_hex': '#FFD45A'},
                'alternate_assets': True, 'runtime_tested': False,
                'resources': {k: hashlib.sha256(v).hexdigest() for k, v in sorted(files.items())}}
    if hd:
        manifest['hd_atlas'] = {'file': 'tools/chateau_romani_3ds/hd/Atlas-4K.png',
                                'size': [4096, 4096], 'native_tile': [64, 64],
                                'flags': 1, 'h_byte_scale': 64., 'v_pixel_scale': 64.,
                                'sha256': hashlib.sha256(hd_atlas.read_bytes()).hexdigest(),
                                'artwork': 'AI reconstructed from original 64x64 atlas; generated 1254x1254, UV packed and resampled to 4K'}
        manifest['hd_reflection'] = {'file': 'tools/chateau_romani_3ds/hd/Glass-Reflection.png',
                                     'size': [512,512], 'native_tile': [32,32],
                                     'sha256': hashlib.sha256(hd_reflection.read_bytes()).hexdigest()}
    return files, manifest


def write_archive(files, output):
    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for path, content in sorted(files.items()):
            info = zipfile.ZipInfo(path, (2026, 10, 19, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            archive.writestr(info, content)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=ROOT / 'tools/chateau_romani_3ds/source.zip')
    parser.add_argument('--output', type=Path)
    parser.add_argument('--write-custom', action='store_true')
    parser.add_argument('--native-textures', action='store_true', help='Explicit POC1 recovery build')
    args = parser.parse_args()
    files, manifest = build(args.source, hd=not args.native_textures)
    if args.write_custom:
        for game in ('soh', 'mm'):
            for path, content in files.items():
                target = ROOT / game / 'assets/custom' / path
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(content)
        (ROOT / 'tools/chateau_romani_3ds/manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
        if not args.native_textures:
            from prepare_chateau_romani import write_bundle
            write_bundle(files[PREFIX + 'AtlasTex'])
    if args.output:
        write_archive(files, args.output)
    print(json.dumps({'resources': len(files), 'label_hex': manifest['label_hex'],
                      'source_triangles': manifest['source_triangles'], 'draw_triangles': manifest['draw_triangles']}))


if __name__ == '__main__':
    main()
