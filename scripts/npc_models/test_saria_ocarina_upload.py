"""The raw upload must retain the reviewed N64 colors and exact native stride."""
import struct
import unittest
import numpy as np
from native_assets import o2r as asset
from native_assets.n64texture import decode
import importlib
import importlib.util


class OcarinaUploadTests(unittest.TestCase):
    def test_repack_keeps_decoded_native_colors_and_fills_the_raw_upload(self):
        self.assertIsNotNone(importlib.util.find_spec('repair_saria_ocarina_upload'),
                             'The reviewed RGBA16 prop is still sent to the RGBA32 raw importer')
        repair = importlib.import_module('repair_saria_ocarina_upload')
        pixels = np.arange(256*128, dtype=np.uint16).astype('>u2').tobytes()
        old = asset._header(asset.T_OTEX, 1)+struct.pack('<IIIIffI', 2,256,128,1,8.,8.,len(pixels))+pixels
        new = repair.repack_texture(old)
        kind,w,h,flags,hs,vs,size = struct.unpack_from('<IIIIffI', new,64)
        self.assertEqual((kind,w,h,flags,hs,vs,size), (1,256,128,1,16.,8.,131072))
        self.assertEqual(new[92:], decode(pixels,0,2,256,128).tobytes())
        self.assertEqual(32*2*hs,4*w)
        self.assertEqual(32*16*2*hs*vs,size)
        self.assertEqual(repair.repack_texture(new),new)

    def test_unknown_texture_contract_is_rejected(self):
        self.assertIsNotNone(importlib.util.find_spec('repair_saria_ocarina_upload'))
        repair = importlib.import_module('repair_saria_ocarina_upload')
        raw = asset._header(asset.T_OTEX,1)+struct.pack('<IIIIffI',2,128,128,1,8.,8.,32768)+bytes(32768)
        with self.assertRaisesRegex(ValueError,'reviewed'):
            repair.repack_texture(raw)


if __name__ == '__main__':
    unittest.main()
