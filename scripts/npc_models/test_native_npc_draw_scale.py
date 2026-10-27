"""Archive-owned visual scale is an additive, deterministic resource only."""
import hashlib
import json
import math
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
import zipfile

SCRIPT = Path(__file__).with_name("set_native_npc_draw_scale.py")


class DrawScaleArchiveTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)

    def source(self, model="young-malon"):
        namespace = {"young-malon": "objects/object_ma1/NativeNpcV1", "saria": "objects/object_sa/NativeNpcV1",
                     "young-fado": "objects/object_fa/YoungFadoNpcPOC1"}[model]
        data = {namespace + "/Skel": b"base-rig", "alt/" + namespace + "/Skel": b"private-rig",
                "alt/" + namespace + "/Body/Limb14": b"mesh", "untouched/animation": b"native-timing-and-root",
                "untouched/texture": b"local-pixels", "alt/" + namespace + "/Props/Basket": b"prop"}
        source = self.root / "input.o2r"
        with zipfile.ZipFile(source, "w") as archive:
            for name, value in data.items():
                archive.writestr(name, value)
        return source, namespace, data

    def invoke(self, source, output, model="young-malon", scale="1.10"):
        self.assertTrue(SCRIPT.is_file(), "DrawScale archive writer is missing")
        return subprocess.run([sys.executable, str(SCRIPT), "--input", str(source), "--output", str(output),
                               "--model", model, "--scale", scale], capture_output=True, text=True)

    def test_adds_only_scale_resource_for_requested_models(self):
        for model in ["young-malon", "saria", "young-fado"]:
            source, namespace, data = self.source(model)
            before = hashlib.sha256(source.read_bytes()).hexdigest()
            output = self.root / (model + ".o2r")
            expected_factor = 1.25 if model == "young-fado" else 1.10
            result = self.invoke(source, output, model, str(expected_factor))
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(hashlib.sha256(source.read_bytes()).hexdigest(), before)
            with zipfile.ZipFile(output) as archive:
                path = "alt/" + namespace + "/DrawScale"
                self.assertEqual(set(archive.namelist()), {*data, path})
                for name, value in data.items():
                    self.assertEqual(archive.read(name), value, name)
                raw = archive.read(path)
                self.assertEqual(raw[4:8], b"RRAO")
                self.assertEqual(struct.unpack_from("<II", raw, 64), (16, 4))
                words = []
                for offset in range(72, len(raw), 6):
                    scalar_type, value = struct.unpack_from("<IH", raw, offset)
                    self.assertEqual(scalar_type, 5)
                    words.append(value)
                self.assertEqual(words[:2], [0x4e44, 1])
                factor = struct.unpack("<f", struct.pack("<HH", *words[2:]))[0]
                self.assertAlmostEqual(factor, expected_factor, places=6)
            report = json.loads(result.stdout)
            self.assertEqual(report["changed_entries"], [path])
            self.assertEqual(report["source_sha256"], before)
            self.assertFalse(report["runtime_tested"])
            self.assertTrue(report["requires_new_build"])

    def test_reproducible_output(self):
        source, _, _ = self.source()
        first, second = self.root / "first.o2r", self.root / "second.o2r"
        self.assertEqual(self.invoke(source, first).returncode, 0)
        self.assertEqual(self.invoke(source, second).returncode, 0)
        self.assertEqual(first.read_bytes(), second.read_bytes())

    def test_rejects_invalid_scale_without_output(self):
        source, _, _ = self.source()
        for factor in ["0", "-1", "0.49", "2.01", "nan", "inf", "-inf"]:
            output = self.root / (factor + ".o2r")
            result = self.invoke(source, output, scale=factor)
            self.assertNotEqual(result.returncode, 0)
            self.assertFalse(output.exists())

    def test_rejects_wrong_model_and_in_place_write(self):
        source, _, _ = self.source()
        before = source.read_bytes()
        output = self.root / "wrong.o2r"
        self.assertNotEqual(self.invoke(source, output, "saria").returncode, 0)
        self.assertFalse(output.exists())
        self.assertNotEqual(self.invoke(source, source).returncode, 0)
        self.assertEqual(source.read_bytes(), before)


if __name__ == "__main__":
    unittest.main()
