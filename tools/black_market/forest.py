#!/usr/bin/env python3
"""Create a separate native solo forest addon from the immutable MM POC9.

POC9 scenery is copied into a closed namespace. Native scene framing replaces
only the prologue machinery; physical scene/collision data stays inherited.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import stat
import xml.etree.ElementTree as ET
import zipfile

ROOT = Path(__file__).resolve().parents[2]
DONOR_SHA = "72d750b00583379f3619dac52dce59e2957a351fdfabd3a3a8da11f9bc19daf0"
NATIVE_SHA = "f10167e5682d74cc8da0137c524b1521f4e7ff47438b8888b63db6c7c6dc8d59"
OLD = "custom/cor_mm3d/lost_woods_poc1/"
PRIVATE = "custom/skiejer_forest/"
NATIVE = "scenes/nonmq/Z2_LOST_WOODS/"
MM = "scenes/nonmq/skiejer_forest/skiejer_forest"
OOT = "scenes/shared/skiejer_forest_oot/skiejer_forest_oot"
MANIFEST = PRIVATE + "manifest.json"


def sha(data):
    return hashlib.sha256(data).hexdigest()


def string(value):
    value = value.encode()
    return struct.pack("<I", len(value)) + value


def command(number, data=b""):
    return struct.pack("<I", number) + data


def scene(commands):
    header = struct.pack("<B3xIIQ", 0, 0x4F524F4D, 0, 0xDEADBEEFDEADBEEF).ljust(64, b"\0")
    return header + struct.pack("<I", len(commands)) + b"".join(commands)


class Reader:
    def __init__(self, data):
        self.data, self.offset = data, 64

    def take(self, fmt):
        value = struct.unpack_from("<" + fmt, self.data, self.offset)
        self.offset += struct.calcsize("<" + fmt)
        return value[0] if len(value) == 1 else value

    def text(self):
        size = self.take("I")
        value = self.data[self.offset:self.offset + size].decode()
        self.offset += size
        return value


def native_commands(data):
    """Read every donor command using the actual MM factory field widths."""
    reader, result = Reader(data), {}
    for _ in range(reader.take("I")):
        identity, start = reader.take("I"), reader.offset
        if identity in (0, 1, 14):
            count = reader.take("I"); reader.offset += count * 16
        elif identity in (3, 26):
            reader.text()
        elif identity == 4:
            for _ in range(reader.take("I")):
                reader.text(); reader.take("II")
        elif identity == 6:
            count = reader.take("I"); reader.offset += count * 2
        elif identity == 7:
            reader.take("bh")
        elif identity == 8:
            reader.take("6b")
        elif identity == 10:
            assert reader.take("4B") == (0, 0, 1, 0)
            reader.text(); reader.text()
        elif identity in (11, 19):
            count = reader.take("I"); reader.offset += count * 2
        elif identity == 15:
            count = reader.take("I"); reader.offset += count * 22
        elif identity in (16, 21):
            reader.take("3B")
        elif identity == 17:
            reader.take("4B")
        elif identity == 18:
            reader.take("2B")
        elif identity in (20, 25):
            pass
        elif identity == 22:
            reader.take("B")
        elif identity == 24:
            for _ in range(reader.take("I")): reader.text()
        elif identity == 27:
            count = reader.take("I"); reader.offset += count * 16
        elif identity == 28:
            size = reader.take("I"); reader.take("H"); reader.offset += size * 10
        elif identity == 31:
            for _ in range(reader.take("B")):
                reader.text(); reader.take("HBB")
        else:
            raise ValueError(f"unsupported donor command {identity:#x}")
        result[identity] = data[start:reader.offset]
    if reader.offset != len(data):
        raise ValueError("donor scene did not consume EOF")
    return result


def playable_collision(original):
    data = bytearray(original)
    reader = Reader(data)
    reader.take("6h")
    count = reader.take("I"); reader.offset += count * 6
    count = reader.take("I"); reader.offset += count * 16
    count = reader.take("I"); reader.offset += count * 8
    cameras = reader.take("I")
    assert cameras == 3
    mapping = []
    for index in range(cameras):
        offset = reader.offset
        setting, count, points = reader.take("Hhi")
        assert count == 0 and points == 0
        mapping.append({"index": index, "from": setting, "to": 1})
        struct.pack_into("<H", data, offset, 1)
    return bytes(data), mapping


def convert(donor, native, oot=None):
    if sha(donor.read_bytes()) != DONOR_SHA or sha(native.read_bytes()) != NATIVE_SHA:
        raise ValueError("forest donors differ from the retained POC9/native references")
    resources, copied, removed = {}, {}, []
    with zipfile.ZipFile(donor) as source, zipfile.ZipFile(native) as base:
        assert source.testzip() is None and base.testzip() is None
        for name in source.namelist():
            if not name.startswith("alt/" + OLD): continue
            data = source.read(name)
            target = name.removeprefix("alt/").replace(OLD, PRIVATE)
            if data.startswith(b"<DisplayList"):
                tree = ET.fromstring(data)
                for node in tree:
                    if "Path" in node.attrib:
                        node.set("Path", node.get("Path").replace(OLD, PRIVATE))
                data = ET.tostring(tree, encoding="utf-8")
            resources[target] = data
            copied[target] = {"source": name, "source_sha256": sha(source.read(name)), "sha256": sha(data)}
        source_scene = native_commands(base.read(NATIVE + "Z2_LOST_WOODS"))
        collision, cameras = playable_collision(base.read(NATIVE + "Z2_LOST_WOODSCollisionHeader_005DF8"))
        transition = bytearray(source_scene[14])
        assert struct.unpack_from("<I", transition)[0] == 1
        for engine, prefix in (("mm", MM), ("oot", OOT)):
            resources[prefix + "_collision"] = collision
            portal = bytes(transition)
            if engine == "oot":
                struct.pack_into("<h", transition, 8, 0x23) # native En_Holl
                # MM stores degrees in bits 7..15 and an actor cutscene in
                # bits 0..6. SoH consumes a direct signed binary angle.
                degrees = (struct.unpack_from("<h", transition, 16)[0] >> 7) & 0x1FF
                yaw = int(degrees * (0x8000 / 180.0))
                struct.pack_into("<h", transition, 16, ((yaw + 0x8000) & 0xFFFF) - 0x8000)
                struct.pack_into("<H", transition, 18, 4 << 6) # horizontal, invisible
                portal = bytes(transition)
            # MM Actor_SpawnEntry decodes packed degrees/cutscene/day bits.
            # Use its nearest integer-degree heading (68 versus 67.5), no
            # actor cutscene, and every half-day. SoH uses direct binang.
            spawn = struct.pack("<HhhhhhhH", 0, -486, 0, -227,
                                0 if engine == "oot" else 7,
                                12288 if engine == "oot" else (68 << 7) | 127,
                                0 if engine == "oot" else 127,
                                0x0DFF if engine == "oot" else 0x0EFF)
            rooms = struct.pack("<I", 2) + b"".join(string(prefix + "_room_" + str(i)) + struct.pack("<II", 0, 0) for i in range(2))
            commands = [command(21, bytes([0, 4, 0x3E]) if engine == "oot" else source_scene[21]),
                        command(4, rooms), command(14, portal), command(3, string(prefix + "_collision")),
                        command(6, struct.pack("<Ibb", 1, 0, 0)), command(7, struct.pack("<bh", 0, 2)),
                        command(0, struct.pack("<I", 1) + spawn),
                        command(17, bytes([0, 0, 0, 0]) if engine == "oot" else source_scene[17]),
                        command(19, struct.pack("<IH", 1, 0x61C if engine == "oot" else 0xC000)),
                        command(15, source_scene[15]), command(20)]
            resources[prefix] = scene(commands)
            for room in range(2):
                name = f"alt/{NATIVE}Z2_LOST_WOODS_room_0{room}"
                source_room = native_commands(source.read(name))
                actors, original = [], source_room[1]
                for index in range(struct.unpack_from("<I", original)[0]):
                    row = list(struct.unpack_from("<HhhhhhhH", original, 4 + index * 16))
                    identity = row[0]
                    if identity in (0xD, 0x191, 0x192) or (engine == "oot" and identity != 0xB3):
                        removed.append({"engine": engine, "room": room, "id": identity, "position": row[1:4]})
                        continue
                    if engine == "oot":
                        row[0], row[4], row[5], row[6] = 0x151, 0, 0, 0
                    actors.append(struct.pack("<HhhhhhhH", *row))
                mesh = bytes([0, 0, 1, 0]) + string(PRIVATE + f"room_{room}/" + ("opa_combo_return_centered_v8" if room == 0 else "opa")) + string(PRIVATE + f"room_{room}/xlu")
                commands = [command(22, source_room[22]), command(8, struct.pack("<bi", 0, 0) if engine == "oot" else source_room[8]),
                            command(18, source_room[18]), command(16, bytes([255, 255, 0])), command(10, mesh),
                            command(11, struct.pack("<I", 0)), command(1, struct.pack("<I", len(actors)) + b"".join(actors)), command(20)]
                resources[prefix + "_room_" + str(room)] = scene(commands)
    title = {}
    if oot is not None:
        with zipfile.ZipFile(oot) as source:
            path = "textures/g_pn_36/gLostWoodsTitleCardENGTex"
            resources[PRIVATE + "title"] = source.read(path)
            title = {"archive_sha256": sha(oot.read_bytes()), "source": path, "sha256": sha(resources[PRIVATE + "title"])}
    receipt = {"version": 1, "donor_sha256": DONOR_SHA, "native_sha256": NATIVE_SHA,
               "preservation": "POC9 geometry/UVs/vertex colors/textures/material values and native collision topology/surfaces unchanged; resource paths private",
               "copied": copied, "converted": [p for p in resources if p.startswith("scenes/")],
               "camera_mapping": cameras, "removed_actors": removed, "title": title,
               "native_replacements": {"oot_en_holl": "MM24/params127 -> OoT35/params256; exact sides/position and decoded270-degree yaw retained",
                                       "player_spawn": "MM packed68degrees/cutscene127/allhalf-days; OoT binang12288; same safe position",
                                       "oot_obj_mure2": "MM179 -> OoT337, native rotation/param framing",
                                       "oot_audio": "native Lost Woods BGM62/Kokiri nature4",
                                       "oot_sky": "MM covered empty SKYBOX2 -> OoT SKYBOX_NONE0, outdoor; original8light settings"},
               "runtime": "unverified native scene candidate; no full executable or game traversal"}
    resources[MANIFEST] = (json.dumps(receipt, sort_keys=True, indent=2) + "\n").encode()
    return resources, receipt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("donor", type=Path)
    parser.add_argument("native", type=Path)
    parser.add_argument("oot", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--contract", type=Path)
    args = parser.parse_args()
    if args.output.exists() or args.output.resolve() in [p.resolve() for p in (args.donor, args.native, args.oot)]:
        raise ValueError("choose a new addon output; all donors stay immutable")
    resources, receipt = convert(args.donor, args.native, args.oot)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(args.output, "w", zipfile.ZIP_DEFLATED, compresslevel=6) as pack:
        for path, data in sorted(resources.items()):
            entry = zipfile.ZipInfo(path, date_time=(1980, 1, 1, 0, 0, 0))
            entry.create_system = 3
            entry.external_attr = (stat.S_IFREG | 0o644) << 16
            pack.writestr(entry, data, compress_type=zipfile.ZIP_DEFLATED, compresslevel=6)
    if args.contract:
        args.contract.write_text("#pragma once\n// Generated by tools/black_market/forest.py; dependency-closed native forest addon.\n"
                                 "static constexpr const char* kSoloForestResources[] = {\n" +
                                 "".join(f'    "{path}",\n' for path in sorted(resources)) + "};\n")
    print(f"PASS native solo forest addon: {len(resources)} resources, SHA256 {sha(args.output.read_bytes())}")


if __name__ == "__main__":
    main()
