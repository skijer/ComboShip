"""Ocarina HD texture binding preserves every non-texture display-list word."""
import struct
import unittest
from pathlib import Path
import numpy as np
from native_assets import o2r as asset
import native_npc_pose_fit as fit
from verify_native_npc_models import Archive

class PropTextureTests(unittest.TestCase):
    def test_hd_ocarina_raw_upload_has_rgba32_pixels_and_native_byte_stride(self):
        # Native DL: RGBA16 32x16, 64 bytes/row. The raw importer uploads
        # four-byte pixels, so an 8x pixel remaster needs 16x byte scaling.
        art = Path(fit.__file__).with_name('prop_art')/'saria-fairy-ocarina.png'
        raw = fit.ocarina_hd_resource(art)
        kind, width, height, flags, horizontal, vertical, length = struct.unpack_from('<IIIIffI', raw, 64)
        self.assertEqual((kind, width, height, flags), (1, 256, 128, 1))
        self.assertEqual((horizontal, vertical), (16., 8.))
        self.assertEqual(64*horizontal, width*4)
        self.assertEqual(16*vertical, height)
        self.assertEqual(1024*horizontal*vertical, length)
        self.assertEqual(length, width*height*4)
        from PIL import Image
        self.assertEqual(raw[92:], np.asarray(Image.open(art).convert('RGBA'), np.uint8).tobytes())

    def test_rebinding_changes_only_matching_texture_reference_hash(self):
        self.assertTrue(callable(getattr(fit,'bind_ocarina_texture',None)),'Private prop texture binding is missing')
        d=asset.DLBuilder();d.marker('prop');d.settimg_hash(0,2,1,'native/ocarina');d.end();raw=d.to_resource()
        changed=fit.bind_ocarina_texture(raw,'native/ocarina','private/hd')
        h=asset.crc64('private/hd');expected=bytearray(raw);struct.pack_into('<II',expected,96,h>>32,h&0xffffffff)
        self.assertEqual(changed,bytes(expected))
        with self.assertRaisesRegex(ValueError,'one texture'):
            fit.bind_ocarina_texture(raw,'wrong/reference','private/hd')

    def test_texture_scale_uses_resource_metadata_instead_of_filename(self):
        self.assertTrue(callable(getattr(Archive,'texture_scale',None)),'Texture scale metadata reader is missing')
        a=Archive.__new__(Archive)
        raw=asset._header(asset.T_OTEX,1)+struct.pack('<IIIIffI',2,256,128,1,8,8,65536)+bytes(65536)
        a.read=lambda path:raw
        self.assertEqual(a.texture_scale('any/path'),(8,8))
        raw=asset.build_texture_rgba16(bytes(1024),32,16)
        self.assertEqual(a.texture_scale('any/path'),(1,1))
