#!/usr/bin/env python3
"""Bake only the accepted POC5 bridge into 2ship's normal custom assets.

The source archives are read-only. Generated assets contain no room headers,
collision, actors, routes, grass, or other makeover resources.
"""
from pathlib import Path
import argparse
import collections
import hashlib
import json
import math
import struct
import xml.etree.ElementTree as ET

import numpy as np

from build_candidate import ROOT, SCENE, WALL, Resources, emit_mesh, bridge_surround, conform_wall_edges, section_profile, lower_floor_vertices

PREFIX = 'custom/cor_woods_bridge/'
ACCEPTED_SHA256 = 'cc1963d9dae25cd684cb0fc79948a398a611f1f49fad82ccd45f455f6f4c9257'
NATIVE_WALL = SCENE + 'Z2_26SARUNOMORI_room_01DL_000B90'
NATIVE_CANOPY = SCENE + 'Z2_26SARUNOMORI_room_01DL_002968'
NATIVE_CANOPY_VERTICES = SCENE + 'Z2_26SARUNOMORI_room_01Vtx_002708'
CANOPY_PIVOT_Y = 119
TEXTURES = {
    'custom/cor_mm3d/lost_woods_poc1/textures/lw_tutu_01_2A000': PREFIX + 'textures/planks',
    'custom/cor_mm3d/lost_woods_poc1/textures/lw_wood_02_2CA00': PREFIX + 'textures/bark',
    ROOT + 'textures/black': PREFIX + 'textures/black',
}


