#!/usr/bin/env python3
"""Inspect Zelda placement in a full POC4 addon without trusting its manifest.

Pass the accepted POC3 as --candidate with the normal-setup test selected to
demonstrate RED. Full verification also rebuilds the candidate twice.
"""
import argparse
import binascii
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import struct
import sys
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/temple_sages'))
import resource_geometry as geo

TEMPLE = 'scenes/shared/tokinoma_scene/'
MAIN = TEMPLE + 'tokinoma_room_0'
ADULT = TEMPLE + 'tokinoma_room_0ZeldaAdult'
SCENE = TEMPLE + 'tokinoma_scene'
COLLISION = TEMPLE + 'tokinoma_sceneCollisionHeader_0032F8'
MANIFEST = 'custom/temple_sages/manifest.json'
PARENT_SHA256 = '2ae8cdd5ea919e299eefcf0a9730b97d6badd15d20b01692dcc4df4556e54e42'
BASE_SHA256 = 'ea80d61b223ee38075fe5383b652998f3b08164e903134922f63f205d995717b'
EXPECTED = ((42, 0, -110, -2400, 0, 0, 0, 0x7D23),
            (42, 0, -110, -2400, 0, 0, 0, 0x7D23),
            (42, 0, -110, -2400, 0, 0, 0, 0x7F34),
            (42, 0, -110, -2400, 0, 0, 0, 0x7F34))
ARGS = None


def read_pack(path):
    with zipfile.ZipFile(path) as pack:
        names = pack.namelist()
        if len(set(names)) != len(names):
            raise AssertionError('duplicate archive member')
        if pack.testzip() is not None:
            raise AssertionError('archive CRC mismatch')
        return {name: pack.read(name) for name in names}


def alternates(data):
    # Both actual scene and room roots start with the alternate dispatcher.
    if struct.unpack_from('<I', data, 68)[0] != 0x18:
        return []
    count = struct.unpack_from('<I', data, 72)[0]
    offset, paths = 76, []
    for _ in range(count):
        size = struct.unpack_from('<I', data, offset)[0]
        offset += 4
        paths.append(data[offset:offset+size].decode())
        offset += size
    return paths


def selected(resources, root, setup, prefix=''):
    seen = set()
    while root not in seen:
        seen.add(root)
        paths = alternates(resources[prefix+root])
        if setup == 0 or not paths:
            return root
        path = paths[setup-1]
        if not path and setup == 3:
            path = paths[1]  # Actual OTR adult-night -> adult-day fallback.
        if not path:
            return root
        root = path
    raise AssertionError('recursive alternate-header dispatcher')


def actor_bytes(data):
    matches = [info['actors'] for command, _, _, info in geo.room_commands(data) if command == 1]
    if len(matches) != 1:
        raise AssertionError('expected exactly one room actor list')
    return matches[0]


def actors(data):
    values = actor_bytes(data)
    return [struct.unpack_from('<HhhhhhhH', values, offset) for offset in range(0, len(values), 16)]


def support_hits(vertices, polygons, x, z):
    hits = []
    for index, polygon in enumerate(polygons):
        if polygon[5] < 23170 or polygon[1] & 0x4000:
            continue
        a, b, c = [vertices[i & 0x1fff] for i in polygon[1:4]]
        denominator = (b[2]-c[2])*(a[0]-c[0])+(c[0]-b[0])*(a[2]-c[2])
        if not denominator:
            continue
        u = ((b[2]-c[2])*(x-c[0])+(c[0]-b[0])*(z-c[2])) / denominator
        v = ((c[2]-a[2])*(x-c[0])+(a[0]-c[0])*(z-c[2])) / denominator
        if min(u, v, 1-u-v) >= -1e-7:
            hits.append((index, u*a[1]+v*b[1]+(1-u-v)*c[1]))
    return hits


