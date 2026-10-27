"""A bounded normal-only hand appearance POC, never a geometry retarget."""
import hashlib
import importlib
import json
import os
from pathlib import Path
import struct
import unittest
import zipfile

import numpy as np

try:
    hands = importlib.import_module('repair_anime_saria_hands')
except ModuleNotFoundError:
    hands = None


def vertex(xyz, normal, slot=0):
    return (0x0d000001+slot*64, struct.pack('<hhhHhhbbbB', *xyz, slot << 8, 123, -234, *normal, 255))


class HandNormalTests(unittest.TestCase):
    def setUp(self):
        self.assertIsNotNone(hands, 'bounded Saria hand-normal repair is required')

    def test_internal_hotspot_softens_with_f3dex_light_and_capped_normal_change(self):
        center = vertex((0,0,0), (82,0,97))
        ring = [vertex(p, (0,0,127)) for p in ((-20,0,0),(0,20,0),(20,0,0),(0,-20,0))]
        triangles = [(center,ring[i],ring[(i+1)%4]) for i in range(4)]
        anchors = {hands.point_key(v) for v in ring}
        normals = hands.soften_normals(triangles, anchors)
        old = np.array((82,0,97),float);old /= np.linalg.norm(old)
        new = np.array(struct.unpack('<bbb', normals[hands.point_key(center)]),float);new /= np.linalg.norm(new)
        angle = np.degrees(np.arccos(np.clip(old@new,-1,1)))
        self.assertLessEqual(angle,15.5)  # Includes signed-byte quantization.
        self.assertGreater(.15+.85*old[0]-(.15+.85*new[0]),.1)
        for v in ring:
            self.assertEqual(normals[hands.point_key(v)],v[1][12:15])

    def test_different_matrix_owners_do_not_mix_normal_bases(self):
        a = vertex((0,0,0),(82,0,97),0)
        b,c = vertex((20,0,0),(0,0,127),1),vertex((0,20,0),(0,0,127),1)
        result = hands.soften_normals([(a,b,c)],set())
        self.assertEqual(result[hands.point_key(a)],a[1][12:15])

    def test_conflicting_coincident_normals_are_preserved_without_blind_welding(self):
        a,b = vertex((0,0,0),(127,0,0)),vertex((0,0,0),(-95,0,-84))
        c,d = vertex((0,20,0),(0,0,127)),vertex((20,0,0),(0,0,127))
        result = hands.soften_normals([(a,c,d),(b,d,c)],set())
        self.assertNotIn(hands.point_key(a),result)

    def test_asset_changes_only_hand_normal_bytes_and_preserves_all_wrist_rims(self):
        path = os.environ.get('SARIA_ANIME_HANDS_ARCHIVE')
        if not path:
            self.skipTest('Set SARIA_ANIME_HANDS_ARCHIVE for exact archive checks')
        with zipfile.ZipFile(path) as archive:
            source = {n:archive.read(n) for n in archive.namelist()}
        # Independent face/ocarina metadata can coexist with this bounded patch.
        source['SeparateFaceAndOcarinaRepair.json'] = b'{"other repairs": "retain exactly"}'
        output,report = hands.repair_records(source)
        self.assertEqual(set(source),set(output))
        self.assertGreater(report['changed_normal_records'],100)
        self.assertTrue(report['wrist_normals_byte_identical'])
        self.assertEqual(report['hand_meshes'],20)
        self.assertEqual(report['ambiguous_points_preserved'],10)
        for name,before in source.items():
            after = output[name]
            if before != after:
                self.assertTrue(name.endswith('/V'),name)
                self.assertEqual(len(before),len(after))
                changed = [i for i,(a,b) in enumerate(zip(before,after)) if a!=b]
                self.assertTrue(all(i>=72 and (i-72)%16 in (12,13,14) for i in changed),name)
            if '/Face' in name or '/Props/' in name or 'Limb10/V' in name or 'Limb13/V' in name:
                self.assertEqual(before,after,name)
        again,second = hands.repair_records(output)
        self.assertEqual(again,output)
        self.assertEqual(second['changed_normal_records'],0)

    def test_asset_rejects_unknown_hand_shape_and_source_identity(self):
        path = os.environ.get('SARIA_ANIME_HANDS_ARCHIVE')
        if not path:
            self.skipTest('Set SARIA_ANIME_HANDS_ARCHIVE for exact archive checks')
        with zipfile.ZipFile(path) as archive:
            source = {n:archive.read(n) for n in archive.namelist()}
        changed = dict(source)
        name = 'alt/objects/object_sa/NativeNpcV1/Body/Limb11/V'
        raw = bytearray(changed[name]);raw[72] ^= 1;changed[name]=bytes(raw)
        with self.assertRaisesRegex(ValueError,'hand geometry'):
            hands.repair_records(changed)
        changed = dict(source)
        name = 'saria_anime_NativeNpcModelInfo.json'
        info = json.loads(changed[name]);info['source_sha256']='different source'
        changed[name]=json.dumps(info).encode()
        with self.assertRaisesRegex(ValueError,'source identity'):
            hands.repair_records(changed)


if __name__ == '__main__':
    unittest.main()
