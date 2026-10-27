#!/usr/bin/env python3
"""Bake the accepted POC4 into deterministic, dependency-closed source parts.

Only authoring needs the unchanged native OoT and Djipi map donors. Normal
builds consume the committed parts and manifest; no mod archive is required.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys
import zipfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(ROOT / 'tools/temple_sages'))
import packaged_resources as dependencies
import resource_geometry as G
import build as chamber
import native_materials
import temple_sages_pool
import collision_audio

TEMPLE = 'scenes/shared/tokinoma_scene/'
PRIVATE = 'custom/temple_sages/'


def identity(data):
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}


def build(addon, base, maps, output):
    with zipfile.ZipFile(addon) as archive:
        result = {name: archive.read(name) for name in archive.namelist()
                  if not name.startswith('prelude/') and not name.endswith('/')}
    native, native_paths = G.archive(base)
    alternate, _ = G.archive(maps, ('alt/' + TEMPLE,))
    alternate = {dependencies.canonical(name): data for name, data in alternate.items()}
    # Private resources must have canonical entries for pre-Alt hash indexing
    # and for a physical chamber when Alt Assets is disabled.
    for name, data in list(result.items()):
        if name.startswith('alt/custom/'):
            result.setdefault(name[4:], data)
    host = chamber.render_resources({name: data for name, data in native.items()
                                     if name.startswith(TEMPLE)})
    root = TEMPLE + 'tokinoma_room_0DL_008128'
    clipped, count = G.aperture(host, [root], PRIVATE + 'native_host', chamber.DOOR_LO, chamber.DOOR_HI)
    if count != 3:
        raise ValueError('Unexpected native Temple doorway aperture count')
    result.update(clipped)
    result[PRIVATE + 'native_host_opa'] = result.pop(root, host[root])
    result[root] = chamber.call_list([PRIVATE + 'native_host_opa', PRIVATE + 'annex_opa'])
    # The addon previously relied on the external scene map for unchanged
    # Temple host leaves. Copy only the reachable host dependency closure.
    all_paths = set(native_paths) | set(result) | {dependencies.canonical(n) for n in result} | set(alternate)
    hashes = dependencies.hash_index(all_paths)
    native_needed = set()
    queue = [('alt/' if name.startswith('alt/') else '', dependencies.canonical(name)) for name in result]
    visited = set()
    while queue:
        prefix, name = queue.pop()
        if (prefix, name) in visited:
            continue
        visited.add((prefix, name))
        actual = prefix + name if prefix + name in result else name
        if prefix and name in alternate and prefix + name not in result:
            result[prefix + name] = alternate[name]
            actual = prefix + name
        data = result.get(actual, native.get(name))
        if data is None and name in alternate:
            # A canonical alias for an Alt-only private graph can refer to
            # host leaves which have no ROM identity. Keep their index entries
            # available without replacing any existing native host leaf.
            result[name] = alternate[name]
            actual = name
            data = result[name]
        if data is None:
            raise ValueError('Missing chamber dependency ' + name)
        if actual not in result:
            native_needed.add(name)
        for dependency in dependencies.references(data, hashes):
            queue.append((prefix, dependencies.canonical(dependency)))
    missing = dependencies.missing_dependencies(result, native_needed)
    if missing:
        raise ValueError('Unresolved chamber dependencies: ' + repr(missing[:8]))
    # The default base owns geometry and native artwork. The optional visual
    # archive keeps the original material/texture identities and overrides them.
    previous_paths = set(result)
    result, native_bindings = native_materials.apply(result, native)
    retired = sorted(previous_paths - set(result))
    collision_path = TEMPLE + 'tokinoma_sceneCollisionHeader_0032F8'
    for prefix in ('', 'alt/'):
        result[prefix + collision_path] = collision_audio.apply(result[prefix + collision_path])
    missing = dependencies.missing_dependencies(result, native_needed)
    if missing:
        raise ValueError('Unresolved native-base chamber dependencies: ' + repr(missing[:8]))
    bundle = temple_sages_pool.pack(result, output)
    manifest = {'schema': 1, 'bundle': bundle,
                'source': {'kind': 'native-chamber-geometry'},
                'native_dependencies': sorted(native_needed),
                'native_apertures': count, 'external_mod_dependencies': [],
                'texture_policy': 'native-oot-base-optional-visual-addon',
                'native_texture_bindings': native_bindings,
                'retired_resources': retired,
                'collision_audio': collision_audio.inspect(result[collision_path]),
                'resources': {name: identity(data) for name, data in sorted(result.items())},
                'runtime_verified': False}
    (output / 'manifest.json').write_text(json.dumps(manifest, indent=2, sort_keys=True) + '\n')
    current_parts = {entry['name'] for entry in bundle['parts']}
    for path in (output / 'parts').iterdir():
        if (path.is_file() and path.relative_to(output).as_posix() not in current_parts and
                path.name.startswith(('pool.zlib.', 'temple_sages.zip.part'))):
            path.unlink()
    print(f'PASS baked chamber base: {len(result)} resources, {len(native_needed)} inherited native resources, '
          f'{bundle["packed_bytes"]} compressed bytes in {len(bundle["parts"])} bounded text parts')
    return manifest


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for option in ('addon', 'base', 'maps'):
        parser.add_argument('--' + option, type=Path, required=True)
    parser.add_argument('--output', type=Path, default=HERE)
    args = parser.parse_args()
    build(args.addon, args.base, args.maps, args.output)
