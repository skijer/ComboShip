#!/usr/bin/env python3
"""Build the selected TP-to-MM get-item model port, without editing donor archives.

Requires mpyq; accepts explicit donor paths. The live colours/shimmer live in
the companion source candidate, not in this independently usable model pack.
"""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import re
import struct
import sys
import xml.etree.ElementTree as ET
import zipfile

NAME = 'zzzz_MM_TP_Dungeon_Bottles_POC1.o2r'
PRIVATE = 'objects/tp_dungeon_bottles_poc1/'
MAP = 'objects/object_gi_map/gGiDungeonMapDL'
COMPASS = 'objects/object_gi_compass/gGiCompassDL'
GLASS = 'objects/object_gi_compass/gGiCompassGlassDL'
CONTENTS = 'objects/object_gi_ghost/gGiPoeContainerContentsDL'
VERTICES = 'objects/object_gi_ghost/object_gi_ghostVtx_000910'
ROUTES = {
    MAP: MAP,
    'objects/object_gi_ghost/gGiGhostContainerLidDL': 'objects/object_gi_ghost/gGiPoeContainerLidDL',
    'objects/object_gi_ghost/gGiGhostContainerGlassDL': 'objects/object_gi_ghost/gGiPoeContainerGlassDL',
}
EXTENDED = {0x20, 0x31, 0x32, 0x33, 0x35, 0x36, 0x42}


def sha(blob):
    return hashlib.sha256(blob).hexdigest()


def file_sha(path):
    with path.open('rb') as handle:
        return hashlib.file_digest(handle, 'sha256').hexdigest()


def crc64(name):
    value = 0xFFFFFFFFFFFFFFFF
    for byte in name.encode():
        value ^= byte << 56
        for _ in range(8):
            value = ((value << 1) ^ (0x42F0E1EBA9EA3693 if value >> 63 else 0)) & 0xFFFFFFFFFFFFFFFF
    return value


def commands(blob):
    assert blob[4:8] == b'TLDO'
    offset = 72
    while offset + 8 <= len(blob):
        w0, w1 = struct.unpack_from('<II', blob, offset)
        opcode = w0 >> 24
        offset += 8
        h = None
        if opcode in EXTENDED:
            high, low = struct.unpack_from('<II', blob, offset)
            h = (high << 32) | low
            offset += 8
        yield opcode, w0, w1, h, offset - 8
        if opcode == 0xDF:
            return
    raise ValueError('Unterminated native display list')


def xml_bytes(root):
    ET.indent(root, space='  ')
    return ET.tostring(root) + b'\n'


