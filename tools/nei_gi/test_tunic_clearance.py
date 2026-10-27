"""Check belt/harness clearance against real quantized torso triangles.

The ray/triangle measurement is independent of the garment-fitting recipe.
Check both rebuilt recipes and installed checkpoints to catch stale exports.
"""
import json
from pathlib import Path
import struct
import sys
import unittest

import numpy as np

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / "SOURCE"))
import equipment_revamp as equipment

SLUGS = ("champions_tunic", "sages_tunic", "spirit_breastplate")
BODY_NAMES = ("Blue woven Champion tunic", "White woven sage robe",
              "Fitted brown Spirit leather torso")
BELT_NAMES = ("Fitted waist belt", "Waist belt sewn piping",
              "Square waist buckle rim", "Waist buckle cross pin")
HARNESS_NAMES = ("Champion rear leather seam strap",
                 "Champion rear harness stitched edge")


def checkpoint_parts(slug):
    raw = (ROOT / "CHECKPOINTS" / slug / (slug + ".glb")).read_bytes()
    size = struct.unpack_from("<I", raw, 12)[0]
    doc = json.loads(raw[20:20 + size])
    binary = raw[28 + size:]

    def accessor(index):
        item = doc["accessors"][index]
        view = doc["bufferViews"][item["bufferView"]]
        columns = {"SCALAR": 1, "VEC3": 3}[item["type"]]
        dtype = {5125: "<u4", 5126: "<f4"}[item["componentType"]]
        offset = view.get("byteOffset", 0) + item.get("byteOffset", 0)
        return np.frombuffer(binary, dtype=dtype, count=item["count"] * columns,
                             offset=offset).reshape(-1, columns)

    parts = []
    for mesh in doc["meshes"]:
        for primitive in mesh["primitives"]:
            parts.append({"name": mesh["name"],
                          "p": accessor(primitive["attributes"]["POSITION"]).astype(float) / 16,
                          "tri": accessor(primitive["indices"]).reshape(-1, 3)})
    return parts


def surface_samples(part):
    """Vertices, edge thirds and interior samples on every exported triangle."""
    weights = np.array([(a / 3, b / 3, (3 - a - b) / 3)
                        for a in range(4) for b in range(4 - a)])
    faces = part["p"][part["tri"]]
    return np.unique(np.einsum("wk,fkj->fwj", weights, faces).reshape(-1, 3), axis=0)


def radial_clearance(body, points):
    """Intersect horizontal radial rays with actual cloth faces, in batches."""
    faces = body["p"][body["tri"]]
    edge1, edge2 = faces[:, 1] - faces[:, 0], faces[:, 2] - faces[:, 0]
    gaps = []
    for start in range(0, len(points), 128):
        sample = points[start:start + 128]
        radius = np.hypot(sample[:, 0], sample[:, 2])
        direction = sample * [1, 0, 1] / radius[:, None]
        origin = sample * [0, 1, 0]
        pvec = np.cross(direction[:, None], edge2)
        det = np.einsum("tj,ntj->nt", edge1, pvec)
        valid = np.abs(det) > 1e-9
        inverse = np.divide(1, det, out=np.zeros_like(det), where=valid)
        tvec = origin[:, None] - faces[:, 0]
        u = np.einsum("ntj,ntj->nt", tvec, pvec) * inverse
        qvec = np.cross(tvec, edge1)
        v = np.einsum("nj,ntj->nt", direction, qvec) * inverse
        distance = np.einsum("tj,ntj->nt", edge2, qvec) * inverse
        hit = valid & (u >= -1e-8) & (v >= -1e-8) & (u + v <= 1 + 1e-8) & (distance >= 0)
        skin = np.max(np.where(hit, distance, -np.inf), axis=1)
        if not np.isfinite(skin).all():
            raise AssertionError("clearance ray missed torso")
        gaps.extend(radius - skin)
    return np.array(gaps)


class TunicClearance(unittest.TestCase):
    def assert_clearance(self, parts, names, minimum):
        body = next(part for part in parts if part["name"] in BODY_NAMES)
        targets = [part for part in parts if part["name"] in names]
        self.assertTrue(targets, "missing fitting geometry")
        for index, part in enumerate(targets):
            with self.subTest(part=part["name"], index=index):
                gap = float(radial_clearance(body, surface_samples(part)).min())
                self.assertGreaterEqual(gap, minimum,
                                        f"{part['name']} penetrates/approaches cloth: {gap:.3f} author units")

    def test_recipe_waist_belts_clear_cloth(self):
        for slug in SLUGS:
            with self.subTest(slug=slug):
                self.assert_clearance(equipment.BUILDERS[slug]().parts, BELT_NAMES, .25)

    def test_installed_waist_belts_clear_cloth(self):
        for slug in SLUGS:
            with self.subTest(slug=slug):
                self.assert_clearance(checkpoint_parts(slug), BELT_NAMES, .25)

    def test_recipe_champion_harness_clears_cloth(self):
        self.assert_clearance(equipment.champions_tunic().parts, HARNESS_NAMES, .20)

    def test_installed_champion_harness_clears_cloth(self):
        self.assert_clearance(checkpoint_parts("champions_tunic"), HARNESS_NAMES, .20)


if __name__ == "__main__":
    unittest.main()
