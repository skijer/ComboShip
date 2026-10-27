"""Verify the accepted POC6 cast artwork and native resource layout."""
import hashlib
import json
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[2]
assets = ROOT / "soh/assets/custom/objects/nei_rod_cast_poc6"
manifest = json.loads((ROOT / "tools/nei_rod_cast_poc6/material_manifest.json").read_text())
sources = {
    "fire_natural": "9f2553e2546bb745db1421535200fea13368440d45409cfb9e9c6ab6a3bd389e",
    "ice_fracture_release": "50bf077b004ca6d3650cb1e7a38f19393f875c811ff1fdbc24ab40aefb57a24b",
}
accepted_resources = {
    "fire_natural": "69ecf822beec19257c8714895a5ca1643b01c7b61b3259edb043bf2cc0ad0a11",
    "ice_fracture_release": "4aa7501dc95a49f99f7f788a56456a66c68d2ce352bf057409559b023dcdb982",
}
assert set(manifest) == {path.name for path in assets.iterdir()} == set(sources)
for name, source_hash in sources.items():
    metadata = manifest[name]
    raw = (assets / name).read_bytes()
    assert metadata["source_sha256"] == source_hash
    assert hashlib.sha256(raw).hexdigest() == metadata["resource_sha256"] == accepted_resources[name]
    assert struct.unpack_from("<III", raw) == (0, 0x4F544558, 1)
    assert struct.unpack_from("<IIIIffI", raw, 64) == (1, 512, 512, 1, 16., 16., 512 * 512 * 4)
    assert len(raw) == 92 + 512 * 512 * 4
    alpha = raw[95::4]
    assert min(alpha) == 0 and max(alpha) > 200
    assert sum(0 < value < 255 for value in alpha) > len(alpha) / 20
print("PASS accepted POC6 cast artwork: source identity, native RGBA32, logical scale, transparency and pinned content hashes")
