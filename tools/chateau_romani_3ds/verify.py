#!/usr/bin/env python3
"""Validate the actual resource graph; optionally render its exported triangles."""
import argparse
from copy import deepcopy
import hashlib
import io
import json
from pathlib import Path
import struct
import sys
import zipfile

import numpy as np
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/npc_models'))
from native_assets.o2r import crc64, read_texture, read_vertex_array
from native_assets.gbi import Triangle, TileState, Vertex
from native_assets.render import Tex, render

PREFIX = 'objects/combo_chateau_romani_3ds/'


class Graph:
    def __init__(self, files):
        self.files = files
        self.hashes = {crc64(k): k for k in files}
        self.used = set()

    def triangles(self, root):
        records = []
        state = dict(cache=[None] * 32, geom=0, prim=255, scale=(65535, 65535), texture=None, lit=True)

        def walk(path, active=()):
            if path in active:
                raise ValueError('Display-list cycle: ' + ' -> '.join((*active, path)))
            assert len(active) < 16
            self.used.add(path)
            raw = self.files[path]
            assert raw[4:8] == b'TLDO' and raw[64] == 4
            offset = 72
            while offset + 8 <= len(raw):
                a, b = struct.unpack_from('<II', raw, offset)
                op = a >> 24
                size = 16 if op in (0x20, 0x31, 0x32, 0x33) else 8
                if size == 16:
                    high, low = struct.unpack_from('<II', raw, offset + 8)
                    target = self.hashes[(high << 32) | low]
                    if op == 0x31:
                        walk(target, (*active, path))
                    elif op == 0x20:
                        self.used.add(target)
                        state['texture'] = target
                    elif op == 0x32:
                        self.used.add(target)
                        vertices = read_vertex_array(self.files[target])
                        count = (a >> 12) & 255
                        start = ((a >> 1) & 127) - count
                        assert 0 <= start and start + count <= 32 and b % 16 == 0
                        data = vertices[b // 16:b // 16 + count]
                        assert len(data) == count
                        state['cache'][start:start + count] = data
                    else:
                        assert target == path
                elif op == 0xD9:
                    state['geom'] = (state['geom'] & (a & 0xFFFFFF)) | b
                elif op == 0xD7:
                    state['scale'] = (b >> 16, b & 65535)
                elif op == 0xFA:
                    state['prim'] = b & 255
                elif op == 0xFC:
                    state['lit'] = ((a >> 15) & 31) == 4
                elif op == 0x05:
                    ids = [(a >> s & 255) // 2 for s in (16, 8, 0)]
                    vertices = [state['cache'][i] for i in ids]
                    assert all(v is not None for v in vertices)
                    assert state['texture'] in self.files
                    records.append((vertices, {k: deepcopy(v) for k, v in state.items() if k != 'cache'}))
                elif op == 0xDF:
                    assert offset + 8 == len(raw)
                    return
                offset += size
            raise ValueError('Display list has no END: ' + path)

        walk(root)
        return records


def preview(graph, opaque, glass, output):
    angles = [0, 45, 180]
    size = 800
    sheet = Image.new('RGB', (size * 3, 920), '#111622')
    canvas = ImageDraw.Draw(sheet)
    try:
        font = ImageFont.truetype('DejaVuSans.ttf', 21)
    except OSError:
        font = ImageFont.load_default()
    textures = {}
    for _, material in opaque + glass:
        path = material['texture']
        if path not in textures:
            raw = graph.files[path]
            _, w, h, pixels = read_texture(raw)
            tex = Tex(np.frombuffer(pixels, np.uint8).reshape(h, w, 4), 2, 2)
            if struct.unpack_from('<I', raw, 8)[0] == 1:
                tex.scale_s, tex.scale_t = struct.unpack_from('<ff', raw, 80)
            textures[path] = tex
    for view, angle in enumerate(angles):
        a = np.radians(angle)
        camera = np.array([[np.cos(a), 0, np.sin(a)], [0, 1, 0], [-np.sin(a), 0, np.cos(a)]])
        triangles = []
        for vertices, material in opaque + glass:
            xyz = np.array([v[:3] for v in vertices], float) @ camera.T
            normals = np.array([[q if q < 128 else q - 256 for q in v[6:9]] for v in vertices], float) / 127
            normals = normals @ camera.T
            st = np.array([v[4:6] for v in vertices], float)
            if material['geom'] & 0x40000:
                st = (normals[:, :2] + 1) / 4 * material['scale']
            tex = textures[material['texture']]
            tile = TileState(cms=2, cmt=2, prim=(255, 255, 255, material['prim']),
                             othermode_l=(0, 0x4000), combine_alpha_uses_prim=True)
            shade = (.6 + .4 * np.maximum(normals @ np.array([.35, .75, .55]), 0)
                     if material['lit'] else np.ones(3))
            vv = tuple(Vertex(0, 0, 0, int(uv[0]), int(uv[1]), 255, 255, 255, 255) for uv in st)
            triangles.append((Triangle(vv, tile), xyz, [(float(c),) * 3 for c in shade], tex))
        image = render(triangles, {}, size=size, bg=(17, 22, 34, 255), supersample=2, cull=1)
        sheet.paste(image.convert('RGB'), (view * size, 50))
        canvas.text((view * size + 28, 18), f'{angle} degrees', fill='white', font=font)
    canvas.text((28, 875), '4K label / detailed cork / clear glass - exported-resource render; in-game review pending', fill='#AAB8CF', font=font)
    output.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(output)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--archive', required=True, type=Path)
    parser.add_argument('--preview', type=Path)
    args = parser.parse_args()
    manifest = json.loads((ROOT / 'tools/chateau_romani_3ds/manifest.json').read_text())
    with zipfile.ZipFile(args.archive) as archive:
        assert archive.testzip() is None
        assert len(set(archive.namelist())) == len(archive.namelist())
        files = {k: archive.read(k) for k in archive.namelist()}
    assert set(files) == set(manifest['resources'])
    for path, content in files.items():
        assert hashlib.sha256(content).hexdigest() == manifest['resources'][path]
        for game in ('soh', 'mm'):
            assert (ROOT / game / 'assets/custom' / path).read_bytes() == content
    graph = Graph(files)
    opaque = graph.triangles(PREFIX + 'OpaqueDL')
    glass = graph.triangles(PREFIX + 'GlassDL')
    assert len(opaque) == 96 and len(glass) == 376
    for root, count in [('gGiChateauRomaniBottleEmptyDL', 96), ('gGiChateauRomaniBottleDL', 376)]:
        assert len(graph.triangles('alt/objects/object_gi_bottle_21/' + root)) == count
    assert graph.used == set(files)
    all_xyz = np.array([v[:3] for records, _ in opaque + glass for v in records], float)
    assert all_xyz[:, 1].min() == -30 and all_xyz[:, 1].max() == 30
    degenerate = 0
    for vertices, material in opaque + glass:
        p = np.array([v[:3] for v in vertices], float)
        degenerate += np.linalg.norm(np.cross(p[1] - p[0], p[2] - p[0])) < .001
        assert 0 < material['prim'] <= 255
    assert not degenerate
    with zipfile.ZipFile(ROOT / 'tools/chateau_romani_3ds/source.zip') as original:
        for target, source in [('AtlasTex', '0'), ('GlassReflectionTex', '2100'), ('GoldReflectionTex', '2000')]:
            _, w, h, raw = read_texture(files[PREFIX + target])
            actual = np.frombuffer(raw, np.uint8).reshape(h, w, 4)
            if target == 'AtlasTex' and manifest.get('hd_atlas'):
                expected = np.array(Image.open(ROOT / manifest['hd_atlas']['file']).convert('RGBA'))
            elif target == 'GlassReflectionTex' and manifest.get('hd_reflection'):
                expected = np.array(Image.open(ROOT / manifest['hd_reflection']['file']).convert('RGBA'))
                assert np.array_equal(actual[:,:,3], actual[:,:,:3].max(axis=2))
            else:
                expected = np.array(Image.open(io.BytesIO(original.read('zelda_gi_superm_' + source + '.png'))).convert('RGBA'))
            assert np.array_equal(actual[:, :, :3], expected[:, :, :3])
    if manifest.get('hd_atlas'):
        sys.path.insert(0, str(ROOT / 'scripts/assets'))
        import build_chateau_romani_3ds as builder
        from verify_hd import verify_hd
        baseline, _ = builder.build(ROOT / 'tools/chateau_romani_3ds/source.zip', hd=False)
        verify_hd(files, baseline)
    if args.preview:
        preview(graph, opaque, glass, args.preview)
    print(json.dumps({'result': 'PASS', 'resources': len(files), 'triangles': len(opaque) + len(glass),
                      'unresolved_references': 0, 'cycles': 0, 'degenerate_triangles': 0,
                      'native_height': 60, 'both_port_copies_match': True,
                      'packed_texture_pixels_verified': True, 'original_gold_reflection_rgb_preserved': True,
                      'hd_atlas': manifest.get('hd_atlas', {}).get('size'), 'runtime_proven': False}))


if __name__ == '__main__':
    main()
