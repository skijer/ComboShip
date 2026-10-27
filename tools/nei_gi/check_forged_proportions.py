"""Check supplied sword-image proportions without fitting models to a viewport.

Steel Master and gold True Master ratios come from the supplied user images.
The latest visual review widened Master's blade root and shortened Gilded.
Fairy's existing component ratios use stock held geometry as secondary evidence.
No ratio below is presented as recovered Nintendo get-item vertices.
"""
import hashlib
import json
from pathlib import Path
import sys
import tempfile

import numpy as np

ROOT = Path(__file__).resolve().parent
TOOLKIT = ROOT / "SOURCE" / "forged_swords"
REPO = ROOT.parents[1]
sys.path.insert(0, str(TOOLKIT / "SOURCE"))
import preview
import sword_forged


# Captured before the Master-root/Gilded-length adjustment. The other seven
# models, including the accepted True Master, remain byte-for-byte intact.
PROTECTED = {
    "kokiri_sword": ("a86974807df75b3f35c0eb83d88b3fae95bc50086d0ba3e7da6fe32cdec5e7be",
                     "b973501bd4fe6d8370cb7f83e580d1a7f6807d8824f2c1e92a12729fda909028"),
    "mm_kokiri_sword": ("b30ca60f9ba459334d225072722a210f747664ec8970489304cf94357412911f",
                        "ec07cafe47d911f98ad89f6c614a352906df573d6f5f6174b306b110df622845"),
    "four_sword": ("23d165c71c6a1b4119bdf6115f538cb05a96c2534b43b239702550e8e464e527",
                   "bfc8b5c88a64ae43a667e0d4384fce3a4ef509d04be8cdd13467410b01950c1f"),
    "great_fairy_sword": ("50cab76ab68bd38161a97085b2b20c4d71390a502e75cb7700db3530a2037e7f",
                          "c9ea30f9c3a0789549b660e78090bffa445dbac4ecba6698d596bca9d9497abd"),
    "biggoron_sword": ("51e20f8a88a7690d9ef210ab1eafe871c6309feb035990500f6bed8c44895f69",
                       "501d4d7e81cfe13bb0e4897e846f7e5ca8db66c6b69519408fcd8b162c7ae5a4"),
    "razor_sword": ("c2a7c6bedb5167588e36b1d102936d8c5f5977a2ce7a56b0122c4e7e452162cf",
                    "5e3df076b964b0c6aacfab38d3bb9824647a14af09898b224e6c5e2d45113421"),
    "true_master_sword": ("86eb15acd183d082dbee2ea1144eb21733d9b6a3d5292a0f296a8a2091b4600e",
                          "e6909ac20ace877e62a3f21b3b49962526210afc4e585fd99bb994d80932a9f6"),
}


def positions(model, names=None):
    parts = model.parts if names is None else [p for p in model.parts if p["name"] in names]
    return np.concatenate([part["p"] for part in parts])


def span(model, axis, names=None):
    return float(np.ptp(positions(model, names)[:, axis]))


def width_at(points, y):
    row = points[np.isclose(points[:, 1], y, atol=.032)]
    assert len(row), ("missing blade landmark", y)
    return float(np.ptp(row[:, 0]))


