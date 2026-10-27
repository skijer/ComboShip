"""Attach the exact reviewed GI swords to native hands with resource matrices.

Only writes held display-list wrappers, their grip matrices, and a bounded
dependency manifest. The original GI meshes, materials and checkpoints stay
byte-for-byte intact. Native melee reach is a visual length target, never an
input to animation or collision. Four Sword retains its shipped X=3389 tip.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import xml.etree.ElementTree as ET

import numpy as np

ROOT = Path(__file__).resolve().parents[2]
SWORDS = {
    "kokiri_sword": (-14.5, 61., 3000.),
    "mm_kokiri_sword": (-13.5, 63., 3000.),
    "razor_sword": (-33.5, 56.5, 4000.),
    "gilded_sword": (-18.75, 93.625, 4000.),
    "master_sword": (-18.75, 106.375, 4000.),
    "true_master_sword": (-18.75, 106.375, 4000.),
    "biggoron_sword": (-25.75, 166., 5500.),
    "great_fairy_sword": (-21.90625, 154.5, 5500.),
    "four_sword": (-15.40625, 78., 3389.),
    "giants_knife": (-23.25, 152., 5500.),
    # Keep the intact Knife's grip and uniform scale; never stretch the stump.
    "broken_giants_knife": (-23.25, 56.375, (56.375 + 23.25) * 5500. / (152. + 23.25)),
}
FRAMES = {"oot_adult": (0., 328., -77.), "oot_child": (0., 216.22, 4.5),
          "mm_human": (0., 216.22, 4.5)}


def source_folder(slug):
    # A held-only fracture does not add or replace an item in the GI catalog.
    if slug == "broken_giants_knife":
        return "objects/nei_held_swords/broken_giants_knife/model/"
    return "objects/nei_gi_redesign/" + slug + "/"


def decode_matrix(raw):
    assert len(raw) == 128 and struct.unpack_from("<I", raw, 4)[0] == 0x4F4D5458
    words = struct.unpack_from("<16I", raw, 64)
    fixed = []
    for integer, fraction in zip(words[:8], words[8:]):
        fixed += [(integer & 0xffff0000) | (fraction >> 16),
                  ((integer << 16) & 0xffff0000) | (fraction & 0xffff)]
    return np.array([v if v < 2**31 else v - 2**32 for v in fixed]).reshape(4, 4) / 65536


def matrix_resource(matrix):
    header = bytearray(64)
    struct.pack_into("<IIIQQ", header, 0, 0, 0x4F4D5458, 0, 0xDEADBEEFDEADBEEF, 1 << 32)
    fixed = np.rint(matrix * 65536).astype(np.int64).reshape(-1, 2)
    integer = [(((int(a) >> 16) & 65535) << 16) | ((int(b) >> 16) & 65535) for a, b in fixed]
    fraction = [((int(a) & 65535) << 16) | (int(b) & 65535) for a, b in fixed]
    return bytes(header) + struct.pack("<16I", *(integer + fraction))


def build(output):
    assets = ROOT / "soh/assets/custom"
    output = Path(output)
    paths, graphs, report = [], [], {}
    for slug, (grip, tip, reach) in SWORDS.items():
        source = source_folder(slug)
        source_dl = source + "gi_dl"
        graph = sorted({source_dl, *(node.get("Path") for node in ET.parse(assets / source_dl).getroot()
                                      if node.get("Path"))})
        native = decode_matrix((assets / source / "scale_mtx").read_bytes())
        assert np.allclose(native, np.diag([native[0, 0]] * 3 + [1]))
        source_scale = native[0, 0] * 16
        frame_paths = []
        report[slug] = {"source": source_dl, "author_grip_y": grip, "author_tip_y": tip,
                        "source_author_to_gi_native": source_scale,
                        "source_sha256": {path: hashlib.sha256((assets / path).read_bytes()).hexdigest()
                                          for path in graph}, "frames": {}}
        for frame, socket in FRAMES.items():
            target = 3000. if slug == "razor_sword" and frame == "mm_human" else reach
            author_to_held = target / (tip - grip)
            uniform = author_to_held / source_scale
            # Row-vector N64 modelview: authored +Y -> native +X, +X -> -Y.
            transform = np.array([[0, -uniform, 0, 0], [uniform, 0, 0, 0],
                                  [0, 0, uniform, 0], [-grip * author_to_held, socket[1], socket[2], 1]], float)
            prefix = "objects/nei_held_swords/" + slug + "/" + frame + "/"
            folder = output / prefix
            folder.mkdir(parents=True, exist_ok=True)
            encoded = matrix_resource(transform)
            (folder / "grip_mtx").write_bytes(encoded)
            (folder / "held_dl").write_text(
                '<DisplayList Version="0">\n'
                f'<Matrix Path="{prefix}grip_mtx" Param="G_MTX_PUSH"/>\n'
                f'<CallDisplayList Path="{source_dl}"/>\n'
                '<PopMatrix Param="G_MTX_MODELVIEW"/>\n'
                '<EndDisplayList/>\n</DisplayList>\n')
            frame_paths.append((prefix + "held_dl", prefix + "grip_mtx"))
            report[slug]["frames"][frame] = {"socket": socket, "tip_x": target,
                                            "author_to_held": author_to_held,
                                            "gi_native_to_held_uniform": uniform,
                                            "row_matrix": transform.tolist(),
                                            "serialized_row_matrix": decode_matrix(encoded).tolist(),
                                            "runtime_tested": False}
        paths.append(frame_paths)
        graphs.append(graph)
    width = max(map(len, graphs)) + 1
    lines = ["// Generated by tools/nei_held/build_swords.py; exact GI dependencies only."]
    for suffix, owner in (("", ""), ("Base", "@oot-gi-base:")):
        prefix = "__OTR__" + owner
        lines.append("static const char* const sNeiHeldSword" + suffix +
                     "Paths[NEI_HELD_SWORD_COUNT][NEI_HELD_SWORD_FRAME_COUNT][2] = {")
        for slug, frames in zip(SWORDS, paths):
            lines.append("    { // " + slug)
            lines.extend('        { "' + prefix + dl + '", "' + prefix + matrix + '" },' for dl, matrix in frames)
            lines.append("    },")
        lines += ["};", "static const char* const sNeiHeldSword" + suffix +
                  "Resources[NEI_HELD_SWORD_COUNT][" + str(width) + "] = {"]
        for slug, graph in zip(SWORDS, graphs):
            lines.append("    { // " + slug)
            lines.extend('        "' + prefix + path + '",' for path in graph)
            lines.append("        NULL\n    },")
        lines.append("};\n")
    return "\n".join(lines), report


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=ROOT / "soh/assets/custom")
    parser.add_argument("--manifest", type=Path, default=ROOT / "combo/NeiHeldSwordResources.inc")
    parser.add_argument("--report", type=Path, default=ROOT / "tools/nei_held/SWORD_TRANSFORMS.json")
    args = parser.parse_args()
    manifest, report = build(args.output)
    args.manifest.write_text(manifest)
    args.report.write_text(json.dumps(report, indent=2) + "\n")
    print(f"Wrote {len(SWORDS) * len(FRAMES)} rigid held sword attachments; source mesh/material resources unchanged")
