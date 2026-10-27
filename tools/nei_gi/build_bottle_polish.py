"""Serialize accepted bottle OBJ meshes without recoloring or resizing donor pixels.

Uses the established NEI resource writer: 1/256-unit positions, 32-entry RSP
loads, an internal scale matrix, and high-resolution RGBA32 resource metadata.
The accepted fitted OBJ/MTL/PNG/GLB files remain the immutable source checkpoint.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys
import tempfile

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
SOURCE = Path(__file__).resolve().parent / "SOURCE/bottle_polish_20261008"
sys.path.insert(0, str(Path(__file__).resolve().parent / "SOURCE"))
import meshkit

# Fitted contents are small enough for finer coordinates to remain signed-16.
meshkit.Q = 256

FILES = {"mushroom": "Magic_Mushroom", "princess": "princess", "seahorse": "seahorse"}
BASE = "objects/combo_bottle_gi/"


def digest(data):
    return hashlib.sha256(data).hexdigest()


def model(slug):
    path = SOURCE / "models" / slug / (FILES[slug] + ".obj")
    maps, material = {}, None
    for line in path.with_suffix(".mtl").read_text().splitlines():
        fields = line.split()
        if not fields:
            continue
        if fields[0] == "newmtl":
            material = fields[1]
        elif fields[0] == "map_Kd":
            maps[material] = path.parent / " ".join(fields[1:])
    positions, normals, uvs, groups = [], [], [], {}
    for line in path.read_text().splitlines():
        fields = line.split()
        if not fields:
            continue
        if fields[0] == "v":
            positions.append([float(x) for x in fields[1:4]])
        elif fields[0] == "vn":
            normals.append([float(x) for x in fields[1:4]])
        elif fields[0] == "vt":
            uvs.append([float(x) for x in fields[1:3]])
        elif fields[0] == "usemtl":
            material = fields[1]
        elif fields[0] == "f":
            refs = [tuple(int(x) for x in token.split("/")) for token in fields[1:]]
            for i in range(1, len(refs) - 1):
                groups.setdefault(material, []).append([refs[0], refs[i], refs[i + 1]])
    result = meshkit.Model(slug, slug, BASE + "polish/" + slug + "/gi_dl", 1, effective_scale=1)
    result.prefix = BASE + "polish/" + slug + "/"
    for name, faces in groups.items():
        pixels = np.asarray(Image.open(maps[name]).convert("RGBA")).copy()
        result.material(name, (1, 1, 1), tex=pixels)
        # Preserve source winding, normals and fitted geometry. OBJ V is bottom
        # origin; native textures are top origin, so convert UVs, never pixels.
        unique = dict.fromkeys(ref for face in faces for ref in face)
        for index, ref in enumerate(unique):
            unique[ref] = index
        refs = np.asarray(list(unique))
        result.parts.append(dict(name=name, mat=name,
                                 p=np.asarray(positions)[refs[:, 0] - 1],
                                 n=meshkit.unit(np.asarray(normals)[refs[:, 2] - 1]),
                                 uv=np.asarray(uvs)[refs[:, 1] - 1] * [1, -1] + [0, 1],
                                 tri=np.asarray([[unique[ref] for ref in face] for face in faces])))
    return result


def build(repo):
    approved = json.loads((SOURCE / "approved-inputs.json").read_text())
    for name, expected in approved["models"].items():
        assert digest((SOURCE / name).read_bytes()) == expected, (name, "accepted source changed")
    with tempfile.TemporaryDirectory(prefix="bottle-polish-assets-") as temporary:
        meshkit.ROOT = Path(temporary)
        stats = {}
        for slug in FILES:
            stats[slug] = meshkit.export_resources(model(slug))
            entry = Path(temporary) / "RESOURCES" / (BASE + "polish/" + slug + "/gi_dl")
            text = entry.read_text()
            # Match the accepted texture addressing: donors repeat; mushroom
            # has a vertical atlas and retains its clamped T edge.
            if slug == "princess":
                # Both preserved donor atlases have transparent pixels (and the
                # eye atlas has partial alpha). Preserve those pixels with XLU
                # blending; the enclosing host composer chooses the same pass.
                text = text.replace("G_RM_AA_ZB_OPA_SURF2", "G_RM_AA_ZB_XLU_SURF2")
            if slug == "mushroom":
                text = text.replace('CMT_TXWrap="1"', 'CMT_TXClamp="1"')
            entry.write_text(text)
        resources = {str(p.relative_to(Path(temporary) / "RESOURCES")): p.read_bytes()
                     for p in (Path(temporary) / "RESOURCES").rglob("*") if p.is_file()}
    # A casing-only marker is also the safe foreign fallback, matching the
    # unchanged mushroom/princess/gold marker contract from the parent candidate.
    resources[BASE + "SeahorseBottle"] = (ROOT / "soh/assets/custom" / (BASE + "MushroomBottle")).read_bytes()
    for host in ("soh", "mm"):
        for name, data in sorted(resources.items()):
            dest = repo / host / "assets/custom" / name
            dest.parent.mkdir(parents=True, exist_ok=True)
            dest.write_bytes(data)
    report = {"checkpoint": approved["checkpoint"], "models": stats,
              "resources": {name: digest(data) for name, data in sorted(resources.items())},
              "position_quantization": "nearest 1/256 GI unit; no extra fit transform",
              "pixels": "original RGBA pixels; OBJ V converted to native top origin",
              "runtime": "untested"}
    dest = repo / "tools/nei_gi/SOURCE/bottle_polish_20261008/resources.json"
    dest.parent.mkdir(parents=True, exist_ok=True)
    dest.write_text(json.dumps(report, indent=2) + "\n")
    print("PASS", len(resources), "identical bottle-polish resources per host", stats)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", type=Path, default=ROOT)
    build(parser.parse_args().repo)
