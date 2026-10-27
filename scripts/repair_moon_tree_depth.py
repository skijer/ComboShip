#!/usr/bin/env python3
"""Repair depth testing in cor's exact Termina Moon-tree export, without replacing it.

Only the G_ZBUFFER bit in each tree draw's geometry-mode reset changes. Update
codec-2 stored display lists as well, preserving the editable Prelude project.
"""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import struct
import zipfile

from diagnostics.check_moon_tree_depth import PROJECT, POOL, audit, blob_range, commands, tree_shapes

BASELINE_SHA256 = '9fc14e4cc00a40aa0a95c17828008bee6c0fa1d303804bedb1999e36615bd062'


def sha(path):
    digest = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()


def patched_draw(data):
    matches = [offset for offset, w0, w1, opcode in commands(data)
               if w0 == 0xD9000000 and w1 == 4]
    if len(matches) != 1:
        raise ValueError('Expected exactly one G_SHADE-only geometry-mode reset')
    offset = matches[0] + 4
    result = bytearray(data)
    struct.pack_into('<I', result, offset, 5)  # G_SHADE | G_ZBUFFER; every other bit stays exact.
    return bytes(result), offset


def verify_preservation(source, candidate, updates):
    with zipfile.ZipFile(source) as before, zipfile.ZipFile(candidate) as after:
        if before.namelist() != after.namelist():
            raise ValueError('Candidate changed resource inventory/order')
        changed, exact = [], 0
        for name in before.namelist():
            old, new = before.read(name), after.read(name)
            if new != updates.get(name, old):
                raise ValueError('Unexpected resource change: ' + name)
            if old == new:
                exact += 1
            else:
                if len(old) != len(new):
                    raise ValueError('Repair changed resource length: ' + name)
                positions = [i for i, (a, b) in enumerate(zip(old, new)) if a != b]
                if any(old[i] != 4 or new[i] != 5 for i in positions):
                    raise ValueError('Repair changed more than the depth-enable bit: ' + name)
                changed.append({'path': name, 'changed_bytes': len(positions)})
        if set(updates) != {row['path'] for row in changed}:
            raise ValueError('Candidate change inventory differs from the bounded patch')
        if after.testzip() is not None:
            raise ValueError('Candidate archive CRC failure')
    result = audit(candidate)
    if result['depth_failures']:
        raise ValueError('Candidate still contains untested-depth tree triangles')
    return {'unchanged_resources': exact, 'changed_resources': changed, **result}


def repair(source, output, receipt):
    source, output, receipt = map(Path, (source, output, receipt))
    if len({path.resolve() for path in (source, output, receipt)}) != 3 or output.exists() or receipt.exists():
        raise ValueError('Use separate new output and receipt paths; preserve the original archive')
    if sha(source) != BASELINE_SHA256:
        raise ValueError('Input differs from cor\'s exact supplied Termina Moon-tree archive')
    baseline = audit(source)
    if len(baseline['depth_failures']) != 134:
        raise ValueError('Baseline does not reproduce the 134-draw depth defect')
    with zipfile.ZipFile(source) as archive:
        project = json.loads(archive.read(PROJECT))
        pool = bytearray(archive.read(POOL))
        updates, rows, pool_offsets = {}, [], set()
        for shape in tree_shapes(project):
            name = shape['newDlPath']
            original = archive.read(name)
            fixed, offset = patched_draw(original)
            owner, start, length = blob_range(project, shape['stored']['dl'])
            if length != len(original):
                raise ValueError('Stored display-list length differs from the runtime draw')
            if owner == POOL:
                if pool[start:start + length] != original:
                    raise ValueError('Stored display-list bytes differ from runtime draw')
                pool[start + offset:start + offset + 4] = fixed[offset:offset + 4]
                pool_offsets.add(start + offset)
            elif owner != name or start != 0:
                raise ValueError('Unexpected external stored-display-list owner')
            updates[name] = fixed
            rows.append({'path': name, 'word_offset': offset,
                         'material': shape['stored']['textures'][0]['label'], 'stored_owner': owner})
        updates[POOL] = bytes(pool)
        # Preserve all other codec-2 blobs, including collision and vertex edits.
        selected_blobs = {shape['stored']['dl']['$blob'] for shape in tree_shapes(project)}
        for index, desc in enumerate(project['blobs']):
            if index in selected_blobs:
                continue
            owner = desc.get('p', POOL)
            start, end = desc['o'], desc['o'] + desc['n']
            if owner in updates and updates[owner][start:end] != archive.read(owner)[start:end]:
                raise ValueError('Patch would alter an unrelated embedded project blob')
        output.parent.mkdir(parents=True, exist_ok=True)
        with zipfile.ZipFile(output, 'x', zipfile.ZIP_DEFLATED, compresslevel=6) as target:
            for original_info in archive.infolist():
                info = copy.copy(original_info)
                info.date_time = (1980, 1, 1, 0, 0, 0)
                info.compress_type = zipfile.ZIP_DEFLATED
                target.writestr(info, updates.get(info.filename, archive.read(info.filename)))
    verification = verify_preservation(source, output, updates)
    if sha(source) != BASELINE_SHA256:
        raise ValueError('Original input changed')
    report = {
        'candidate': output.name, 'baseline': source.name, 'baseline_sha256': BASELINE_SHA256,
        'candidate_sha256': sha(output), 'candidate_bytes': output.stat().st_size,
        'fix': 'G_SHADE-only full geometry reset 0x00000004 becomes G_SHADE|G_ZBUFFER 0x00000005',
        'runtime_tree_draws': len(rows), 'embedded_pool_draws': len(pool_offsets),
        'preserved': 'All vertices, triangles, UVs, textures, palettes, alpha, collision, actors, placements, scene/room headers, culling, project JSON and unrelated draws remain byte-identical',
        'changes': rows, 'verification': verification,
        'status': 'Archive/command verified candidate; configured game runtime and user acceptance pending',
        'installation': 'Use this candidate in place of the original Termina tree archive, in the same host and load-order position; keep Alt Assets enabled. Keep the original as rollback and load one version at a time.',
    }
    receipt.parent.mkdir(parents=True, exist_ok=True)
    with receipt.open('x') as stream:
        json.dump(report, stream, indent=2)
        stream.write('\n')
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--receipt', type=Path, required=True)
    args = parser.parse_args()
    report = repair(args.input, args.output, args.receipt)
    print(json.dumps({key: report[key] for key in ('candidate', 'candidate_sha256', 'runtime_tree_draws', 'embedded_pool_draws', 'status')}, indent=2))


if __name__ == '__main__':
    main()
