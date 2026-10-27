#!/usr/bin/env python3
"""Losslessly merge adjacent 32-vertex NPC batches in an existing O2R.

Only the single-material native_npc_models.emit_mesh contract is supported.
Triangle order, winding, matrix ownership and all 16 vertex bytes remain exact.
Textures, palettes, skeletons, animations and unsupported resources are preserved.
The resulting archive is a runtime-unverified POC, never an accepted master.
"""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict
from dataclasses import dataclass
import hashlib
import json
from pathlib import Path
import struct
import zipfile

from native_assets import o2r

DEFAULT_NAMESPACES = tuple('objects/'+name+'/NativeNpcV1/' for name in
                           ('object_kw1', 'object_sa', 'object_ma1'))
EXPANDED = {0x20, 0x31, 0x32, 0x33, 0x35, 0x36, 0x42}
PREFIX_OPS = (0x33, 0xe7, 0xe3, 0xd7, 0xfc, 0xd9, 0xd9, 0xfa,
              0x20, 0xf5, 0xe6, 0xf4, 0xe7, 0xf5, 0xf2)
FIXED_PREFIX = ((0xE7000000, 0), (0xE3001001, 0), (0xD7000002, 0xFFFFFFFF),
                (0xFC127E03, 0xFFFFFBFD), (0xD9F1F9FF, 0),
                (0xD9FFFFFF, 0x00830005), (0xFA000000, 0xFFFFFFFF))
FIXED_TILES = ((0xF5180000, 0x07080200), (0xE6000000, 0), (0xF4000000, 0x077FC7FC),
               (0xE7000000, 0), (0xF5190000, 0x00080200), (0xF2000000, 0x007FC7FC))


def _require(condition, message):
    if not condition:
        raise ValueError(message)


def _commands(raw):
    _require(len(raw) >= 80 and raw[0] == 0 and raw[4:8] == b'TLDO' and raw[64] == 4,
             'unsupported display-list header or microcode')
    result, at = [], 72
    while at + 8 <= len(raw):
        a, b = struct.unpack_from('<II', raw, at)
        op = a >> 24
        size = 16 if op in EXPANDED else 8
        _require(at + size <= len(raw), 'truncated display-list command')
        hashed = None
        if size == 16:
            hi, lo = struct.unpack_from('<II', raw, at+8)
            hashed = hi << 32 | lo
        result.append((at, op, a, b, hashed))
        at += size
        if op == 0xdf:
            _require(at == len(raw), 'unsupported data following end-display-list')
            return result
    raise ValueError('missing end-display-list')


def _index(records):
    result = {}
    for name in records:
        hashed = o2r.crc64(name)
        _require(hashed not in result, 'indexed resource hash collision')
        result[hashed] = name
    return result


def _matrix(command):
    _, _, a, b, _ = command
    _require(a == 0xda380003 and b & 0xff00003f == 0x0d000001 and (b-0x0d000001)//64 < 256,
             'unsupported matrix ownership; expected segment-D load slot')
    return b


@dataclass
class Mesh:
    prefix: bytes
    restore: bytes
    vertex_path: str
    triangles: list
    stats: dict


