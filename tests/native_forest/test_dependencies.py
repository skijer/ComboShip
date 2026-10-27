#!/usr/bin/env python3
"""Complete per-host readiness inventory and strict, image-free Blob framing."""
import struct
import tempfile
from pathlib import Path
import unittest

from test_generator import BARK, COMBINED, MM, OOT, PREFIX, SCENE, fixture, header, load_module


def literal_blob(core, stock, version=1):
    """Independent wire encoder for malformed fixtures; no production helpers."""
    payload = struct.pack('<BHH', version, len(core), len(stock))
    for path in [*core, *stock]:
        encoded = path.encode('utf-8') if isinstance(path, str) else path
        payload += struct.pack('<H', len(encoded)) + encoded
    return header(0x4F424C42) + struct.pack('<I', len(payload)) + payload


class DependencyTests(unittest.TestCase):
    def test_each_host_descriptor_contains_the_exact_generated_closure_and_all_stock(self):
        module = load_module(self)
        resources, receipt = module.build_resources(*fixture(module))
        modes = {'combined': COMBINED, 'mm': MM, 'oot': OOT}
        descriptors = {PREFIX + 'dependencies_' + mode + '_v1' for mode in modes}
        self.assertTrue(descriptors <= resources.keys(), 'Complete dependency Blobs are required for readiness')
        host_paths = {prefix: {prefix, prefix + '_collision', *(prefix + f'_room_{i}' for i in range(3))}
                      for prefix in modes.values()}
        shared = set(resources) - descriptors - set().union(*host_paths.values())
        private_child = PREFIX + 'geometry/Z2_LOST_WOODS_room_00DL_fixture_child'
        private_root = PREFIX + 'geometry/Z2_LOST_WOODS_room_00DL_002548'
        self.assertIn(private_child, shared)
        self.assertIn(private_root, shared)
        self.assertNotIn(private_child, module.native_core_paths(), 'The fixture must exercise donor-derived ancestry')
        for mode, prefix in modes.items():
            name = PREFIX + 'dependencies_' + mode + '_v1'
            core, stock = module.read_dependency_blob(resources[name])
            self.assertEqual(core, sorted(shared | host_paths[prefix]))
            self.assertEqual(stock, sorted(receipt['external_resources']))
            self.assertIn(private_child, core)
            self.assertIn(private_root, core)
            for path in (BARK, SCENE + 'Tex_00B9D0', SCENE + '_room_02DL_000B48',
                         SCENE + '_room_02Vtx_fixture'):
                self.assertIn(path, stock)
            self.assertFalse(set(core) & set(stock))
            self.assertFalse(set(core) & descriptors, 'Runtime checks the descriptor itself before loading it')
            for other in set(modes.values()) - {prefix}:
                self.assertFalse(set(core) & host_paths[other])
            self.assertEqual(resources[name], module.dependency_blob(core, stock))
        text = module.native_core_header()
        for constant, mode in (('kCombinedDependencies', 'combined'), ('kMmSoloDependencies', 'mm'),
                               ('kOotSoloDependencies', 'oot')):
            self.assertIn(f'{constant} = "{PREFIX}dependencies_{mode}_v1"', text)
        self.assertEqual(receipt['dependency_paths'], {mode: PREFIX + 'dependencies_' + mode + '_v1' for mode in modes})

    def test_dependency_blob_matches_hand_literal_and_is_deterministic(self):
        module = load_module(self)
        self.assertTrue(hasattr(module, 'dependency_blob'), 'Strict dependency serialization is missing')
        payload = b'\x01\x01\x00\x01\x00\x08\x00custom/a\x08\x00native/b'
        expected = header(0x4F424C42) + struct.pack('<I', len(payload)) + payload
        self.assertEqual(module.dependency_blob(['custom/a'], ['native/b']), expected)
        self.assertEqual(module.read_dependency_blob(expected), (['custom/a'], ['native/b']))
        self.assertEqual(module.dependency_blob(['custom/z', 'custom/a'], ['native/z', 'native/b']),
                         module.dependency_blob(['custom/a', 'custom/z'], ['native/b', 'native/z']))
        maximum = 'c/' + 'a' * 510
        unicode_maximum = 'c/' + '\u00e9' * 255
        for path in (maximum, unicode_maximum):
            self.assertEqual(module.read_dependency_blob(module.dependency_blob([path], ['stock/a'])),
                             ([path], ['stock/a']))

    def test_dependency_codec_rejects_unsafe_ambiguous_or_unbounded_inventory(self):
        module = load_module(self)
        self.assertTrue(hasattr(module, 'dependency_blob'), 'Strict dependency serialization is missing')
        bad_paths = ['', '.', '/absolute', '../escape', 'a/../escape', 'a//b', 'a/./b', 'a\\b',
                     'a\0b', 'a\nb', 'c/' + 'a' * 511, 'c/' + '\u00e9' * 256]
        invalid = [([], ['stock/a']), (['custom/a'], []),
                   (['custom/a', 'custom/a'], ['stock/a']), (['custom/a'], ['stock/a', 'stock/a']),
                   (['same/a'], ['same/a']),
                   ([f'custom/{i}' for i in range(4097)], ['stock/a']),
                   (['custom/a'], [f'stock/{i}' for i in range(4097)])]
        invalid += [([path], ['stock/a']) for path in bad_paths]
        for core, stock in invalid:
            with self.subTest(core=core[:2], stock=stock[:2]):
                with self.assertRaises(ValueError):
                    module.dependency_blob(core, stock)
                with self.assertRaises(ValueError):
                    module.read_dependency_blob(literal_blob(core, stock))
        valid = literal_blob(['custom/a'], ['stock/a'])
        invalid_blobs = [literal_blob(['custom/a'], ['stock/a'], version=2),
                         literal_blob([b'custom/\xff'], ['stock/a']), valid[:-1], valid + bytes(1),
                         valid[:64] + struct.pack('<I', 4) + b'\x01\x01\x00\x01',
                         b'\x01' + valid[1:]]
        for data in invalid_blobs:
            with self.subTest(data=data[-24:]), self.assertRaises(ValueError):
                module.read_dependency_blob(data)

    def test_bundle_validation_rejects_missing_or_incomplete_descriptor_before_writing(self):
        module = load_module(self)
        resources, receipt = module.build_resources(*fixture(module))
        name = PREFIX + 'dependencies_oot_v1'
        self.assertTrue(name in resources, 'Prepared archives need complete descriptors')
        core, stock = module.read_dependency_blob(resources[name])
        ancestor = PREFIX + 'geometry/Z2_LOST_WOODS_room_00DL_fixture_child'
        changes = [dict(resources, **{name: module.dependency_blob([path for path in core if path != ancestor], stock)}),
                   {path: data for path, data in resources.items() if path != name},
                   {path: data for path, data in resources.items() if path != ancestor},
                   dict(resources, **{name: resources[name][:-1]}),
                   dict(resources, **{name: module.dependency_blob(core, [path for path in stock if path != BARK])})]
        with tempfile.TemporaryDirectory(prefix='native-dependency-corruption-') as folder:
            for index, altered in enumerate(changes):
                output = Path(folder) / f'bad-{index}.zip'
                with self.subTest(index=index), self.assertRaises(ValueError):
                    module.validate_resources(altered)
                with self.subTest(index=index), self.assertRaisesRegex(ValueError, 'dependency|dependencies|Blob'):
                    module.write_bundle(altered, receipt, output)
                self.assertFalse(output.exists())


if __name__ == '__main__':
    unittest.main()
