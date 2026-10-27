#!/usr/bin/env python3
"""Verify private MM display layout identity, closure and original preservation."""
from pathlib import Path
import re
import struct
import sys
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(ROOT / 'tools/black_market'), str(ROOT / 'scripts/assets')]
import forest
import mm_native_shop
import mm_shop_presentation as presentation
import native_market

REAR_PIECES = {
    0: [([[49, 37, 24], [97, 37, 72]], 25, 36)],
    2: [([[-120, 0, 0], [-62, 54, 54]], 116, 106),
        ([[-103, 60, 42], [-92, 62, 53]], 20, 16),
        ([[42, 0, 20], [105, 39, 79]], 94, 88)],
    6: [([[-104, 51, 27], [-53, 62, 53]], 80, 60)],
    18: [([[-114, 51, 2], [-69, 51, 47]], 30, 36)],
}

def triangle_indices(data):
    cache, faces = {}, []
    for node in ET.fromstring(data):
        if node.tag == 'LoadVertices':
            start, offset, count = [int(node.get(key)) for key in ('VertexBufferIndex', 'VertexOffset', 'Count')]
            cache.update({start+i: offset+i for i in range(count)})
        elif node.tag in ('Triangle1', 'Triangles2'):
            for face in range(2 if node.tag == 'Triangles2' else 1):
                faces.append((tuple(cache[int(node.get(f'V{face}{i}'))] for i in range(3)),
                              int(node.get(f'Flag{face}', '0'))))
    return faces


class PrivateMmPresentation(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.original = mm_native_shop.apply(native_market.read_bundle(ROOT / 'tools/native_market/bundled'))
        cls.candidate = presentation.apply(cls.original)

    def test_only_mm_room_mesh_changes_and_private_owners_are_added(self):
        self.assertEqual(set(self.candidate) - set(self.original), set(presentation.CLONES.values()))
        self.assertEqual({name for name in self.original if self.original[name] != self.candidate[name]},
                         {mm_native_shop.ROOM})
        before = mm_native_shop.native_commands(self.original[mm_native_shop.ROOM])
        after = mm_native_shop.native_commands(self.candidate[mm_native_shop.ROOM])
        self.assertEqual({k: v for k, v in before.items() if k != 10},
                         {k: v for k, v in after.items() if k != 10})
        old = forest.string(presentation.ROOM_ROOT)
        new = forest.string(presentation.CLONES[presentation.ROOM_ROOT])
        self.assertEqual(after[10], before[10].replace(old, new))
        self.assertEqual(presentation.apply(self.original), self.candidate)

    def test_whole_components_move_rigidly_and_keep_all_other_vertex_attributes(self):
        moved = 0
        for suffix, moves in presentation.MOVES.items():
            name = f'{presentation.GEOMETRY}_vtx_{suffix}'
            a = list(ET.fromstring(self.original[name]))
            b = list(ET.fromstring(self.candidate[presentation.CLONES[name]]))
            parts = presentation.market_interior.vertex_components(self.original,
                        f'{presentation.GEOMETRY}_tri_{suffix}')
            expected = {index: delta for bounds, delta, _, _ in moves for part in parts
                        if part['bounds'] == bounds for index in part['indices']}
            self.assertEqual(len(a), len(b))
            for index, (first, last) in enumerate(zip(a, b)):
                values = dict(first.attrib)
                if index in expected:
                    for axis, offset in zip('XYZ', expected[index]):
                        values[axis] = str(int(values[axis]) + offset)
                    moved += 1
                self.assertEqual(values, last.attrib)
        self.assertEqual(moved, 558)

    def test_rear_cauldron_bodies_contents_and_attachments_are_removed_without_other_faces(self):
        for suffix, pieces in REAR_PIECES.items():
            name = f'{presentation.GEOMETRY}_tri_{suffix}'
            parts = presentation.market_interior.vertex_components(self.original, name)
            removed = set()
            for bounds, count, triangles in pieces:
                part = next(part for part in parts if part['bounds'] == bounds)
                self.assertEqual((len(part['indices']), part['triangles']), (count, triangles))
                removed.update(part['indices'])
            before = triangle_indices(self.original[name])
            after = triangle_indices(self.candidate.get(presentation.CLONES.get(name, name), b'<DisplayList/>'))
            expected = [face for face in before if not any(v in removed for v in face[0])]
            self.assertEqual(len(before)-len(expected), sum(piece[2] for piece in pieces))
            self.assertEqual(after, expected, f'pot remnants or unrelated face loss in group {suffix}')

    def test_actual_mm_mesh_has_a_closed_resource_graph(self):
        data = mm_native_shop.native_commands(self.candidate[mm_native_shop.ROOM])[10]
        reader = forest.Reader(data)
        reader.offset = 4
        root, xlu = reader.text(), reader.text()
        self.assertEqual(root, presentation.CLONES[presentation.ROOM_ROOT])
        seen = set()
        def visit(name):
            self.assertIn(name, self.candidate)
            if name in seen:
                return
            seen.add(name)
            content = self.candidate[name]
            if content.lstrip().startswith(b'<'):
                for node in ET.fromstring(content).iter():
                    path = node.get('Path')
                    if path:
                        if path == native_market.STOCK_TEXTURE:
                            continue
                        visit(path)
        visit(root)
        self.assertTrue(set(presentation.CLONES.values()) <= seen)
        self.assertNotIn(presentation.ROOM_ROOT, seen)

    def test_changed_component_identity_is_rejected(self):
        for suffix, moves in presentation.MOVES.items():
            with self.subTest(suffix=suffix):
                malformed = dict(self.original)
                name = f'{presentation.GEOMETRY}_vtx_{suffix}'
                root = ET.fromstring(malformed[name])
                parts = presentation.market_interior.vertex_components(self.original,
                            f'{presentation.GEOMETRY}_tri_{suffix}')
                part = next(part for part in parts if part['bounds'] == moves[0][0])
                root[next(iter(part['indices']))].set('Y', '-1000')
                malformed[name] = ET.tostring(root)
                with self.assertRaisesRegex(ValueError, 'component identity'):
                    presentation.apply(malformed)

    def test_runtime_requires_every_generated_private_owner(self):
        source = (ROOT / 'combo/BlackMarketMmShopScene.h').read_text()
        required = re.findall(r'"(custom/skiejer_market/mm_shop_presentation_v1/[^"\n]+)"', source)
        self.assertEqual(set(required), set(presentation.CLONES.values()))


if __name__ == '__main__':
    unittest.main()
