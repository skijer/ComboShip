"""Lossless archive recompilation checks; fixtures contain real O2R commands."""
import hashlib
import importlib
import pathlib
import struct
import subprocess
import sys
import tempfile
import unittest
import zipfile

from native_assets import o2r

try:
    rebatch = importlib.import_module('rebatch_native_npc_archives')
except ModuleNotFoundError:
    rebatch = None


NS = 'alt/objects/object_kw1/NativeNpcV1'
MESH = NS + '/Skin/Limbs/Limb01'
VERTICES = MESH + '/V'
PREFIX = [(0xE7000000, 0), (0xE3001001, 0), (0xD7000002, 0xFFFFFFFF),
          (0xFC127E03, 0xFFFFFBFD), (0xD9F1F9FF, 0),
          (0xD9FFFFFF, 0x00830005), (0xFA000000, 0xFFFFFFFF)]
TILES = [(0xF5180000, 0x07080200), (0xE6000000, 0), (0xF4000000, 0x077FC7FC),
         (0xE7000000, 0), (0xF5190000, 0x00080200), (0xF2000000, 0x007FC7FC)]


def fixture(count=6):
    records = []
    for i in range(count):
        slot = i % 3
        # Signed positions/UVs/normals and flag bytes all participate in identity.
        records.append((slot, (-301+i*7, 62-i, 19, slot << 8, -200+i, 151,
                               128+i % 128, 255-i % 256, i % 128, 255)))
    if count > 6:
        # Same complete vertex bytes under two current matrices must stay distinct.
        records[1] = (1, records[0][1])
    triangles = [(0, 1, 2), (3, 1, 4), (5, 1, 4)] if count == 6 else [
        tuple(range(i, i+3)) for i in range(0, count-2, 3)]
    d = o2r.DLBuilder()
    d.marker(MESH)
    for a, b in PREFIX:
        d.raw(a, b)
    d.settimg_hash(0, 3, 512, 'objects/object_kw1/NativeNpcV1/Textures/Diffuse')
    for a, b in TILES:
        d.raw(a, b)
    vertices = []
    # Deliberately flush each triangle to reproduce matrix-group fragmentation.
    for tri in triangles:
        ordered = sorted({records[i] for i in tri})
        lookup = {rec: i for i, rec in enumerate(ordered)}
        for slot in sorted({r[0] for r in ordered}):
            d.raw(0xDA380003, 0x0D000001 + slot*64)
            subset = [(i, rec) for i, rec in enumerate(ordered) if rec[0] == slot]
            d.vtx_hash(len(subset), subset[0][0], len(vertices)*16, VERTICES)
            vertices.extend(rec[1] for _, rec in subset)
        d.tri1(*(lookup[records[i]] for i in tri))
    d.raw(0xDA380003, 0x0D000041)
    d.end()
    return {MESH: d.to_resource(), VERTICES: o2r.build_vertex_array(vertices),
            NS+'/Skin/Palette': b'untouched palette',
            'objects/object_kw1/NativeNpcV1/Textures/Diffuse': b'untouched texture',
            'ModelInfo.json': b'{"runtime_tested": false}'}, triangles, records


