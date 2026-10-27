#!/usr/bin/env python3
"""POC4 actual-resource, mutation, texture-mask and shared eye-cache regression tests."""

import argparse
import copy
from pathlib import Path
import struct
import sys
import tempfile
import unittest
import zipfile

import numpy as np

import build_tp_epona_cosmetics as b


def load(path):
    with zipfile.ZipFile(path) as archive:
        if archive.testzip() is not None:
            raise ValueError("ZIP CRC failure")
        if len(archive.namelist()) != len(set(archive.namelist())):
            raise ValueError("duplicate archive paths")
        return {p: archive.read(p) for p in archive.namelist()}


def triangles(data):
    cache = {}
    result = []
    for w0, w1, _ in b.commands(data):
        if w0 >> 24 == 1:
            count = (w0 >> 12) & 255
            destination = ((w0 >> 1) & 127) - count
            start = (w1 & 0x00FFFFFE) // 16
            cache.update({destination + i: start + i for i in range(count)})
        elif w0 >> 24 == 5:
            result.append(tuple(cache[((w0 >> s) & 255) // 2] for s in (16, 8, 0)))
    return result


class ActualPoc4Tests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not getattr(sys, "tp_source", None):
            raise unittest.SkipTest("provide --source --candidate --checkpoint for actual-resource tests")
        cls.original, cls.files = load(sys.tp_source), load(sys.tp_candidate)
        cls.h = b.helpers(sys.tp_checkpoint)
        cls.selected = b.source_selection(sys.tp_checkpoint, cls.h)

    def test_pinned_baseline_and_complete_structural_contract(self):
        self.assertEqual(b.sha(Path(sys.tp_source).read_bytes()), b.BASE_SHA)
        report = b.validate(self.original, self.files, self.h)
        self.assertEqual(report["eye_cache_vertices"], 20)
        self.assertEqual(report["body_buffer_vertices"], 5912)

    def test_original_textures_display_lists_and_animations_remain_exact(self):
        changed = {p for p in self.original if self.original[p] != self.files[p]}
        self.assertEqual(changed, {b.BODY_LIMB, b.HEAD_LIMB})
        self.assertEqual(len(self.files), 124)
        self.assertEqual(sum(p.endswith("Anim") for p in self.original), 9)

    def test_all_skin_records_uv_normals_weights_and_bone_fields_remain_exact(self):
        for name, field in ((b.BODY_LIMB, "skin_display_list2"), (b.HEAD_LIMB, "skin_display_list")):
            original = self.h["native_roundtrip"].decode_resource(self.original[name])
            actual = self.h["native_roundtrip"].decode_resource(self.files[name])
            actual[field] = original[field]
            self.assertEqual(actual, original)

    def test_default_command_stream_is_original_including_triangle_winding(self):
        for name in ("BodyDL", "EyesDL"):
            original = [raw for a, _, raw in b.commands(self.original[b.OLD + name]) if a >> 24 != 0x33]
            actual = [raw for a, _, raw in b.commands(self.files[b.NEW + name]) if a >> 24 not in (0x33, 0xDE)]
            self.assertEqual(actual, original)

    def test_coat_and_hair_overlays_reuse_exact_selected_original_triangles(self):
        body = triangles(self.original[b.OLD + "BodyDL"])
        expected_coat = [body[i] for i in sorted(self.selected[3])]
        expected_hair = expected_coat + [body[1771 + i] for i in sorted(self.selected[2])] + [
            body[1860 + i] for i in sorted(self.selected[4])]
        self.assertEqual(triangles(self.files[b.NEW + "CoatOverlayDL"]), expected_coat)
        self.assertEqual(triangles(self.files[b.NEW + "HairOverlayDL"]), expected_hair)
        self.assertEqual((len(expected_coat), len(expected_hair)), (1164, 1309))

    def test_overlay_source_rgb_dimensions_and_raw_texture_scales_are_unchanged(self):
        original = self.original[b.OLD + "HorsSS00Tex"]
        _, w, h, raw = self.h["o2r"].read_texture(original)
        source = np.frombuffer(raw, np.uint8).reshape(h, w, 4)
        alpha = []
        for name in ("CoatMaskTex", "WhiteHairMaskTex"):
            texture = self.files[b.NEW + name]
            typ, width, height, payload = self.h["o2r"].read_texture(texture)
            self.assertEqual((typ, width, height), (1, 1254, 1254))
            self.assertEqual(texture[:-len(payload)], original[:-len(raw)])
            image = np.frombuffer(payload, np.uint8).reshape(h, w, 4)
            np.testing.assert_array_equal(image[:, :, :3], source[:, :, :3])
            alpha.append(image[:, :, 3])
        self.assertTrue(np.all(alpha[0].astype(int) + alpha[1].astype(int) <= 255))
        # Actual emitted texels: coat, head hair, hoof feathering, and tack.
        self.assertGreater(int(alpha[0][300, 250]), 250)
        self.assertGreater(int(alpha[1][250, 820]), 240)
        self.assertGreater(int(alpha[1][1100, 480]), 240)
        protected = b.polygon_mask(b.PROTECTED_TACK, (1254, 1254)).astype(bool)
        for channel in alpha:
            self.assertFalse(np.any(channel[protected]))
            for x, y in b.TACK_PROBES:
                self.assertEqual(int(channel[y, x]), 0)

    def test_eye_preload_remains_last_and_blink_never_inherits_iris_tint(self):
        body = list(b.commands(self.files[b.NEW + "BodyDL"]))
        self.assertEqual([raw for _, _, raw in body[-4:]],
                         [b.segment(0xD), b.segment(0xE), b.EYE_PRELOAD, b.END])
        eyes = list(b.commands(self.files[b.NEW + "EyesDL"]))
        self.assertFalse(any(a >> 24 in (1, 2, 0x31, 0x32, 0xDB) for a, _, _ in eyes))
        self.assertEqual([(a, c) for a, c, _ in eyes if a >> 24 == 0xFD], [(0xFD100000, 0x08000001)])
        state = False
        counted = [0, 0]
        for a, _, raw in eyes:
            if raw == b.segment(0xB): state = True
            if raw == b.segment(0xC): state = False
            if a >> 24 == 5: counted[0 if state else 1] += 1
        self.assertEqual(counted, [6, 6])

    def test_regression_changing_one_skin_coordinate_is_rejected(self):
        corrupted = self.files.copy()
        limb = self.h["native_roundtrip"].decode_resource(corrupted[b.BODY_LIMB])
        limb["modifications"][0]["transforms"][0][1] += 1
        corrupted[b.BODY_LIMB] = self.h["native_roundtrip"].encode_resource(limb)
        with self.assertRaisesRegex(ValueError, "non-path skin"):
            b.validate(self.original, corrupted, self.h)

    def test_regression_eye_cache_preload_loss_is_rejected(self):
        corrupted = self.files.copy()
        blob = corrupted[b.NEW + "BodyDL"]
        corrupted[b.NEW + "BodyDL"] = blob[:-16] + b.END
        with self.assertRaises(ValueError):
            b.validate(self.original, corrupted, self.h)

    def test_regression_eye_overlay_call_that_could_clobber_cache_is_rejected(self):
        corrupted = self.files.copy()
        path = b.NEW + "EyesDL"
        corrupted[path] = corrupted[path][:-8] + b.segment(0xD) + b.END
        with self.assertRaisesRegex(ValueError, "state-only iris"):
            b.validate(self.original, corrupted, self.h)

    def test_regression_tint_leaking_to_closed_eyelid_is_rejected(self):
        corrupted = self.files.copy()
        path = b.NEW + "EyesDL"
        corrupted[path] = corrupted[path].replace(b.segment(0xC), b"")[:-8] + b.segment(0xC) + b.END
        with self.assertRaisesRegex(ValueError, "iris color affects blink"):
            b.validate(self.original, corrupted, self.h)

    def test_regression_out_of_bounds_overlay_vertex_load_is_rejected(self):
        corrupted = self.files.copy()
        path = b.NEW + "CoatOverlayDL"
        blob = bytearray(corrupted[path])
        offset = 72
        for a, c, raw in b.commands(bytes(blob)):
            if a >> 24 == 1:
                struct.pack_into("<I", blob, offset + 4, 0x08000001 + 5910 * 16)
                break
            offset += len(raw)
        corrupted[path] = bytes(blob)
        with self.assertRaisesRegex(ValueError, "overrun"):
            b.validate(self.original, corrupted, self.h)

    def test_archive_rebuild_is_deterministic_and_crc_clean(self):
        files, _, _ = b.build_files(self.original, sys.tp_checkpoint, self.h)
        self.assertEqual(files, self.files)
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "rebuilt.o2r"
            b.write_archive(output, files)
            self.assertEqual(output.read_bytes(), Path(sys.tp_candidate).read_bytes())
            self.assertEqual(load(output), files)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--candidate", required=True, type=Path)
    parser.add_argument("--checkpoint", required=True, type=Path)
    args, remaining = parser.parse_known_args()
    sys.tp_source, sys.tp_candidate, sys.tp_checkpoint = args.source, args.candidate, args.checkpoint
    unittest.main(argv=[sys.argv[0], *remaining])
