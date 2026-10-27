#!/usr/bin/env python3
"""Inspect the compiled production Draw references; optionally resolve real hooks.

The geometry/lifetime executable omits Draw, so it cannot catch accidental
anonymous-namespace declarations introduced by OPEN_DISPS/CLOSE_DISPS.
"""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

HOOKS={'FrameInterpolation_RecordOpenChild','FrameInterpolation_RecordCloseChild'}

def unresolved(path):
    output=subprocess.check_output([os.environ.get('NM','nm'),'-u',str(path)],text=True)
    return {line.split()[-1] for line in output.splitlines() if line.split()}

def hook_references(symbols):
    return {symbol for symbol in symbols if any(hook in symbol for hook in HOOKS)}

if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('object',type=Path)
    parser.add_argument('--provider-object',type=Path)
    args=parser.parse_args()
    references=hook_references(unresolved(args.object))
    assert references==HOOKS, f'production Draw must reference the native C hooks, got {sorted(references)}'
    print('PASS complete valley object references unmangled native C interpolation hooks')
    if args.provider_object:
        definitions=subprocess.check_output([os.environ.get('NM','nm'),'--defined-only',str(args.provider_object)],text=True)
        defined={line.split()[-1] for line in definitions.splitlines() if line.split()}
        assert HOOKS<=defined, 'actual interpolation provider must define both C hooks'
        with tempfile.TemporaryDirectory(prefix='valley-linkage-') as td:
            linked=Path(td)/'valley-with-interpolation.o'
            subprocess.run([os.environ.get('CXX','c++'),'-r','-nostdlib',str(args.object),str(args.provider_object),'-o',str(linked)],check=True)
            assert not hook_references(unresolved(linked)), 'interpolation hooks unresolved after actual-object partial link'
        print('PASS actual interpolation provider resolves both valley Draw hooks in relocatable link')