def oracle(records, path=MESH):
    """Independent fixture interpreter, retaining ordered state/triangle events."""
    hashes = {o2r.crc64(n): n for n in records}
    raw = records[path]
    at, matrix = 72, None
    cache, events, counts = {}, [], dict(vertices=0, loads=0, matrices=0)
    while at < len(raw):
        a, b = struct.unpack_from('<II', raw, at)
        op = a >> 24
        size = 16 if op in (0x20, 0x31, 0x32, 0x33, 0x35, 0x36, 0x42) else 8
        if op == 0xDA:
            matrix = b
            counts['matrices'] += 1
        elif op == 0x32:
            hi, lo = struct.unpack_from('<II', raw, at+8)
            vraw = records[hashes[hi << 32 | lo]]
            n, end = a >> 12 & 255, a >> 1 & 127
            assert 0 < n <= 32 and 0 <= end-n < end <= 32
            assert b % 16 == 0 and 72+b+n*16 <= len(vraw)
            for i in range(n):
                cache[end-n+i] = (matrix, vraw[72+b+16*i:72+b+16*(i+1)])
            counts['vertices'] += n
            counts['loads'] += 1
        elif op in (5, 6):
            for word in [a & 0xffffff] + ([b] if op == 6 else []):
                indices = tuple((word >> s & 255)//2 for s in (16, 8, 0))
                events.append(('triangle', tuple(cache[i] for i in indices)))
        else:
            events.append(('state', raw[at:at+size]))
        at += size
    return events, matrix, counts


class NativeNpcRebatchTests(unittest.TestCase):
    def setUp(self):
        self.assertIsNotNone(rebatch, 'lossless NPC archive converter is required')

    def test_merges_fragmented_groups_without_changing_ordered_attributes(self):
        source, _, _ = fixture()
        output, report = rebatch.rebatch_records(source)
        before, restore, cost = oracle(source)
        after, new_restore, new_cost = oracle(output)
        self.assertEqual(before, after)
        self.assertEqual(restore, new_restore)
        self.assertLess(new_cost['loads'], cost['loads'])
        self.assertLess(new_cost['matrices'], cost['matrices'])
        self.assertEqual(new_cost['vertices'], 6)
        self.assertEqual(set(output), set(source))
        self.assertEqual(set(report['converted'][0]['changed_resources']), {MESH, VERTICES})
        for name in set(source)-{MESH, VERTICES}:
            self.assertEqual(source[name], output[name])
        self.assertEqual(source, fixture()[0], 'converter mutated input map')

    def test_cache_boundary_and_same_position_with_different_matrix_remain_exact(self):
        source, _, _ = fixture(39)
        output, _ = rebatch.rebatch_records(source)
        self.assertEqual(oracle(source)[:2], oracle(output)[:2])
        # The independent interpreter validates every cache index and source bound.
        self.assertGreater(oracle(output)[2]['loads'], 3)

    def test_skip_midstream_material_change_without_modifying_input(self):
        source, _, _ = fixture()
        raw = source[MESH]
        source[MESH] = raw[:-16] + struct.pack('<II', 0xFA000000, 0xff00ffff) + raw[-16:]
        output, report = rebatch.rebatch_records(source)
        self.assertEqual(output, source)
        self.assertEqual(report['converted'], [])
        self.assertIn('unsupported', report['skipped'][0]['reason'])

    def test_shared_vertex_array_is_skipped_explicitly(self):
        source, _, _ = fixture()
        source[NS+'/OtherSurface'] = source[MESH]
        output, report = rebatch.rebatch_records(source)
        self.assertEqual(output, source)
        self.assertTrue(any('shared' in x['reason'] for x in report['skipped']))

    def test_unsupported_reader_cannot_hide_a_shared_vertex_array(self):
        source, _, _ = fixture()
        raw = bytearray(source[MESH])
        raw[64] = 0  # This sibling cannot itself be compiled as F3DEX2.
        source[NS+'/UnsupportedReader'] = bytes(raw)
        output, report = rebatch.rebatch_records(source)
        self.assertEqual(output, source)
        self.assertTrue(any('shared' in x['reason'] for x in report['skipped']))

    def test_big_endian_reader_cannot_hide_a_shared_vertex_array(self):
        source, _, _ = fixture()
        little = source[MESH]
        big = bytearray(little)
        big[0] = 1
        struct.pack_into('>I', big, 4, o2r.T_ODLT)
        # Preserve every command word's value in the sibling's declared order.
        for at in range(72, len(big), 4):
            struct.pack_into('>I', big, at, struct.unpack_from('<I', little, at)[0])
        source[NS+'/BigEndianReader'] = bytes(big)
        self.assertEqual(struct.unpack_from('<I', source[VERTICES], 68)[0], 9)
        output, report = rebatch.rebatch_records(source)
        self.assertEqual(output[VERTICES], source[VERTICES])
        self.assertEqual(output, source)
        self.assertEqual(report['converted'], [])
        self.assertTrue(any('shared' in x['reason'] for x in report['skipped']))

    def test_unindexed_hash_and_bad_load_bounds_are_skipped(self):
        source, _, _ = fixture()
        original = source[MESH]
        at = next(at for at in range(72, len(original)-15, 8)
                  if struct.unpack_from('<I', original, at)[0] >> 24 == 0x32)
        cases = [(at+8, struct.pack('<II', 0x12345678, 0xabcdef01), 'indexed'),
                 (at+4, struct.pack('<I', 0x100000), 'offset'),
                 (at, struct.pack('<I', 0x32001000), 'cache')]
        for offset, replacement, expected in cases:
            with self.subTest(expected=expected):
                data = bytearray(original)
                data[offset:offset+len(replacement)] = replacement
                source[MESH] = bytes(data)
                output, report = rebatch.rebatch_records(source)
                self.assertEqual(output, source)
                self.assertTrue(any(expected in x['reason'] for x in report['skipped']), report)

    def test_proof_rejects_changed_vertex_normal_and_host_restore(self):
        source, _, _ = fixture()
        output, _ = rebatch.rebatch_records(source)
        rebatch.prove_equivalence(source, output, MESH)
        changed = dict(output)
        raw = bytearray(changed[VERTICES])
        raw[84] ^= 1
        changed[VERTICES] = bytes(raw)
        with self.assertRaisesRegex(ValueError, 'triangle'):
            rebatch.prove_equivalence(source, changed, MESH)
        changed = dict(output)
        raw = bytearray(changed[MESH])
        struct.pack_into('<I', raw, len(raw)-12, 0x0d000081)
        changed[MESH] = bytes(raw)
        with self.assertRaisesRegex(ValueError, 'restore'):
            rebatch.prove_equivalence(source, changed, MESH)

    def test_archive_output_is_deterministic_and_preserves_source(self):
        source, _, _ = fixture()
        with tempfile.TemporaryDirectory() as folder:
            folder = pathlib.Path(folder)
            parent = folder/'source.o2r'
            with zipfile.ZipFile(parent, 'w') as archive:
                for name, raw in source.items():
                    archive.writestr(name, raw)
            digest = hashlib.sha256(parent.read_bytes()).hexdigest()
            for name in ('candidate1.o2r', 'candidate2.o2r'):
                report = rebatch.convert_archive(parent, folder/name)
                self.assertEqual(report['source_sha256'], digest)
                self.assertTrue(report['all_untouched_resources_byte_identical'])
            self.assertEqual((folder/'candidate1.o2r').read_bytes(), (folder/'candidate2.o2r').read_bytes())
            self.assertEqual(hashlib.sha256(parent.read_bytes()).hexdigest(), digest)
            with self.assertRaisesRegex(ValueError, 'source'):
                rebatch.convert_archive(parent, parent)

    def _assert_report_hardlink_rejected(self, target):
        records, _, _ = fixture()
        with tempfile.TemporaryDirectory() as folder:
            folder = pathlib.Path(folder)
            source, output, report = (folder/name for name in ('source.o2r', 'output.o2r', 'report.json'))
            with zipfile.ZipFile(source, 'w') as archive:
                for name, raw in records.items():
                    archive.writestr(name, raw)
            if target == 'output':
                with zipfile.ZipFile(output, 'w') as archive:
                    archive.writestr('preserve-existing-output', b'original output archive')
            report.hardlink_to(source if target == 'source' else output)
            self.assertTrue(report.samefile(source if target == 'source' else output))
            originals = {path: path.read_bytes() for path in (source, output, report) if path.exists()}
            result = subprocess.run([sys.executable, str(pathlib.Path(rebatch.__file__).resolve()),
                                     '--source', str(source), '--output', str(output), '--report', str(report)],
                                    capture_output=True, text=True, check=False)
            self.assertNotEqual(result.returncode, 0, 'CLI accepted a destructive report hardlink')
            self.assertIn('report must be separate', result.stderr)
            for path, before in originals.items():
                self.assertEqual(hashlib.sha256(path.read_bytes()).digest(), hashlib.sha256(before).digest())
            if target == 'source':
                self.assertFalse(output.exists(), 'invalid report alias must fail before any archive write')

    def test_cli_refuses_report_hardlink_to_source_before_writing(self):
        self._assert_report_hardlink_rejected('source')

    def test_cli_refuses_report_hardlink_to_output_before_writing(self):
        self._assert_report_hardlink_rejected('output')


if __name__ == '__main__':
    unittest.main()