def build(native_path, accepted_path):
    accepted_hash = hashlib.sha256(accepted_path.read_bytes()).hexdigest()
    if accepted_hash != ACCEPTED_SHA256:
        raise ValueError('Bridge source is not the accepted POC5 archive')
    native = Resources(native_path)
    accepted = Resources(native_path, accepted_path)
    origin = np.array([110, -110])
    axis = np.array([1, -1]) / math.sqrt(2)
    side = np.array([1, 1]) / math.sqrt(2)
    source_path = ROOT + 'room_1/geometry'
    bridge = []
    for tri in accepted.mesh(source_path):
        v = tri['v']
        along = (v[:, [0, 2]] - origin) @ axis
        across = (v[:, [0, 2]] - origin) @ side
        if tri['path'] == source_path and along.min() >= -1 and along.max() <= 152 and abs(across).max() <= 70 and v[:, 1].max() <= 130:
            bridge.append(tri)
    counts = collections.Counter(t['texture'] for t in bridge)
    assert counts == {old: number for old, number in zip(TEXTURES, (142, 24, 28))}, counts
    # Reuse the approved aperture rule on the serialized accepted sleeve.
    # The small crown clearance keeps the native wall out of the inner roof.
    aperture = section_profile(bridge, origin, axis, 100)
    aperture[:, 0] *= .99
    bottom = aperture[:, 1].min()
    aperture[:, 1] = 60 + (aperture[:, 1] - 60) * .99
    aperture[:, 1] += 3 * np.clip((aperture[:, 1] - 60) / 15, 0, 1)
    aperture[aperture[:, 1] < bottom + 1, 1] = -2
    # The mesh reader pairs the last image with tile0's extent in this native
    # dual-texture material. A1C0 actually uses tile1: RGBA16,32x32. Derive the
    # baked single-texture load from that resource rather than the other sampler.
    wall_texture = native.get(WALL)
    wall_format, wall_width, wall_height, wall_bytes = struct.unpack_from('<IIII', wall_texture, 64)
    assert wall_format == 2 and wall_bytes == wall_width * wall_height * 2
    assert len(wall_texture) == 80 + wall_bytes
    wall_tile = (wall_width, wall_height)
    original_wall = native.mesh(NATIVE_WALL)
    assert all(tri['texture'] == WALL for tri in original_wall)
    original_wall = [{**tri, 'tile': wall_tile} for tri in original_wall]
    retained = []
    clipped = []
    for tri in original_wall:
        corner = ((tri['v'][:, 0] >= 80) & (tri['v'][:, 2] <= -80)).all()
        corridor = any(((tri['v'][:, [0, 2]] @ np.asarray(direction)) >= 239).all()
                       for direction in ((1, 0), (-1, 0), (0, 1), (0, -1)))
        if corner and not corridor:
            clipped.extend(bridge_surround(tri, aperture, origin, axis))
        else:
            retained.append(tri)
    # Native corridor endpoints already lie on integer, axis-aligned edges.
    # Conform only the diagonal corner, so every cardinal face stays exact.
    wall = retained + conform_wall_edges(clipped)
    wall_triangles = len(wall)
    # The accepted outer annulus seals the back of the corner. Give this
    # backing the original native wall material, with no HD-pack dependency.
    surround = [{**tri, 'texture': WALL, 'tile': wall_tile, 'lit': True}
                for tri in accepted.mesh(ROOT + 'room_1/bridge_surround')]
    files = {}
    emit_mesh(files, PREFIX + 'wall', wall + surround, lighting=True)
    # Emit before renaming: emit_mesh recognizes this exact black source path
    # and supplies its opaque, fog-free cap treatment.
    emit_mesh(files, PREFIX + 'bridge', bridge)
    output = {}
    for path, data in files.items():
        xml = ET.fromstring(data)
        if xml.tag == 'Vertex':
            output[path.removeprefix('alt/')] = lower_floor_vertices(data)
            continue
        for element in xml:
            if element.get('Path') in TEXTURES:
                element.set('Path', TEXTURES[element.get('Path')])
        if path == 'alt/' + PREFIX + 'wall':
            xml.insert(len(xml) - 1, ET.Element('CallDisplayList', Path=PREFIX + 'bridge'))
            xml.insert(len(xml) - 1, ET.Element('SetGeometryMode', G_LIGHTING='1'))
        output[path.removeprefix('alt/')] = ET.tostring(xml, encoding='utf-8')
    for old, new in TEXTURES.items():
        output[new] = accepted.get(old)
    # Keep the native display list's palette/material/lighting and cull setup.
    # Runtime redirects only its two rendering vertex loads to this exact copy.
    canopy_vertices = bytearray(native.get(NATIVE_CANOPY_VERTICES))
    assert len(canopy_vertices) == 680
    assert struct.unpack_from('<I', canopy_vertices, 8)[0] == 0
    assert struct.unpack_from('<II', canopy_vertices, 64) == (25, 38)
    for index in (0, 27):
        assert struct.unpack_from('<hhh', canopy_vertices, 72 + index * 16) == (213, 88, -214)
        struct.pack_into('<h', canopy_vertices, 72 + index * 16 + 2, CANOPY_PIVOT_Y)
    output[PREFIX + 'canopy_vtx'] = bytes(canopy_vertices)
    assert all(np.linalg.norm(t['v'][:, :3] - [0, 87, 0], axis=1).max() < 456 for t in bridge + wall + surround)
    manifest = dict(
        candidate='POC6 native accepted bridge',
        accepted_sha256=accepted_hash,
        native_sha256=hashlib.sha256(native_path.read_bytes()).hexdigest(),
        source_bridge=source_path,
        source_native_wall=NATIVE_WALL,
        native_wall_sha256=hashlib.sha256(native.get(NATIVE_WALL)).hexdigest(),
        wall_material=dict(
            texture=WALL,
            texture_sha256=hashlib.sha256(wall_texture).hexdigest(),
            native_sampler=1,
            logical_size=list(wall_tile),
            rgba16_bytes=wall_bytes,
            preservation='Native RGBA16 image, geometry, UVs and normals unchanged; baked load uses its actual dimensions',
        ),
        bridge_triangles=len(bridge),
        bridge_material_triangles={new.rsplit('/', 1)[-1]: counts[old] for old, new in TEXTURES.items()},
        original_wall_triangles=len(original_wall),
        clipped_wall_triangles=wall_triangles,
        outer_annulus_triangles=len(surround),
        mouth=[110, 0, -110],
        mouth_yaw=-8192,
        trigger_center=[160, 0, -160],
        native_wall_cull_sphere=[0, 87, 0, 456],
        aperture_depth=100,
        aperture_scale=.99,
        aperture_crown_clearance=3,
        aperture_floor=-2,
        floor_contact='Serialized Y1/2 wood floor and matching annulus vertices lowered4 units beneath the native grass; all higher vertices and every X/Z/UV/color remain exact',
        canopy_clearance=dict(
            source_display_list=NATIVE_CANOPY,
            source_display_list_sha256=hashlib.sha256(native.get(NATIVE_CANOPY)).hexdigest(),
            source_vertex_resource=NATIVE_CANOPY_VERTICES,
            source_vertex_sha256=hashlib.sha256(native.get(NATIVE_CANOPY_VERTICES)).hexdigest(),
            generated_vertex_resource=PREFIX + 'canopy_vtx',
            vertex_count=38,
            vertex_indices=[0, 27],
            pivot_old_xyz=[213, 88, -214],
            pivot_new_xyz=[213, CANOPY_PIVOT_Y, -214],
            patch_gfx_indices=[28, 29, 49, 50],
            unchanged_cull_gfx_indices=[2, 3],
            preservation='Only the two shared pivot Y fields change; native display-list materials, palettes, normals, UVs, alpha, bounds and all remaining vertex bytes are exact',
        ),
        preservation='All194 accepted bridge triangles and texture resource bytes; bridge floor Y1/2 change by-4; native canopy shared pivot Y88 changes to119; native cardinal corridors, floor, collision, headers, actors and route logic unchanged',
        asset_sha256={name: hashlib.sha256(data).hexdigest() for name, data in sorted(output.items())},
    )
    return output, manifest


def main():
    repo = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native', type=Path, required=True)
    parser.add_argument('--accepted', type=Path, required=True)
    parser.add_argument('--output', type=Path, default=repo / 'mm/assets/custom', help='2ship custom-asset root')
    parser.add_argument('--manifest', type=Path, default=Path(__file__).with_name('native_bridge_manifest.json'))
    args = parser.parse_args()
    files, manifest = build(args.native, args.accepted)
    for name, data in sorted(files.items()):
        target = args.output / name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
    args.manifest.parent.mkdir(parents=True, exist_ok=True)
    args.manifest.write_text(json.dumps(manifest, indent=2, sort_keys=True) + '\n')
    print('Built accepted 194-triangle bridge and native diagonal-only wall replacement into ' + str(args.output))


if __name__ == '__main__':
    main()
