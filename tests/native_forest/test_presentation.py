#!/usr/bin/env python3
"""Real authored additions, independently pinned against the native control."""
import importlib.util
import io
import json
import os
from pathlib import Path
import struct
import tempfile
import unittest
import xml.etree.ElementTree as ET
import zipfile

from test_generator import ROOT, load_module

DONOR = Path(os.environ.get('NATIVE_FOREST_AUTHORED', str(
    ROOT.parent / 'inputs/zzzz_Skiejer_Native_Solo_Forest_FINAL_avatar_POC4.o2r')))
SOLO = 'custom/skiejer_forest/'
CORE = 'custom/comboship/native_forest/'
ART = CORE + 'authored/'
SHOPS = ('recipe7624751371212635', 'recipe3635031522573470', 'recipe215851318676066')
BENCHES = ('recipe449915211495247', 'recipe7187869513033518', 'recipe388081030655925')
PORTAL = ('ReturnRootTunnel', 'ReturnMushrooms_-1', 'ReturnMushrooms_1')


def source_target(path):
    tail = path.removeprefix(SOLO)
    if tail.removeprefix('prelude/').removesuffix('_vtx') in SHOPS:
        return CORE + 'building/' + tail.removeprefix('prelude/')
    return ART + tail


def without_access_collision(data):
    """Independent inverse of the reviewed additive 16/28/1 collision contract."""
    offset, parts = 76, [data[:76]]
    for stride, added in ((6, 16), (16, 28), (8, 1)):
        count = struct.unpack_from('<I', data, offset)[0]
        offset += 4
        parts.extend((struct.pack('<I', count - added), data[offset:offset + (count - added) * stride]))
        offset += count * stride
    return b''.join(parts) + data[offset:]


