#!/usr/bin/env python3
"""Create a self-contained optional visual archive with chamber floor acoustics.

The original archive is read only. The two scene collision resources receive
stone/concrete footsteps and native echo63 on walkable faces. Every visual,
room/setup/actor resource and every non-audio collision field is preserved.
An explicit authoring donor supplies only the pinned missing host materials
and textures. Geometry resources are never imported from that donor.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys
import xml.etree.ElementTree as ET
import zipfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/temple_sages_bundle'))
import collision_audio
sys.path.insert(0, str(ROOT / 'tools/temple_sages'))
import resource_geometry

CLOSURE = Path(__file__).with_name('optional_visual_closure.json')
HOST = 'scenes/shared/tokinoma_scene/'


def sha(data):
    return hashlib.sha256(data).hexdigest()


def visual_dependencies(resources, donor):
    """Copy only exact, pinned Alt materials/textures; preserve every input member."""
    metadata = json.loads(CLOSURE.read_text())
    inventory = metadata.get('resources', {})
    if metadata.get('schema') != 1 or len(inventory) != 53:
        raise ValueError('Invalid optional host visual inventory')
    candidates = None
    if donor is not None:
        candidates, _ = resource_geometry.archive(Path(donor), ('alt/' + HOST, HOST))
    added = {}
    for name, entry in inventory.items():
        target = entry['optional_path']
        if (target != 'alt/' + name or not name.startswith(HOST) or
                entry['source_path'] != target or entry['kind'] not in ('material', 'texture')):
            raise ValueError('Optional visual dependency escapes its material/texture scope')
        data = resources.get(target)
        if data is None:
            if candidates is None or entry['source_path'] not in candidates:
                raise ValueError('Missing optional visual dependency donor: ' + target)
            data = candidates[entry['source_path']]
        if len(data) != entry['bytes'] or sha(data) != entry['sha256']:
            raise ValueError('Optional visual dependency hash/size mismatch: ' + target)
        if entry['kind'] == 'texture':
            if data[4:8] != b'XETO':
                raise ValueError('Expected an optional host texture: ' + target)
        else:
            tree = ET.fromstring(data)
            if (tree.tag != 'DisplayList' or any(element.tag in ('Vertex', 'Vtx', 'Triangle1', 'Triangle2',
                    'CallDisplayList', 'CallDisplayListByHash', 'CullDisplayList') for element in tree.iter())):
                raise ValueError('Optional material contains a geometry command: ' + target)
        if target not in resources:
            resources[target] = data
            added[target] = {'bytes': len(data), 'sha256': sha(data)}
    return added


def patch_archive(source, output, visual_donor=None):
    source, output = Path(source), Path(output)
    if source.resolve() == output.resolve():
        raise ValueError('The original visual archive must remain unchanged')
    source_bytes = source.read_bytes()
    with zipfile.ZipFile(source) as archive:
        names = archive.namelist()
        if len(names) != len(set(names)):
            raise ValueError('Duplicate visual archive resource paths')
        resources = {name: archive.read(name) for name in names}
    targets = [collision_audio.COLLISION, 'alt/' + collision_audio.COLLISION]
    changed = {}
    before_hashes = {name: sha(data) for name, data in resources.items()}
    added_visual = visual_dependencies(resources, visual_donor)
    for name in targets:
        if name not in resources:
            raise ValueError('Missing visual archive collision: ' + name)
        before = resources[name]
        resources[name] = collision_audio.apply(before)
        changed[name] = {'before_sha256': sha(before), 'after_sha256': sha(resources[name]),
                         'before_bytes': len(before), 'after_bytes': len(resources[name]),
                         'audio': collision_audio.inspect(resources[name])}
    # Refresh the embedded authoring inventory so its two collision hashes
    # describe this candidate. Visual payloads and their recorded hashes stay
    # unchanged; the manifest does not hash itself.
    manifest_path = 'custom/temple_sages/manifest.json'
    metadata_change = None
    if manifest_path in resources:
        metadata = json.loads(resources[manifest_path])
        inventory = metadata.get('resources')
        if not isinstance(inventory, dict):
            raise ValueError('The visual authoring manifest has no resource inventory')
        for name in targets:
            if inventory.get(name) != before_hashes[name]:
                raise ValueError('The visual authoring manifest has a stale collision hash')
            inventory[name] = sha(resources[name])
        for name, data in resources.items():
            if name not in before_hashes:
                inventory[name] = sha(data)
        metadata['visual_dependency_closure'] = {
            'inventory_sha256': sha(CLOSURE.read_bytes()),
            'material_resources': 26,
            'texture_resources': 27,
            'geometry_resources': 0,
            'alt_only': True,
        }
        metadata['collision_audio'] = {
            'stone_sound_index': collision_audio.STONE,
            'native_echo': collision_audio.ECHO,
            'floor_normal_y_strict_minimum': collision_audio.FLOOR_NORMAL_MIN,
            'geometry_camera_water_and_other_surface_flags_preserved': True,
        }
        resources[manifest_path] = json.dumps(metadata, sort_keys=True, separators=(',', ':')).encode()
        metadata_change = {'path': manifest_path, 'before_sha256': before_hashes[manifest_path],
                           'after_sha256': sha(resources[manifest_path])}
    untouched = [name for name in names if name not in targets and name != manifest_path]
    if any(sha(resources[name]) != before_hashes[name] for name in untouched):
        raise ValueError('A visual or other protected resource changed')
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = output.with_name('.' + output.name + '.tmp')
    try:
        with zipfile.ZipFile(temporary, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
            for name, data in sorted(resources.items()):
                info = zipfile.ZipInfo(name, (2026, 10, 27, 0, 0, 0))
                info.compress_type = zipfile.ZIP_DEFLATED
                info.external_attr = 0o100644 << 16
                info.create_system = 3
                archive.writestr(info, data, compress_type=zipfile.ZIP_DEFLATED, compresslevel=9)
        # Verify the delivered archive, not only the pre-write dictionary.
        with zipfile.ZipFile(temporary) as archive:
            if set(archive.namelist()) != set(resources) or archive.testzip() is not None:
                raise ValueError('Visual archive membership or CRC mismatch')
            for name, data in resources.items():
                if archive.read(name) != data:
                    raise ValueError('Visual archive output mismatch: ' + name)
        temporary.replace(output)
    finally:
        temporary.unlink(missing_ok=True)
    if source.read_bytes() != source_bytes:
        raise ValueError('The original visual archive changed')
    receipt = {'schema': 1, 'source_sha256': sha(source_bytes),
               'output': output.name, 'output_bytes': output.stat().st_size,
               'output_sha256': sha(output.read_bytes()), 'resources': len(resources),
               'changed_collision_resources': changed,
               'updated_authoring_manifest': metadata_change,
               'added_visual_dependencies': added_visual,
               'unchanged_visual_and_other_resources': len(untouched),
               'native_stone_sound_index': collision_audio.STONE,
               'native_collision_echo': collision_audio.ECHO,
               'runtime_verified': False}
    output.with_suffix('.verification.json').write_text(json.dumps(receipt, indent=2, sort_keys=True) + '\n')
    return receipt


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--visual-donor', type=Path,
                        help='Authoring-only archive with the pinned host visual leaves; unnecessary when already present')
    args = parser.parse_args()
    try:
        result = patch_archive(args.source, args.output, args.visual_donor)
    except (ValueError, OSError, zipfile.BadZipFile) as error:
        parser.exit(1, str(error) + '\n')
    print(f'PASS optional chamber audio: {result["resources"]} resources, '
          f'{result["unchanged_visual_and_other_resources"]} protected resources unchanged; '
          f'stone footsteps, native echo{result["native_collision_echo"]}')
