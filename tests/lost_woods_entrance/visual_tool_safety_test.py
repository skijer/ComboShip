#!/usr/bin/env python3
"""Failed repair must release its receipt before Windows removes the file."""
import importlib.util
from pathlib import Path
import sys
import tempfile
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('forest_repair',ROOT/'tools/repair_mm_lost_woods_visuals.py')
tool = importlib.util.module_from_spec(spec)
spec.loader.exec_module(tool)
with tempfile.TemporaryDirectory(prefix='forest-repair-safety-') as temporary:
    root = Path(temporary)
    source, target, receipt = [root/n for n in ('source.o2r','output.o2r','receipt.json')]
    source.write_bytes(b'protected wrong baseline')
    real_open, real_unlink = Path.open, Path.unlink
    streams = []

    def tracked_open(path,*args,**kwargs):
        stream = real_open(path,*args,**kwargs)
        streams.append((path.resolve(),stream))
        return stream

    def windows_unlink(path,*args,**kwargs):
        if any(p == path.resolve() and not stream.closed for p,stream in streams):
            raise PermissionError('Windows cannot unlink an open receipt')
        return real_unlink(path,*args,**kwargs)

    with patch.object(sys,'argv',['repair',str(source),str(target),'--receipt',str(receipt)]), \
         patch.object(Path,'open',tracked_open), patch.object(Path,'unlink',windows_unlink):
        try:
            tool.main()
        except ValueError as error:
            assert 'exact Stump Return POC3 baseline' in str(error)
        else:
            raise AssertionError('wrong baseline must be rejected')
    assert source.read_bytes() == b'protected wrong baseline'
    assert not target.exists() and not receipt.exists()
    print('PASS repair failure: original error retained, receipt closed before Windows cleanup, input unchanged')
