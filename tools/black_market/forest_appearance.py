#!/usr/bin/env python3
"""Optional MM3D terrain appearance for the pinned native-v2 forest.

Only six room mesh commands are overridden. Gameplay and every authored
addition, material, scroll track and log remain owned by native port assets.
"""
import argparse
import io
import json
from pathlib import Path
import struct
import xml.etree.ElementTree as ET
import zipfile

import native_forest as nf

DONOR_SHA = '65901fcc27453a915be9414c7203e8295fce36ce69b7658653bbcdf978af8d0e'
NATIVE_SHA = 'b0a33016b32d7e5415090038cff4cb8f4328cc8fd8a4b3f09d55918318be9fb5'
SOLO = 'custom/skiejer_forest/'
COMBINED = 'custom/cor_mm3d/lost_woods_poc1/'
APPEARANCE = 'custom/comboship/forest_appearance_v1/'
META = APPEARANCE + 'manifest.json'
SEASONAL = ('lw_enkei_01_0', 'lw_kabe_01_5400', 'lw_kusa_01_12600',
            'lw_wood_02_2CA00', 'lw_wood_03_31E00', 'lw_wood_04_37200',
            'lw_yuka_02_39C00', 'lw_yuka_03_44400')
TERRAIN_TEXTURES = (*SEASONAL, 'lw_kinoko_01_FC00', 'lw_nenrin_01_27600', 'lw_tutu_01_2A000')
TERRAIN_DRAWS = tuple(f'room_{room}/Geometry{index if index else ""}'
                      for room, indices in ((0, range(11)), (1, range(11, 22))) for index in indices)
NATIVE_OPA = frozenset(nf.PRIVATE + name for name in ('building_root', 'portal_root', 'mirror_access_root'))


def _root(mode, room):
    return APPEARANCE + ('combined' if mode == 'combined' else 'solo') + f'/room_{room}_opa'


def _room_overrides():
    return {'alt/' + prefix + f'_room_{room}' for prefix in nf.SCENES.values() for room in (0, 1)}


def _art_paths():
    return ({SOLO + name + suffix for name in TERRAIN_DRAWS for suffix in ('', '_vtx')} |
            {SOLO + 'textures/' + name for name in TERRAIN_TEXTURES} |
            {COMBINED + name for name in TERRAIN_DRAWS} |
            {COMBINED + 'textures/' + name for name in SEASONAL})


def _scope(resources):
    overrides = _room_overrides()
    roots = {_root(mode, room) for mode in ('combined', 'mm') for room in (0, 1)}
    allowed = overrides | roots | _art_paths() | {META}
    if {name for name in resources if name.startswith('alt/')} != overrides:
        raise ValueError('Appearance override scope must contain exactly six private room meshes')
    for name in resources:
        if not nf.safe_resource_path(name) or name not in allowed:
            raise ValueError('Outside appearance terrain scope; native resources cannot be copied: ' + name)


def _retained(mesh):
    return [entry for entry in mesh['entries'] if entry['opaque'] in NATIVE_OPA or
            entry['translucent'] == nf.PRIVATE + 'stump_shimmer_root']


def _point_reader(resources):
    cache, active = {}, set()
    def points(path):
        if not path:
            return []
        if path in cache:
            return cache[path]
        if path not in resources or path in active:
            raise ValueError('Missing or recursive terrain geometry dependency: ' + path)
        active.add(path)
        data, result = resources[path], []
        if data.startswith(b'<Vertex'):
            result = [tuple(int(node.get(axis)) for axis in ('X', 'Y', 'Z')) for node in ET.fromstring(data)]
        elif data[4:8] == b'RRAO':
            result = nf.vertex_positions(data)
        elif data.startswith(b'<DisplayList'):
            for node in ET.fromstring(data):
                if node.tag in ('LoadVertices', 'CallDisplayList'):
                    result.extend(points(node.get('Path')))
        active.remove(path)
        cache[path] = result
        return result
    return points


