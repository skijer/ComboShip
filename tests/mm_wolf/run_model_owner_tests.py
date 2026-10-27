#!/usr/bin/env python3
"""Exercise MM PAK body ownership independently of its equipment activity."""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_mm_nei_tests import flags

source = (ROOT / 'mm/mods/pak_loader/pak_loader.cpp').read_text()
def body(name):
    match = re.search(r'^(?:extern "C" |static inline )?(?:u8|s32) ' + name + r'\(void\) \{.*?^}',
                      source, re.M | re.S)
    return match[0] if match else None
start = source.index('struct PakModel {')
end = source.index('\n};', start) + 3
owner = body('PakLoader_HasActiveBodyModel')
if owner is None:
    # Characterize the actual broad predicate previously used by Wolf.
    owner = body('PakLoader_HasActiveModel').replace('PakLoader_HasActiveModel', 'PakLoader_HasActiveBodyModel', 1)

with tempfile.TemporaryDirectory(prefix='mm-wolf-owner-') as temporary:
    build = Path(temporary)
    (build / 'pak-owner.inc').write_text(source[start:end] + '\n' +
                                       body('sGetActiveIndex') + '\n' +
                                       body('PakLoader_HasActiveModel') + '\n' + owner)
    binary = build / 'owner'
    result = subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', *flags(), '-I' + str(build),
                             '-O1', '-g', '-fsanitize=undefined', '-fno-sanitize-recover=all',
                             str(ROOT / 'tests/mm_wolf/model_owner_test.cpp'), '-o', str(binary)],
                            capture_output=True, text=True)
    if result.returncode:
        print(result.stdout + result.stderr)
        raise SystemExit(result.returncode)
    subprocess.run([str(binary)], check=True)
