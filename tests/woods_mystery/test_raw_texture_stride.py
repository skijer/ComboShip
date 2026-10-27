"""Raw RGBA32 replacements must match the native RGBA16 loader's byte stride."""
from pathlib import Path
import struct
import sys
import zipfile

tiles = {'grass': (32, 32), 'rock': (32, 32), 'vines': (32, 32),
         'ruins': (32, 64), 'black': (4, 1),
         'henriko_canopy': (32, 32), 'henriko_forest_edge': (32, 16)}
with zipfile.ZipFile(Path(sys.argv[1])) as archive:
    for role, (tile_width, tile_height) in tiles.items():
        data = archive.read('alt/custom/cor_woods_mystery/poc1/textures/' + role)
        assert struct.unpack_from('<I', data, 8)[0] == 1
        fmt, width, height, flags, h_scale, v_scale, size = struct.unpack_from('<IIIIffI', data, 64)
        assert fmt == 2 and flags & 1, 'Expected RGBA16 resource with raw RGBA32 payload'
        assert size == width * height * 4 == len(data) - 92
        # ImportTextureRaw multiplies the native RGBA16 byte stride by HByteScale.
        # The result must span a complete RGBA32 image row, including alpha.
        native_row_bytes=((tile_width * 2 + 7)//8)*8
        assert native_row_bytes * h_scale == width * 4, f'{role}: raw image row is truncated'
        assert tile_height * v_scale == height, f'{role}: raw image height is truncated'
print('PASS: seven raw RGBA32 resources match native loader row strides, heights and payload sizes')
