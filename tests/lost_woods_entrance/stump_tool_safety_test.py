#!/usr/bin/env python3
"""The receipt output must never overwrite source assets or a produced archive."""
import importlib.util
from pathlib import Path
import sys
import tempfile
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('stump_tool', ROOT/'tools/relocate_mm3d_lost_woods_return.py')
tool = importlib.util.module_from_spec(spec)
spec.loader.exec_module(tool)
with tempfile.TemporaryDirectory(prefix='stump-cli-safety-') as temporary:
    root=Path(temporary); source=root/'baseline.o2r'; target=root/'candidate.o2r'; receipt=root/'receipt.json'
    for destination in (source,target,receipt):
        source.write_bytes(b'protected baseline'); target.write_bytes(b'produced archive'); receipt.write_text('previous evidence')
        if destination == target:
            target.unlink()  # It will not exist until a successful transformation.
        expected={p:(p.read_bytes() if p.exists() else None) for p in (source,target,receipt)}
        # Archive transformation is independently verified. Its return is the
        # boundary here: exercise the actual CLI write policy against real files.
        with patch.object(sys,'argv',['relocate',str(source),str(target),'--receipt',str(destination)]), \
             patch.object(tool,'relocate',return_value={'candidate':'verified'}) as mutation:
            try:
                tool.main()
            except ValueError:
                pass
            else:
                raise AssertionError('receipt aliases/existing evidence must be rejected before writing '+str(destination))
            assert not mutation.called, 'receipt policy must be checked before any archive mutation'
            for p,data in expected.items():
                assert (p.read_bytes() if p.exists() else None)==data, 'receipt changed a protected input/output: '+str(p)
    print('PASS stump CLI: source, candidate and previous receipt cannot be overwritten by receipt output')
