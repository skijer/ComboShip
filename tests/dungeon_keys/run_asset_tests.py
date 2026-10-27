#!/usr/bin/env python3
"""Verify the bundled, MM-owned keys against the preserved surface repair."""
import importlib.util
from pathlib import Path
import unittest
import xml.etree.ElementTree as ET
import zipfile

ROOT = Path(__file__).resolve().parents[2]
ASSETS = ROOT / 'mm/assets/custom'
PREFIX = 'objects/combo_dungeon_keys/'
OLD_PREFIX = 'alt/objects/cor_mm_keys_poc2/'
REFERENCE = ROOT / 'tools/polish_20261007/mods/2ship/zzzz_MM_Dungeon_Keys_Surface_Clearance_POC.o2r'
ORIGINAL = ROOT / 'tools/polish_20261007/rollback/zzzz_MM_Dungeon_Keys_POC3.o2r'
spec = importlib.util.spec_from_file_location('key_surfaces', ROOT / 'scripts/assets/repair_mm_dungeon_key_surfaces.py')
surfaces = importlib.util.module_from_spec(spec)
spec.loader.exec_module(surfaces)


class BundledKeyAssets(unittest.TestCase):
    def setUp(self):
        with zipfile.ZipFile(REFERENCE) as archive:
            self.reference = {name.replace(OLD_PREFIX, PREFIX): archive.read(name).replace(
                OLD_PREFIX.encode(), PREFIX.encode()) for name in archive.namelist()}
        self.assets = {path.relative_to(ASSETS).as_posix(): path.read_bytes()
                       for path in (ASSETS / PREFIX).rglob('*') if path.is_file()}

    def test_all_eight_keys_are_bundled_without_an_external_mod(self):
        for dungeon in ('Woodfall', 'Snowhead', 'GreatBay', 'StoneTower'):
            for kind in ('Small', 'Boss'):
                for layer in ('Metal', 'Emblem'):
                    path = PREFIX + dungeon + kind + 'Key' + layer + 'DL'
                    self.assertIn(path, self.assets, 'Missing bundled key root: ' + path)
        self.assertEqual(set(self.assets), set(self.reference))
        self.assertFalse((ROOT / 'soh/assets/custom' / PREFIX).exists(), 'MM owns these resources')

    def test_every_display_list_dependency_is_bundled(self):
        self.assertTrue(self.assets, 'No bundled key resources')
        for path, data in self.assets.items():
            if not data.startswith(b'<'):
                continue
            for node in ET.fromstring(data):
                if 'Path' in node.attrib:
                    dependency = node.get('Path')
                    self.assertTrue(dependency.startswith(PREFIX), (path, dependency))
                    self.assertIn(dependency, self.assets, (path, dependency))
        self.assertEqual(surfaces.validate_archive(self.assets),
                         {'resources': 53, 'vertices': 17319, 'triangles': 5773})

    def test_repaired_shapes_materials_and_key_bodies_are_preserved(self):
        self.assertTrue(self.assets, 'No bundled key resources')
        self.assertEqual(self.assets, self.reference)
        with zipfile.ZipFile(ORIGINAL) as archive:
            for name in archive.namelist():
                if 'MetalDL_metal_vtx' not in name:
                    continue
                count = (126 if 'BossKey' in name else 78) * 3
                before = [node.attrib for node in ET.fromstring(archive.read(name))][:count]
                after = [node.attrib for node in ET.fromstring(self.assets[name.replace(OLD_PREFIX, PREFIX)])][:count]
                self.assertEqual(before, after, name)

    def test_both_woodfall_flowers_have_clearance_and_closed_petals(self):
        with zipfile.ZipFile(ORIGINAL) as archive:
            for kind, overlaps in (('Small', 44), ('Boss', 56)):
                name = 'Woodfall' + kind + 'KeyEmblemDL_emblem_vtx'
                before = archive.read(OLD_PREFIX + name)
                self.assertEqual(len(surfaces.analyze(before)[2]), overlaps)
                self.assertIn(PREFIX + name, self.assets, 'Missing bundled Woodfall flower')
                after = self.assets[PREFIX + name]
                _, _, conflicts, topology = surfaces.analyze(after)
                self.assertFalse(conflicts, 'Woodfall petals overlap')
                self.assertEqual(topology, {'boundaryEdges': 0, 'nonmanifoldEdges': 0})
                old_points, new_points = surfaces.positions(before), surfaces.positions(after)
                for axis in range(3):
                    self.assertEqual((min(p[axis] for p in old_points), max(p[axis] for p in old_points)),
                                     (min(p[axis] for p in new_points), max(p[axis] for p in new_points)))


if __name__ == '__main__':
    unittest.main()
