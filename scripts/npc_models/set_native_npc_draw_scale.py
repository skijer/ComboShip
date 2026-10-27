#!/usr/bin/env python3
"""Add an optional visual DrawScale resource without changing model/pose bytes.

Requires the matching source feature to affect a draw; old builds ignore it.
The source archive is read-only and output metadata is deterministic.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import zipfile

from native_assets import o2r as asset

NAMESPACES = {"young-malon": "objects/object_ma1/NativeNpcV1", "saria": "objects/object_sa/NativeNpcV1",
              "young-fado": "objects/object_fa/YoungFadoNpcPOC1"}


def scale_resource(factor):
    if not math.isfinite(factor) or not .5 <= factor <= 2:
        raise ValueError("Draw scale must be finite and within 0.5–2.0")
    low, high = struct.unpack("<HH", struct.pack("<f", factor))
    words = (0x4e44, 1, low, high)
    return (asset._header(asset.T_OARR) + struct.pack("<II", 16, len(words)) +
            b"".join(struct.pack("<IH", 5, word) for word in words))


def set_draw_scale(source, output, model, factor=1.10):
    source, output = Path(source), Path(output)
    if source.resolve() == output.resolve():
        raise ValueError("Use a separate candidate output; in-place writes are not permitted")
    if output.exists():
        raise ValueError("Candidate output already exists")
    payload = scale_resource(factor)
    namespace = NAMESPACES[model]
    with zipfile.ZipFile(source) as archive:
        names = archive.namelist()
        if len(set(names)) != len(names):
            raise ValueError("Duplicate archive entries are not supported")
        records = {name: archive.read(name) for name in names}
    required = [namespace + "/Skel", "alt/" + namespace + "/Skel", "alt/" + namespace + "/Body/Limb14"]
    if any(name not in records for name in required):
        raise ValueError("Input does not contain the requested private native model")
    path = "alt/" + namespace + "/DrawScale"
    changed = [path] if records.get(path) != payload else []
    records[path] = payload
    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output, "x", compression=zipfile.ZIP_STORED) as archive:
        for name, value in sorted(records.items()):
            entry = zipfile.ZipInfo(name, (2026, 10, 20, 0, 0, 0))
            entry.create_system = 3
            entry.external_attr = 0o100644 << 16
            archive.writestr(entry, value)
    return {"source": str(source.resolve()), "source_sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
            "candidate": str(output.resolve()), "candidate_sha256": hashlib.sha256(output.read_bytes()).hexdigest(),
            "model": model, "uniform_draw_scale": factor, "resource": path, "changed_entries": changed,
            "preserved_entries": len(records) - len(changed), "requires_new_build": True,
            "runtime_tested": False, "accepted": False, "promoted": False}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--model", choices=NAMESPACES, required=True)
    parser.add_argument("--scale", type=float, default=1.10)
    args = parser.parse_args()
    try:
        result = set_draw_scale(args.input, args.output, args.model, args.scale)
    except (ValueError, OSError, zipfile.BadZipFile) as error:
        parser.error(str(error))
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
