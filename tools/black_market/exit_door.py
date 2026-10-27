#!/usr/bin/env python3
"""Add native handle doors to the two prepared interior scene headers only."""
from pathlib import Path
import re
import struct

import forest

ROOT = Path(__file__).resolve().parents[2]
SCENES = {
    'mm': 'scenes/nonmq/skiejer_market/skiejer_market',
    'oot': 'scenes/shared/skiejer_market_oot/skiejer_market_oot',
}


def contract():
    source = (ROOT / 'combo/BlackMarketExitDoor.h').read_text()
    values = {}
    for name in ('X', 'Y', 'Z', 'Yaw', 'MmPackedYaw', 'MmActorId', 'OotActorId',
                 'MmParams', 'OotParams', 'FarExitZ'):
        match = re.search(r'inline constexpr int16_t k' + name + r' = (-?\d+);', source)
        if not match:
            raise ValueError('inspect the changed native exit door contract: ' + name)
        values[name] = int(match[1])
    return values


def apply(resources):
    result, values = dict(resources), contract()
    for host, path in SCENES.items():
        data = result[path]
        commands = forest.native_commands(data)
        prefix = 'Oot' if host == 'oot' else 'Mm'
        record = struct.pack('<I4bHhhhhH', 1, 0, -1, -1, -1, values[prefix + 'ActorId'],
                             values['X'], values['Y'], values['Z'],
                             values['Yaw'] if host == 'oot' else values['MmPackedYaw'],
                             values[prefix + 'Params'])
        if 14 in commands:
            if commands[14] != record:
                raise ValueError('unexpected existing native interior door: ' + path)
            continue
        if 0 not in commands:
            raise ValueError('native interior player spawn is missing: ' + path)
        encoded = []
        for identity, payload in commands.items():
            if identity == 0:
                encoded.append(struct.pack('<I', 14) + record)
            encoded.append(struct.pack('<I', identity) + payload)
        result[path] = data[:64] + struct.pack('<I', len(encoded)) + b''.join(encoded)
    return result
