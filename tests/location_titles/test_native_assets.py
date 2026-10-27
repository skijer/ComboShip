#!/usr/bin/env python3
"""Real built-in artwork, default preparation and smooth HD geometry parity."""
from pathlib import Path
import base64
import importlib.util
import io
import json
import shutil
import struct
import sys
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[2]
MODULE = ROOT / 'tools/location_titles/native_assets.py'
PREFIX = 'custom/oot_location_titles/v1/'
sys.path.insert(0, str(ROOT / 'tools/location_titles'))


def load_module(path):
    spec = importlib.util.spec_from_file_location(path.stem, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class NativeAssets(unittest.TestCase):
    def assets(self):
        self.assertTrue(MODULE.is_file(), 'native OoT title artwork must be built in, not optional-only')
        return load_module(MODULE)

    def test_every_english_scene_title_is_built_in(self):
        module = self.assets()
        resources = module.resources()
        coverage = json.loads((ROOT / 'tools/location_titles/assets/native-v3/manifest.json').read_text())['titles']
        self.assertEqual(len(coverage), 67)
        self.assertEqual(len(resources), 69)
        self.assertEqual(set(resources), {PREFIX + '1/' + row['id'] for row in coverage} |
                         {PREFIX + 'market', PREFIX + 'lost_woods'})
        for row in coverage:
            data = resources[PREFIX + '1/' + row['id']]
            self.assertEqual(struct.unpack_from('<IIII', data, 64), (8, 240, 24, 5760))
        names = {row['text'] for row in coverage}
        self.assertTrue({'East Clock Town', 'North Clock Town', "Honey & Darling's Shop", 'Post Office',
                         'Astral Observatory', 'Oceanside Spider House'} <= names)

    def test_optional_pack_uses_smooth_hd_art_with_native_resource_geometry(self):
        module = self.assets()
        native, hd = module.resources(), module.hd_resources()
        self.assertEqual(set(native), set(hd))
        for name, data in native.items():
            kind, width, height, size = struct.unpack_from('<IIII', data, 64)
            self.assertEqual((kind, size, len(data)), (8, width * height, 80 + width * height))
            raw = data[80:]
            self.assertTrue(any(v >> 4 and v & 15 for v in raw), name)
            self.assertTrue(any(not v >> 4 and v & 15 for v in raw), name + ' baked black halo')
            if not name.endswith('/lost_woods'):
                self.assertFalse(any(v & 15 for v in raw[:width] + raw[-width:] + raw[::width] + raw[width - 1::width]), name)
            out = hd[name]
            self.assertEqual(struct.unpack_from('<IIIIffI', out, 64),
                             (1, width * 4, height * 4, 1, 16.0, 4.0, width * height * 64))
            pixels = out[92:]
            if not name.endswith('/lost_woods'):
                alpha = pixels[3::4]
                physical_width = width * 4
                self.assertFalse(any(alpha[:physical_width] + alpha[-physical_width:] +
                                     alpha[::physical_width] + alpha[physical_width - 1::physical_width]),
                                 name + ' HD must preserve the native transparent perimeter')
            self.assertGreater(len(set(pixels[3::4])), 32, name + ' needs smooth 8-bit HD alpha')
            varying_blocks = 0
            for y in range(0, height * 4, 4):
                for x in range(0, width * 4, 4):
                    block = {pixels[((y + dy) * width * 4 + x + dx) * 4 + 3]
                             for dy in range(4) for dx in range(4)}
                    varying_blocks += len(block) > 1
            self.assertGreater(varying_blocks, 20, name + ' cannot just repeat native pixels')

    def test_corrupt_encoded_source_is_rejected(self):
        module = self.assets()
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            shutil.copytree(module.ASSETS, root / 'assets')
            p = root / 'assets/cards.zlib.b64'
            p.write_bytes(p.read_bytes() + b'!')
            with self.assertRaises(ValueError):
                module.resources(root / 'assets')

    def test_hd_preserves_native_letter_contours(self):
        import hd_assets
        metrics = json.loads((self.assets().ASSETS / 'manifest.json').read_text())['glyph_layout']
        for ch, (face, _, _) in hd_assets.glyphs().items():
            native = base64.b64decode(metrics[ch]['face_coverage_b64'], validate=True)
            reduced = [sum(face[(y * 4 + dy) * 96 + x * 4 + dx] for dy in range(4) for dx in range(4)) / 16
                       for y in range(24) for x in range(24)]
            # Compare fractional coverage, not thresholded pixels: 119/136
            # native edge values straddle 128 without defining a hard edge.
            error = sum(abs(value - reduced[i]) for i, value in enumerate(native)) / sum(native)
            self.assertLessEqual(error, 0.30, repr(ch) + ' HD outline must preserve native letter shape')

        module = self.assets()
        native = module.resources()[PREFIX + 'lost_woods'][80:]
        rgba = module.hd_resources()[PREFIX + 'lost_woods'][92:]
        face = [rgba[i] * rgba[i + 3] / 255 for i in range(0, len(rgba), 4)]
        source = [(value >> 4) * (value & 15) * 255 / 225 for value in native]
        reduced = [sum(face[(y * 4 + dy) * 576 + x * 4 + dx] for dy in range(4) for dx in range(4)) / 16
                   for y in range(24) for x in range(144)]
        error = sum(abs(value - reduced[i]) for i, value in enumerate(source)) / sum(source)
        self.assertLessEqual(error, 0.25, 'HD Lost Woods must preserve the original whole-word layout and shapes')

    def test_hd_letters_do_not_gain_detached_neighbor_strokes(self):
        import hd_assets

        def components(plane, width, threshold=32):
            pending = {index for index, value in enumerate(plane) if value >= threshold}
            count = 0
            while pending:
                count += 1
                seed = pending.pop()
                todo = [seed]
                while todo:
                    index = todo.pop()
                    x, y = index % width, index // width
                    neighbors = {yy * width + xx
                                 for yy in range(max(0, y - 1), min(len(plane) // width, y + 2))
                                 for xx in range(max(0, x - 1), min(width, x + 2))}
                    connected = neighbors & pending
                    pending -= connected
                    todo.extend(connected)
            return count

        glyphs = hd_assets.glyphs()
        # These letters have one face. Adjacent donor-letter slices and a
        # thresholded E top bar previously became detached white strokes.
        for ch in sorted(set(glyphs) - set('ij')):
            with self.subTest(letter=ch):
                self.assertEqual(components(glyphs[ch][0], 96), 1,
                                 repr(ch) + ' has a detached white artifact')
        # A global "keep largest component" cleanup would erase real dots.
        for ch in 'ij':
            self.assertEqual(components(glyphs[ch][0], 96), 2,
                             repr(ch) + ' must retain its separate native dot')

    def test_native_letters_keep_every_sample_except_audited_donor_neighbors(self):
        metrics = json.loads((self.assets().ASSETS / 'manifest.json').read_text())['glyph_layout']
        corrections = json.loads((ROOT / 'tools/location_titles/assets/native-v3/glyph-isolation.json')
                                 .read_text())['glyphs']
        # The immutable earlier glyph archive retains the native donor crops.
        # Compare every sample, including faint antialiasing and i's dot; a
        # blanket component/threshold cleanup would fail this preservation check.
        legacy = load_module(ROOT / 'tools/location_titles/assets/native-oot-v2/build_pack.py')
        packed, manifest = legacy.build()
        with zipfile.ZipFile(io.BytesIO(packed)) as archive:
            for ch, metric in manifest['native_metrics'].items():
                with self.subTest(letter=ch):
                    alpha = archive.read(f'custom/mm_location_titles/oot_native_v1/{ord(ch):02x}')[95::4]
                    start, _, end, _ = metric['crop']
                    expected = bytearray(576)
                    for y in range(24):
                        for x in range(end - start):
                            expected[y * 24 + x + 3] = alpha[y * 4 * 96 + (x + 1) * 4]
                    for x, y, value in corrections.get(ch, {}).get('removed_neighbor_pixels', []):
                        index = y * 24 + x - start + 3
                        self.assertEqual(expected[index], value)
                        expected[index] = 0
                    face = base64.b64decode(metrics[ch]['face_coverage_b64'], validate=True)
                    self.assertEqual(face, expected, repr(ch) + ' changed an unlisted native face sample')
                    self.assertEqual(metrics[ch]['advance'], metric['advance'])

    def test_market_wordmark_closes_the_gap_before_the_dotted_j(self):
        module = self.assets()
        native, hd = module.resources(), module.hd_resources()
        key = PREFIX + 'market'
        for label, record, expected in [('native', native[key], [(57, 65), (66, 70)]),
                                        ('HD', hd[key], [(228, 259), (262, 280)])]:
            with self.subTest(art=label):
                kind, width, height = struct.unpack_from('<III', record, 64)
                if kind == 8:
                    face = [(value >> 4) * (value & 15) * 255 / 225 for value in record[80:]]
                else:
                    pixels = record[92:]
                    face = [pixels[i] * pixels[i + 3] / 255 for i in range(0, len(pixels), 4)]
                columns = [x for x in range(width) if any(face[y * width + x] >= 128 for y in range(height))]
                runs = []
                for x in columns:
                    if runs and runs[-1][1] == x:
                        runs[-1] = (runs[-1][0], x + 1)
                    else:
                        runs.append((x, x + 1))
                # Actual packed face extents of the first e and following j in
                # "Skiejer's". Keep its spelling; tighten this shop title only.
                self.assertEqual(runs[3:5], expected)

    def test_lowercase_pairs_keep_native_white_face_spacing(self):
        module = self.assets()
        native, hd = module.resources(), module.hd_resources()

        def face_runs(record, rows, scale):
            kind, width, height = struct.unpack_from('<III', record, 64)
            if kind == 8:
                face = [(value >> 4) * (value & 15) * 255 / 225 for value in record[80:]]
            else:
                pixels = record[92:]
                face = [pixels[i] * pixels[i + 3] / 255 for i in range(0, len(pixels), 4)]
            columns = [x for x in range(width)
                       if any(face[y * width + x] >= 128 for y in range(rows[0] * scale, rows[1] * scale))]
            runs = []
            for x in columns:
                if runs and runs[-1][1] == x:
                    runs[-1] = (runs[-1][0], x + 1)
                else:
                    runs.append((x, x + 1))
            return runs

        # Measured from actual packed faces, excluding the black halo. The
        # f hook sits above a: compare its stems on shared lower rows. y's
        # donor crop has a three-pixel leading bearing; native Back Alley
        # leaves one white-face pixel between e and y, not four.
        # Run indices differ where native M splits but its HD contour joins.
        cases = [
            ('0117', 'fa', (12, 17), (7, 3), (7, 12)),       # Woodfall
            ('0139', 'ay', (8, 18), (6, 1), (6, 6)),        # Great Bay
            ('0108', 'ey', (8, 18), (3, 1), (3, 5)),        # Honey & Darling's Shop
            ('0130', 'gy', (8, 18), (3, 2), (3, 8)),        # Doggy Racetrack
            ('0127', 'hy', (8, 18), (13, 2), (12, 10)),     # Mountain Smithy
            ('0141', 'ny', (8, 18), (7, 2), (7, 8)),        # Ikana Canyon
            ('0105', 'ry', (8, 18), (5, 1), (5, 4)),        # Laundry Pool
            ('010e', 'ty', (8, 18), (7, 2), (7, 8)),        # Curiosity Shop
        ]
        for title_id, pair, rows, native_measure, hd_measure in cases:
            for art, resources, scale, (index, expected) in [
                    ('native', native, 1, native_measure), ('HD', hd, 4, hd_measure)]:
                with self.subTest(title=title_id, pair=pair, art=art):
                    runs = face_runs(resources[PREFIX + '1/' + title_id], rows, scale)
                    self.assertEqual(runs[index + 1][0] - runs[index][1], expected,
                                     pair + ' has excess spacing between its visible faces')

    def test_default_preparation_repairs_both_hosts_without_mods(self):
        module = self.assets()
        prepare = load_module(ROOT / 'scripts/assets/prepare_native_titles.py')
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            target = root / 'mm/assets/custom' / (PREFIX + '1/0104')
            target.parent.mkdir(parents=True)
            target.write_bytes(b'stale MM glyph candidate')
            self.assertEqual(prepare.prepare(root), 138)
            self.assertEqual(prepare.prepare(root), 0)
            resources = module.resources()
            for host in ('mm', 'soh'):
                for name, data in resources.items():
                    self.assertEqual((root / host / 'assets/custom' / name).read_bytes(), data)


if __name__ == '__main__':
    unittest.main()
