"""Package generated RGBA artwork as private MM texture resources.

The PNG is retained unchanged. Cropping its four grid cells and sampling them
at 128px is an asset-format conversion; this tool creates no replacement art.
"""
import hashlib
import json
import struct
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
SOURCE = Path(__file__).with_name("leaves-generated.png")
DEST = ROOT / "mm/assets/custom/objects/nei_autumn/leaves"
NAMES = ("crimson", "orange", "gold", "copper")

def build():
    image = Image.open(SOURCE).convert("RGBA")
    assert image.width == image.height and image.width % 2 == 0
    assert image.getextrema()[3][0] == 0, "Generated source must have real transparency"
    cell = image.width // 2
    DEST.mkdir(parents=True, exist_ok=True)
    manifest = {"source": SOURCE.name, "source_sha256": hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
                "source_pixels": list(image.size), "generator": "built-in imagegen", "textures": {}}
    for index, name in enumerate(NAMES):
        x, y = (index % 2) * cell, (index // 2) * cell
        pixels = image.crop((x, y, x + cell, y + cell)).resize((128, 128), Image.Resampling.LANCZOS)
        header = bytearray(64)
        struct.pack_into("<IIIQQ", header, 0, 0, 0x4F544558, 1, 0xDEADBEEFDEADBEEF, 1 << 32)
        resource = header + struct.pack("<IIIIffI", 1, 128, 128, 1, 4.0, 4.0, 128 * 128 * 4) + pixels.tobytes()
        path = DEST / (name + "_tex")
        path.write_bytes(resource)
        manifest["textures"][name] = {"path": str(path.relative_to(ROOT)), "logical_tile": [32, 32],
                                      "pixels": [128, 128], "sha256": hashlib.sha256(resource).hexdigest()}
    Path(__file__).with_name("manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print("Built four generated RGBA32 autumn textures with preserved alpha")

if __name__ == "__main__":
    build()
