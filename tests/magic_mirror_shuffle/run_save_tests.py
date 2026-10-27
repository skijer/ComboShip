"""Exercise the full native MM save JSON conversion against legacy/new/future arrays."""
from pathlib import Path
import os
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix='mirror-shuffle-save-') as td:
    binary=Path(td)/'test'
    flags=['-std=c++20','-O1','-w','-DCOMBO_BUILD','-DF3DEX_GBI_2','-DCONTROLLERBUTTONS_T=uint32_t','-DFMT_HEADER_ONLY']
    for p in ['', 'mm','mm/2s2h','mm/include','mm/include/PR','mm/src','mm/assets','libultraship/include','combo']:
        flags+=['-I'+str(ROOT/p)]
    result=subprocess.run([os.environ.get('CXX','c++'),*flags,str(ROOT/'tests/magic_mirror_shuffle/mm_save_test.cpp'),'-o',str(binary)],capture_output=True,text=True)
    if result.returncode: raise RuntimeError(result.stdout+result.stderr)
    subprocess.run([str(binary)],check=True)
