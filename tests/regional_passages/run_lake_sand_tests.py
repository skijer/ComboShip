#!/usr/bin/env python3
"""Exercise the production surface partition independently of collision."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix='lake-sand-') as td:
    binary=Path(td)/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',
        '-I'+str(ROOT/'combo'),'-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',
        str(ROOT/'tests/regional_passages/lake_sand_test.cpp'),'-o',str(binary)],check=True)
    env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'}
    results=[subprocess.run([str(binary),case],env=env).returncode for case in ('sand','cap')]
    if any(results):raise SystemExit(1)
