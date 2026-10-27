#!/usr/bin/env python3
"""Regression: replacement flame must not make zero-envelope pixels visible.

The test exercises serialized archive resources and the shipped alpha equation
with its actual wrap stages, rather than assuming intermediate values clamp.
It does not claim a configured gameplay test or a Windows GPU test.
"""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import struct
from zipfile import ZipFile
import numpy as np
from gpu_check import FlameGPU

ROOT = Path(__file__).resolve().parent
ROUTES = {
    'OoT': ('soh', 'objects/object_gi_fire/object_gi_fireTex_000000',
            'objects/object_gi_fire/object_gi_fireTex_000200'),
    'MM': ('2ship', 'objects/gameplay_keep/gameplay_keep_Tex_01A8B0',
           'objects/gameplay_keep/gameplay_keep_Tex_01AAB0'),
}


def texture(data):
    version = struct.unpack_from('<I', data, 8)[0]
    kind, w, h = struct.unpack_from('<III', data, 64)
    assert kind == 6
    if version == 0:
        length = struct.unpack_from('<I', data, 76)[0]
        assert length == w * h and len(data) == 80 + length
        return np.repeat(np.frombuffer(data[80:], np.uint8).reshape(h, w, 1), 4, axis=2)
    assert version == 1
    flags, sx, sy, length = struct.unpack_from('<IffI', data, 76)
    assert flags == 3 and sx == sy == 1.0
    assert length == w * h * 4 and len(data) == 92 + length
    return np.frombuffer(data[92:], np.uint8).reshape(h, w, 4)


def resources(path, owner):
    _, fixed, moving = ROUTES[owner]
    with ZipFile(path) as z:
        assert z.testzip() is None
        assert set(z.namelist()) == {'alt/' + fixed, 'alt/' + moving}
        return z.read('alt/' + fixed), z.read('alt/' + moving)


def check(path, owner, gpu, phases):
    fixed, moving = resources(path, owner)
    assert sha256(fixed).hexdigest() == '7fee10e9e4cb91c9b6c8c0627c07fb486a614f4d45dbc7c31bec7f51ea5ce733'
    results = {}
    for mode, name in enumerate(('nearest', 'bilinear', 'three-point')):
        gpu.upload(texture(fixed), texture(moving), mode)
        maximum, worst_phase = 0, 0
        for phase in phases:
            pixels = gpu.render(owner, phase)
            outside = int(np.count_nonzero((pixels[:, :, 1] == 0) & (pixels[:, :, 0] > 8)))
            if outside > maximum:
                maximum, worst_phase = outside, phase
        results[name] = {'maximum_visible_pixels_outside_zero_native_envelope': maximum,
                         'worst_scroll_phase': worst_phase, 'phases': len(phases)}
    return results


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--mm', required=True, type=Path)
    p.add_argument('--oot', type=Path)
    p.add_argument('--out', type=Path, default=ROOT / 'verification.json')
    p.add_argument('--quarter-phases', action='store_true')
    args = p.parse_args()
    step = .25 if args.quarter_phases else 1
    # MM's native phase is signed s8. Its scroll period is 64 frames; OoT's
    # moving S domain has period 256, so sample that full cycle separately.
    gpu = FlameGPU()
    report = {'renderer': gpu.renderer, 'resolution': [gpu.width, gpu.height],
              'scope': 'Archive resources and headless OpenGL alpha combiner; gameplay untested',
              'results': {}}
    try:
        for owner, path, period in (('MM', args.mm, 64), ('OoT', args.oot, 256)):
            if path is not None:
                phases = np.arange(0, period, step).tolist()
                report['results'][owner] = check(path, owner, gpu, phases)
    finally:
        gpu.close()
    args.out.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))
    failures = [(owner, mode, result['maximum_visible_pixels_outside_zero_native_envelope'])
                for owner, modes in report['results'].items()
                for mode, result in modes.items()
                if result['maximum_visible_pixels_outside_zero_native_envelope']]
    assert not failures, f'Flame alpha escapes the native envelope: {failures}'
    print('PASS: no wrapped opaque flecks outside the native flame envelope')


if __name__ == '__main__':
    main()
