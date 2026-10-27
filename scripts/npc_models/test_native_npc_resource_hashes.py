"""All confirmed NativeNpcV1 families must emit runtime-resolvable hashes."""
import struct
import tempfile
import unittest
import zipfile
from pathlib import Path
from native_assets import o2r as asset
from repair_saria_resource_hashes import repair_records


class NativeNpcHashTests(unittest.TestCase):
    def test_existing_twins_and_malon_wrappers_bind_their_literal_entries(self):
        for family in ("object_kw1", "object_ma1", "object_ma2"):
            with self.subTest(family=family):
                root = "objects/" + family + "/NativeNpcV1/"
                target = root + "Body/Limb14"
                wrapper = asset.DLBuilder()
                wrapper.dl_hash(target)
                wrapper.end()
                raw = wrapper.to_resource()
                records = {"alt/" + root + "Faces/L0R0M0": raw, "alt/" + target: b"mesh"}
                repaired, changes = repair_records(records)
                self.assertEqual(len(changes), 1, "This family has the same unindexed canonical-hash failure")
                actual = asset.crc64("alt/" + target)
                self.assertEqual(struct.unpack_from("<II", repaired["alt/" + root + "Faces/L0R0M0"], 80),
                                 (actual >> 32, actual & 0xffffffff))

    def test_production_writer_cannot_recreate_unindexed_private_hashes(self):
        for family in ("object_sa", "object_kw1", "object_ma1", "object_ma2"):
            with self.subTest(family=family), tempfile.TemporaryDirectory() as directory:
                root = "objects/" + family + "/NativeNpcV1/"
                target = root + "Body/Limb14"
                wrapper = asset.DLBuilder()
                wrapper.dl_hash(target)
                wrapper.end()
                raw = wrapper.to_resource()
                archive = asset.O2R()
                archive.add("alt/" + root + "Faces/L0R0M0", raw)
                archive.add("alt/" + target, b"mesh")
                output = Path(directory) / "model.o2r"
                archive.write(output, compression=zipfile.ZIP_DEFLATED, timestamp=(2026, 10, 18, 0, 0, 0))
                with zipfile.ZipFile(output) as written:
                    self.assertEqual(written.namelist(), sorted(archive.files))
                    self.assertTrue(all(info.compress_type == zipfile.ZIP_DEFLATED and
                                        info.date_time == (2026, 10, 18, 0, 0, 0) for info in written.infolist()))
                    data = written.read("alt/" + root + "Faces/L0R0M0")
                    high, low = struct.unpack_from("<II", data, 80)
                    literal = {asset.crc64(name) for name in written.namelist()}
                    self.assertIn(high << 32 | low, literal,
                                  "O2R.write must bind final literal entries, without synthetic Alt aliases")
                self.assertEqual(archive.files["alt/" + root + "Faces/L0R0M0"], raw)

    def test_writer_defaults_keep_unrelated_stored_resource_order(self):
        with tempfile.TemporaryDirectory() as directory:
            archive = asset.O2R(files={"z": b"last alphabetically", "a": b"first alphabetically"})
            output = Path(directory) / "default.o2r"
            archive.write(output)
            with zipfile.ZipFile(output) as written:
                self.assertEqual(written.namelist(), ["z", "a"])
                self.assertTrue(all(info.compress_type == zipfile.ZIP_STORED for info in written.infolist()))
                self.assertEqual({name: written.read(name) for name in written.namelist()}, archive.files)

    def test_native_and_unrelated_hashes_remain_exact(self):
        root = "objects/object_ma2/NativeNpcV1/"
        native = "objects/object_ma2/gMalonAdultBasketTex"
        unrelated = "objects/not_a_native_npc/Mesh"
        wrapper = asset.DLBuilder()
        wrapper.settimg_hash(0, 2, 32, native)
        wrapper.dl_hash(unrelated)
        wrapper.end()
        records = {"alt/" + root + "Body/Limb01": wrapper.to_resource(),
                   "alt/" + native: b"native replacement", "alt/" + unrelated: b"other"}
        self.assertEqual(repair_records(records), (records, []))


if __name__ == "__main__":
    unittest.main()