def validate(resources, native):
    """Check mesh-only scope, base ownership, preserved records and draw closure."""
    _scope(resources)
    if nf.AUTHORED_MARKER not in native or nf.PRIVATE + 'mirror_access_root' not in native:
        raise ValueError('Appearance requires complete native v2 presentation and Mirror access')
    nf.validate_resources(native)
    stock = set()
    for mode in nf.SCENES:
        descriptor = nf.dependency_path(mode)
        if descriptor not in native:
            raise ValueError('Missing native dependency descriptor: ' + descriptor)
        core, external = nf.read_dependency_blob(native[descriptor])
        missing = set(core) - native.keys()
        if missing:
            raise ValueError('Missing native dependency: ' + ', '.join(sorted(missing)))
        stock.update(external)
    effective = dict(native)
    effective.update({name.removeprefix('alt/'): data for name, data in resources.items()})
    hashes = {nf.crc64(name): name for name in [*effective, *stock]}
    reached, active, segments = set(), set(), set()
    def walk(path):
        if not path or path in reached:
            return
        if path in active:
            raise ValueError('Recursive appearance draw dependency: ' + path)
        if path not in effective:
            if path not in stock:
                raise ValueError('Missing appearance draw dependency: ' + path)
            reached.add(path)
            return
        active.add(path)
        data = effective[path]
        if data.startswith(b'<DisplayList'):
            for node in ET.fromstring(data):
                if node.get('Path'):
                    walk(node.get('Path'))
        elif data[4:8] == b'TLDO':
            for offset, w0, w1, opcode in nf.display_commands(data, path):
                if opcode in nf.EXPANDED and opcode != 0x33:
                    hi, lo = struct.unpack_from('<II', data, offset + 8)
                    dependency = hashes.get((hi << 32) | lo)
                    if dependency is None:
                        raise ValueError('Missing native/appearance CRC64 dependency: ' + path)
                    walk(dependency)
                elif opcode == 0xDE:
                    if w0 != 0xDE000000 or w1 != ((w1 >> 24) << 24) | 1 or w1 >> 24 not in range(8, 14):
                        raise ValueError('Invalid native authored scroll call: ' + path)
                    segments.add(w1 >> 24)
        active.remove(path)
        reached.add(path)
    terrain_points = _point_reader(resources)
    for mode, prefix in nf.SCENES.items():
        reached.clear()
        for room in (0, 1):
            path = prefix + f'_room_{room}'
            if path not in native:
                raise ValueError('Missing native room dependency: ' + path)
            old = nf.read_scene(native[path], oot=mode == 'oot')
            new = nf.read_scene(resources['alt/' + path], oot=mode == 'oot')
            if {key: value for key, value in old.items() if key != 10} != \
                    {key: value for key, value in new.items() if key != 10}:
                raise ValueError('Appearance changed native gameplay room commands: ' + path)
            old_mesh, mesh = nf.read_mesh(old[10]), nf.read_mesh(new[10])
            retained = _retained(old_mesh)
            if len(retained) != (4 if room == 0 else 0):
                raise ValueError('Unexpected native v2 addition entries: ' + path)
            root = _root(mode, room)
            expected_entry = {'type': 2, 'bounds': nf.bounding_sphere(terrain_points(root)),
                              'opaque': root, 'translucent': ''}
            if mesh != {'data': old_mesh['data'], 'type': 2, 'entries': [expected_entry, *retained]}:
                raise ValueError('Appearance changed native additions or terrain culling: ' + path)
            for entry in mesh['entries']:
                walk(entry['opaque'])
                walk(entry['translucent'])
        owner = COMBINED if mode == 'combined' else SOLO
        if not {owner + 'textures/' + name for name in SEASONAL} <= reached:
            raise ValueError('Seasonal appearance texture identities are not reachable: ' + prefix)
    if segments != set(range(8, 14)):
        raise ValueError('Appearance lost native authored scroll closure')
    return {'room_mesh_overrides': 6, 'native_addition_entries_preserved': 12,
            'native_resource_overrides': 0, 'native_resources_copied': 0,
            'native_scroll_segments': sorted(segments), 'draw_closure': 'passed'}


def native_bytes(path):
    """Accept the canonical ZIP or the verified tracked source-parts directory."""
    path = Path(path)
    if path.is_dir():
        from native_forest_bundle import load_bundle
        return load_bundle(path)
    return path.read_bytes()


