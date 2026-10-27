"""Probe native collision exported in the addon, independently of route metadata."""
import importlib.util
import math
import os
from pathlib import Path
import struct
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[2]
INPUTS = Path(os.environ.get('TEMPLE_SAGES_INPUTS', ROOT.parents[1] / 'temple_inputs'))
COLLISION = 'scenes/shared/tokinoma_scene/tokinoma_sceneCollisionHeader_0032F8'
CENTER = (0, -116, -2400)
FLOOR = -120
DIRECTIONS = ((-math.sqrt(3)/2, .5), (-math.sqrt(3)/2, -.5), (0, -1),
              (math.sqrt(3)/2, -.5), (math.sqrt(3)/2, .5), (0, 1))


def decode_collision(data):
    """Decode serialized collision directly; do not ask builder for its faces."""
    assert data[4:8] == b'LOCO'
    count = struct.unpack_from('<I', data, 76)[0]
    vertices = [struct.unpack_from('<3h', data, 80+i*6) for i in range(count)]
    offset = 80+count*6
    count = struct.unpack_from('<I', data, offset)[0]
    polygons = [struct.unpack_from('<4H4h', data, offset+4+i*16) for i in range(count)]
    return vertices, polygons


def subtract(a, b):
    return tuple(x-y for x, y in zip(a, b))


def dot(a, b):
    return sum(x*y for x, y in zip(a, b))


def cross(a, b):
    return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])


def wall_hits(vertices, polygons, start, end):
    """Native entity-facing wall intersection, including triangle edges.

    Native one-face line checks ignore travel in the normal's direction;
    entity-ignore is 4000. This test does not share builder clipping code.
    """
    hits = []
    direction = subtract(end, start)
    for index, poly in enumerate(polygons):
        if not -26214 <= poly[5] <= 16383 or poly[1] & 0x4000:
            continue
        if dot(poly[4:7], direction) >= 0:
            continue
        a, b, c = [vertices[i & 0x1fff] for i in poly[1:4]]
        edge1, edge2 = subtract(b, a), subtract(c, a)
        h = cross(direction, edge2)
        determinant = dot(edge1, h)
        if abs(determinant) < 1e-9:
            continue
        inverse = 1/determinant
        s = subtract(start, a)
        u = inverse*dot(s, h)
        q = cross(s, edge1)
        v = inverse*dot(direction, q)
        t = inverse*dot(edge2, q)
        if min(u, v, t) >= -1e-7 and u+v <= 1+1e-7 and t <= 1+1e-7:
            hits.append((index, poly))
    return hits


def route_point(direction, radius, tangent=0, height=26):
    dx, dz = direction
    return (dx*radius-dz*tangent, FLOOR+height, CENTER[2]+dz*radius+dx*tangent)