def check():
    models = {slug: sword_forged.build(slug) for slug in
              ("master_sword", "true_master_sword", "gilded_sword", "great_fairy_sword")}
    master, true, gilded = (models[s] for s in ("master_sword", "true_master_sword", "gilded_sword"))
    blade_names = {"Forged blade cutting bevel", "Forged blade fuller face"}
    master_points, true_points = (positions(m, blade_names) for m in (master, true))
    master_ratio = float(master_points[:, 1].max() / -positions(master)[:, 1].min())
    true_ratio = float(true_points[:, 1].max() / -positions(true)[:, 1].min())
    assert 3.6 <= master_ratio <= 3.8, ("steel reference blade/hilt", master_ratio)
    assert 2.7 <= true_ratio <= 3.0, ("gold reference blade/hilt", true_ratio)
    assert master_points[:, 1].max() > 82 and true_points[:, 1].max() > 82, "reference blades shortened"
    master_root_ratio = width_at(master_points, 14.5) / width_at(master_points, 23)
    assert .70 <= master_root_ratio <= .75, ("Master widened blade root", master_root_ratio)
    assert .94 < width_at(master_points, 90) / width_at(master_points, 23) < 1.01, "Master body not nearly parallel"
    assert width_at(master_points, 103) < .65 * width_at(master_points, 94), "Master final tip taper lost"
    assert width_at(true_points, 10) < .7 * width_at(true_points, 14), "gold stepped shoulder lost"
    assert width_at(true_points, 62) < width_at(true_points, 19), "gold gentle taper lost"
    assert any(p["name"] == "Sculpted winged crossguard" for p in master.parts)
    assert not any(p["name"] == "Sculpted winged crossguard" for p in true.parts), "gold guard incorrectly uses steel wings"
    true_guard = {"True Master straight capped crossbar", "True Master rounded crossbar end cap"}
    true_guard_ratio = span(true, 0, true_guard) / true_points[:, 1].max()
    assert .33 <= true_guard_ratio <= .38, ("gold capped bar proportions", true_guard_ratio)
    assert all(p["mat"] == "white" for p in true.parts if p["name"] in true_guard)
    assert true.materials["gold"]["source_hex"] == "#E0A800"
    assert true.materials["blue_gem"]["source_hex"] == "#4870D0"

    gilded_blade = {"Continuous Gilded blade tang and seat cutting bevel",
                    "Continuous Gilded blade tang and seat fuller face",
                    "Forged blade inlay surround", "Tempered gold fitted diamond"}
    gilded_points = positions(gilded, gilded_blade)
    gilded_ratio = float(gilded_points[:, 1].max() / -positions(gilded)[:, 1].min())
    assert 3.6 <= gilded_ratio <= 3.85, ("Gilded shortened blade/hilt", gilded_ratio)
    assert gilded_points[:, 1].max() == 100, "Gilded approved review length changed"
    gilded_aspect = float(gilded_points[:, 1].max() / span(gilded, 0, gilded_blade))
    assert 9.0 <= gilded_aspect <= 9.6, ("Gilded shortened blade aspect", gilded_aspect)
    quillons = [p for p in gilded.parts if p["name"] == "Forged silver forked quillon"]
    assert len(quillons) == 2 and all(p["mat"] == "steel" for p in quillons), "Gilded silver quillons lost"
    gold_points = positions(gilded, {"Tempered gold fitted diamond"})
    diamond_coverage = []
    for y in (6 + (old_y - 6) * 94 / 121 for old_y in (26.125, 66.375, 106.75)):
        fraction = width_at(gold_points, y) / width_at(gilded_points, y)
        assert .87 <= fraction <= .94, ("Gilded broad gold diamond", y, fraction)
        diamond_coverage.append(fraction)

    fairy = models["great_fairy_sword"]
    fairy_blade = {"Rose-tempered fairy blade cutting bevel", "Rose-tempered fairy blade fuller face"}
    visible_blade = float(positions(fairy, fairy_blade)[:, 1].max())
    blade_aspect = visible_blade / span(fairy, 0, fairy_blade)
    assert abs(blade_aspect - 5091 / 900) < .03, ("Fairy visible blade aspect", blade_aspect)
    grip_ratio = span(fairy, 1, {"Shaped oval leather grip"}) / visible_blade
    assert abs(grip_ratio - 1148 / 5091) < .003, ("Fairy grip/blade proportions", grip_ratio)
    guard_ratio = span(fairy, 0, {"Lily-shaped fairy quillon"}) / visible_blade
    assert abs(guard_ratio - 1450 / 5091) < .003, ("Fairy guard/blade proportions", guard_ratio)
    assert visible_blade == positions(fairy, {"Domed lilac forged blade tip"})[:, 1].max(), "Fairy ornament detached"

    # Check actual source-generated faces, not just rendered silhouettes.
    for model in (master, true, gilded):
        for part in model.parts:
            p, normals, faces = part["p"], part["n"], part["tri"]
            assert np.isfinite(p).all() and np.isfinite(normals).all()
            face_normals = np.cross(p[faces[:, 1]] - p[faces[:, 0]], p[faces[:, 2]] - p[faces[:, 0]])
            assert np.linalg.norm(face_normals, axis=1).min() > 1e-7, (model.slug, part["name"], "collapsed surface")
            assert np.einsum("ij,ij->i", face_normals, normals[faces].sum(axis=1)).min() >= -1e-7, (model.slug, part["name"], "winding/normal disagreement")

    with tempfile.TemporaryDirectory(prefix="forged-proportions-") as directory:
        for slug, (glb_hash, resource_hash) in PROTECTED.items():
            actual = hashlib.sha256((ROOT / "CHECKPOINTS" / slug / (slug + ".glb")).read_bytes()).hexdigest()
            assert actual == glb_hash, (slug, "protected checkpoint changed")
            digest = hashlib.sha256()
            for path in sorted((REPO / "soh/assets/custom/objects/nei_gi_redesign" / slug).glob("*")):
                if path.is_file():
                    digest.update(path.name.encode())
                    digest.update(path.read_bytes())
            assert digest.hexdigest() == resource_hash, (slug, "protected native resources changed")
            if slug in sword_forged.BUILDERS:
                path = Path(directory) / (slug + ".glb")
                preview.glb(sword_forged.build(slug), path)
                assert hashlib.sha256(path.read_bytes()).hexdigest() == glb_hash, (slug, "protected authoring changed")

    return {"master_blade_to_hilt": master_ratio, "master_root_to_body_width": master_root_ratio,
            "true_blade_to_hilt": true_ratio,
            "true_bar_to_blade": float(true_guard_ratio), "gilded_blade_to_hilt": gilded_ratio,
            "gilded_blade_aspect": gilded_aspect, "gilded_gold_diamond_coverage": diamond_coverage,
            "fairy_visible_blade_aspect": blade_aspect, "fairy_grip_to_blade": grip_ratio,
            "unchanged_approved_models": len(PROTECTED)}


if __name__ == "__main__":
    print(json.dumps(check(), indent=2))
