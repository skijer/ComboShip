#!/usr/bin/env python3
"""Exercise the native market bake with the immutable real POC14 resources."""
import base64
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import tempfile
import unittest
import xml.etree.ElementTree as ET
import zipfile

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT.parent / "inputs/zzzz_Skiejer_Black_Market_POC14_EntranceOrientation.o2r"
TOOL = ROOT / "tools/black_market/native_market.py"
PREPARE = ROOT / "scripts/assets/prepare_native_market.py"
SOURCE_SHA = "3ac1432d0e8f75e51f08892739a13e16762bb4d820a2bbc7a35469af1db872e2"
STOCK_TEXTURE = "scenes/nonmq/Z2_26SARUNOMORI/Z2_26SARUNOMORITex_00BAC0"
MARKER = "custom/skiejer_market/native_geometry_v1"
MM_SCENE = "scenes/nonmq/skiejer_market/skiejer_market"
OOT_SCENE = "scenes/shared/skiejer_market_oot/skiejer_market_oot"


def digest(data):
    return hashlib.sha256(data).hexdigest()


def load(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def retained(path):
    return (path.startswith("custom/skiejer_market/geometry/") or
            path.startswith("custom/skiejer_market/entrance") or
            path == "custom/skiejer_market/room" or
            path.startswith("scenes/nonmq/skiejer_market/") or
            path.startswith("scenes/shared/skiejer_market_oot/"))


class NativeMarketAssets(unittest.TestCase):
    def tool(self):
        self.assertTrue(TOOL.is_file(), "Missing native market geometry-only transformer")
        return load(TOOL, "native_market_test_builder")

    def prepare_tool(self):
        self.assertTrue(PREPARE.is_file(), "Missing native market build preparation")
        return load(PREPARE, "native_market_test_prepare")

    def test_real_geometry_collision_headers_and_uvs_survive_without_texture_payloads(self):
        module = self.tool()
        self.assertEqual(digest(SOURCE.read_bytes()), SOURCE_SHA)
        resources, receipt = module.convert(SOURCE)
        with zipfile.ZipFile(SOURCE) as original:
            expected = {name for name in original.namelist() if retained(name)}
            self.assertEqual(set(resources), expected | {MARKER})
            self.assertEqual(len(expected), 124)
            for name in expected:
                if "/mat_" not in name:
                    self.assertEqual(resources[name], original.read(name), name)
        self.assertEqual(receipt["source_sha256"], SOURCE_SHA)
        self.assertEqual(receipt["material_count"], 28)
        self.assertEqual(receipt["external_resources"], [STOCK_TEXTURE])
        self.assertNotIn(STOCK_TEXTURE, resources)
        for name, data in resources.items():
            self.assertFalse(data[4:8] in (b"XETO", b"OTEX"), name)
            self.assertNotIn("/keeper/", name)
            self.assertNotIn("/texture/", name)
            self.assertNotEqual(name, "custom/skiejer_market/title")
            self.assertNotEqual(name, "CosmeticEntries")

    def test_all_materials_are_static_stock_rgba16_and_xml_dependencies_close(self):
        resources, _ = self.tool().convert(SOURCE)
        materials = []
        for name, data in resources.items():
            if not data.lstrip().startswith(b"<"):
                continue
            tree = ET.fromstring(data)
            for command in tree.iter():
                self.assertNotEqual(command.tag, "ScrollTexture", name)
                self.assertNotIn("CosmeticEntry", command.attrib, name)
                self.assertNotIn("CosmeticCategory", command.attrib, name)
                path = command.get("Path")
                if path:
                    self.assertIn(path, set(resources) | {STOCK_TEXTURE}, (name, path))
            if "/mat_" not in name:
                continue
            materials.append(name)
            image = tree.find("SetTextureImage")
            self.assertEqual(image.get("Path"), STOCK_TEXTURE)
            self.assertEqual(image.get("Format"), "G_IM_FMT_RGBA")
            for tile in tree.findall("SetTile"):
                self.assertEqual((tile.get("MaskS"), tile.get("MaskT")), ("5", "5"))
                self.assertEqual((tile.get("ShiftS"), tile.get("ShiftT")), ("0", "0"))
                if tile.get("Tile") == "0":
                    self.assertEqual(tile.get("Line"), "8")
                    self.assertEqual(tile.get("Size"), "G_IM_SIZ_16b")
            block = tree.find("LoadBlock")
            self.assertEqual((block.get("Lrs"), block.get("Dxt")), ("1023", "256"))
            size = tree.find("SetTileSize")
            self.assertEqual((size.get("Lrs"), size.get("Lrt")), ("124", "124"))
            color = tree.find("SetPrimColor")
            self.assertEqual([color.get(c) for c in "RGBA"], ["255"] * 4)
        self.assertEqual(len(materials), 28)

    def test_core_contract_requires_only_current_host_and_native_keeper_essentials(self):
        module = self.tool()
        resources, _ = module.convert(SOURCE)
        for host, own, other in [("mm", MM_SCENE, OOT_SCENE), ("oot", OOT_SCENE, MM_SCENE)]:
            contract = module.core_contract(resources, host)
            self.assertIn(own, contract)
            self.assertIn(own + "_room_0", contract)
            self.assertNotIn(other, contract)
            self.assertNotIn(other + "_room_0", contract)
            self.assertIn(MARKER, contract)
            self.assertNotIn(STOCK_TEXTURE, contract)
            for name in ('baseline_timber', 'baseline_slate', 'baseline_plaster', 'arcane_stone'):
                self.assertIn('custom/skiejer_market/texture/' + name, contract)
            self.assertIn('custom/skiejer_market/title', contract)
            for name in ("gTreasureChestShopGalSkel", "object_bg_Anim_009890",
                         "gTreasureChestShopGalEyeOpenUpTex", "gTreasureChestShopGalEyeHalfUpTex",
                         "gTreasureChestShopGalEyeClosedTex", "gTreasureChestShopGalTLUT"):
                self.assertIn("objects/object_bg/" + name, contract)
            self.assertFalse(any("/keeper/" in name or
                                 name == "CosmeticEntries" for name in contract))
        header = module.core_contract_header(resources)
        for host in ("Mm", "Oot"):
            body = re.search(r"kNativeBlackMarket" + host + r"Resources\[\] = \{(.*?)\};", header, re.S)
            self.assertIsNotNone(body)
            self.assertEqual(re.findall(r'"([^"]+)"', body[1]), module.core_contract(resources, host.lower()))

    def test_checked_in_bundle_reproduces_the_real_geometry_transform(self):
        module = self.tool()
        bundled = ROOT / "tools/native_market/bundled"
        self.assertTrue((bundled / "manifest.json").is_file(), "Missing checked-in geometry source bundle")
        resources, _ = module.convert(SOURCE)
        self.assertEqual(module.read_bundle(bundled), resources)
        with tempfile.TemporaryDirectory(prefix="native-market-rebuild-") as directory:
            rebuilt = Path(directory)
            _, receipt = module.convert(SOURCE)
            module.write_bundle(resources, receipt, rebuilt)
            self.assertEqual({p.name: p.read_bytes() for p in rebuilt.iterdir()},
                             {p.name: p.read_bytes() for p in bundled.iterdir()})

    def test_bundle_rebuild_is_identical_and_prepare_repairs_only_bad_outputs(self):
        module, prepare = self.tool(), self.prepare_tool()
        resources, receipt = module.convert(SOURCE)
        with tempfile.TemporaryDirectory(prefix="native-market-") as directory:
            root = Path(directory)
            first, second = root / "first", root / "second"
            module.write_bundle(resources, receipt, first)
            module.write_bundle(resources, receipt, second)
            self.assertEqual({p.name: p.read_bytes() for p in first.iterdir()},
                             {p.name: p.read_bytes() for p in second.iterdir()})
            output = root / "output"
            prepared = prepare.prepared_resources(first)
            prepared_by_game = {'soh': prepared, 'mm': prepare.prepared_resources(first, host='mm')}
            prepare.prepare(output, first)
            targets = [output / game / "assets/custom" / name
                       for game in ("soh", "mm") for name in prepared]
            for path in targets:
                name = path.relative_to(output / path.relative_to(output).parts[0] / "assets/custom").as_posix()
                self.assertEqual(path.read_bytes(), prepared_by_game[path.relative_to(output).parts[0]][name])
            times = {path: path.stat().st_mtime_ns for path in targets}
            prepare.prepare(output, first)
            self.assertEqual(times, {path: path.stat().st_mtime_ns for path in targets})
            damaged = output / "mm/assets/custom/custom/skiejer_market/room"
            damaged.write_bytes(b"damaged")
            prepare.prepare(output, first)
            self.assertEqual(damaged.read_bytes(), resources["custom/skiejer_market/room"])
            self.assertEqual(times[targets[0]], targets[0].stat().st_mtime_ns)

    def test_corrupt_bundle_and_changed_donor_fail_before_writing_assets(self):
        module, prepare = self.tool(), self.prepare_tool()
        resources, receipt = module.convert(SOURCE)
        with tempfile.TemporaryDirectory(prefix="native-market-reject-") as directory:
            root = Path(directory)
            wrong = root / "wrong.o2r"
            wrong.write_bytes(b"different donor")
            with self.assertRaisesRegex(ValueError, "POC14|donor|identity"):
                module.convert(wrong)
            bundle = root / "bundle"
            module.write_bundle(resources, receipt, bundle)
            metadata = json.loads((bundle / "manifest.json").read_text())
            packed_path = bundle / metadata["file"]
            packed = bytearray(base64.b64decode(packed_path.read_bytes(), validate=True))
            packed[len(packed) // 2] ^= 1
            packed_path.write_bytes(base64.b64encode(packed))
            output = root / "output"
            with self.assertRaisesRegex(ValueError, "hash|size|corrupt"):
                prepare.prepare(output, bundle)
            self.assertFalse(output.exists())


if __name__ == "__main__":
    unittest.main()
