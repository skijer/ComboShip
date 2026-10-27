"""Increase sword depth while retaining accepted front silhouettes and assets."""
import copy
import io
import json
from pathlib import Path
import struct
import subprocess
import xml.etree.ElementTree as ET

import numpy as np
from PIL import Image

import meshkit
import preview

BASELINE = 'c0181856968dd6f3869be0d1aa240743091fd6a8'
RETAINED = ('kokiri_sword', 'mm_kokiri_sword', 'razor_sword', 'four_sword', 'great_fairy_sword')


def factor(slug, part):
    """Keep rounded grips; thicken blades/relief and, less, planar fittings."""
    name = part['name'].lower()
    blade = any(word in name for word in (
        'tp blade', 'forged blade', 'fullered blade', 'rose-tempered fairy blade',
        'razor hooked', 'razor shorter', 'razor integral', 'razor seated',
        'fairy motif', 'enamel motif field', 'fairy vine', 'embossed fairy leaf',
        'leaf central engraving', 'fairy rosette', 'lilac forged blade',
        'lily blade shoulder', 'lilac central blade frame'))
    if slug == 'great_fairy_sword' and name == 'faceted inset gemstone':
        blade = True
    if blade:
        return 1.5
    if any(word in name for word in (
        'leather grip', 'leather wrap', 'textured leather', 'spiral leather',
        'grip core', 'close-wound ivory', 'grip collar', 'grip ferrule',
        'grip metal ferrule', 'upper ferrule', 'pommel ferrule', 'pommel collar',
        'fitted collar', 'lower collar', 'leaf bud pommel', 'sculpted leaf grip',
        'violet kokiri pommel', 'faceted silver pommel', 'heavy capped pommel',
        'pommel machined ring', 'heavy blade seat collar')):
        return 1.0
    return 1.2


def deepen(model):
    factors = []
    for part in model.parts:
        depth = factor(model.slug, part)
        assert 'native_normal_bytes' not in part
        part['p'][:, 2] = np.rint(part['p'][:, 2] * depth * meshkit.Q) / meshkit.Q
        if depth != 1:
            part['n'] = meshkit.unit(part['n'] / [1, 1, depth])
        factors.append({'part': part['name'], 'factor': depth})
    model.markers.update(blade_depth_factor=1.5, planar_fittings_depth_factor=1.2,
                         depth_parts=factors, runtime_tested=False,
                         visual_review_candidate=True, imported_approved_model=False)
    model.notes.append('Rotation depth pass: blade and attached relief 1.5x; planar fittings 1.2x; rounded grips retained. Face-on X/Y silhouette, UVs and materials retained.')
    return model


def decode_glb(raw):
    assert struct.unpack_from('<4sII', raw) == (b'glTF', 2, len(raw))
    n, tag = struct.unpack_from('<I4s', raw, 12)
    assert tag == b'JSON'
    size, tag = struct.unpack_from('<I4s', raw, 20 + n)
    assert tag == b'BIN\0'
    doc, binary = json.loads(raw[20:20+n]), bytearray(raw[28+n:])
    assert size == len(binary)
    return doc, binary


def accessor(doc, binary, index):
    a = doc['accessors'][index]
    v = doc['bufferViews'][a['bufferView']]
    columns = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3}[a['type']]
    dtype = np.dtype({5126: '<f4', 5125: '<u4'}[a['componentType']])
    return np.ndarray((a['count'], columns), dtype=dtype, buffer=binary,
                      offset=v.get('byteOffset', 0) + a.get('byteOffset', 0),
                      strides=(v.get('byteStride', dtype.itemsize * columns), dtype.itemsize))


def load_model(slug, doc, binary, meta):
    m = meshkit.Model(slug, meta['name'], meta['entry'], meta['draw_scale'],
                      meta['matrix_scale'] * meshkit.Q * meta['draw_scale'])
    m.native_scale = meta['matrix_scale']
    m.markers, m.notes = copy.deepcopy(meta['markers']), list(meta['notes'])
    for material in doc['materials']:
        pbr, extra = material['pbrMetallicRoughness'], material.get('extras', {})
        rgba = pbr.get('baseColorFactor', [1, 1, 1, 1])
        texture = None
        if 'baseColorTexture' in pbr:
            texture_def = doc['textures'][pbr['baseColorTexture']['index']]
            image = doc['images'][texture_def['source']]
            view = doc['bufferViews'][image['bufferView']]
            offset = view.get('byteOffset', 0)
            texture = np.array(Image.open(io.BytesIO(binary[offset:offset+view['byteLength']])).convert('RGBA'))
        m.materials[material['name']] = dict(color=rgba[:3], alpha=rgba[3], tex=texture,
            metal=pbr.get('metallicFactor', 1), rough=pbr.get('roughnessFactor', 1),
            emission=max(material.get('emissiveFactor', [0, 0, 0])),
            reflection=extra.get('strength', 0), decal=extra.get('neiDecal', False),
            double_sided=material.get('doubleSided', False))
    for mesh in doc['meshes']:
        assert len(mesh['primitives']) == 1
        primitive = mesh['primitives'][0]
        a = primitive['attributes']
        assert '_NEI_NATIVE_NORMAL' not in a
        m.parts.append(dict(name=mesh['name'], mat=doc['materials'][primitive['material']]['name'],
            p=accessor(doc, binary, a['POSITION']).astype(float) / meshkit.Q,
            n=accessor(doc, binary, a['NORMAL']).astype(float),
            uv=accessor(doc, binary, a['TEXCOORD_0']).astype(float),
            tri=accessor(doc, binary, primitive['indices']).reshape(-1, 3).astype(int)))
    return m


