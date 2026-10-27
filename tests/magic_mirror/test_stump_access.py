#!/usr/bin/env python3
"""Exercise the isolated log recipe against retained real native collision."""
import hashlib
import importlib.util
import math
from pathlib import Path
import struct
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def collision(data):
    offset = 76
    count, = struct.unpack_from('<I', data, offset)
    vertices = [struct.unpack_from('<3h', data, offset + 4 + i * 6) for i in range(count)]
    offset += 4 + count * 6
    count, = struct.unpack_from('<I', data, offset)
    polygons = [struct.unpack_from('<8h', data, offset + 4 + i * 16) for i in range(count)]
    offset += 4 + count * 16
    count, = struct.unpack_from('<I', data, offset)
    surfaces = [data[offset + 4 + i * 8:offset + 12 + i * 8] for i in range(count)]
    return vertices, polygons, surfaces, data[offset + 4 + count * 8:]


class StumpAccessTests(unittest.TestCase):
    def setUp(self):
        self.native = load('mirror_native_forest', ROOT / 'tools/black_market/native_forest.py')
        try:
            self.access = load('mirror_stump_access', ROOT / 'tools/black_market/mirror_stump_access.py')
        except FileNotFoundError:
            self.fail('The native stump has no fallen-log access recipe yet')
        self.source = (Path(__file__).parent / 'native_stump_collision.bin').read_bytes()
        self.assertEqual(hashlib.sha256(self.source).hexdigest(),
                         'd98991a03ff914e651f7f0e276fa86faa6060636716500e25e7f7627dbe807ad')
        self.paths = [prefix + '_collision' for prefix in self.native.SCENES.values()]
        self.resources = {path: self.source for path in self.paths}
        self.resources['unrelated-scene-header'] = b'original scene, actors and room commands'

    def test_appends_only_a_matching_wood_log_and_preserves_every_existing_collision_record(self):
        before = collision(self.source)
        report = self.access.augment(self.native, self.resources)
        self.assertEqual(self.resources['unrelated-scene-header'], b'original scene, actors and room commands')
        self.assertEqual(len(set(self.resources[path] for path in self.paths)), 1)
        for path in self.paths:
            data = self.resources[path]
            after = collision(data)
            self.assertEqual(data[:76], self.source[:76])
            self.assertEqual(after[0][:710], before[0])
            self.assertEqual(after[1][:958], before[1])
            self.assertEqual(after[2][:33], before[2])
            self.assertEqual(after[3], before[3])
            self.assertEqual(tuple(len(part) for part in after[:3]), (726, 986, 34))
            self.assertEqual(after[2][33], before[2][0])
            self.assertTrue(all(poly[0] == 33 for poly in after[1][958:]))
            points = set(after[0][710:])
            self.assertEqual((min(p[0] for p in points), max(p[0] for p in points)), (-345, -105))
            self.assertEqual((min(p[1] for p in points), max(p[1] for p in points)), (-28, 80))
            self.assertEqual((min(p[2] for p in points), max(p[2] for p in points)), (376, 456))
            for poly in after[1][958:]:
                normal = [n / 32767 for n in poly[4:7]]
                self.assertAlmostEqual(sum(n * n for n in normal), 1, delta=0.0002)
                for index in poly[1:4]:
                    self.assertLess(abs(sum(v * n for v, n in zip(after[0][index], normal)) + poly[7]), 1)
        mesh = self.native.vertex_positions(self.resources[report['vertex_resource']])
        self.assertEqual(set(mesh), set(collision(self.resources[self.paths[0]])[0][710:]))
        display = ET.fromstring(self.resources[report['root']])
        self.assertEqual(len(display.findall('Triangle1')), 28)
        for load_vertices in display.findall('LoadVertices'):
            self.assertLessEqual(int(load_vertices.get('Count')), 32)
        material = ET.fromstring(self.resources[report['material']])
        image = material.find('LoadTextureBlock')
        self.assertEqual(image.get('Path'), 'custom/comboship/native_forest/authored/textures/portal_bark')
        self.assertEqual((image.get('Format'), image.get('Size'), image.get('Width'), image.get('Height')),
                         ('0', '2', '32', '32'))

    def test_ramp_supports_a_broad_walk_to_cap_without_blocking_base_recall(self):
        self.access.augment(self.native, self.resources)
        scene = load('mirror_scene_geometry', ROOT / 'tests/magic_mirror/check_scene.py')
        original = scene.collision(self.source)
        polys = scene.collision(self.resources[self.paths[0]])
        for z in (388, 402, 416, 430, 444):
            last = None
            for step in range(25):
                x, y = -105 - step * 10, step * 10 / 3
                height = scene.floor_at(polys, x, z, 100)
                self.assertGreaterEqual(height, y - 0.01)
                self.assertLessEqual(height, y + 12)
                self.assertFalse(scene.blocked(polys, (x, y + 20, z), (x, y + 110, z)))
                if last:
                    for h in (18, 55, 100):
                        self.assertFalse(scene.blocked(polys, (last[0], last[1] + h, z), (x, y + h, z)))
                last = (x, y)
        vertices, polygons, _, _ = collision(self.resources[self.paths[0]])
        tops = [p for p in polygons[958:] if p[5] > 30000]
        self.assertEqual(len(tops), 2)
        for poly in tops:
            self.assertGreater(poly[5] / 32767, math.cos(math.radians(20)))
        for dx, dz in ((0, 0), (40, 0), (-40, 0), (0, 40), (0, -40), (28, 28), (-28, -28)):
            x, z = -377 + dx, 296 + dz
            self.assertEqual(scene.floor_at(polys, x, z, 5), scene.floor_at(original, x, z, 5))
            self.assertFalse(scene.blocked(polys, (x, 5, z), (x, 100, z)))
        # The full top endpoint lies on the unchanged cap, within one ordinary
        # step of the sloping log before walking onto the center of the stump.
        for z in (384, 416, 448):
            self.assertEqual(scene.floor_at(original, -345, z, 81), 80)

    def test_missing_collision_fails_without_partial_mutation(self):
        del self.resources[self.paths[-1]]
        before = dict(self.resources)
        with self.assertRaises(ValueError):
            self.access.augment(self.native, self.resources)
        self.assertEqual(self.resources, before)

    def test_rebaking_preserves_the_existing_log_without_duplicate_collision(self):
        self.access.augment(self.native, self.resources)
        before = dict(self.resources)
        report = self.access.augment(self.native, self.resources)
        self.assertTrue(report['already_present'])
        self.assertEqual(self.resources, before)
        self.resources[self.paths[-1]] = self.source
        before = dict(self.resources)
        with self.assertRaises(ValueError):
            self.access.augment(self.native, self.resources)
        self.assertEqual(self.resources, before)


if __name__ == '__main__':
    unittest.main()
