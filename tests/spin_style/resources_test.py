"""Check shipped private spin resource graphs, not just their filenames."""
from pathlib import Path
import hashlib
import json
import struct
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/assets"))
from build_din_fire_shield import crc64
from build_spin_style_resources import material_combine


def main():
    from prepare_spin_styles import prepare
    prepare()
    def alpha_mux(w0, w1):
        return ((w0 >> 12) & 7, (w0 >> 9) & 7, (w1 >> 12) & 7, (w1 >> 9) & 7,
                (w1 >> 21) & 7, (w1 >> 18) & 7, (w1 >> 3) & 7, w1 & 7)
    for w0, w1 in ((0xFCFF9DFF, 0xFF0DA4FF), (0xFCFFFFFF, 0xFF0DF4FF),
                   (0xFCFFFFFF, 0xFF0DF8FF), (0xFCFF95FF, 0xFF0DFEFF)):
        for dark in (False, True):
            assert alpha_mux(*material_combine(w0, w1, dark)) == alpha_mux(w0, w1)
    manifest = json.loads((ROOT / "tools/spin_styles/manifest.json").read_text())
    expected = {"fire", "ice", "light", "lightning", "arcane", "astral", "dark_energy"}
    assert set(manifest["styles"]) == expected
    shared = set(manifest["outputs"]["soh"]) & set(manifest["outputs"]["mm"])
    assert all(manifest["outputs"]["soh"][name] == manifest["outputs"]["mm"][name] for name in shared), "Cross-game spin resource collision"
    for host in ("soh", "mm"):
        all_files = {}
        for relative, digest in manifest["outputs"][host].items():
            data = (ROOT / host / "assets/custom" / relative).read_bytes()
            assert hashlib.sha256(data).hexdigest() == digest, relative
            assert relative.startswith("objects/combo_spin/"), relative
            all_files[crc64(relative)] = (relative, data)
        for style in expected:
            resource_style = style + "_" + host if style == "astral" else style
            for root in ("charge", "disk_regular", "cylinder_regular", "disk_great", "cylinder_great"):
                key = crc64(f"objects/combo_spin/{resource_style}/{root}")
                assert key in all_files, (host, style, root)
            for texture in ("burst0", "burst1", "charge0", "charge1"):
                assert crc64(f"objects/combo_spin/{resource_style}/{texture}") in all_files, (host, style, texture)
        references = 0
        for name, data in all_files.values():
            kind, version = struct.unpack_from("<II", data, 4)
            if kind == 0x4F544558:
                assert version == 1, name
                typ, width, height, flags = struct.unpack_from("<IIII", data, 64)
                sx, sy, length = struct.unpack_from("<ffI", data, 80)
                assert typ == 6 and flags == 3 and sx == sy == 1.0, name
                assert len(data) == 92 + length == 92 + width * height * 4, name
                assert 0 < width <= 4096 and 0 < height <= 4096, name
            elif kind == 0x4F444C54:
                assert data[64] == 4, name  # Both native hosts use F3DEX2.
                i, cache = 72, set()
                ended = False
                while i < len(data):
                    w0, w1 = struct.unpack_from("<II", data, i)
                    i += 8
                    op = w0 >> 24
                    if op in (0x20, 0x31, 0x32, 0x33, 0x36, 0x37, 0x39):
                        hi, lo = struct.unpack_from("<II", data, i)
                        i += 8
                        target = (hi << 32) | lo
                        assert target in all_files, (name, hex(op), hex(target))
                        references += 1
                        if op == 0x32:
                            vertices = all_files[target][1]
                            typ, total = struct.unpack_from("<II", vertices, 64)
                            assert typ == 25 and len(vertices) == 72 + total * 16
                            count = (w0 >> 12) & 255
                            first = ((w0 >> 1) & 127) - count
                            assert 0 < count <= 32 and 0 <= first <= first + count <= 32
                            assert w1 % 16 == 0 and w1 // 16 + count <= total
                            cache.update(range(first, first + count))
                    if op in (0x05, 0x06):
                        for packed in ((w0, w1) if op == 0x06 else (w0,)):
                            assert all(((packed >> s) & 255) // 2 in cache for s in (16, 8, 0)), name
                    if op == 0xFC:
                        # Decode the shipped graph: both cosmetic channels
                        # must contribute in each cycle, including charge.
                        a, b = (5, 3) if "/dark_energy/" in name else (3, 5)
                        assert ((w0 >> 20) & 15, (w0 >> 15) & 31,
                                (w1 >> 28) & 15, (w1 >> 15) & 7) == (a, 8, b, b), name
                        assert ((w0 >> 5) & 15, w0 & 31,
                                (w1 >> 24) & 15, (w1 >> 6) & 7) == (a, 8, b, b), name
                    assert op not in (0xFA, 0xFB), (name, "embedded color overrides editor")
                    if op == 0xDF:
                        assert i == len(data), name
                        ended = True
                assert ended, name
        assert references > 100
    print("PASS private resource closure, both host formats, texture sampling metadata and vertex bounds")


if __name__ == "__main__":
    main()