class CollisionRoutes(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        spec = importlib.util.spec_from_file_location('sages_collision_builder', ROOT/'tools/temple_sages/build.py')
        builder = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(builder)
        directory = tempfile.TemporaryDirectory(prefix='temple-sages-collision-')
        cls.addClassCleanup(directory.cleanup)
        candidate = Path(directory.name)/'candidate.o2r'
        builder.build(INPUTS/'oot.o2r', INPUTS/"Djipi's 3DE - 20 Scenes Map.otr",
                      INPUTS/'Stone Entrance Lost Woods OOT.glb', candidate)
        cls.geometry = builder.G
        with zipfile.ZipFile(candidate) as pack:
            cls.resources = {name: pack.read(name) for name in pack.namelist()}
            cls.collisions = {prefix: decode_collision(pack.read(prefix+COLLISION)) for prefix in ('', 'alt/')}
        with zipfile.ZipFile(INPUTS/'oot.o2r') as pack:
            cls.original_center = decode_collision(pack.read(
                'scenes/shared/kenjyanoma_scene/kenjyanoma_sceneCollisionHeader_00359C'))

    def test_rooted_bridge_triangles_trace_the_bound_arcane_texture(self):
        active = {n: b for n, b in self.resources.items() if not n.startswith('alt/')}
        active.update({n[4:]: b for n, b in self.resources.items() if n.startswith('alt/')})
        for theme in ('forest', 'fire', 'water', 'spirit', 'shadow', 'light'):
            records = self.geometry.collect(active, 'custom/temple_sages/bridge_'+theme)
            self.assertTrue(records, theme)
            self.assertEqual({rec['texture'] for rec in records}, {'custom/temple_sages/texture/arcane_stone'})

    def test_all_bridge_mouths_clear_in_both_directions_across_player_width(self):
        for prefix, (vertices, polygons) in self.collisions.items():
            for route, direction in enumerate(DIRECTIONS):
                # Probe lane centers and their 18-unit adult collision envelope.
                for tangent in ((-83, -65, 0, 65, 83) if route == 5 else (-58, -40, 0, 40, 58)):
                    for height in (26, 50, 120):
                        a = route_point(direction, 210, tangent, height)
                        b = route_point(direction, 360, tangent, height)
                        for start, end in ((a, b), (b, a)):
                            with self.subTest(prefix=prefix, route=route, tangent=tangent, height=height, start=start):
                                self.assertFalse(wall_hits(vertices, polygons, start, end), 'central bridge mouth obstructed')

    def test_long_bridge_sides_block_exit_but_ignore_camera(self):
        for prefix, (vertices, polygons) in self.collisions.items():
            for route, direction in enumerate(DIRECTIONS):
                for side in (-1, 1):
                    for height in (26, 120, 500):
                        start = route_point(direction, 500, 0, height)
                        end = route_point(direction, 500, side*(115 if route == 5 else 90), height)
                        with self.subTest(prefix=prefix, route=route, side=side, height=height):
                            hits = wall_hits(vertices, polygons, start, end)
                            self.assertTrue(hits, 'bridge side has no protective collision')
                            for _, poly in hits:
                                self.assertEqual(poly[1] & 0x6000, 0x2000, 'guard must ignore camera but collide with entities')

    def test_longitudinal_endpoints_are_open(self):
        for prefix, (vertices, polygons) in self.collisions.items():
            for route, direction in enumerate(DIRECTIONS):
                for tangent in ((-65, 0, 65) if route == 5 else (-40, 0, 40)):
                    for height in (26, 120, 500):
                        for near, far in ((210, 360), ((1060, 1160) if route == 5 else (850, 950))):
                            a = route_point(direction, near, tangent, height)
                            b = route_point(direction, far, tangent, height)
                            with self.subTest(prefix=prefix, route=route, tangent=tangent, height=height, endpoint=near):
                                self.assertFalse(wall_hits(vertices, polygons, a, b), 'bridge endpoint capped')
                                self.assertFalse(wall_hits(vertices, polygons, b, a), 'bridge endpoint capped on reverse approach')

    def test_center_guards_remain_between_bridge_mouths(self):
        for prefix, (vertices, polygons) in self.collisions.items():
            for route, (dx, dz) in enumerate(DIRECTIONS):
                angle = math.atan2(dz, dx)+math.pi/6
                direction = (math.cos(angle), math.sin(angle))
                with self.subTest(prefix=prefix, route=route):
                    self.assertTrue(wall_hits(vertices, polygons, route_point(direction, 180), route_point(direction, 330)),
                                    'useful center perimeter guard disappeared')

    def test_center_floor_medallions_and_small_steps_are_retained(self):
        source_vertices, source_polygons = self.original_center
        protected = set()
        for poly in source_polygons:
            points = [source_vertices[i & 0x1fff] for i in poly[1:4]]
            if max(math.hypot(p[0], p[2]) for p in points) >= 400:
                continue
            if max(p[1] for p in points)-min(p[1] for p in points) > 100:
                continue
            protected.add(tuple(sorted((p[2], CENTER[1]+p[1], CENTER[2]-p[0]) for p in points)))
        self.assertGreater(len(protected), 100)
        for prefix, (vertices, polygons) in self.collisions.items():
            actual = {tuple(sorted(vertices[i & 0x1fff] for i in p[1:4])) for p in polygons}
            self.assertTrue(protected <= actual, (prefix, 'central floor/relief collision changed', protected-actual))


if __name__ == '__main__':
    unittest.main()
