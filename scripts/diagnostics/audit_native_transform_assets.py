#!/usr/bin/env python3
"""Read-only MM transformation asset closure/stride audit; no GPU/audio proof."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zipfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path)
    args = parser.parse_args()
    mask = (1 << 64) - 1
    table = []
    for byte in range(256):
        value = byte << 56
        for _ in range(8):
            value = ((value << 1) ^ (0x42F0E1EBA9EA3693 if value >> 63 else 0)) & mask
        table.append(value)

    def crc(path):
        value = mask
        for byte in path.encode():
            value = ((value << 8) & mask) ^ table[((value >> 56) ^ byte) & 255]
        return value

    with zipfile.ZipFile(args.archive) as archive:
        index = {crc(name): name for name in archive.namelist()}
        visited, dynamic = set(), set()
        frames = {}
        for name, count in {"cl_setmask": 66, "cl_setmaskend": 3, "cl_maskoff": 10,
                            "pg_maskoffstart": 15, "pz_maskoffstart": 15, "pn_maskoffstart": 15}.items():
            data = archive.read("misc/link_animetion/gPlayerAnim_" + name + "_Data")
            values, = struct.unpack_from("<I", data, 64)
            assert values == count * 67 and len(data) == 68 + values * 2, (name, "invalid native frame stride")
            frames[name] = count

        def visit(path):
            if path in visited: return
            visited.add(path)
            data = archive.read(path)
            assert struct.unpack_from("<I", data, 4)[0] == 0x4F444C54 and data[64] == 4, path
            cursor, ended = 72, False
            while cursor < len(data):
                w0, w1 = struct.unpack_from("<II", data, cursor)
                cursor += 8
                opcode = w0 >> 24
                if opcode in (0x20, 0x31, 0x32, 0x33, 0x35, 0x36, 0x37):
                    hi, lo = struct.unpack_from("<II", data, cursor)
                    cursor += 8
                    target = index.get(hi << 32 | lo)
                    assert target is not None, (path, "unresolved native hash", hex(hi << 32 | lo))
                    payload = archive.read(target)
                    if opcode == 0x31: visit(target)
                    elif opcode == 0x32:
                        assert struct.unpack_from("<I", payload, 4)[0] == 0x4F415252, target
                        kind, count = struct.unpack_from("<II", payload, 64)
                        assert kind == 25 and len(payload) == 72 + count * 16, target
                        assert w1 % 16 == 0 and w1 // 16 + ((w0 >> 12) & 255) <= count, target
                elif opcode in (0xDE, 0x3D) and w1:
                    dynamic.add(hex(w1))
                if opcode == 0xDF:
                    ended = True
                    break
            assert ended, (path, "unterminated native display list")

        for form in ("FierceDeity", "Goron", "Zora", "Deku"):
            visit("objects/gameplay_keep/g" + form + "MaskDL")
        for form, offset in (("boy", "000900"), ("goron", "0014A0"), ("zora", "000DB0"), ("nuts", "001D90")):
            visit(f"objects/object_mask_{form}/object_mask_{form}_DL_{offset}")
        visit("objects/gameplay_keep/gameplay_keep_DL_054C90")
        scroll = archive.read("objects/gameplay_keep/gameplay_keep_Matanimheader_054F18")
        assert scroll[64:] == struct.pack("<IbhbbBBbbBB", 1, -4, 1, -1, 0, 16, 16, 1, -2, 16, 16)
    print(json.dumps({"result": "PASS", "archive_sha256": hashlib.file_digest(args.archive.open("rb"), "sha256").hexdigest(),
                      "animation_frames": frames, "display_lists": len(visited), "dynamic_calls": sorted(dynamic),
                      "veil_scroll": "segment 0x0B; (-1,0), (1,2), dual 16x16",
                      "runtime_visual_audio": "untested"}, indent=2))


if __name__ == "__main__":
    main()
