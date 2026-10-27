#!/usr/bin/env python3
"""Exercise the real RAM adapter using user-supplied Djipi vertices (no bundled fixture)."""
from pathlib import Path
import argparse, os, struct, subprocess, sys, tempfile
import xml.etree.ElementTree as E

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/black_market'))
from build import archive

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--maps', type=Path, required=True)
parser.add_argument('--stormlib', type=Path, required=True)
args = parser.parse_args()
assert (ROOT / 'combo/BlackMarketGranny.h').exists(), 'Missing external-only Granny room adapter'
resources = archive(args.maps, args.stormlib)
rows = E.fromstring(resources['alt/scenes/shared/mahouya_scene/mahouya_room_0DL_006548_vtx_2'])
with tempfile.TemporaryDirectory(prefix='external-granny-') as temp:
    temp = Path(temp)
    vertices = temp/'donor-vertices.bin'
    vertices.write_bytes(b''.join(struct.pack('<3hH2h4B',
        *[int(v.get(k)) for k in ('X','Y','Z')], 0,
        *[int(v.get(k)) for k in ('S','T','R','G','B','A')]) for v in rows))
    binary = temp/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-DF3DEX_GBI_2','-g',
        '-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',
        '-I'+str(ROOT),'-I'+str(ROOT/'libultraship/include'),
        str(ROOT/'tests/black_market/granny_room_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(vertices)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
