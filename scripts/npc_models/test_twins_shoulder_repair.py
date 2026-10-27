"""The twins shoulder bake must cover a native 512-pixel tile exactly once."""
import os
import struct
import unittest
import zipfile

import numpy as np

from native_assets import o2r as asset
from repair_twins_shoulders import repair_texture_resource, TEXTURES


def texture_resource(pixels, scale):
    h, w = pixels.shape[:2]
    return (asset._header(asset.T_OTEX, 1) +
            struct.pack('<IIIIffI', 1, w, h, 1, *scale, pixels.nbytes) + pixels.tobytes())


def native_vertical_samples(raw):
    """GfxSpTri divides ST by loaded height, capped at the render-tile height.

    For an undersized image, the GPU wraps before reaching the tile's 512-pixel
    shader clamp. Oversized atlases instead use the 512-pixel denominator.
    """
    _, w, h, pixels = asset.read_texture(raw)
    image = np.frombuffer(pixels, np.uint8).reshape(h, w, 4)
    st = np.linspace(.01, .99, 99)*512
    denominator = min(h, 512)
    y = np.floor((st/denominator % 1)*h).astype(int)
    return image[y, 0, 0].astype(int)


class TwinsShoulderBakeTests(unittest.TestCase):
    def setUp(self):
        self.image = np.zeros((128, 256, 4), np.uint8)
        self.image[:, :, 0] = np.arange(128)[:, None]
        self.image[:, :, 1] = np.arange(256)[None, :]
        self.image[:, :, 3] = 255
        self.raw = texture_resource(self.image, (.5, .25))

    def test_native_gradient_has_no_three_internal_wrap_resets(self):
        before = native_vertical_samples(self.raw)
        self.assertEqual(int((np.diff(before) < 0).sum()), 3,
                         'The control must reproduce four repeats of the shoulder tone')
        after = native_vertical_samples(repair_texture_resource(self.raw))
        self.assertEqual(int((np.diff(after) < 0).sum()), 0,
                         'The native 512 tile must show the shoulder gradient only once')

    def test_expansion_preserves_every_authored_pixel_exactly(self):
        fixed = repair_texture_resource(self.raw)
        kind, w, h, pixels = asset.read_texture(fixed)
        self.assertEqual((kind, w, h), (1, 512, 512))
        self.assertEqual(struct.unpack_from('<ff', fixed, 80), (1., 1.))
        image = np.frombuffer(pixels, np.uint8).reshape(h, w, 4)
        np.testing.assert_array_equal(image, np.repeat(np.repeat(self.image, 4, axis=0), 2, axis=1))

    def test_unreviewed_texture_layout_is_rejected(self):
        with self.assertRaisesRegex(ValueError, '256.*128'):
            repair_texture_resource(texture_resource(self.image[:64], (.5, .125)))


class ExportedTwinsShoulderBakeTests(unittest.TestCase):
    @unittest.skipUnless(os.environ.get('TWINS_SHOULDER_ARCHIVE'), 'Set TWINS_SHOULDER_ARCHIVE for exported-resource proof')
    def test_exported_shoulder_bakes_cover_native_tile_exactly_once(self):
        with zipfile.ZipFile(os.environ['TWINS_SHOULDER_ARCHIVE']) as archive:
            for name in TEXTURES:
                with self.subTest(resource=name):
                    _, w, h, _ = asset.read_texture(archive.read(name))
                    self.assertEqual((512/min(w, 512), 512/min(h, 512)), (1., 1.),
                                     'Native cap ST repeats inside the rendered shoulder tile')


if __name__ == '__main__':
    unittest.main()
