#!/usr/bin/env python3
"""Verify the native interior door's scene record and supported approach."""
from pathlib import Path
import struct
import sys
import argparse

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/black_market'))
sys.path.insert(0, str(ROOT / 'scripts/assets'))
import forest
import exit_door
import native_market
import prepare_native_market


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prepared', action='store_true', help='also check the integrated default asset pipeline')
    args = parser.parse_args()
    original = native_market.read_bundle(ROOT / 'tools/native_market/bundled')
    resources = exit_door.apply(original)
    assert resources == exit_door.apply(resources), 'native door preparation must be idempotent'
    assert set(resources) == set(original), 'door uses existing native assets and adds no resources'
    assert {name for name in resources if resources[name] != original[name]} == set(exit_door.SCENES.values())
    if args.prepared:
        prepared = prepare_native_market.prepared_resources()
        for path in exit_door.SCENES.values():
            assert prepared[path] == resources[path], 'default build must prepare the exact native door scene'
    for host, scene, identity, yaw, params in (
        ('mm', 'scenes/nonmq/skiejer_market/skiejer_market', 5, 127, 7),
        ('oot', 'scenes/shared/skiejer_market_oot/skiejer_market_oot', 9, 0, 3 << 7),
    ):
        commands = forest.native_commands(resources[scene])
        assert 14 in commands, f'{host}: the visible native door requires a transition-actor record'
        assert struct.unpack('<I4bHhhhhH', commands[14]) == (
            1, 0, -1, -1, -1, identity, -10, 0, 420, yaw, params)
        count, = struct.unpack_from('<I', commands[0])
        spawn = struct.unpack_from('<H6hH', commands[0], 4)
        assert count == 1 and spawn[1:4] == (-11, 0, 381) and spawn[5] == -32768
        assert 420 - spawn[3] == 39, 'arrival stays inside the native handle range, facing the counter'
        before = forest.native_commands(original[scene])
        assert {identity: data for identity, data in commands.items() if identity != 14} == before
    collision = resources[native_market.COLLISION]
    minimum, maximum = struct.unpack_from('<3h', collision, 64), struct.unpack_from('<3h', collision, 70)
    count, = struct.unpack_from('<I', collision, 76)
    vertices = [struct.unpack_from('<3h', collision, 80 + i * 6) for i in range(count)]
    offset = 80 + count * 6
    count, = struct.unpack_from('<I', collision, offset)
    polygons = [struct.unpack_from('<4H4h', collision, offset + 4 + i * 16) for i in range(count)]
    def floor_at(x, z):
        for polygon in polygons:
            if polygon[5] < 23170:
                continue
            a, b, c = (vertices[index & 8191] for index in polygon[1:4])
            bx, bz, cx, cz = b[0] - a[0], b[2] - a[2], c[0] - a[0], c[2] - a[2]
            determinant = bx * cz - bz * cx
            if not determinant:
                continue
            dx, dz = x - a[0], z - a[2]
            u, v = (dx * cz - dz * cx) / determinant, (bx * dz - bz * dx) / determinant
            y = a[1] + u * (b[1] - a[1]) + v * (c[1] - a[1])
            if u >= 0 and v >= 0 and u + v <= 1.000001 and -1 <= y <= 2:
                return True
        return False
    for x in (-30, -10, 10):
        for z in (381, 400, 420, 430, exit_door.contract()['FarExitZ']):
            assert floor_at(x, z), ('native door approach or far-side escape lacks a floor', x, z)
    assert maximum[2] == 458 and exit_door.contract()['FarExitZ'] < maximum[2]
    assert not floor_at(-10, 470), 'do not move the fallback beyond the unchanged native room floor'
    malformed = dict(resources)
    path = exit_door.SCENES['mm']
    commands = forest.native_commands(malformed[path]); commands[14] = commands[14][:-2] + b'\0\0'
    malformed[path] = malformed[path][:64] + struct.pack('<I', len(commands)) + b''.join(
        struct.pack('<I', identity) + data for identity, data in commands.items())
    try:
        exit_door.apply(malformed)
    except ValueError:
        pass
    else:
        raise AssertionError('preparation must reject an unexpected existing door record')
    print('PASS native interior exit doors: exact host records, unchanged resources/spawn, supported approach/escape, bounded preparation and idempotence')


if __name__ == '__main__':
    main()
