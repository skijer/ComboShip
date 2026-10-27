"""Read back exact replacement archives; this does not measure game runtime."""
import argparse
import collections
import hashlib
import json
from pathlib import Path
import struct
import sys
import zipfile


EXPANDED = {0x20, 0x31, 0x32, 0x33, 0x35, 0x36, 0x42}


def sha256(path):
    digest = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024*1024), b''):
            digest.update(chunk)
    return digest.hexdigest()


def inspect(path, native_hashes, crc64):
    with zipfile.ZipFile(path) as archive:
        names = archive.namelist()
        assert len(names) == len(set(names)), ('duplicate resource', str(path))
        assert all(not name.startswith('/') and '..' not in Path(name).parts for name in names)
        assert archive.testzip() is None, ('ZIP CRC failure', str(path))
        indexed = {crc64(name): name for name in names}
        assert len(indexed) == len(names), ('literal hash collision', str(path))
        references = collections.Counter()
        missing = collections.Counter()
        dl_count = 0
        for name in names:
            with archive.open(name) as stream:
                header = stream.read(72)
                if len(header) < 72 or header[0] not in (0, 1):
                    continue
                endian = '<' if header[0] == 0 else '>'
                if struct.unpack_from(endian+'I', header, 4)[0] != 0x4f444c54:
                    continue
                raw = header+stream.read()
            dl_count += 1
            at = 72
            ended = False
            while at+8 <= len(raw):
                first = struct.unpack_from(endian+'I', raw, at)[0]
                opcode = first >> 24
                size = 16 if opcode in EXPANDED else 8
                assert at+size <= len(raw), ('truncated command', name, at)
                if size == 16 and opcode != 0x33:
                    hi, lo = struct.unpack_from(endian+'II', raw, at+8)
                    hashed = hi << 32 | lo
                    references[f'{opcode:02x}'] += 1
                    if hashed not in indexed and hashed not in native_hashes:
                        missing[f'{hashed:016x}'] += 1
                at += size
                if opcode == 0xdf:
                    ended = True
                    break
            assert ended, ('missing end display list', name)
        assert not missing, (str(path), dict(missing))
        return dict(path=str(path), sha256=sha256(path), bytes=Path(path).stat().st_size,
                    resources=len(names), literal_hash_index=indexed,
                    display_lists=dl_count, hash_references_by_opcode=dict(references),
                    unresolved_references=dict(missing), zip_crc_valid=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', required=True, type=Path)
    parser.add_argument('--bundle', required=True, type=Path)
    parser.add_argument('--native', required=True, type=Path)
    parser.add_argument('--retained-adult-fado', required=True, type=Path)
    parser.add_argument('--report', required=True, type=Path)
    args = parser.parse_args()
    sys.path.insert(0, str(args.repo/'scripts/npc_models'))
    from native_assets.o2r import crc64
    from native_assets.path_guard import require_separate_destination
    common = sorted((args.bundle/'Common').glob('*.o2r'))
    styles = sorted((args.bundle/'Choose-One-Saria').glob('*/*.o2r'))
    assert len(common) == 4 and len(styles) == 2, ('expected four replacements and two styles', common, styles)
    paths = common+styles+[args.retained_adult_fado]
    require_separate_destination(args.report, args.native, *paths)
    with zipfile.ZipFile(args.native) as archive:
        native_hashes = {crc64(name): name for name in archive.namelist()}
    audits = {path: inspect(path, native_hashes, crc64) for path in paths}
    combinations = []
    for style in styles:
        owners = {}
        collisions = []
        for path in common+[args.retained_adult_fado, style]:
            for hashed, resource in audits[path]['literal_hash_index'].items():
                if hashed in owners:
                    collisions.append(dict(resource=resource, previous=owners[hashed], current=str(path)))
                owners[hashed] = str(path)
        assert not collisions, collisions
        combinations.append(dict(saria_style=style.parent.name, cross_pack_collisions=collisions,
                                 indexed_resources=len(owners), unresolved_hashes=0))
    for audit in audits.values():
        audit.pop('literal_hash_index')
    report = dict(status='ARCHIVE_READBACK_VERIFIED_RUNTIME_UNTESTED',
                  source_commit='ee83e2613ebcd0cb029869fc6dce90f7a09fdd75',
                  native_sha256=sha256(args.native), archives=list(audits.values()),
                  active_combinations=combinations, runtime_tested=False, user_accepted=False)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(dict(status=report['status'], archives=len(audits),
                          combinations=len(combinations), unresolved_references=0, cross_pack_collisions=0)))


if __name__ == '__main__':
    main()