class PresentationTests(unittest.TestCase):
    def bake(self):
        module = load_module(self)
        self.assertTrue(hasattr(module, 'bake_presentation'),
                        'Native forest must bake the exact authored additions without a scene mod')
        if not DONOR.is_file():
            self.skipTest('Set NATIVE_FOREST_AUTHORED to the immutable FINAL-avatar solo donor')
        spec = importlib.util.spec_from_file_location('native_forest_bundle', ROOT / 'tools/black_market/native_forest_bundle.py')
        bundle = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(bundle)
        with zipfile.ZipFile(io.BytesIO(bundle.load_bundle(ROOT / 'tools/native_forest'))) as archive:
            before = {name: archive.read(name) for name in archive.namelist()}
        receipt = json.loads((ROOT / 'tools/native_forest/manifest.json').read_text())
        untouched = dict(before)
        resources, report = module.bake_presentation(before, receipt, DONOR)
        self.assertEqual(before, untouched, 'Bake must not mutate the preserved native source')
        return module, before, resources, report

    def test_native_base_reaches_exact_authored_pixels_vertices_and_poster_without_terrain(self):
        module, before, resources, report = self.bake()
        self.assertEqual(report['presentation']['version'], 2)
        self.assertEqual(report['presentation']['donor']['sha256'],
                         '65901fcc27453a915be9414c7203e8295fce36ce69b7658653bbcdf978af8d0e')
        self.assertEqual(report['custom_texture_payloads'], 12)
        self.assertEqual(len(report['presentation']['sources']), 45)
        with zipfile.ZipFile(DONOR) as source:
            for name in report['presentation']['sources']:
                data = source.read(name)
                if data[4:8] == b'XETO' or name.endswith('_vtx'):
                    self.assertEqual(resources[source_target(name)], data, name)
        self.assertFalse(any(name.startswith(ART + 'room_1/') for name in resources))
        self.assertFalse(any(name.startswith(ART + 'room_0/Geometry') and not name.endswith('_vtx')
                             for name in resources))
        self.assertFalse(any(name.startswith(('alt/', SOLO, 'custom/cor_mm3d/')) for name in resources))
        roots = ET.fromstring(resources[CORE + 'building_root'])
        paths = [node.get('Path') for node in roots if node.get('Path')]
        self.assertIn(ART + 'prelude/paste2461349930323740', paths,
                      'Poster is a separate authored draw, not the plain shop door')
        for recipe in BENCHES:
            self.assertIn(ART + 'prelude/' + recipe, paths)
        self.assertEqual([node.get('Path') for node in ET.fromstring(resources[CORE + 'portal_root'])
                          if node.get('Path')], [ART + 'room_0/' + name for name in PORTAL])

    def test_only_portal_primitive_rgb_changes_and_shared_purple_materials_survive(self):
        module, _, resources, _ = self.bake()
        source_name = SOLO + 'room_0/ReturnTunnelMouth'
        target_name = ART + 'room_0/ReturnTunnelMouth'
        with zipfile.ZipFile(DONOR) as source:
            original = source.read(source_name)
            changed = resources[target_name]
            self.assertEqual(struct.unpack_from('<II', changed, 208), (0xFA000000, 0x44FF7460))
            # Ignore only the intended resource-identity fixups, then prove the
            # authored command stream changed at exactly its RGB bytes.
            restored = bytearray(changed)
            for offset, _, _, opcode in module.display_commands(original, source_name):
                if opcode in module.EXPANDED:
                    restored[offset + 8:offset + 16] = original[offset + 8:offset + 16]
            restored[212:216] = original[212:216]
            self.assertEqual(bytes(restored), original)
            self.assertEqual(resources[ART + 'prelude/black_market_soft_magic_mask'],
                             source.read(SOLO + 'prelude/black_market_soft_magic_mask'))
            for path, expected in [('prelude/black_magic_walls', 0x00000040),
                                   ('prelude/black_magic_roof', 0x00000040),
                                   ('prelude/mirror_stump_violet_shimmer', 0x5C1C9828)]:
                colors = [w1 for _, _, w1, opcode in module.display_commands(resources[ART + path], ART + path)
                          if opcode == 0xFA]
                self.assertEqual(colors, [expected])

    def test_authored_bake_preserves_native_gameplay_records_and_owned_shop_geometry(self):
        module, before, resources, report = self.bake()
        for prefix in module.SCENES.values():
            for suffix in ('', '_collision', '_room_1', '_room_2'):
                self.assertEqual(resources[prefix + suffix], before[prefix + suffix])
            old = module.read_scene(before[prefix + '_room_0'], oot=prefix == module.OOT)
            new = module.read_scene(resources[prefix + '_room_0'], oot=prefix == module.OOT)
            self.assertEqual({key: value for key, value in old.items() if key != 10},
                             {key: value for key, value in new.items() if key != 10})
            mesh = module.read_mesh(new[10])
            self.assertEqual(mesh['type'], 2)
            for root in ('building_root', 'portal_root'):
                self.assertEqual(sum(entry['opaque'] == CORE + root for entry in mesh['entries']), 1)
            for root in ('building_xlu', 'portal_xlu', 'stump_shimmer_root'):
                self.assertEqual(sum(entry['translucent'] == CORE + root for entry in mesh['entries']), 1)
        for recipe in SHOPS:
            for suffix in ('', '_vtx'):
                path = CORE + 'building/' + recipe + suffix
                self.assertEqual(resources[path], before[path])
        for path in before:
            if path.startswith(CORE + 'geometry/'):
                self.assertEqual(resources[path], before[path])
        # These literals pin the original native control, independent of bake.
        protected = json.loads((Path(__file__).parent / 'presentation_control.json').read_text())
        for path, expected in protected.items():
            data = resources[path]
            if path.endswith('_collision') and report.get('mirror_access'):
                data = without_access_collision(data)
            self.assertEqual(module.sha(data), expected, path)
        self.assertEqual(report['presentation']['portal']['radius'], 68)
        self.assertEqual(report['presentation']['portal']['center_y'], 66)
        self.assertEqual(report['portal'].get('radius'), 68,
                         'Manifest must describe the active authored portal rather than the inert control arch')

    def test_six_scroll_tracks_close_all_draw_paths_and_cull_bounds(self):
        module, _, resources, report = self.bake()
        expected = [(8, [(0, 0, 32, 32), (0, -1, 32, 64)]),
                    (9, [(0, 0, 32, 32), (-1, 0, 32, 64)]),
                    (10, [(-1, -1, 64, 64)]), (11, [(1, 1, 64, 64)]),
                    (12, [(-1, 1, 64, 64)]), (13, [(0, -1, 64, 64)])]
        tracks = module.scroll_bindings(resources[CORE + 'texanim'])
        self.assertEqual([(row['segment'], [tuple(layer[key] for key in ('x', 'y', 'width', 'height'))
                                           for layer in row['layers']]) for row in tracks], expected)
        self.assertEqual(resources[CORE + 'stock_materials'], module.material_blob(tracks))
        with zipfile.ZipFile(DONOR) as source:
            self.assertEqual(resources[CORE + 'texanim'], source.read(SOLO + 'final_texanim'))
        hashes = {module.crc64(path): path for path in [*resources, *report['external_resources']]}
        reached, segments = set(), set()
        def walk(path):
            if not path or path in reached:
                return []
            reached.add(path)
            if path not in resources:
                self.assertIn(path, report['external_resources'])
                return []
            data, points = resources[path], []
            if data.startswith(b'<Vertex'):
                return [tuple(int(node.get(axis)) for axis in ('X', 'Y', 'Z')) for node in ET.fromstring(data)]
            if data[4:8] == b'RRAO':
                return module.vertex_positions(data)
            if data.startswith(b'<DisplayList'):
                for node in ET.fromstring(data):
                    if node.get('Path'):
                        points.extend(walk(node.get('Path')))
            elif data[4:8] == b'TLDO':
                for offset, _, w1, opcode in module.display_commands(data, path):
                    if opcode in module.EXPANDED and opcode != 0x33:
                        hi, lo = struct.unpack_from('<II', data, offset + 8)
                        self.assertIn((hi << 32) | lo, hashes, path)
                        points.extend(walk(hashes[(hi << 32) | lo]))
                    elif opcode == 0xDE:
                        segments.add(w1 >> 24)
            return points
        for prefix in module.SCENES.values():
            mesh = module.read_mesh(module.read_scene(resources[prefix + '_room_0'], oot=prefix == module.OOT)[10])
            for entry in mesh['entries']:
                if entry['opaque'] in (CORE + 'building_root', CORE + 'portal_root') or \
                        entry['translucent'] == CORE + 'stump_shimmer_root':
                    reached.clear()
                    points = walk(entry['opaque']) + walk(entry['translucent'])
                    self.assertTrue(points)
                    x, y, z, radius = entry['bounds']
                    self.assertTrue(all(sum((p[i] - (x, y, z)[i]) ** 2 for i in range(3)) <= radius ** 2
                                        for p in points), entry)
        self.assertEqual(segments, {8, 9, 10, 11, 12, 13})
        module.validate_resources(resources, report)

    def test_donor_identity_gate_rejects_a_changed_archive(self):
        module, before, _, report = self.bake()
        with tempfile.TemporaryDirectory(prefix='native-authored-bad-') as folder:
            bad = Path(folder) / 'changed.o2r'
            bad.write_bytes(b'not the authored donor')
            with self.assertRaisesRegex(ValueError, 'SHA256|immutable|identity'):
                module.bake_presentation(before, report, bad)

    def test_separate_mirror_access_step_preserves_all_original_collision_and_is_idempotent(self):
        module, _, resources, report = self.bake()
        self.assertTrue(hasattr(module, 'bake_mirror_access'),
                        'Authorized fallen-log access must integrate separately from the exact authored bake')
        # Exercise a fresh append even after the checked-in candidate includes
        # the ramp; the pinned control hashes above independently verify undo.
        if report.get('mirror_access'):
            for prefix in module.SCENES.values():
                path = prefix + '_collision'
                resources[path] = without_access_collision(resources[path])
                path = prefix + '_room_0'
                commands = module.read_scene(resources[path], oot=prefix == module.OOT)
                mesh = module.read_mesh(commands[10])
                mesh['entries'] = [entry for entry in mesh['entries'] if entry['opaque'] != CORE + 'mirror_access_root']
                commands[10] = module.write_mesh(mesh)
                resources[path] = module.write_scene(commands)
            for suffix in ('root', 'material', 'vtx'):
                resources.pop(CORE + 'mirror_access_' + suffix)
            report = {key: value for key, value in report.items() if key != 'mirror_access'}
        original = dict(resources)
        baked, augmented = module.bake_mirror_access(resources, report)
        self.assertEqual(resources, original)
        self.assertEqual(augmented['mirror_access']['collision_delta'], {'vertices': 16, 'polygons': 28, 'surfaces': 1})
        self.assertEqual(augmented['mirror_access']['start'], [-105, 0, 416])
        self.assertEqual(augmented['mirror_access']['end'], [-345, 80, 416])
        for prefix in module.SCENES.values():
            collision = prefix + '_collision'
            self.assertEqual(without_access_collision(baked[collision]), original[collision])
            for suffix in ('', '_room_1', '_room_2'):
                self.assertEqual(baked[prefix + suffix], original[prefix + suffix])
            commands = module.read_scene(baked[prefix + '_room_0'], oot=prefix == module.OOT)
            entries = module.read_mesh(commands[10])['entries']
            self.assertEqual(sum(entry['opaque'] == CORE + 'mirror_access_root' for entry in entries), 1)
        again, repeated = module.bake_mirror_access(baked, augmented)
        self.assertEqual(again, baked)
        self.assertTrue(repeated['mirror_access']['already_present'])
        authored_again, _ = module.bake_presentation(baked, augmented, DONOR)
        self.assertEqual(authored_again, baked, 'Authored rebake must preserve access geometry and mesh ordering')
        module.validate_resources(baked, augmented)

    def test_one_source_bundle_prepares_both_hosts_and_serializes_deterministically(self):
        module, _, resources, report = self.bake()
        spec = importlib.util.spec_from_file_location('prepare_native_forest', ROOT / 'scripts/assets/prepare_native_forest.py')
        prepare = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(prepare)
        with tempfile.TemporaryDirectory(prefix='native-authored-prepare-') as folder:
            directory = Path(folder)
            one, two = directory / 'one.zip', directory / 'two.zip'
            manifest = module.write_bundle(resources, report, one)
            module.write_bundle(resources, report, two)
            self.assertEqual(module.file_sha(one), module.file_sha(two))
            metadata = directory / 'manifest.json'
            metadata.write_text(json.dumps(manifest))
            effective = prepare.prepared_resources(metadata)
            prepare.prepare(directory / 'output', metadata)
            for engine in ('soh', 'mm'):
                root = directory / 'output' / engine / 'assets/custom'
                for path, data in effective.items():
                    self.assertEqual((root / path).read_bytes(), data)


if __name__ == '__main__':
    unittest.main()
