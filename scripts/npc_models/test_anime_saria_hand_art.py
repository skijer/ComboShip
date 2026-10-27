"""Hand-paint POC safety: imagegen recolor rejection and native UV ownership."""
import json
import os
import struct
from pathlib import Path
import tempfile
import unittest
import zipfile

import numpy as np

import repair_anime_saria_hand_art as paint


class HandPaintUnitTests(unittest.TestCase):
    def test_native_linear_filter_footprint_protects_the_real_nail_witness(self):
        raw_st = np.array([[[9037,1565],[8995,1620],[9018,1617]]], float)
        support = paint.sampling_support(raw_st/16384, 4096, 4096, padding=3)
        self.assertTrue(support[394,2262], 'Native linear filtering reaches this protected nail texel')

    def test_big_endian_display_list_cannot_hide_a_shared_atlas_reader(self):
        dl = paint.o2r.DLBuilder()
        dl.settimg_hash(0, 3, 512, paint.ATLAS); dl.end()
        raw = bytearray(dl.to_resource())
        raw[0] = 1; raw[4:8] = b'ODLT'
        for at in range(72, len(raw), 4):
            value = struct.unpack_from('<I', raw, at)[0]
            struct.pack_into('>I', raw, at, value)
        with self.assertRaisesRegex(ValueError, 'Unsupported display-list reader'):
            paint.archive_ownership({'unexpected/reader': bytes(raw)}, 16, 16)

    def test_uniform_generator_brightness_shift_is_discarded(self):
        base = np.full((200, 200, 4), [140, 110, 80, 255], np.uint8)
        authored = base.copy(); authored[..., :3] += 50
        lift, offset = paint.normalized_lift(base, authored, np.ones((200, 200), bool))
        self.assertAlmostEqual(offset, 50.)
        self.assertFalse(lift.any())

    def test_local_shadow_reduction_survives_but_new_dark_lines_do_not(self):
        base = np.full((200, 200, 4), [140, 110, 80, 255], np.uint8)
        authored = base.copy(); authored[..., :3] += 50
        authored[60:80, 60:80, :3] += 20
        authored[100:120, 100:120, :3] -= 20
        lift, offset = paint.normalized_lift(base, authored, np.ones((200, 200), bool))
        self.assertAlmostEqual(offset, 50.)
        self.assertAlmostEqual(float(lift[70, 70]), 20.)
        self.assertEqual(float(lift[110, 110]), 0.)

    def test_bilinear_protection_includes_subpixel_and_collapsed_uv_triangles(self):
        uv = np.array([[[.501, .501], [.502, .501], [.501, .502]],
                       [[.25, .25], [.25, .25], [.25, .25]]])
        protected = paint.raster_triangles(uv, 100, 100, padding=2)
        self.assertTrue(protected[49:52, 49:52].all())
        self.assertTrue(protected[24:27, 24:27].all())
        self.assertFalse(protected[10, 10])

    def test_output_and_report_aliases_reject_before_any_input_is_parsed(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            files = [root/name for name in ('parent', 'source', 'art', 'native', 'report')]
            for file in files:
                file.write_bytes(b'input must remain exact')
            parent, source, art, native, report = files
            for protected in files:
                alias = root/'output'; alias.hardlink_to(protected)
                with self.assertRaisesRegex(ValueError, 'separate'):
                    paint.build(parent, source, art, alias, report, native)
                self.assertEqual(protected.read_bytes(), b'input must remain exact')
                alias.unlink()
            report_alias = root/'report-alias'; report_alias.symlink_to(art)
            with self.assertRaisesRegex(ValueError, 'separate'):
                paint.build(parent, source, art, root/'new-output', report_alias, native)
            self.assertEqual(art.read_bytes(), b'input must remain exact')


class ExactArchiveTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        keys = ('SARIA_HAND_PAINT_PARENT', 'SARIA_HAND_PAINT_SOURCE', 'SARIA_HAND_PAINT_ART')
        if not all(os.environ.get(key) for key in keys):
            raise unittest.SkipTest('Set SARIA_HAND_PAINT_PARENT, _SOURCE and _ART for exact source checks')
        cls.parent, cls.source, cls.art = [Path(os.environ[key]) for key in keys]
        with zipfile.ZipFile(cls.parent) as archive:
            cls.records = {p: archive.read(p) for p in archive.namelist()}
        cls.result, cls.report, cls.masks = paint.repair_records(cls.records, cls.source, cls.art)

    def test_changes_one_existing_texture_and_no_geometry_or_prior_repair_bytes(self):
        self.assertEqual(set(self.records), set(self.result))
        changed = [p for p in self.records if self.records[p] != self.result[p]]
        self.assertEqual(changed, [paint.ATLAS])
        _, w, h, before = paint.o2r.read_texture(self.records[paint.ATLAS])
        _, _, _, after = paint.o2r.read_texture(self.result[paint.ATLAS])
        self.assertEqual(self.records[paint.ATLAS][:-len(before)], self.result[paint.ATLAS][:-len(after)])
        old, new = [np.frombuffer(raw, np.uint8).reshape(h, w, 4) for raw in (before, after)]
        self.assertTrue(np.array_equal(old[..., 3], new[..., 3]))
        self.assertTrue(np.array_equal(old[~self.masks['safe']], new[~self.masks['safe']]))
        delta = new[..., :3].astype(int)-old[..., :3].astype(int)
        self.assertGreater(self.report['changed_pixels'], 1000)
        self.assertGreaterEqual(int(delta.min()), 0)
        self.assertLessEqual(int(delta.max()), 24)

    def test_all_hand_routes_wrist_rims_nails_and_nonhand_bilinear_taps_are_protected(self):
        self.assertEqual(self.report['hand_readers'], 20)
        self.assertEqual(self.report['direct_atlas_mesh_readers'], 73)
        changed = self.masks['changed']
        for name in ('wrist', 'nails', 'nonhand'):
            self.assertFalse((changed & self.masks[name]).any(), name)
            self.assertGreater(int(self.masks[name].sum()), 100)
        self.assertFalse(changed[~self.masks['hand']].any())
        self.assertFalse(self.report['runtime_tested'])
        self.assertFalse(self.report['accepted'])

    def test_unknown_hand_uv_and_unreviewed_art_fail_closed(self):
        altered = dict(self.records)
        path = paint.NS+'/Body/Limb11/V'
        raw = bytearray(altered[path]); raw[80] ^= 1; altered[path] = bytes(raw)
        with self.assertRaisesRegex(ValueError, 'hand UV'):
            paint.archive_ownership(altered, 4096, 4096)
        with tempfile.TemporaryDirectory() as directory:
            art = Path(directory)/'art'; art.write_bytes(b'unknown image')
            with self.assertRaisesRegex(ValueError, 'reviewed, preserved'):
                paint.repair_records(self.records, self.source, art)

    def test_an_unrelated_reader_of_hand_uv_is_never_authorized_by_source_projection(self):
        altered = dict(self.records)
        altered[paint.NS+'/Body/UnexpectedAtlasReader'] = altered[paint.NS+'/Body/Limb11']
        safe, _, nonhand, _, _ = paint.archive_ownership(altered, 4096, 4096)
        left = paint._decode(self.records, paint.NS+'/Body/Limb11')
        footprint = paint.raster_triangles(paint.uv_triangles(left.triangles)/16384., 4096, 4096)
        self.assertTrue(footprint.any())
        self.assertFalse(safe[footprint].any())
        self.assertTrue(nonhand[footprint].all())


if __name__ == '__main__':
    unittest.main()