def build_retained(slug, output, repo):
    """Transform the exact baseline GLB and Vtx, leaving its draw graph intact."""
    def git_bytes(path):
        return subprocess.check_output(['git', 'show', BASELINE + ':' + path], cwd=repo)
    cp = 'tools/nei_gi/CHECKPOINTS/' + slug + '/'
    prefix = 'soh/assets/custom/objects/nei_gi_redesign/' + slug + '/'
    raw = git_bytes(cp + slug + '.glb')
    doc, binary = decode_glb(raw)
    metadata = json.loads(git_bytes(cp + 'checkpoint.json'))
    original = load_model(slug, doc, binary, metadata)
    model = deepen(copy.deepcopy(original))
    records = {'opa': [], 'xlu': []}
    for mesh, before, after in zip(doc['meshes'], original.parts, model.parts):
        a = mesh['primitives'][0]['attributes']
        old_p = np.rint(before['p'] * meshkit.Q).astype(int)
        old_n = np.rint(before['n'] * 127).astype(int) & 255
        new_p = np.rint(after['p'] * meshkit.Q).astype(int)
        new_n = np.rint(after['n'] * 127).astype(int) & 255
        # Reproduce the existing 32-entry vertex-cache order. A position and
        # packed-normal key is insufficient: separate bevel vertices can round
        # to the same old normal byte but acquire different new normals.
        render_pass = 'xlu' if original.materials[before['mat']]['alpha'] < 1 else 'opa'
        cache = {}
        def flush():
            for vi in cache:
                records[render_pass].append((tuple(old_p[vi]) + tuple(old_n[vi]),
                                             (int(new_p[vi, 2]), *map(int, new_n[vi])),
                                             factor(slug, before)))
            cache.clear()
        for face in before['tri']:
            if len(set(map(int, face)) - cache.keys()) + len(cache) > 32:
                flush()
            for vi in face:
                cache.setdefault(int(vi), len(cache))
        flush()
        for attribute, data in (('POSITION', new_p), ('NORMAL', after['n'])):
            index = a[attribute]
            target = accessor(doc, binary, index)
            target[:] = data
            doc['accessors'][index].update(min=target.min(0).tolist(), max=target.max(0).tolist())
    resources = output / 'RESOURCES/objects/nei_gi_redesign' / slug
    resources.mkdir(parents=True, exist_ok=True)
    paths = subprocess.check_output(['git', 'ls-tree', '-r', '--name-only', BASELINE, prefix], cwd=repo).decode().splitlines()
    for path in paths:
        data = git_bytes(path)
        if Path(path).name.startswith('mesh_') and Path(path).name.endswith('_vtx'):
            vertices = ET.fromstring(data)
            render_pass = 'xlu' if 'xlu' in Path(path).name else 'opa'
            assert len(vertices) == len(records[render_pass]), (slug, 'vertex cache count changed')
            for vertex, (expected, updated, depth) in zip(vertices, records[render_pass]):
                key = tuple(int(vertex.get(k)) for k in ('X', 'Y', 'Z', 'R', 'G', 'B'))
                # GLB float32 can round a source float64 half-byte normal in
                # the other direction (e.g. 63.499999 -> 63.5). The cache
                # position must match exactly; allow only that one-byte loss.
                signed = lambda values: np.where(np.array(values) >= 128, np.array(values) - 256, values)
                assert key[:3] == expected[:3] and np.max(np.abs(signed(key[3:]) - signed(expected[3:]))) <= 1, (slug, 'vertex cache order or normal differs', key, expected)
                if depth == 1:
                    updated = (updated[0], *key[3:])
                for key, value in zip(('Z', 'R', 'G', 'B'), updated):
                    vertex.set(key, str(value))
            data = ET.tostring(vertices) + b'\n'
        (resources / Path(path).name).write_bytes(data)
    checkpoint = output / 'CHECKPOINTS' / slug
    checkpoint.mkdir(parents=True, exist_ok=True)
    js = json.dumps(doc, separators=(',', ':')).encode()
    js += b' ' * (-len(js) % 4)
    body = struct.pack('<I4s', len(js), b'JSON') + js + struct.pack('<I4s', len(binary), b'BIN\0') + binary
    (checkpoint / (slug + '.glb')).write_bytes(struct.pack('<4sII', b'glTF', 2, len(body) + 12) + body)
    positions = np.concatenate([part['p'] for part in model.parts])
    metadata.update(bounds_author=[positions.min(0).tolist(), positions.max(0).tolist()],
                    markers=model.markers, notes=model.notes, runtime_tested=False)
    metadata['depth_parent'] = {'baseline_commit': BASELINE, 'glb_path': cp + slug + '.glb',
                                'preserved': 'X/Y positions, topology, UVs, materials, texture resources, display lists and scale matrix'}
    (checkpoint / 'checkpoint.json').write_text(json.dumps(metadata, indent=2) + '\n')
    preview.render(model, model.markers.get('preview_front_angle', 16)).save(checkpoint / 'front.png')
    preview.render(model, model.markers.get('preview_back_angle', 164)).save(checkpoint / 'back.png')
    print(json.dumps({'checkpoint': slug, 'depth_pass': True, 'triangles': metadata['triangles']}), flush=True)
    return model
