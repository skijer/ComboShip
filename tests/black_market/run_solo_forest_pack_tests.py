#!/usr/bin/env python3
"""Verify the actual forest CLI emits reproducible archives with immutable donors."""
import argparse
import hashlib
import importlib.util
from pathlib import Path
import stat
import sys
import tempfile
from unittest.mock import patch
import zipfile

ROOT = Path(__file__).resolve().parents[2]


def digest(path):
    with path.open("rb") as source:
        return hashlib.file_digest(source, "sha256").hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--donor", type=Path, default=ROOT.parent / "forest-asset-recovery-work/zzzz_MM3D_Lost_Woods_Tiny_Right_POC9.o2r")
    parser.add_argument("--native-mm", type=Path, default=ROOT.parent / "native-inputs/mm(1).o2r")
    parser.add_argument("--native-oot", type=Path, default=ROOT.parent / "native-inputs/oot(3).o2r")
    args = parser.parse_args()
    inputs = [args.donor, args.native_mm, args.native_oot]
    before = [digest(path) for path in inputs]
    spec = importlib.util.spec_from_file_location("forest", ROOT / "tools/black_market/forest.py")
    forest = importlib.util.module_from_spec(spec); spec.loader.exec_module(forest)
    assert before[0] == forest.DONOR_SHA and before[1] == forest.NATIVE_SHA
    with tempfile.TemporaryDirectory(prefix="solo-forest-pack-") as temporary:
        directory = Path(temporary)
        outputs = [directory / "one.o2r", directory / "two.o2r"]
        contracts = [directory / "one.h", directory / "two.h"]
        clocks = [(2021, 4, 1, 2, 3, 4, 3, 91, 0), (2033, 11, 9, 19, 22, 50, 2, 313, 0)]
        for output, contract, clock in zip(outputs, contracts, clocks):
            command = [str(ROOT / "tools/black_market/forest.py"), *[str(path) for path in inputs],
                       str(output), "--contract", str(contract)]
            with patch.object(sys, "argv", command), patch.object(zipfile.time, "localtime", return_value=clock):
                forest.main()
        assert contracts[0].read_bytes() == contracts[1].read_bytes()
        with zipfile.ZipFile(outputs[0]) as first, zipfile.ZipFile(outputs[1]) as second:
            assert first.testzip() is None and second.testzip() is None
            assert first.namelist() == second.namelist() == sorted(first.namelist())
            assert len(first.namelist()) == 88 and len(set(first.namelist())) == 88
            for path in first.namelist():
                with first.open(path) as one, second.open(path) as two:
                    while chunk := one.read(1024 * 1024):
                        assert chunk == two.read(len(chunk)), f"payload changed between clocks: {path}"
                    assert not two.read(1)
        hashes = [digest(path) for path in outputs]
        assert hashes[0] == hashes[1], f"CLI archive bytes depend on clock metadata: {hashes}"
        with zipfile.ZipFile(outputs[0]) as archive:
            for entry in archive.infolist():
                assert entry.date_time == (1980, 1, 1, 0, 0, 0)
                assert entry.create_system == 3 and entry.external_attr >> 16 == (stat.S_IFREG | 0o644)
                assert entry.compress_type == zipfile.ZIP_DEFLATED and not entry.extra and not entry.comment
        # The CLI must still refuse to overwrite an existing candidate.
        with patch.object(sys, "argv", [str(ROOT / "tools/black_market/forest.py"),
                                        *[str(path) for path in inputs], str(outputs[0])]):
            try:
                forest.main()
            except ValueError as error:
                assert "new addon output" in str(error)
            else:
                raise AssertionError("CLI overwrote an existing candidate")
        assert digest(outputs[0]) == hashes[0]
        print(f"PASS actual CLI deterministic forest addon: 88 identical resources, exact SHA256 {hashes[0]}")
    assert before == [digest(path) for path in inputs], "an immutable donor changed"


if __name__ == "__main__":
    main()
