"""The shipped chamber must resolve without its external Prelude addon."""
import importlib.util
import base64
import hashlib
import json
from pathlib import Path
import shutil
import struct
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]


class PackagedChamber(unittest.TestCase):
    def load_preparer(self):
        path = ROOT / 'scripts/assets/prepare_temple_sages.py'
        self.assertTrue(path.is_file(), 'normal asset preparation does not package the chamber')
        spec = importlib.util.spec_from_file_location('prepare_temple_sages', path)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        return module

    def test_default_chamber_keeps_only_native_texture_artwork(self):
        module = self.load_preparer()
        resources = module.verified_resources()
        metadata = json.loads(module.DEFAULT_MANIFEST.read_text())
        self.assertEqual(metadata.get('texture_policy'), 'native-oot-base-optional-visual-addon')
        bindings = metadata['native_texture_bindings']
        textures = {name: data for name, data in resources.items() if data[4:8] == b'XETO'}
        self.assertTrue(textures)
        self.assertEqual(set(textures), set(bindings))
        for path, data in textures.items():
            source = bindings[path]
            self.assertEqual(hashlib.sha256(data).hexdigest(), source['resource_sha256'])
            self.assertTrue(source['native_paths'])
            self.assertTrue(all(not native.startswith(('alt/', 'custom/')) for native in source['native_paths']))

    def test_default_chamber_is_static_and_has_one_geometry_layout(self):
        module = self.load_preparer()
        resources = module.verified_resources()
        vertices = 0
        for path, data in resources.items():
            if data.lstrip().startswith(b'<'):
                tree = ET.fromstring(data)
                self.assertFalse(any(element.tag == 'ScrollTexture' for element in tree.iter()), path)
                if tree.tag == 'Vertex':
                    vertices += 1
                    if not path.startswith('alt/'):
                        self.assertEqual(resources.get('alt/' + path), data, path)
        self.assertGreater(vertices, 1000)

    def test_chamber_prepares_for_both_normal_port_archives(self):
        module = self.load_preparer()
        with tempfile.TemporaryDirectory() as directory:
            resources = module.prepare(Path(directory))
            self.assertGreater(len(resources), 1900)
            for game in ('soh', 'mm'):
                for path, expected in resources.items():
                    self.assertEqual((Path(directory) / game / 'assets/custom' / path).read_bytes(), expected)
                self.assertFalse((Path(directory) / game / 'assets/custom/prelude').exists())

    def test_both_asset_modes_have_reachable_chamber_and_doorway(self):
        module = self.load_preparer()
        resources = module.verified_resources()
        for prefix in ('', 'alt/'):
            root = resources[prefix + 'scenes/shared/tokinoma_scene/tokinoma_room_0DL_008128']
            self.assertIn(b'custom/temple_sages/annex_opa', root)
            for path in ('annex_opa', 'annex_xlu', 'stone_tunnel', 'enclosure'):
                self.assertIn(prefix + 'custom/temple_sages/' + path, resources)
            self.assertIn(prefix + 'scenes/shared/tokinoma_scene/tokinoma_room_0ZeldaAdult', resources)
            self.assertIn(prefix + 'scenes/shared/tokinoma_scene/tokinoma_room_0Set_000120', resources)
            self.assertIn(prefix + 'scenes/shared/tokinoma_scene/tokinoma_room_0Set_0001E0', resources)
        self.assertTrue(any('/native_host/aperture_' in name for name in resources))
        self.assertEqual(module.validate_dependencies(resources), [])

    def test_missing_storage_part_fails_before_creating_port_outputs(self):
        module = self.load_preparer()
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            manifest = json.loads(module.DEFAULT_MANIFEST.read_text())
            path = root / 'manifest.json'
            path.write_text(json.dumps(manifest))
            with self.assertRaisesRegex(ValueError, 'Missing .*part'):
                module.prepare(root / 'output', path)
            self.assertFalse((root / 'output').exists())

    def test_source_collision_and_age_specific_zelda_headers_survive_packaging(self):
        module = self.load_preparer()
        resources = module.verified_resources()
        sys.path.insert(0, str(ROOT / 'tools/temple_sages'))
        import resource_geometry as geometry
        temple = 'scenes/shared/tokinoma_scene/'
        for prefix in ('', 'alt/'):
            collision = resources[prefix + temple + 'tokinoma_sceneCollisionHeader_0032F8']
            # Geometry/header bytes precede the polygon table; the permitted
            # acoustic change remaps only floor type indices and audio bits.
            metadata = json.loads(module.DEFAULT_MANIFEST.read_text())
            sys.path.insert(0, str(ROOT / 'tools/temple_sages_bundle'))
            import collision_audio
            self.assertEqual(collision_audio.inspect(collision), metadata['collision_audio'])
            self.assertTrue(all(row['sound'] == 2 and row['echo'] == 63
                                for row in metadata['collision_audio']['floor_types']))
            base = resources[prefix + temple + 'tokinoma_room_0']
            table = next(info['paths'] for command, _, _, info in geometry.room_commands(base) if command == 0x18)
            # Empty child-night/adult-night entries inherit the prior effective
            # header, preserving both normal ages and all story setup entries.
            self.assertEqual(table[:3], ['', temple + 'tokinoma_room_0ZeldaAdult', ''])
            for suffix, parameter in (('', 0x7d23), ('ZeldaAdult', 0x7f34)):
                data = resources[prefix + temple + 'tokinoma_room_0' + suffix]
                actor_data = next(info['actors'] for command, _, _, info in geometry.room_commands(data) if command == 1)
                actors = [struct.unpack_from('<8h', actor_data, offset) for offset in range(0, len(actor_data), 16)]
                self.assertEqual([actor for actor in actors if actor[0] == 42],
                                 [(42, 0, -110, -2400, 0, 0, 0, parameter)])

    def test_repreparing_native_base_removes_only_owned_stale_visual_resources(self):
        module = self.load_preparer()
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for game in ('soh', 'mm'):
                owned = root / game / 'assets/custom/custom/temple_sages/retired_hd_texture'
                owned.parent.mkdir(parents=True, exist_ok=True)
                owned.write_bytes(b'superseded visual texture')
                unrelated = root / game / 'assets/custom/scenes/shared/tokinoma_scene/unrelated_user_resource'
                unrelated.parent.mkdir(parents=True, exist_ok=True)
                unrelated.write_bytes(b'keep')
            module.prepare(root)
            for game in ('soh', 'mm'):
                self.assertFalse((root / game / 'assets/custom/custom/temple_sages/retired_hd_texture').exists())
                self.assertEqual((root / game / 'assets/custom/scenes/shared/tokinoma_scene/unrelated_user_resource').read_bytes(), b'keep')

    def test_symlinked_private_namespace_cannot_sweep_unrelated_assets(self):
        module = self.load_preparer()
        for game in ('soh', 'mm'):
            for prefix in ('custom/temple_sages', 'alt/custom/temple_sages', 'custom', 'alt/custom'):
                with self.subTest(game=game, prefix=prefix), tempfile.TemporaryDirectory() as directory:
                    root = Path(directory)
                    assets = root / game / 'assets/custom'
                    unrelated = assets / 'scenes/shared/tokinoma_scene'
                    unrelated.mkdir(parents=True)
                    sentinel = unrelated / 'unrelated_user_resource'
                    sentinel.write_bytes(b'preserve unrelated scene asset')
                    link = assets / prefix
                    link.parent.mkdir(parents=True, exist_ok=True)
                    link.symlink_to(unrelated, target_is_directory=True)
                    with self.assertRaisesRegex(ValueError, 'redirected|symlink'):
                        module.prepare(root)
                    self.assertEqual(sentinel.read_bytes(), b'preserve unrelated scene asset')

    def test_source_parts_use_the_proven_bounded_lossless_pool(self):
        module = self.load_preparer()
        metadata = json.loads(module.DEFAULT_MANIFEST.read_text())
        bundle = metadata['bundle']
        self.assertEqual(bundle['version'], 1)
        self.assertLessEqual(bundle['raw_bytes'], 64 * 1024 * 1024)
        for number, part in enumerate(bundle['parts'], 1):
            self.assertEqual(part['name'], f'parts/pool.zlib.{number:03d}.b64')
            data = (module.DEFAULT_MANIFEST.parent / part['name']).read_bytes()
            self.assertLessEqual(len(data), 128 * 1024)
            self.assertLessEqual(len(base64.b64decode(data, validate=True)), 96 * 1024)
        self.assertEqual(set(metadata['resources']), {row['path'] for row in bundle['files']})

    def test_invalid_retired_path_fails_before_writes(self):
        module = self.load_preparer()
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            metadata = json.loads(module.DEFAULT_MANIFEST.read_text())
            metadata['retired_resources'] = ['custom/temple_sages/../../unrelated']
            manifest = root / 'manifest.json'
            manifest.write_text(json.dumps(metadata))
            for part in metadata['bundle']['parts']:
                target = root / part['name']
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(module.DEFAULT_MANIFEST.parent / part['name'], target)
            sentinel = root / 'output/soh/assets/custom/custom/temple_sages/prior_visual'
            sentinel.parent.mkdir(parents=True)
            sentinel.write_bytes(b'preserve on invalid source')
            with self.assertRaisesRegex(ValueError, 'retired chamber resource'):
                module.prepare(root / 'output', manifest)
            self.assertEqual(sentinel.read_bytes(), b'preserve on invalid source')

    def test_corrupt_storage_part_fails_before_creating_port_outputs(self):
        module = self.load_preparer()
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            metadata = json.loads(module.DEFAULT_MANIFEST.read_text())
            manifest = root / 'manifest.json'
            manifest.write_text(json.dumps(metadata))
            for part in metadata['bundle']['parts']:
                target = root / part['name']
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(module.DEFAULT_MANIFEST.parent / part['name'], target)
            part = root / metadata['bundle']['parts'][0]['name']
            with part.open('r+b') as stream:
                first = stream.read(1)
                stream.seek(0)
                stream.write(bytes([first[0] ^ 1]))
            with self.assertRaisesRegex(ValueError, 'hash/size mismatch'):
                module.prepare(root / 'output', manifest)
            self.assertFalse((root / 'output').exists())


if __name__ == '__main__':
    unittest.main()
