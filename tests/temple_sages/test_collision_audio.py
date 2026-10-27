"""Check collision acoustics through serialized preservation and native readers."""
import importlib.util
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


def load_module():
    path = ROOT / 'tools/temple_sages_bundle/collision_audio.py'
    assert path.is_file(), 'walkable chamber collision needs the stone/echo correction'
    spec = importlib.util.spec_from_file_location('chamber_collision_audio', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def synthetic():
    header = bytearray(64)
    header[4:8] = b'LOCO'
    vertices = [(0, 0, 0), (100, 0, 0), (0, 0, 100)]
    polygons = [(0, 0x2000, 1, 2, 0, 32767, 0, 0),
                (0, 0, 1, 2, 32767, 0, 0, 0),
                (0, 0, 1, 2, 0, 16383, 0, 0),
                (0, 0, 1, 2, 0, 16384, 0, 0)]
    data1 = 0x8E6407F3 | (5 << 11)
    data0 = 0xA51234FF
    return (bytes(header) + struct.pack('<6hI', 0, 0, 0, 100, 0, 100, len(vertices)) +
            b''.join(struct.pack('<3h', *p) for p in vertices) + struct.pack('<I', len(polygons)) +
            b''.join(struct.pack('<4H4h', *p) for p in polygons) +
            struct.pack('<III', 1, data1, data0) + b'camera-and-water-payload')


def parts(data):
    vertex_count = struct.unpack_from('<I', data, 76)[0]
    poly_count_offset = 80 + vertex_count * 6
    poly_count = struct.unpack_from('<I', data, poly_count_offset)[0]
    poly_offset = poly_count_offset + 4
    surface_count_offset = poly_offset + poly_count * 16
    surface_count = struct.unpack_from('<I', data, surface_count_offset)[0]
    surface_offset = surface_count_offset + 4
    polygons = [struct.unpack_from('<4H4h', data, poly_offset + i * 16) for i in range(poly_count)]
    surfaces = [struct.unpack_from('<II', data, surface_offset + i * 8) for i in range(surface_count)]
    return data[:poly_offset], polygons, surfaces, data[surface_offset + surface_count * 8:]


def function(source, signature):
    start = source.index(signature)
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


class CollisionAudio(unittest.TestCase):
    def test_only_floor_type_references_and_audio_bits_change(self):
        module = load_module()
        before = synthetic()
        after = module.apply(before)
        old_prefix, old_polys, old_types, old_tail = parts(before)
        new_prefix, new_polys, new_types, new_tail = parts(after)
        self.assertEqual(new_prefix, old_prefix, 'bounds/vertices/counts changed')
        self.assertEqual(new_types[:len(old_types)], old_types, 'shared original types changed')
        self.assertEqual(new_tail, old_tail, 'camera/water tail changed')
        self.assertEqual(len(new_types), len(old_types) + 1)
        for old, new in zip(old_polys, new_polys):
            self.assertEqual(new[1:], old[1:], 'indices/flags/normals/plane changed')
            if old[5] > 16383:
                data1, data0 = new_types[new[0]]
                self.assertEqual(data1 & 15, 2)
                self.assertEqual((data1 >> 11) & 63, 63)
                self.assertEqual(data0, old_types[old[0]][1])
                self.assertEqual(data1 & ~0x1F80F, old_types[old[0]][0] & ~0x1F80F)
            else:
                self.assertEqual(new, old, 'wall/ceiling classification was changed')
        self.assertEqual(module.apply(after), after, 'audio correction is not idempotent')

    def test_actual_native_sound_echo_readers_observe_corrected_fields(self):
        module = load_module()
        _, old_polys, old_types, _ = parts(synthetic())
        _, polys, types, _ = parts(module.apply(synthetic()))
        before = old_types[old_polys[0][0]][0]
        corrected = types[polys[0][0]][0]
        source = (ROOT / 'soh/src/code/z_bgcheck.c').read_text()
        table_start = source.index('u16 D_80119E10[14]')
        table = source[table_start:source.index(';', table_start) + 1]
        names = ['GROUND', 'SAND', 'CONCRETE', 'DIRT', 'WATER0', 'WATER1', 'WATER2',
                 'MAGMA', 'GRASS', 'GLASS', 'LADDER', 'ICE', 'IRON']
        fixture = '#include <cassert>\n#include <cstdint>\n'
        fixture += 'using u16=uint16_t; using u32=uint32_t; using s32=int32_t; using s8=int8_t;\n'
        fixture += 'struct CollisionContext {}; struct CollisionPoly {};\n'
        fixture += 'enum { SFX_FLAG=0, ' + ', '.join('NA_SE_PL_WALK_' + n + '=' + str(i + 100) for i, n in enumerate(names)) + ' };\n'
        fixture += 'static u32 data; static s8 sAudioCodeReverb;\nu32 SurfaceType_GetData(CollisionContext*,CollisionPoly*,s32,s32){return data;}\n'
        fixture += table + '\n'
        for sig in ('u32 func_80041F10(', 'u16 SurfaceType_GetSfx(', 'u32 SurfaceType_GetEcho('):
            fixture += function(source, sig) + '\n'
        audio = (ROOT / 'soh/src/code/code_800EC960.c').read_text()
        fixture += function(audio, 'void Audio_SetCodeReverb(') + '\n'
        fixture += ('int main(){ CollisionContext c; CollisionPoly p; data=' + str(before) + 'u;'
                    'assert(SurfaceType_GetSfx(&c,&p,0)!=NA_SE_PL_WALK_CONCRETE);'
                    'data=' + str(corrected) + 'u;'
                    'assert(SurfaceType_GetSfx(&c,&p,0)==NA_SE_PL_WALK_CONCRETE);'
                    'assert(SurfaceType_GetEcho(&c,&p,0)==63);'
                    'Audio_SetCodeReverb(SurfaceType_GetEcho(&c,&p,0)); assert(sAudioCodeReverb==63);}\n')
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'native.cpp'
            path.write_text(fixture)
            subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror',
                            '-Wno-unused-parameter', '-fsanitize=address,undefined',
                            '-fno-sanitize-recover=all', '-no-pie', str(path), '-o', str(path.with_suffix(''))], check=True)
            subprocess.run([str(path.with_suffix(''))], check=True,
                           env={**__import__('os').environ, 'ASAN_OPTIONS': 'detect_leaks=0'})

    def test_rejects_wrong_header_and_truncated_surface_array(self):
        module = load_module()
        for data in (b'', synthetic()[:64], synthetic().replace(b'LOCO', b'NOPE', 1), synthetic()[:174]):
            with self.assertRaises(ValueError):
                module.apply(data)


if __name__ == '__main__':
    unittest.main()
