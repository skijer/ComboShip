"""Match the engine's literal archive hash index, without invented Alt aliases."""
import tempfile
import unittest
import zipfile
from pathlib import Path
import importlib.util
import struct
import os
import re

from native_assets import o2r as asset
from verify_native_npc_models import Archive


class SariaHashTests(unittest.TestCase):
    @unittest.skipUnless(os.environ.get("SARIA_PACK"), "SARIA_PACK supplies a real model archive")
    def test_supplied_pack_references_resolve_against_literal_archive_names(self):
        with zipfile.ZipFile(os.environ["SARIA_PACK"]) as archive:
            records = {name: archive.read(name) for name in archive.namelist()}
        header = Path(__file__).resolve().parents[2] / "soh/assets/objects/object_sa/object_sa.h"
        native_names = re.findall(r'"__OTR__([^"\n]+)"', header.read_text())
        hashes = {asset.crc64(name) for name in [*records, *native_names]}
        missing = []
        for name, data in records.items():
            if data[4:8] != b"TLDO":
                continue
            offset = 72
            while offset + 8 <= len(data):
                opcode = struct.unpack_from("<I", data, offset)[0] >> 24
                if opcode in (0x20, 0x31, 0x32, 0x33, 0x35, 0x36, 0x42):
                    high, low = struct.unpack_from("<II", data, offset + 8)
                    hashed = high << 32 | low
                    if opcode != 0x33 and hashed not in hashes:
                        missing.append(f"{name}: {hashed:016x}")
                    offset += 16
                else:
                    offset += 8
                if opcode == 0xDF:
                    break
        self.assertEqual(len(missing), 0, f"unresolved dependencies; first: {missing[:1]}")

    def test_offline_index_does_not_invent_a_canonical_private_head(self):
        with tempfile.TemporaryDirectory() as directory:
            native = Path(directory) / "native.zip"
            candidate = Path(directory) / "candidate.o2r"
            head = "objects/object_sa/NativeNpcV1/Body/Limb15"
            with zipfile.ZipFile(native, "w"):
                pass
            with zipfile.ZipFile(candidate, "w") as archive:
                archive.writestr("alt/" + head, b"head")
            lookup = Archive(native, candidate)
            self.assertIn(asset.crc64("alt/" + head), lookup.hashes)
            self.assertNotIn(asset.crc64(head), lookup.hashes,
                             "Archive::IndexFile indexes literal names; this hash cannot resolve in game")

    def test_repair_binds_head_hands_and_prop_to_real_entries(self):
        spec = importlib.util.find_spec("repair_saria_resource_hashes")
        self.assertIsNotNone(spec, "Saria needs a bounded repair of unresolved private wrapper hashes")
        from repair_saria_resource_hashes import repair_records
        ns = "objects/object_sa/NativeNpcV1"
        head, hand, prop = [ns + suffix for suffix in
                            ("/Body/Limb15", "/Body/Limb14", "/Props/OcarinaGrip")]
        native = "objects/object_sa/gSariaFairyOcarinaTex"
        display = asset.DLBuilder()
        display.marker("alt/" + ns + "/Faces/L0R0M0")
        for part in (head, hand, prop):
            display.dl_hash(part)
        display.settimg_hash(0, 2, 32, native)
        display.end()
        wrapper = "alt/" + ns + "/Faces/L0R0M0"
        original = {wrapper: display.to_resource(), **{"alt/" + p: b"geometry" for p in (head, hand, prop)}}
        repaired, changes = repair_records(original)
        self.assertEqual(len(changes), 3)
        for part in (head, hand, prop):
            hashed = asset.crc64("alt/" + part)
            self.assertIn(struct.pack("<II", hashed >> 32, hashed & 0xffffffff), repaired[wrapper])
            self.assertEqual(repaired["alt/" + part], original["alt/" + part])
        hashed = asset.crc64(native)
        self.assertIn(struct.pack("<II", hashed >> 32, hashed & 0xffffffff), repaired[wrapper])
        self.assertEqual(repair_records(repaired), (repaired, []))
        self.assertEqual(original[wrapper], display.to_resource(), "Accepted input was mutated")


if __name__ == "__main__":
    unittest.main()
