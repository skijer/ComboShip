#!/usr/bin/env python3
"""Exercise clean-build expansion of the actual bundled HD Chateau resource."""
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "scripts/assets/prepare_chateau_romani.py"
assert SCRIPT.is_file(), "Stock Chateau preparation step is missing"
RESOURCE = "objects/combo_chateau_romani_3ds/AtlasTex"
metadata = json.loads((ROOT / "tools/chateau_romani_3ds/bundled/manifest.json").read_text())
with tempfile.TemporaryDirectory(prefix="chateau-bundled-") as temporary:
    output = Path(temporary)
    command = [sys.executable, "-B", str(SCRIPT), "--output-root", str(output)]
    subprocess.run(command, check=True)
    paths = [output / game / "assets/custom" / RESOURCE for game in ("soh", "mm")]
    for path in paths:
        raw = path.read_bytes()
        assert hashlib.sha256(raw).hexdigest() == metadata["resource_sha256"]
        assert struct.unpack_from("<I", raw, 8)[0] == 1
        assert struct.unpack_from("<IIIIffI", raw, 64) == (1, 4096, 4096, 1, 64., 64., 67108864)
    assert paths[0].read_bytes() == paths[1].read_bytes()
    before = [path.stat().st_mtime_ns for path in paths]
    subprocess.run(command, check=True)
    assert before == [path.stat().st_mtime_ns for path in paths], "Matching assets were rewritten"
    sys.path.insert(0, str(ROOT / "scripts/assets"))
    from prepare_chateau_romani import read_parts, prepare_source_art
    art = prepare_source_art(output / "Atlas.png")
    assert hashlib.sha256(art.read_bytes()).hexdigest() == json.loads(
        (ROOT / "tools/chateau_romani_3ds/hd/source-parts/manifest.json").read_text())["sha256"]
    broken = output / "broken.zlib"
    raw = bytearray(read_parts(ROOT / "tools/chateau_romani_3ds/bundled", metadata))
    raw[len(raw) // 2] ^= 1
    broken.write_bytes(raw)
    failure = subprocess.run([*command, "--packed", str(broken)], capture_output=True, text=True)
    assert failure.returncode != 0, "Corrupt source resource was accepted"
    for path in paths:
        assert hashlib.sha256(path.read_bytes()).hexdigest() == metadata["resource_sha256"]
    print("PASS: exact 4K native resource in both clean asset trees; idempotent; corruption preserves outputs")