def wrapper(path):
    root = ET.Element('DisplayList', Version='0')
    ET.SubElement(root, 'CallDisplayList', Path=path)
    ET.SubElement(root, 'EndDisplayList')
    return xml_bytes(root)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for option in ['models', 'textures', 'native-mm', 'source-repo', 'output']:
        parser.add_argument('--' + option, type=Path, required=True)
    parser.add_argument('--mpyq-dir', type=Path)
    args = parser.parse_args()
    if args.mpyq_dir:
        sys.path.insert(0, str(args.mpyq_dir))
    from mpyq import MPQArchive

    args.output.mkdir(parents=True, exist_ok=True)
    inputs = {key: {'file': str(path), 'sha256': file_sha(path)} for key, path in
              [('models', args.models), ('textures', args.textures), ('native_mm', args.native_mm)]}
    archives = {key: MPQArchive(str(path)) for key, path in [('models', args.models), ('textures', args.textures)]}
    names = {key: set(archive.read_file('(listfile)').decode().splitlines()) for key, archive in archives.items()}
    native = zipfile.ZipFile(args.native_mm)
    native_names = set(native.namelist())
    targets = set(ROUTES.values()) | {COMPASS, GLASS, CONTENTS}
    assert targets <= native_names
    draw_source = (args.source_repo / 'mm/src/code/z_draw.c').read_text()
    for target in targets:
        assert Path(target).name in draw_source

    source, origin, edits = {}, {}, {}

    def donor(path):
        for kind in ['textures', 'models']:
            if 'alt/' + path in names[kind]:
                return archives[kind].read_file('alt/' + path), kind
        raise ValueError('Missing TP dependency: ' + path)

    # TP puts the transparent compass lens in the same root as the opaque housing.
    # Keep the material/triangles/revert triples intact and route each pass separately.
    compass_blob, _ = donor(COMPASS)
    compass = ET.fromstring(compass_blob)
    calls = compass.findall('CallDisplayList')
    assert len(calls) == 6 and 'glass' in calls[0].attrib['Path'] and 'body' in calls[3].attrib['Path']
    compass_roots = {}
    for target, selected in [(COMPASS, calls[3:6]), (GLASS, calls[:3])]:
        root = ET.Element('DisplayList', Version='0')
        root.extend(copy.deepcopy(selected))
        root.extend(copy.deepcopy([node for node in compass if node.tag != 'CallDisplayList']))
        compass_roots[target] = root

    pending = list(ROUTES) + [node.attrib['Path'] for node in calls]
    allowed = ('objects/object_gi_map/', 'objects/object_gi_compass/', 'objects/object_gi_ghost/')
    while pending:
        path = pending.pop()
        if path in source:
            continue
        assert path.startswith(allowed), 'Unexpected dependency: ' + path
        blob, kind = donor(path)
        source[path], origin[path] = blob, kind
        if blob.startswith(b'<'):
            pending.extend(n.attrib['Path'] for n in ET.fromstring(blob).iter() if 'Path' in n.attrib)
        else:
            assert blob[4:8] == b'XETO'

    # Keep the MM scrolling/billboard contents, fitted to the TP shell. Only vertex
    # positions change: texture coordinates, colours, native textures and commands survive.
    hashes = {crc64(path): path for path in native_names if path.startswith('objects/object_gi_ghost/')}
    pending = [CONTENTS]
    while pending:
        path = pending.pop()
        if path in source:
            continue
        blob = native.read(path)
        source[path], origin[path] = blob, 'native_mm'
        if blob[4:8] == b'TLDO':
            for opcode, w0, w1, h, offset in commands(blob):
                if h is not None:
                    assert h in hashes, 'Unexpected native reference'
                    pending.append(hashes[h])
                if opcode == 0xDE:
                    assert w1 == 0x08000001, 'Only the native segment-8 scroll list is dynamic'

    mapping = {path: PRIVATE + path.removeprefix('objects/') for path in source}
    blobs, records = {}, []
    for path, blob in source.items():
        out = blob
        if blob.startswith(b'<'):
            out = re.sub(rb'Path="([^"]+)"', lambda m: ('Path="' + mapping[m[1].decode()] + '"').encode(), blob)
            expected = ET.fromstring(blob)
            for node in expected.iter():
                if 'Path' in node.attrib:
                    node.attrib['Path'] = mapping[node.attrib['Path']]
            assert ET.tostring(expected) == ET.tostring(ET.fromstring(out))
        elif blob[4:8] == b'TLDO':
            out = bytearray(blob)
            for opcode, w0, w1, h, offset in commands(blob):
                if h is not None:
                    new_hash = crc64(mapping[hashes[h]])
                    struct.pack_into('<II', out, offset, new_hash >> 32, new_hash & 0xFFFFFFFF)
            out = bytes(out)
        elif path == VERTICES:
            assert blob[4:8] == b'RRAO' and struct.unpack_from('<II', blob, 64) == (25, 4)
            out = bytearray(blob)
            for i in range(4):
                x, y, z = struct.unpack_from('<hhh', blob, 72 + i * 16)
                struct.pack_into('<hhh', out, 72 + i * 16, round(x * 0.75), round(y * 0.75 + 2), z)
                assert out[78 + i * 16:88 + i * 16] == blob[78 + i * 16:88 + i * 16]
            out = bytes(out)
            edits[path] = 'Poe contents XY scaled by 0.75; Y offset +2 to fit TP glass; UVs/colour retained'
        else:
            assert out == blob and blob[4:8] == b'XETO'
        blobs[mapping[path]] = out
        records.append({'source': path, 'archive': origin[path], 'target': mapping[path],
                        'source_sha256': sha(blob), 'output_sha256': sha(out), 'edit': edits.get(path, 'resource paths only')})

    for donor_path, target in ROUTES.items():
        blobs[target] = wrapper(mapping[donor_path])
    for target, root in compass_roots.items():
        for node in root.iter():
            if 'Path' in node.attrib:
                node.attrib['Path'] = mapping[node.attrib['Path']]
        blobs[target] = xml_bytes(root)
    blobs[CONTENTS] = wrapper(mapping[CONTENTS])
    assert {path for path in blobs if not path.startswith(PRIVATE)} == targets

    factory = (args.source_repo / 'libultraship/src/fast/resource/factory/DisplayListFactory.cpp').read_text()
    counts = {'display_lists': 0, 'vertex_arrays': 0, 'vertices': 0, 'triangles': 0, 'textures': 0}
    texture_info, bounds = {}, {}
    output_hashes = {crc64(path): path for path in blobs}
    for path, blob in blobs.items():
        if blob.startswith(b'<'):
            root = ET.fromstring(blob)
            if root.tag == 'Vertex':
                vertices = root.findall('Vtx')
                counts['vertex_arrays'] += 1
                counts['vertices'] += len(vertices)
                bounds[path] = {axis: [min(int(v.attrib[axis]) for v in vertices), max(int(v.attrib[axis]) for v in vertices)]
                                for axis in ['X', 'Y', 'Z']}
            else:
                assert root.tag == 'DisplayList'
                counts['display_lists'] += 1
                for node in root:
                    assert '"' + node.tag + '"' in factory, 'Unsupported XML command: ' + node.tag
            for node in root.iter():
                if 'Path' in node.attrib:
                    assert node.attrib['Path'] in blobs
        elif blob[4:8] == b'XETO':
            version = struct.unpack_from('<I', blob, 8)[0]
            if version == 1:
                typ, w, h, flags, hs, vs, size = struct.unpack_from('<IIIIffI', blob, 64)
                header = 92
            else:
                assert version == 0
                typ, w, h, size = struct.unpack_from('<IIII', blob, 64)
                flags, hs, vs, header = 0, 1, 1, 80
            bpp = 4 if flags & 1 else {1: 4, 2: 2, 5: 0.5, 6: 1, 8: 1, 9: 2}[typ]
            assert size == w * h * bpp and len(blob) == header + size
            texture_info[path] = (w, h, hs, vs, size, bpp)
            counts['textures'] += 1
        elif blob[4:8] == b'TLDO':
            counts['display_lists'] += 1
            for opcode, w0, w1, h, offset in commands(blob):
                if h is not None:
                    assert h in output_hashes
                if opcode == 0x06:
                    counts['triangles'] += 2
                if opcode == 0x32:
                    assert ((w0 >> 12) & 255) == 4 and w1 == 0
        else:
            assert blob[4:8] == b'RRAO' and struct.unpack_from('<II', blob, 64) == (25, 4)
            counts['vertex_arrays'] += 1
            counts['vertices'] += 4

    def walk(path, buffer, chain):
        assert path not in chain, 'Recursive display list'
        blob = blobs[path]
        if not blob.startswith(b'<'):
            return
        current_texture = None
        for node in ET.fromstring(blob):
            a = node.attrib
            if node.tag == 'CallDisplayList':
                walk(a['Path'], buffer, chain + [path])
            elif node.tag == 'LoadVertices':
                vertices = ET.fromstring(blobs[a['Path']]).findall('Vtx')
                offset, count, start = (int(a[k]) for k in ['VertexOffset', 'Count', 'VertexBufferIndex'])
                assert 0 <= offset and 0 < count <= 32 and offset + count <= len(vertices)
                assert 0 <= start and start + count <= 32
                buffer.update(range(start, start + count))
            elif node.tag in ['Triangle1', 'Triangles2']:
                indices = [int(v) for k, v in a.items() if re.fullmatch(r'V\d+', k)]
                assert len(indices) in [3, 6] and all(i in buffer for i in indices)
                counts['triangles'] += len(indices) // 3
            elif node.tag == 'SetTextureImage':
                current_texture = texture_info[a['Path']]
                assert (a['Format'], a['Size']) in {
                    ('G_IM_FMT_RGBA', 'G_IM_SIZ_16b_LOAD_BLOCK'),
                    ('G_IM_FMT_I', 'G_IM_SIZ_8b_LOAD_BLOCK'),
                }
            elif node.tag == 'LoadBlock' and current_texture:
                w, h, hs, vs, size, bpp = current_texture
                assert (int(a['Lrs']) + 1) * 2 * hs * vs == size
            elif node.tag == 'SetTile' and a['Tile'] != '7' and current_texture:
                w, h, hs, vs, size, bpp = current_texture
                assert int(a['Line']) * 8 * hs == w * bpp

    for root in sorted(targets):
        walk(root, set(), [])
    assert b'glass' not in blobs[COMPASS] and b'body' not in blobs[GLASS]
    assert b'G_RM_AA_ZB_XLU_SURF2' in blobs[mapping[calls[0].attrib['Path']]]
    assert len(targets) == 6
    outpath = args.output / NAME
    with zipfile.ZipFile(outpath, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for path, blob in sorted(blobs.items()):
            info = zipfile.ZipInfo('alt/' + path, (2026, 9, 22, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o644 << 16
            archive.writestr(info, blob, compresslevel=9)
    with zipfile.ZipFile(outpath) as archive:
        assert archive.testzip() is None and len(archive.namelist()) == len(blobs)
        assert all(archive.read('alt/' + path) == blob for path, blob in blobs.items())
    assert all(file_sha(Path(entry['file'])) == entry['sha256'] for entry in inputs.values())
    manifest = {'candidate': NAME, 'sha256': file_sha(outpath), 'bytes': outpath.stat().st_size,
                'status': 'Static verified; in-game rendering and acceptance pending', 'inputs': inputs,
                'roots': sorted(targets), 'namespace': PRIVATE, 'counts': counts, 'bounds': bounds,
                'resources': records, 'compass_source_sha256': sha(compass_blob),
                'verification': {'dependency_closure': True, 'vertices_and_triangles': True, 'texture_payloads': True,
                                 'compass_glass_separate': True, 'native_poe_scroll_preserved': True,
                                 'donors_unchanged': True, 'zip_round_trip': True}}
    (args.output / 'MM_TP_Dungeon_Bottles_POC1_manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(json.dumps({key: manifest[key] for key in ['candidate', 'sha256', 'bytes', 'counts', 'verification']}, indent=2))


if __name__ == '__main__':
    main()
