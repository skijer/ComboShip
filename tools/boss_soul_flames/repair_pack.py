#!/usr/bin/env python3
"""Repair the serialized MM flame alpha encoding without retouching RGB art.

The native MM primitive LOD is 192/255. With a zero fixed envelope, alpha is
  (moving_alpha - 1) * 192/255.
The shipped 2-cycle shaders wrap results below -0.51 back to bright values.
The input art reaches 66/255 and crosses that threshold. An alpha-only minimum
of 86/255 keeps the value above -0.5, including a margin from the wrap boundary.
OoT uses LOD 128/255 and cannot cross it with this art: preserve all its bytes.
"""
from hashlib import sha256
import json
from pathlib import Path
import struct
from zipfile import ZipFile, ZipInfo, ZIP_DEFLATED

ROOT = Path(__file__).resolve().parent
INPUTS = {
    'OoT': ('soh', 'POC4_OoT.o2r', '066f877c737ef9113b9971ec687921d25afbc16446c99dbb20499d886176e79a',
            'alt/objects/object_gi_fire/object_gi_fireTex_000200'),
    'MM': ('2ship', 'POC4_MM.o2r', 'cf1b71ef263f59a4eda10f6b0ca04a9825b5db91c28d2335251799452ef3ed80',
           'alt/objects/gameplay_keep/gameplay_keep_Tex_01AAB0'),
}
ALPHA_FLOOR = 86


def digest(data):
    return sha256(data).hexdigest()


def build():
    records = []
    for owner, (folder, filename, expected_sha, moving_path) in INPUTS.items():
        original = ROOT / 'inputs' / filename
        assert digest(original.read_bytes()) == expected_sha
        with ZipFile(original) as z:
            assert z.testzip() is None
            original_entries = {n: z.read(n) for n in z.namelist()}
        assert len(original_entries) == 2 and moving_path in original_entries
        entries = dict(original_entries)
        changed = 0
        if owner == 'MM':
            resource = bytearray(entries[moving_path])
            assert struct.unpack_from('<I', resource, 8)[0] == 1
            assert struct.unpack_from('<IIIIffI', resource, 64) == (6, 256, 512, 3, 1.0, 1.0, 524288)
            assert len(resource) == 524380
            for offset in range(95, len(resource), 4):
                if resource[offset] < ALPHA_FLOOR:
                    resource[offset] = ALPHA_FLOOR
                    changed += 1
            entries[moving_path] = bytes(resource)
        name = f'zz_BossSoul_Realistic_Flames_{owner}_POC5_AlphaSafe.o2r'
        output = ROOT / 'mods' / folder / name
        output.parent.mkdir(parents=True, exist_ok=True)
        with ZipFile(output, 'w', ZIP_DEFLATED, compresslevel=9) as z:
            for path, payload in entries.items():
                info = ZipInfo(path, (2026, 10, 20, 0, 0, 0))
                info.compress_type = ZIP_DEFLATED
                info.external_attr = 0o100644 << 16
                z.writestr(info, payload, compresslevel=9)
        with ZipFile(output) as z:
            assert z.testzip() is None and z.namelist() == list(entries)
            for path, payload in entries.items():
                assert z.read(path) == payload
                old = original_entries[path]
                if path != moving_path or owner == 'OoT':
                    assert payload == old, 'An unaffected resource changed'
                else:
                    assert payload[:92] == old[:92], 'Texture header changed'
                    for channel in range(3):
                        assert payload[92 + channel::4] == old[92 + channel::4], 'RGB artwork changed'
                    assert min(payload[95::4]) >= ALPHA_FLOOR
                    assert sum(a != b for a, b in zip(payload, old)) == changed
        records.append({'owner': owner, 'path': str(output.relative_to(ROOT)),
                        'sha256': digest(output.read_bytes()),
                        'alpha_bytes_changed': changed, 'rgb_bytes_changed': 0,
                        'resources': {path: digest(payload) for path, payload in entries.items()}})
    manifest = {
        'candidate': 'BossSoul realistic flames POC5 alpha encoding repair',
        'baseline': {owner: {'input': value[1], 'sha256': value[2]} for owner, value in INPUTS.items()},
        'cause': 'POC4 moving-alpha minimum 66/255 makes the native MM combiner dip below the shader wrap floor; '
                 'its negative alpha wraps into opaque detached flecks outside the fixed flame envelope.',
        'repair': 'Clamp only MM serialized physical alpha below 86/255. Keep all RGB bytes, native fixed envelope, '
                  'resource headers and the entire OoT resources unchanged.',
        'changed_MM_alpha_fraction': records[1]['alpha_bytes_changed'] / (256 * 512),
        'worst_case_MM_intermediate_alpha': (ALPHA_FLOOR / 255 - 1) * (192 / 255),
        'native_shader_wrap_floor': -0.51,
        'preservation': ['Original RGB artwork byte-for-byte', 'Native fixed envelope byte-for-byte',
                         'OoT textures byte-for-byte', 'Native colors, scrolling, meshes, models and jaw',
                         'Accepted boss soul dialogue and progression'],
        'scope': 'Two Alt texture resources per owner. Shared consumers include OoT blue-fire GI and MM EnLight flames.',
        'runtime': 'Configured game pickup and installed mod stack remain untested; no master promotion implied.',
        'archives': records,
    }
    (ROOT / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(json.dumps(manifest, indent=2))


if __name__ == '__main__':
    build()
