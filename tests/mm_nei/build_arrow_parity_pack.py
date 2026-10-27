"""Repackage the accepted Ice/Light resource bytes under native MM names.

No artwork, texture metadata or source archive is modified. Source hashes pin
the final Light iteration rather than the older Henriko Light replacement.
"""
import argparse
import hashlib
import io
import json
from pathlib import Path
import struct
import zipfile


def sha(data):
    return hashlib.sha256(data).hexdigest()


def read_source(directory, name, expected):
    data = (directory / name).read_bytes()
    if sha(data) != expected:
        raise ValueError(f'Unexpected source bytes: {name}')
    return data


def write_zip(path, files):
    with zipfile.ZipFile(path, 'w') as output:
        for name, data in files.items():
            info = zipfile.ZipInfo(name, (2026, 9, 29, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o644 << 16
            output.writestr(info, data)


def build(sources, output):
    snow_name = 'zz_Henriko_Ice_Arrow_Snowflake_POC2.o2r'
    source_zip_name = 'Henriko_Ice_Arrow_Snowflake_POC2.zip'
    zip_bytes = read_source(sources, source_zip_name,
                           'cce82c05bc40c8d22f5e54d3a53f809e2914452331f9925a8305e2228b4b2da1')
    with zipfile.ZipFile(io.BytesIO(zip_bytes)) as bundle:
        snow_bytes = bundle.read(snow_name)
    if sha(snow_bytes) != '7b172fc2a3cef98fc85992964dbbcb41e51b71d0dc733775a199afbcb840c25a':
        raise ValueError('Unexpected accepted snowflake archive')
    light_name = 'zzz_Medallion_Magic_POC1_HD.o2r'
    light_bytes = read_source(sources, light_name,
                             '5fb1e5d6889cfaa3a2f5d1e2cf9eb190eddbd3333e5cae99f5c07c8c244da7ad')
    snow_key = 'alt/custom/henriko_effects/arrows/ice_snowflake_poc2'
    mapping = [
        (snow_name, snow_key, snow_key),
        (light_name, 'alt/overlays/ovl_Arrow_Light/s1Tex',
         'alt/overlays/ovl_Arrow_Light/gLightArrowTex'),
        (light_name, 'alt/overlays/ovl_Arrow_Light/s2Tex',
         'alt/overlays/ovl_Arrow_Light/gLightArrowMaskTex'),
    ]
    archives = {snow_name: zipfile.ZipFile(io.BytesIO(snow_bytes)),
                light_name: zipfile.ZipFile(io.BytesIO(light_bytes))}
    files, entries = {}, []
    for source_name, source_key, target_key in mapping:
        data = archives[source_name].read(source_key)
        # OTR V1 header followed by texture header; check physical image metadata
        # and complete RGBA payload without touching a single authored byte.
        endian, resource_type, version = struct.unpack_from('<3I', data)
        kind, width, height, flags, hs, vs, size = struct.unpack_from('<4I2fI', data, 64)
        if (endian, resource_type, version) != (0, 0x4F544558, 1):
            raise ValueError(f'Unexpected resource header: {source_key}')
        expected = (1, 1024, 1024, 1, 64.0, 16.0) if source_name == snow_name else (
            6, 512, 1024, 3, 1.0, 1.0)
        if (kind, width, height, flags, hs, vs) != expected or size != width * height * 4 or len(data) != 92 + size:
            raise ValueError(f'Unexpected texture metadata: {source_key}')
        files[target_key] = data
        entries.append(dict(source_archive=source_name, source_key=source_key, mm_key=target_key,
                            bytes=len(data), sha256=sha(data), payload_sha256=sha(data[92:]),
                            texture=dict(version=version, type=kind, width=width, height=height,
                                         flags=flags, horizontal_byte_scale=hs, vertical_pixel_scale=vs)))
    output.mkdir(parents=True, exist_ok=True)
    pack = output / 'zzzz_MM_Ice_Light_Arrow_Parity_POC1.o2r'
    write_zip(pack, files)
    with zipfile.ZipFile(pack) as check:
        assert check.namelist() == list(files)
        for source_name, source_key, target_key in mapping:
            assert check.read(target_key) == archives[source_name].read(source_key)
    manifest = dict(pack=pack.name, sha256=sha(pack.read_bytes()),
                    source_archives={snow_name: sha(snow_bytes), light_name: sha(light_bytes)},
                    entries=entries, validation='Exact complete resource and image payload byte equality; '
                    'native MM factory and actor checks recorded separately. Runtime appearance untested.')
    (output / 'MM_Ice_Light_Arrow_Parity_manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    readme = '''MM Ice/Light Arrow Parity POC1

Copy zzzz_MM_Ice_Light_Arrow_Parity_POC1.o2r into mods/2ship/ and enable Alt Assets.
In MM's Mod Menu, place it above MM Reloaded and other effect packs, save the
order, and restart ComboShip so this small pack has priority for its three keys.
Keep the existing Henriko and final Medallion Magic packs installed in their current
locations. This small addition contains only the existing POC2 snowflake and the
final accepted Light Arrow pair, copied byte-for-byte under native MM keys. It does
not replace Fire, rod artwork, models, collision, audio, timing or gameplay.

The supplied log mounted the snowflake archive only in mods/soh/. Native MM checks
its own resource manager. The final Light pack's s1Tex/s2Tex names serve OoT; native
MM consumes gLightArrowTex/gLightArrowMaskTex. This archive supplies those MM names.
Alt off uses the original native effects. The original packs remain unchanged.

Source pins and per-resource hashes are in MM_Ice_Light_Arrow_Parity_manifest.json.
Rebuild with: python3 -B tests/mm_nei/build_arrow_parity_pack.py --sources PATH --output PATH

Verification: exact archive scope and full resource/pixel byte equality; MM factory
parsing preserves all three authored payloads and metadata; native MM snowflake
charge/release/flight/impact and Alt/fallback tests pass. This is an implemented,
source-verified candidate. Appearance and installed archive priority need a check
in the intended running build. Test charge, release, flight and hit for native Ice
and Light arrows, then toggle Alt Assets and check the native fallback.
'''
    (output / 'README_MM_Ice_Light_Arrow_Parity.txt').write_text(readme)
    bundle_files = {p.name: p.read_bytes() for p in
                    (pack, output / 'README_MM_Ice_Light_Arrow_Parity.txt',
                     output / 'MM_Ice_Light_Arrow_Parity_manifest.json')}
    write_zip(output / 'MM_Ice_Light_Arrow_Parity_POC1.zip', bundle_files)
    print('PASS accepted source hashes, exact three-key scope, metadata, full resource/payload byte equality')
    print(f'{pack.name} sha256={manifest["sha256"]}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sources', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    build(args.sources, args.output)