def build(donor, native):
    data = native_bytes(native)
    if nf.file_sha(donor) != DONOR_SHA or nf.sha(data) != NATIVE_SHA:
        raise ValueError('Input identity differs from immutable native-v2/donor SHA256 baselines')
    with zipfile.ZipFile(io.BytesIO(data)) as base:
        baseline = dict(nf.ArchiveResources(base))
    source, roots = {}, {}
    allowed_source = {path for path in _art_paths() if path.startswith(SOLO)}
    with zipfile.ZipFile(donor) as archive:
        indexed = nf.ArchiveResources(archive)
        active = set()
        def collect(path):
            if path in source:
                return
            if path not in allowed_source or path in active:
                raise ValueError('Outside exact terrain closure or recursive donor reference: ' + path)
            active.add(path)
            data = indexed[path]
            if data.startswith(b'<DisplayList'):
                for node in ET.fromstring(data):
                    if node.get('Path'):
                        collect(node.get('Path'))
            elif not (data.startswith(b'<Vertex') or data[4:8] in (b'XETO', b'RRAO')):
                raise ValueError('Unsupported terrain donor encoding: ' + path)
            active.remove(path)
            source[path] = data
        for room in (0, 1):
            original = ET.fromstring(indexed[SOLO + f'room_{room}/opa'])
            paths = [node.get('Path') for node in original if node.get('Path') in allowed_source]
            expected = [SOLO + name for name in TERRAIN_DRAWS if name.startswith(f'room_{room}/')]
            if paths != expected:
                raise ValueError('Donor terrain roots differ from the retained 22-mesh baseline')
            roots[room] = paths
            for path in paths:
                collect(path)
    if set(source) != allowed_source:
        raise ValueError('Exact terrain source inventory is incomplete')
    resources = dict(source)
    mapping = {SOLO + name: COMBINED + name for name in TERRAIN_DRAWS}
    mapping.update({SOLO + 'textures/' + name: COMBINED + 'textures/' + name for name in SEASONAL})
    for original, target in mapping.items():
        data = source[original]
        if data.startswith(b'<DisplayList'):
            tree = ET.fromstring(data)
            for node in tree:
                if node.get('Path'):
                    node.set('Path', mapping.get(node.get('Path'), node.get('Path')))
            data = nf._xml(tree)
        resources[target] = data
    for mode in ('combined', 'mm'):
        for room in (0, 1):
            resources[_root(mode, room)] = nf._calls(mapping.get(path, path) if mode == 'combined' else path
                                                     for path in roots[room])
    points = _point_reader(resources)
    culling = {}
    for mode, prefix in nf.SCENES.items():
        for room in (0, 1):
            path = prefix + f'_room_{room}'
            commands = nf.read_scene(baseline[path], oot=mode == 'oot')
            original = nf.read_mesh(commands[10])
            root = _root(mode, room)
            entry = {'type': 2, 'bounds': nf.bounding_sphere(points(root)), 'opaque': root, 'translucent': ''}
            commands[10] = nf.write_mesh({'data': original['data'], 'type': 2, 'entries': [entry, *_retained(original)]})
            resources['alt/' + path] = nf.write_scene(commands)
            culling[path] = list(entry['bounds'])
    report = {
        'candidate': 'Lost Woods MM3D appearance for native v2 — POC1', 'schema': 1,
        'donor': {'name': Path(donor).name, 'sha256': DONOR_SHA},
        'requires_native_sha256': NATIVE_SHA, 'requires_native_presentation_version': 2,
        'requires_mirror_access': True, 'terrain_sources': sorted(source),
        'room_overrides': sorted(_room_overrides()), 'culling': culling,
        'seasonal_aliases': {SOLO + 'textures/' + name: COMBINED + 'textures/' + name for name in SEASONAL},
        'preserved': 'All native v2 gameplay, green portal, shop/poster/bench, six scroll tracks, stump shimmer and log remain native-owned; exact terrain pixels/vertices retained',
        'installation': 'Matching native-v2 build/assets and normal MM base owner required. Enable Alt Assets. Install one appearance pack in the MM host for combined/MM solo, and the OoT host for OoT solo. Disable older full-scene/retarget forest packs. Restart after changing packs.',
        'runtime': 'unverified configured gameplay; source/archive/native-readiness checks only',
        'promotion': 'candidate only; accepted input archives unchanged',
    }
    # Scope/closure validation does not rely on this informational manifest.
    resources[META] = (json.dumps(report, indent=2, sort_keys=True) + '\n').encode()
    report['verification'] = validate(resources, baseline)
    resources[META] = (json.dumps(report, indent=2, sort_keys=True) + '\n').encode()
    return resources, report


def write(resources, report, output):
    _scope(resources)
    output = Path(output)
    if output.exists():
        raise ValueError('Output exists; choose a new candidate and preserve accepted inputs')
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open('xb') as stream, zipfile.ZipFile(stream, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for path, data in sorted(resources.items()):
            info = zipfile.ZipInfo(path, (1980, 1, 1, 0, 0, 0))
            info.create_system, info.external_attr, info.compress_type = 3, 0o100644 << 16, zipfile.ZIP_DEFLATED
            archive.writestr(info, data, compresslevel=9)
    return {**report, 'archive': {'name': output.name, 'bytes': output.stat().st_size,
                                'sha256': nf.file_sha(output)}, 'resources': len(resources)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--donor', type=Path, required=True)
    parser.add_argument('--native', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--receipt', type=Path, required=True)
    args = parser.parse_args()
    if (len({path.resolve() for path in (args.donor, args.native, args.output, args.receipt)}) != 4 or
            args.output.exists() or args.receipt.exists()):
        parser.error('Choose separate new outputs; immutable inputs and existing candidates cannot be overwritten')
    resources, report = build(args.donor, args.native)
    receipt = write(resources, report, args.output)
    args.receipt.parent.mkdir(parents=True, exist_ok=True)
    args.receipt.write_text(json.dumps(receipt, indent=2, sort_keys=True) + '\n')
    print(json.dumps({'archive': receipt['archive'], 'resources': len(resources),
                      'verification': receipt['verification']}, indent=2))


if __name__ == '__main__':
    main()
