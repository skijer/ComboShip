#!/usr/bin/env python3
"""Expand the exact checked-in Chateau atlas before either port archive is packed."""
import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import tempfile
import zlib

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "tools/chateau_romani_3ds"
RESOURCE = "objects/combo_chateau_romani_3ds/AtlasTex"
PART_BYTES = 2 * 1024 * 1024


def digest(path):
    result = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            result.update(block)
    return result.hexdigest()


def write_bundle(resource):
    packed = zlib.compress(resource, 9)
    directory = SOURCE / "bundled"
    directory.mkdir(parents=True, exist_ok=True)
    metadata = {"resource": RESOURCE, "encoding": "zlib",
                "resource_bytes": len(resource), "resource_sha256": hashlib.sha256(resource).hexdigest(),
                "packed_bytes": len(packed), "packed_sha256": hashlib.sha256(packed).hexdigest()}
    metadata["parts"] = write_parts(packed, directory, "AtlasTex.zlib")
    (directory / "manifest.json").write_text(json.dumps(metadata, indent=2) + "\n")


def write_parts(data, directory, name):
    parts = []
    for old in directory.glob(name + ".[0-9][0-9][0-9]*"):
        old.unlink()
    for index, offset in enumerate(range(0, len(data), PART_BYTES)):
        content = data[offset:offset + PART_BYTES]
        filename = f"{name}.{index:03d}.b64"
        (directory / filename).write_bytes(base64.b64encode(content))
        parts.append({"file": filename, "encoding": "base64", "bytes": len(content),
                      "sha256": hashlib.sha256(content).hexdigest()})
    return parts


def read_parts(directory, metadata):
    result = []
    for part in metadata["parts"]:
        content = (directory / part["file"]).read_bytes()
        if part.get("encoding") == "base64":
            content = base64.b64decode(content, validate=True)
        if len(content) != part["bytes"] or hashlib.sha256(content).hexdigest() != part["sha256"]:
            raise ValueError("Bundled Chateau source part failed its hash check")
        result.append(content)
    return b"".join(result)


def prepare_source_art(output=None):
    directory = SOURCE / "hd/source-parts"
    metadata = json.loads((directory / "manifest.json").read_text())
    output = output or SOURCE / "hd/Atlas-4K.png"
    if output.is_file() and digest(output) == metadata["sha256"]:
        return output
    data = read_parts(directory, metadata)
    if len(data) != metadata["bytes"] or hashlib.sha256(data).hexdigest() != metadata["sha256"]:
        raise ValueError("Bundled Chateau source artwork failed its hash check")
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(data)
    return output


def prepare(output_root, packed_path):
    metadata = json.loads((SOURCE / "bundled/manifest.json").read_text())
    expected = json.loads((SOURCE / "manifest.json").read_text())["resources"][RESOURCE]
    if metadata["resource"] != RESOURCE or metadata["resource_sha256"] != expected:
        raise ValueError("Bundled Chateau resource does not match the model manifest")
    if metadata["resource_bytes"] != 67108956:
        raise ValueError("Unexpected 4K Chateau resource size")
    packed = packed_path.read_bytes() if packed_path else read_parts(SOURCE / "bundled", metadata)
    if len(packed) != metadata["packed_bytes"] or hashlib.sha256(packed).hexdigest() != metadata["packed_sha256"]:
        raise ValueError("Bundled Chateau compressed resource failed its hash check")
    decoder = zlib.decompressobj()
    resource = decoder.decompress(packed, metadata["resource_bytes"] + 1)
    if (len(resource) != metadata["resource_bytes"] or not decoder.eof or decoder.unused_data or
            decoder.unconsumed_tail or hashlib.sha256(resource).hexdigest() != expected):
        raise ValueError("Bundled Chateau decoded resource failed its size/hash check")
    for game in ("soh", "mm"):
        target = output_root / game / "assets/custom" / RESOURCE
        if target.is_file() and digest(target) == expected:
            continue
        target.parent.mkdir(parents=True, exist_ok=True)
        temporary = None
        try:
            with tempfile.NamedTemporaryFile(dir=target.parent, prefix=".chateau-", delete=False) as stream:
                temporary = Path(stream.name)
                stream.write(resource)
            os.replace(temporary, target)
        finally:
            if temporary is not None:
                temporary.unlink(missing_ok=True)
    print("PASS: bundled 4K Chateau atlas prepared for soh.o2r and 2ship.o2r")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-root", type=Path, default=ROOT)
    parser.add_argument("--packed", type=Path)
    args = parser.parse_args()
    prepare(args.output_root, args.packed)
