#!/usr/bin/env python3
"""Inspect the real forest addon against immutable POC9/native scene donors."""
import argparse
import hashlib
import importlib.util
from pathlib import Path
import struct
import sys
import xml.etree.ElementTree as ET
import zipfile

ROOT = Path(__file__).resolve().parents[2]


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
        assert len(value.encode()) == size
        self.offset += size
        return value


def commands(data, oot):
    """Strict layouts consumed by the two native scene-command factories."""
    reader, result = Reader(data), {}
    for _ in range(reader.take("I")):
        cmd = reader.take("I")
        assert cmd not in result
        if cmd in (0, 1):
            value = [reader.take("HhhhhhhH") for _ in range(reader.take("I"))]
        elif cmd == 3:
            value = reader.text()
        elif cmd == 4:
            value = [(reader.text(), reader.take("II")) for _ in range(reader.take("I"))]
        elif cmd == 6:
            value = [reader.take("bb") for _ in range(reader.take("I"))]
        elif cmd == 7:
            value = reader.take("bh")
        elif cmd == 8:
            value = reader.take("bi" if oot else "6b")
        elif cmd == 10:
            value = (reader.take("4B"), reader.text(), reader.text())
        elif cmd == 11:
            value = [reader.take("h") for _ in range(reader.take("I"))]
        elif cmd == 14:
            value = [reader.take("BBBBhhhhhH") for _ in range(reader.take("I"))]
        elif cmd == 15:
            value = [reader.take("18Bhh") for _ in range(reader.take("I"))]
        elif cmd in (16, 21):
            value = reader.take("3B")
        elif cmd == 17:
            value = reader.take("4B")
        elif cmd == 18:
            value = reader.take("2B")
        elif cmd == 19:
            value = [reader.take("H") for _ in range(reader.take("I"))]
        elif cmd == 20:
            value = None
        elif cmd == 22:
            value = reader.take("B")
        elif cmd == 25 and oot:
            value = reader.take("bi")
        else:
            raise AssertionError(f"unsupported or prologue command {cmd:#x}")
        result[cmd] = value
    assert reader.offset == len(data), "scene schema did not consume EOF"
    return result


def collision(data):
    reader = Reader(data)
    bounds = reader.take("6h")
    vertices = [reader.take("3h") for _ in range(reader.take("I"))]
    polygons = [reader.take("8H") for _ in range(reader.take("I"))]
    surfaces = [reader.take("II") for _ in range(reader.take("I"))]
    cameras = [reader.take("Hhi") for _ in range(reader.take("I"))]
    points = [reader.take("3h") for _ in range(reader.take("I"))]
    water = [reader.take("5hi") for _ in range(reader.take("I"))]
    assert reader.offset == len(data)
    return bounds, vertices, polygons, surfaces, cameras, points, water


def verify(donor, native, resources):
    old_prefix, new_prefix = "custom/cor_mm3d/lost_woods_poc1/", "custom/skiejer_forest/"
    with zipfile.ZipFile(donor) as source, zipfile.ZipFile(native) as base:
        # Native geometry, UVs, colors, images and material commands are retained;
        # only logical resource identities change to make the port self-contained.
        expected = set()
        for name in source.namelist():
            if not name.startswith("alt/" + old_prefix):
                continue
            path = name.removeprefix("alt/").replace(old_prefix, new_prefix)
            expected.add(path)
            original, actual = source.read(name), resources[path]
            if original.startswith(b"<DisplayList"):
                before, after = ET.fromstring(original), ET.fromstring(actual)
                for command in before:
                    if "Path" in command.attrib:
                        command.set("Path", command.get("Path").replace(old_prefix, new_prefix))
                assert ET.tostring(before) == ET.tostring(after), path
            else:
                assert actual == original, f"changed physical donor data: {path}"
        assert set(p for p in resources if p.startswith(new_prefix) and p not in (new_prefix + "manifest.json", new_prefix + "title")) == expected
        for path in expected:
            raw = resources[path]
            if raw.startswith(b"<DisplayList"):
                for node in ET.fromstring(raw):
                    if "Path" in node.attrib:
                        assert node.get("Path") in resources, node.get("Path")
        original_collision = collision(base.read("scenes/nonmq/Z2_LOST_WOODS/Z2_LOST_WOODSCollisionHeader_005DF8"))
        for engine in ("mm", "oot"):
            prefix = "scenes/nonmq/skiejer_forest/skiejer_forest" if engine == "mm" else "scenes/shared/skiejer_forest_oot/skiejer_forest_oot"
            scene = commands(resources[prefix], engine == "oot")
            assert len(scene[4]) == 2 and scene[6] == [(0, 0)]
            assert scene[0][0][7] == (0x0DFF if engine == "oot" else 0x0EFF)
            assert scene[0][0][1:4] == (-486, 0, -227), "safe POC9 stump arrival changed"
            assert scene[0][0][5] == (12288 if engine == "oot" else (68 << 7) | 127)
            assert scene[19] == ([0x61C] if engine == "oot" else [0xC000])
            transition = scene[14]
            assert len(transition) == 1 and transition[0][:4] == (1, 255, 0, 255)
            assert transition[0][4:9] == ((0x23 if engine == "oot" else 0x18), 800, 0, 0,
                                        -16384 if engine == "oot" else -30849)
            assert transition[0][9] == (0x100 if engine == "oot" else 0x7F)
            candidate = collision(resources[scene[3]])
            assert candidate[:4] == original_collision[:4], "collision shape/surfaces changed"
            assert candidate[5:] == original_collision[5:]
            assert candidate[4] == [(1, 0, 0)] * 3
            for room in range(2):
                data = commands(resources[prefix + "_room_" + str(room)], engine == "oot")
                assert data[16] == (255, 255, 0), "forest changed saved day/time"
                assert data[10][0] == (0, 0, 1, 0)
                assert data[10][1] == new_prefix + f"room_{room}/" + ("opa_combo_return_centered_v8" if room == 0 else "opa")
                actors = data[1]
                assert all(actor[0] not in (0xD, 0x191, 0x192) for actor in actors), "intro actor survived"
                if engine == "oot":
                    assert all(actor[0] == 0x151 for actor in actors), "MM actor ID leaked into OoT"
                else:
                    assert [actor[0] for actor in actors] == ([0xB3, 0xB3, 0x49] if room == 0 else [0x49, 0x171])
    print("PASS solo forest: exact POC9 scenery, closed private dependencies, native two-room schemas, collision and no prologue")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("donor", type=Path)
    parser.add_argument("native", type=Path)
    args = parser.parse_args()
    module_path = ROOT / "tools/black_market/forest.py"
    assert module_path.exists(), "native solo forest conversion is missing"
    spec = importlib.util.spec_from_file_location("forest", module_path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    original_hashes = [hashlib.sha256(p.read_bytes()).hexdigest() for p in (args.donor, args.native)]
    resources, receipt = module.convert(args.donor, args.native)
    verify(args.donor, args.native, resources)
    assert receipt["donor_sha256"] == original_hashes[0] and receipt["native_sha256"] == original_hashes[1]
    assert [hashlib.sha256(p.read_bytes()).hexdigest() for p in (args.donor, args.native)] == original_hashes


if __name__ == "__main__":
    main()