def _decode(records, path, index=None):
    index = _index(records) if index is None else index
    raw = records[path]
    cmds = _commands(raw)
    _require(len(cmds) >= 18 and tuple(c[1] for c in cmds[:15]) == PREFIX_OPS,
             'unsupported single-material emit_mesh preamble')
    _require(tuple((c[2], c[3]) for c in cmds[1:8]) == FIXED_PREFIX and
             tuple((c[2], c[3]) for c in cmds[9:15]) == FIXED_TILES and cmds[8][2] == 0x201801ff,
             'unsupported single-material emit_mesh state')
    _require(cmds[-2][1] == 0xda, 'unsupported mesh without final host restore')
    _matrix(cmds[-2])
    prefix = raw[:cmds[15][0]]
    restore = raw[cmds[-2][0]:cmds[-1][0]]
    matrix, vertex_path = None, None
    cache, triangles = {}, []
    stats = Counter(commands=len(cmds), matrix_commands=1)
    had_triangle = False
    for command in cmds[15:-2]:
        _, op, a, b, hashed = command
        if op == 0xda:
            matrix = _matrix(command)
            stats['matrix_commands'] += 1
        elif op == 0x32:
            _require(matrix is not None, 'vertex load has no matrix owner')
            _require(hashed in index, 'vertex hash is not a literal indexed resource')
            target = index[hashed]
            _require(vertex_path in (None, target), 'unsupported multiple vertex arrays')
            vertex_path = target
            vraw = records[target]
            _require(len(vraw) >= 72 and vraw[0] == 0 and vraw[4:8] == b'RRAO' and
                     struct.unpack_from('<I', vraw, 64)[0] == 25,
                     'unsupported vertex resource')
            count = struct.unpack_from('<I', vraw, 68)[0]
            _require(len(vraw) == 72+16*count, 'invalid vertex resource length')
            n, end = a >> 12 & 255, a >> 1 & 127
            _require(0 < n <= 32 and 0 <= end-n < end <= 32, 'vertex cache bounds exceed 32 records')
            _require(b <= 0xfffff and b % 16 == 0 and b+16*n <= 16*count,
                     'vertex offset is not an aligned in-bounds archive offset')
            for i in range(n):
                cache[end-n+i] = (matrix, vraw[72+b+16*i:72+b+16*(i+1)])
            if had_triangle:
                stats['batches'] += 1
            had_triangle = False
            stats['vertex_commands'] += 1
            stats['loaded_vertices'] += n
        elif op in (5, 6):
            for word in [a & 0xffffff] + ([b] if op == 6 else []):
                fields = [(word >> shift) & 255 for shift in (16, 8, 0)]
                _require(all(f % 2 == 0 and f//2 < 32 and f//2 in cache for f in fields),
                         'triangle references an unloaded cache vertex')
                triangles.append(tuple(cache[f//2] for f in fields))
            had_triangle = True
            stats['triangle_commands'] += 1
        else:
            raise ValueError(f'unsupported command 0x{op:02x} within geometry')
    if had_triangle:
        stats['batches'] += 1
    _require(vertex_path is not None and triangles, 'unsupported empty mesh')
    stats['triangles'] = len(triangles)
    return Mesh(prefix, restore, vertex_path, triangles, dict(stats))


def _triangle_digest(triangles):
    result = hashlib.sha256()
    for triangle in triangles:
        for matrix, vertex in triangle:
            result.update(struct.pack('<I', matrix))
            result.update(vertex)
    return result.hexdigest()


def prove_equivalence(before, after, path):
    """Fail closed if any emitted triangle attribute/state/restore differs."""
    old, new = _decode(before, path), _decode(after, path)
    _require(old.prefix == new.prefix, 'material/preamble commands changed')
    _require(old.restore == new.restore, 'final host matrix restore changed')
    _require(old.triangles == new.triangles, 'ordered triangle attributes or matrix ownership changed')
    return dict(triangles=len(old.triangles), ordered_triangle_stream_sha256=_triangle_digest(old.triangles),
                ordered_triangles_identical=True, material_preamble_identical=True, host_restore_identical=True,
                cache_limit=32, vertex_offsets_valid=True)


def _compile(mesh, array_header):
    d, vertices = o2r.DLBuilder(), []
    current, batch, used = None, [], set()

    def flush():
        nonlocal current
        unique = sorted(used)
        lookup = {record: i for i, record in enumerate(unique)}
        for matrix in sorted({record[0] for record in unique}):
            subset = [(i, record[1]) for i, record in enumerate(unique) if record[0] == matrix]
            if matrix != current:
                d.raw(0xda380003, matrix)
                current = matrix
            offset = len(vertices)*16
            _require(offset <= 0xfffff, 'rebatched vertex offset exceeds inline-pointer threshold')
            d.vtx_hash(len(subset), subset[0][0], offset, mesh.vertex_path)
            vertices.extend(vertex for _, vertex in subset)
        indices = [[lookup[record] for record in triangle] for triangle in batch]
        for i in range(0, len(indices), 2):
            a, b, c = (x*2 for x in indices[i])
            first = a << 16 | b << 8 | c
            if i+1 < len(indices):
                a, b, c = (x*2 for x in indices[i+1])
                d.raw(0x06000000 | first, a << 16 | b << 8 | c)
            else:
                d.raw(0x05000000 | first, 0)

    for triangle in mesh.triangles:
        more = set(triangle)
        if len(used | more) > 32:
            flush()
            batch, used = [], set()
        batch.append(triangle)
        used |= more
    flush()
    a, b = struct.unpack('<II', mesh.restore)
    d.raw(a, b)
    d.end()
    return (mesh.prefix+d.to_resource()[72:],
            array_header+struct.pack('<II', 25, len(vertices))+b''.join(vertices))


def rebatch_records(records, namespaces=None):
    """Return a new resource map and report; input bytes are never mutated."""
    namespaces = DEFAULT_NAMESPACES if namespaces is None else tuple(
        name.removeprefix('alt/').rstrip('/')+'/' for name in namespaces)
    indexed = _index(records)
    readers = defaultdict(set)
    for name, raw in records.items():
        # Both serialized FourCC forms must reach the declared-endian fallback.
        if raw[4:8] not in (b'TLDO', b'ODLT'):
            continue
        try:
            for _, op, _, _, hashed in _commands(raw):
                if op == 0x32 and hashed in indexed:
                    readers[indexed[hashed]].add(name)
        except ValueError:
            # Unknown microcode or malformed command boundaries cannot hide a
            # second array owner. A conservative aligned scan may refuse an
            # optimization, but must never rewrite an unsupported reader's data.
            endian = '>' if raw and raw[0] == 1 else '<'
            for at in range(72, len(raw)-15, 8):
                if struct.unpack_from(endian+'I', raw, at)[0] >> 24 == 0x32:
                    hi, lo = struct.unpack_from(endian+'II', raw, at+8)
                    hashed = hi << 32 | lo
                    if hashed in indexed:
                        readers[indexed[hashed]].add(name)
    output, converted, skipped = dict(records), [], []
    for path, raw in sorted(records.items()):
        if (not path.startswith('alt/') or not path[4:].startswith(namespaces) or
                raw[4:8] not in (b'TLDO', b'ODLT')):
            continue
        try:
            mesh = _decode(records, path, indexed)
            _require(readers[mesh.vertex_path] == {path}, 'shared vertex array is unsupported')
            new_dl, new_vertices = _compile(mesh, records[mesh.vertex_path][:64])
            trial = dict(output)
            trial[path], trial[mesh.vertex_path] = new_dl, new_vertices
            new = _decode(trial, path, indexed)
            _require(new.stats['loaded_vertices'] <= mesh.stats['loaded_vertices'] and
                     new.stats['commands'] < mesh.stats['commands'], 'no measured command/vertex improvement')
            proof = prove_equivalence(records, trial, path)
            output = trial
            converted.append(dict(resource=path, changed_resources=[path, mesh.vertex_path],
                                  before=mesh.stats, after=new.stats, proof=proof))
        except ValueError as error:
            skipped.append(dict(resource=path, reason=str(error)))
    changed = {name for name in records if output[name] != records[name]}
    allowed = {name for entry in converted for name in entry['changed_resources']}
    _require(changed <= allowed and set(output) == set(records), 'unexpected resource changes')
    totals = {}
    for which in ('before', 'after'):
        summary = Counter()
        for entry in converted:
            summary.update(entry[which])
        totals[which] = dict(summary)
    return output, dict(converted=converted, skipped=skipped, converted_resource_count=len(converted),
                        resource_count=len(records), changed_resources=sorted(changed), totals=totals,
                        all_untouched_resources_byte_identical=True, runtime_tested=False,
                        status='STATIC_EQUIVALENCE_VERIFIED_RUNTIME_UNTESTED')


def convert_archive(source, output, namespaces=None):
    source, output = Path(source), Path(output)
    _require(source.resolve() != output.resolve() and
             not (output.exists() and output.samefile(source)), 'output must be separate from source')
    source_bytes = source.read_bytes()
    with zipfile.ZipFile(source) as archive:
        _require(len(archive.namelist()) == len(set(archive.namelist())), 'duplicate archive resource names')
        records = {name: archive.read(name) for name in archive.namelist()}
    result, report = rebatch_records(records, namespaces)
    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
        for name, raw in sorted(result.items()):
            entry = zipfile.ZipInfo(name, (2026, 10, 20, 0, 0, 0))
            entry.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(entry, raw)
    with zipfile.ZipFile(output) as archive:
        _require({name: archive.read(name) for name in archive.namelist()} == result,
                 'written archive differs from verified resources')
    _require(source.read_bytes() == source_bytes, 'source archive changed')
    report.update(source=str(source), source_sha256=hashlib.sha256(source_bytes).hexdigest(), output=str(output),
                  output_sha256=hashlib.sha256(output.read_bytes()).hexdigest(),
                  source_preserved=True, written_archive_verified=True)
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--report', required=True, type=Path)
    parser.add_argument('--namespace', action='append', help='optional private namespace filter; repeatable')
    args = parser.parse_args()
    _require(args.report.resolve() not in (args.source.resolve(), args.output.resolve()) and
             not any(args.report.exists() and archive.exists() and args.report.samefile(archive)
                     for archive in (args.source, args.output)),
             'report must be separate from source and output archives')
    report = convert_archive(args.source, args.output, args.namespace)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps({key: report[key] for key in ('converted_resource_count', 'totals', 'status')}))


if __name__ == '__main__':
    main()
