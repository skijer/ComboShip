"""Execute the active ComboShip combined fill, playthrough, requiredness and cross-dungeon path."""
from pathlib import Path
import os
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix='mirror-active-route-') as td:
    binary=Path(td)/'active'
    result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-O1','-I'+str(ROOT),'-I'+str(ROOT/'combo'),
        str(ROOT/'tests/magic_mirror_shuffle/active_test.cpp'),'-o',str(binary)],capture_output=True,text=True)
    if result.returncode:raise RuntimeError(result.stdout+result.stderr)
    result=subprocess.run([str(binary)],capture_output=True,text=True,timeout=120,cwd=td)
    if result.returncode:raise RuntimeError(result.stdout[-8000:]+result.stderr)
    print(result.stdout.strip().splitlines()[-1])
