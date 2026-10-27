"""Copy accepted medallion pixels into private receipt-only RGBA32 resources."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zipfile

ROOT = Path(__file__).resolve().parents[2]
DEST = ROOT / 'soh/assets/custom/objects/nei_elemental_arrow_gi'
SOURCE_SHA = '5fb1e5d6889cfaa3a2f5d1e2cf9eb190eddbd3333e5cae99f5c07c8c244da7ad'
INPUTS = {
    'fire': 'alt/custom/medallion_magic/spells/fire/s1Tex',
    'ice': 'alt/custom/medallion_magic/arrows/water/s2Tex',
    'light': 'alt/overlays/ovl_Arrow_Light/s1Tex',
}
sha = lambda data: hashlib.sha256(data).hexdigest()


def build(source, output):
    assert sha(source.read_bytes()) == SOURCE_SHA, 'Unexpected medallion reference pack'
    DEST.mkdir(parents=True, exist_ok=True)
    output.mkdir(parents=True, exist_ok=True)
    manifest = {'source_sha256': SOURCE_SHA, 'entries': {}}
    files = {}
    with zipfile.ZipFile(source) as reference:
        for slug, key in INPUTS.items():
            original = reference.read(key)
            assert struct.unpack_from('<3I', original) == (0, 0x4F544558, 1)
            kind, width, height, flags, hs, vs, size = struct.unpack_from('<4I2fI', original, 64)
            assert kind in (5, 6) and flags == 3 and hs == vs == 1
            assert size == width * height * 4 and len(original) == 92 + size
            resource = bytearray(original)
            # Payload already contains expanded RGBA image bytes. Keep the IMG
            # flag and physical dimensions; declare its actual native format.
            struct.pack_into('<I', resource, 64, 1)
            resource = bytes(resource)
            assert resource[92:] == original[92:]
            target = 'objects/nei_elemental_arrow_gi/' + slug
            files[target] = resource
            (DEST / slug).write_bytes(resource)
            manifest['entries'][slug] = dict(source=key, target=target, pixels=[width, height],
                                             payload_sha256=sha(original[92:]), resource_sha256=sha(resource))
    pack = output / 'Elemental_Arrow_GI_POC1_Assets.o2r'
    with zipfile.ZipFile(pack, 'w', compression=zipfile.ZIP_DEFLATED) as archive:
        for key, data in files.items():
            entry = zipfile.ZipInfo(key, (2026, 10, 7, 0, 0, 0))
            entry.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(entry, data)
    with zipfile.ZipFile(pack) as archive:
        assert archive.namelist() == list(files)
        for key, data in files.items():
            assert archive.read(key) == data
    manifest['pack_sha256'] = sha(pack.read_bytes())
    Path(__file__).with_name('material_manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print('PASS three private GI resources: original medallion pixels, exact payload equality, closed archive scope')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('reference_pack', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    build(args.reference_pack, args.output)
