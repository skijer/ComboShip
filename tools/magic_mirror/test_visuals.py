"""Run with python -m unittest discover -s tools/magic_mirror -v."""
from pathlib import Path
import subprocess
import tempfile
import unittest
import hashlib
import json
import struct
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]


class MagicMirrorVisualTests(unittest.TestCase):
    def test_hd_reflection_scale_matches_double_width_native_tile(self):
        from build_forest_addon import texture
        raw = texture(Image.new('RGBA', (512, 512), (60, 100, 40, 255)), 2)
        self.assertEqual(struct.unpack_from('<IIIIffI', raw, 64), (2, 512, 512, 1, 16., 16., 1048576))
        self.assertEqual(len(raw), 1048668)

    def test_private_forest_cast_resources(self):
        art = ROOT / 'tools/magic_mirror/cast'
        manifest = json.loads((art/'manifest.json').read_text())
        self.assertEqual(set(manifest), {'forest_seal', 'forest_wisp'})
        for name, metadata in manifest.items():
            with self.subTest(texture=name):
                self.assertEqual(metadata['source_sha256'], hashlib.sha256((art/(name+'.png')).read_bytes()).hexdigest())
                paths = [ROOT/host/'assets/custom/objects/magic_mirror'/name for host in ('soh', 'mm')]
                raw = paths[0].read_bytes()
                self.assertEqual(raw, paths[1].read_bytes())
                self.assertEqual(hashlib.sha256(raw).hexdigest(), metadata['resource_sha256'])
                self.assertEqual(struct.unpack_from('<IIIIffI', raw, 64), (1, 512, 512, 1, 16., 16., 1048576))
                self.assertEqual(len(raw), 1048668)
                alpha = raw[95::4]
                self.assertEqual(min(alpha), 0)
                self.assertGreater(max(alpha), 200)
                self.assertGreater(sum(a == 0 for a in alpha), len(alpha)//4)
                self.assertTrue(all(a == 0 for a in alpha[:512] + alpha[-512:]))

    def test_real_forest_reflection_native_parity(self):
        directory = ROOT / 'tools/magic_mirror/reflection'
        receipt = json.loads((directory/'forest-scene.json').read_text())
        self.assertNotIn('source_library_file_id', receipt)
        self.assertNotIn('source_name', receipt)
        self.assertEqual(receipt['output_sha256'], hashlib.sha256((directory/'forest-scene.png').read_bytes()).hexdigest())
        self.assertEqual(receipt['saria']['sprite_sha256'], hashlib.sha256((directory/'saria-silhouette.png').read_bytes()).hexdigest())
        paths = [ROOT/game/'assets/custom/textures/magic_mirror/gMagicMirrorForestReflectionTex.rgb5a1.png'
                 for game in ('soh','mm')]
        self.assertEqual(paths[0].read_bytes(),paths[1].read_bytes())
        image = Image.open(paths[0]).convert('RGBA')
        self.assertEqual(image.size,(64,32))
        self.assertLessEqual(image.width * image.height * 2, 4096)
        self.assertEqual(image.getchannel('A').getextrema(),(255,255))

    def test_shared_mesh_behavior(self):
        header = ROOT / "combo/gameplay/MagicMirrorGeometry.h"
        self.assertTrue(header.exists(), "the shared native mirror mesh is missing")
        with tempfile.TemporaryDirectory(prefix="magic-mirror-test-") as temp:
            exe = Path(temp) / "geometry-test"
            subprocess.run(["c++", "-std=c++17", "-Wall", "-Wextra", "-Werror", "-pedantic",
                            str(Path(__file__).with_name("test_geometry.cpp")), "-o", str(exe)], check=True)
            subprocess.run([str(exe)], check=True)

    def test_icon_is_identical_in_both_games(self):
        paths = [ROOT / game / "assets/custom/textures/icon_item_custom/gItemIconMagicMirrorTex.rgba32.png"
                 for game in ("soh", "mm")]
        self.assertTrue(all(p.exists() for p in paths), "both native inventory icons must exist")
        self.assertEqual(paths[0].read_bytes(), paths[1].read_bytes())
        image = Image.open(paths[0])
        self.assertEqual(image.size, (32, 32))
        self.assertEqual(image.mode, "RGBA")
        alpha = image.getchannel("A")
        self.assertEqual(alpha.getextrema(), (0, 255))
        self.assertGreater(sum(a > 0 for a in alpha.tobytes()), 200)
        self.assertLess(sum(a > 0 for a in alpha.tobytes()), 800)

    def test_name_is_native_ia4_in_both_games(self):
        paths = [ROOT / game / "assets/custom/textures/item_name_custom/gMagicMirrorNameTex.ia4.png"
                 for game in ("soh", "mm")]
        self.assertTrue(all(p.exists() for p in paths), "both native item names must exist")
        self.assertEqual(paths[0].read_bytes(), paths[1].read_bytes())
        image = Image.open(paths[0]).convert("RGBA")
        self.assertEqual(image.size, (128, 16))
        raw = image.tobytes()
        pixels = [tuple(raw[i:i+4]) for i in range(0, len(raw), 4)]
        self.assertLessEqual({p[3] for p in pixels}, {0, 255})
        visible = {(r, g, b) for r, g, b, a in pixels if a}
        self.assertTrue(visible)
        self.assertLessEqual(len(visible), 8)
        self.assertTrue(all(r == g == b for r, g, b in visible))
        bbox = image.getchannel("A").getbbox()
        self.assertGreaterEqual(bbox[0], 1)
        self.assertGreaterEqual(bbox[1], 1)
        self.assertLessEqual(bbox[2], 127)
        self.assertLessEqual(bbox[3], 15)


if __name__ == "__main__":
    unittest.main()
