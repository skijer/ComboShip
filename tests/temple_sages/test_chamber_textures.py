"""Keep the three source-authentic HD center images and their logical addressing."""
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import unittest


ROOT = Path(__file__).resolve().parents[2]
NAMESPACE = 'custom/temple_sages/chamber/'
EXPECTED = {
    'rm_kja_water_02_13C00': (
        '3c494ee4c0337786d8824a656ec59978a8548244faacbc50184f3a24f07a4a19',
        (128, 128), (32, 32)),
    'rm_kja_mark_01_C000': (
        '3675f4755ad531081cbfc0b142f9286b771930ad00b86f8470b24074fa95bfa4',
        (128, 256), (32, 64)),
    'rm_kja_yuka_01_16C00': (
        '83ec24507977fc7a9d0a42a50084635d2204fd1148787a6e850573a792cdea63',
        (256, 256), (64, 64)),
}


class ChamberTextures(unittest.TestCase):
    def setUp(self):
        path = ROOT / 'tools/temple_sages/chamber_textures.py'
        self.assertTrue(path.exists(), 'the copied center still uses embedded low-resolution MAP images')
        spec = importlib.util.spec_from_file_location('chamber_textures', path)
        self.module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(self.module)

    def test_exact_authentic_donor_bytes_are_indexed_under_both_names(self):
        resources = self.module.apply({})
        self.assertEqual(set(resources), {prefix + NAMESPACE + name
            for prefix in ('', 'alt/') for name in EXPECTED})
        for name, (digest, _, _) in EXPECTED.items():
            blob = resources[NAMESPACE + name]
            self.assertEqual(hashlib.sha256(blob).hexdigest(), digest, name)
            self.assertEqual(resources['alt/' + NAMESPACE + name], blob)

    def test_raw_payload_metadata_keeps_existing_sampler_and_atlas_uvs(self):
        resources = self.module.apply({})
        for name, (_, dimensions, logical) in EXPECTED.items():
            blob = resources['alt/' + NAMESPACE + name]
            self.assertEqual(blob[4:8], b'XETO')
            self.assertEqual(struct.unpack_from('<I', blob, 8)[0], 1)
            kind, width, height, flags, hscale, vscale, size = struct.unpack_from('<IIIIffI', blob, 64)
            self.assertEqual((kind, flags, hscale, vscale), (2, 1, 8.0, 4.0), name)
            self.assertEqual((width, height), dimensions)
            self.assertEqual((width * 2 / hscale, height / vscale), logical)
            self.assertEqual(size, width * height * 4)
            self.assertEqual(len(blob), 92 + size)

    def test_materials_vertices_collision_and_all_other_entries_are_preserved(self):
        resources = {
            NAMESPACE + 'mat_ChamberSage01': b'<DisplayList><ScrollTexture Tile="0" SpeedX="-1" SpeedY="1" /></DisplayList>',
            NAMESPACE + 'center_vtx': b'authored positions, colors and atlas UVs',
            'scenes/shared/tokinoma_scene/collision': b'physical hub and bridge collision',
            NAMESPACE + 'rm_kja_water_02_13C00___Copie': b'non-target water texture',
            'alt/scenes/shared/kenjyanoma_scene/rm_kja_mark_01_C000': b'original Sages runtime untouched',
        }
        for name in EXPECTED:
            resources['alt/' + NAMESPACE + name] = b'embedded MAP texture before HD restoration'
        before = dict(resources)
        restored = self.module.apply(resources)
        self.assertEqual(resources, before, 'applying textures mutates the supplied graph')
        replaced = {prefix + NAMESPACE + name for prefix in ('', 'alt/') for name in EXPECTED}
        self.assertEqual({name: data for name, data in restored.items() if name not in replaced},
                         {name: data for name, data in before.items() if name not in replaced})

    def test_custom_private_namespace_is_closed_and_repeatable(self):
        namespace = 'custom/temple_candidate/chamber/'
        first = self.module.apply({}, namespace)
        self.assertEqual(first, self.module.apply(first, namespace))
        self.assertTrue(all(name.startswith((namespace, 'alt/' + namespace)) for name in first))
        for name, (digest, _, _) in EXPECTED.items():
            self.assertEqual(hashlib.sha256(first[namespace + name]).hexdigest(), digest)

    def test_public_resource_checksums_identify_only_three_active_images(self):
        path = ROOT / 'tools/temple_sages/assets/chamber_hd/provenance.json'
        provenance = json.loads(path.read_text())
        self.assertEqual(provenance['source']['kind'], 'resource-checksums')
        self.assertEqual(set(provenance['textures']), set(EXPECTED))
        for name, (digest, physical, logical) in EXPECTED.items():
            entry = provenance['textures'][name]
            self.assertEqual(entry['resource_sha256'], digest)
            self.assertEqual(entry['source_path'], 'alt/scenes/shared/kenjyanoma_scene/' + name)
            self.assertEqual(entry['physical_dimensions'], list(physical))
            self.assertEqual(entry['logical_dimensions'], list(logical))


if __name__ == '__main__':
    unittest.main()