class ZeldaPlacements(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parent = read_pack(ARGS.input)
        cls.candidate = read_pack(ARGS.candidate)
        with zipfile.ZipFile(ARGS.base) as base:
            cls.native_names = set(base.namelist())
            cls.scene = base.read(SCENE)

    def test_normal_setups_select_exactly_one_correct_age_zelda(self):
        for prefix in ('', 'alt/'):
            for setup, expected in enumerate(EXPECTED):
                with self.subTest(prefix=prefix, setup=setup):
                    header = selected(self.candidate, MAIN, setup, prefix)
                    entries = actors(self.candidate[prefix+header])
                    self.assertEqual([entry for entry in entries if entry[0] == 42], [expected],
                                     'normal setup must contain one correctly aged cutscene Zelda')
                    self.assertEqual(header, MAIN if setup < 2 else ADULT)

    def test_actual_native_scene_and_fallback_graph_are_preserved(self):
        self.assertEqual(alternates(self.scene)[:3], ['', '', ''])
        scene_resources = {SCENE: self.scene}
        self.assertEqual([selected(scene_resources, SCENE, i) for i in range(4)], [SCENE]*4)
        self.assertNotIn(SCENE, self.candidate)
        self.assertNotIn('alt/'+SCENE, self.candidate)
        for prefix in ('', 'alt/'):
            before = alternates(self.parent[prefix+MAIN])
            after = alternates(self.candidate[prefix+MAIN])
            self.assertEqual(before[:3], ['', '', ''])
            self.assertEqual(after[:3], ['', ADULT, ''])
            self.assertEqual(after[3:], before[3:])
            self.assertEqual(alternates(self.candidate[prefix+ADULT]), [])

    def test_center_is_on_topmost_accepted_floor_and_facing_light(self):
        for prefix in ('', 'alt/'):
            data = self.candidate[prefix+COLLISION]
            self.assertEqual(data, self.parent[prefix+COLLISION])
            vertices, polygons, _, _ = geo.collision_parts(data)
            hits = support_hits(vertices, polygons, 0, -2400)
            self.assertEqual(hits, [(678, -120.0), (679, -120.0), (753, -114.0), (788, -110.0)])
            for radius in (0, 18):
                for step in range(64):
                    angle = step * math.tau / 64
                    local = support_hits(vertices, polygons, radius*math.cos(angle), -2400+radius*math.sin(angle))
                    self.assertTrue(local)
                    self.assertAlmostEqual(max(height for _, height in local), -110)
            for setup in range(4):
                header = selected(self.candidate, MAIN, setup, prefix)
                zelda = next(entry for entry in actors(self.candidate[prefix+header]) if entry[0] == 42)
                self.assertEqual(zelda[1:7], (0, -110, -2400, 0, 0, 0))
                self.assertEqual((math.sin(zelda[5]*math.tau/65536), math.cos(zelda[5]*math.tau/65536)), (0, 1))

    def test_existing_actor_bytes_and_non_dispatch_commands_unchanged(self):
        for prefix in ('', 'alt/'):
            source = self.parent[prefix+MAIN]
            original = actor_bytes(source)
            source_commands = [(command, source[start:end]) for command, start, end, _ in geo.room_commands(source)]
            for path, parameter in ((MAIN, 0x7D23), (ADULT, 0x7F34)):
                candidate = self.candidate[prefix+path]
                self.assertEqual(actor_bytes(candidate), original + struct.pack('<HhhhhhhH', 42, 0, -110, -2400, 0, 0, 0, parameter))
                actual = [(command, candidate[start:end]) for command, start, end, _ in geo.room_commands(candidate)]
                self.assertEqual([(c, raw) for c, raw in actual if c not in (1, 0x18)],
                                 [(c, raw) for c, raw in source_commands if c not in (1, 0x18)])
                self.assertEqual(candidate[:64], source[:64])

    def test_all_cutscene_headers_and_unrelated_members_unchanged(self):
        added = {ADULT, 'alt/'+ADULT}
        self.assertEqual(set(self.candidate)-set(self.parent), added)
        self.assertEqual(set(self.parent)-set(self.candidate), set())
        self.assertEqual(len(self.candidate), 1988)
        changed = {name for name in self.parent if self.parent[name] != self.candidate[name]}
        self.assertEqual(changed, {MAIN, 'alt/'+MAIN, MANIFEST})
        for prefix in ('', 'alt/'):
            for path in alternates(self.parent[prefix+MAIN])[3:]:
                self.assertEqual(self.candidate[prefix+path], self.parent[prefix+path])
                self.assertFalse(any(entry[0] == 42 for entry in actors(self.candidate[prefix+path])))
        protected = [name for name in self.parent if '/tokinoma_room_1' in name or '/kenjyanoma_scene/' in name]
        self.assertEqual({name: self.candidate[name] for name in protected},
                         {name: self.parent[name] for name in protected})
        self.assertNotIn(TEMPLE+'tokinoma_room_1', self.candidate)
        self.assertNotIn('scenes/shared/kenjyanoma_scene/kenjyanoma_scene', self.candidate)

    def test_canonical_alt_parity_and_native_asset_dependencies(self):
        for path in (MAIN, ADULT, COLLISION):
            self.assertEqual(self.candidate[path], self.candidate['alt/'+path])
        self.assertFalse(any(name.startswith('objects/') for name in set(self.candidate)-set(self.parent)))
        self.assertTrue(any(name.startswith('objects/object_zl1/') for name in self.native_names))
        self.assertTrue(any(name.startswith('objects/object_zl2/') for name in self.native_names))

    def test_manifest_preserves_prior_fields_and_records_real_placements(self):
        before = json.loads(self.parent[MANIFEST])
        after = json.loads(self.candidate[MANIFEST])
        self.assertEqual({k: v for k, v in after.items() if k in before and k != 'resources'},
                         {k: v for k, v in before.items() if k != 'resources'})
        self.assertEqual(after['resources'], {name: hashlib.sha256(data).hexdigest() for name, data in self.candidate.items() if name != MANIFEST})
        placement = after['zelda_placement']
        self.assertEqual(placement['parent_sha256'], PARENT_SHA256)
        self.assertEqual(placement['base_sha256'], BASE_SHA256)
        self.assertEqual(placement['position'], [0, -110, -2400])
        self.assertEqual(placement['yaw'], 0)
        self.assertEqual(placement['floor_support_polygons'], [788])
        self.assertEqual([item['params'] for item in placement['normal_setups']], [0x7D23, 0x7D23, 0x7F34, 0x7F34])
        for setup, item in enumerate(placement['normal_setups']):
            self.assertEqual(item['setup'], setup)
            self.assertEqual(item['scene_header'], SCENE)
            self.assertEqual(item['room_header'], selected(self.candidate, MAIN, setup))
        self.assertFalse(placement['runtime_verified'])

    def test_rebuild_is_deterministic_with_valid_crc_hashes_and_inputs_unchanged(self):
        spec = importlib.util.spec_from_file_location('place_zelda', ROOT / 'tools/temple_sages/place_zelda.py')
        builder = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(builder)
        with tempfile.TemporaryDirectory(prefix='zelda-placement-') as directory:
            first, second = (Path(directory)/name for name in ('first.o2r', 'second.o2r'))
            builder.build(ARGS.input, ARGS.base, first)
            builder.build(ARGS.input, ARGS.base, second)
            self.assertEqual(first.read_bytes(), second.read_bytes())
            self.assertEqual(first.read_bytes(), ARGS.candidate.read_bytes())
            self.assertEqual(read_pack(first), self.candidate)
            with zipfile.ZipFile(first) as pack:
                self.assertEqual(pack.namelist(), sorted(self.candidate))
                for info in pack.infolist():
                    self.assertEqual(info.date_time, (2026, 10, 27, 0, 0, 0))
                    self.assertEqual(info.CRC, binascii.crc32(self.candidate[info.filename]) & 0xffffffff)
            forged = Path(directory)/'forged.o2r'
            forged.write_bytes(ARGS.input.read_bytes()+b'changed')
            with self.assertRaisesRegex(ValueError, 'accepted POC3'):
                builder.build(forged, ARGS.base, Path(directory)/'rejected.o2r')
            with self.assertRaisesRegex(ValueError, 'input'):
                builder.build(ARGS.input, ARGS.base, ARGS.input)
        self.assertEqual(hashlib.sha256(ARGS.input.read_bytes()).hexdigest(), PARENT_SHA256)
        self.assertEqual(hashlib.sha256(ARGS.base.read_bytes()).hexdigest(), BASE_SHA256)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--base', type=Path, required=True)
    parser.add_argument('--candidate', type=Path, required=True)
    ARGS, remaining = parser.parse_known_args()
    unittest.main(argv=[sys.argv[0], *remaining])
