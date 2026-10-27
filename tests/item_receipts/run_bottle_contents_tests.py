#!/usr/bin/env python3
"""Every inventory bottle content needs an instructive localized receipt."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix='bottle-receipt-catalog-') as directory:
    binary = Path(directory) / 'test'
    subprocess.run([*shlex.split(os.environ.get('CXX','c++')), '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    '-I'+str(ROOT),str(ROOT/'tests/item_receipts/bottle_contents_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
