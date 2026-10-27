"""Right potion floor panels must draw above their coplanar timber support."""
from pathlib import Path
import sys
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/black_market'))
import market_interior
import native_market

GEOMETRY = 'custom/skiejer_market/geometry/mahouya_room_0DL_006548'
DRAW = GEOMETRY + '_tri_23'
VERTICES = GEOMETRY + '_vtx_23'
MATERIAL = 'custom/skiejer_market/geometry/mat_mahouya_room_0DL_006548_GranmaShop01'
SURFACE = 'G_RM_AA_ZB_OPA_SURF2'
DECAL = 'G_RM_AA_ZB_OPA_DECAL2'


def draw_faces(resources, path=DRAW, initial=SURFACE):
    """Follow the actual vertex loads and native render-mode commands."""
    cache, faces, mode = {}, [], initial
    for node in ET.fromstring(resources[path]):
        if node.tag == 'LoadVertices':
            vertices = list(ET.fromstring(resources[node.get('Path')]))
            start, offset, count = (int(node.get(key)) for key in
                                    ('VertexBufferIndex', 'VertexOffset', 'Count'))
            for index, vertex in enumerate(vertices[offset:offset + count]):
                cache[start + index] = (offset + index,
                    tuple(int(vertex.get(key)) for key in ('X', 'Y', 'Z')))
        elif node.tag == 'SetOtherMode' and node.get('Cmd') == 'G_SETOTHERMODE_L':
            mode, = [key for key in (SURFACE, DECAL) if node.get(key) == '1']
        elif node.tag in ('Triangle1', 'Triangles2'):
            for face in range(2 if node.tag == 'Triangles2' else 1):
                points = [cache[int(node.get(f'V{face}{index}'))] for index in range(3)]
                faces.append((tuple(point[0] for point in points),
                              tuple(point[1] for point in points), mode))
    return faces, mode


def covers_xz(triangle, point):
    def cross(a, b, c):
        return (b[0] - a[0]) * (c[2] - a[2]) - (b[2] - a[2]) * (c[0] - a[0])
    edges = [cross(triangle[index], triangle[(index + 1) % 3], point) for index in range(3)]
    return min(edges) >= -1e-6 or max(edges) <= 1e-6


class PlatformOverlay(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.base = native_market.read_bundle(ROOT / 'tools/native_market/bundled')
        cls.prepared = market_interior.apply(cls.base)

    def test_coplanar_potion_panels_receive_decal_depth_mode(self):
        before, _ = draw_faces(self.base)
        after, _ = draw_faces(self.prepared)
        self.assertEqual(len(after), len(before))
        self.assertEqual([face[:2] for face in after], [face[:2] for face in before])
        panels = [face for face in after if min(face[0]) >= 63]
        self.assertEqual(len(panels), 8)
        timber, _ = draw_faces(self.base, GEOMETRY + '_tri_6')
        support = [face[1] for face in timber if min(face[0]) >= 849 and
                   all(point[1] == 8 for point in face[1])]
        for indices, points, mode in panels:
            self.assertTrue(all(point[1] == 8 for point in points))
            center = tuple(sum(point[axis] for point in points) / 3 for axis in range(3))
            self.assertTrue(any(covers_xz(face, center) for face in support),
                            f'Panel {indices} no longer overlaps the timber support')
            self.assertEqual(mode, DECAL, f'Coplanar panel {indices} still fights the timber depth')
        self.assertTrue(all(mode == SURFACE for indices, _, mode in after if max(indices) < 63))

    def test_normal_opaque_depth_mode_is_restored_after_panels(self):
        _, final = draw_faces(self.prepared)
        self.assertEqual(final, SURFACE)
        modes = [node for node in ET.fromstring(self.prepared[DRAW]) if node.tag == 'SetOtherMode']
        self.assertEqual(len(modes), 2)
        original = ET.fromstring(self.base[MATERIAL]).find("SetOtherMode[@Cmd='G_SETOTHERMODE_L']")
        self.assertEqual(modes[-1].attrib, original.attrib)
        decal = dict(original.attrib)
        decal.pop(SURFACE)
        decal[DECAL] = '1'
        self.assertEqual(modes[0].attrib, decal)

    def test_all_floor_vertices_collision_and_other_draw_lists_are_unchanged(self):
        self.assertEqual(self.prepared[VERTICES], self.base[VERTICES])
        self.assertEqual(self.prepared[native_market.COLLISION], self.base[native_market.COLLISION])
        for name, data in self.base.items():
            if '_tri_' in name and name != DRAW:
                self.assertEqual(self.prepared[name], data, name)
        original = list(ET.fromstring(self.base[DRAW]))
        corrected = list(ET.fromstring(self.prepared[DRAW]))
        retained = [node for node in corrected if node.tag not in ('PipeSync', 'SetOtherMode')]
        self.assertEqual([(node.tag, node.attrib) for node in retained],
                         [(node.tag, node.attrib) for node in original])

    def test_changed_panel_identity_is_rejected_before_preparation(self):
        changed = dict(self.base)
        vertices = ET.fromstring(changed[VERTICES])
        vertices[63].set('X', '97')
        changed[VERTICES] = ET.tostring(vertices)
        with self.assertRaisesRegex(ValueError, 'potion floor panel'):
            market_interior.apply(changed)


if __name__ == '__main__':
    unittest.main()
